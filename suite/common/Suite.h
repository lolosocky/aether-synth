// Shared plumbing for the LGN mixing suite: a processor base (parameters, state, meters, a scope feed for displays)
// and an editor base in the Station look (header with wordmark and meters, scalable content, standard widgets).
#pragma once
#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_dsp/juce_dsp.h>
#include "Station.h"
#include <array>
#include <atomic>
#include <map>
#include "../dsp/Common.h"

namespace lgn {

// Lock-free single-writer feed of recent samples for displays. The audio thread pushes, the UI reads the newest N.
class ScopeFeed {
public:
    static constexpr int kSize = 16384;
    void push(const float* a, const float* b, int n) {
        int w = write.load(std::memory_order_relaxed);
        for (int i = 0; i < n; i++) { buf[(size_t)w] = b ? 0.5f * (a[i] + b[i]) : a[i]; w = (w + 1) & (kSize - 1); }
        write.store(w, std::memory_order_release);
    }
    void latest(float* out, int n) const {
        const int w = write.load(std::memory_order_acquire);
        for (int i = 0; i < n; i++) out[i] = buf[(size_t)((w - n + i) & (kSize - 1))];
    }
private:
    std::array<float, kSize> buf{};
    std::atomic<int> write{ 0 };
};

// Peak meter value with a hold the UI can decay. Linear amplitude.
struct PeakMeter {
    std::atomic<float> peak{ 0 };
    void feed(const juce::AudioBuffer<float>& b, int chans, int n) {
        float p = 0; for (int c = 0; c < chans; c++) p = juce::jmax(p, b.getMagnitude(c, 0, n));
        float old = peak.load(std::memory_order_relaxed);
        while (p > old && !peak.compare_exchange_weak(old, p)) {}
    }
    float take() { return peak.exchange(0.0f); }
};

class Processor : public juce::AudioProcessor {
public:
    Processor(const juce::String& stateName, juce::AudioProcessorValueTreeState::ParameterLayout layout);
    juce::AudioProcessorValueTreeState apvts;
    PeakMeter inMeter, outMeter;
    ScopeFeed scopeIn, scopeOut;

    // raw parameter read for the audio thread
    float p(const char* id) const { return params.at(id)->load(std::memory_order_relaxed); }

