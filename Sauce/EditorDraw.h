#pragma once

#include "EditorLayout.h"

//==============================================================================
//  Dibujo de perillas, botones, LEDs y medidores
//==============================================================================
namespace lpui
{
inline void drawKnob (juce::Graphics& g, juce::Point<float> c, float r, float ang,
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

inline void drawBlueKnob (juce::Graphics& g, juce::Point<float> c, float r, float ang)
{
    drawKnob (g, c, r, ang,
              juce::Colour (0xffeaf6ff), juce::Colour (0xff5d89b6),
              juce::Colour (0xffd8ebf9), juce::Colour (0xff6192c2),
              juce::Colour (0xfffdfdfd), juce::Colour (0xffaab2b9));
}

inline void drawRedKnob (juce::Graphics& g, juce::Point<float> c, float r, float ang)
{
    drawKnob (g, c, r, ang,
              juce::Colour (0xffe8574a), juce::Colour (0xff7a140d),
              juce::Colour (0xffd9dcdf), juce::Colour (0xff8a9096),
              juce::Colour (0xfff06a5a), juce::Colour (0xffa6241a));
}

inline void drawChromeButton (juce::Graphics& g, juce::Point<float> c, float r, bool pressed)
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

inline void drawToggle (juce::Graphics& g, juce::Point<float> base, juce::Point<float> tip)
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

inline void drawLed (juce::Graphics& g, juce::Point<float> c, bool on, juce::Colour onCol, juce::Colour offCol)
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

inline void drawBellGlyph (juce::Graphics& g, juce::Point<float> c)
{
    juce::Path p;
    p.startNewSubPath (c.x - 9.f, c.y + 3.f);
    p.cubicTo (c.x - 3.f, c.y + 3.f, c.x - 3.f, c.y - 4.f, c.x, c.y - 4.f);
    p.cubicTo (c.x + 3.f, c.y - 4.f, c.x + 3.f, c.y + 3.f, c.x + 9.f, c.y + 3.f);
    g.setColour (juce::Colour (0xffb9bec2));
    g.strokePath (p, juce::PathStrokeType (1.6f));
}

inline void drawShelfGlyph (juce::Graphics& g, juce::Point<float> c, bool lowShelf)
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

inline float dbToAngleDeg (float db)
{
    return -28.f + (db + 30.f) * (56.f / 33.f);
}

inline void drawMeterFace (juce::Graphics& g, juce::Rectangle<float> r)
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

} // namespace lpui
