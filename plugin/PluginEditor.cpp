#include "PluginEditor.h"
#include "EditPages.h"

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
    ui::drawRecess(g, r, 4);
    const auto in = r.reduced(12);
    // dot grid, like a touch surface
    for (int i = 0; i <= 8; i++) for (int j = 0; j <= 8; j++) {
        const float x = in.getX() + in.getWidth() * i / 8, y = in.getY() + in.getHeight() * j / 8;
        g.setColour((i == 4 || j == 4) ? ui::accent.withAlpha(0.25f) : juce::Colours::white.withAlpha(0.10f)); g.fillEllipse(x - 1.2f, y - 1.2f, 2.4f, 2.4f);
    }
    auto toPt = [&in](float x, float y) { return juce::Point<float>(in.getCentreX() + x * in.getWidth() / 2, in.getCentreY() - y * in.getHeight() / 2); };
    const auto set = toPt(px->convertFrom0to1(px->getValue()), py->convertFrom0to1(py->getValue()));
    const auto live = toPt(proc.orbX.load(), proc.orbY.load());
    g.setColour(ui::accent.withAlpha(0.18f)); g.drawHorizontalLine((int)live.y, in.getX(), in.getRight()); g.drawVerticalLine((int)live.x, in.getY(), in.getBottom());
    g.setColour(ui::dim); g.drawEllipse(set.x - 8, set.y - 8, 16, 16, 1.2f);
    for (int k = 5; k >= 1; k--) { g.setColour(ui::accent.withAlpha(0.06f * (6 - k))); const float sz = 10.0f + k * 8; g.fillEllipse(live.x - sz / 2, live.y - sz / 2, sz, sz); }
    g.setColour(ui::accent.brighter(0.4f)); g.fillEllipse(live.x - 6, live.y - 6, 12, 12);
    g.setColour(juce::Colours::white.withAlpha(0.8f)); g.fillEllipse(live.x - 2.5f, live.y - 2.5f, 5, 5);
    ui::drawEngraved(g, "X", in.toNearestInt().removeFromBottom(14).removeFromRight(14), juce::Justification::centred, 11);
    ui::drawEngraved(g, "Y", in.toNearestInt().removeFromTop(14).removeFromLeft(14), juce::Justification::centred, 11);
}

// ------------------------------------------------------------------ keys
void HardwareKeys::drawWhiteNote(int note, juce::Graphics& g, juce::Rectangle<float> a, bool down, bool over, juce::Colour, juce::Colour) {
    const auto k = a.reduced(0.6f, 0);
    const float lip = down ? 3.0f : 7.0f;
    g.setGradientFill(juce::ColourGradient(juce::Colour(0xffd8d4cb), k.getX(), k.getY(), juce::Colour(0xfff4f1ea), k.getX(), k.getBottom() - lip, false));
    g.fillRect(k.withTrimmedBottom(lip));
    g.setGradientFill(juce::ColourGradient(juce::Colour(0xffcfcac0), k.getX(), k.getBottom() - lip, juce::Colour(0xff9c978e), k.getX(), k.getBottom(), false));
    g.fillRect(k.withTop(k.getBottom() - lip));
    g.setColour(juce::Colours::black.withAlpha(0.35f)); g.drawVerticalLine((int)a.getRight() - 1, a.getY(), a.getBottom());
    g.setColour(juce::Colours::white.withAlpha(0.6f)); g.drawVerticalLine((int)a.getX() + 1, a.getY(), a.getBottom() - lip);
    if (down) { g.setColour(ui::accent.withAlpha(0.45f)); g.fillRect(k.withTrimmedBottom(lip)); }
    else if (over) { g.setColour(ui::accent.withAlpha(0.12f)); g.fillRect(k); }
    if (note % 12 == 0) {
        g.setFont(ui::font(11.0f, true)); g.setColour(juce::Colour(0xff7a756c));
        g.drawText(getWhiteNoteText(note), k.withTrimmedBottom(lip + 2).removeFromBottom(14), juce::Justification::centred);
    }
}
void HardwareKeys::drawBlackNote(int, juce::Graphics& g, juce::Rectangle<float> a, bool down, bool over, juce::Colour) {
    g.setColour(juce::Colours::black.withAlpha(0.45f)); g.fillRoundedRectangle(a.translated(1.5f, 2.0f), 2);
    g.setGradientFill(juce::ColourGradient(juce::Colour(0xff2a2b2e), a.getX(), a.getY(), juce::Colour(0xff060607), a.getX(), a.getBottom(), false));
    g.fillRoundedRectangle(a, 2);
    const auto top = a.reduced(a.getWidth() * 0.16f, 0).withTrimmedBottom(down ? 4.0f : 9.0f);
    g.setGradientFill(juce::ColourGradient(juce::Colour(0xff17181a), top.getX(), top.getY(), juce::Colour(0xff3a3c40), top.getX(), top.getBottom(), false));
    g.fillRoundedRectangle(top, 1.5f);
    g.setColour(juce::Colours::white.withAlpha(0.22f)); g.drawLine(top.getX() + 1, top.getBottom() - 1, top.getRight() - 1, top.getBottom() - 1, 1.0f);
    if (down) { g.setColour(ui::accent.withAlpha(0.6f)); g.fillRoundedRectangle(top, 1.5f); }
    else if (over) { g.setColour(ui::accent.withAlpha(0.2f)); g.fillRoundedRectangle(a, 2); }
}

