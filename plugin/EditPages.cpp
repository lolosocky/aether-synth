#include "EditPages.h"
#include "PluginEditor.h"

using aether::json;
namespace edit {

// ------------------------------------------------------------------ value formatting
static juce::String fmtValue(Fmt f, double v) {
    auto pct = [](double x) { return juce::String(juce::roundToInt(x * 100)) + "%"; };
    switch (f) {
        case Fmt::pct: return pct(v);
        case Fmt::sec: return v < 1 ? juce::String(juce::roundToInt(v * 1000)) + " ms" : juce::String(v, v < 10 ? 2 : 1) + " s";
        case Fmt::ms: return juce::String(juce::roundToInt(v * 1000)) + " ms";
        case Fmt::hz: { const double hz = 20 * std::pow(2.0, v * 9.9658); return hz >= 1000 ? juce::String(hz / 1000, 1) + " kHz" : juce::String(juce::roundToInt(hz)) + " Hz"; }
        case Fmt::freq: return v >= 1000 ? juce::String(v / 1000, 1) + " kHz" : juce::String(juce::roundToInt(v)) + " Hz";
        case Fmt::st: return (v > 0 ? "+" : "") + juce::String(juce::roundToInt(v)) + " st";
        case Fmt::cents: return (v > 0 ? "+" : "") + juce::String(juce::roundToInt(v)) + " ct";
        case Fmt::integer: return juce::String(juce::roundToInt(v));
        case Fmt::db: return (v > 0 ? "+" : "") + juce::String(v, 1) + " dB";
        case Fmt::ratio: return juce::String(v, 2) + "x";
        case Fmt::rate: return v < 1 ? juce::String(v, 2) + " Hz" : juce::String(v, 1) + " Hz";
        case Fmt::bits: return juce::String(v, 1);
        case Fmt::div: return juce::String::fromUTF8("\xc3\xb7") + juce::String(v, 1);
        case Fmt::shape: { static const char* n[4] = { "Sine", "Tri", "Saw", "Pulse" }; const double x = v * 3; const int i = juce::roundToInt(x);
            if (std::abs(x - i) < 0.06) return n[juce::jlimit(0, 3, i)]; const int a = juce::jlimit(0, 3, (int)std::floor(x)); return juce::String(n[a]) + ">" + n[juce::jmin(3, a + 1)]; }
        case Fmt::vowel: { const char* n = "AEIOU"; const double x = v * 4; const int i = juce::roundToInt(x);
            if (std::abs(x - i) < 0.12) return juce::String::charToString(n[juce::jlimit(0, 4, i)]); const int a = juce::jlimit(0, 4, (int)std::floor(x)); return juce::String::charToString(n[a]) + ">" + juce::String::charToString(n[juce::jmin(4, a + 1)]); }
        case Fmt::pan: return std::abs(v) < 0.02 ? juce::String("C") : (v < 0 ? "L" : "R") + juce::String(juce::roundToInt(std::abs(v) * 100));
        case Fmt::bip: return (v > 0 ? "+" : "") + juce::String(juce::roundToInt(v * 100)) + "%";
        case Fmt::gsz: return juce::String(juce::roundToInt(5 * std::pow(200.0, v))) + " ms";
        case Fmt::gden: return juce::String(juce::roundToInt(std::pow(200.0, v))) + "/s";
        case Fmt::gpit: return juce::String::fromUTF8("\xc2\xb1") + juce::String(v * 12, 1) + " st";
        case Fmt::bal: return std::abs(v - 0.5) < 0.02 ? juce::String("50/50") : juce::String(juce::roundToInt((1 - v) * 100)) + "/" + juce::String(juce::roundToInt(v * 100));
        case Fmt::scan: return std::abs(v) < 0.005 ? juce::String("Freeze") : (v > 0 ? "+" : "") + juce::String(juce::roundToInt(v * 100)) + "%";
        case Fmt::raw: default: return juce::String(v, 2);
    }
}

static juce::NormalisableRange<double> makeRange(double lo, double hi, Curve c) {
    switch (c) {
        case Curve::integer: return { lo, hi, 1.0 };
        case Curve::expo: return { lo, hi, [](double a, double b, double x) { return a * std::pow(b / a, x); }, [](double a, double b, double v) { return std::log(juce::jmax(v, a) / a) / std::log(b / a); } };
        case Curve::pow3: return { lo, hi, [](double a, double b, double x) { return a + (b - a) * x * x * x; }, [](double a, double b, double v) { return std::cbrt(juce::jmax(0.0, (v - a) / (b - a))); } };
        case Curve::lin: default: return { lo, hi };
    }
}

static const json* walk(const json& root, const std::string& path) {
    const json* o = &root; juce::StringArray parts; parts.addTokens(juce::String(path), ".", "");
    for (const auto& p : parts) {
        if (o->is_array() && p.containsOnly("0123456789")) { const size_t i = (size_t)p.getIntValue(); if (i >= o->size()) return nullptr; o = &(*o)[i]; }
        else if (o->is_object()) { auto it = o->find(p.toStdString()); if (it == o->end()) return nullptr; o = &*it; }
        else return nullptr;
    }
    return o;
}

// ------------------------------------------------------------------ control
Control::Control(AetherProcessor& p, Kind k, std::string pth, juce::String lbl) : proc(p), kind(k), path(std::move(pth)), text(std::move(lbl)) {
    label.setText(text, juce::dontSendNotification); label.setJustificationType(kind == Kind::knob ? juce::Justification::centred : juce::Justification::centredLeft);
    label.setFont(juce::FontOptions(11.5f)); label.setColour(juce::Label::textColourId, ui::dim); label.setMinimumHorizontalScale(0.7f);
    label.setInterceptsMouseClicks(false, false);
    if (kind == Kind::knob) {
        slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        slider.setTextBoxStyle(juce::Slider::TextBoxBelow, true, 70, 15);
        slider.setColour(juce::Slider::textBoxTextColourId, ui::text.withAlpha(0.75f)); slider.setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
        slider.onValueChange = [this] { if (!loading && !att) push(slider.getValue()); };
        addAndMakeVisible(slider); addAndMakeVisible(label);
    } else if (kind == Kind::menu) {
        box.onChange = [this] {
            if (loading) return;
            const int i = box.getSelectedItemIndex(); if (i < 0) return;
            if (asString) push(values[i].toStdString()); else push(values.isEmpty() ? json(i) : json(values[i].getIntValue()));
        };
        addAndMakeVisible(box); addAndMakeVisible(label);
    } else {
        tog.setButtonText(text);
        tog.setColour(juce::ToggleButton::textColourId, ui::text); tog.setColour(juce::ToggleButton::tickColourId, ui::accent); tog.setColour(juce::ToggleButton::tickDisabledColourId, ui::dim);
        tog.onClick = [this] { if (!loading) push(tog.getToggleState() ? 1 : 0); };
        addAndMakeVisible(tog);
    }
}
Control::~Control() { att.reset(); }

Control& Control::range(double lo, double hi, Curve c, double def, Fmt f) {
    const juce::ScopedValueSetter<bool> guard(loading, true);
    slider.setNormalisableRange(makeRange(lo, hi, c));
    slider.setDoubleClickReturnValue(true, def);
    slider.textFromValueFunction = [f](double v) { return fmtValue(f, v); };
    slider.valueFromTextFunction = [](const juce::String& t) { return t.retainCharacters("-0123456789.").getDoubleValue(); };
    slider.updateText();
    return *this;
}
Control& Control::choices(const juce::StringArray& names, bool storeAsString, const juce::StringArray& vals) {
    const juce::ScopedValueSetter<bool> guard(loading, true);
    asString = storeAsString; values = vals.isEmpty() && storeAsString ? names : vals;
    box.clear(juce::dontSendNotification);
    for (int i = 0; i < names.size(); i++) box.addItem(names[i], i + 1);
    return *this;
}
Control& Control::host(const char* paramId) {
    const juce::ScopedValueSetter<bool> guard(loading, true);
    att = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(proc.apvts, paramId, slider);
    return *this;
}
void Control::push(const json& v) {
    proc.setPatchValue(path, v, resetVoices);
    if (after) after();
}
void Control::load(const json& patch) {
    if (att) return;
    const json* v = walk(patch, path);
    const juce::ScopedValueSetter<bool> guard(loading, true);
    if (kind == Kind::knob) { if (v && v->is_number()) slider.setValue(v->get<double>(), juce::dontSendNotification); }
    else if (kind == Kind::toggle) { tog.setToggleState(v && ((v->is_number() && v->get<double>() != 0) || (v->is_boolean() && v->get<bool>())), juce::dontSendNotification); }
    else {
        int idx = -1;
        if (v && v->is_string()) idx = values.indexOf(juce::String(v->get<std::string>()));
        else if (v && v->is_number()) { const int n = (int)std::lround(v->get<double>()); idx = values.isEmpty() ? n : values.indexOf(juce::String(n)); }
        box.setSelectedItemIndex(juce::jmax(0, idx), juce::dontSendNotification);
    }
}
void Control::resized() {
    auto r = getLocalBounds();
    if (kind == Kind::knob) { label.setBounds(r.removeFromBottom(15)); slider.setBounds(r); }
    else if (kind == Kind::menu) { label.setBounds(r.removeFromTop(16)); box.setBounds(r.removeFromTop(28)); }
    else tog.setBounds(r);
}

// ------------------------------------------------------------------ page
Control& Page::knob(const std::string& path, const juce::String& label, double lo, double hi, Curve c, double def, Fmt f) {
    controls.push_back(std::make_unique<Control>(proc, Control::Kind::knob, path, label)); auto& k = *controls.back(); k.range(lo, hi, c, def, f); addAndMakeVisible(k);
    if (!sections.empty()) sections.back().items.push_back(&k);
    return k;
}
Control& Page::menu(const std::string& path, const juce::String& label, const juce::StringArray& names, bool asString, const juce::StringArray& values) {
    controls.push_back(std::make_unique<Control>(proc, Control::Kind::menu, path, label)); auto& k = *controls.back(); k.choices(names, asString, values); addAndMakeVisible(k);
    if (!sections.empty()) sections.back().items.push_back(&k);
    return k;
}
Control& Page::toggle(const std::string& path, const juce::String& label) {
    controls.push_back(std::make_unique<Control>(proc, Control::Kind::toggle, path, label)); auto& k = *controls.back(); addAndMakeVisible(k);
    if (!sections.empty()) sections.back().items.push_back(&k);
    return k;
}
void Page::clear() { sections.clear(); controls.clear(); extras.clear(); cards.clear(); }
void Page::refresh() { const json p = proc.patchJson(); for (auto& c : controls) c->load(p); }

static int itemW(juce::Component* c) { if (auto* k = dynamic_cast<Control*>(c)) return k->prefWidth(); return c->getProperties().getWithDefault("w", 110); }
static int itemH(juce::Component* c) { if (auto* k = dynamic_cast<Control*>(c)) return k->prefHeight(); return c->getProperties().getWithDefault("h", 30); }

int Page::layout(int width) {
    const int pad = 10, gap = 10, titleH = 22, top = (int)getProperties().getWithDefault("top", 0);
    cards.clear();
    int x = 0, y = top, rowH = 0;
    for (auto& s : sections) {
        // natural card width: items on one line, capped at the page width
        int natural = pad * 2; for (auto* c : s.items) natural += itemW(c) + 4;
        const int w = juce::jmin(width, juce::jmax(160, natural));
        // height after wrapping
        int lx = 0, lines = 0, lineH = 0, h = 0;
        for (auto* c : s.items) { const int iw = itemW(c) + 4; if (lx > 0 && lx + iw > w - pad * 2) { h += lineH + 4; lx = 0; lineH = 0; } lx += iw; lineH = juce::jmax(lineH, itemH(c)); lines++; }
        h += lineH; const int cardH = titleH + h + pad + 4;
        if (x > 0 && x + w > width) { x = 0; y += rowH + gap; rowH = 0; }
        const juce::Rectangle<int> card(x, y, w, cardH); cards.push_back(card);
        if (s.header) s.header->setBounds(card.getRight() - pad - itemW(s.header), card.getY() + 3, itemW(s.header), 18);
        // place items
        int ix = card.getX() + pad, iy = card.getY() + titleH; lineH = 0;
        for (auto* c : s.items) {
            const int iw = itemW(c), ih = itemH(c);
            if (ix > card.getX() + pad && ix + iw > card.getRight() - pad) { ix = card.getX() + pad; iy += lineH + 4; lineH = 0; }
            c->setBounds(ix, iy, iw, ih); ix += iw + 4; lineH = juce::jmax(lineH, ih);
        }
        x += w + gap; rowH = juce::jmax(rowH, cardH);
    }
    return y + rowH + 8;
}
void Page::paint(juce::Graphics& g) {
    for (size_t i = 0; i < cards.size() && i < sections.size(); i++) {
        const auto r = cards[i].toFloat();
        g.setColour(ui::panel); g.fillRoundedRectangle(r, 8);
        g.setColour(ui::line); g.drawRoundedRectangle(r.reduced(0.5f), 8, 1);
        g.setColour(ui::dim); g.setFont(juce::FontOptions(11.0f, juce::Font::bold));
        g.drawText(sections[i].title.toUpperCase(), cards[i].withHeight(22).reduced(10, 0), juce::Justification::centredLeft);
    }
}

// ------------------------------------------------------------------ shared lists
static juce::StringArray toSA(const std::vector<std::string>& v) { juce::StringArray a; for (auto& s : v) a.add(s); return a; }
static const juce::StringArray& divNames() { static const juce::StringArray d{ "8 bar", "4 bar", "2 bar", "1 bar", "1/2", "1/2T", "1/4.", "1/4", "1/4T", "1/8.", "1/8", "1/8T", "1/16.", "1/16", "1/16T", "1/32" }; return d; }
static juce::StringArray divsFrom(int start) { juce::StringArray a; for (int i = start; i < divNames().size(); i++) a.add(divNames()[i]); return a; }

// ------------------------------------------------------------------ layers
LayersPage::LayersPage(AetherProcessor& p) : Page(p) {
    for (int i = 0; i < 4; i++) {
        tabs[i].setClickingTogglesState(false);
        tabs[i].onClick = [this, i] { layer = i; rebuild(); };
        addAndMakeVisible(tabs[i]);
    }
    copyBtn.onClick = [this] { const json pj = proc.patchJson(); if (pj.contains("L") && pj["L"].size() > (size_t)layer) clip = pj["L"][(size_t)layer]; pasteBtn.setEnabled(true); };
    pasteBtn.onClick = [this] { if (!clip.is_null()) { proc.setPatchValue("L." + std::to_string(layer), clip, true); rebuild(); } };
    resetBtn.onClick = [this] { json d = Library::get().data.value("defLayer", json::object()); d["on"] = 1; proc.setPatchValue("L." + std::to_string(layer), d, true); rebuild(); };
    pasteBtn.setEnabled(false);
    for (auto* b : { &copyBtn, &pasteBtn, &resetBtn }) addAndMakeVisible(b);
    getProperties().set("top", 40);
    rebuild();
}
void LayersPage::resized() {
    auto r = getLocalBounds().removeFromTop(32);
    for (int i = 0; i < 4; i++) tabs[i].setBounds(r.removeFromLeft(juce::jmin(150, (getWidth() - 270) / 4)).reduced(0, 0).withTrimmedRight(6));
    resetBtn.setBounds(r.removeFromRight(96)); r.removeFromRight(6); pasteBtn.setBounds(r.removeFromRight(60)); r.removeFromRight(6); copyBtn.setBounds(r.removeFromRight(60));
    Page::resized();
}
void LayersPage::rebuild() {
    clear();
    const json pj = proc.patchJson();
    static const char* nice[8] = { "Analog", "Wavetable", "String", "Resonator", "Vocal", "Noise", "Granular", "Sample" };
    aether::Patch P = aether::patchFromJson(pj);
    for (int i = 0; i < 4; i++) {
        tabs[i].setButtonText(juce::String::charToString((juce::juce_wchar)('A' + i)) + "  " + (P.L[i].on ? nice[juce::jlimit(0, 7, P.L[i].mode)] : "off"));
        tabs[i].setColour(juce::TextButton::buttonColourId, i == layer ? ui::accent.withAlpha(0.35f) : ui::panel);
        tabs[i].setColour(juce::TextButton::textColourOffId, P.L[i].on ? ui::text : ui::dim);
    }
    const aether::Layer& L = P.L[layer];
    const std::string b = "L." + std::to_string(layer) + ".";
    auto re = [sp = juce::Component::SafePointer<LayersPage>(this)] { juce::MessageManager::callAsync([sp] { if (auto* pg = sp.getComponent()) pg->rebuild(); }); };

    { auto& s = section("Layer " + juce::String::charToString((juce::juce_wchar)('A' + layer)));
      toggle(b + "on", "Layer on").onEdit(re);
      menu(b + "mode", "Engine", juce::StringArray(nice, 8), true, toSA(aether::modeNames())).resets().onEdit(re);
      (void)s; }

    { section("Oscillator");
      switch (L.mode) {
        case 0: knob(b + "shp", "Shape", 0, 1, Curve::lin, 0.667, Fmt::shape); knob(b + "pw", "Pulse W", 0.02, 0.98, Curve::lin, 0.5, Fmt::pct); knob(b + "sync", "Sync", 0, 1, Curve::lin, 0, Fmt::pct); break;
        case 1: menu(b + "wt", "Wavetable", toSA(aether::wtNames()), true); knob(b + "pos", "Position", 0, 1, Curve::lin, 0, Fmt::pct); knob(b + "sync", "Sync", 0, 1, Curve::lin, 0, Fmt::pct); break;
        case 2: knob(b + "pdec", "Decay", 0.05, 15, Curve::expo, 2, Fmt::sec); knob(b + "pdmp", "Damping", 0, 1, Curve::lin, 0.4, Fmt::pct); knob(b + "ppos", "Pick Pos", 0.02, 0.5, Curve::lin, 0.2, Fmt::pct); knob(b + "pexc", "Excite", 0, 1, Curve::lin, 0.6, Fmt::pct); break;
        case 3: { juce::StringArray m; for (auto& x : aether::models()) m.add(x.name); menu(b + "mdl", "Model", m, true);
                  knob(b + "mbri", "Bright", 0, 1, Curve::lin, 0.5, Fmt::pct); knob(b + "mdec", "Decay", 0.05, 20, Curve::expo, 1.5, Fmt::sec); knob(b + "mhard", "Hardness", 0, 1, Curve::lin, 0.5, Fmt::pct); knob(b + "minh", "Inharm", 0, 1, Curve::lin, 0, Fmt::pct); knob(b + "mbow", "Bow", 0, 1, Curve::lin, 0, Fmt::pct); } break;
        case 4: knob(b + "vow", "Vowel", 0, 1, Curve::lin, 0, Fmt::vowel); knob(b + "fsh", "Formant", 0, 1, Curve::lin, 0.5, Fmt::pct); knob(b + "brth", "Breath", 0, 1, Curve::lin, 0.1, Fmt::pct); knob(b + "shp", "Source", 0, 1, Curve::lin, 0.667, Fmt::pct); break;
        case 5: menu(b + "ntyp", "Noise type", { "Color", "Crackle", "Rain", "Digital", "Wind" }, false); knob(b + "ncol", "Color", 0, 1, Curve::lin, 0.6, Fmt::pct); break;
        case 6: menu(b + "src", "Source", toSA(aether::sourceNames()), true).resets();
                knob(b + "gpos", "Position", 0, 1, Curve::lin, 0.3, Fmt::pct); knob(b + "gspr", "Spray", 0, 1, Curve::lin, 0.1, Fmt::pct); knob(b + "gsz", "Grain", 0, 1, Curve::lin, 0.45, Fmt::gsz); knob(b + "gden", "Density", 0, 1, Curve::lin, 0.55, Fmt::gden);
                knob(b + "gpit", "Pitch Rand", 0, 1, Curve::lin, 0, Fmt::gpit); knob(b + "grev", "Reverse", 0, 1, Curve::lin, 0, Fmt::pct); knob(b + "gscan", "Scan", -1, 1, Curve::lin, 0, Fmt::scan); break;
        default: menu(b + "src", "Source", toSA(aether::sourceNames()), true).resets(); knob(b + "sst", "Start", 0, 1, Curve::lin, 0, Fmt::pct); toggle(b + "sloop", "Loop"); break;
      } }

    const bool hasUni = L.mode == 0 || L.mode == 1 || L.mode == 2 || L.mode == 4;
    { section("Pitch & Unison");
      knob(b + "crs", "Coarse", -48, 48, Curve::integer, 0, Fmt::st); knob(b + "fin", "Fine", -100, 100, Curve::lin, 0, Fmt::cents);
      if (hasUni) { knob(b + "uni", "Voices", 1, L.mode == 2 ? 3 : 7, Curve::integer, 1, Fmt::integer); knob(b + "det", "Detune", 0, 100, Curve::lin, 15, Fmt::cents); }
      knob(b + "spr", "Spread", 0, 1, Curve::lin, 0.5, Fmt::pct);
      if (L.mode == 0 || L.mode == 1 || L.mode == 4) knob(b + "drift", "Drift", 0, 1, Curve::lin, 0.15, Fmt::pct);
      toggle(b + "kt", "Key tracking"); }

    if (L.mode == 0 || L.mode == 1) {
        section("Modifiers");
        knob(b + "fm", "FM", 0, 1, Curve::lin, 0, Fmt::pct); knob(b + "fmr", "FM Ratio", 0.25, 16, Curve::expo, 1, Fmt::ratio); knob(b + "ring", "Ring", 0, 1, Curve::lin, 0, Fmt::pct);
        knob(b + "rngr", "Ring Ratio", 0.25, 16, Curve::expo, 2, Fmt::ratio); knob(b + "fold", "Fold", 0, 1, Curve::lin, 0, Fmt::pct); knob(b + "sub", "Sub", 0, 1, Curve::lin, 0, Fmt::pct); knob(b + "nz", "Noise", 0, 1, Curve::lin, 0, Fmt::pct);
    }

    const juce::StringArray ft{ "Off", "LP 12", "LP 24", "Ladder", "HP 12", "HP 24", "Band", "Notch", "Peak", "Comb +", "Comb -", "Vowel" };
    for (int f = 1; f <= 2; f++) {
        const std::string fb = b + "f" + std::to_string(f);
        section("Filter " + juce::String(f));
        menu(fb + "t", "Type", ft, true, toSA(aether::ftypeNames()));
        knob(fb + "c", "Cutoff", 0, 1, Curve::lin, f == 1 ? 0.8 : 0.6, Fmt::hz); knob(fb + "r", "Reso", 0, 1, Curve::lin, 0.1, Fmt::pct); knob(fb + "d", "Drive", 0, 1, Curve::lin, 0, Fmt::pct);
        knob(fb + "k", "Key", 0, 1, Curve::lin, 0.5, Fmt::pct); knob(fb + "e", "Env", -1, 1, Curve::lin, 0, Fmt::bip); knob(fb + "v", "Vel", 0, 1, Curve::lin, 0, Fmt::pct);
    }
    { section("Filter Routing"); menu(b + "frt", "Routing", { "Series", "Parallel" }, false); knob(b + "fbal", "Balance", 0, 1, Curve::lin, 0.5, Fmt::bal); }
    { section("Amp"); knob(b + "lvl", "Level", 0, 1.5, Curve::lin, 0.8, Fmt::pct); knob(b + "pan", "Pan", -1, 1, Curve::lin, 0, Fmt::pan); knob(b + "vs", "Vel Sens", 0, 1, Curve::lin, 0.6, Fmt::pct); }
    { section("Amp Envelope"); knob(b + "aa", "Attack", 0, 20, Curve::pow3, 0.005, Fmt::sec); knob(b + "ad", "Decay", 0, 20, Curve::pow3, 0.5, Fmt::sec); knob(b + "as", "Sustain", 0, 1, Curve::lin, 1, Fmt::pct); knob(b + "ar", "Release", 0, 20, Curve::pow3, 0.3, Fmt::sec); }
    { section("Filter Envelope"); knob(b + "fa", "Attack", 0, 20, Curve::pow3, 0.005, Fmt::sec); knob(b + "fd", "Decay", 0, 20, Curve::pow3, 0.5, Fmt::sec); knob(b + "fs", "Sustain", 0, 1, Curve::lin, 0.3, Fmt::pct); knob(b + "fr", "Release", 0, 20, Curve::pow3, 0.5, Fmt::sec); }
    refresh();
    if (auto* sc = findParentComponentOfClass<Scroller>()) sc->relayout(); else resized();
    repaint();
}

// ------------------------------------------------------------------ mod
namespace {
const juce::StringArray& srcLabels() { static const juce::StringArray s{ "-", "Mod Env 1", "Mod Env 2", "LFO 1", "LFO 2", "LFO 3", "LFO 4", "Velocity", "Key Track", "Mod Wheel", "Aftertouch", "Random", "Macro 1", "Macro 2", "Macro 3", "Macro 4", "Orb X", "Orb Y", "Alternate", "Pitch Bend" }; return s; }
const juce::StringArray& ldLabels() { static const juce::StringArray s{ "Pitch", "Shape", "Pulse Width", "Sync", "WT Position", "FM", "Ring Mod", "Fold", "Sub", "Noise", "Detune", "Spread", "String Damping", "Resonator Bright", "Vowel", "Formant", "Breath", "Noise Color", "Grain Position", "Grain Size", "Grain Density", "Grain Spray", "Grain Pitch Rand", "Sample Start", "Filter 1 Cutoff", "Filter 1 Reso", "Filter 1 Drive", "Filter 2 Cutoff", "Filter 2 Reso", "Filter Balance", "Level", "Pan", "Amp Attack", "Amp Decay", "Amp Release", "Filter Env Decay" }; return s; }
const juce::StringArray& fxLabels() { static const juce::StringArray s{ "Drive Amount", "LoFi Mix", "Chorus Mix", "Phaser Mix", "Gate Depth", "Delay Mix", "Delay Feedback", "Reverb Mix", "EQ Low", "EQ High", "Stereo Width" }; return s; }

// One routing in the matrix: source, via, destination, amount, remove.
class ModRow : public juce::Component {
public:
    ModRow(AetherProcessor& p, int index, const json& m, std::function<void()> changed) : proc(p), idx(index), onStructure(std::move(changed)) {
        const auto& ids = aether::srcIds();
        for (int i = 1; i < (int)ids.size(); i++) src.addItem(srcLabels()[i], i);
        via.addItem("No via", 100); for (int i = 1; i < (int)ids.size(); i++) via.addItem(srcLabels()[i], i);
        // destinations, grouped in submenus
        auto* root = dst.getRootMenu(); int id = 1;
        const char* groups[5] = { "Layer A", "Layer B", "Layer C", "Layer D", "All layers" }; const char* pre[5] = { "A", "B", "C", "D", "*" };
        for (int g = 0; g < 5; g++) { juce::PopupMenu sub; for (int k = 0; k < ldLabels().size(); k++) { sub.addItem(id, ldLabels()[k]); dests.add(juce::String(pre[g]) + "." + aether::ldstNames()[(size_t)k]); destNames.add(juce::String(pre[g] == std::string("*") ? "All" : pre[g]) + " / " + ldLabels()[k]); id++; } root->addSubMenu(groups[g], sub); }
        { juce::PopupMenu sub; for (int k = 0; k < 4; k++) { sub.addItem(id++, "LFO " + juce::String(k + 1) + " Rate"); dests.add("G.l" + juce::String(k + 1) + "r"); destNames.add("LFO " + juce::String(k + 1) + " Rate"); } root->addSubMenu("LFO rates", sub); }
        { juce::PopupMenu sub; for (int k = 0; k < fxLabels().size(); k++) { sub.addItem(id++, fxLabels()[k]); dests.add("FX." + juce::String(aether::fxdstNames()[(size_t)k])); destNames.add("FX / " + fxLabels()[k]); } root->addSubMenu("Effects", sub); }
        amt.setSliderStyle(juce::Slider::LinearHorizontal); amt.setRange(-1, 1, 0.001); amt.setDoubleClickReturnValue(true, 0);
        amt.setTextBoxStyle(juce::Slider::TextBoxRight, false, 52, 20);
        del.setButtonText("x");
        for (juce::Component* c : { (juce::Component*)&src, (juce::Component*)&via, (juce::Component*)&dst, (juce::Component*)&amt, (juce::Component*)&del }) addAndMakeVisible(c);
        // load
        src.setSelectedId(juce::jmax(1, aether::indexOf(ids, aether::jstr(m, "s", "l1"))), juce::dontSendNotification);
        const int v = aether::indexOf(ids, aether::jstr(m, "v", "")); via.setSelectedId(v > 0 ? v : 100, juce::dontSendNotification);
        const int d = dests.indexOf(juce::String(aether::jstr(m, "d", "A.f1c"))); dst.setText(d >= 0 ? destNames[d] : juce::String("-"), juce::dontSendNotification);
        amt.setValue(aether::jnum(m, "a", 0), juce::dontSendNotification);
        src.onChange = [this] { set("s", aether::srcIds()[(size_t)src.getSelectedId()]); };
        via.onChange = [this] { const int i = via.getSelectedId(); set("v", i == 100 ? std::string() : aether::srcIds()[(size_t)i]); };
        dst.onChange = [this] { const int i = dst.getSelectedId() - 1; if (i >= 0 && i < dests.size()) { set("d", dests[i].toStdString()); dst.setText(destNames[i], juce::dontSendNotification); } };
        amt.onValueChange = [this] { set("a", amt.getValue()); };
        del.onClick = [this] { const int i = idx; auto cb = onStructure; auto& pr = proc;
            juce::MessageManager::callAsync([&pr, i, cb] { pr.editPatch([i](json& p) { if (p.contains("mods") && p["mods"].is_array() && (size_t)i < p["mods"].size()) p["mods"].erase((size_t)i); }); if (cb) cb(); }); };
    }
    void resized() override {
        auto r = getLocalBounds();
        del.setBounds(r.removeFromRight(28).reduced(0, 3)); r.removeFromRight(6);
        const int w = (r.getWidth() - 18) / 4;
        src.setBounds(r.removeFromLeft(w).reduced(0, 2)); r.removeFromLeft(6); via.setBounds(r.removeFromLeft(w).reduced(0, 2)); r.removeFromLeft(6);
        dst.setBounds(r.removeFromLeft(w + w / 2).reduced(0, 2)); r.removeFromLeft(6); amt.setBounds(r);
    }
private:
    void set(const char* key, const json& v) { proc.setPatchValue("mods." + std::to_string(idx) + "." + key, v); }
    AetherProcessor& proc; int idx; std::function<void()> onStructure;
    juce::ComboBox src, via, dst; juce::Slider amt; juce::TextButton del;
    juce::StringArray dests, destNames;
};
} // namespace

void ModPage::rebuild() {
    clear();
    const json pj = proc.patchJson();
    auto re = [sp = juce::Component::SafePointer<ModPage>(this)] { juce::MessageManager::callAsync([sp] { if (auto* pg = sp.getComponent()) pg->rebuild(); }); };
    const juce::StringArray shapes{ "Sine", "Triangle", "Ramp Up", "Ramp Down", "Square", "Sample & Hold", "Smooth Random" };
    for (int i = 0; i < 4; i++) {
        const std::string b = "lfo." + std::to_string(i) + ".";
        const bool sync = pj.contains("lfo") && pj["lfo"].size() > (size_t)i && aether::jflag(pj["lfo"][(size_t)i], "sync", 0);
        section("LFO " + juce::String(i + 1));
        menu(b + "shp", "Shape", shapes, false);
        if (sync) menu(b + "div", "Rate", divNames(), true); else knob(b + "rate", "Rate", 0.01, 40, Curve::expo, 1, Fmt::rate);
        knob(b + "fade", "Fade In", 0, 10, Curve::pow3, 0, Fmt::sec); knob(b + "ph", "Phase", 0, 1, Curve::lin, 0, Fmt::pct);
        toggle(b + "sync", "Tempo sync").onEdit(re); toggle(b + "rtg", "Retrigger");
    }
    for (int i = 0; i < 2; i++) {
        const std::string b = "me." + std::to_string(i) + ".";
        section("Mod Env " + juce::String(i + 1));
        knob(b + "a", "Attack", 0, 20, Curve::pow3, 0.005, Fmt::sec); knob(b + "d", "Decay", 0, 20, Curve::pow3, 0.5, Fmt::sec); knob(b + "s", "Sustain", 0, 1, Curve::lin, 0, Fmt::pct); knob(b + "r", "Release", 0, 20, Curve::pow3, 0.5, Fmt::sec);
        toggle(b + "loop", "Loop");
    }
    {
        auto& s = section("Modulation Matrix");
        const json mods = pj.contains("mods") && pj["mods"].is_array() ? pj["mods"] : json::array();
        for (size_t i = 0; i < mods.size(); i++) {
            auto& row = own(std::make_unique<ModRow>(proc, (int)i, mods[i], re));
            row.getProperties().set("w", 640); row.getProperties().set("h", 30); s.items.push_back(&row);
        }
        auto& add = own(std::make_unique<juce::TextButton>("+ Add routing"));
        add.getProperties().set("w", 130); add.getProperties().set("h", 28);
        add.onClick = [this, re] { proc.editPatch([](json& p) { if (!p.contains("mods") || !p["mods"].is_array()) p["mods"] = json::array(); if (p["mods"].size() < 24) p["mods"].push_back({ { "s", "l1" }, { "d", "A.f1c" }, { "a", 0 }, { "v", "" } }); }); re(); };
        s.items.push_back(&add);
        auto& hint = own(std::make_unique<juce::Label>());
        hint.setText("Effects respond to macros, the Orb, mod wheel, aftertouch and pitch bend.", juce::dontSendNotification);
        hint.setFont(juce::FontOptions(11.5f)); hint.setColour(juce::Label::textColourId, ui::dim);
        hint.getProperties().set("w", 500); hint.getProperties().set("h", 28); s.items.push_back(&hint);
    }
    {
        auto& s = section("Macro Names");
        for (int i = 0; i < 4; i++) {
            auto& ed = own(std::make_unique<juce::TextEditor>());
            std::string n = "Macro " + std::to_string(i + 1);
            if (pj.contains("mac") && pj["mac"].is_array() && pj["mac"].size() > (size_t)i) n = aether::jstr(pj["mac"][(size_t)i], "n", n);
            ed.setText(n, false); ed.setInputRestrictions(16); ed.setIndents(6, 0); ed.setJustification(juce::Justification::centredLeft);
            ed.onReturnKey = ed.onFocusLost = [this, &ed, i] { const auto t = ed.getText().trim(); proc.setPatchValue("mac." + std::to_string(i) + ".n", (t.isEmpty() ? "Macro " + juce::String(i + 1) : t).toStdString()); };
            ed.getProperties().set("w", 140); ed.getProperties().set("h", 28); s.items.push_back(&ed);
        }
    }
    refresh();
    if (auto* sc = findParentComponentOfClass<Scroller>()) sc->relayout(); else resized();
    repaint();
}

// ------------------------------------------------------------------ fx
void FxPage::rebuild() {
    clear();
    const json pj = proc.patchJson();
    auto re = [sp = juce::Component::SafePointer<FxPage>(this)] { juce::MessageManager::callAsync([sp] { if (auto* pg = sp.getComponent()) pg->rebuild(); }); };
    auto onSwitch = [this](Section& s, const std::string& path) { auto& t = *controls.emplace_back(std::make_unique<Control>(proc, Control::Kind::toggle, path, "On")); addAndMakeVisible(t); s.header = &t; t.getProperties().set("w", 60); };
    { auto& s = section("Drive"); onSwitch(s, "fx.drv.on");
      menu("fx.drv.type", "Type", { "Soft", "Tube", "Hard Clip", "Fuzz", "Wavefold" }, false);
      knob("fx.drv.amt", "Amount", 0, 1, Curve::lin, 0.3, Fmt::pct); knob("fx.drv.tone", "Tone", 0, 1, Curve::lin, 0.75, Fmt::pct); knob("fx.drv.mix", "Mix", 0, 1, Curve::lin, 1, Fmt::pct); }
    { auto& s = section("Lo-Fi"); onSwitch(s, "fx.lofi.on");
      knob("fx.lofi.bits", "Bits", 2, 16, Curve::lin, 12, Fmt::bits); knob("fx.lofi.rate", "Rate", 1, 40, Curve::expo, 1, Fmt::div); knob("fx.lofi.wow", "Wow", 0, 1, Curve::lin, 0.25, Fmt::pct); knob("fx.lofi.flut", "Flutter", 0, 1, Curve::lin, 0.15, Fmt::pct);
      knob("fx.lofi.noise", "Noise", 0, 1, Curve::lin, 0.15, Fmt::pct); knob("fx.lofi.tone", "Tone", 0, 1, Curve::lin, 0.7, Fmt::pct); knob("fx.lofi.mix", "Mix", 0, 1, Curve::lin, 1, Fmt::pct); }
    { auto& s = section("Trance Gate"); onSwitch(s, "fx.gate.on");
      std::vector<int> pat(16, 0);
      if (pj.contains("fx") && pj["fx"].contains("gate") && pj["fx"]["gate"].contains("pat")) { const auto& a = pj["fx"]["gate"]["pat"]; for (size_t i = 0; i < a.size() && i < 16; i++) pat[i] = a[i].is_number() ? (a[i].get<double>() != 0) : 0; }
      else for (int i = 0; i < 16; i++) pat[(size_t)i] = (i & 1) ? 0 : 1;
      for (int i = 0; i < 16; i++) {
          auto& st = own(std::make_unique<juce::TextButton>());
          st.setClickingTogglesState(true); st.setToggleState(pat[(size_t)i] != 0, juce::dontSendNotification);
          st.setColour(juce::TextButton::buttonOnColourId, ui::accent);
          st.onClick = [this, i, &st] { proc.editPatch([i, on = st.getToggleState()](json& p) { auto& g = p["fx"]["gate"]; if (!g.contains("pat") || !g["pat"].is_array()) { g["pat"] = json::array(); for (int k = 0; k < 16; k++) g["pat"].push_back((k & 1) ? 0 : 1); } g["pat"][(size_t)i] = on ? 1 : 0; }); };
          st.getProperties().set("w", 22); st.getProperties().set("h", 30); s.items.push_back(&st);
      }
      menu("fx.gate.div", "Rate", divsFrom(7), true); knob("fx.gate.depth", "Depth", 0, 1, Curve::lin, 1, Fmt::pct); knob("fx.gate.smooth", "Smooth", 0, 1, Curve::lin, 0.25, Fmt::pct); }
    { auto& s = section("Chorus"); onSwitch(s, "fx.cho.on");
      menu("fx.cho.mode", "Mode", { "Chorus", "Ensemble", "Flanger" }, false);
      knob("fx.cho.rate", "Rate", 0.02, 10, Curve::expo, 0.5, Fmt::rate); knob("fx.cho.depth", "Depth", 0, 1, Curve::lin, 0.5, Fmt::pct); knob("fx.cho.fb", "Feedback", -0.95, 0.95, Curve::lin, 0, Fmt::bip); knob("fx.cho.mix", "Mix", 0, 1, Curve::lin, 0.4, Fmt::pct); }
    { auto& s = section("Phaser"); onSwitch(s, "fx.pha.on");
      knob("fx.pha.rate", "Rate", 0.01, 10, Curve::expo, 0.25, Fmt::rate); knob("fx.pha.depth", "Depth", 0, 1, Curve::lin, 0.7, Fmt::pct); knob("fx.pha.fb", "Feedback", -0.95, 0.95, Curve::lin, 0.4, Fmt::bip); knob("fx.pha.mix", "Mix", 0, 1, Curve::lin, 0.5, Fmt::pct); }
    { auto& s = section("EQ"); onSwitch(s, "fx.eq.on");
      knob("fx.eq.low", "Low", -18, 18, Curve::lin, 0, Fmt::db); knob("fx.eq.mid", "Mid", -18, 18, Curve::lin, 0, Fmt::db); knob("fx.eq.mf", "Mid Freq", 100, 12000, Curve::expo, 1000, Fmt::freq); knob("fx.eq.high", "High", -18, 18, Curve::lin, 0, Fmt::db); }
    { section("Delay");
      const bool sync = !pj.contains("fx") || !pj["fx"].contains("dly") || aether::jflag(pj["fx"]["dly"], "sync", 1);
      toggle("fx.dly.sync", "Tempo sync").onEdit(re); toggle("fx.dly.pp", "Ping-pong");
      if (sync) menu("fx.dly.div", "Time", divsFrom(3), true); else knob("fx.dly.time", "Time", 0.01, 2, Curve::expo, 0.35, Fmt::ms);
      knob("fx.dly.fb", "Feedback", 0, 0.95, Curve::lin, 0.35, Fmt::pct); knob("fx.dly.lc", "Low Cut", 20, 2000, Curve::expo, 250, Fmt::freq); knob("fx.dly.hc", "High Cut", 500, 18000, Curve::expo, 5000, Fmt::freq);
      knob("fx.dly.mod", "Wobble", 0, 1, Curve::lin, 0.15, Fmt::pct); knob("fx.dly.mix", "Mix", 0, 1, Curve::lin, 0.2, Fmt::pct).host("delay"); }
    { section("Reverb");
      menu("fx.rev.type", "Space", { "Hall", "Room", "Plate", "Space" }, false);
      knob("fx.rev.size", "Size", 0, 1, Curve::lin, 0.6, Fmt::pct); knob("fx.rev.decay", "Decay", 0.2, 14, Curve::expo, 3, Fmt::sec); knob("fx.rev.damp", "Damping", 0, 1, Curve::lin, 0.5, Fmt::pct);
      knob("fx.rev.pre", "Pre-delay", 0, 0.25, Curve::lin, 0.02, Fmt::ms); knob("fx.rev.lc", "Low Cut", 20, 2000, Curve::expo, 180, Fmt::freq); knob("fx.rev.mix", "Mix", 0, 1, Curve::lin, 0.22, Fmt::pct).host("reverb"); }
    { auto& s = section("Compressor"); onSwitch(s, "fx.comp.on");
      knob("fx.comp.thr", "Threshold", -48, 0, Curve::lin, -18, Fmt::db); knob("fx.comp.ratio", "Ratio", 1, 20, Curve::expo, 3, Fmt::ratio); knob("fx.comp.gain", "Makeup", -12, 24, Curve::lin, 3, Fmt::db); }
    { section("Output & Voicing");
      knob("fx.width", "Width", 0, 2, Curve::lin, 1, Fmt::pct); knob("g.vol", "Patch Vol", 0, 1.5, Curve::lin, 0.8, Fmt::pct); knob("g.bend", "Bend", 0, 24, Curve::integer, 2, Fmt::integer); knob("g.poly", "Voices", 1, 16, Curve::integer, 8, Fmt::integer);
      menu("g.vmode", "Play mode", { "Poly", "Mono", "Legato" }, false).resets(); knob("g.glide", "Glide", 0, 2, Curve::pow3, 0, Fmt::sec); menu("g.gmode", "Glide", { "Always", "Legato only" }, false); }
    refresh();
    if (auto* sc = findParentComponentOfClass<Scroller>()) sc->relayout(); else resized();
    repaint();
}

// ------------------------------------------------------------------ arp
void ArpPage::rebuild() {
    clear();
    const json pj = proc.patchJson();
    juce::StringArray modes{ "Up", "Down", "Up/Down", "Down/Up", "Random", "As played", "Chord" };
    { section("Arpeggiator");
      toggle("arp.on", "Arp on").resets(); menu("arp.mode", "Mode", modes, true, toSA(aether::arpModes())); menu("arp.div", "Rate", divsFrom(7), true);
      knob("arp.oct", "Octaves", 1, 4, Curve::integer, 1, Fmt::integer); knob("arp.gate", "Gate", 0.05, 1, Curve::lin, 0.6, Fmt::pct); knob("arp.swing", "Swing", 0, 1, Curve::lin, 0, Fmt::pct);
      knob("arp.len", "Steps", 1, 16, Curve::integer, 16, Fmt::integer); toggle("arp.latch", "Latch"); }
    { auto& s = section("Step Velocity");
      for (int i = 0; i < 16; i++) {
          auto& k = *controls.emplace_back(std::make_unique<Control>(proc, Control::Kind::knob, "arp.vel." + std::to_string(i), juce::String(i + 1)));
          k.range(0, 1, Curve::lin, 1, Fmt::pct); addAndMakeVisible(k); s.items.push_back(&k);
      } }
    { auto& s = section("Step Transpose");
      for (int i = 0; i < 16; i++) {
          auto& k = *controls.emplace_back(std::make_unique<Control>(proc, Control::Kind::knob, "arp.tr." + std::to_string(i), juce::String(i + 1)));
          k.range(-24, 24, Curve::integer, 0, Fmt::st); addAndMakeVisible(k); s.items.push_back(&k);
      } }
    juce::StringArray chordNames; for (auto& c : aether::chords()) chordNames.add(c.name);
    juce::StringArray scaleNames; for (auto& c : aether::scales()) scaleNames.add(c.name);
    { section("Chord"); toggle("chord.on", "Chord on").resets(); menu("chord.type", "Chord", chordNames, true); menu("chord.spread", "Voicing", { "Close", "Open", "Wide" }, false); }
    { section("Scale"); toggle("scale.on", "Snap to scale").resets(); menu("scale.root", "Root", { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" }, false); menu("scale.type", "Scale", scaleNames, true); }
    refresh();
    if (auto* sc = findParentComponentOfClass<Scroller>()) sc->relayout(); else resized();
    repaint();
    (void)pj;
}

} // namespace edit

namespace edit {

// ------------------------------------------------------------------ song sections
void SectionsPage::rebuild() {
    clear(); batt.clear(); catt.clear();
    for (auto& b : secBtn) b = nullptr;
    const json pj = proc.patchJson();
    {
        auto& s = section("Song Sections");
        auto& on = own(std::make_unique<juce::ToggleButton>("Sections on"));
        on.setColour(juce::ToggleButton::textColourId, ui::text); on.setColour(juce::ToggleButton::tickColourId, ui::accent);
        batt.push_back(std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(proc.apvts, "secon", on));
        on.getProperties().set("w", 130); on.getProperties().set("h", 30); s.items.push_back(&on);
        for (int i = 0; i < 4; i++) {
            auto& b = own(std::make_unique<juce::TextButton>(aether::sectionName(i)));
            b.onClick = [this, i] { auto* prm = proc.apvts.getParameter("section"); prm->beginChangeGesture(); prm->setValueNotifyingHost(prm->convertTo0to1((float)i)); prm->endChangeGesture();
                                    if (auto* o = proc.apvts.getParameter("secon")) if (o->getValue() < 0.5f) o->setValueNotifyingHost(1.0f); };
            b.getProperties().set("w", 84); b.getProperties().set("h", 32); s.items.push_back(&b); secBtn[i] = &b;
        }
        auto& now = own(std::make_unique<juce::Label>()); now.setColour(juce::Label::textColourId, ui::dim); now.setFont(juce::FontOptions(12.0f));
        now.getProperties().set("w", 200); now.getProperties().set("h", 32); s.items.push_back(&now); nowLabel = &now;
    }
    {
        auto& s = section("Follow Your Song");
        auto& fol = own(std::make_unique<juce::ToggleButton>("Follow FL playback"));
        fol.setColour(juce::ToggleButton::textColourId, ui::text); fol.setColour(juce::ToggleButton::tickColourId, ui::accent);
        batt.push_back(std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(proc.apvts, "follow", fol));
        fol.getProperties().set("w", 170); fol.getProperties().set("h", 30); s.items.push_back(&fol);
        auto& morph = own(std::make_unique<juce::ComboBox>());
        morph.addItemList({ "Instant", "1 beat", "2 beats", "1 bar", "2 bars" }, 1);
        catt.push_back(std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(proc.apvts, "morph", morph));
        morph.getProperties().set("w", 110); morph.getProperties().set("h", 30); s.items.push_back(&morph);
        auto& arr = own(std::make_unique<juce::TextEditor>());
        arr.setText(proc.arrangementText(), false); arr.setIndents(8, 0); arr.setJustification(juce::Justification::centredLeft);
        arr.onReturnKey = arr.onFocusLost = [this, &arr] { proc.setArrangement(arr.getText()); };
        arr.getProperties().set("w", 300); arr.getProperties().set("h", 30); s.items.push_back(&arr);
        auto& tmpl = own(std::make_unique<juce::ComboBox>());
        const juce::StringArray names{ "Rap song (3 verses)", "Two verses", "Hook first", "Beat tape loop", "Short single" };
        const juce::StringArray maps{ "I8 V16 H8 V16 H8 V16 H8 O8", "I8 V16 H8 V16 H8 O8", "H8 V16 H8 V16 H8 O8", "I4 V8 H8", "I4 V16 H8 V8 H8 O4" };
        tmpl.addItemList(names, 1); tmpl.setTextWhenNothingSelected("Templates");
        tmpl.onChange = [this, &tmpl, &arr, maps] { const int i = tmpl.getSelectedItemIndex(); if (i >= 0) { arr.setText(maps[i], false); proc.setArrangement(maps[i]); } };
        tmpl.getProperties().set("w", 170); tmpl.getProperties().set("h", 30); s.items.push_back(&tmpl);
        auto& hint = own(std::make_unique<juce::Label>());
        hint.setText("I = intro, V = verse, H = hook, O = outro, then bars. After the map ends, the last verse and hook keep alternating.", juce::dontSendNotification);
        hint.setFont(juce::FontOptions(11.5f)); hint.setColour(juce::Label::textColourId, ui::dim);
        hint.getProperties().set("w", 640); hint.getProperties().set("h", 22); s.items.push_back(&hint);
    }
    static const char* blurb[4] = { "Intro", "Verse (leaves room for vocals)", "Hook", "Outro" };
    for (int i = 0; i < 4; i++) {
        const std::string b = "sec." + std::to_string(i) + ".";
        section(blurb[i]);
        knob(b + "cut", "Filter", -0.5, 0.5, Curve::lin, 0, Fmt::bip); knob(b + "res", "Reso", -0.3, 0.3, Curve::lin, 0, Fmt::bip);
        knob(b + "rev", "Reverb", -0.4, 0.4, Curve::lin, 0, Fmt::bip); knob(b + "dly", "Delay", -0.4, 0.4, Curve::lin, 0, Fmt::bip);
        knob(b + "wid", "Width", 0, 2, Curve::lin, 1, Fmt::pct); knob(b + "pocket", "Vocal Pocket", 0, 1, Curve::lin, 0, Fmt::pct);
        knob(b + "drive", "Drive", 0, 0.6, Curve::lin, 0, Fmt::pct); knob(b + "rel", "Release", 0.25, 4, Curve::expo, 1, Fmt::ratio);
        knob(b + "gain", "Level", -12, 6, Curve::lin, 0, Fmt::db);
        for (int l = 0; l < 4; l++) knob(b + "lvl." + std::to_string(l), juce::String("Layer ") + juce::String::charToString((juce::juce_wchar)('A' + l)), 0, 1.5, Curve::lin, 1, Fmt::pct);
    }
    refresh();
    if (auto* sc = findParentComponentOfClass<Scroller>()) sc->relayout(); else resized();
    repaint();
    (void)pj;
}
void SectionsPage::timerCallback() {
    const float now = proc.sectionNow.load();
    const float sel = proc.apvts.getRawParameterValue("section")->load();
    for (int i = 0; i < 4; i++) if (secBtn[i]) {
        const bool cur = now >= 0 ? std::abs(now - (float)i) < 0.5f : std::abs(sel - (float)i) < 0.5f;
        secBtn[i]->setColour(juce::TextButton::buttonColourId, cur ? (now >= 0 ? ui::accent.withAlpha(0.6f) : ui::accent.withAlpha(0.25f)) : ui::panel);
    }
    if (nowLabel) nowLabel->setText(now < 0 ? juce::String("Off: the preset plays as designed") : "Playing: " + juce::String(aether::sectionName(juce::roundToInt(now))) + (std::abs(now - std::round(now)) > 0.02f ? " (morphing)" : ""), juce::dontSendNotification);
}

// ------------------------------------------------------------------ crate mode
namespace {
class CrateView : public juce::Component {
public:
    explicit CrateView(AetherProcessor& p) : proc(p) {}
    void paint(juce::Graphics& g) override {
        auto r = getLocalBounds().toFloat();
        g.setColour(ui::bg); g.fillRoundedRectangle(r, 6);
        auto buf = proc.crateSnapshot();
        if (!buf || buf->len <= 0) { g.setColour(ui::dim); g.setFont(juce::FontOptions(13.0f)); g.drawText("Turn Crate Mode on to press this preset into a loop.", r, juce::Justification::centred); return; }
        const int W = juce::jmax(1, (int)r.getWidth()); const float mid = r.getCentreY(), hh = r.getHeight() * 0.45f;
        const int hit = proc.crateHit.load();
        for (int c = 0; c < buf->chops; c++) {
            const float x0 = r.getX() + r.getWidth() * buf->starts[(size_t)c] / buf->len, x1 = r.getX() + r.getWidth() * buf->starts[(size_t)c + 1] / buf->len;
            if (c == hit) { g.setColour(ui::accent.withAlpha(0.18f)); g.fillRect(x0, r.getY(), x1 - x0, r.getHeight()); }
            g.setColour(ui::line); g.drawVerticalLine((int)x0, r.getY(), r.getBottom());
            g.setColour(ui::dim); g.setFont(juce::FontOptions(10.0f)); g.drawText(juce::String(c + 1), (int)x0 + 3, (int)r.getY() + 2, 20, 12, juce::Justification::left);
        }
        g.setColour(ui::accent2.withAlpha(0.85f));
        for (int x = 0; x < W; x++) {
            const int a = (int)((long long)buf->len * x / W), b = juce::jmax(a + 1, (int)((long long)buf->len * (x + 1) / W));
            float pk = 0; for (int i = a; i < b && i < buf->len; i += 4) pk = juce::jmax(pk, std::abs(buf->L[(size_t)i]), std::abs(buf->R[(size_t)i]));
            g.drawVerticalLine((int)r.getX() + x, mid - pk * hh * 1.6f, mid + pk * hh * 1.6f);
        }
    }
    AetherProcessor& proc;
};
}

void CratePage::rebuild() {
    clear(); status = nullptr; view = nullptr;
    {
        auto& s = section("Crate Mode");
        toggle("crate.on", "Crate Mode on");
        juce::StringArray progs; for (auto& p : aether::crateProgs()) progs.add(p.name);
        menu("crate.prog", "Progression", progs, false);
        menu("crate.key", "Key", { "C", "C#", "D", "Eb", "E", "F", "F#", "G", "Ab", "A", "Bb", "B" }, false);
        menu("crate.rhythm", "Rhythm", { "Held", "Stabs", "Pushes", "Half-time" }, false);
        menu("crate.chops", "Chops", { "16 chops", "8 chops" }, false, { "16", "8" });
        knob("crate.age", "Record Age", 0, 1, Curve::lin, 0.5, Fmt::pct);
        auto& re = own(std::make_unique<juce::TextButton>("Re-press"));
        re.onClick = [this] { proc.repressCrate(); };
        re.getProperties().set("w", 100); re.getProperties().set("h", 30); s.items.push_back(&re);
    }
    {
        auto& s = section("The Record");
        auto& v = own(std::make_unique<CrateView>(proc)); v.getProperties().set("w", 680); v.getProperties().set("h", 120); s.items.push_back(&v); view = &v;
        auto& st = own(std::make_unique<juce::Label>()); st.setColour(juce::Label::textColourId, ui::text); st.setFont(juce::FontOptions(13.0f));
        st.getProperties().set("w", 680); st.getProperties().set("h", 22); s.items.push_back(&st); status = &st;
    }
    {
        section("Playing the Chops");
        menu("crate.base", "First chop on", { "C2", "C3", "C4", "C5" }, false, { "24", "36", "48", "60" });
        knob("crate.pitch", "Pitch", -12, 12, Curve::integer, 0, Fmt::st);
        toggle("crate.choke", "Choke (one at a time)"); toggle("crate.oneshot", "Play whole chop");
        auto& hint = own(std::make_unique<juce::Label>());
        hint.setText("Chops sit on consecutive keys from the first-chop key. Every other key still plays the synth, so you can play over your flip.", juce::dontSendNotification);
        hint.setFont(juce::FontOptions(11.5f)); hint.setColour(juce::Label::textColourId, ui::dim);
        hint.getProperties().set("w", 640); hint.getProperties().set("h", 22); sections.back().items.push_back(&hint);
    }
    // show sensible values for presets that have never used Crate Mode
    const json pj = proc.patchJson();
    refresh();
    if (!pj.contains("crate")) for (auto& c : controls) c->load(json{ { "crate", { { "on", 0 }, { "prog", 0 }, { "key", 0 }, { "rhythm", 0 }, { "chops", 16 }, { "age", 0.5 }, { "base", 48 }, { "pitch", 0 }, { "choke", 1 }, { "oneshot", 1 } } } });
    if (auto* sc = findParentComponentOfClass<Scroller>()) sc->relayout(); else resized();
    repaint();
}
void CratePage::timerCallback() {
    if (status) status->setText(proc.crateStatus(), juce::dontSendNotification);
    if (view) view->repaint();
}

} // namespace edit