    bool isBusesLayoutSupported(const BusesLayout&) const override;
    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool hasEditor() const override { return true; }
    // Factory presets. Index 0 is always "Default" (every parameter at its default).
    struct Preset { juce::String category, name; std::vector<std::pair<juce::String, float>> values; };
    const std::vector<Preset>& presets() const { return presetList; }
    void loadPreset(int index);
    int currentPreset() const { return current.load(); }
    // true when a parameter has moved since the current preset was loaded
    bool presetModified() const;
    int getNumPrograms() override { return (int)presetList.size(); }
    int getCurrentProgram() override { return current.load(); }
    void setCurrentProgram(int i) override { loadPreset(i); }
    const juce::String getProgramName(int i) override { return juce::isPositiveAndBelow(i, (int)presetList.size()) ? presetList[(size_t)i].name : juce::String(); }
    void changeProgramName(int, const juce::String&) override {}
    void releaseResources() override {}
    void getStateInformation(juce::MemoryBlock&) override;
    void setStateInformation(const void*, int) override;

protected:
    // Call from the subclass constructor. Parameters a preset doesn't mention go to their defaults,
    // except the ones in `keep` (view-only switches like Listen or Spectrum).
    void setPresets(std::vector<Preset> list, const juce::StringArray& keep = {});
    // Call at the start and end of processBlock.
    void meterInput(const juce::AudioBuffer<float>& b, int n) { inMeter.feed(b, getTotalNumInputChannels(), n); scopeIn.push(b.getReadPointer(0), getTotalNumInputChannels() > 1 ? b.getReadPointer(1) : nullptr, n); }
    void meterOutput(const juce::AudioBuffer<float>& b, int n) { outMeter.feed(b, getTotalNumOutputChannels(), n); scopeOut.push(b.getReadPointer(0), getTotalNumOutputChannels() > 1 ? b.getReadPointer(1) : nullptr, n); }
private:
    std::map<juce::String, std::atomic<float>*> params;
    std::vector<Preset> presetList{ { "", "Default", {} } };
    juce::StringArray keepIds;
    std::atomic<int> current{ 0 };
    std::vector<float> loaded;   // normalised values right after the last preset load
};

// ---------------------------------------------------------------- parameter helpers
namespace param {
using Layout = juce::AudioProcessorValueTreeState::ParameterLayout;
juce::NormalisableRange<float> logRange(float lo, float hi);                 // frequency-style skew
juce::NormalisableRange<float> skewRange(float lo, float hi, float centre);   // value at the middle of travel
void add(Layout&, const char* id, const char* name, juce::NormalisableRange<float>, float def, const char* unit, int decimals = 1);
void addHz(Layout&, const char* id, const char* name, float lo, float hi, float def);
void addMs(Layout&, const char* id, const char* name, float lo, float hi, float def, float centre);
void addDb(Layout&, const char* id, const char* name, float lo, float hi, float def);
void addPct(Layout&, const char* id, const char* name, float def);
void addBool(Layout&, const char* id, const char* name, bool def);
void addChoice(Layout&, const char* id, const char* name, const juce::StringArray&, int def);
juce::String formatHz(float);
}

// ---------------------------------------------------------------- widgets
// Knob with a caps label above or below and a value readout. Double-click resets; drag the readout to adjust.
class Dial : public juce::Component {
public:
    Dial(juce::AudioProcessorValueTreeState&, const char* id, const juce::String& label, bool small = false);
    void resized() override;
    void paint(juce::Graphics&) override;
    juce::Slider slider;
    juce::String label;
    bool small;
    int maxKnob = 72;   // knob diameter cap; the dial centres itself in whatever space it is given
private:
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> att;
    juce::Rectangle<int> labelArea;
};

// Labelled switch bound to a bool parameter.
class Switch : public juce::Component {
public:
    Switch(juce::AudioProcessorValueTreeState&, const char* id, const juce::String& label);
    void resized() override { button.setBounds(getLocalBounds()); }
    juce::ToggleButton button;
private:
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> att;
};

// Segmented selector bound to a choice parameter: one tab per option.
class Segments : public juce::Component, private juce::AudioProcessorValueTreeState::Listener, private juce::AsyncUpdater {
public:
    Segments(juce::AudioProcessorValueTreeState&, const char* id, juce::StringArray labels = {});
    ~Segments() override;
    void resized() override;
    void paint(juce::Graphics&) override;
private:
    void parameterChanged(const juce::String&, float) override { triggerAsyncUpdate(); }
    void handleAsyncUpdate() override { refresh(); }
    void refresh();
    juce::AudioProcessorValueTreeState& state; juce::String id;
    juce::OwnedArray<juce::TextButton> buttons;
};

// Drop-down bound to a choice parameter.
class Choice : public juce::Component {
public:
    Choice(juce::AudioProcessorValueTreeState&, const char* id, const juce::String& label);
    void resized() override;
    void paint(juce::Graphics&) override;
    juce::ComboBox box; juce::String label;
private:
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> att;
};

// Vertical level meter (dBFS) or gain-reduction meter (drawn from the top in the accent colour).
class Meter : public juce::Component {
public:
    enum Mode { level, reduction };
    Meter(Mode m, juce::Colour accent) : mode(m), accent(accent) {}
    void setValue(float linOrDb);   // level: linear peak; reduction: dB (positive)
    void paint(juce::Graphics&) override;
    Mode mode; juce::Colour accent; float shown = 0, hold = 0; int holdAge = 0;
};

// Scrolling history: level as a soft graphite area rising from the bottom, gain reduction hanging from the top
// in the accent colour. push() once per UI tick.
class History : public juce::Component {
public:
    explicit History(juce::Colour a, float maxReductionDb = 24.0f) : accent(a), maxGr(maxReductionDb) { inH.assign(kLen, -90); grH.assign(kLen, 0); }
    void push(float levelDb, float grDb) { inH[(size_t)pos] = levelDb; grH[(size_t)pos] = grDb; pos = (pos + 1) % kLen; repaint(); }
    void paint(juce::Graphics&) override;
private:
    static constexpr int kLen = 150;
    juce::Colour accent; float maxGr; std::vector<float> inH, grH; int pos = 0;
};

// Horizontal in/out meter pair for the header.
class IoMeters : public juce::Component {
public:
    explicit IoMeters(Processor& p) : proc(p) {}
    void tick();
    void paint(juce::Graphics&) override;
private:
    Processor& proc; float in = 0, out = 0;
};

// Base editor: fixed design size scaled to the window, a header with the wordmark and I/O meters,
// and modules drawn by the subclass. Subclasses add controls to `body` and lay them out in body coordinates.
class Editor : public juce::AudioProcessorEditor, private juce::Timer {
public:
    Editor(Processor&, const juce::String& name, const juce::String& subtitle, juce::Colour accent, int designW, int designH);
    ~Editor() override;
    void resized() override;
    void paint(juce::Graphics&) override {}
    // module helpers in body coordinates
    void module(juce::Graphics&, juce::Rectangle<int>, const juce::String& title, const juce::String& code = {});
    juce::Colour accent;
protected:
    virtual void layout(juce::Rectangle<int> area) = 0;    // area: below the header, inside the margins
    virtual void paintBody(juce::Graphics&) {}
    virtual void tick() {}                                  // ~30 Hz, for displays and meters
    void setReady() { ready = true; resized(); }            // call at the end of the subclass constructor
    Processor& proc;
    station::Look look;
    struct Body : juce::Component { Editor* ed = nullptr; void paint(juce::Graphics& g) override; } body;
    const int designW, designH;
private:
    void timerCallback() override;
    juce::String name, subtitle;
    IoMeters io;
    class PresetBar : public juce::Component {
    public:
        explicit PresetBar(Processor& p);
        void paint(juce::Graphics&) override;
        void resized() override;
        void mouseUp(const juce::MouseEvent&) override;
        void tick();
    private:
        void step(int d);
        void openMenu();
        Processor& proc; juce::TextButton prev{ "<" }, next{ ">" }; int shown = -1; bool modified = false;
        juce::Rectangle<int> nameArea;
    } presetBar;
    bool ready = false;
    juce::TooltipWindow tips{ nullptr, 600 };
};

// Log-frequency axis helpers for displays.
struct FreqAxis {
    float lo = 20, hi = 20000;
    float x(float f, juce::Rectangle<float> r) const { return r.getX() + r.getWidth() * std::log(f / lo) / std::log(hi / lo); }
    float f(float x, juce::Rectangle<float> r) const { return lo * std::pow(hi / lo, (x - r.getX()) / r.getWidth()); }
    void drawGrid(juce::Graphics&, juce::Rectangle<float>, bool labels = true) const;
};

// Smoothed spectrum of a scope feed, for display (dB per pixel column).
class Spectrum {
public:
    explicit Spectrum(int order = 12);
    // Reads the newest block from the feed and updates the smoothed magnitudes.
    void update(const ScopeFeed&, double sampleRate);
    // dB at frequency f (interpolated), smoothed over time.
    float dbAt(float f) const;
    juce::Path path(juce::Rectangle<float> r, const FreqAxis&, float dbLo, float dbHi, bool closed) const;
private:
    juce::dsp::FFT fft; int n; std::vector<float> win, work, mags; double sr = 48000;
};
} // namespace lgn
