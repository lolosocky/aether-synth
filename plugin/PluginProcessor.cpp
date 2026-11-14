#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "BinaryData.h"

using aether::json;

const Library& Library::get() {
    static const Library lib = [] {
        Library l;
        juce::MemoryInputStream in(BinaryData::presets_json_gz, (size_t)BinaryData::presets_json_gzSize, false);
        juce::GZIPDecompressorInputStream gz(&in, false, juce::GZIPDecompressorInputStream::gzipFormat);
        juce::MemoryOutputStream out; out.writeFromInputStream(gz, -1);
        l.data = json::parse((const char*)out.getData(), (const char*)out.getData() + out.getDataSize(), nullptr, false);
        if (!l.data.is_object() || !l.data.contains("presets")) l.data = json{ { "cats", json::array() }, { "presets", json::array() } };
        for (const auto& c : l.data["cats"]) if (c.is_string()) l.cats.push_back(c.get<std::string>());
        for (const auto& p : l.data["presets"]) {
            Entry e; e.name = aether::jstr(p, "name", "Preset"); e.cat = aether::jstr(p, "cat", "");
            if (p.contains("tags") && p["tags"].is_array()) for (const auto& t : p["tags"]) if (t.is_string()) e.tags += t.get<std::string>() + " ";
            l.entries.push_back(e);
        }
        return l;
    }();
    return lib;
}

juce::AudioProcessorValueTreeState::ParameterLayout AetherProcessor::layout() {
    using P = juce::AudioParameterFloat; using R = juce::NormalisableRange<float>;
    juce::AudioProcessorValueTreeState::ParameterLayout l;
    auto add = [&l](const char* id, const char* name, float lo, float hi, float def) { l.add(std::make_unique<P>(juce::ParameterID{ id, 1 }, name, R(lo, hi, 0.0f), def)); };
    add("volume", "Volume", 0, 1, 0.8f);
    add("mac1", "Macro 1", 0, 1, 0); add("mac2", "Macro 2", 0, 1, 0); add("mac3", "Macro 3", 0, 1, 0); add("mac4", "Macro 4", 0, 1, 0);
    add("orbx", "Orb X", -1, 1, 0); add("orby", "Orb Y", -1, 1, 0);
    add("cutoff", "Cutoff", -1, 1, 0); add("reso", "Resonance", -1, 1, 0); add("attack", "Attack", -1, 1, 0); add("release", "Release", -1, 1, 0);
    add("reverb", "Reverb", 0, 1, 0.22f); add("delay", "Delay", 0, 1, 0);
    add("lvla", "Layer A Level", 0, 1.5f, 1); add("lvlb", "Layer B Level", 0, 1.5f, 1); add("lvlc", "Layer C Level", 0, 1.5f, 1); add("lvld", "Layer D Level", 0, 1.5f, 1);
    return l;
}

AetherProcessor::AetherProcessor()
    : AudioProcessor(BusesProperties().withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, nullptr, "AETHER", layout()) {
    pVol = apvts.getRawParameterValue("volume");
    const char* mac[4] = { "mac1", "mac2", "mac3", "mac4" }; const char* lvl[4] = { "lvla", "lvlb", "lvlc", "lvld" };
    for (int i = 0; i < 4; i++) { pMac[i] = apvts.getRawParameterValue(mac[i]); pLvl[i] = apvts.getRawParameterValue(lvl[i]); }
    pOx = apvts.getRawParameterValue("orbx"); pOy = apvts.getRawParameterValue("orby");
    pCut = apvts.getRawParameterValue("cutoff"); pRes = apvts.getRawParameterValue("reso"); pAtk = apvts.getRawParameterValue("attack"); pRel = apvts.getRawParameterValue("release");
    pRev = apvts.getRawParameterValue("reverb"); pDly = apvts.getRawParameterValue("delay");
    aether::Assets::get().fill(base);
    loadPreset(0);
}

void AetherProcessor::setParam(const char* id, float value) {
    if (auto* p = apvts.getParameter(id)) p->setValueNotifyingHost(p->convertTo0to1(value));
}

