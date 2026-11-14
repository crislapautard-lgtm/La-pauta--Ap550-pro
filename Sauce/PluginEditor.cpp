#include "PluginEditor.h"
#include "EditorLayout.h"
#include "EditorDraw.h"

using namespace lpui;

//==============================================================================
AP550Editor::AP550Editor (AP550Processor& p)
    : juce::AudioProcessorEditor (&p), proc (p)
{
    setResizable (true, true);
    setResizeLimits ((int) (kW * 0.6), (int) (kH * 0.6), kW * 2, kH * 2);
    getConstrainer()->setFixedAspectRatio ((double) kW / (double) kH);
    setSize (kW, kH);

    buildBackground();
    startTimerHz (30);
}

AP550Editor::~AP550Editor()
{
    stopTimer();
}

//==============================================================================
//  Fondo estático (placa, medidores, textos fijos). Se dibuja a 2x una sola vez.
//==============================================================================
void AP550Editor::buildBackground()
{
    bg = juce::Image (juce::Image::ARGB, kW * 2, kH * 2, true);
    juce::Graphics g (bg);
    g.addTransform (juce::AffineTransform::scale (2.0f));

    g.fillAll (juce::Colour (0xff101211));

    // barra superior
    g.setGradientFill (juce::ColourGradient::vertical (juce::Colour (0xff4a4d4d), 0.f, juce::Colour (0xff262828), 26.f));
    g.fillRect (0.f, 0.f, (float) kW, 26.f);

    // zona de medidores
    g.setGradientFill (juce::ColourGradient::vertical (juce::Colour (0xff1d1f1f), 50.f, juce::Colour (0xff0d0f0e), 193.f));
    g.fillRect (0.f, 50.f, (float) kW, 143.f);

    // placa izquierda (negra)
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff272a29), 0.f, 193.f, juce::Colour (0xff111312), 219.f, (float) kH, false));
    g.fillRect (0.f, 193.f, 219.f, (float) (kH - 193));

    // placa derecha (gris plateado oscuro)
    g.setGradientFill (juce::ColourGradient::vertical (cSilverTop, 193.f, cSilverBot, (float) kH));
    g.fillRect (219.f, 193.f, 219.f, (float) (kH - 193));

    // textura cepillada
    for (int y = 194; y < kH; y += 2)
    {
        g.setColour (juce::Colour ((y / 2) % 3 == 0 ? 0x16ffffffu : 0x0c000000u));
        g.drawHorizontalLine (y, 219.f, (float) kW);
    }

    // separación entre placas
    g.setColour (juce::Colour (0xff070808));
    g.fillRect (217.f, 193.f, 3.f, (float) (kH - 193));
    g.setColour (juce::Colours::white.withAlpha (0.18f));
    g.fillRect (220.f, 193.f, 1.f, (float) (kH - 193));

    drawMeterFace (g, kLeftFace);
    drawMeterFace (g, kRightFace);

    // puntos del knob de salida
    for (int i = 0; i < 19; ++i)
    {
        const float a = juce::degreesToRadians (-135.f + 270.f * (float) i / 18.f);
        const auto p = arcPt (kOutC, 49.f, a);
        g.setColour (juce::Colour (0xff2d63b0));
        g.fillEllipse (juce::Rectangle<float> (3.4f, 3.4f).withCentre (p));
        g.setColour (juce::Colours::white.withAlpha (0.25f));
        g.fillEllipse (juce::Rectangle<float> (1.4f, 1.4f).withCentre (p + juce::Point<float> (-0.5f, -0.5f)));
    }

    // rótulos de la placa derecha
    drawEmboss (g, "POL",    { 286.f, 216.f }, 12.f);
    drawEmboss (g, "ANALOG", { 365.f, 216.f }, 12.f);
    drawEmboss (g, "ON",     { 365.f, 235.f }, 11.f);
    drawEmboss (g, "OFF",    { 365.f, 305.f }, 11.f);
    drawEmboss (g, "OUTPUT", { 327.f, 340.f }, 12.f);
    drawEmboss (g, "-18",    { 292.f, 460.f }, 13.f);
    drawEmboss (g, "+18",    { 356.f, 460.f }, 13.f);

    // logo "La Pauta": cinco líneas de pauta y una nota
    for (int i = 0; i < 5; ++i)
    {
        const float y = 560.f + (float) i * 7.f;
        g.setColour (juce::Colours::white.withAlpha (0.35f));
        g.drawLine (286.f, y + 1.f, 368.f, y + 1.f, 1.6f);
        g.setColour (cInk);
        g.drawLine (286.f, y, 368.f, y, 1.6f);
    }
    g.setColour (cInk);
    g.fillEllipse (juce::Rectangle<float> (13.f, 10.f).withCentre ({ 322.f, 581.f }));
    g.drawLine (327.f, 579.f, 327.f, 552.f, 2.2f);

    drawEmboss (g, "LA PAUTA",       { 327.f, 616.f }, 17.f, true, 160.f);
    drawEmboss (g, "AP550",          { 327.f, 667.f }, 38.f, true, 200.f);
    drawEmboss (g, "PRO  EQUALIZER", { 327.f, 700.f }, 15.f, true, 200.f);

    // contorno general
    g.setColour (juce::Colour (0xff3a3d3d));
    g.drawRect (0.5f, 0.5f, (float) kW - 1.f, (float) kH - 1.f, 1.f);
}

//==============================================================================
int AP550Editor::getIdx (const char* id) const
{
    return (int) std::lround (proc.apvts.getRawParameterValue (id)->load());
}

bool AP550Editor::getBool (const char* id) const
{
    return proc.apvts.getRawParameterValue (id)->load() > 0.5f;
}

float AP550Editor::getVal (const char* id) const
{
    return proc.apvts.getRawParameterValue (id)->load();
}

void AP550Editor::setIdx (const char* id, int index)
{
    if (auto* p = proc.apvts.getParameter (id))
        p->setValueNotifyingHost (p->convertTo0to1 ((float) index));
}

void AP550Editor::setVal (const char* id, float value)
{
    if (auto* p = proc.apvts.getParameter (id))
        p->setValueNotifyingHost (p->convertTo0to1 (value));
}

void AP550Editor::toggleBool (const char* id)
{
    if (auto* p = proc.apvts.getParameter (id))
    {
        p->beginChangeGesture();
        p->setValueNotifyingHost (getBool (id) ? 0.0f : 1.0f);
        p->endChangeGesture();
    }
}

//==============================================================================
juce::String AP550Editor::stateKey() const
{
    juce::String k;
    for (auto* id : { ap::kLfGain, ap::kLfFreq, ap::kLfBell, ap::kMfGain, ap::kMfFreq,
                      ap::kHfGain, ap::kHfFreq, ap::kHfBell, ap::kEqIn, ap::kFilter,
                      ap::kPolarity, ap::kAnalog, ap::kOutput })
        k << juce::String (getVal (id), 1) << ',';

    k << proc.getPresetName() << ',' << proc.getActiveSlot() << ',' << pressedTool << ','
      << (int) proc.undo.canUndo() << (int) proc.undo.canRedo() << ','
      << juce::String (needleL, 1) << ',' << juce::String (needleR, 1);
    return k;
}

void AP550Editor::timerCallback()
{
    const float l = juce::jmax (-60.f, proc.vuL.load());
    const float r = juce::jmax (-60.f, proc.vuR.load());
    needleL += (l - needleL) * 0.35f;
    needleR += (r - needleR) * 0.35f;

    const auto key = stateKey();
    if (key != lastKey)
    {
        lastKey = key;
        repaint();
    }
}
