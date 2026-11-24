#include "Suite.h"

namespace lgn {
using namespace station;

// ================================================================ processor
Processor::Processor(const juce::String& stateName, juce::AudioProcessorValueTreeState::ParameterLayout layout)
    : Processor(BusesProperties().withInput("Input", juce::AudioChannelSet::stereo(), true).withOutput("Output", juce::AudioChannelSet::stereo(), true), stateName, std::move(layout)) {}
Processor::Processor(const BusesProperties& buses, const juce::String& stateName, juce::AudioProcessorValueTreeState::ParameterLayout layout)
    : AudioProcessor(buses), apvts(*this, nullptr, stateName, std::move(layout)) {
    for (auto* prm : getParameters())
        if (auto* r = dynamic_cast<juce::RangedAudioParameter*>(prm)) params[r->getParameterID()] = apvts.getRawParameterValue(r->getParameterID());
}

bool Processor::isBusesLayoutSupported(const BusesLayout& b) const {
    const auto in = b.getMainInputChannelSet(), out = b.getMainOutputChannelSet();
    return in == out && (out == juce::AudioChannelSet::mono() || out == juce::AudioChannelSet::stereo());
}

void Processor::getStateInformation(juce::MemoryBlock& dest) {
    auto st = apvts.copyState();
    st.setProperty("preset", current.load(), nullptr);
    st.setProperty("presetModified", presetModified(), nullptr);
    if (auto xml = st.createXml()) copyXmlToBinary(*xml, dest);
}
void Processor::setStateInformation(const void* data, int size) {
    if (auto xml = getXmlFromBinary(data, size))
        if (xml->hasTagName(apvts.state.getType())) {
            auto st = juce::ValueTree::fromXml(*xml);
            const int pr = juce::jlimit(0, (int)presetList.size() - 1, (int)st.getProperty("preset", 0));
            const bool mod = (bool)st.getProperty("presetModified", false);
            apvts.replaceState(st);
            current = pr;
            loaded.clear();
            for (auto* prm : getParameters()) loaded.push_back(mod ? -1.0f : prm->getValue());
        }
}

void Processor::setPresets(std::vector<Preset> list, const juce::StringArray& keep) {
    for (auto& pr : list) {
        for (auto& [id, v] : pr.values) jassert(params.count(id));   // a typo in a preset would silently do nothing
        presetList.push_back(std::move(pr));
    }
    keepIds = keep;
}
void Processor::loadPreset(int index) {
    if (!juce::isPositiveAndBelow(index, (int)presetList.size())) return;
    const auto& pr = presetList[(size_t)index];
    for (auto* prm : getParameters())
        if (auto* r = dynamic_cast<juce::RangedAudioParameter*>(prm)) {
            if (keepIds.contains(r->getParameterID())) continue;
            float v = r->getDefaultValue();
            for (auto& [id, val] : pr.values) if (id == r->getParameterID()) v = r->convertTo0to1(val);
            r->beginChangeGesture(); r->setValueNotifyingHost(v); r->endChangeGesture();
        }
    current = index;
    loaded.clear();
    for (auto* prm : getParameters()) loaded.push_back(prm->getValue());
}
bool Processor::presetModified() const {
    if (loaded.empty()) return false;
    auto& ps = getParameters();
    for (int i = 0; i < ps.size() && i < (int)loaded.size(); i++) {
        if (auto* r = dynamic_cast<juce::RangedAudioParameter*>(ps[i]); r && keepIds.contains(r->getParameterID())) continue;
        if (std::abs(ps[i]->getValue() - loaded[(size_t)i]) > 1e-4f) return true;
    }
    return false;
}

// ================================================================ parameters
namespace param {
juce::NormalisableRange<float> logRange(float lo, float hi) {
    return { lo, hi, [](float a, float b, float t) { return a * std::pow(b / a, t); }, [](float a, float b, float v) { return std::log(v / a) / std::log(b / a); },
             [](float a, float b, float v) { return juce::jlimit(a, b, v); } };
}
juce::NormalisableRange<float> skewRange(float lo, float hi, float centre) { juce::NormalisableRange<float> r(lo, hi); r.setSkewForCentre(centre); return r; }
juce::String formatHz(float v) { return v >= 1000 ? juce::String(v / 1000.0f, v >= 10000 ? 1 : 2) + " kHz" : juce::String(juce::roundToInt(v)) + " Hz"; }

void add(Layout& l, const char* id, const char* name, juce::NormalisableRange<float> r, float def, const char* unit, int decimals) {
    const juce::String u(unit);
    auto attrs = juce::AudioParameterFloatAttributes().withLabel(u).withStringFromValueFunction([u, decimals](float v, int) {
        if (u == "Hz") return formatHz(v);
        if (u == "ms") return v >= 1000 ? juce::String(v / 1000.0f, 2) + " s" : juce::String(v, v < 10 ? 1 : 0) + " ms";
        if (u == "s") return juce::String(v, v < 10 ? 2 : 1) + " s";
        if (u == "dB") return (v > 0.04f ? "+" : "") + juce::String(std::abs(v) < 0.05f ? 0.0f : v, decimals) + " dB";
        if (u == "%") return juce::String(juce::roundToInt(v)) + "%";
        if (u == ":1") return juce::String(v, v < 10 ? 1 : 0) + ":1";
        return juce::String(v, decimals) + (u.isEmpty() ? "" : " " + u);
    });
    l.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{ id, 1 }, name, r, def, attrs));
}
void addHz(Layout& l, const char* id, const char* name, float lo, float hi, float def) { add(l, id, name, logRange(lo, hi), def, "Hz"); }
void addMs(Layout& l, const char* id, const char* name, float lo, float hi, float def, float centre) { add(l, id, name, skewRange(lo, hi, centre), def, "ms"); }
void addDb(Layout& l, const char* id, const char* name, float lo, float hi, float def) { add(l, id, name, { lo, hi, 0.01f }, def, "dB"); }
void addPct(Layout& l, const char* id, const char* name, float def) { add(l, id, name, { 0, 100, 0.1f }, def, "%"); }
void addBool(Layout& l, const char* id, const char* name, bool def) { l.add(std::make_unique<juce::AudioParameterBool>(juce::ParameterID{ id, 1 }, name, def)); }
void addChoice(Layout& l, const char* id, const char* name, const juce::StringArray& c, int def) { l.add(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID{ id, 1 }, name, c, def)); }
}