// ------------------------------------------------------------------ knob
Knob::Knob(juce::AudioProcessorValueTreeState& s, const char* id, const juce::String& text) {
    slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    slider.setDoubleClickReturnValue(true, (double)s.getParameter(id)->convertFrom0to1(s.getParameter(id)->getDefaultValue()));
    addAndMakeVisible(slider);
    label.setText(text, juce::dontSendNotification); label.setJustificationType(juce::Justification::centred);
    label.setFont(ui::font(14.0f, true)); label.setColour(juce::Label::textColourId, ui::text.withAlpha(0.85f)); label.setMinimumHorizontalScale(0.7f); label.getProperties().set("caps", true);
    addAndMakeVisible(label);
    att = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(s, id, slider);
}
void Knob::resized() { auto r = getLocalBounds(); label.setBounds(r.removeFromBottom(20)); slider.setBounds(r); }

// ------------------------------------------------------------------ editor
AetherEditor::AetherEditor(AetherProcessor& p)
    : AudioProcessorEditor(&p), proc(p), orb(p), keys(p.keyboard, juce::MidiKeyboardComponent::horizontalKeyboard) {
    setLookAndFeel(&look);
    const auto& lib = Library::get();

    search.setTextToShowWhenEmpty("SEARCH PRESETS / TAGS", ui::lcdDim); search.setJustification(juce::Justification::centredLeft); search.setIndents(8, 0); search.setFont(ui::mono(14.0f));
    search.onTextChange = [this] { refilter(); };
    addAndMakeVisible(search);
    catBox.addItem("All categories (" + juce::String((int)lib.entries.size()) + ")", 1);
    for (size_t i = 0; i < lib.cats.size(); i++) catBox.addItem(lib.cats[i], (int)i + 2);
    catBox.setSelectedId(1, juce::dontSendNotification);
    catBox.onChange = [this] { refilter(); };
    addAndMakeVisible(catBox);
    list.setRowHeight(24); list.setColour(juce::ListBox::outlineColourId, juce::Colours::black); list.setOutlineThickness(1);
    addAndMakeVisible(list);

    prev.onClick = [this] { proc.stepPreset(-1, &shown); };
    next.onClick = [this] { proc.stepPreset(1, &shown); };
    rnd.onClick = [this] { if (!shown.empty()) proc.loadPreset(shown[(size_t)random.nextInt((int)shown.size())]); };
    prev.setButtonText(juce::String::fromUTF8("\xe2\x97\x80")); next.setButtonText(juce::String::fromUTF8("\xe2\x96\xb6"));
    for (auto* b : { &prev, &next, &rnd }) addAndMakeVisible(b);

    nameLabel.setFont(ui::mono(30.0f)); nameLabel.setColour(juce::Label::textColourId, ui::lcdText); nameLabel.setMinimumHorizontalScale(0.6f); addAndMakeVisible(nameLabel);
    catLabel.setFont(ui::mono(14.0f)); catLabel.setColour(juce::Label::textColourId, ui::lcdText.withAlpha(0.7f)); addAndMakeVisible(catLabel);
    statusLabel.setFont(ui::mono(14.0f)); statusLabel.setColour(juce::Label::textColourId, ui::lcdText.withAlpha(0.7f)); statusLabel.setJustificationType(juce::Justification::centredRight); addAndMakeVisible(statusLabel);
    sectionLcd.setFont(ui::mono(14.0f)); sectionLcd.setColour(juce::Label::textColourId, ui::lcdText.withAlpha(0.7f)); sectionLcd.setJustificationType(juce::Justification::centredRight); addAndMakeVisible(sectionLcd);
    for (auto* l : { &nameLabel, &catLabel, &statusLabel, &sectionLcd }) l->setInterceptsMouseClicks(false, false);


    addAndMakeVisible(orb);

    const char* mid[4] = { "mac1", "mac2", "mac3", "mac4" };
    for (int i = 0; i < 4; i++) { macros[i] = std::make_unique<Knob>(p.apvts, mid[i], "Macro " + juce::String(i + 1)); addAndMakeVisible(*macros[i]); }
    const char* pid[6] = { "cutoff", "reso", "attack", "release", "reverb", "delay" }; const char* pn[6] = { "Cutoff", "Resonance", "Attack", "Release", "Reverb", "Delay" };
    for (int i = 0; i < 6; i++) { perf[i] = std::make_unique<Knob>(p.apvts, pid[i], pn[i]); addAndMakeVisible(*perf[i]); }
    const char* lid[4] = { "lvla", "lvlb", "lvlc", "lvld" };
    for (int i = 0; i < 4; i++) { layers[i] = std::make_unique<Knob>(p.apvts, lid[i], "Layer"); addAndMakeVisible(*layers[i]); }
    volume = std::make_unique<Knob>(p.apvts, "volume", "Volume"); addAndMakeVisible(*volume);

    keys.setColour(juce::MidiKeyboardComponent::whiteNoteColourId, juce::Colour(0xffe9e6df));
    keys.setColour(juce::MidiKeyboardComponent::blackNoteColourId, juce::Colour(0xff17181a));
    keys.setColour(juce::MidiKeyboardComponent::keySeparatorLineColourId, juce::Colour(0xff8c8a85));
    keys.setColour(juce::MidiKeyboardComponent::keyDownOverlayColourId, ui::accent.withAlpha(0.75f));
    keys.setColour(juce::MidiKeyboardComponent::mouseOverKeyOverlayColourId, ui::accent.withAlpha(0.25f));
    keys.setColour(juce::MidiKeyboardComponent::shadowColourId, juce::Colours::black.withAlpha(0.5f));
    keys.setColour(juce::MidiKeyboardComponent::textLabelColourId, juce::Colour(0xff6b6862));
    keys.setAvailableRange(24, 108); keys.setOctaveForMiddleC(5); keys.setLowestVisibleKey(24);
    addAndMakeVisible(keys);

    const char* tabNames[kPages + 1] = { "Play", "Layers", "Mod", "FX", "Arp", "Sections", "Crate" };
    for (int i = 0; i <= kPages; i++) {
        pageTabs[i].setButtonText(tabNames[i]); pageTabs[i].getProperties().set("led", true);
        pageTabs[i].onClick = [this, i] { showPage(i); };
        addAndMakeVisible(pageTabs[i]);
    }
    pages[0] = std::make_unique<edit::LayersPage>(p); pages[1] = std::make_unique<edit::ModPage>(p);
    pages[2] = std::make_unique<edit::FxPage>(p); pages[3] = std::make_unique<edit::ArpPage>(p);
    pages[4] = std::make_unique<edit::SectionsPage>(p); pages[5] = std::make_unique<edit::CratePage>(p);
    for (int i = 0; i < kPages; i++) { scrollers[i] = std::make_unique<edit::Scroller>(*pages[i]); addChildComponent(*scrollers[i]); }
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
    for (juce::Component* c : { (juce::Component*)&orb, (juce::Component*)volume.get() }) c->setVisible(play);
    for (auto& k : macros) k->setVisible(play); for (auto& k : perf) k->setVisible(play); for (auto& k : layers) k->setVisible(play);
    for (int k = 0; k < kPages; k++) scrollers[k]->setVisible(i == k + 1);
    if (!play) { pages[i - 1]->rebuild(); }
    for (int k = 0; k <= kPages; k++) pageTabs[k].setToggleState(k == i, juce::dontSendNotification);
    resized(); repaint();
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
    nameLabel.setText(inf.name.toUpperCase() + (proc.isEdited() ? " *" : ""), juce::dontSendNotification);
    if (proc.patchGeneration() != lastGen) { lastGen = proc.patchGeneration(); if (page > 0) pages[page - 1]->rebuild(); }
    {
        const auto& lib = Library::get(); const int idx = proc.currentIndex();
        catLabel.setText(inf.cat.toUpperCase() + (idx >= 0 ? "   " + juce::String(idx + 1).paddedLeft('0', 4) + "/" + juce::String((int)lib.entries.size()) : juce::String()), juce::dontSendNotification);
    }
    for (int i = 0; i < 4; i++) {
        macros[i]->label.setText(inf.macro[i], juce::dontSendNotification);
        layers[i]->label.setText(juce::String::charToString((juce::juce_wchar)('A' + i)) + " " + (inf.layerOn[i] ? inf.layer[i] : juce::String("off")), juce::dontSendNotification);
        layers[i]->setEnabled(inf.layerOn[i]); layers[i]->setAlpha(inf.layerOn[i] ? 1.0f : 0.35f);
    }
    const auto it = std::find(shown.begin(), shown.end(), proc.currentIndex());
    if (it != shown.end()) { const int row = (int)(it - shown.begin()); list.selectRow(row, true, true); }
    else list.deselectAllRows();
    list.repaint();
}
void AetherEditor::timerCallback() {
    statusLabel.setText("VOICES " + juce::String(proc.voiceCount.load()).paddedLeft('0', 2), juce::dontSendNotification);
    const float sn = proc.sectionNow.load();
    sectionLcd.setText(sn < 0 ? juce::String("SECTIONS OFF") : "SECTION " + juce::String(aether::sectionName(juce::roundToInt(sn))).toUpperCase(), juce::dontSendNotification);
}
void AetherEditor::paintListBoxItem(int row, juce::Graphics& g, int w, int h, bool) {
    if (row < 0 || row >= (int)shown.size()) return;
    const auto& e = Library::get().entries[(size_t)shown[(size_t)row]];
    const bool cur = shown[(size_t)row] == proc.currentIndex();
    if (cur) { g.setColour(ui::lcdText); g.fillRect(0, 1, w, h - 2); }
    else if (row % 2) { g.setColour(juce::Colours::white.withAlpha(0.02f)); g.fillRect(0, 0, w, h); }
    g.setFont(ui::mono(15.0f)); g.setColour(cur ? ui::lcdBg : ui::lcdText);
    g.drawText(juce::String(e.name).toUpperCase(), 10, 0, w - 112, h, juce::Justification::centredLeft, true);
    g.setFont(ui::mono(11.5f)); g.setColour(cur ? ui::lcdBg.withAlpha(0.8f) : ui::lcdDim.brighter(0.4f));
    g.drawText(juce::String(e.cat).toUpperCase(), w - 110, 0, 102, h, juce::Justification::centredRight, true);
}
void AetherEditor::listBoxItemClicked(int row, const juce::MouseEvent&) { if (row >= 0 && row < (int)shown.size()) proc.loadPreset(shown[(size_t)row]); }

