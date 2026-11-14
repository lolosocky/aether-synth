#include "PluginEditor.h"
#include "EditPages.h"

// ------------------------------------------------------------------ look
AetherLook::AetherLook() {
    setColour(juce::ResizableWindow::backgroundColourId, ui::bg);
    setColour(juce::Label::textColourId, ui::text);
    setColour(juce::TextEditor::backgroundColourId, ui::panel);
    setColour(juce::TextEditor::outlineColourId, ui::line);
    setColour(juce::TextEditor::focusedOutlineColourId, ui::accent);
    setColour(juce::TextEditor::textColourId, ui::text);
    setColour(juce::ComboBox::backgroundColourId, ui::panel);
    setColour(juce::ComboBox::outlineColourId, ui::line);
    setColour(juce::ComboBox::textColourId, ui::text);
    setColour(juce::ComboBox::arrowColourId, ui::dim);
    setColour(juce::PopupMenu::backgroundColourId, ui::panel);
    setColour(juce::PopupMenu::textColourId, ui::text);
    setColour(juce::PopupMenu::highlightedBackgroundColourId, ui::accent.withAlpha(0.3f));
    setColour(juce::ListBox::backgroundColourId, ui::panel);
    setColour(juce::TextButton::buttonColourId, ui::panel);
    setColour(juce::TextButton::textColourOffId, ui::text);
    setColour(juce::ScrollBar::thumbColourId, ui::line.brighter(0.3f));
    setColour(juce::Slider::textBoxTextColourId, ui::dim);
    setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
}
void AetherLook::drawRotarySlider(juce::Graphics& g, int x, int y, int w, int h, float pos, float a0, float a1, juce::Slider& s) {
    const auto r = juce::Rectangle<float>((float)x, (float)y, (float)w, (float)h).reduced(4);
    const float rad = juce::jmin(r.getWidth(), r.getHeight()) / 2, cx = r.getCentreX(), cy = r.getCentreY();
    const bool bipolar = s.getMinimum() < 0;
    const float zero = bipolar ? (a0 + a1) / 2 : a0, ang = a0 + pos * (a1 - a0);
    juce::Path track; track.addCentredArc(cx, cy, rad - 3, rad - 3, 0, a0, a1, true);
    g.setColour(ui::line); g.strokePath(track, juce::PathStrokeType(4, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    juce::Path val; val.addCentredArc(cx, cy, rad - 3, rad - 3, 0, juce::jmin(zero, ang), juce::jmax(zero, ang), true);
    g.setGradientFill(juce::ColourGradient(ui::accent, cx - rad, cy, ui::accent2, cx + rad, cy, false));
    g.strokePath(val, juce::PathStrokeType(4, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    g.setColour(ui::panel.brighter(0.15f)); g.fillEllipse(cx - rad + 9, cy - rad + 9, (rad - 9) * 2, (rad - 9) * 2);
    juce::Path tick; tick.addRoundedRectangle(-1.5f, -(rad - 10), 3.0f, rad * 0.45f, 1.5f);
    g.setColour(ui::text); g.fillPath(tick, juce::AffineTransform::rotation(ang).translated(cx, cy));
}
void AetherLook::drawLinearSlider(juce::Graphics& g, int x, int y, int w, int h, float pos, float, float, juce::Slider::SliderStyle, juce::Slider&) {
    const float cy = y + h * 0.5f;
    g.setColour(ui::line); g.fillRoundedRectangle((float)x, cy - 2, (float)w, 4, 2);
    g.setColour(ui::accent); g.fillRoundedRectangle((float)x, cy - 2, pos - x, 4, 2);
    g.setColour(ui::text); g.fillEllipse(pos - 6, cy - 6, 12, 12);
}
void AetherLook::drawButtonBackground(juce::Graphics& g, juce::Button& b, const juce::Colour&, bool over, bool down) {
    auto r = b.getLocalBounds().toFloat().reduced(0.5f);
    const bool on = b.getClickingTogglesState() && b.getToggleState();
    g.setColour(on ? ui::accent.withAlpha(down ? 0.6f : 0.85f) : down ? ui::accent.withAlpha(0.35f) : over ? ui::panel.brighter(0.12f) : ui::panel); g.fillRoundedRectangle(r, 6);
    g.setColour(ui::line); g.drawRoundedRectangle(r, 6, 1);
}

// ------------------------------------------------------------------ orb
OrbPad::OrbPad(AetherProcessor& p) : proc(p), px(p.apvts.getParameter("orbx")), py(p.apvts.getParameter("orby")) { startTimerHz(30); setRepaintsOnMouseActivity(false); }
void OrbPad::drag(const juce::MouseEvent& e) {
    const auto r = getLocalBounds().toFloat().reduced(12);
    const float x = juce::jlimit(-1.0f, 1.0f, (e.position.x - r.getCentreX()) / (r.getWidth() / 2));
    const float y = juce::jlimit(-1.0f, 1.0f, -(e.position.y - r.getCentreY()) / (r.getHeight() / 2));
    if (!held) { px->beginChangeGesture(); py->beginChangeGesture(); held = true; }
    px->setValueNotifyingHost(px->convertTo0to1(x)); py->setValueNotifyingHost(py->convertTo0to1(y));
}
void OrbPad::mouseUp(const juce::MouseEvent&) { if (held) { px->endChangeGesture(); py->endChangeGesture(); held = false; } }
void OrbPad::mouseDoubleClick(const juce::MouseEvent&) { for (auto* p : { px, py }) { p->beginChangeGesture(); p->setValueNotifyingHost(p->convertTo0to1(0)); p->endChangeGesture(); } }
void OrbPad::paint(juce::Graphics& g) {
    auto r = getLocalBounds().toFloat();
    g.setColour(ui::panel); g.fillRoundedRectangle(r, 10);
    g.setColour(ui::line); g.drawRoundedRectangle(r.reduced(0.5f), 10, 1);
    const auto in = r.reduced(12);
    g.setColour(ui::line.withAlpha(0.6f));
    for (int i = 1; i < 4; i++) { const float fx = in.getX() + in.getWidth() * i / 4, fy = in.getY() + in.getHeight() * i / 4; g.drawVerticalLine((int)fx, in.getY(), in.getBottom()); g.drawHorizontalLine((int)fy, in.getX(), in.getRight()); }
    auto toPt = [&in](float x, float y) { return juce::Point<float>(in.getCentreX() + x * in.getWidth() / 2, in.getCentreY() - y * in.getHeight() / 2); };
    const auto set = toPt(px->convertFrom0to1(px->getValue()), py->convertFrom0to1(py->getValue()));
    const auto live = toPt(proc.orbX.load(), proc.orbY.load());
    g.setColour(ui::dim.withAlpha(0.6f)); g.drawEllipse(set.x - 7, set.y - 7, 14, 14, 1.5f);
    for (int k = 4; k >= 1; k--) { g.setColour(ui::accent2.withAlpha(0.08f * (5 - k))); const float s = 10.0f + k * 7; g.fillEllipse(live.x - s / 2, live.y - s / 2, s, s); }
    g.setGradientFill(juce::ColourGradient(ui::accent.brighter(0.4f), live.x - 6, live.y - 6, ui::accent2, live.x + 6, live.y + 6, false));
    g.fillEllipse(live.x - 8, live.y - 8, 16, 16);
}

// ------------------------------------------------------------------ knob
Knob::Knob(juce::AudioProcessorValueTreeState& s, const char* id, const juce::String& text) {
    slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    slider.setDoubleClickReturnValue(true, (double)s.getParameter(id)->convertFrom0to1(s.getParameter(id)->getDefaultValue()));
    addAndMakeVisible(slider);
    label.setText(text, juce::dontSendNotification); label.setJustificationType(juce::Justification::centred);
    label.setFont(juce::FontOptions(12.0f)); label.setColour(juce::Label::textColourId, ui::dim); label.setMinimumHorizontalScale(0.7f);
    addAndMakeVisible(label);
    att = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(s, id, slider);
}
void Knob::resized() { auto r = getLocalBounds(); label.setBounds(r.removeFromBottom(18)); slider.setBounds(r); }

// ------------------------------------------------------------------ editor
AetherEditor::AetherEditor(AetherProcessor& p)
    : AudioProcessorEditor(&p), proc(p), orb(p), keys(p.keyboard, juce::MidiKeyboardComponent::horizontalKeyboard) {
    setLookAndFeel(&look);
    const auto& lib = Library::get();

    search.setTextToShowWhenEmpty("Search presets or tags", ui::dim); search.setJustification(juce::Justification::centredLeft); search.setIndents(8, 0);
    search.onTextChange = [this] { refilter(); };
    addAndMakeVisible(search);
    catBox.addItem("All categories (" + juce::String((int)lib.entries.size()) + ")", 1);
    for (size_t i = 0; i < lib.cats.size(); i++) catBox.addItem(lib.cats[i], (int)i + 2);
    catBox.setSelectedId(1, juce::dontSendNotification);
    catBox.onChange = [this] { refilter(); };
    addAndMakeVisible(catBox);
    list.setRowHeight(24); list.setColour(juce::ListBox::outlineColourId, ui::line); list.setOutlineThickness(1);
    addAndMakeVisible(list);

    prev.onClick = [this] { proc.stepPreset(-1, &shown); };
    next.onClick = [this] { proc.stepPreset(1, &shown); };
    rnd.onClick = [this] { if (!shown.empty()) proc.loadPreset(shown[(size_t)random.nextInt((int)shown.size())]); };
    for (auto* b : { &prev, &next, &rnd }) addAndMakeVisible(b);

    nameLabel.setFont(juce::FontOptions(26.0f, juce::Font::bold)); addAndMakeVisible(nameLabel);
    catLabel.setFont(juce::FontOptions(13.0f)); catLabel.setColour(juce::Label::textColourId, ui::accent); addAndMakeVisible(catLabel);
    statusLabel.setFont(juce::FontOptions(12.0f)); statusLabel.setColour(juce::Label::textColourId, ui::dim); statusLabel.setJustificationType(juce::Justification::centredRight); addAndMakeVisible(statusLabel);

    auto section = [this](juce::Label& l, const char* t) { l.setText(t, juce::dontSendNotification); l.setFont(juce::FontOptions(11.0f, juce::Font::bold)); l.setColour(juce::Label::textColourId, ui::dim); addAndMakeVisible(l); };
    section(secMacro, "MACROS"); section(secPerf, "SHAPE"); section(secLayers, "LAYERS"); section(secOrb, "ORB");
    addAndMakeVisible(orb);

    const char* mid[4] = { "mac1", "mac2", "mac3", "mac4" };
    for (int i = 0; i < 4; i++) { macros[i] = std::make_unique<Knob>(p.apvts, mid[i], "Macro " + juce::String(i + 1)); addAndMakeVisible(*macros[i]); }
    const char* pid[6] = { "cutoff", "reso", "attack", "release", "reverb", "delay" }; const char* pn[6] = { "Cutoff", "Resonance", "Attack", "Release", "Reverb", "Delay" };
    for (int i = 0; i < 6; i++) { perf[i] = std::make_unique<Knob>(p.apvts, pid[i], pn[i]); addAndMakeVisible(*perf[i]); }
    const char* lid[4] = { "lvla", "lvlb", "lvlc", "lvld" };
    for (int i = 0; i < 4; i++) { layers[i] = std::make_unique<Knob>(p.apvts, lid[i], "Layer"); addAndMakeVisible(*layers[i]); }
    volume = std::make_unique<Knob>(p.apvts, "volume", "Volume"); addAndMakeVisible(*volume);

    keys.setColour(juce::MidiKeyboardComponent::keySeparatorLineColourId, ui::line);
    keys.setColour(juce::MidiKeyboardComponent::keyDownOverlayColourId, ui::accent.withAlpha(0.6f));
    keys.setColour(juce::MidiKeyboardComponent::mouseOverKeyOverlayColourId, ui::accent.withAlpha(0.2f));
    keys.setAvailableRange(24, 108); keys.setLowestVisibleKey(24);
    addAndMakeVisible(keys);

    const char* tabNames[5] = { "Play", "Layers", "Mod", "FX", "Arp" };
    for (int i = 0; i < 5; i++) {
        pageTabs[i].setButtonText(tabNames[i]);
        pageTabs[i].onClick = [this, i] { showPage(i); };
        addAndMakeVisible(pageTabs[i]);
    }
    pages[0] = std::make_unique<edit::LayersPage>(p); pages[1] = std::make_unique<edit::ModPage>(p);
    pages[2] = std::make_unique<edit::FxPage>(p); pages[3] = std::make_unique<edit::ArpPage>(p);
    for (int i = 0; i < 4; i++) { scrollers[i] = std::make_unique<edit::Scroller>(*pages[i]); addChildComponent(*scrollers[i]); }
    lastGen = p.patchGeneration();
    p.addChangeListener(this);
    refilter(); refreshInfo();
    showPage(0);
    setResizable(true, true); setResizeLimits(900, 600, 1800, 1200);
    setSize(1120, 720);
    startTimerHz(10);
}
AetherEditor::~AetherEditor() { proc.removeChangeListener(this); for (auto& s : scrollers) s.reset(); for (auto& pg : pages) pg.reset(); setLookAndFeel(nullptr); }

void AetherEditor::showPage(int i) {
    page = i;
    const bool play = i == 0;
    for (juce::Component* c : { (juce::Component*)&secMacro, (juce::Component*)&secPerf, (juce::Component*)&secLayers, (juce::Component*)&secOrb, (juce::Component*)&orb, (juce::Component*)volume.get() }) c->setVisible(play);
    for (auto& k : macros) k->setVisible(play); for (auto& k : perf) k->setVisible(play); for (auto& k : layers) k->setVisible(play);
    for (int k = 0; k < 4; k++) scrollers[k]->setVisible(i == k + 1);
    if (!play) { pages[i - 1]->rebuild(); }
    for (int k = 0; k < 5; k++) {
        pageTabs[k].setColour(juce::TextButton::buttonColourId, k == i ? ui::accent.withAlpha(0.35f) : ui::panel);
        pageTabs[k].setColour(juce::TextButton::textColourOffId, k == i ? ui::text : ui::dim);
    }
    resized();
}

void AetherEditor::refilter() {
    const auto& lib = Library::get();
    const juce::String q = search.getText().trim().toLowerCase();
    const int ci = catBox.getSelectedId() - 2;
    const std::string cat = ci >= 0 && ci < (int)lib.cats.size() ? lib.cats[(size_t)ci] : std::string();
    juce::StringArray words; words.addTokens(q, " ", ""); words.removeEmptyStrings();
    shown.clear();
    for (size_t i = 0; i < lib.entries.size(); i++) {
        const auto& e = lib.entries[i];
        if (!cat.empty() && e.cat != cat) continue;
        const juce::String hay = (juce::String(e.name) + " " + e.cat + " " + e.tags).toLowerCase();
        bool ok = true; for (const auto& w : words) if (!hay.contains(w)) { ok = false; break; }
        if (ok) shown.push_back((int)i);
    }
    list.updateContent(); list.repaint();
    refreshInfo();
}
void AetherEditor::refreshInfo() {
    const auto inf = proc.info();
    nameLabel.setText(inf.name + (proc.isEdited() ? "  (edited)" : ""), juce::dontSendNotification);
    if (proc.patchGeneration() != lastGen) { lastGen = proc.patchGeneration(); if (page > 0) pages[page - 1]->rebuild(); }
    catLabel.setText(inf.cat.toUpperCase(), juce::dontSendNotification);
    for (int i = 0; i < 4; i++) {
        macros[i]->label.setText(inf.macro[i], juce::dontSendNotification);
        layers[i]->label.setText(juce::String::charToString((juce::juce_wchar)('A' + i)) + "  " + (inf.layerOn[i] ? inf.layer[i] : juce::String("off")), juce::dontSendNotification);
        layers[i]->setEnabled(inf.layerOn[i]); layers[i]->setAlpha(inf.layerOn[i] ? 1.0f : 0.35f);
    }
    const auto it = std::find(shown.begin(), shown.end(), proc.currentIndex());
    if (it != shown.end()) { const int row = (int)(it - shown.begin()); list.selectRow(row, true, true); }
    else list.deselectAllRows();
    list.repaint();
}
void AetherEditor::timerCallback() {
    statusLabel.setText(juce::String(proc.voiceCount.load()) + " voices   " + juce::String((int)shown.size()) + " shown", juce::dontSendNotification);
}
void AetherEditor::paintListBoxItem(int row, juce::Graphics& g, int w, int h, bool) {
    if (row < 0 || row >= (int)shown.size()) return;
    const auto& e = Library::get().entries[(size_t)shown[(size_t)row]];
    const bool cur = shown[(size_t)row] == proc.currentIndex();
    if (cur) { g.setColour(ui::accent.withAlpha(0.22f)); g.fillRect(0, 0, w, h); g.setColour(ui::accent); g.fillRect(0, 0, 3, h); }
    g.setColour(cur ? ui::text : ui::text.withAlpha(0.88f)); g.setFont(juce::FontOptions(14.0f));
    g.drawText(juce::String(e.name), 10, 0, w - 120, h, juce::Justification::centredLeft, true);
    g.setColour(ui::dim); g.setFont(juce::FontOptions(11.0f));
    g.drawText(juce::String(e.cat), w - 114, 0, 108, h, juce::Justification::centredRight, true);
}
void AetherEditor::listBoxItemClicked(int row, const juce::MouseEvent&) { if (row >= 0 && row < (int)shown.size()) proc.loadPreset(shown[(size_t)row]); }

void AetherEditor::paint(juce::Graphics& g) {
    g.fillAll(ui::bg);
    g.setColour(ui::text); g.setFont(juce::FontOptions(15.0f, juce::Font::bold));
    g.drawText("AETHER", 18, 12, 120, 22, juce::Justification::centredLeft);
}
void AetherEditor::resized() {
    auto r = getLocalBounds().reduced(14);
    r.removeFromTop(28);
    keys.setBounds(r.removeFromBottom(78)); keys.setKeyWidth(keys.getWidth() / 50.0f);
    r.removeFromBottom(10);
    auto left = r.removeFromLeft(juce::jmax(280, r.getWidth() * 30 / 100));
    {
        auto top = left.removeFromTop(30); catBox.setBounds(top);
        left.removeFromTop(6); search.setBounds(left.removeFromTop(30)); left.removeFromTop(6);
        list.setBounds(left);
    }
    r.removeFromLeft(16);
    {
        auto head = r.removeFromTop(64);
        auto btns = head.removeFromRight(230).withSizeKeepingCentre(230, 32);
        prev.setBounds(btns.removeFromLeft(40)); btns.removeFromLeft(6); next.setBounds(btns.removeFromLeft(40)); btns.removeFromLeft(6); rnd.setBounds(btns.removeFromLeft(80));
        catLabel.setBounds(head.removeFromTop(20)); nameLabel.setBounds(head.removeFromTop(36));
    }
    statusLabel.setBounds(getLocalBounds().reduced(14).removeFromTop(24));
    {
        auto tabs = r.removeFromTop(30);
        for (auto& t : pageTabs) { t.setBounds(tabs.removeFromLeft(78)); tabs.removeFromLeft(6); }
    }
    r.removeFromTop(10);
    centre = r;
    for (auto& s : scrollers) if (s) s->setBounds(r);
    if (page != 0) return;
    const int rowH = juce::jlimit(84, 120, (r.getHeight() - 60) / 3);
    auto knobRow = [](juce::Rectangle<int> area, std::unique_ptr<Knob>* ks, int n) { const int w = area.getWidth() / n; for (int i = 0; i < n; i++) ks[i]->setBounds(area.removeFromLeft(w).reduced(4, 0)); };
    auto orbArea = r.removeFromRight(juce::jmin(r.getWidth() * 38 / 100, rowH * 3 + 20));
    secOrb.setBounds(orbArea.removeFromTop(18)); orb.setBounds(orbArea.withHeight(juce::jmin(orbArea.getHeight(), orbArea.getWidth())));
    r.removeFromRight(16);
    secMacro.setBounds(r.removeFromTop(18)); knobRow(r.removeFromTop(rowH), macros, 4); r.removeFromTop(6);
    secPerf.setBounds(r.removeFromTop(18)); knobRow(r.removeFromTop(rowH), perf, 6); r.removeFromTop(6);
    secLayers.setBounds(r.removeFromTop(18));
    auto lr = r.removeFromTop(rowH); const int w = lr.getWidth() / 5;
    for (int i = 0; i < 4; i++) layers[i]->setBounds(lr.removeFromLeft(w).reduced(4, 0));
    volume->setBounds(lr.reduced(4, 0));
}
