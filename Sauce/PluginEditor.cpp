#include "PluginEditor.h"
#include <cmath>

// ============================================================
// Helpers
// ============================================================
static juce::Rectangle<float> scaled (juce::Rectangle<float> r, float s)
{
    return { r.getX() * s, r.getY() * s, r.getWidth() * s, r.getHeight() * s };
}

static juce::Font fBold (float size) { return juce::Font (juce::FontOptions (size, juce::Font::bold)); }
static juce::Font fPlain (float size) { return juce::Font (juce::FontOptions (size)); }

// Colores API 550
static const juce::Colour kPanelDark  = juce::Colour (0xff1c1c1e);
static const juce::Colour kPanelLight = juce::Colour (0xff2e2e30);
static const juce::Colour kPanelEdge  = juce::Colour (0xff0a0a0a);
static const juce::Colour kCream      = juce::Colour (0xfff2e6c8);
static const juce::Colour kOrange     = juce::Colour (0xffd97a1f);
static const juce::Colour kYellow     = juce::Colour (0xfff2c200);
static const juce::Colour kRed       = juce::Colour (0xffd32f2f);
static const juce::Colour kGreen     = juce::Colour (0xff4caf50);
static const juce::Colour kGreyText   = juce::Colour (0xffb8b8b8);
static const juce::Colour kKnobFace   = juce::Colour (0xff232323);
static const juce::Colour kKnobEdge   = juce::Colour (0xff555555);

// ============================================================
// Constructor / Destructor
// ============================================================
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

// ============================================================
// Acceso a parametros
// ============================================================
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
void AP550Editor::setIdx (const char* id, int idx)
{
    if (auto* par = proc.apvts.getParameter (id))
        par->setValueNotifyingHost (par->convertTo0to1 ((float) idx));
}
void AP550Editor::setVal (const char* id, float v)
{
    if (auto* par = proc.apvts.getParameter (id))
        par->setValueNotifyingHost (par->convertTo0to1 (v));
}
void AP550Editor::toggleBool (const char* id)
{
    if (auto* par = proc.apvts.getParameter (id))
        par->setValueNotifyingHost (par->getValue() > 0.5f ? 0.0f : 1.0f);
}
juce::String AP550Editor::stateKey() const
{
    juce::String s;
    s << getIdx (ap::kLfGain) << "," << getIdx (ap::kLfFreq) << "," << (getBool (ap::kLfBell) ? 1 : 0) << ","
      << getIdx (ap::kMfGain) << "," << getIdx (ap::kMfFreq) << ","
      << getIdx (ap::kHfGain) << "," << getIdx (ap::kHfFreq) << "," << (getBool (ap::kHfBell) ? 1 : 0) << ","
      << (getBool (ap::kEqIn) ? 1 : 0) << "," << (getBool (ap::kFilter) ? 1 : 0) << "," << (getBool (ap::kPolarity) ? 1 : 0) << ","
      << (getBool (ap::kAnalog) ? 1 : 0) << "," << getVal (ap::kOutput);
    return s;
}