// ================================================================ widgets
Dial::Dial(juce::AudioProcessorValueTreeState& s, const char* id, const juce::String& l, bool sm) : label(l), small(sm) {
    slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 80, 16);
    slider.setColour(juce::Slider::textBoxTextColourId, ink);
    slider.setRotaryParameters(juce::MathConstants<float>::pi * 1.25f, juce::MathConstants<float>::pi * 2.75f, true);
    slider.setMouseDragSensitivity(220);
    slider.setVelocityBasedMode(false);
    auto* prm = s.getParameter(id);
    slider.setDoubleClickReturnValue(true, (double)prm->convertFrom0to1(prm->getDefaultValue()));
    addAndMakeVisible(slider);
    att = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(s, id, slider);
    slider.textFromValueFunction = [prm](double v) { return prm->getText(prm->convertTo0to1((float)v), 0); };
    slider.updateText();
    slider.setTooltip(prm->getName(64));
}
void Dial::resized() {
    const int labelH = small ? 14 : 16, textH = small ? 15 : 17, gap = 4;
    auto r = getLocalBounds();
    const int cap = small ? juce::jmin(maxKnob, 54) : maxKnob;
    const int knob = juce::jmax(24, juce::jmin(cap, juce::jmin(r.getWidth() - 8, r.getHeight() - labelH - textH - gap * 2)));
    const int total = labelH + gap + knob + textH;
    auto col = r.withSizeKeepingCentre(r.getWidth(), total);
    labelArea = col.removeFromTop(labelH); col.removeFromTop(gap);
    slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, r.getWidth(), textH);
    slider.setBounds(col.withSizeKeepingCentre(juce::jmax(knob, r.getWidth()), col.getHeight()));
}
void Dial::paint(juce::Graphics& g) {
    drawCaps(g, label, labelArea.toFloat(), juce::Justification::centred, small ? 9.0f : 9.5f, dim);
}

