#pragma once
#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_dsp/juce_dsp.h>
#include "../src/AetherPatch.h"
#include "../src/AetherPost.h"
#include "../src/AetherSections.h"
#include "../src/AetherCrate.h"
#include "SplitConvolver.h"

// The factory library, shared by every instance of the plugin.
struct Library {
    aether::json data;
    std::vector<std::string> cats;
    struct Entry { std::string name, cat, tags; };
    std::vector<Entry> entries;
    static const Library& get();
};

class AetherProcessor : public juce::AudioProcessor, public juce::ChangeBroadcaster, private juce::Timer {
public:
    AetherProcessor();
    ~AetherProcessor() override;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override { return layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo(); }
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return "Aether"; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 8.0; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}
    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    // Preset handling (message thread).
    void loadPreset(int index);
    void stepPreset(int delta, const std::vector<int>* within = nullptr);
    int currentIndex() const { return curIndex.load(); }
    // Sound editing (message thread). Paths are dotted, e.g. "L.0.f1c", "fx.dly.fb", "mods".
    aether::json patchJson() const { const juce::ScopedLock sl(infoLock); return curPatch; }
    void setPatchValue(const std::string& path, const aether::json& value, bool resetVoices = false);
    void editPatch(const std::function<void(aether::json&)>& fn, bool resetVoices = false);
    bool isEdited() const { return edited.load(); }
    int patchGeneration() const { return patchGen.load(); }   // changes when a whole new patch is loaded
    struct Info { juce::String name, cat; juce::String macro[4]; juce::String layer[4]; bool layerOn[4] = { false, false, false, false }; };
    Info info() const { const juce::ScopedLock sl(infoLock); return curInfo; }

    // Song Sections
    static constexpr const char* kDefaultArrangement = "I8 V16 H8 V16 H8 V16 H8 O8";
    juce::String arrangementText() const { const juce::ScopedLock sl(infoLock); return arrangement; }
    void setArrangement(const juce::String& text);
    std::atomic<float> sectionNow{ -1 };          // the section actually playing (0-3, fractional while morphing)
    // Crate Mode
    std::shared_ptr<const aether::CrateBuffer> crateSnapshot() const { const juce::ScopedLock sl(infoLock); return crateUi; }
    juce::String crateStatus() const { const juce::ScopedLock sl(infoLock); return crateMsg; }
    void repressCrate();
    void pumpCrate(bool force);                 // starts a crate render if the loop is out of date (message thread)
    std::atomic<int> crateHit{ -1 };             // last chop triggered, for the display
    std::atomic<double> hostBpm{ 90 };

    juce::AudioProcessorValueTreeState apvts;
    juce::MidiKeyboardState keyboard;
    std::atomic<int> voiceCount{ 0 };
    std::atomic<float> orbX{ 0 }, orbY{ 0 }, outPeak{ 0 };

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout layout();
    void applyPatch(const aether::json& patch, bool resetParams, bool resetVoices = true);
    void updateIR(const aether::Patch& p);
    void setParam(const char* id, float value);
    void timerCallback() override;
    void scheduleCrate();
    struct ArrFixed { int n = 0, total = 0; int sec[64] = {}; int bars[64] = {}; };
    static ArrFixed toFixed(const aether::Arrangement& a);
    struct CrateCfg { bool on = false, choke = true, oneShot = true; int base = 48, chops = 16; double pitch = 0; };
    static CrateCfg crateCfg(const aether::json& patch);

    aether::Engine engine;
    aether::PostFX post;
    SplitConvolver conv; bool revWasOn = false;
    aether::Assets::Bundle base, pending;
    aether::Sections secBase, secPending; aether::SectionState secLast; double secPos = 2;
    ArrFixed arrAudio, arrPending; bool arrHasPending = false;
    CrateCfg crateAudio, cratePendingCfg;
    std::shared_ptr<const aether::CrateBuffer> crateBuf, crateIncoming, crateUi; bool crateHasIncoming = false;
    std::vector<std::shared_ptr<const aether::CrateBuffer>> crateRetired; juce::SpinLock crateLock;
    aether::CratePlayer cratePlayer;
    juce::ThreadPool renderPool{ 1 };
    std::atomic<int> crateJob{ 0 }; std::string crateKey; double crateBpm = 0; int crateDebounce = 0; uint32_t crateSeed = 1;
    juce::String arrangement{ kDefaultArrangement }, crateMsg;
    bool hasPending = false, pendingReset = true; juce::SpinLock pendLock; std::atomic<bool> edited{ false }; std::atomic<int> patchGen{ 0 };
    std::vector<float> bufL, bufR, sendL, sendR;
    double sr = 48000; int maxBlock = 512;

    juce::CriticalSection infoLock;
    Info curInfo; aether::json curPatch; std::atomic<int> curIndex{ 0 };
    juce::String irKey; double irSr = 0;

    std::atomic<float>* pVol = nullptr; std::atomic<float>* pMac[4] = {}; std::atomic<float>* pOx = nullptr; std::atomic<float>* pOy = nullptr;
    std::atomic<float>* pCut = nullptr; std::atomic<float>* pRes = nullptr; std::atomic<float>* pAtk = nullptr; std::atomic<float>* pRel = nullptr;
    std::atomic<float>* pRev = nullptr; std::atomic<float>* pDly = nullptr; std::atomic<float>* pLvl[4] = {};
    std::atomic<float>* pSecOn = nullptr; std::atomic<float>* pSection = nullptr; std::atomic<float>* pFollow = nullptr; std::atomic<float>* pMorph = nullptr;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AetherProcessor)
};
