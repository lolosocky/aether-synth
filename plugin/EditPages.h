// Sound-editing pages: Layers (oscillators, filters, envelopes), Mod (LFOs, mod envelopes, matrix), FX, Arp.
#pragma once
#include "PluginProcessor.h"

namespace edit {

enum class Curve { lin, integer, expo, pow3 };
enum class Fmt { pct, sec, hz, st, cents, integer, db, ratio, raw, shape, vowel, pan, bip, ms, gsz, gden, gpit, bal, scan, rate, freq, bits, div };

// One knob, menu or switch bound to a dotted patch path (or to a host parameter).
class Control : public juce::Component {
public:
    enum class Kind { knob, menu, toggle };
    Control(AetherProcessor& p, Kind k, std::string path, juce::String label);
    ~Control() override;
    // knob setup
    Control& range(double lo, double hi, Curve c, double def, Fmt f);
    // menu setup: values stored as strings (names) or as their index
    Control& choices(const juce::StringArray& names, bool storeAsString, const juce::StringArray& values = {});
    Control& onEdit(std::function<void()> f) { after = std::move(f); return *this; }
    Control& resets() { resetVoices = true; return *this; }
    Control& host(const char* paramId);
    void load(const aether::json& patch);
    void resized() override;
    int prefWidth() const { return kind == Kind::knob ? 70 : kind == Kind::menu ? 150 : juce::jmax(100, text.length() * 8 + 34); }
    int prefHeight() const { return kind == Kind::knob ? 84 : kind == Kind::menu ? 46 : 30; }
private:
    void push(const aether::json& v);
    AetherProcessor& proc; Kind kind; std::string path; juce::String text;
    juce::Slider slider; juce::ComboBox box; juce::ToggleButton tog; juce::Label label;
    juce::StringArray values; bool asString = false, resetVoices = false, loading = false;
    std::function<void()> after;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> att;
};

// A titled group of controls that lays itself out as a wrapping row.
struct Section {
    juce::String title;
    std::vector<juce::Component*> items;
    juce::Component* header = nullptr;   // optional control shown next to the title (e.g. an on switch)
};

// Base for an editing page: owns its controls and sections and stacks the sections as cards in a scrolling column.
class Page : public juce::Component {
public:
    explicit Page(AetherProcessor& p) : proc(p) {}
    virtual void rebuild() = 0;                       // recreate controls (structure changed)
    void refresh();                                   // reload values from the patch
    void paint(juce::Graphics&) override;
    void resized() override { layout(getWidth()); }
    int layout(int width);                            // returns the height needed
protected:
    Control& knob(const std::string& path, const juce::String& label, double lo, double hi, Curve c, double def, Fmt f);
    Control& menu(const std::string& path, const juce::String& label, const juce::StringArray& names, bool asString, const juce::StringArray& values = {});
    Control& toggle(const std::string& path, const juce::String& label);
    template <typename T> T& own(std::unique_ptr<T> c) { T& r = *c; addAndMakeVisible(r); extras.push_back(std::move(c)); return r; }
    void clear();
    Section& section(const juce::String& title) { sections.push_back({ title, {}, nullptr }); return sections.back(); }
    AetherProcessor& proc;
    std::vector<std::unique_ptr<Control>> controls;
    std::vector<std::unique_ptr<juce::Component>> extras;
    std::vector<Section> sections;
    std::vector<juce::Rectangle<int>> cards;
};

class LayersPage : public Page {
public:
    explicit LayersPage(AetherProcessor& p);
    void rebuild() override;
    void resized() override;
    int layer = 0;
private:
    juce::TextButton tabs[4];
    juce::TextButton copyBtn{ "Copy" }, pasteBtn{ "Paste" }, resetBtn{ "Reset layer" };
    aether::json clip;
};

class ModPage : public Page {
public:
    explicit ModPage(AetherProcessor& p) : Page(p) { rebuild(); }
    void rebuild() override;
};

class FxPage : public Page {
public:
    explicit FxPage(AetherProcessor& p) : Page(p) { rebuild(); }
    void rebuild() override;
};

class ArpPage : public Page {
public:
    explicit ArpPage(AetherProcessor& p) : Page(p) { rebuild(); }
    void rebuild() override;
};

class SectionsPage : public Page, private juce::Timer {
public:
    explicit SectionsPage(AetherProcessor& p) : Page(p) { rebuild(); startTimerHz(15); }
    ~SectionsPage() override { stopTimer(); }
    void rebuild() override;
private:
    void timerCallback() override;
    juce::TextButton* secBtn[4] = {};
    juce::Label* nowLabel = nullptr;
    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment>> batt;
    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment>> catt;
};

class CratePage : public Page, private juce::Timer {
public:
    explicit CratePage(AetherProcessor& p) : Page(p) { rebuild(); startTimerHz(20); }
    ~CratePage() override { stopTimer(); }
    void rebuild() override;
private:
    void timerCallback() override;
    juce::Label* status = nullptr;
    juce::Component* view = nullptr;
};

// Scrolling wrapper so long pages work at any window size.
class Scroller : public juce::Viewport {
public:
    explicit Scroller(Page& pg) : page(pg) { setViewedComponent(&page, false); setScrollBarsShown(true, false); setScrollBarThickness(8); }
    void resized() override { juce::Viewport::resized(); const int w = getMaximumVisibleWidth(); page.setSize(w, juce::jmax(getHeight(), page.layout(w))); }
    void relayout() { resized(); }
    Page& page;
};

} // namespace edit
