#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <atomic>
#include "DuckDSP.h"

class DuckProcessor : public juce::AudioProcessor {
public:
    DuckProcessor();
    void prepareToPlay(double sampleRate, int) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported(const BusesLayout&) const override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return "LGN Duck"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 0; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}
    void getStateInformation(juce::MemoryBlock&) override;
    void setStateInformation(const void*, int) override;

    juce::AudioProcessorValueTreeState params;
    static constexpr double kRateBeats[5] = { 4.0, 2.0, 1.0, 0.5, 0.25 };   // 1/1, 1/2, 1/4, 1/8, 1/16
    std::atomic<float> uiReduction { 0 }, uiHitAge { 1e9f }, uiPhase { 0 };
    std::atomic<bool> uiPlaying { false }, uiHasSidechain { false };
private:
    static juce::AudioProcessorValueTreeState::ParameterLayout layout();
    lgn::Duck duck;
    double fallbackPpq = 0;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DuckProcessor)
};
