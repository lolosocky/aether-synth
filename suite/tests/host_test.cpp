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

double run(juce::AudioProcessor& p, double sr, int chans, std::function<int()> blockSize, double seconds, bool fuzz, float& peak) {
    juce::AudioProcessor::BusesLayout lay;
    lay.inputBuses.add(chans == 1 ? juce::AudioChannelSet::mono() : juce::AudioChannelSet::stereo());
    lay.outputBuses.add(chans == 1 ? juce::AudioChannelSet::mono() : juce::AudioChannelSet::stereo());
    check(p.setBusesLayout(lay), "layout " + juce::String(chans) + " ch");
    p.prepareToPlay(sr, 1024);
    Signal sig; sig.sr = sr; juce::Random rng{ 3 };
    juce::AudioBuffer<float> buf(chans, 1024); juce::MidiBuffer midi;
    const long long N = (long long)(seconds * sr); long long done = 0; double busy = 0;
    auto& params = p.getParameters();
    while (done < N) {
        const int n = (int)std::min<long long>(blockSize(), N - done);
        if (fuzz) for (auto* prm : params) if (rng.nextFloat() < 0.15f) prm->setValueNotifyingHost(rng.nextFloat());
        sig.fill(buf, n);
        juce::AudioBuffer<float> view(buf.getArrayOfWritePointers(), chans, n);
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
}

int main(int argc, char** argv) {
    juce::ScopedJuceInitialiser_GUI init;
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
            lp->setCurrentProgram(i);
            check(lp->getCurrentProgram() == i && !lp->presetModified(), "preset " + juce::String(i) + " loads clean");
            float pk = 0;
            run(*lp, 48000, 2, [] { return 512; }, 2.0, false, pk);
            // loudness change: RMS out vs in over 3 s of the test signal (after a 1 s settle)
            double si = 0, so = 0;
            {
                lp->prepareToPlay(48000, 512); Signal sg; juce::AudioBuffer<float> b(2, 512); juce::MidiBuffer m;
                for (int k = 0; k < 375; k++) {
                    sg.fill(b, 512); const double ei = b.getRMSLevel(0, 0, 512); lp->processBlock(b, m); const double eo = b.getRMSLevel(0, 0, 512);
                    if (k >= 94) { si += ei * ei; so += eo * eo; }
                }
                lp->releaseResources();
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

    // editor: render while audio plays so meters and displays show something real
    {
        const juce::File dir(juce::File::getCurrentWorkingDirectory().getChildFile(argc > 1 ? argv[1] : "snapshots"));
        dir.createDirectory();
        juce::AudioProcessor::BusesLayout lay; lay.inputBuses.add(juce::AudioChannelSet::stereo()); lay.outputBuses.add(juce::AudioChannelSet::stereo());
        p->setBusesLayout(lay);
        p->prepareToPlay(48000, 512);
        // a few settings per plugin so the snapshot shows the plugin doing something
        static const std::map<juce::String, std::vector<std::pair<const char*, float>>> demo = {
            { "LGN EQ", { { "lc_on", 1 }, { "lc_freq", 35 }, { "ls_gain", 3 }, { "ls_freq", 90 }, { "b1_gain", -4.5f }, { "b1_freq", 320 }, { "b1_q", 1.6f },
                          { "b2_gain", 2.5f }, { "b2_freq", 2400 }, { "b3_gain", -3 }, { "b3_freq", 5200 }, { "b3_q", 4 }, { "hs_gain", 4 }, { "hs_freq", 11000 } } },
            { "LGN Compressor", { { "thresh", -26 }, { "ratio", 4 }, { "attack", 8 }, { "release", 90 }, { "style", 1 } } },
            { "LGN De-Esser", { { "thresh", -40 }, { "freq", 5500 } } },
            { "LGN Saturator", { { "mode", 1 }, { "drive", 15 }, { "tone", 0.2f } } },
            { "LGN Reverb", { { "decay", 3.2f }, { "predelay", 35 }, { "mix", 30 } } },
            { "LGN Delay", { { "feedback", 55 }, { "mix", 30 } } },
            { "LGN Limiter", { { "input", 9 }, { "ceiling", -0.5f } } },
            { "LGN Gravity", { { "key", 9 }, { "scale", 1 }, { "pull", 85 } } } };
        if (auto it = demo.find(name); it != demo.end())
            for (auto& [id, v] : it->second)
                for (auto* prm : p->getParameters())
                    if (auto* r = dynamic_cast<juce::RangedAudioParameter*>(prm); r && r->getParameterID() == id) r->setValueNotifyingHost(r->convertTo0to1(v));
        std::unique_ptr<juce::AudioProcessorEditor> ed(p->createEditor());
        check(ed != nullptr, "editor");
        Signal sig; juce::AudioBuffer<float> buf(2, 512); juce::MidiBuffer midi;
        for (int k = 0; k < 60; k++) {
            sig.fill(buf, 512, 0.35f); p->processBlock(buf, midi);
            juce::Thread::sleep(9); juce::Timer::callPendingTimersSynchronously();
        }
        const auto img = ed->createComponentSnapshot(ed->getLocalBounds(), true, 2.0f);
        const auto f = dir.getChildFile(name.replace(" ", "-").toLowerCase() + ".png"); f.deleteFile();
        { juce::FileOutputStream os(f); juce::PNGImageFormat().writeImageToStream(img, os); }
        std::cout << "  editor " << ed->getWidth() << "x" << ed->getHeight() << " -> " << f.getFullPathName() << "\n";
        ed->setSize(ed->getWidth() * 3 / 2, ed->getHeight() * 3 / 2);
        juce::Timer::callPendingTimersSynchronously();
        ed.reset();
        p->releaseResources();
    }
    std::cout << (failures ? "FAILED " : "PASS ") << name << " (" << failures << " failures)\n";
    return failures ? 1 : 0;
}
