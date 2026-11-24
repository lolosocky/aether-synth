#include "Suite.h"

namespace lgn {
using namespace station;

// ================================================================ processor
Processor::Processor(const juce::String& stateName, juce::AudioProcessorValueTreeState::ParameterLayout layout)
    : AudioProcessor(BusesProperties().withInput("Input", juce::AudioChannelSet::stereo(), true).withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, nullptr, stateName, std::move(layout)) {
    for (auto* prm : getParameters())
        if (auto* r = dynamic_cast<juce::RangedAudioParameter*>(prm)) params[r->getParameterID()] = apvts.getRawParameterValue(r->getParameterID());
}

bool Processor::isBusesLayoutSupported(const BusesLayout& b) const {
    const auto in = b.getMainInputChannelSet(), out = b.getMainOutputChannelSet();
    return in == out && (out == juce::AudioChannelSet::mono() || out == juce::AudioChannelSet::stereo());
}

void Processor::getStateInformation(juce::MemoryBlock& dest) {
    if (auto xml = apvts.copyState().createXml()) copyXmlToBinary(*xml, dest);
}
void Processor::setStateInformation(const void* data, int size) {
    if (auto xml = getXmlFromBinary(data, size))
        if (xml->hasTagName(apvts.state.getType())) apvts.replaceState(juce::ValueTree::fromXml(*xml));
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
    auto r = getLocalBounds();
    r.removeFromTop(small ? 14 : 16);
    slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, r.getWidth(), small ? 15 : 17);
    slider.setBounds(r);
}
void Dial::paint(juce::Graphics& g) {
    drawCaps(g, label, getLocalBounds().removeFromTop(small ? 14 : 16).toFloat(), juce::Justification::centred, small ? 9.0f : 9.5f, dim);
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
    : AudioProcessorEditor(&p), accent(a), proc(p), look(a), designW(w), designH(h), name(n), subtitle(sub), io(p) {
    setLookAndFeel(&look);
    body.ed = this;
    body.setSize(w, h);
    addAndMakeVisible(body);
    body.addAndMakeVisible(io);
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
void Editor::timerCallback() { io.tick(); tick(); }

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