Switch::Switch(juce::AudioProcessorValueTreeState& s, const char* id, const juce::String& l) {
    button.setButtonText(l); addAndMakeVisible(button);
    att = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(s, id, button);
}

Segments::Segments(juce::AudioProcessorValueTreeState& s, const char* pid, juce::StringArray labels) : state(s), id(pid) {
    auto* c = dynamic_cast<juce::AudioParameterChoice*>(s.getParameter(pid));
    if (labels.isEmpty()) labels = c->choices;
    for (int i = 0; i < labels.size(); i++) {
        auto* b = buttons.add(new juce::TextButton(labels[i]));
        b->getProperties().set("led", true);
        b->onClick = [this, i, c] { c->beginChangeGesture(); *c = i; c->endChangeGesture(); refresh(); };
        addAndMakeVisible(b);
    }
    s.addParameterListener(id, this);
    refresh();
}
Segments::~Segments() { state.removeParameterListener(id, this); }
void Segments::refresh() {
    const int v = juce::roundToInt(state.getRawParameterValue(id)->load());
    for (int i = 0; i < buttons.size(); i++) buttons[i]->setToggleState(i == v, juce::dontSendNotification);
}
void Segments::resized() {
    auto r = getLocalBounds().reduced(3);
    const int w = r.getWidth() / juce::jmax(1, buttons.size());
    for (auto* b : buttons) b->setBounds(r.removeFromLeft(w));
}
void Segments::paint(juce::Graphics& g) { drawWell(g, getLocalBounds().toFloat(), juce::jmin(9.0f, getHeight() / 2.0f)); }

Choice::Choice(juce::AudioProcessorValueTreeState& s, const char* id, const juce::String& l) : label(l) {
    auto* c = dynamic_cast<juce::AudioParameterChoice*>(s.getParameter(id));
    box.addItemList(c->choices, 1);
    addAndMakeVisible(box);
    att = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(s, id, box);
}
void Choice::resized() { auto r = getLocalBounds(); if (label.isNotEmpty()) r.removeFromTop(16); box.setBounds(r); }
void Choice::paint(juce::Graphics& g) { if (label.isNotEmpty()) drawCaps(g, label, getLocalBounds().removeFromTop(14).toFloat(), juce::Justification::centredLeft, 9.5f, dim); }

void Meter::setValue(float v) {
    if (mode == level) {
        const float db = (float)gainToDb(v);
        const float t = juce::jlimit(0.0f, 1.0f, (db + 60.0f) / 60.0f);
        shown = t > shown ? t : shown + (t - shown) * 0.18f;
        if (t >= hold) { hold = t; holdAge = 0; } else if (++holdAge > 45) hold = juce::jmax(shown, hold - 0.01f);
    } else {
        const float t = juce::jlimit(0.0f, 1.0f, v / 24.0f);
        shown = t > shown ? t : shown + (t - shown) * 0.25f;
    }
    repaint();
}
void Meter::paint(juce::Graphics& g) {
    auto r = getLocalBounds().toFloat();
    g.setColour(well); g.fillRoundedRectangle(r, 3);
    if (mode == level) {
        const float h = r.getHeight() * shown;
        g.setColour(graphite); g.fillRoundedRectangle(r.withTop(r.getBottom() - h), 3);
        const float yh = r.getBottom() - r.getHeight() * hold;
        if (hold > 0.01f) { g.setColour(hold > 0.999f ? juce::Colour(0xffe5352e) : dim); g.fillRect(r.getX(), yh - 1, r.getWidth(), 2.0f); }
    } else {
        g.setColour(accent); g.fillRoundedRectangle(r.withHeight(r.getHeight() * shown), 3);
    }
}

