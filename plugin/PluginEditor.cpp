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
    station::drawWell(g, r, 10);
    const auto in = r.reduced(14);
    // dot grid, like a touch surface; the centre lines are a touch darker
    for (int i = 0; i <= 8; i++) for (int j = 0; j <= 8; j++) {
        const float x = in.getX() + in.getWidth() * i / 8, y = in.getY() + in.getHeight() * j / 8;
        g.setColour((i == 4 || j == 4) ? station::faint : station::hairline); g.fillEllipse(x - 1.3f, y - 1.3f, 2.6f, 2.6f);
    }
    auto toPt = [&in](float x, float y) { return juce::Point<float>(in.getCentreX() + x * in.getWidth() / 2, in.getCentreY() - y * in.getHeight() / 2); };
    const auto set = toPt(px->convertFrom0to1(px->getValue()), py->convertFrom0to1(py->getValue()));
    const auto live = toPt(proc.orbX.load(), proc.orbY.load());
    g.setColour(ui::accent.withAlpha(0.28f)); g.drawHorizontalLine((int)live.y, in.getX(), in.getRight()); g.drawVerticalLine((int)live.x, in.getY(), in.getBottom());
    g.setColour(station::faint); g.drawEllipse(set.x - 9, set.y - 9, 18, 18, 1.2f);
    for (int k = 4; k >= 1; k--) { g.setColour(ui::accent.withAlpha(0.05f * (5 - k))); const float sz = 12.0f + k * 9; g.fillEllipse(live.x - sz / 2, live.y - sz / 2, sz, sz); }
    station::drawShadow(g, { live.x - 8, live.y - 8, 16, 16 }, 8, 0.8f);
    g.setColour(ui::accent); g.fillEllipse(live.x - 8, live.y - 8, 16, 16);
    g.setColour(juce::Colours::white); g.fillEllipse(live.x - 3, live.y - 3, 6, 6);
    station::drawCaps(g, "X", in.withTrimmedTop(in.getHeight() - 14).withTrimmedLeft(in.getWidth() - 14), juce::Justification::centred, 9.5f, station::faint);
    station::drawCaps(g, "Y", in.withHeight(14).withWidth(14), juce::Justification::centred, 9.5f, station::faint);
}

// ------------------------------------------------------------------ keys
void HardwareKeys::drawWhiteNote(int note, juce::Graphics& g, juce::Rectangle<float> a, bool down, bool over, juce::Colour, juce::Colour) {
    const auto k = a.reduced(0.5f, 0).withTrimmedBottom(1);
    g.setColour(juce::Colours::white);
    g.fillRoundedRectangle(k, 3);
    if (down) { g.setColour(ui::accent.withAlpha(0.22f)); g.fillRoundedRectangle(k, 3); g.setColour(ui::accent); g.fillRoundedRectangle(k.getX() + 3, k.getBottom() - 5, k.getWidth() - 6, 2.5f, 1.2f); }
    else if (over) { g.setColour(station::well); g.fillRoundedRectangle(k, 3); }
    g.setColour(station::hairline); g.drawRoundedRectangle(k.reduced(0.5f), 3, 1.0f);
    if (note % 12 == 0) {
        g.setFont(station::mono(9.5f)); g.setColour(station::faint);
        g.drawText(getWhiteNoteText(note), k.withTrimmedBottom(7).removeFromBottom(12), juce::Justification::centred);
    }
}
void HardwareKeys::drawBlackNote(int, juce::Graphics& g, juce::Rectangle<float> a, bool down, bool over, juce::Colour) {
    const auto k = a.reduced(a.getWidth() * 0.06f, 0);
    g.setColour(juce::Colour(0xff1a2030).withAlpha(0.18f)); g.fillRoundedRectangle(k.translated(0, 1.5f), 3);
    g.setColour(down ? ui::accent : (over ? station::graphite.brighter(0.25f) : station::graphite)); g.fillRoundedRectangle(k, 3);
    g.setColour(juce::Colours::white.withAlpha(down ? 0.0f : 0.08f)); g.fillRoundedRectangle(k.reduced(2).withHeight(k.getHeight() * 0.55f), 2);
}

// ------------------------------------------------------------------ knob
Knob::Knob(juce::AudioProcessorValueTreeState& s, const char* id, const juce::String& text) {
    slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    slider.setDoubleClickReturnValue(true, (double)s.getParameter(id)->convertFrom0to1(s.getParameter(id)->getDefaultValue()));
    addAndMakeVisible(slider);
    label.setText(text, juce::dontSendNotification); label.setJustificationType(juce::Justification::centred);
    label.setFont(station::caps(10.5f)); label.setColour(juce::Label::textColourId, station::dim); label.setMinimumHorizontalScale(0.7f); label.getProperties().set("caps", true);
    addAndMakeVisible(label);
    att = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(s, id, slider);
}
void Knob::resized() { auto r = getLocalBounds(); label.setBounds(r.removeFromBottom(20)); slider.setBounds(r); }

