// Headless host test for one LGN suite plugin (compiled once per plugin).
// Plays a music-like test signal at several sample rates and block sizes, fuzzes every parameter, checks for
// NaN/inf and runaway levels, round-trips state, then renders the editor to PNG.
// Usage: <plugin>_host [snapshot dir]
#include <juce_audio_utils/juce_audio_utils.h>
#include "../common/Suite.h"
#include <iostream>
#include <chrono>
#include <map>

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter();

namespace {
int failures = 0;
void check(bool ok, const juce::String& what) { if (!ok) { failures++; std::cout << "  FAIL " << what << "\n"; } }

// pink-ish noise, a chord, and drum-like bursts: enough to exercise dynamics, EQ and time effects
struct Signal {
    double sr = 48000; juce::Random rng{ 7 }; double ph[4] = {}; double b0 = 0, b1 = 0, b2 = 0; long long t = 0;
    void fill(juce::AudioBuffer<float>& buf, int n, float level = 0.25f) {
        const double f[4] = { 110, 220 * 1.189, 220 * 1.498, 880 * 1.122 };
        for (int i = 0; i < n; i++, t++) {
            const double w = rng.nextFloat() * 2 - 1;
            b0 = 0.99765 * b0 + w * 0.0990460; b1 = 0.96300 * b1 + w * 0.2965164; b2 = 0.57000 * b2 + w * 1.0526913;
            const double pink = (b0 + b1 + b2 + w * 0.1848) * 0.11;
            double tone = 0; for (int k = 0; k < 4; k++) { ph[k] += f[k] / sr; tone += std::sin(juce::MathConstants<double>::twoPi * ph[k]) * (k == 3 ? 0.08 : 0.18); }
            const double beat = std::fmod((double)t / sr, 0.5);
            const double hit = std::exp(-beat * 30) * (rng.nextFloat() * 2 - 1) * 0.9 + std::exp(-beat * 12) * std::sin(juce::MathConstants<double>::twoPi * 55 * beat) * 0.8;
            const double s = level * (pink * 0.6 + tone + hit);
            for (int c = 0; c < buf.getNumChannels(); c++) buf.setSample(c, i, (float)(s * (c ? 0.93 : 1.0) + (c ? 0.02 * pink : 0)));
        }
    }
};

bool finiteAndSane(const juce::AudioBuffer<float>& b, int n, float limit, float& peak) {
    for (int c = 0; c < b.getNumChannels(); c++) for (int i = 0; i < n; i++) {
        const float x = b.getSample(c, i);
        if (!std::isfinite(x)) return false;
        peak = juce::jmax(peak, std::abs(x));
    }
    return peak < limit;
}

// A kick on every beat at 120 BPM, written to channels from `first` on (a sidechain bus, when there is one).
struct KickTrack {
    double sr = 48000; long long t = 0;
    void fill(juce::AudioBuffer<float>& b, int first, int n) {
        for (int i = 0; i < n; i++, t++) {
            const double ph = std::fmod((double)t / sr, 0.5);
            const double v = 0.8 * std::exp(-ph * 18) * std::sin(juce::MathConstants<double>::twoPi * (45 + 120 * std::exp(-ph * 40)) * ph);
            for (int c = first; c < b.getNumChannels(); c++) b.setSample(c, i, (float)v);
        }
    }
};
// For instruments: an E major / A minor chord every half second, held for 0.4 s, plus a short melody line.
struct ChordMidi {
    double sr = 48000; long long t = 0;
    void fill(juce::MidiBuffer& m, int n) {
        static const int chords[2][6] = { { 40, 47, 52, 56, 59, 64 }, { 45, 52, 57, 60, 64, 69 } };
        const long long period = (long long)(0.5 * sr), hold = (long long)(0.4 * sr);
        for (int i = 0; i < n; i++, t++) {
            const long long ph = t % period, k = t / period;
            const int* c = chords[k % 2];
            if (ph == 0) for (int j = 0; j < 6; j++) m.addEvent(juce::MidiMessage::noteOn(1, c[j], (juce::uint8)(70 + (k * 13 + j * 7) % 50)), i);
            if (ph == hold) for (int j = 0; j < 6; j++) m.addEvent(juce::MidiMessage::noteOff(1, c[j]), i);
            if (ph == period / 2 && k % 3 == 2) m.addEvent(juce::MidiMessage::noteOn(1, 76 + (int)(k % 5), (juce::uint8)100), i);
            if (ph == period - 10 && k % 3 == 2) m.addEvent(juce::MidiMessage::noteOff(1, 76 + (int)(k % 5)), i);
        }
    }
};
// A transport that plays at 120 BPM.
struct TestHead : juce::AudioPlayHead {
    double ppq = 0; bool playing = true;
    juce::Optional<PositionInfo> getPosition() const override { PositionInfo i; i.setBpm(120.0); i.setIsPlaying(playing); i.setPpqPosition(ppq); return i; }
};

double run(juce::AudioProcessor& p, double sr, int chans, std::function<int()> blockSize, double seconds, bool fuzz, float& peak) {
    auto lay = p.getBusesLayout();
    if (!lay.inputBuses.isEmpty()) lay.inputBuses.getReference(0) = chans == 1 ? juce::AudioChannelSet::mono() : juce::AudioChannelSet::stereo();
    lay.outputBuses.getReference(0) = chans == 1 ? juce::AudioChannelSet::mono() : juce::AudioChannelSet::stereo();
    check(p.setBusesLayout(lay), "layout " + juce::String(chans) + " ch");
    p.prepareToPlay(sr, 1024);
    Signal sig; sig.sr = sr; juce::Random rng{ 3 }; KickTrack kick; kick.sr = sr; ChordMidi notes; notes.sr = sr;
    const int bufCh = juce::jmax(p.getTotalNumInputChannels(), p.getTotalNumOutputChannels());   // includes any sidechain
    juce::AudioBuffer<float> buf(bufCh, 1024); juce::MidiBuffer midi;
    const long long N = (long long)(seconds * sr); long long done = 0; double busy = 0;
    auto& params = p.getParameters();
    while (done < N) {
        const int n = (int)std::min<long long>(blockSize(), N - done);
        if (fuzz) for (auto* prm : params) if (rng.nextFloat() < 0.15f) prm->setValueNotifyingHost(rng.nextFloat());
        sig.fill(buf, n);
        if (bufCh > chans) kick.fill(buf, chans, n);
        midi.clear(); if (p.acceptsMidi()) notes.fill(midi, n);
        juce::AudioBuffer<float> view(buf.getArrayOfWritePointers(), bufCh, n);
        const auto t0 = std::chrono::steady_clock::now();
        p.processBlock(view, midi);
        busy += std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count();
        float pk = 0;
        if (!finiteAndSane(view, n, fuzz ? 64.0f : 8.0f, pk)) { check(false, "output not finite or runaway at " + juce::String(sr) + " Hz, peak " + juce::String(pk)); p.releaseResources(); return -1; }
        peak = juce::jmax(peak, pk);
        done += n;
    }
    p.releaseResources();
    return busy / seconds;
}

void setAll(juce::AudioProcessor& p, float v) { for (auto* prm : p.getParameters()) prm->setValueNotifyingHost(v); }
void setDefaults(juce::AudioProcessor& p) { for (auto* prm : p.getParameters()) prm->setValueNotifyingHost(prm->getDefaultValue()); }

// Benchmark: every preset, 6 s each at 48 kHz / 512, with a running transport and a kick on any sidechain.
// Prints CPU per preset (best of 3 runs) and writes the exact output to `outFile` so builds can be compared.
// Demo renders (instruments): every preset plays a short phrase (strummed chords, or a melody for legato presets)
// and is written as a WAV into `dir`.
int demos(const juce::String& dir) {
    std::unique_ptr<juce::AudioProcessor> p(createPluginFilter());
    auto* lp = dynamic_cast<lgn::Processor*>(p.get());
    if (!lp || !p->acceptsMidi()) return 1;
    const double sr = 48000, bpm = 90, beat = 60.0 / bpm; const int B = 256;
    juce::File out(juce::File::getCurrentWorkingDirectory().getChildFile(dir)); out.createDirectory();
    struct Ev { double t; int note; bool on; };
    std::vector<Ev> chords, melody;
    const std::vector<std::vector<int>> prog = { { 43, 47, 50, 55, 59, 67 }, { 40, 47, 52, 55, 59, 64 }, { 48, 52, 55, 60, 64 }, { 50, 57, 62, 66 } };
    const double pat[] = { 0, 1, 1.5, 2.5, 3, 3.5 };
    for (int bar = 0; bar < 4; bar++) for (double q : pat) for (int n : prog[(size_t)bar]) { const double t = bar * 4 * beat + q * beat; chords.push_back({ t, n, true }); chords.push_back({ t + beat * 0.45, n, false }); }
    for (int n : prog[0]) { chords.push_back({ 16 * beat, n, true }); chords.push_back({ 19 * beat, n, false }); }
    const int mel[] = { 64, 67, 69, 71, 74, 71, 69, 67, 69, 71, 72, 71, 69, 67, 64, 67 };
    for (int k = 0; k < 16; k++) { const double t = k * beat * 0.5 + (k >= 8 ? beat : 0); melody.push_back({ t, mel[k], true }); melody.push_back({ t + beat * (k == 7 || k == 15 ? 1.4 : 0.48), mel[k], false }); }
    for (auto* v : { &chords, &melody }) std::sort(v->begin(), v->end(), [](const Ev& a, const Ev& b) { return a.t < b.t || (a.t == b.t && !a.on && b.on); });
    for (int i = 1; i < lp->getNumPrograms(); i++) {
        lp->setCurrentProgram(i);
        const bool legato = juce::roundToInt(lp->p("mode")) == 2;
        const auto& ev = legato ? melody : chords;
        p->setRateAndBufferSizeDetails(sr, B); TestHead head; head.playing = true; p->setPlayHead(&head); p->prepareToPlay(sr, B);
        const int N = (int)(sr * (legato ? 10 * beat : 21 * beat));
        juce::AudioBuffer<float> all(2, N), buf(2, B); juce::MidiBuffer midi; size_t e = 0;
        for (int o = 0; o < N; o += B) {
            const int m = std::min(B, N - o); midi.clear();
            while (e < ev.size() && ev[e].t * sr < o + m) { const int off = std::max(0, (int)(ev[e].t * sr) - o); midi.addEvent(ev[e].on ? juce::MidiMessage::noteOn(1, ev[e].note, (juce::uint8)96) : juce::MidiMessage::noteOff(1, ev[e].note), off); e++; }
            juce::AudioBuffer<float> view(buf.getArrayOfWritePointers(), 2, m);
            p->processBlock(view, midi); head.ppq += m * bpm / 60.0 / sr;
            for (int c = 0; c < 2; c++) all.copyFrom(c, o, view, c, 0, m);
        }
        p->releaseResources(); p->setPlayHead(nullptr);
        const auto name = lp->presets()[(size_t)i].category + " - " + lp->getProgramName(i);
        auto f = out.getChildFile(juce::String(i).paddedLeft('0', 2) + " " + name.replaceCharacters("/&", "-+") + ".wav"); f.deleteFile();
        std::unique_ptr<juce::OutputStream> os(f.createOutputStream().release());
        juce::WavAudioFormat wav;
        if (auto* w = wav.createWriterFor(os.get(), sr, 2, 24, {}, 0)) { os.release(); std::unique_ptr<juce::AudioFormatWriter> wr(w); wr->writeFromAudioSampleBuffer(all, 0, N); }
        std::cout << "  " << f.getFileName() << "\n";
    }
    return 0;
}


// Loudness of every instrument preset (BS.1770-style K-weighting, gated), playing a phrase that suits it:
// a held note or chord for pattern presets, a melody in the bass or lead register otherwise.
int levels(const juce::String& outFile, const juce::String& wavDir = {}) {
    std::unique_ptr<juce::AudioProcessor> p(createPluginFilter());
    auto* lp = dynamic_cast<lgn::Processor*>(p.get());
    if (!lp || !p->acceptsMidi()) return 1;
    const double sr = 48000, bpm = 120, beat = 60.0 / bpm; const int B = 256;
    juce::File f(juce::File::getCurrentWorkingDirectory().getChildFile(outFile)); f.deleteFile(); juce::FileOutputStream os(f);
    for (int i = 1; i < lp->getNumPrograms(); i++) {
        lp->setCurrentProgram(i);
        const auto cat = lp->presets()[(size_t)i].category;
        const int pat = lp->apvts.getParameter("patmode") ? juce::roundToInt(lp->p("patmode")) : 0;
        struct Ev { double t; int note; bool on; }; std::vector<Ev> ev;
        const bool high = cat.startsWith("Lead") || cat == "Pluck" || cat == "Arp";
        if (pat == 2) for (int n : { 48, 52, 55 }) { ev.push_back({ 0, n, true }); ev.push_back({ 15.5 * beat, n, false }); }
        else if (pat > 0 || cat == "FX" || (lp->apvts.getParameter("aa") && lp->p("aa") > 300)) { ev.push_back({ 0, high ? 60 : 36, true }); ev.push_back({ 15.5 * beat, high ? 60 : 36, false }); }
        else { const int mel[] = { 0, 0, 3, 7, 5, 3, 0, -2 }; for (int r = 0; r < 2; r++) for (int k = 0; k < 8; k++) { const double t = (r * 8 + k) * beat; ev.push_back({ t, (high ? 60 : 36) + mel[k], true }); ev.push_back({ t + 0.75 * beat, (high ? 60 : 36) + mel[k], false }); } }
        std::sort(ev.begin(), ev.end(), [](const Ev& a, const Ev& b) { return a.t < b.t || (a.t == b.t && !a.on && b.on); });
        // measured before the output limiter, so the peak is the real one
        if (auto* lim = lp->apvts.getParameter("limiter"); lim && wavDir.isEmpty()) lim->setValueNotifyingHost(0.0f);
        p->setRateAndBufferSizeDetails(sr, B); TestHead head; p->setPlayHead(&head); p->prepareToPlay(sr, B);
        const int N = (int)(sr * 17 * beat);
        juce::AudioBuffer<float> buf(2, B), all(2, N); juce::MidiBuffer midi; size_t e = 0;
        lgn::Svf kw[2][2]; for (auto& c : kw) { c[0].set(lgn::Svf::highShelf, sr, 1681, 0.71, 4.0); c[1].set(lgn::Svf::highpass, sr, 38, 0.5); }
        std::vector<double> blocks; double acc = 0; int accN = 0; float peak = 0;
        for (int o = 0; o < N; o += B) {
            const int m = std::min(B, N - o); midi.clear();
            while (e < ev.size() && ev[e].t * sr < o + m) { const int off = std::max(0, (int)(ev[e].t * sr) - o); midi.addEvent(ev[e].on ? juce::MidiMessage::noteOn(1, ev[e].note, (juce::uint8)100) : juce::MidiMessage::noteOff(1, ev[e].note), off); e++; }
            juce::AudioBuffer<float> view(buf.getArrayOfWritePointers(), 2, m);
            p->processBlock(view, midi); head.ppq += m * bpm / 60.0 / sr;
            for (int c = 0; c < 2; c++) all.copyFrom(c, o, view, c, 0, m);
            for (int k = 0; k < m; k++) for (int c = 0; c < 2; c++) { const double x = view.getSample(c, k); peak = std::max(peak, (float)std::abs(x)); const double y = kw[c][1].process(kw[c][0].process(x)); acc += y * y; }
            accN += m; if (accN >= (int)(0.4 * sr)) { blocks.push_back(acc / accN); acc = 0; accN = 0; }
        }
        p->releaseResources(); p->setPlayHead(nullptr);
        if (wavDir.isNotEmpty()) {   // the render, through the limiter as a listener would hear it
            juce::File d(juce::File::getCurrentWorkingDirectory().getChildFile(wavDir)); d.createDirectory();
            for (int c = 0; c < 2; c++) for (int k = 0; k < N; k++) all.setSample(c, k, juce::jlimit(-0.944f, 0.944f, all.getSample(c, k)));
            auto wf = d.getChildFile(juce::String(i).paddedLeft('0', 3) + " " + (cat + " - " + lp->getProgramName(i)).replaceCharacters("/&", "-+") + ".wav"); wf.deleteFile();
            std::unique_ptr<juce::OutputStream> ws(wf.createOutputStream().release()); juce::WavAudioFormat wav;
            if (auto* w = wav.createWriterFor(ws.get(), sr, 2, 24, {}, 0)) { ws.release(); std::unique_ptr<juce::AudioFormatWriter> wr(w); wr->writeFromAudioSampleBuffer(all, 0, N); }
        }
        auto lufs = [](double ms) { return -0.691 + 10 * std::log10(ms + 1e-20); };
        double s1 = 0; int n1 = 0; for (double b : blocks) if (lufs(b) > -70) { s1 += b; n1++; }
        const double rel = n1 ? lufs(s1 / n1) - 10 : -70; double s2 = 0; int n2 = 0;
        for (double b : blocks) if (lufs(b) > -70 && lufs(b) > rel) { s2 += b; n2++; }
        const double L = n2 ? lufs(s2 / n2) : -100;
        os << cat << "/" << lp->getProgramName(i) << "\t" << juce::String(L, 2) << "\t" << juce::String(juce::Decibels::gainToDecibels(peak, -200.0f), 2) << "\t" << juce::String(lp->p("volume"), 1) << "\n";
    }
    return 0;
}

int bench(const juce::String& outFile) {
    std::unique_ptr<juce::AudioProcessor> p(createPluginFilter());
    auto* lp = dynamic_cast<lgn::Processor*>(p.get());
    const double sr = 48000; const int B = 512, blocks = (int)(6 * sr / B);
    juce::File f(juce::File::getCurrentWorkingDirectory().getChildFile(outFile)); f.deleteFile();
    juce::FileOutputStream os(f);
    double total = 0, worst = 0; juce::String worstName;
    const int np = lp ? lp->getNumPrograms() : 1;
    for (int i = 0; i < np; i++) {
        double best = 1e9;
        for (int rep = 0; rep < 3; rep++) {
            if (lp) lp->setCurrentProgram(i);
            p->setRateAndBufferSizeDetails(sr, B);
            TestHead head; p->setPlayHead(&head);
            p->prepareToPlay(sr, B);
            Signal sig; KickTrack kick; ChordMidi cm; juce::AudioBuffer<float> buf(juce::jmax(2, p->getTotalNumInputChannels()), B); juce::MidiBuffer midi;
            double busy = 0;
            for (int k = 0; k < blocks; k++) {
                sig.fill(buf, B); if (buf.getNumChannels() > 2) kick.fill(buf, 2, B);
                midi.clear(); if (p->acceptsMidi()) cm.fill(midi, B);
                const auto t0 = std::chrono::steady_clock::now();
                p->processBlock(buf, midi);
                busy += std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count();
                head.ppq += B * 2.0 / sr;
                if (rep == 0) for (int n = 0; n < B; n++) for (int c = 0; c < 2; c++) os.writeFloat(buf.getSample(c, n));
            }
            p->releaseResources(); p->setPlayHead(nullptr);
            best = std::min(best, busy / (blocks * B / sr));
        }
        total += best;
        const juce::String nm = lp ? (lp->presets()[(size_t)i].category + " / " + lp->getProgramName(i)) : juce::String("default");
        if (best > worst) { worst = best; worstName = nm; }
        std::cout << "    " << nm.paddedRight(' ', 40) << juce::String(best * 100, 2).paddedLeft(' ', 6) << " %\n";
    }
    std::cout << "  " << p->getName() << ": average " << juce::String(total / np * 100, 2) << " %, worst " << juce::String(worst * 100, 2) << " % (" << worstName << ")\n";
    return 0;
}
}

