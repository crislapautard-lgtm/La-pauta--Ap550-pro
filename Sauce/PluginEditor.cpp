#include "PluginEditor.h"

//==============================================================================
//  Paleta.  Para cambiar el tono del plateado, edita solo estas dos líneas.
//==============================================================================
namespace
{
const juce::Colour cSilverTop { 0xffa6acb2 };   // gris plateado (parte alta)
const juce::Colour cSilverBot { 0xff71777d };   // gris plateado oscuro (parte baja)

const juce::Colour cCyan    { 0xff2aa4e8 };     // frecuencias
const juce::Colour cOrange  { 0xfff2a31b };     // valor seleccionado
const juce::Colour cWhite   { 0xfff1f1f1 };     // escala de ganancia
const juce::Colour cInk     { 0xff1c2126 };     // texto sobre la placa plateada

//------------------------------------------------------------------------------
//  Geometría (coordenadas lógicas 438 x 830, igual que la referencia)
//------------------------------------------------------------------------------
struct BandDef
{
    float cx, cy;
    const char* gainId;
    const char* freqId;
    const char* bellId;      // nullptr = sin selector shelf/campana
    const char* labels[5];
    const char* unit;
};

const std::array<BandDef, 3>& bands()
{
    static const std::array<BandDef, 3> b
    {{
        { 108.f, 276.f, ap::kHfGain, ap::kHfFreq, ap::kHfBell, { "5", "7.5", "10", "12.5", "15" }, "kHz" },
        { 108.f, 415.f, ap::kMfGain, ap::kMfFreq, nullptr,     { ".4", ".8", "1.5", "3", "5" },     "kHz" },
        { 108.f, 555.f, ap::kLfGain, ap::kLfFreq, ap::kLfBell, { "50", "100", "200", "300", "400" }, "Hz" }
    }};
    return b;
}

const juce::Point<float> kFreqOff[5] = { { -67.f, 40.f }, { -41.f, 61.f }, { -2.f, 61.f }, { 38.f, 61.f }, { 70.f, 40.f } };

const juce::Rectangle<float> kLeftFace  { 16.f, 63.f, 180.f, 117.f };
const juce::Rectangle<float> kRightFace { 234.f, 63.f, 180.f, 117.f };

const juce::Point<float> kBtnLF { 51.f, 737.f }, kBtnIN { 107.f, 737.f }, kBtnHF { 162.f, 737.f };
const juce::Point<float> kPolC { 286.f, 273.f };
const juce::Point<float> kAnalogC { 365.f, 271.f };
const juce::Point<float> kFilterC { 131.f, 663.f };
const juce::Point<float> kOutC { 326.f, 380.f };
constexpr float kOutR = 34.f;
constexpr float kKnobR = 33.f;

const std::array<juce::Rectangle<float>, 8>& toolRects()
{
    static const std::array<juce::Rectangle<float>, 8> r
    {{
        { 40.f, 3.f, 38.f, 20.f },    // undo
        { 82.f, 3.f, 38.f, 20.f },    // redo
        { 126.f, 3.f, 40.f, 20.f },   // preset anterior
        { 170.f, 3.f, 40.f, 20.f },   // preset siguiente
        { 216.f, 3.f, 48.f, 20.f },   // A->B
        { 270.f, 3.f, 56.f, 20.f },   // Load
        { 330.f, 3.f, 56.f, 20.f },   // Save
        { 392.f, 3.f, 28.f, 20.f }    // ?
    }};
    return r;
}

const juce::Rectangle<float> kNameBar { 2.f, 28.f, 434.f, 18.f };

juce::Rectangle<float> freqRect (int band, int f)
{
    const auto& b = bands()[(size_t) band];
    return juce::Rectangle<float> (36.f, 18.f).withCentre ({ b.cx + kFreqOff[f].x, b.cy + kFreqOff[f].y });
}

//------------------------------------------------------------------------------
//  Utilidades de dibujo
//------------------------------------------------------------------------------
juce::Font font (float h, bool bold = true)
{
    return juce::Font (juce::FontOptions (h, bold ? juce::Font::bold : juce::Font::plain));
}

void drawTxt (juce::Graphics& g, const juce::String& s, juce::Point<float> c, float size,
              juce::Colour col, bool bold = true, float w = 60.f)
{
    g.setColour (col);
    g.setFont (font (size, bold));
    g.drawText (s, juce::Rectangle<float> (w, size * 1.5f).withCentre (c), juce::Justification::centred, false);
}

void drawEmboss (juce::Graphics& g, const juce::String& s, juce::Point<float> c, float size,
                 bool bold = true, float w = 120.f)
{
    drawTxt (g, s, c + juce::Point<float> (0.f, 1.f), size, juce::Colours::white.withAlpha (0.35f), bold, w);
    drawTxt (g, s, c, size, cInk, bold, w);
}

juce::Point<float> arcPt (juce::Point<float> pivot, float radius, float angleRad)
{
    return { pivot.x + radius * std::sin (angleRad), pivot.y - radius * std::cos (angleRad) };
}

void drawKnob (juce::Graphics& g, juce::Point<float> c, float r, float ang,
               juce::Colour skirtL, juce::Colour skirtD,
               juce::Colour lugL, juce::Colour lugD,
               juce::Colour capL, juce::Colour capD)
{
    // sombra
    g.setColour (juce::Colours::black.withAlpha (0.45f));
    g.fillEllipse (juce::Rectangle<float> (r * 2.15f, r * 2.15f).withCentre (c + juce::Point<float> (3.f, 5.f)));

    // pestaña (indicador), rotada hacia el valor actual
    g.saveState();
    g.addTransform (juce::AffineTransform::rotation (ang, c.x, c.y));
    juce::Rectangle<float> lug (c.x - r * 0.36f, c.y - r * 1.18f, r * 0.72f, r * 0.95f);
    g.setGradientFill (juce::ColourGradient (lugL, lug.getX(), lug.getY(), lugD, lug.getRight(), lug.getBottom(), false));
    g.fillRoundedRectangle (lug, r * 0.25f);
    g.setColour (juce::Colour (0xff0b1a26));
    g.drawRoundedRectangle (lug, r * 0.25f, 1.2f);
    g.restoreState();

    // falda
    auto skirt = juce::Rectangle<float> (r * 2.f, r * 2.f).withCentre (c);
    g.setGradientFill (juce::ColourGradient (skirtL, c.x - r * 0.4f, c.y - r * 0.45f, skirtD, c.x + r * 0.9f, c.y + r * 0.9f, true));
    g.fillEllipse (skirt);
    g.setColour (juce::Colour (0xff0b1a26));
    g.drawEllipse (skirt, 1.3f);

    // tapa central
    const float cr = r * 0.68f;
    auto cap = juce::Rectangle<float> (cr * 2.f, cr * 2.f).withCentre (c);
    g.setGradientFill (juce::ColourGradient (capL, c.x - cr * 0.4f, c.y - cr * 0.5f, capD, c.x + cr * 0.8f, c.y + cr * 0.9f, true));
    g.fillEllipse (cap);
    g.setColour (juce::Colour (0xff222b33).withAlpha (0.8f));
    g.drawEllipse (cap, 1.1f);

    // marca del indicador
    auto dot = arcPt (c, cr * 0.72f, ang);
    g.setColour (juce::Colour (0xff27323b));
    g.fillEllipse (juce::Rectangle<float> (4.f, 4.f).withCentre (dot));
}

void drawBlueKnob (juce::Graphics& g, juce::Point<float> c, float r, float ang)
{
    drawKnob (g, c, r, ang,
              juce::Colour (0xffeaf6ff), juce::Colour (0xff5d89b6),
              juce::Colour (0xffd8ebf9), juce::Colour (0xff6192c2),
              juce::Colour (0xfffdfdfd), juce::Colour (0xffaab2b9));
}

void drawRedKnob (juce::Graphics& g, juce::Point<float> c, float r, float ang)
{
    drawKnob (g, c, r, ang,
              juce::Colour (0xffe8574a), juce::Colour (0xff7a140d),
              juce::Colour (0xffd9dcdf), juce::Colour (0xff8a9096),
              juce::Colour (0xfff06a5a), juce::Colour (0xffa6241a));
}

void drawChromeButton (juce::Graphics& g, juce::Point<float> c, float r, bool pressed)
{
    g.setColour (juce::Colours::black.withAlpha (0.5f));
    g.fillEllipse (juce::Rectangle<float> (r * 2.2f, r * 2.2f).withCentre (c + juce::Point<float> (2.f, 3.f)));

    auto outer = juce::Rectangle<float> (r * 2.f, r * 2.f).withCentre (c);
    g.setGradientFill (juce::ColourGradient::vertical (juce::Colour (0xff5a5f63), outer.getY(), juce::Colour (0xff15181a), outer.getBottom()));
    g.fillEllipse (outer);

    auto inner = outer.reduced (r * 0.14f);
    const auto top = pressed ? juce::Colour (0xffb7bcc1) : juce::Colour (0xfffafafa);
    const auto bot = pressed ? juce::Colour (0xff6e757b) : juce::Colour (0xff9aa1a7);
    g.setGradientFill (juce::ColourGradient (top, c.x - r * 0.35f, c.y - r * 0.45f, bot, c.x + r * 0.7f, c.y + r * 0.8f, true));
    g.fillEllipse (inner);

    g.setColour (juce::Colours::white.withAlpha (pressed ? 0.15f : 0.55f));
    g.fillEllipse (juce::Rectangle<float> (r * 0.8f, r * 0.45f).withCentre (c + juce::Point<float> (-r * 0.15f, -r * 0.45f)));
}

void drawToggle (juce::Graphics& g, juce::Point<float> base, juce::Point<float> tip)
{
    g.setColour (juce::Colours::black.withAlpha (0.4f));
    g.fillEllipse (juce::Rectangle<float> (26.f, 26.f).withCentre (base + juce::Point<float> (2.f, 3.f)));

    auto ring = juce::Rectangle<float> (24.f, 24.f).withCentre (base);
    g.setGradientFill (juce::ColourGradient::vertical (juce::Colour (0xffd6d9dc), ring.getY(), juce::Colour (0xff4b5056), ring.getBottom()));
    g.fillEllipse (ring);
    g.setColour (juce::Colour (0xff111416));
    g.drawEllipse (ring, 1.2f);
    g.setColour (juce::Colour (0xff23282c));
    g.fillEllipse (juce::Rectangle<float> (12.f, 12.f).withCentre (base));

    g.setColour (juce::Colours::black.withAlpha (0.6f));
    g.drawLine (base.x, base.y, tip.x, tip.y, 9.f);
    g.setColour (juce::Colour (0xffcfd3d6));
    g.drawLine (base.x, base.y, tip.x, tip.y, 6.f);
    g.setColour (juce::Colours::white.withAlpha (0.6f));
    g.drawLine (base.x - 1.f, base.y - 1.f, tip.x - 1.f, tip.y - 1.f, 1.5f);

    g.setGradientFill (juce::ColourGradient (juce::Colour (0xfff4f6f7), tip.x - 2.f, tip.y - 2.f, juce::Colour (0xff6b7278), tip.x + 4.f, tip.y + 4.f, true));
    g.fillEllipse (juce::Rectangle<float> (11.f, 11.f).withCentre (tip));
    g.setColour (juce::Colour (0xff111416));
    g.drawEllipse (juce::Rectangle<float> (11.f, 11.f).withCentre (tip), 1.f);
}

void drawLed (juce::Graphics& g, juce::Point<float> c, bool on, juce::Colour onCol, juce::Colour offCol)
{
    if (on)
    {
        g.setColour (onCol.withAlpha (0.30f));
        g.fillEllipse (juce::Rectangle<float> (15.f, 15.f).withCentre (c));
    }
    g.setColour (on ? onCol : offCol);
    g.fillEllipse (juce::Rectangle<float> (7.f, 7.f).withCentre (c));
    g.setColour (juce::Colours::white.withAlpha (on ? 0.55f : 0.12f));
    g.fillEllipse (juce::Rectangle<float> (2.5f, 2.5f).withCentre (c + juce::Point<float> (-1.f, -1.2f)));
}

void drawBellGlyph (juce::Graphics& g, juce::Point<float> c)
{
    juce::Path p;
    p.startNewSubPath (c.x - 9.f, c.y + 3.f);
    p.cubicTo (c.x - 3.f, c.y + 3.f, c.x - 3.f, c.y - 4.f, c.x, c.y - 4.f);
    p.cubicTo (c.x + 3.f, c.y - 4.f, c.x + 3.f, c.y + 3.f, c.x + 9.f, c.y + 3.f);
    g.setColour (juce::Colour (0xffb9bec2));
    g.strokePath (p, juce::PathStrokeType (1.6f));
}

void drawShelfGlyph (juce::Graphics& g, juce::Point<float> c, bool lowShelf)
{
    juce::Path p;
    if (lowShelf)
    {
        p.startNewSubPath (c.x - 9.f, c.y - 4.f);
        p.lineTo (c.x - 3.f, c.y - 4.f);
        p.cubicTo (c.x, c.y - 4.f, c.x, c.y + 3.f, c.x + 3.f, c.y + 3.f);
        p.lineTo (c.x + 9.f, c.y + 3.f);
    }
    else
    {
        p.startNewSubPath (c.x - 9.f, c.y + 3.f);
        p.lineTo (c.x - 3.f, c.y + 3.f);
        p.cubicTo (c.x, c.y + 3.f, c.x, c.y - 4.f, c.x + 3.f, c.y - 4.f);
        p.lineTo (c.x + 9.f, c.y - 4.f);
    }
    g.setColour (juce::Colour (0xffb9bec2));
    g.strokePath (p, juce::PathStrokeType (1.6f));
}

float dbToAngleDeg (float db)
{
    return -28.f + (db + 30.f) * (56.f / 33.f);
}

void drawMeterFace (juce::Graphics& g, juce::Rectangle<float> r)
{
    // marco
    g.setColour (juce::Colour (0xff060707));
    g.fillRoundedRectangle (r.expanded (6.f), 6.f);
    g.setColour (juce::Colour (0xff3d4040));
    g.drawRoundedRectangle (r.expanded (6.f), 6.f, 1.5f);

    // cara crema
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xfff7f0c9), r.getCentreX(), r.getY() + r.getHeight() * 0.7f,
                                             juce::Colour (0xffb5a462), r.getX(), r.getY(), true));
    g.fillRoundedRectangle (r, 3.f);

    const juce::Point<float> pivot (r.getCentreX(), r.getBottom() + 95.f);
    const float R = 150.f;

    g.saveState();
    g.reduceClipRegion (r.toNearestInt());

    // zona roja
    {
        juce::Path red;
        red.addCentredArc (pivot.x, pivot.y, R + 2.f, R + 2.f, 0.f,
                           juce::degreesToRadians (dbToAngleDeg (-4.f)),
                           juce::degreesToRadians (dbToAngleDeg (3.f)), true);
        g.setColour (juce::Colour (0xffc7311f));
        g.strokePath (red, juce::PathStrokeType (3.5f));

        juce::Path blk;
        blk.addCentredArc (pivot.x, pivot.y, R + 2.f, R + 2.f, 0.f,
                           juce::degreesToRadians (dbToAngleDeg (-30.f)),
                           juce::degreesToRadians (dbToAngleDeg (-4.f)), true);
        g.setColour (juce::Colour (0xff2a2418));
        g.strokePath (blk, juce::PathStrokeType (1.2f));
    }

    // marcas y números
    for (int db = -30; db <= 3; db += 3)
    {
        const bool major = (db % 6 == 0) && db <= 0;
        const float a = juce::degreesToRadians (dbToAngleDeg ((float) db));
        const auto p0 = arcPt (pivot, R + 2.f, a);
        const auto p1 = arcPt (pivot, R + (major ? 9.f : 5.f), a);
        g.setColour (db > -4 ? juce::Colour (0xffc7311f) : juce::Colour (0xff2a2418));
        g.drawLine (p0.x, p0.y, p1.x, p1.y, major ? 1.5f : 1.0f);

        if (major)
        {
            const auto pt = arcPt (pivot, R - 9.f, a);
            drawTxt (g, juce::String (db), pt, 9.5f, db >= 0 ? juce::Colour (0xffc7311f) : juce::Colour (0xff2a2418), true, 24.f);
        }
    }

    drawTxt (g, "VU", { r.getCentreX(), r.getBottom() - 34.f }, 15.f, juce::Colour (0xff2a2418).withAlpha (0.85f));
    drawTxt (g, "LA PAUTA", { r.getCentreX(), r.getBottom() - 18.f }, 8.f, juce::Colour (0xff2a2418).withAlpha (0.7f), true, 70.f);

    // brillo de cristal
    g.setGradientFill (juce::ColourGradient::vertical (juce::Colours::white.withAlpha (0.20f), r.getY(),
                                                       juce::Colours::white.withAlpha (0.0f), r.getY() + r.getHeight() * 0.45f));
    g.fillRect (r);
    g.restoreState();

    g.setColour (juce::Colours::black.withAlpha (0.6f));
    g.drawRoundedRectangle (r, 3.f, 1.5f);
}
} // namespace

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

//==============================================================================
//  Dibujo dinámico
//==============================================================================
void AP550Editor::drawTool