// ------------------------------------------------------------------ editor
AetherEditor::AetherEditor(AetherProcessor& p)
    : AudioProcessorEditor(&p), proc(p), orb(p), keys(p.keyboard, juce::MidiKeyboardComponent::horizontalKeyboard) {
    setLookAndFeel(&look);
    const auto& lib = Library::get();

    search.setTextToShowWhenEmpty("Search presets and tags", station::faint); search.setJustification(juce::Justification::centredLeft); search.setIndents(12, 0); search.setFont(station::sans(13.5f, station::regular));
    search.onTextChange = [this] { refilter(); };
    addAndMakeVisible(search);
    catBox.addItem("All categories (" + juce::String((int)lib.entries.size()) + ")", 1);
    for (size_t i = 0; i < lib.cats.size(); i++) catBox.addItem(lib.cats[i], (int)i + 2);
    catBox.setSelectedId(1, juce::dontSendNotification);
    catBox.onChange = [this] { refilter(); };
    addAndMakeVisible(catBox);
    list.setRowHeight(30); list.setOutlineThickness(0);
    addAndMakeVisible(list);

    prev.onClick = [this] { proc.stepPreset(-1, &shown); };
    next.onClick = [this] { proc.stepPreset(1, &shown); };
    rnd.onClick = [this] { if (!shown.empty()) proc.loadPreset(shown[(size_t)random.nextInt((int)shown.size())]); };
    prev.setButtonText(juce::String::fromUTF8("\xe2\x97\x80")); next.setButtonText(juce::String::fromUTF8("\xe2\x96\xb6"));
    for (auto* b : { &prev, &next, &rnd }) addAndMakeVisible(b);

    nameLabel.setFont(station::sans(26.0f, station::medium)); nameLabel.setColour(juce::Label::textColourId, station::ink); nameLabel.setMinimumHorizontalScale(0.6f); addAndMakeVisible(nameLabel);
    catLabel.setFont(station::mono(11.0f)); catLabel.setColour(juce::Label::textColourId, station::dim); addAndMakeVisible(catLabel);
    statusLabel.setFont(station::mono(11.0f)); statusLabel.setColour(juce::Label::textColourId, station::dim); statusLabel.setJustificationType(juce::Justification::centredRight); addAndMakeVisible(statusLabel);
    sectionLcd.setFont(station::mono(11.0f)); sectionLcd.setColour(juce::Label::textColourId, station::dim); sectionLcd.setJustificationType(juce::Justification::centredRight); addAndMakeVisible(sectionLcd);
    for (auto* l : { &nameLabel, &catLabel, &statusLabel, &sectionLcd }) l->setInterceptsMouseClicks(false, false);


    addAndMakeVisible(orb);

    const char* mid[4] = { "mac1", "mac2", "mac3", "mac4" };
    for (int i = 0; i < 4; i++) { macros[i] = std::make_unique<Knob>(p.apvts, mid[i], "Macro " + juce::String(i + 1)); addAndMakeVisible(*macros[i]); }
    const char* pid[6] = { "cutoff", "reso", "attack", "release", "reverb", "delay" }; const char* pn[6] = { "Cutoff", "Resonance", "Attack", "Release", "Reverb", "Delay" };
    for (int i = 0; i < 6; i++) { perf[i] = std::make_unique<Knob>(p.apvts, pid[i], pn[i]); addAndMakeVisible(*perf[i]); }
    const char* lid[4] = { "lvla", "lvlb", "lvlc", "lvld" };
    for (int i = 0; i < 4; i++) { layers[i] = std::make_unique<Knob>(p.apvts, lid[i], "Layer"); addAndMakeVisible(*layers[i]); }
    volume = std::make_unique<Knob>(p.apvts, "volume", "Volume"); addAndMakeVisible(*volume);

    keys.setColour(juce::MidiKeyboardComponent::whiteNoteColourId, juce::Colours::white);
    keys.setColour(juce::MidiKeyboardComponent::blackNoteColourId, station::graphite);
    keys.setColour(juce::MidiKeyboardComponent::keySeparatorLineColourId, station::hairline);
    keys.setColour(juce::MidiKeyboardComponent::shadowColourId, juce::Colours::transparentBlack);
    keys.setColour(juce::MidiKeyboardComponent::textLabelColourId, station::faint);
    keys.setColour(juce::MidiKeyboardComponent::upDownButtonBackgroundColourId, station::module);
    keys.setColour(juce::MidiKeyboardComponent::upDownButtonArrowColourId, station::dim);
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
    nameLabel.setText(inf.name + (proc.isEdited() ? "  \xe2\x80\xa2 edited" : ""), juce::dontSendNotification);
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
    if (cur) {
        g.setColour(station::well); g.fillRoundedRectangle(2.0f, 1.0f, w - 4.0f, h - 2.0f, 7);
        g.setColour(ui::accent); g.fillRoundedRectangle(6.0f, h * 0.5f - 7, 3.0f, 14.0f, 1.5f);
    }
    g.setFont(station::sans(13.5f, cur ? station::medium : station::regular)); g.setColour(station::ink);
    g.drawText(juce::String(e.name), 16, 0, w - 124, h, juce::Justification::centredLeft, true);
    g.setFont(station::mono(10.0f)); g.setColour(cur ? station::dim : station::faint);
    g.drawText(juce::String(e.cat).toUpperCase(), w - 116, 0, 106, h, juce::Justification::centredRight, true);
}
void AetherEditor::listBoxItemClicked(int row, const juce::MouseEvent&) { if (row >= 0 && row < (int)shown.size()) proc.loadPreset(shown[(size_t)row]); }