void AetherEditor::paint(juce::Graphics& g) {
    // chassis with end cheeks
    g.fillAll(ui::bg);
    const auto b = getLocalBounds().toFloat();
    g.setGradientFill(juce::ColourGradient(juce::Colour(0xff222428), 0, 0, juce::Colour(0xff121315), 0, b.getHeight(), false));
    g.fillRect(b);
    g.setTiledImageFill(ui::anodizedTile(), 0, 0, 1.0f); g.fillRect(b);
    g.setGradientFill(juce::ColourGradient(juce::Colours::white.withAlpha(0.03f), 0, 0, juce::Colours::black.withAlpha(0.35f), 0, b.getHeight(), false)); g.fillRect(b);
    ui::drawWoodCheek(g, { 0.0f, 0.0f, 16.0f, b.getHeight() });
    ui::drawWoodCheek(g, { b.getWidth() - 16, 0.0f, 16.0f, b.getHeight() });
    // header plate: brand + display
    const auto head = juce::Rectangle<float>(24, 10, b.getWidth() - 48, 92);
    ui::drawBrushedPlate(g, head, true);
    {
        auto br = head.reduced(26, 0).withWidth(200).toNearestInt();
        g.setFont(juce::Font(ui::font(40.0f, true)).withExtraKerningFactor(0.18f));
        g.setColour(juce::Colours::white.withAlpha(0.45f)); g.drawText("AETHER", br.withTrimmedBottom(18).translated(0, 1), juce::Justification::bottomLeft);
        g.setColour(juce::Colour(0xff141516)); g.drawText("AETHER", br.withTrimmedBottom(18), juce::Justification::bottomLeft);
        ui::drawPrinted(g, "LAYERED SYNTHESIS ENGINE", br.withTrimmedTop(br.getHeight() - 24).withTrimmedBottom(6), juce::Justification::topLeft, 11.5f, juce::Colour(0xff2a2b2d));
    }
    ui::drawLcd(g, lcdArea.toFloat());
    // browser and the play page plates
    ui::drawPlate(g, browserPlate.toFloat(), "Library");
    if (page == 0) {
        static const char* titles[4] = { "Macros", "Shape", "Layers & Output", "Orb" };
        for (int i = 0; i < 4; i++) ui::drawPlate(g, playPlates[i].toFloat(), titles[i]);
    }
    // keyboard bed
    const auto kb = keys.getBounds().toFloat().expanded(8, 8);
    ui::drawPlate(g, kb.withTrimmedTop(-4));
    g.setColour(juce::Colour(0xff7a1f12)); g.fillRect(keys.getX() - 1.0f, keys.getY() - 4.0f, keys.getWidth() + 2.0f, 4.0f);   // felt strip
}
void AetherEditor::resized() {
    auto r = getLocalBounds().reduced(16, 10).withTrimmedLeft(12).withTrimmedRight(12);
    // header
    auto head = r.removeFromTop(92).reduced(26, 12);
    head.removeFromLeft(210);
    {
        auto btns = head.removeFromRight(196).withSizeKeepingCentre(196, 46);
        prev.setBounds(btns.removeFromLeft(48)); btns.removeFromLeft(6); next.setBounds(btns.removeFromLeft(48)); btns.removeFromLeft(6); rnd.setBounds(btns);
        head.removeFromRight(14);
        lcdArea = head.reduced(4, 2);
        auto l = lcdArea.reduced(12, 3);
        auto top = l.removeFromTop(18);
        catLabel.setBounds(top.removeFromLeft(top.getWidth() / 2));
        statusLabel.setBounds(top.removeFromRight(90)); sectionLcd.setBounds(top);
        nameLabel.setBounds(l);
    }
    r.removeFromTop(12);
    keys.setBounds(r.removeFromBottom(84).reduced(10, 0).withTrimmedBottom(8)); keys.setKeyWidth(keys.getWidth() / 50.0f);
    r.removeFromBottom(18);
    // library
    browserPlate = r.removeFromLeft(juce::jmax(290, r.getWidth() * 28 / 100));
    {
        auto left = browserPlate.reduced(12).withTrimmedTop(18);
        catBox.setBounds(left.removeFromTop(32)); left.removeFromTop(8);
        search.setBounds(left.removeFromTop(32)); left.removeFromTop(8);
        list.setBounds(left);
    }
    r.removeFromLeft(14);
    {
        auto tabs = r.removeFromTop(40);
        const int tw = juce::jmin(96, (tabs.getWidth() - 6 * kPages) / (kPages + 1));
        for (auto& t : pageTabs) { t.setBounds(tabs.removeFromLeft(tw)); tabs.removeFromLeft(6); }
    }
    r.removeFromTop(10);
    centre = r;
    for (auto& s : scrollers) if (s) s->setBounds(r);
    if (page != 0) return;
    // play page: macros + shape on the left, orb on the right, layers along the bottom
    auto bottom = r.removeFromBottom(juce::jlimit(120, 150, r.getHeight() * 30 / 100)); r.removeFromBottom(10);
    playPlates[2] = bottom;
    playPlates[3] = r.removeFromRight(juce::jmin(r.getHeight(), r.getWidth() * 36 / 100)); r.removeFromRight(10);
    const int mh = r.getHeight() * 55 / 100;
    playPlates[0] = r.removeFromTop(mh); r.removeFromTop(10); playPlates[1] = r;
    auto knobRow = [](juce::Rectangle<int> area, std::unique_ptr<Knob>* ks, int n) { const int w = area.getWidth() / n; for (int i = 0; i < n; i++) ks[i]->setBounds(area.removeFromLeft(w).reduced(4, 0)); };
    knobRow(playPlates[0].reduced(10, 8).withTrimmedTop(16), macros, 4);
    knobRow(playPlates[1].reduced(10, 8).withTrimmedTop(16), perf, 6);
    {
        auto lr = playPlates[2].reduced(10, 8).withTrimmedTop(16); const int w = lr.getWidth() / 6;
        for (int i = 0; i < 4; i++) layers[i]->setBounds(lr.removeFromLeft(w).reduced(4, 0));
        lr.removeFromLeft(w / 2); volume->setBounds(lr.removeFromLeft(w).reduced(4, 0));
    }
    orb.setBounds(playPlates[3].reduced(12).withTrimmedTop(16));
}
