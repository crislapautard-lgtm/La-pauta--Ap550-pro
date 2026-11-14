#include "PluginEditor.h"
#include "EditorLayout.h"
#include "EditorDraw.h"

using namespace lpui;

//==============================================================================
//  Dibujo dinámico
//==============================================================================
void AP550Editor::drawToolButton (juce::Graphics& g, juce::Rectangle<float> r, int kind, bool pressed) const
{
    const auto top = pressed ? juce::Colour (0xff2a2c2c) : juce::Colour (0xff585b5b);
    const auto bot = pressed ? juce::Colour (0xff3f4242) : juce::Colour (0xff2c2e2e);
    g.setGradientFill (juce::ColourGradient::vertical (top, r.getY(), bot, r.getBottom()));
    g.fillRect (r);
    g.setColour (juce::Colour (0xff9a9d9d));
    g.drawRect (r, 1.f);
    g.setColour (juce::Colours::white.withAlpha (0.9f));

    const auto c = r.getCentre();

    switch (kind)
    {
        case 0: case 1: // undo / redo
        {
            const bool canDo = (kind == 0) ? proc.undo.canUndo() : proc.undo.canRedo();
            g.setColour (juce::Colours::white.withAlpha (canDo ? 0.95f : 0.35f));
            g.saveState();
            if (kind == 1)
                g.addTransform (juce::AffineTransform::scale (-1.f, 1.f, c.x, c.y));

            juce::Path arc;
            arc.addCentredArc (c.x + 1.f, c.y + 1.f, 5.5f, 5.5f, 0.f, -1.1f, 2.6f, true);
            g.strokePath (arc, juce::PathStrokeType (1.8f));

            const auto s = arcPt ({ c.x + 1.f, c.y + 1.f }, 5.5f, -1.1f);
            juce::Path head;
            head.addTriangle (s.x - 5.f, s.y + 0.5f, s.x + 2.f, s.y - 3.5f, s.x + 2.f, s.y + 4.f);
            g.fillPath (head);
            g.restoreState();
            break;
        }
        case 2: g.drawArrow (juce::Line<float> (c.x + 9.f, c.y, c.x - 9.f, c.y), 2.f, 8.f, 7.f); break;
        case 3: g.drawArrow (juce::Line<float> (c.x - 9.f, c.y, c.x + 9.f, c.y), 2.f, 8.f, 7.f); break;
        case 4:
        {
            const juce::String from = proc.getActiveSlot() == 0 ? "A" : "B";
            const juce::String to   = proc.getActiveSlot() == 0 ? "B" : "A";
            drawTxt (g, from, { c.x - 14.f, c.y }, 12.f, juce::Colours::white, true, 14.f);
            g.setColour (juce::Colours::white);
            g.drawArrow (juce::Line<float> (c.x - 8.f, c.y, c.x + 7.f, c.y), 1.6f, 6.f, 5.f);
            drawTxt (g, to, { c.x + 14.f, c.y }, 12.f, juce::Colours::white, true, 14.f);
            break;
        }
        case 5: drawTxt (g, "Load", c, 12.f, juce::Colours::white, true, r.getWidth()); break;
        case 6: drawTxt (g, "Save", c, 12.f, juce::Colours::white, true, r.getWidth()); break;
        default: drawTxt (g, "?", c, 13.f, juce::Colours::white, true, r.getWidth()); break;
    }
}

void AP550Editor::drawMeterNeedle (juce::Graphics& g, juce::Rectangle<float> face, float db) const
{
    const juce::Point<float> pivot (face.getCentreX(), face.getBottom() + 95.f);
    const float deg = juce::jlimit (-33.f, 31.f, dbToAngleDeg (db));
    const float a = juce::degreesToRadians (deg);

    const auto p0 = arcPt (pivot, 92.f, a);
    const auto p1 = arcPt (pivot, 160.f, a);

    g.saveState();
    g.reduceClipRegion (face.toNearestInt());
    g.setColour (juce::Colours::black.withAlpha (0.22f));
    g.drawLine (p0.x + 3.f, p0.y + 3.f, p1.x + 3.f, p1.y + 3.f, 2.6f);
    g.setColour (juce::Colour (0xff131313));
    g.drawLine (p0.x, p0.y, p1.x, p1.y, 2.2f);
    g.restoreState();
}

