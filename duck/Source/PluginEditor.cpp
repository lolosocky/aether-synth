#include "PluginEditor.h"

namespace {
const juce::Colour BG(0xff16181d), PANEL(0xff20242b), LINE(0xff2e333c), TEXT(0xffd9dee5), DIM(0xff7d8794), ACCENT(0xff4fb3ff), HOT(0xffff6b6b);
const char* kRates[5] = { "1/1", "1/2", "1/4", "1/8", "1/16" };
}

void DuckLook::drawRotarySlider(juce::Graphics& g, int x, int y, int w, int h, float pos, float start, float end, juce::Slider& slider) {
    auto r = juce::Rectangle<float>((float)x, (float)y, (float)w, (float)h).reduced(6);
    const float size = juce::jmin(r.getWidth(), r.getHeight()); r = r.withSizeKeepingCentre(size, size);
    const float angle = start + pos * (end - start), thick = size * 0.09f, rad = size * 0.5f - thick;
    juce::Path track, value;
    track.addCentredArc(r.getCentreX(), r.getCentreY(), rad, rad, 0, start, end, true);
    value.addCentredArc(r.getCentreX(), r.getCentreY(), rad, rad, 0, start, angle, true);
    g.setColour(LINE); g.strokePath(track, juce::PathStrokeType(thick, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    g.setColour(ACCENT); g.strokePath(value, juce::PathStrokeType(thick, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    auto knob = r.reduced(thick * 2.2f);
    g.setColour(PANEL.brighter(0.15f)); g.fillEllipse(knob);
    g.setColour(TEXT);
    const auto tip = knob.getCentre().getPointOnCircumference(knob.getWidth() * 0.36f, angle);
    if (slider.getName() != "amount")
        g.drawLine({ knob.getCentre().getPointOnCircumference(knob.getWidth() * 0.12f, angle), tip }, 3.0f);
    else {   // the big Amount knob shows its value in the middle
        g.setColour(TEXT); g.setFont(juce::Font(juce::FontOptions(size * 0.2f, juce::Font::bold)));
        g.drawText(juce::String((int)std::round(slider.getValue())) + "%", knob.toNearestInt(), juce::Justification::centred);
    }
}

void DuckLook::drawButtonBackground(juce::Graphics& g, juce::Button& b, const juce::Colour&, bool over, bool) {
    auto r = b.getLocalBounds().toFloat().reduced(2);
    g.setColour(b.getToggleState() ? ACCENT : (over ? LINE.brighter(0.2f) : LINE));
    g.fillRoundedRectangle(r, 6);
}

DuckEditor::DuckEditor(DuckProcessor& p) : AudioProcessorEditor(&p), proc(p) {
    setLookAndFeel(&look);
    amount.setName("amount");
    amount.onValueChange = [this] { repaint(); };
    for (auto* s : { &amount, &release }) {
        s->setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        s->setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
        s->setDoubleClickReturnValue(true, s == &amount ? 75.0 : 50.0);
        addAndMakeVisible(*s);
    }
    amountAtt = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(proc.params, "amount", amount);
    releaseAtt = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(proc.params, "release", release);
    for (int i = 0; i < 5; ++i) {
        rateBtn[i].setButtonText(kRates[i]); rateBtn[i].setClickingTogglesState(false);
        rateBtn[i].onClick = [this, i] { setChoice("rate", i); };
        addAndMakeVisible(rateBtn[i]);
    }
    const char* modes[2] = { "TEMPO", "KICK" };
    for (int i = 0; i < 2; ++i) {
        modeBtn[i].setButtonText(modes[i]);
        modeBtn[i].onClick = [this, i] { setChoice("mode", i); };
        addAndMakeVisible(modeBtn[i]);
    }
    for (auto* b : { &rateBtn[0], &rateBtn[1], &rateBtn[2], &rateBtn[3], &rateBtn[4], &modeBtn[0], &modeBtn[1] }) {
        b->setColour(juce::TextButton::textColourOffId, TEXT); b->setColour(juce::TextButton::textColourOnId, BG);
    }
    setSize(480, 320);
    timerCallback();
    startTimerHz(30);
}
DuckEditor::~DuckEditor() { setLookAndFeel(nullptr); }

void DuckEditor::setChoice(const juce::String& id, int index) {
    if (auto* prm = dynamic_cast<juce::AudioParameterChoice*>(proc.params.getParameter(id))) {
        prm->beginChangeGesture(); *prm = index; prm->endChangeGesture();
    }
}

void DuckEditor::timerCallback() {
    const int rate = (int)proc.params.getRawParameterValue("rate")->load(), mode = (int)proc.params.getRawParameterValue("mode")->load();
    for (int i = 0; i < 5; ++i) { rateBtn[i].setToggleState(i == rate, juce::dontSendNotification); rateBtn[i].setEnabled(mode == 0); rateBtn[i].setAlpha(mode == 0 ? 1.0f : 0.35f); }
    for (int i = 0; i < 2; ++i) modeBtn[i].setToggleState(i == mode, juce::dontSendNotification);
    shownReduction = proc.uiReduction.load();
    repaint();
}

void DuckEditor::drawCurve(juce::Graphics& g, juce::Rectangle<float> r) {
    g.setColour(PANEL); g.fillRoundedRectangle(r, 10);
    auto plot = r.reduced(14, 16);
    const int mode = (int)proc.params.getRawParameterValue("mode")->load();
    const double amt = proc.params.getRawParameterValue("amount")->load() / 100.0, rel = proc.params.getRawParameterValue("release")->load() / 100.0;
    // one pump: tempo mode spans one beat division, kick mode spans 700 ms
    const double span = mode == 0 ? 1.0 : 0.7, len = mode == 0 ? lgn::Duck::tempoLength(rel) : lgn::Duck::kickSeconds(rel);
    juce::Path p, fill;
    for (int i = 0; i <= 200; ++i) {
        const double t = span * i / 200.0, gain = 1 - amt * lgn::Duck::curve(t / len);
        const float x = plot.getX() + plot.getWidth() * (float)i / 200.0f, y = plot.getBottom() - plot.getHeight() * (float)gain;
        if (i == 0) { p.startNewSubPath(x, y); fill.startNewSubPath(x, plot.getBottom()); }
        else p.lineTo(x, y);
        fill.lineTo(x, y);
    }
    fill.lineTo(plot.getRight(), plot.getBottom()); fill.closeSubPath();
    g.setColour(ACCENT.withAlpha(0.15f)); g.fillPath(fill);
    g.setColour(ACCENT); g.strokePath(p, juce::PathStrokeType(2.5f));
    g.setColour(DIM); g.setFont(11.0f);
    g.drawText(mode == 0 ? "one " + juce::String(kRates[(int)proc.params.getRawParameterValue("rate")->load()]) + " note" : "700 ms after each kick",
               plot.withTrimmedTop(plot.getHeight() - 12).translated(0, 14), juce::Justification::centredRight);
    g.drawText("VOLUME", r.reduced(10, 4).removeFromTop(14), juce::Justification::topLeft);
    // live dot: where the volume is right now
    if (mode == 0 && proc.uiPlaying.load()) {
        const float ph = proc.uiPhase.load(), x = plot.getX() + plot.getWidth() * ph;
        const float y = plot.getBottom() - plot.getHeight() * (1 - shownReduction);
        g.setColour(juce::Colours::white); g.fillEllipse(x - 5, y - 5, 10, 10);
    }
    // status line
    juce::String status; juce::Colour col = DIM;
    if (mode == 0) status = proc.uiPlaying.load() ? "Pumping in time with your song" : "Press play: it pumps in time with your song";
    else if (!proc.uiHasSidechain.load()) { status = "Route your kick into this plugin's sidechain input"; col = HOT; }
    else if (proc.uiHitAge.load() > 3.0f) { status = "Waiting for a kick on the sidechain..."; col = HOT; }
    else status = "Ducking to your kick";
    g.setColour(col); g.setFont(12.0f);
    g.drawText(status, r.withTrimmedTop(r.getHeight() + 6).withHeight(18), juce::Justification::centredLeft);
}

void DuckEditor::paint(juce::Graphics& g) {
    g.fillAll(BG);
    auto r = getLocalBounds().reduced(16);
    auto head = r.removeFromTop(30);
    g.setColour(TEXT); g.setFont(juce::Font(juce::FontOptions(22.0f, juce::Font::bold)));
    g.drawText("LGN DUCK", head.removeFromLeft(160), juce::Justification::centredLeft);
    drawCurve(g, curveArea);
    g.setFont(juce::Font(juce::FontOptions(12.0f, juce::Font::bold))); g.setColour(DIM);
    g.drawText("AMOUNT", amount.getBounds().withY(amount.getBottom() - 2).withHeight(16), juce::Justification::centred);
    g.drawText("RELEASE", release.getBounds().withY(release.getBottom() - 2).withHeight(16), juce::Justification::centred);
    g.setColour(DIM); g.setFont(12.0f);
    g.drawText("RATE", rateBtn[0].getBounds().withX(16).withWidth(50), juce::Justification::centredLeft);
}

void DuckEditor::resized() {
    auto r = getLocalBounds().reduced(16);
    auto head = r.removeFromTop(30);
    auto modes = head.removeFromRight(170);
    modeBtn[0].setBounds(modes.removeFromLeft(85)); modeBtn[1].setBounds(modes);
    r.removeFromTop(10);
    auto bottom = r.removeFromBottom(36);
    bottom.removeFromLeft(50);
    const int bw = bottom.getWidth() / 5;
    for (int i = 0; i < 5; ++i) rateBtn[i].setBounds(bottom.removeFromLeft(bw));
    r.removeFromBottom(30);
    auto knobs = r.removeFromRight(150);
    amount.setBounds(knobs.removeFromTop(knobs.getHeight() * 2 / 3).reduced(4, 0).withTrimmedBottom(14));
    release.setBounds(knobs.withSizeKeepingCentre(64, knobs.getHeight()).withTrimmedBottom(14));
    curveArea = r.withTrimmedRight(12).toFloat();
}
