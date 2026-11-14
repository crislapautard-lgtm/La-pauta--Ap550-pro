#pragma once

#include <array>
#include "PluginProcessor.h"

//==============================================================================
//  Paleta, geometría y utilidades de texto de La Pauta AP550 Pro
//==============================================================================
namespace lpui
{
inline const juce::Colour cSilverTop { 0xffa6acb2 };   // gris plateado (parte alta)
inline const juce::Colour cSilverBot { 0xff71777d };   // gris plateado oscuro (parte baja)

inline const juce::Colour cCyan    { 0xff2aa4e8 };     // frecuencias
inline const juce::Colour cOrange  { 0xfff2a31b };     // valor seleccionado
inline const juce::Colour cWhite   { 0xfff1f1f1 };     // escala de ganancia
inline const juce::Colour cInk     { 0xff1c2126 };     // texto sobre la placa plateada

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

inline const std::array<BandDef, 3>& bands()
{
    static const std::array<BandDef, 3> b
    {{
        { 108.f, 276.f, ap::kHfGain, ap::kHfFreq, ap::kHfBell, { "5", "7.5", "10", "12.5", "15" }, "kHz" },
        { 108.f, 415.f, ap::kMfGain, ap::kMfFreq, nullptr,     { ".4", ".8", "1.5", "3", "5" },     "kHz" },
        { 108.f, 555.f, ap::kLfGain, ap::kLfFreq, ap::kLfBell, { "50", "100", "200", "300", "400" }, "Hz" }
    }};
    return b;
}

inline const juce::Point<float> kFreqOff[5] = { { -67.f, 40.f }, { -41.f, 61.f }, { -2.f, 61.f }, { 38.f, 61.f }, { 70.f, 40.f } };

inline const juce::Rectangle<float> kLeftFace  { 16.f, 63.f, 180.f, 117.f };
inline const juce::Rectangle<float> kRightFace { 234.f, 63.f, 180.f, 117.f };

inline const juce::Point<float> kBtnLF { 51.f, 737.f }, kBtnIN { 107.f, 737.f }, kBtnHF { 162.f, 737.f };
inline const juce::Point<float> kPolC { 286.f, 273.f };
inline const juce::Point<float> kAnalogC { 365.f, 271.f };
inline const juce::Point<float> kFilterC { 131.f, 663.f };
inline const juce::Point<float> kOutC { 326.f, 380.f };
inline constexpr float kOutR = 34.f;
inline constexpr float kKnobR = 33.f;

inline const std::array<juce::Rectangle<float>, 8>& toolRects()
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

inline const juce::Rectangle<float> kNameBar { 2.f, 28.f, 434.f, 18.f };

inline juce::Rectangle<float> freqRect (int band, int f)
{
    const auto& b = bands()[(size_t) band];
    return juce::Rectangle<float> (36.f, 18.f).withCentre ({ b.cx + kFreqOff[f].x, b.cy + kFreqOff[f].y });
}

//------------------------------------------------------------------------------
//  Utilidades de dibujo
//------------------------------------------------------------------------------
inline juce::Font font (float h, bool bold = true)
{
    return juce::Font (juce::FontOptions (h, bold ? juce::Font::bold : juce::Font::plain));
}

inline void drawTxt (juce::Graphics& g, const juce::String& s, juce::Point<float> c, float size,
              juce::Colour col, bool bold = true, float w = 60.f)
{
    g.setColour (col);
    g.setFont (font (size, bold));
    g.drawText (s, juce::Rectangle<float> (w, size * 1.5f).withCentre (c), juce::Justification::centred, false);
}

inline void drawEmboss (juce::Graphics& g, const juce::String& s, juce::Point<float> c, float size,
                 bool bold = true, float w = 120.f)
{
    drawTxt (g, s, c + juce::Point<float> (0.f, 1.f), size, juce::Colours::white.withAlpha (0.35f), bold, w);
    drawTxt (g, s, c, size, cInk, bold, w);
}

inline juce::Point<float> arcPt (juce::Point<float> pivot, float radius, float angleRad)
{
    return { pivot.x + radius * std::sin (angleRad), pivot.y - radius * std::cos (angleRad) };
}

} // namespace lpui