int main(int argc, char** argv) {
    juce::ScopedJuceInitialiser_GUI init;
    if (argc > 2 && juce::String(argv[1]) == "--bench") return bench(argv[2]);
    if (argc > 2 && juce::String(argv[1]) == "--demos") return demos(argv[2]);
    if (argc > 2 && juce::String(argv[1]) == "--levels") return levels(argv[2], argc > 3 ? juce::String(argv[3]) : juce::String());
    std::unique_ptr<juce::AudioProcessor> p(createPluginFilter());
    const juce::String name = p->getName();
    std::cout << name << ": " << p->getParameters().size() << " parameters\n";

    // normal playback at several rates and block sizes
    float peak = 0; double cpu = 0;
    for (double sr : { 44100.0, 48000.0, 96000.0 }) {
        cpu = juce::jmax(cpu, run(*p, sr, 2, [] { return 256; }, 3.0, false, peak));
        juce::Random r(sr); run(*p, sr, 2, [&r] { return 1 + r.nextInt(900); }, 2.0, false, peak);
    }
    run(*p, 48000, 1, [] { return 128; }, 1.5, false, peak);
    std::cout << "  defaults: peak " << juce::Decibels::gainToDecibels(peak) << " dBFS, cpu " << cpu * 100 << "% of one core, latency " << p->getLatencySamples() << " samples\n";

    // parameter fuzz and extremes
    float fp = 0;
    run(*p, 48000, 2, [] { return 128; }, 6.0, true, fp);
    setAll(*p, 0.0f); run(*p, 48000, 2, [] { return 333; }, 1.5, false, fp = 0);
    setAll(*p, 1.0f); { float x = 0; juce::AudioProcessor::BusesLayout lay; (void)lay; run(*p, 48000, 2, [] { return 64; }, 1.5, true, x); }
    setDefaults(*p);
    std::cout << "  fuzz/extremes: ok\n";

    // state round trip
    {
        juce::Random r(11);
        for (auto* prm : p->getParameters()) prm->setValueNotifyingHost(r.nextFloat());
        juce::MemoryBlock mb; p->getStateInformation(mb);
        std::unique_ptr<juce::AudioProcessor> q(createPluginFilter());
        q->setStateInformation(mb.getData(), (int)mb.getSize());
        int diff = 0;
        for (int i = 0; i < p->getParameters().size(); i++)
            if (p->getParameters()[i]->getCurrentValueAsText() != q->getParameters()[i]->getCurrentValueAsText()) {
                diff++;
                std::cout << "    " << p->getParameters()[i]->getName(40) << ": " << p->getParameters()[i]->getCurrentValueAsText() << " vs " << q->getParameters()[i]->getCurrentValueAsText() << "\n";
            }
        check(diff == 0, juce::String(diff) + " parameters differ after state restore");
        setDefaults(*p);
    }

    // factory presets: each loads, plays clean, keeps a sane level, and survives a state round trip
    if (auto* lp = dynamic_cast<lgn::Processor*>(p.get())) {
        const int np = lp->getNumPrograms();
        check(np > 1, "has factory presets");
        for (int i = 0; i < np; i++) {
            for (auto& [pid, pv] : lp->presets()[(size_t)i].values) check(lp->apvts.getParameter(pid) != nullptr, "preset " + lp->getProgramName(i) + " uses a real parameter: " + pid);
            lp->setCurrentProgram(i);
            check(lp->getCurrentProgram() == i && !lp->presetModified(), "preset " + juce::String(i) + " loads clean");
            float pk = 0;
            run(*lp, 48000, 2, [] { return 512; }, 2.0, false, pk);
            // loudness change: RMS out vs in over 3 s of the test signal (after a 1 s settle)
            double si = 0, so = 0;
            {
                lp->prepareToPlay(48000, 512); Signal sg; KickTrack kt; ChordMidi cm; TestHead head; lp->setPlayHead(&head);
                juce::AudioBuffer<float> b(juce::jmax(2, lp->getTotalNumInputChannels()), 512); juce::MidiBuffer m;
                for (int k = 0; k < 375; k++) {
                    sg.fill(b, 512); if (b.getNumChannels() > 2) kt.fill(b, 2, 512); head.ppq += 512 * 2.0 / 48000;
                    m.clear(); if (lp->acceptsMidi()) cm.fill(m, 512);
                    const double ei = b.getRMSLevel(0, 0, 512); lp->processBlock(b, m); const double eo = b.getRMSLevel(0, 0, 512);
                    if (k >= 94) { si += ei * ei; so += eo * eo; }
                }
                lp->releaseResources(); lp->setPlayHead(nullptr);
            }
            const double db = juce::Decibels::gainToDecibels(pk, -200.0f), gain = 10 * std::log10((so + 1e-20) / (si + 1e-20));
            std::cout << "    " << juce::String(i).paddedLeft(' ', 2) << "  " << (lp->presets()[(size_t)i].category + " / " + lp->getProgramName(i)).paddedRight(' ', 38) << " peak " << juce::String(db, 1) << " dBFS  loudness " << (gain >= 0 ? "+" : "") << juce::String(gain, 1) << " dB\n";
            check(db < 12.0 && db > -40.0, "preset level sane: " + lp->getProgramName(i));
            juce::MemoryBlock mb; lp->getStateInformation(mb);
            std::unique_ptr<juce::AudioProcessor> q(createPluginFilter()); q->setStateInformation(mb.getData(), (int)mb.getSize());
            check(q->getCurrentProgram() == i, "preset index restored");
        }
        lp->setCurrentProgram(0);
    }

    // editor: render while audio plays (with a running transport) so meters and displays show something real
    {
        const juce::File dir(juce::File::getCurrentWorkingDirectory().getChildFile(argc > 1 ? argv[1] : "snapshots"));
        dir.createDirectory();
        auto lay = p->getBusesLayout();
        if (!lay.inputBuses.isEmpty()) lay.inputBuses.getReference(0) = juce::AudioChannelSet::stereo();
        lay.outputBuses.getReference(0) = juce::AudioChannelSet::stereo();
        p->setBusesLayout(lay);
        using Settings = std::vector<std::pair<const char*, float>>;
        // a few settings per plugin so the snapshot shows the plugin doing something; some plugins get two shots
        static const std::map<juce::String, std::vector<std::pair<juce::String, Settings>>> demo = {
            { "LGN EQ", { { "", { { "lc_on", 1 }, { "lc_freq", 35 }, { "ls_gain", 3 }, { "ls_freq", 90 }, { "b1_gain", -4.5f }, { "b1_freq", 320 }, { "b1_q", 1.6f },
                          { "b2_gain", 2.5f }, { "b2_freq", 2400 }, { "b3_gain", -3 }, { "b3_freq", 5200 }, { "b3_q", 4 }, { "hs_gain", 4 }, { "hs_freq", 11000 } } } } },
            { "LGN Compressor", { { "", { { "thresh", -26 }, { "ratio", 4 }, { "attack", 8 }, { "release", 90 }, { "style", 1 } } } } },
            { "LGN De-Esser", { { "", { { "thresh", -40 }, { "freq", 5500 } } } } },
            { "LGN Saturator", { { "", { { "mode", 1 }, { "drive", 15 }, { "tone", 0.2f } } } } },
            { "LGN Reverb", { { "", { { "decay", 3.2f }, { "predelay", 35 }, { "mix", 30 } } } } },
            { "LGN Delay", { { "", { { "feedback", 55 }, { "mix", 30 } } } } },
            { "LGN Limiter", { { "", { { "input", 9 }, { "ceiling", -0.5f } } } } },
            { "LGN Gravity", { { "", { { "key", 9 }, { "scale", 1 }, { "pull", 85 } } } } },
            { "LGN Fret", { { "", { { "pickup", 1 }, { "pupos", 40 }, { "amp", 1 }, { "cab", 1 }, { "body", 0 }, { "chorus", 25 }, { "vmix", 22 } } } } },
            { "LGN Distance", { { "", { { "distance", 0.62f }, { "angle", 35 }, { "space", 3 } } } } },
            { "LGN Era", { { "", { { "era", 4 }, { "age", 100 } } } } },
            { "LGN Swing", { { "", { { "swing", 64 }, { "s5", 10 }, { "s13", 10 } } } } },
            { "LGN Mood", { { "", { { "key", 0 }, { "flip", 0 } } } } },
            { "LGN Haze", { { "", { { "o1voices", 3 }, { "o1detune", 25 }, { "sublevel", 40 }, { "res", 40 } } },
                            { "-pattern", { { "patmode", 3 }, { "s3pitch", 12 }, { "s3acc", 1 }, { "s5on", 0 }, { "s7pitch", 7 }, { "s7slide", 1 }, { "s10pitch", -5 }, { "s12acc", 1 }, { "s14pitch", 10 } } } } },
            { "LGN Flipside", { { "", { { "time", 30 }, { "vol", 0 } } } } },
            { "LGN Duck", { { "-tempo", { { "mode", 0 }, { "rate", 2 }, { "amount", 80 }, { "release", 55 } } },
                            { "-kick", { { "mode", 1 }, { "amount", 85 }, { "release", 30 } } } } } };
        std::vector<std::pair<juce::String, Settings>> shots{ { "", {} } };
        if (auto it = demo.find(name); it != demo.end()) shots = it->second;
        for (auto& [suffix, settings] : shots) {
            setDefaults(*p);
            for (auto& [id, v] : settings)
                for (auto* prm : p->getParameters())
                    if (auto* r = dynamic_cast<juce::RangedAudioParameter*>(prm); r && r->getParameterID() == id) r->setValueNotifyingHost(r->convertTo0to1(v));
            TestHead head; p->setPlayHead(&head);
            p->prepareToPlay(48000, 512);
            std::unique_ptr<juce::AudioProcessorEditor> ed(p->createEditor());
            check(ed != nullptr, "editor");
            Signal sig; KickTrack kick; ChordMidi cm; juce::AudioBuffer<float> buf(juce::jmax(2, p->getTotalNumInputChannels()), 512); juce::MidiBuffer midi;
            for (int k = 0; k < 280; k++) {
                sig.fill(buf, 512, 0.35f); if (buf.getNumChannels() > 2) kick.fill(buf, 2, 512);
                midi.clear(); if (p->acceptsMidi()) cm.fill(midi, 512);
                p->processBlock(buf, midi); head.ppq += 512 * 2.0 / 48000;
                if (k % 3 == 0) { juce::Thread::sleep(3); juce::Timer::callPendingTimersSynchronously(); }
            }
            juce::Thread::sleep(40); juce::Timer::callPendingTimersSynchronously();
            const auto img = ed->createComponentSnapshot(ed->getLocalBounds(), true, 2.0f);
            const auto f = dir.getChildFile(name.replace(" ", "-").toLowerCase() + suffix + ".png"); f.deleteFile();
            { juce::FileOutputStream os(f); juce::PNGImageFormat().writeImageToStream(img, os); }
            std::cout << "  editor " << ed->getWidth() << "x" << ed->getHeight() << " -> " << f.getFullPathName() << "\n";
            ed->setSize(ed->getWidth() * 3 / 2, ed->getHeight() * 3 / 2);
            juce::Timer::callPendingTimersSynchronously();
            ed.reset();
            p->releaseResources(); p->setPlayHead(nullptr);
        }
    }
    std::cout << (failures ? "FAILED " : "PASS ") << name << " (" << failures << " failures)\n";
    return failures ? 1 : 0;
}