void AP550Editor::paint (juce::Graphics& g)
{
    g.addTransform (juce::AffineTransform::scale (scale()));
    g.setImageResamplingQuality (juce::Graphics::highResamplingQuality);
    g.drawImage (bg, juce::Rectangle<float> (0.f, 0.f, (float) kW, (float) kH));

    // ---- barra superior ------------------------------------------------------
    drawTxt (g, "LP", { 18.f, 13.f }, 12.f, juce::Colours::white.withAlpha (0.85f), true, 30.f);
    const auto& tr = toolRects();
    for (int i = 0; i < 8; ++i)
        drawToolButton (g, tr[(size_t) i], i, pressedTool == i);

    // ---- barra de preset -----------------------------------------------------
    g.setGradientFill (juce::ColourGradient::vertical (juce::Colour (0xff0b0d0d), kNameBar.getY(), juce::Colour (0xff232626), kNameBar.getBottom()));
    g.fillRect (kNameBar);
    g.setColour (juce::Colour (0xff6b6f6f));
    g.drawRect (kNameBar, 1.f);
    drawTxt (g, juce::String (proc.getActiveSlot() == 0 ? "A" : "B") + ": " + proc.getPresetName(),
             kNameBar.getCentre(), 12.f, juce::Colour (0xffe9e9e9), true, kNameBar.getWidth());

    // ---- medidores -----------------------------------------------------------
    drawMeterNeedle (g, kLeftFace,  needleL);
    drawMeterNeedle (g, kRightFace, needleR);

    drawLed (g, { 218.f, 121.f }, getBool (ap::kEqIn), juce::Colour (0xffff3a2a), juce::Colour (0xff4a1612));

    // ---- bandas HF / MF / LF -------------------------------------------------
    static const int nums[11] = { 12, 9, 6, 4, 2, 0, 2, 4, 6, 9, 12 };

    for (int b = 0; b < 3; ++b)
    {
        const auto& bd = bands()[(size_t) b];
        const juce::Point<float> c (bd.cx, bd.cy);
        const int gi = getIdx (bd.gainId);
        const int fi = getIdx (bd.freqId);

        for (int i = 0; i < 11; ++i)
        {
            const float a = juce::degreesToRadians ((float) (i - 5) * 28.8f);
            drawTxt (g, juce::String (nums[i]), arcPt (c, 58.f, a), 14.f, i == gi ? cOrange : cWhite, true, 26.f);
        }
        drawTxt (g, "-", { c.x - 49.f, c.y - 58.f }, 14.f, cWhite, true, 14.f);
        drawTxt (g, "+", { c.x + 51.f, c.y - 58.f }, 14.f, cWhite, true, 14.f);

        for (int f = 0; f < 5; ++f)
            drawTxt (g, bd.labels[f], c + kFreqOff[f], 14.f, f == fi ? cOrange : cCyan, true, 36.f);
        drawTxt (g, bd.unit, c + juce::Point<float> (82.f, 61.f), 12.f, cCyan.withAlpha (0.85f), true, 30.f);

        drawBlueKnob (g, c, kKnobR, juce::degreesToRadians ((float) (gi - 5) * 28.8f));
    }

    // ---- botones LF / IN / HF ------------------------------------------------
    drawTxt (g, "LF", { kBtnLF.x, 703.f }, 13.f, cWhite, true, 30.f);
    drawTxt (g, "IN", { kBtnIN.x, 703.f }, 13.f, cWhite, true, 30.f);
    drawTxt (g, "HF", { kBtnHF.x, 703.f }, 13.f, cWhite, true, 30.f);

    const bool lfBell = getBool (ap::kLfBell);
    const bool hfBell = getBool (ap::kHfBell);
    const bool eqIn   = getBool (ap::kEqIn);

    drawChromeButton (g, kBtnLF, 17.f, lfBell);
    drawChromeButton (g, kBtnIN, 17.f, eqIn);
    drawChromeButton (g, kBtnHF, 17.f, hfBell);

    const juce::Colour ledOn (0xffffb020), ledOff (0xff4a3a18);
    drawLed (g, { 59.f, 773.f },  lfBell,  ledOn, ledOff);
    drawLed (g, { 59.f, 797.f },  ! lfBell, ledOn, ledOff);
    drawBellGlyph  (g, { 31.f, 770.f });
    drawShelfGlyph (g, { 31.f, 797.f }, true);

    drawLed (g, { kBtnIN.x, 773.f }, eqIn, juce::Colour (0xffff3a2a), juce::Colour (0xff4a1612));

    drawLed (g, { 158.f, 773.f }, hfBell,  ledOn, ledOff);
    drawLed (g, { 158.f, 797.f }, ! hfBell, ledOn, ledOff);
    drawBellGlyph  (g, { 190.f, 770.f });
    drawShelfGlyph (g, { 190.f, 797.f }, false);

    // ---- FLTR ----------------------------------------------------------------
    drawTxt (g, "FLTR", { 51.f, kFilterC.y }, 13.f, cWhite, true, 40.f);
    {
        g.setColour (juce::Colour (0xffb9bec2));
        g.drawLine (82.f, kFilterC.y, 100.f, kFilterC.y, 1.6f);

        juce::Path roll;
        roll.startNewSubPath (168.f, kFilterC.y - 3.f);
        roll.lineTo (176.f, kFilterC.y - 3.f);
        roll.cubicTo (181.f, kFilterC.y - 3.f, 184.f, kFilterC.y - 1.f, 186.f, kFilterC.y + 4.f);
        g.strokePath (roll, juce::PathStrokeType (1.6f));

        const bool on = getBool (ap::kFilter);
        drawToggle (g, kFilterC, kFilterC + juce::Point<float> (on ? 17.f : -17.f, -4.f));
    }

    // ---- POL / ANALOG --------------------------------------------------------
    {
        const bool pol = getBool (ap::kPolarity);
        drawChromeButton (g, kPolC, 19.f, pol);
        g.setColour (pol ? juce::Colour (0xffb34a00) : cInk);
        g.drawEllipse (juce::Rectangle<float> (13.f, 13.f).withCentre (kPolC), 1.9f);
        g.drawLine (kPolC.x - 6.5f, kPolC.y + 6.5f, kPolC.x + 6.5f, kPolC.y - 6.5f, 1.9f);

        const bool analog = getBool (ap::kAnalog);
        drawToggle (g, kAnalogC, kAnalogC + juce::Point<float> (0.f, analog ? -18.f : 18.f));
    }

    // ---- OUTPUT --------------------------------------------------------------
    {
        const float v = getVal (ap::kOutput);
        drawRedKnob (g, kOutC, kOutR, juce::degreesToRadians (v / 18.f * 135.f));

        juce::Rectangle<float> box (55.f, 20.f);
        box.setCentre ({ 327.f, 491.f });
        g.setColour (juce::Colours::black.withAlpha (0.35f));
        g.fillRect (box.translated (1.f, 2.f));
        g.setGradientFill (juce::ColourGradient::vertical (juce::Colour (0xfff2f2f2), box.getY(), juce::Colour (0xffc7c9cb), box.getBottom()));
        g.fillRect (box);
        g.setColour (juce::Colour (0xff25292c));
        g.drawRect (box, 1.f);
        drawTxt (g, (v > 0.049f ? "+" : "") + juce::String (v, 1), box.getCentre(), 13.f, juce::Colour (0xff15181a), true, box.getWidth());
    }
}
