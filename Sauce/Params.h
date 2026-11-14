#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <cmath>
#include <vector>

// =============================================================================
//  La Pauta AP550 Pro - parámetros y presets de fábrica
// =============================================================================
namespace ap
{
// IDs de parámetros
inline constexpr const char* kLfGain   = "lfGain";
inline constexpr const char* kLfFreq   = "lfFreq";
inline constexpr const char* kLfBell   = "lfBell";   // false = shelf, true = campana
inline constexpr const char* kMfGain   = "mfGain";
inline constexpr const char* kMfFreq   = "mfFreq";
inline constexpr const char* kHfGain   = "hfGain";
inline constexpr const char* kHfFreq   = "hfFreq";
inline constexpr const char* kHfBell   = "hfBell";   // false = shelf, true = campana
inline constexpr const char* kEqIn     = "eqIn";
inline constexpr const char* kFilter   = "filter";
inline constexpr const char* kPolarity = "polarity";
inline constexpr const char* kAnalog   = "analog";
inline constexpr const char* kOutput   = "output";

// Posiciones de las perillas (igual que el 550A: pasos de ganancia fijos)
inline constexpr int   kNumGainSteps = 11;
inline constexpr float kGainSteps[kNumGainSteps] = { -12.f, -9.f, -6.f, -4.f, -2.f, 0.f, 2.f, 4.f, 6.f, 9.f, 12.f };

inline constexpr float kLfFreqs[5] = { 50.f, 100.f, 200.f, 300.f, 400.f };
inline constexpr float kMfFreqs[5] = { 400.f, 800.f, 1500.f, 3000.f, 5000.f };
inline constexpr float kHfFreqs[5] = { 5000.f, 7500.f, 10000.f, 12500.f, 15000.f };

inline int gainToIndex (float dB)
{
    int best = 5;
    float bestDist = 1.0e9f;
    for (int i = 0; i < kNumGainSteps; ++i)
    {
        const float d = std::abs (kGainSteps[i] - dB);
        if (d < bestDist) { bestDist = d; best = i; }
    }
    return best;
}

inline juce::StringArray gainNames()
{
    juce::StringArray s;
    for (float g : kGainSteps)
        s.add ((g > 0.f ? "+" : "") + juce::String ((int) g) + " dB");
    return s;
}

inline juce::StringArray lfFreqNames() { return { "50 Hz", "100 Hz", "200 Hz", "300 Hz", "400 Hz" }; }
inline juce::StringArray mfFreqNames() { return { "0.4 kHz", "0.8 kHz", "1.5 kHz", "3 kHz", "5 kHz" }; }
inline juce::StringArray hfFreqNames() { return { "5 kHz", "7.5 kHz", "10 kHz", "12.5 kHz", "15 kHz" }; }

// -----------------------------------------------------------------------------
//  Presets de fábrica.
//  Índices de frecuencia: LF 0..4 = 50/100/200/300/400 Hz
//                         MF 0..4 = 0.4/0.8/1.5/3/5 kHz
//                         HF 0..4 = 5/7.5/10/12.5/15 kHz
//  lfBell / hfBell: false = shelf, true = campana.
//  Para agregar o cambiar presets, edita solo esta tabla.
// -----------------------------------------------------------------------------
struct Preset
{
    const char* name;
    int   lfF;  float lfG;  bool lfBell;
    int   mfF;  float mfG;
    int   hfF;  float hfG;  bool hfBell;
    bool  filter;
};

inline const std::vector<Preset>& factoryPresets()
{
    static const std::vector<Preset> list
    {
        //  nombre        lfF lfG  bell   mfF mfG   hfF hfG  bell  filtro
        { "Init",          1,  0.f, false,  2,  0.f,  2,  0.f, false, false },

        { "BD 1",          0,  6.f, false,  0, -6.f,  1,  2.f, false, false },
        { "BD 2",          1,  4.f, true,   1, -4.f,  0,  4.f, true,  false },
        { "BD 3",          0,  4.f, false,  0, -9.f,  2,  2.f, false, false },

        { "SD 1",          2,  4.f, true,   3,  2.f,  2,  4.f, false, false },
        { "SD 2",          1,  2.f, false,  1, -4.f,  1,  4.f, true,  false },
        { "SD 3",          2,  6.f, false,  3,  4.f,  3,  2.f, false, false },

        { "Tom 1",         2,  4.f, false,  0, -4.f,  0,  4.f, false, false },
        { "Tom 2",         1,  6.f, true,   2,  2.f,  1,  2.f, true,  false },

        { "OH 1",          3, -4.f, false,  1, -2.f,  2,  4.f, false, false },
        { "OH 2",          2, -6.f, false,  3, -2.f,  3,  6.f, false, false },
        { "HH 1",          4, -9.f, false,  3, -2.f,  3,  4.f, false, false },

        { "Bass DI 1",     0,  6.f, false,  1, -4.f,  0,  2.f, false, false },
        { "Bass DI 2",     1,  4.f, true,   3,  4.f,  1,  2.f, true,  false },
        { "Bass Amp",      0,  4.f, false,  0, -6.f,  0,  2.f, false, false },

        { "E Gtr 1",       2, -4.f, false,  3,  4.f,  2,  2.f, false, false },
        { "E Gtr 2",       3, -6.f, false,  2,  2.f,  1,  4.f, true,  false },
        { "Ac Gtr 1",      2, -4.f, false,  4,  2.f,  3,  6.f, false, false },
        { "Ac Gtr 2",      1, -2.f, true,   3, -2.f,  2,  4.f, false, false },

        { "Piano 1",       1,  2.f, false,  3, -2.f,  2,  4.f, false, false },
        { "Piano 2",       2, -2.f, false,  4,  2.f,  3,  4.f, true,  false },
        { "Keys",          2, -4.f, false,  1, -2.f,  1,  4.f, false, false },

        { "Vox 1",         1, -4.f, false,  3,  4.f,  3,  6.f, false, false },
        { "Vox 2",         2, -2.f, true,   4,  4.f,  2,  4.f, false, false },
        { "Vox 3",         1, -2.f, false,  2,  2.f,  3,  9.f, false, false },
        { "Male Vox",      0,  2.f, false,  2, -2.f,  2,  4.f, false, false },
        { "Female Vox",    2, -6.f, false,  3,  2.f,  3,  6.f, false, false },
        { "BGV",           3, -4.f, false,  4, -2.f,  2,  4.f, false, false },

        { "Strings",       2, -2.f, false,  0, -2.f,  3,  4.f, false, false },
        { "Brass",         1,  2.f, false,  3,  2.f,  1,  2.f, false, false },

        { "Drum Bus",      0,  4.f, false,  4,  2.f,  2,  2.f, false, false },
        { "Mix Bus",       0,  2.f, false,  3, -2.f,  3,  2.f, false, false },
        { "Master Air",    0,  0.f, false,  2,  0.f,  3,  4.f, false, false },

        { "Warm",          0,  4.f, false,  2, -2.f,  0, -2.f, false, true  },
        { "Bright",        4,  0.f, false,  3,  2.f,  3,  6.f, false, false },
        { "De-Mud",        3, -4.f, true,   1,  0.f,  2,  0.f, false, false },
        { "Presence",      2,  0.f, false,  3,  4.f,  2,  2.f, false, false },
        { "Smile",         1,  4.f, false,  1, -4.f,  3,  6.f, false, false },
        { "Telephone",     4, -12.f, false, 3,  6.f,  0, -12.f, false, true  }
    };
    return list;
}

} // namespace ap