void History::paint(juce::Graphics& g) {
    const auto r = getLocalBounds().toFloat();
    drawWell(g, r, 10);
    const auto in = r.reduced(10);
    const int step = maxGr > 12 ? 6 : 3;
    for (int db = step; db < (int)maxGr; db += step) {
        const float y = in.getY() + in.getHeight() * db / maxGr;
        g.setColour(hairline.withAlpha(0.7f)); g.drawHorizontalLine((int)y, in.getX(), in.getRight());
        g.setFont(mono(9.0f)); g.setColour(faint); g.drawText("-" + juce::String(db), juce::Rectangle<float>(in.getRight() - 28, y + 1, 26, 11), juce::Justification::right);
    }
    juce::Path lvl, gr; const float dx = in.getWidth() / (kLen - 1);
    lvl.startNewSubPath(in.getX(), in.getBottom()); gr.startNewSubPath(in.getX(), in.getY());
    for (int i = 0; i < kLen; i++) {
        const int k = (pos + i) % kLen; const float x = in.getX() + i * dx;
        lvl.lineTo(x, in.getBottom() - in.getHeight() * juce::jlimit(0.0f, 1.0f, (inH[(size_t)k] + 60) / 60));
        gr.lineTo(x, in.getY() + in.getHeight() * juce::jlimit(0.0f, 1.0f, grH[(size_t)k] / maxGr));
    }
    lvl.lineTo(in.getRight(), in.getBottom()); lvl.closeSubPath();
    juce::Path grFill(gr); grFill.lineTo(in.getRight(), in.getY()); grFill.closeSubPath();
    g.setColour(juce::Colour(0xffe2e6eb)); g.fillPath(lvl);
    g.setColour(accent.withAlpha(0.16f)); g.fillPath(grFill);
    g.setColour(accent); g.strokePath(gr, juce::PathStrokeType(1.8f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
}

void IoMeters::tick() {
    auto step = [](float& s, float v) { const float t = juce::jlimit(0.0f, 1.0f, ((float)gainToDb(v) + 60.0f) / 60.0f); s = t > s ? t : s + (t - s) * 0.2f; };
    step(in, proc.inMeter.take()); step(out, proc.outMeter.take()); repaint();
}
void IoMeters::paint(juce::Graphics& g) {
    auto r = getLocalBounds().toFloat();
    const float rowH = r.getHeight() / 2;
    const char* names[2] = { "IN", "OUT" }; const float vals[2] = { in, out };
    for (int i = 0; i < 2; i++) {
        auto row = r.removeFromTop(rowH).reduced(0, 3);
        g.setFont(mono(9.5f)); g.setColour(faint); g.drawText(names[i], row.removeFromLeft(28), juce::Justification::centredLeft);
        auto bar = row.withSizeKeepingCentre(row.getWidth(), 4);
        g.setColour(track); g.fillRoundedRectangle(bar, 2);
        g.setColour(vals[i] > 0.995f ? juce::Colour(0xffe5352e) : graphite); g.fillRoundedRectangle(bar.withWidth(bar.getWidth() * vals[i]), 2);
    }
}

// ================================================================ editor
Editor::Editor(Processor& p, const juce::String& n, const juce::String& sub, juce::Colour a, int w, int h)
    : AudioProcessorEditor(&p), accent(a), proc(p), look(a), designW(w), designH(h), name(n), subtitle(sub), io(p), presetBar(p) {
    setLookAndFeel(&look);
    body.ed = this;
    body.setSize(w, h);
    addAndMakeVisible(body);
    body.addAndMakeVisible(io);
    body.addAndMakeVisible(presetBar);
    tips.setLookAndFeel(&look);
    setResizable(true, true);
    getConstrainer()->setFixedAspectRatio((double)w / h);
    setResizeLimits(w * 3 / 4, h * 3 / 4, w * 2, h * 2);
    setSize(w, h);
    startTimerHz(30);
}
Editor::~Editor() { stopTimer(); tips.setLookAndFeel(nullptr); setLookAndFeel(nullptr); }
void Editor::resized() {
    const float s = (float)getWidth() / (float)designW;
    body.setTransform(juce::AffineTransform::scale(s));
    if (!ready) return;
    auto r = juce::Rectangle<int>(designW, designH).reduced(16);
    auto head = r.removeFromTop(52);
    io.setBounds(head.removeFromRight(190).reduced(16, 9));
    presetBar.setBounds(head.removeFromRight(290).reduced(0, 10));
    r.removeFromTop(12);
    layout(r);
}
void Editor::Body::paint(juce::Graphics& g) {
    g.fillAll(hull);
    auto r = getLocalBounds().reduced(16);
    auto head = r.removeFromTop(52).toFloat();
    drawModule(g, head, {}, ed->accent);
    drawWordmark(g, head.reduced(18, 0).withTrimmedRight(200), ed->name, ed->subtitle, ed->accent);
    ed->paintBody(g);
}
void Editor::module(juce::Graphics& g, juce::Rectangle<int> r, const juce::String& title, const juce::String& code) { drawModule(g, r.toFloat(), title, accent, code); }
void Editor::timerCallback() { io.tick(); presetBar.tick(); tick(); }

// ---------------------------------------------------------------- preset bar
Editor::PresetBar::PresetBar(Processor& p) : proc(p) {
    for (auto* b : { &prev, &next }) { addAndMakeVisible(b); b->setTooltip(b == &prev ? "Previous preset" : "Next preset"); }
    prev.onClick = [this] { step(-1); }; next.onClick = [this] { step(1); };
    setMouseCursor(juce::MouseCursor::PointingHandCursor);
}
void Editor::PresetBar::resized() {
    auto r = getLocalBounds();
    next.setBounds(r.removeFromRight(r.getHeight())); r.removeFromRight(4);
    prev.setBounds(r.removeFromRight(r.getHeight())); r.removeFromRight(6);
    nameArea = r;
}
void Editor::PresetBar::paint(juce::Graphics& g) {
    const auto r = nameArea.toFloat();
    drawWell(g, r, 6);
    const auto& ps = proc.presets(); const int i = juce::jlimit(0, (int)ps.size() - 1, proc.currentPreset());
    auto t = r.reduced(12, 0);
    if (ps[(size_t)i].category.isNotEmpty()) {
        g.setFont(caps(9.0f)); g.setColour(dim);
        const auto cat = ps[(size_t)i].category.toUpperCase();
        const float w = juce::jmin(t.getWidth() * 0.45f, (float)juce::GlyphArrangement::getStringWidthInt(g.getCurrentFont(), cat) + 10.0f);
        g.drawText(cat, t.removeFromLeft(w), juce::Justification::centredLeft, true);
    }
    // chevron
    const auto cv = t.removeFromRight(12);
    juce::Path ch; ch.startNewSubPath(cv.getCentreX() - 3.5f, cv.getCentreY() - 1.5f); ch.lineTo(cv.getCentreX(), cv.getCentreY() + 2); ch.lineTo(cv.getCentreX() + 3.5f, cv.getCentreY() - 1.5f);
    g.setColour(dim); g.strokePath(ch, juce::PathStrokeType(1.3f));
    g.setFont(sans(13.0f, medium)); g.setColour(ink);
    g.drawText(ps[(size_t)i].name + (modified ? " *" : ""), t.withTrimmedRight(6), juce::Justification::centredLeft, true);
}
void Editor::PresetBar::tick() {
    const int c = proc.currentPreset(); const bool m = proc.presetModified();
    if (c != shown || m != modified) { shown = c; modified = m; repaint(); }
}
void Editor::PresetBar::step(int d) {
    const int n = (int)proc.presets().size();
    proc.loadPreset(((proc.currentPreset() + d) % n + n) % n);
    tick();
}
void Editor::PresetBar::mouseUp(const juce::MouseEvent& e) { if (nameArea.contains(e.getPosition())) openMenu(); }
void Editor::PresetBar::openMenu() {
    juce::PopupMenu menu; juce::StringArray cats; std::map<juce::String, juce::PopupMenu> subs;
    const auto& ps = proc.presets();
    for (int i = 0; i < (int)ps.size(); i++) {
        if (ps[(size_t)i].category.isEmpty()) { menu.addItem(i + 1, ps[(size_t)i].name, true, i == proc.currentPreset()); menu.addSeparator(); continue; }
        if (!cats.contains(ps[(size_t)i].category)) cats.add(ps[(size_t)i].category);
        subs[ps[(size_t)i].category].addItem(i + 1, ps[(size_t)i].name, true, i == proc.currentPreset());
    }
    for (auto& c : cats) {
        bool has = false; for (int i = 0; i < (int)ps.size(); i++) has |= ps[(size_t)i].category == c && i == proc.currentPreset();
        menu.addSubMenu(c, subs[c], true, nullptr, has);
    }
    juce::Component::SafePointer<PresetBar> safe(this);
    menu.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(this).withMinimumWidth(getWidth()), [safe](int r) {
        if (safe && r > 0) { safe->proc.loadPreset(r - 1); safe->tick(); }
    });
}

// ================================================================ displays
void FreqAxis::drawGrid(juce::Graphics& g, juce::Rectangle<float> r, bool labels) const {
    const float lines[] = { 30, 50, 100, 200, 300, 500, 1000, 2000, 3000, 5000, 10000, 20000 };
    for (float f : lines) {
        if (f < lo || f > hi) continue;
        const float x = this->x(f, r);
        const bool major = f == 100 || f == 1000 || f == 10000;
        g.setColour(major ? hairline : hairline.withAlpha(0.55f)); g.drawVerticalLine((int)x, r.getY(), r.getBottom());
        if (labels && (major || f == 50 || f == 500 || f == 5000)) {
            g.setFont(mono(9.0f)); g.setColour(faint);
            g.drawText(f >= 1000 ? juce::String((int)(f / 1000)) + "k" : juce::String((int)f), juce::Rectangle<float>(x + 3, r.getBottom() - 14, 30, 12), juce::Justification::left);
        }
    }
}

Spectrum::Spectrum(int order) : fft(order), n(1 << order), win((size_t)n), work((size_t)n * 2), mags((size_t)n / 2 + 1, -120.0f) {
    for (int i = 0; i < n; i++) win[(size_t)i] = 0.5f - 0.5f * std::cos(juce::MathConstants<float>::twoPi * i / (n - 1));
}
void Spectrum::update(const ScopeFeed& feed, double rate) {
    sr = rate;
    feed.latest(work.data(), n);
    for (int i = 0; i < n; i++) work[(size_t)i] *= win[(size_t)i];
    std::fill(work.begin() + n, work.end(), 0.0f);
    fft.performFrequencyOnlyForwardTransform(work.data(), true);
    const float norm = 4.0f / n;   // Hann coherent gain 0.5, one-sided spectrum
    for (int k = 0; k <= n / 2; k++) {
        const float db = (float)gainToDb(work[(size_t)k] * norm);
        float& m = mags[(size_t)k];
        m = db > m ? m + (db - m) * 0.6f : m + (db - m) * 0.12f;   // fast rise, slow fall
    }
}
float Spectrum::dbAt(float f) const {
    const float k = f * n / (float)sr; const int i = juce::jlimit(0, n / 2 - 1, (int)k); const float t = juce::jlimit(0.0f, 1.0f, k - i);
    return mags[(size_t)i] + (mags[(size_t)i + 1] - mags[(size_t)i]) * t;
}
juce::Path Spectrum::path(juce::Rectangle<float> r, const FreqAxis& ax, float dbLo, float dbHi, bool closed) const {
    juce::Path p; bool started = false;
    for (float x = r.getX(); x <= r.getRight(); x += 2) {
        // average the bins under this pixel column so high frequencies don't alias into a hedge
        const float f0 = ax.f(x - 1, r), f1 = ax.f(x + 1, r);
        const int k0 = juce::jlimit(1, n / 2, (int)(f0 * n / sr)), k1 = juce::jlimit(k0, n / 2, (int)(f1 * n / sr));
        float db;
        if (k1 > k0) { float s = 0; for (int k = k0; k <= k1; k++) s += std::pow(10.0f, mags[(size_t)k] / 10.0f); db = 10.0f * std::log10(s / (k1 - k0 + 1) + 1e-12f); }
        else db = dbAt(ax.f(x, r));
        // tilt +4.5 dB/oct so music reads flat-ish, like most analysers
        db += 4.5f * std::log2(juce::jmax(20.0f, ax.f(x, r)) / 1000.0f);
        const float y = juce::jmap(juce::jlimit(dbLo, dbHi, db), dbLo, dbHi, r.getBottom(), r.getY());
        if (!started) { if (closed) p.startNewSubPath(x, r.getBottom()), p.lineTo(x, y); else p.startNewSubPath(x, y); started = true; } else p.lineTo(x, y);
    }
    if (closed && started) { p.lineTo(r.getRight(), r.getBottom()); p.closeSubPath(); }
    return p;
}
} // namespace lgn