// ============================================================
// Fondo - dibujado una sola vez
// ============================================================
void AP550Editor::buildBackground()
{
    bg = juce::Image (juce::Image::ARGB, kW, kH, true);
    juce::Graphics g (bg);

    // Fondo exterior
    g.fillAll (kPanelDark);

    // Panel principal
    g.setColour (kPanelLight);
    g.fillRoundedRectangle (8, 8, kW - 16, kH - 16, 16);
    g.setColour (kPanelEdge);
    g.drawRoundedRectangle (8, 8, kW - 16, kH - 16, 16, 1.5f);

    // Cabecera
    g.setColour (kCream);
    g.setFont (fBold (26.0f));
    g.drawText ("LA PAUTA", 20, 16, kW - 40, 28, juce::Justification::centred);
    g.setColour (kOrange);
    g.setFont (fBold (24.0f));
    g.drawText ("AP550 PRO", 20, 44, kW - 40, 26, juce::Justification::centred);
    g.setColour (kGreyText);
    g.setFont (fPlain (11.0f));
    g.drawText ("API 550 STYLE  •  3-BAND PARAMETRIC EQ  •  LA PAUTA STUDIO", 20, 72, kW - 40, 14, juce::Justification::centred);

    // Linea separadora
    g.setColour (juce::Colour (0xff444444));
    g.drawLine (20, 94, kW - 20, 94, 1.0f);

    // --- Helper para dibujar un knob completo ---
    auto drawKnobBase = [&] (int x, int y, const char* title, const char* sub, const char* marks[5])
    {
        // Titulo
        g.setColour (kCream);
        g.setFont (fBold (12.0f));
        g.drawText (title, x - 12, y - 22, 96, 16, juce::Justification::centred);
        g.setColour (kGreyText);
        g.setFont (fPlain (9.0f));
        g.drawText (sub, x - 12, y - 8, 96, 12, juce::Justification::centred);

        // Cuerpo del knob
        g.setColour (kKnobFace);
        g.fillEllipse ((float) x, (float) y, 72, 72);
        g.setColour (kKnobEdge);
        g.drawEllipse ((float) x, (float) y, 72, 72, 2.0f);

        // Sombra interior
        g.setColour (juce::Colour (0x22000000));
        g.fillEllipse ((float) x + 4, (float) y + 4, 64, 64);

        // Marcas de tick alrededor
        g.setColour (kGreyText);
        for (int i = 0; i < 11; ++i)
        {
            float ang = -2.35f + (float) i * 0.47f;
            auto c = juce::Point<float> (x + 36, y + 36);
            auto p1 = c + juce::Point<float> (std::sin (ang) * 40, -std::cos (ang) * 40);
            auto p2 = c + juce::Point<float> (std::sin (ang) * 46, -std::cos (ang) * 46);
            g.drawLine (juce::Line<float> (p1, p2), i == 5 ? 2.0f : 1.0f);
        }

        // Etiquetas de valores (5 posiciones)
        if (marks != nullptr)
        {
            g.setColour (kGreyText);
            g.setFont (fPlain (8.0f));
            const int px[5] = { x - 18, x - 6, x + 22, x + 52, x + 64 };
            const int py[5] = { y + 72, y + 78, y + 80, y + 78, y + 72 };
            for (int i = 0; i < 5; ++i)
                g.drawText (marks[i], px[i], py[i], 30, 10, juce::Justification::centred);
        }
    };

    const char* gainMarks[5] = { "-12", "-6", "0", "+6", "+12" };
    const char* lfMarks[5]   = { "50", "100", "200", "300", "400" };
    const char* mfMarks[5]   = { "0.4", "0.8", "1.5", "3", "5k" };
    const char* hfMarks[5]   = { "5k", "7.5k", "10k", "12.5k", "15k" };

    // Fila 1
    drawKnobBase (28, 128, "LOW", "GAIN / FREQ", nullptr);
    drawKnobBase (128, 128, "LOW", "FREQ", nullptr);
    drawKnobBase (228, 128, "MID", "GAIN", nullptr);
    drawKnobBase (328, 128, "MID", "FREQ", nullptr);
    // Fila 2
    drawKnobBase (28, 268, "HIGH", "GAIN", nullptr);
    drawKnobBase (128, 268, "HIGH", "FREQ", nullptr);
    drawKnobBase (228, 268, "OUTPUT", "LEVEL", nullptr);

    // Etiquetas debajo de cada knob (valores)
    auto labelUnder = [&] (int x, int y, const char* a, const char* b, const char* c, const char* d, const char* e)
    {
        g.setColour (kGreyText);
        g.setFont (fPlain (8.0f));
        g.drawText (a, x - 10, y + 74, 28, 10, juce::Justification::centred);
        g.drawText (b, x + 8, y + 80, 28, 10, juce::Justification::centred);
        g.drawText (c, x + 28, y + 82, 28, 10, juce::Justification::centred);
        g.drawText (d, x + 48, y + 80, 28, 10, juce::Justification::centred);
        g.drawText (e, x + 66, y + 74, 28, 10, juce::Justification::centred);
    };
    labelUnder (28, 128, "-12", "-6", "0", "+6", "+12");
    labelUnder (128, 128, "50", "100", "200", "300", "400");
    labelUnder (228, 128, "-12", "-6", "0", "+6", "+12");
    labelUnder (328, 128, "0.4", "0.8", "1.5", "3", "5k");
    labelUnder (28, 268, "-12", "-6", "0", "+6", "+12");
    labelUnder (128, 268, "5k", "7.5", "10", "12.5", "15k");
    labelUnder (228, 268, "-18", "-9", "0", "+9", "+18");

    // --- Seccion Preset ---
    g.setColour (juce::Colour (0xff1a1a1a));
    g.fillRoundedRectangle (20, 408, kW - 40, 52, 8);
    g.setColour (kCream);
    g.setFont (fBold (12.0f));
    g.drawText ("PRESET", 28, 414, 60, 16, juce::Justification::left);
    g.setColour (kYellow);
    g.setFont (fBold (13.0f));
    g.drawText (proc.getPresetName(), 28, 432, 260, 18, juce::Justification::left);
    g.setColour (kGreyText);
    g.setFont (fPlain (11.0f));
    g.drawText ("A/B SLOT: " + juce::String (proc.getActiveSlot() == 0 ? "A" : "B"), 300, 414, 100, 16, juce::Justification::right);
    g.drawText ("FACTORY + USER", 300, 432, 100, 16, juce::Justification::right);

    // --- Botones ---
    auto drawBtn = [&] (int x, int y, int w, const char* t, bool active = false)
    {
        g.setColour (active ? kOrange : juce::Colour (0xff333333));
        g.fillRoundedRectangle ((float) x, (float) y, (float) w, 30, 6);
        g.setColour (active ? juce::Colours::black : juce::Colours::white);
        g.setFont (fBold (11.0f));
        g.drawText (t, x, y, w, 30, juce::Justification::centred);
        g.setColour (juce::Colour (0x88000000));
        g.drawRoundedRectangle ((float) x, (float) y, (float) w, 30, 6, 1.0f);
    };
    drawBtn (20, 472, 90, "PREV");
    drawBtn (120, 472, 90, "NEXT");
    drawBtn (220, 472, 90, "A/B");
    drawBtn (320, 472, 90, "COPY");
    drawBtn (20, 512, 90, "SAVE");
    drawBtn (120, 512, 90, "LOAD");
    drawBtn (220, 512, 90, "EQ IN", getBool (ap::kEqIn));
    drawBtn (320, 512, 90, "FILTER", getBool (ap::kFilter));
    drawBtn (20, 552, 90, "POLARITY", getBool (ap::kPolarity));
    drawBtn (120, 552, 90, "ANALOG", getBool (ap::kAnalog));
    drawBtn (220, 552, 90, "LF BELL", getBool (ap::kLfBell));
    drawBtn (320, 552, 90, "HF BELL", getBool (ap::kHfBell));

    // --- VU Meters ---
    auto drawVuFrame = [&] (int y, const char* label)
    {
        g.setColour (juce::Colour (0xff0a0a0a));
        g.fillRoundedRectangle (20, y, kW - 40, 52, 8);
        g.setColour (juce::Colour (0xffccaa00));
        g.setFont (fBold (10.0f));
        g.drawText (label, 28, y + 4, 50, 14, juce::Justification::left);
        // Escala dB
        g.setColour (kGreyText);
        g.setFont (fPlain (8.0f));
        const char* scaleTxt[6] = { "-60", "-40", "-20", "-10", "-3", "0" };
        for (int i = 0; i < 6; ++i)
        {
            float fx = 70 + i * 56;
            g.drawText (scaleTxt[i], (int) fx, y + 32, 30, 10, juce::Justification::centred);
            g.drawLine (fx + 15, y + 28, fx + 15, y + 32, 1.0f);
        }
        // Zona roja
        g.setColour (juce::Colour (0x44ff0000));
        g.fillRect (20 + 330, y + 18, 48, 14);
    };
    drawVuFrame (610, "VU  L");
    drawVuFrame (672, "VU  R");

    // Footer
    g.setColour (juce::Colour (0xff666666));
    g.setFont (fPlain (9.0f));
    g.drawText ("LA PAUTA AP550 PRO  •  v1.0  •  JUCE  •  VST3 / AU", 20, kH - 28, kW - 40, 14, juce::Justification::centred);
}