void AetherEditor::paint(juce::Graphics& g) {
    g.fillAll(station::hull);
    // header module: wordmark, display, preset buttons
    station::drawModule(g, header.toFloat());
    {
        const auto br = header.reduced(22, 0).withWidth(230).toFloat();
        station::drawWordmark(g, br.withTrimmedBottom(14), "Aether", {}, ui::accent);
        g.setFont(station::mono(10.0f)); g.setColour(station::faint);
        g.drawText("LAYERED SYNTHESIS ENGINE", br.withTrimmedLeft(22).withTrimmedTop(br.getHeight() * 0.5f + 6).withHeight(14), juce::Justification::centredLeft, false);
    }
    station::drawWell(g, lcdArea.toFloat(), 10);
    // library, play page modules, keyboard bed
    station::drawModule(g, browserPlate.toFloat(), "Library", {}, juce::String(Library::get().entries.size()) + " presets");
    if (page == 0) {
        static const char* titles[4] = { "Macros", "Shape", "Layers & Output", "Orb" };
        static const char* codes[4] = { "M-01", "S-02", "L-03", "O-04" };
        for (int i = 0; i < 4; i++) station::drawModule(g, playPlates[i].toFloat(), titles[i], {}, codes[i]);
    }
    station::drawModule(g, keys.getBounds().toFloat().expanded(10, 10));
}
void AetherEditor::resized() {
    auto r = getLocalBounds().reduced(20, 16);
    // header
    header = r.removeFromTop(80);
    auto head = header.reduced(22, 14);
    head.removeFromLeft(240);
    {
        auto btns = head.removeFromRight(196).withSizeKeepingCentre(196, 40);
        prev.setBounds(btns.removeFromLeft(48)); btns.removeFromLeft(6); next.setBounds(btns.removeFromLeft(48)); btns.removeFromLeft(6); rnd.setBounds(btns);
        head.removeFromRight(14);
        lcdArea = head.reduced(4, 2);
        auto l = lcdArea.reduced(16, 6);
        auto top = l.removeFromTop(16);
        catLabel.setBounds(top.removeFromLeft(top.getWidth() / 2));
        statusLabel.setBounds(top.removeFromRight(90)); sectionLcd.setBounds(top);
        nameLabel.setBounds(l);
    }
    r.removeFromTop(14);
    keys.setBounds(r.removeFromBottom(86).reduced(10, 0).withTrimmedBottom(10)); keys.setKeyWidth(keys.getWidth() / 50.0f);
    r.removeFromBottom(24);
    // library
    browserPlate = r.removeFromLeft(juce::jmax(290, r.getWidth() * 28 / 100));
    {
        auto left = browserPlate.reduced(14).withTrimmedTop(station::kModuleTitle - 10);
        catBox.setBounds(left.removeFromTop(32)); left.removeFromTop(8);
        search.setBounds(left.removeFromTop(32)); left.removeFromTop(10);
        list.setBounds(left);
    }
    r.removeFromLeft(14);
    {
        auto tabs = r.removeFromTop(36);
        const int tw = juce::jmin(96, (tabs.getWidth() - 6 * kPages) / (kPages + 1));
        for (auto& t : pageTabs) { t.setBounds(tabs.removeFromLeft(tw)); tabs.removeFromLeft(6); }
    }
    r.removeFromTop(14);
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
    knobRow(playPlates[0].reduced(12, 10).withTrimmedTop(station::kModuleTitle - 6), macros, 4);
    knobRow(playPlates[1].reduced(12, 10).withTrimmedTop(station::kModuleTitle - 6), perf, 6);
    {
        auto lr = playPlates[2].reduced(12, 10).withTrimmedTop(station::kModuleTitle - 6); const int w = lr.getWidth() / 6;
        for (int i = 0; i < 4; i++) layers[i]->setBounds(lr.removeFromLeft(w).reduced(4, 0));
        lr.removeFromLeft(w / 2); volume->setBounds(lr.removeFromLeft(w).reduced(4, 0));
    }
    orb.setBounds(playPlates[3].reduced(14).withTrimmedTop(station::kModuleTitle - 10));
}
