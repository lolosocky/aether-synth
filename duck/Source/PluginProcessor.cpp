#include "PluginProcessor.h"
#include "PluginEditor.h"

DuckProcessor::DuckProcessor()
    : AudioProcessor(BusesProperties()
          .withInput("Input", juce::AudioChannelSet::stereo(), true)
          .withOutput("Output", juce::AudioChannelSet::stereo(), true)
          .withInput("Sidechain (Kick)", juce::AudioChannelSet::stereo(), false)),
      params(*this, nullptr, "LGNDuck", layout()) {}

juce::AudioProcessorValueTreeState::ParameterLayout DuckProcessor::layout() {
    using namespace juce;
    AudioProcessorValueTreeState::ParameterLayout l;
    l.add(std::make_unique<AudioParameterFloat>(ParameterID { "amount", 1 }, "Amount", NormalisableRange<float>(0, 100, 0.1f), 75.0f,
          AudioParameterFloatAttributes().withLabel("%")));
    l.add(std::make_unique<AudioParameterFloat>(ParameterID { "release", 1 }, "Release", NormalisableRange<float>(0, 100, 0.1f), 50.0f,
          AudioParameterFloatAttributes().withLabel("%")));
    l.add(std::make_unique<AudioParameterChoice>(ParameterID { "rate", 1 }, "Rate", StringArray { "1/1", "1/2", "1/4", "1/8", "1/16" }, 2));
    l.add(std::make_unique<AudioParameterChoice>(ParameterID { "mode", 1 }, "Mode", StringArray { "Tempo", "Kick" }, 0));
    return l;
}

bool DuckProcessor::isBusesLayoutSupported(const BusesLayout& b) const {
    const auto in = b.getMainInputChannelSet(), out = b.getMainOutputChannelSet();
    if (in != out || (out != juce::AudioChannelSet::mono() && out != juce::AudioChannelSet::stereo())) return false;
    if (b.inputBuses.size() > 1) {
        const auto sc = b.getChannelSet(true, 1);
        if (!sc.isDisabled() && sc != juce::AudioChannelSet::mono() && sc != juce::AudioChannelSet::stereo()) return false;
    }
    return true;
}

void DuckProcessor::prepareToPlay(double sr, int) { duck.prepare(sr); fallbackPpq = 0; }

void DuckProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&) {
    juce::ScopedNoDenormals noDenormals;
    const int n = buffer.getNumSamples();
    auto main = getBusBuffer(buffer, true, 0);
    for (int c = getTotalNumInputChannels(); c < getTotalNumOutputChannels(); ++c) buffer.clear(c, 0, n);

    double bpm = 120, ppq = fallbackPpq; bool playing = false;
    if (auto* ph = getPlayHead())
        if (auto pos = ph->getPosition()) {
            if (auto b = pos->getBpm()) bpm = *b;
            playing = pos->getIsPlaying();
            if (auto p = pos->getPpqPosition()) ppq = *p;
        }
    fallbackPpq = ppq + n * bpm / 60.0 / getSampleRate();

    duck.amount = params.getRawParameterValue("amount")->load() / 100.0;
    duck.release = params.getRawParameterValue("release")->load() / 100.0;
    duck.mode = (int)params.getRawParameterValue("mode")->load();
    duck.rateBeats = kRateBeats[juce::jlimit(0, 4, (int)params.getRawParameterValue("rate")->load())];

    const float* sc[2] = { nullptr, nullptr }; int numSc = 0;
    if (getBusCount(true) > 1 && getBus(true, 1)->isEnabled()) {
        auto scBuf = getBusBuffer(buffer, true, 1);
        numSc = juce::jmin(2, scBuf.getNumChannels());
        for (int c = 0; c < numSc; ++c) sc[c] = scBuf.getReadPointer(c);
    }
    uiHasSidechain = numSc > 0;
    duck.process(main.getArrayOfWritePointers(), main.getNumChannels(), n, ppq, bpm, playing, sc, numSc);

    uiReduction = (float)duck.reduction;
    uiHitAge = (float)duck.lastHitAge;
    uiPlaying = playing;
    double ph = std::fmod(ppq, duck.rateBeats) / duck.rateBeats; if (ph < 0) ph += 1;
    uiPhase = (float)ph;
}

void DuckProcessor::getStateInformation(juce::MemoryBlock& dest) {
    if (auto xml = params.copyState().createXml()) copyXmlToBinary(*xml, dest);
}
void DuckProcessor::setStateInformation(const void* data, int size) {
    if (auto xml = getXmlFromBinary(data, size))
        if (xml->hasTagName(params.state.getType())) params.replaceState(juce::ValueTree::fromXml(*xml));
}

juce::AudioProcessorEditor* DuckProcessor::createEditor() { return new DuckEditor(*this); }
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new DuckProcessor(); }