void AP550Editor::drawMeterNeedle (juce::Graphics& g, juce::Rectangle<float> face, float db) const
{
    float t = juce::jlimit (0.0f, 1.0f, (db + 60.0f) / 63.0f);
    float ang = -0.9f + t * 1.8f;
    auto c = face.getCentre();
    float len = face.getWidth() * 0.42f;
    auto tip = c + juce::Point<float> (std::sin (ang) * len, -std::cos (ang) * len);
    g.setColour (kRed);
    g.drawLine (juce::Line<float> (c, tip), 2.0f);
    g.setColour (juce::Colours::white);
    g.fillEllipse (c.x - 2, c.y - 2, 4, 4);
}

void AP550Editor::drawToolButton (juce::Graphics&, juce::Rectangle<float>, int, bool) const {}

// ============================================================
// Paint - dibuja valores dinamicos encima del fondo
// ============================================================
void AP550Editor::paint (juce::Graphics& g)
{
    float s = scale();
    if (bg.isValid())
        g.drawImage (bg, 0, 0, getWidth(), getHeight(), 0, 0, kW, kH);
    else
        g.fillAll (juce::Colours::black);

    // Dibuja aguja + valor de cada knob
    auto drawKnobVal = [&] (int x, int y, float norm, juce::String txt, juce::String dbTxt)
    {
        auto r = scaled ({ (float) x, (float) y, 72, 72 }, s);
        float ang = -2.35f + norm * 4.7f;
        auto c = r.getCentre();
        // Linea blanca del knob
        juce::Line<float> l (c, c + juce::Point<float> (std::sin (ang) * 28 * s, -std::cos (ang) * 28 * s));
        g.setColour (juce::Colours::white);
        g.drawLine (l, 2.5f);
        g.fillEllipse (c.x - 3 * s, c.y - 3 * s, 6 * s, 6 * s);
        // Valor numerico debajo
        g.setColour (kYellow);
        g.setFont (fBold (11.0f * s