void AetherProcessor::applyPatch(const json& patch, bool resetParams, bool resetVoices) {
    aether::Assets::Bundle b; b.p = aether::patchFromJson(patch);
    aether::Assets::get().fill(b);          // builds any wavetable or source this patch needs; safe off the audio thread
    Info inf; inf.name = aether::jstr(patch, "name", "Init"); inf.cat = aether::jstr(patch, "cat", "");
    for (int i = 0; i < 4; i++) {
        inf.macro[i] = "Macro " + juce::String(i + 1);
        if (patch.contains("mac") && patch["mac"].is_array() && patch["mac"].size() > (size_t)i) inf.macro[i] = aether::jstr(patch["mac"][(size_t)i], "n", inf.macro[i].toStdString());
        static const char* nice[8] = { "Analog", "Wavetable", "Pluck", "Modal", "Formant", "Noise", "Granular", "Sample" };
        inf.layerOn[i] = b.p.L[i].on != 0; inf.layer[i] = nice[juce::jlimit(0, 7, b.p.L[i].mode)];
    }
    { const juce::ScopedLock sl(infoLock); curInfo = inf; curPatch = patch; }
    updateIR(b.p);
    if (resetParams) {
        const char* mac[4] = { "mac1", "mac2", "mac3", "mac4" }; const char* lvl[4] = { "lvla", "lvlb", "lvlc", "lvld" };
        for (int i = 0; i < 4; i++) { setParam(mac[i], (float)juce::jlimit(0.0, 1.0, b.p.mac[i])); setParam(lvl[i], 1.0f); }
        setParam("orbx", (float)b.p.orb.x); setParam("orby", (float)b.p.orb.y);
        setParam("cutoff", 0); setParam("reso", 0); setParam("attack", 0); setParam("release", 0);
        setParam("reverb", b.p.fx.rev.on ? (float)b.p.fx.rev.mix : 0.0f); setParam("delay", b.p.fx.dly.on ? (float)b.p.fx.dly.mix : 0.0f);
    }
    { const juce::SpinLock::ScopedLockType sl(pendLock); pending = b; pendingReset = hasPending ? (pendingReset || resetVoices) : resetVoices; hasPending = true; }
    sendChangeMessage();
}

void AetherProcessor::loadPreset(int index) {
    const auto& lib = Library::get(); const auto& ps = lib.data["presets"];
    if (ps.empty()) { applyPatch(json::object(), true); return; }
    index = juce::jlimit(0, (int)ps.size() - 1, index);
    curIndex = index; edited = false; patchGen++;
    applyPatch(ps[(size_t)index], true);
}

void AetherProcessor::editPatch(const std::function<void(json&)>& fn, bool resetVoices) {
    json p; { const juce::ScopedLock sl(infoLock); p = curPatch; }
    fn(p);
    edited = true;
    applyPatch(p, false, resetVoices);
}

void AetherProcessor::setPatchValue(const std::string& path, const json& value, bool resetVoices) {
    editPatch([&](json& root) {
        json* o = &root; juce::StringArray parts; parts.addTokens(juce::String(path), ".", "");
        for (int i = 0; i < parts.size(); i++) {
            const std::string k = parts[i].toStdString(); const bool last = i == parts.size() - 1;
            if (o->is_array() && parts[i].containsOnly("0123456789")) {
                const size_t idx = (size_t)parts[i].getIntValue();
                while (o->size() <= idx) o->push_back(json::object());
                if (last) (*o)[idx] = value; else o = &(*o)[idx];
            } else {
                if (!o->is_object()) *o = json::object();
                if (last) (*o)[k] = value; else o = &(*o)[k];
            }
        }
    }, resetVoices);
}

