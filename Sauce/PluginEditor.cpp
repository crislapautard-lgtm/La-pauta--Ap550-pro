#include "PluginEditor.h"
#include <cmath>
static juce::Rectangle<float> scaled (juce::Rectangle<float> r, float s) { return { r.getX()*s, r.getY()*s, r.getWidth()*s, r.getHeight()*s }; }
AP550Editor::AP550Editor (AP550Processor& p) : juce::AudioProcessorEditor (&p), proc (p)
{
    setResizable (true, true);
    setResizeLimits ((int)(kW*0.6), (int)(kH*0.6), kW*2, kH*2);
    getConstrainer()->setFixedAspectRatio ((double)kW/(double)kH);
    setSize (kW, kH);
    buildBackground();
    startTimerHz (30);
}
AP550Editor::~AP550Editor() { stopTimer(); }
int AP550Editor::getIdx (const char* id) const { return (int) std::lround (proc.apvts.getRawParameterValue (id)->load()); }
bool AP550Editor::getBool (const char* id) const { return proc.apvts.getRawParameterValue (id)->load() > 0.5f; }
float AP550Editor::getVal (const char* id) const { return proc.apvts.getRawParameterValue (id)->load(); }
void AP550Editor::setIdx (const char* id, int idx) { if (auto* par = proc.apvts.getParameter (id)) par->setValueNotifyingHost (par->convertTo0to1 ((float) idx)); }
void AP550Editor::setVal (const char* id, float v) { if (auto* par = proc.apvts.getParameter (id)) par->setValueNotifyingHost (par->convertTo0to1 (v)); }
void AP550Editor::toggleBool (const char* id) { if (auto* par = proc.apvts.getParameter (id)) par->setValueNotifyingHost (par->getValue() > 0.5f ? 0.0f : 1.0f); }
juce::String AP550Editor::stateKey() const
{
    juce::String s;
    s << getIdx (ap::kLfGain) << "," << getIdx (ap::kLfFreq) << "," << (getBool(ap::kLfBell)?1:0) << ","
      << getIdx (ap::kMfGain) << "," << getIdx (ap::kMfFreq) << ","
      << getIdx (ap::kHfGain) << "," << getIdx (ap::kHfFreq) << "," << (getBool(ap::kHfBell)?1:0) << ","
      << (getBool(ap::kEqIn)?1:0) << "," << (getBool(ap::kFilter)?1:0) << "," << (getBool(ap::kPolarity)?1:0) << ","
      << (getBool(ap::kAnalog)?1:0) << "," << getVal (ap::kOutput);
    return s;
}
void AP550Editor::buildBackground()
{
    bg = juce::Image (juce::Image::ARGB, kW, kH, true);
    juce::Graphics g (bg);
    g.fillAll (juce::Colour (0xff1a1a1a));
    g.setColour (juce::Colour (0xff2a2a2a));
    g.fillRoundedRectangle (10, 10, kW-20, kH-20, 18);
    g.setColour (juce::Colours::white);
    g.setFont (juce::Font (22.0f, juce::Font::bold));
    g.drawText ("LA PAUTA AP550 PRO", 20, 18, kW-40, 30, juce::Justification::centred);
    g.setFont (juce::Font (13.0f));
    g.setColour (juce::Colour (0xffaaaaaa));
    g.drawText ("API 550 STYLE EQ", 20, 48, kW-40, 20, juce::Justification::centred);
    auto knob = [&] (int x, int y, const char* label)
    {
        g.setColour (juce::Colour (0xff111111));
        g.fillEllipse ((float)x, (float)y, 72, 72);
        g.setColour (juce::Colour (0xff555555));
        g.drawEllipse ((float)x, (float)y, 72, 72, 2.0f);
        g.setColour (juce::Colours::white);
        g.setFont (juce::Font (11.0f, juce::Font::bold));
        g.drawText (label, x-10, y+76, 92, 16, juce::Justification::centred);
    };
    knob (28, 110, "LF GAIN"); knob (128, 110, "LF FREQ"); knob (228, 110, "MF GAIN"); knob (328, 110, "MF FREQ");
    knob (28, 240, "HF GAIN"); knob (128, 240, "HF FREQ"); knob (228, 240, "OUTPUT");
    g.setColour (juce::Colours::white);
    g.setFont (juce::Font (12.0f, juce::Font::bold));
    g.drawText ("PRESET: " + proc.getPresetName(), 20, 380, kW-40, 22, juce::Justification::centred);
    g.drawText ("A/B SLOT: " + juce::String (proc.getActiveSlot() == 0 ? "A" : "B"), 20, 405, kW-40, 22, juce::Justification::centred);
    auto btn = [&] (int x, int y, int w, const char* t)
    {
        g.setColour (juce::Colour (0xff333333));
        g.fillRoundedRectangle ((float)x, (float)y, (float)w, 30, 6);
        g.setColour (juce::Colours::white);
        g.setFont (juce::Font (12.0f, juce::Font::bold));
        g.drawText (t, x, y, w, 30, juce::Justification::centred);
    };
    btn (20, 440, 90, "PREV"); btn (120, 440, 90, "NEXT"); btn (220, 440, 90, "A/B"); btn (320, 440, 90, "COPY");
    btn (20, 480, 90, "SAVE"); btn (120, 480, 90, "LOAD"); btn (220, 480, 90, "EQ IN"); btn (320, 480, 90, "FILTER");
    btn (20, 520, 90, "POLARITY"); btn (120, 520, 90, "ANALOG"); btn (220, 520, 90, "LF BELL"); btn (320, 520, 90, "HF BELL");
    g.setColour (juce::Colour (0xff0a0a0a));
    g.fillRoundedRectangle (20, 580, kW-40, 120, 8);
    g.setColour (juce::Colour (0xffccaa00));
    g.setFont (juce::Font (11.0f, juce::Font::bold));
    g.drawText ("VU  L", 30, 585, 60, 16, juce::Justification::left);
    g.drawText ("VU  R", 30, 640, 60, 16, juce::Justification::left);
}
void AP550Editor::drawMeterNeedle (juce::Graphics& g, juce::Rectangle<float> face, float db) const
{
    float t = juce::jlimit (0.0f, 1.0f, (db + 60.0f) / 63.0f);
    float ang = -0.9f + t * 1.8f;
    auto c = face.getCentre();
    float len = face.getWidth() * 0.42f;
    juce::Line<float> line (c, c + juce::Point<float> (std::sin (ang) * len, -std::cos (ang) * len));
    g.setColour (juce::Colours::red);
    g.drawLine (line, 2.0f);
}
void AP550Editor::drawToolButton (juce::Graphics&, juce::Rectangle<float>, int, bool) const {}
void AP550Editor::paint (juce::Graphics& g)
{
    float s = scale();
    if (bg.isValid()) g.drawImage (bg, 0, 0, getWidth(), getHeight(), 0, 0, kW, kH);
    else g.fillAll (juce::Colours::black);
    auto drawKnobVal = [&] (int x, int y, float norm, juce::String txt)
    {
        auto r = scaled ({ (float)x, (float)y, 72, 72 }, s);
        float ang = -2.35f + norm * 4.7f;
        auto c = r.getCentre();
        juce::Line<float> l (c, c + juce::Point<float> (std::sin (ang) * 28 * s, -std::cos (ang) * 28 * s));
        g.setColour (juce::Colours::white);
        g.drawLine (l, 2.5f);
        g.setColour (juce::Colours::white);
        g.setFont (juce::Font (11.0f * s, juce::Font::bold));
        g.drawText (txt, r.getX()-10*s, r.getBottom()+2*s, 92*s, 14*s, juce::Justification::centred);
    };
    drawKnobVal (28, 110, getIdx (ap::kLfGain) / 10.0f, juce::String (getIdx (ap::kLfGain)));
    drawKnobVal (128, 110, getIdx (ap::kLfFreq) / 4.0f, juce::String (getIdx (ap::kLfFreq)));
    drawKnobVal (228, 110, getIdx (ap::kMfGain) / 10.0f, juce::String (getIdx (ap::kMfGain)));
    drawKnobVal (328, 110, getIdx (ap::kMfFreq) / 4.0f, juce::String (getIdx (ap::kMfFreq)));
    drawKnobVal (28, 240, getIdx (ap::kHfGain) / 10.0f, juce::String (getIdx (ap::kHfGain)));
    drawKnobVal (128, 240, getIdx (ap::kHfFreq) / 4.0f, juce::String (getIdx (ap::kHfFreq)));
    drawKnobVal (228, 240, (getVal (ap::kOutput) + 18.0f) / 36.0f, juce::String (getVal (ap::kOutput), 1));
    g.setColour (juce::Colours::white);
    g.setFont (juce::Font (12.0f * s, juce::Font::bold));
    g.drawText ("PRESET: " + proc.getPresetName(), 20*s, 380*s, (kW-40)*s, 22*s, juce::Justification::centred);
    g.drawText ("A/B SLOT: " + juce::String (proc.getActiveSlot() == 0 ? "A" : "B"), 20*s, 405*s, (kW-40)*s, 22*s, juce::Justification::centred);
    drawMeterNeedle (g, scaled ({ 30, 585, 360, 45 }, s), needleL);
    drawMeterNeedle (g, scaled ({ 30, 640, 360, 45 }, s), needleR);
    if (getBool (ap::kEqIn)) { g.setColour (juce::Colours::lime); g.fillEllipse (scaled ({ 305, 480, 10, 10 }, s)); }
    if (getBool (ap::kFilter)) { g.setColour (juce::Colours::lime); g.fillEllipse (scaled ({ 405, 480, 10, 10 }, s)); }
}
void AP550Editor::timerCallback()
{
    float targetL = proc.vuL.load();
    float targetR = proc.vuR.load();
    needleL += (targetL - needleL) * 0.3f;
    needleR += (targetR - needleR) * 0.3f;
    juce::String now = stateKey();
    if (now != lastKey) { lastKey = now; repaint(); return; }
    repaint (scaled ({ 20, 580, 398, 120 }, scale()).toNearestInt());
}
void AP550Editor::mouseDown (const juce::MouseEvent& e)
{
    float s = scale();
    auto in = [&] (int x, int y, int w, int h) { return scaled ({ (float)x, (float)y, (float)w, (float)h }, s).contains (e.position); };
    auto knobHit = [&] (int x, int y) { return scaled ({ (float)x, (float)y, 72, 72 }, s).contains (e.position); };
    if (knobHit (28, 110)) { dragKind = DragKind::gain; dragBand = 0; dragParam = proc.apvts.getParameter (ap::kLfGain); }
    else if (knobHit (128, 110)) { dragKind = DragKind::gain; dragBand = 1; dragParam = proc.apvts.getParameter (ap::kLfFreq); }
    else if (knobHit (228, 110)) { dragKind = DragKind::gain; dragBand = 2; dragParam = proc.apvts.getParameter (ap::kMfGain); }
    else if (knobHit (328, 110)) { dragKind = DragKind::gain; dragBand = 3; dragParam = proc.apvts.getParameter (ap::kMfFreq); }
    else if (knobHit (28, 240)) { dragKind = DragKind::gain; dragBand = 4; dragParam = proc.apvts.getParameter (ap::kHfGain); }
    else if (knobHit (128, 240)) { dragKind = DragKind::gain; dragBand = 5; dragParam = proc.apvts.getParameter (ap::kHfFreq); }
    else if (knobHit (228, 240)) { dragKind = DragKind::output; dragBand = 6; dragParam = proc.apvts.getParameter (ap::kOutput); }
    else { dragKind = DragKind::none; dragParam = nullptr; }
    if (dragParam != nullptr)
    {
        dragStartY = e.position.y;
        dragStartVal = dragParam->getValue();
        if (dragKind == DragKind::gain) dragLastIdx = (int) std::lround (dragParam->convertFrom0to1 (dragStartVal));
        else dragLastIdx = 0;
    }
    if (in (20, 440, 90, 30)) { proc.stepPreset (-1); repaint(); return; }
    if (in (120, 440, 90, 30)) { proc.stepPreset (1); repaint(); return; }
    if (in (220, 440, 90, 30)) { proc.toggleSlot(); repaint(); return; }
    if (in (320, 440, 90, 30)) { proc.copyToOtherSlot(); return; }
    if (in (20, 480, 90, 30)) { showSaveDialog(); return; }
    if (in (120, 480, 90, 30)) { showLoadMenu(); return; }
    if (in (220, 480, 90, 30)) { toggleBool (ap::kEqIn); repaint(); return; }
    if (in (320, 480, 90, 30)) { toggleBool (ap::kFilter); repaint(); return; }
    if (in (20, 520, 90, 30)) { toggleBool (ap::kPolarity); repaint(); return; }
    if (in (120, 520, 90, 30)) { toggleBool (ap::kAnalog); repaint(); return; }
    if (in (220, 520, 90, 30)) { toggleBool (ap::kLfBell); repaint(); return; }
    if (in (320, 520, 90, 30)) { toggleBool (ap::kHfBell); repaint(); return; }
}
void AP550Editor::mouseDrag (const juce::MouseEvent& e)
{
    if (dragParam == nullptr) return;
    float dy = dragStartY - e.position.y;
    if (dragKind == DragKind::gain)
    {
        int steps = (int) (dy / 12.0f);
        int nv = juce::jlimit (0, 10, dragLastIdx + steps);
        if (dragBand == 1 || dragBand == 3 || dragBand == 5) nv = juce::jlimit (0, 4, dragLastIdx + steps);
        setIdx (dragParam->getParameterID().getParamID(), nv);
    }
    else if (dragKind == DragKind::output)
    {
        float nv = juce::jlimit (-18.0f, 18.0f, dragParam->convertFrom0to1 (dragStartVal) + dy * 0.08f);
        setVal (ap::kOutput, nv);
    }
    repaint();
}
void AP550Editor::mouseUp (const juce::MouseEvent&) { dragKind = DragKind::none; dragParam = nullptr; }
void AP550Editor::mouseDoubleClick (const juce::MouseEvent& e)
{
    float s = scale();
    if (scaled ({ 28, 110, 72, 72 }, s).contains (e.position)) setIdx (ap::kLfGain, 5);
    else if (scaled ({ 228, 110, 72, 72 }, s).contains (e.position)) setIdx (ap::kMfGain, 5);
    else if (scaled ({ 28, 240, 72, 72 }, s).contains (e.position)) setIdx (ap::kHfGain, 5);
    else if (scaled ({ 228, 240, 72, 72 }, s).contains (e.position)) setVal (ap::kOutput, 0.0f);
    repaint();
}
void AP550Editor::mouseWheelMove (const juce::MouseEvent& e, const juce::MouseWheelDetails& w)
{
    float s = scale();
    auto hit = [&] (int x, int y) { return scaled ({ (float)x, (float)y, 72, 72 }, s).contains (e.position); };
    int dir = w.deltaY > 0 ? 1 : -1;
    if (hit (28, 110)) setIdx (ap::kLfGain, juce::jlimit (0, 10, getIdx (ap::kLfGain) + dir));
    else if (hit (128, 110)) setIdx (ap::kLfFreq, juce::jlimit (0, 4, getIdx (ap::kLfFreq) + dir));
    else if (hit (228, 110)) setIdx (ap::kMfGain, juce::jlimit (0, 10, getIdx (ap::kMfGain) + dir));
    else if (hit (328, 110)) setIdx (ap::kMfFreq, juce::jlimit (0, 4, getIdx (ap::kMfFreq) + dir));
    else if (hit (28, 240)) setIdx (ap::kHfGain, juce::jlimit (0, 10, getIdx (ap::kHfGain) + dir));
    else if (hit (128, 240)) setIdx (ap::kHfFreq, juce::jlimit (0, 4, getIdx (ap::kHfFreq) + dir));
    else if (hit (228, 240)) setVal (ap::kOutput, juce::jlimit (-18.0f, 18.0f, getVal (ap::kOutput) + dir * 0.5f));
    repaint();
}
void AP550Editor::runTool (int) {}
void AP550Editor::showLoadMenu()
{
    juce::PopupMenu m;
    const auto& fac = ap::factoryPresets();
    for (int i = 0; i < (int) fac.size(); ++i) m.addItem (100 + i, fac[(size_t) i].name);
    auto files = proc.listUserPresets();
    for (int i = 0; i < files.size(); ++i) m.addItem (200 + i, files[i].getFileNameWithoutExtension());
    m.showMenuAsync (juce::PopupMenu::Options(), [this, files] (int r)
    {
        if (r >= 100 && r < 200) proc.loadFactoryPreset (r - 100);
        else if (r >= 200 && r < 200 + files.size()) proc.loadUserPreset (files[r - 200]);
        repaint();
    });
}
void AP550Editor::showSaveDialog()
{
    auto* alert = new juce::AlertWindow ("Save Preset", "Nombre del preset", juce::MessageBoxIconType::QuestionIcon);
    alert->addTextEditor ("name", proc.getPresetName());
    alert->addButton ("Save", 1, juce::KeyPress (juce::KeyPress::returnKey));
    alert->addButton ("Cancel", 0, juce::KeyPress (juce::KeyPress::escapeKey));
    alert->enterModalState (true, juce::ModalCallbackFunction::create ([this, alert] (int r)
    {
        if (r == 1) proc.saveUserPreset (alert->getTextEditorContents ("name"));
        delete alert;
        repaint();
    }));
}