void AetherProcessor::stepPreset(int delta, const std::vector<int>* within) {
    const int n = (int)Library::get().entries.size(); if (n == 0) return;
    if (within && !within->empty()) {
        auto it = std::find(within->begin(), within->end(), curIndex.load());
        int pos = it == within->end() ? (delta > 0 ? -1 : 0) : (int)(it - within->begin());
        pos = ((pos + delta) % (int)within->size() + (int)within->size()) % (int)within->size();
        loadPreset((*within)[(size_t)pos]);
    } else loadPreset(((curIndex.load() + delta) % n + n) % n);
}

void AetherProcessor::updateIR(const aether::Patch& p) {
    const auto& r = p.fx.rev;
    const juce::String key = juce::String(r.type) + "|" + juce::String(r.size, 2) + "|" + juce::String(r.decay, 2) + "|" + juce::String(r.damp, 2);
    if (key == irKey && irSr == sr) return;
    irKey = key; irSr = sr;
    std::vector<float> L, R; aether::makeIR(r.type, r.decay, r.size, r.damp, sr, L, R);
    juce::AudioBuffer<float> ir(2, (int)L.size());
    ir.copyFrom(0, 0, L.data(), (int)L.size()); ir.copyFrom(1, 0, R.data(), (int)R.size());
    conv.loadImpulseResponse(std::move(ir), sr, juce::dsp::Convolution::Stereo::yes, juce::dsp::Convolution::Trim::no, juce::dsp::Convolution::Normalise::no);
}

void AetherProcessor::prepareToPlay(double sampleRate, int samplesPerBlock) {
    sr = sampleRate;
    engine.setSampleRate(sampleRate); engine.panic();
    post.prepare(sampleRate);
    const int n = juce::jmax(64, samplesPerBlock); maxBlock = n;
    bufL.assign((size_t)n, 0.0f); bufR.assign((size_t)n, 0.0f); sendL.assign((size_t)n, 0.0f); sendR.assign((size_t)n, 0.0f);
    conv.prepare({ sampleRate, (juce::uint32)n, 2 });
    aether::Patch p; { const juce::ScopedLock sl(infoLock); p = aether::patchFromJson(curPatch); }
    updateIR(p);
}

void AetherProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi) {
    juce::ScopedNoDenormals noDenormals;
    const int n = buffer.getNumSamples();
    if (n <= 0) return;
    if ((int)bufL.size() < n) { bufL.resize((size_t)n); bufR.resize((size_t)n); sendL.resize((size_t)n); sendR.resize((size_t)n); }
    {
        const juce::SpinLock::ScopedTryLockType sl(pendLock);
        if (sl.isLocked() && hasPending) { base = pending; hasPending = false; engine.setBundle(base, pendingReset); }
    }
    // live patch = loaded patch + the performance controls
    aether::Patch live = base.p;
    for (int i = 0; i < 4; i++) { live.mac[i] = pMac[i]->load(); live.L[i].lvl *= pLvl[i]->load(); }
    live.orb.x = pOx->load(); live.orb.y = pOy->load();
    const double cut = pCut->load() * 0.5, res = pRes->load() * 0.5, atk = std::pow(2.0, pAtk->load() * 4.0), rel = std::pow(2.0, pRel->load() * 4.0);
    for (auto& L : live.L) { L.f1c = aether::clampd(L.f1c + cut, 0, 1); L.f1r = aether::clampd(L.f1r + res, 0, 1); L.aa *= atk; L.ar *= rel; }
    const double rev = pRev->load(), dly = pDly->load();
    live.fx.rev.on = rev > 0.0005; live.fx.rev.mix = rev; live.fx.dly.on = dly > 0.0005; live.fx.dly.mix = dly;
    engine.patch = live;

    if (auto* ph = getPlayHead()) if (auto pos = ph->getPosition()) if (auto bpm = pos->getBpm()) engine.bpm = aether::clampd(*bpm, 20, 300);
    keyboard.processNextMidiBuffer(midi, 0, n, true);

    float* L = bufL.data(); float* R = bufR.data();
    int done = 0;
    auto render = [&](int upTo) { if (upTo > done) { engine.process(L + done, R + done, upTo - done); done = upTo; } };
    for (const auto meta : midi) {
        const auto m = meta.getMessage();
        render(juce::jlimit(0, n, meta.samplePosition));
        if (m.isNoteOn()) engine.noteIn(m.getNoteNumber(), juce::jlimit(0.02f, 1.0f, m.getFloatVelocity()));
        else if (m.isNoteOff()) engine.noteOut(m.getNoteNumber());
        else if (m.isPitchWheel()) engine.pbT = (m.getPitchWheelValue() - 8192) / 8192.0;
        else if (m.isChannelPressure()) engine.atT = m.getChannelPressureValue() / 127.0;
        else if (m.isAftertouch()) engine.atT = m.getAfterTouchValue() / 127.0;
        else if (m.isController()) {
            const int cc = m.getControllerNumber(); const double v = m.getControllerValue() / 127.0;
            if (cc == 1) engine.mwT = v; else if (cc == 64) engine.setSus(v >= 0.5); else if (cc == 120 || cc == 123) engine.panic();
        }
    }
    render(n);
    midi.clear();

    // effects run in slices no longer than the block size the convolver was prepared for
    for (int off = 0; off < n; off += maxBlock) {
        const int m = juce::jmin(maxBlock, n - off);
        float* sl = sendL.data() + off; float* sr2 = sendR.data() + off;
        post.stage1(live, engine.fxm, engine.bpm, L + off, R + off, sl, sr2, m);
        if (live.fx.rev.on) {
            float* ch[2] = { sl, sr2 };
            juce::dsp::AudioBlock<float> block(ch, 2, (size_t)m);
            conv.process(juce::dsp::ProcessContextReplacing<float>(block));
        } else { std::fill(sl, sl + m, 0.0f); std::fill(sr2, sr2 + m, 0.0f); }
        post.stage2(live, engine.fxm, pVol->load(), L + off, R + off, sl, sr2, m);
    }

    float pk = 0;
    for (int i = 0; i < n; i++) pk = juce::jmax(pk, std::abs(L[i]), std::abs(R[i]));
    outPeak.store(juce::jmax(pk, outPeak.load() * 0.9f));
    for (int c = 0; c < buffer.getNumChannels(); c++) buffer.copyFrom(c, 0, c == 0 ? L : R, n);
    voiceCount.store(engine.activeVoices()); orbX.store((float)engine.ox); orbY.store((float)engine.oy);
}

void AetherProcessor::getStateInformation(juce::MemoryBlock& destData) {
    auto state = apvts.copyState();
    { const juce::ScopedLock sl(infoLock); state.setProperty("patch", juce::String(curPatch.dump()), nullptr); }
    state.setProperty("presetIndex", curIndex.load(), nullptr); state.setProperty("edited", edited.load(), nullptr);
    if (auto xml = state.createXml()) copyXmlToBinary(*xml, destData);
}

void AetherProcessor::setStateInformation(const void* data, int sizeInBytes) {
    auto xml = getXmlFromBinary(data, sizeInBytes);
    if (!xml) return;
    auto state = juce::ValueTree::fromXml(*xml);
    if (!state.isValid()) return;
    const juce::String patchText = state.getProperty("patch").toString();
    const json patch = json::parse(patchText.toStdString(), nullptr, false);
    if (patch.is_object()) {
        int idx = (int)state.getProperty("presetIndex", 0);
        const auto& entries = Library::get().entries; const std::string name = aether::jstr(patch, "name", "");
        if (idx < 0 || idx >= (int)entries.size() || entries[(size_t)idx].name != name) { idx = -1; for (size_t i = 0; i < entries.size(); i++) if (entries[i].name == name) { idx = (int)i; break; } }
        curIndex = idx; edited = (bool)state.getProperty("edited", false); patchGen++;
        applyPatch(patch, false);
    }
    state.removeProperty("patch", nullptr); state.removeProperty("presetIndex", nullptr); state.removeProperty("edited", nullptr);
    apvts.replaceState(state);
    sendChangeMessage();
}

juce::AudioProcessorEditor* AetherProcessor::createEditor() { return new AetherEditor(*this); }
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new AetherProcessor(); }
