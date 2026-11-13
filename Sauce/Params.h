
#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <cmath>
#include <vector>

// =============================================================================
// LA PAUTA AP550 PRO - PARÁMETROS Y PRESETS DE FÁBRICA
// =============================================================================

namespace ap
{
    inline constexpr const char* kLfGain  = "lfGain";
    inline constexpr const char* kLfFreq  = "lfFreq";
    inline constexpr const char* kLfBell  = "lfBell";

    inline constexpr const char* kMfGain  = "mfGain";
    inline constexpr const char* kMfFreq  = "mfFreq";

    inline constexpr const char* kHfGain  = "hfGain";
    inline constexpr const char* kHfFreq  = "hfFreq";
    inline constexpr const char* kHfBell  = "hfBell";

    inline constexpr const char* kEqIn    = "eqIn";
    inline constexpr const char* kFilter  = "filter";
    inline constexpr const char* kPolarity = "polarity";
    inline constexpr const char* kAnalog  = "analog";
    inline constexpr const char* kOutput  = "output";

    inline constexpr float gainValues[] =
    {
        -12.0f, -9.0f, -6.0f, -4.0f, -2.0f,
          0.0f,  2.0f,  4.0f,  6.0f,  9.0f, 12.0f
    };

    inline juce::StringArray gainNames()
    {
        return { "-12", "-9", "-6", "-4", "-2",
                 "0", "+2", "+4", "+6", "+9", "+12" };
    }

    inline constexpr float lfFreqValues[] =
    {
        50.0f, 100.0f, 200.0f, 300.0f, 400.0f
    };

    inline juce::StringArray lfFreqNames()
    {
        return { "50", "100", "200", "300", "400" };
    }

    inline constexpr float mfFreqValues[] =
    {
        400.0f, 800.0f, 1500.0f, 3000.0f, 5000.0f
    };

    inline juce::StringArray mfFreqNames()
    {
        return { "400", "800", "1.5k", "3k", "5k" };
    }

    inline constexpr float hfFreqValues[] =
    {
        5000.0f, 7500.0f, 10000.0f, 12500.0f, 15000.0f
    };

    inline juce::StringArray hfFreqNames()
    {
        return { "5k", "7.5k", "10k", "12.5k", "15k" };
    }

    inline int gainToIndex (float db)
    {
        int best = 0;
        float distance = std::abs (db - gainValues[0]);

        for (int i = 1; i < 11; ++i)
        {
            const float current = std::abs (db - gainValues[i]);

            if (current < distance)
            {
                distance = current;
                best = i;
            }
        }

        return best;
    }

    struct Preset
    {
        juce::String name;

        int lfGain = 5;
        int lfFreq = 1;
        bool lfBell = false;

        int mfGain = 5;
        int mfFreq = 2;

        int hfGain = 5;
        int hfFreq = 2;
        bool hfBell = false;

        bool eqIn = true;
        bool filter = false;
        bool polarity = false;
        bool analog = true;

        float output = 0.0f;
    };

    inline std::vector<Preset> factoryPresets()
    {
        return
        {
            { "Init", 5, 1, false, 5, 2, 5, 2, false, true, false, false, true, 0.0f },
            { "BD", 7, 2, false, 4, 1, 5, 2, false, true, false, false, true, -1.0f },
            { "SD", 5, 1, false, 7, 3, 7, 2, false, true, false, false, true, -1.0f },
            { "Tom", 7, 2, false, 4, 1, 5, 1, false, true, false, false, true, -1.0f },
            { "Overhead", 4, 1, false, 5, 2, 8, 3, false, true, true, false, true, -1.0f },
            { "Hi-Hat", 5, 1, false, 4, 2, 8, 4, false, true, true, false, true, -2.0f },
            { "Bass", 8, 2, false, 4, 1, 5, 2, false, true, false, false, true, -1.0f },
            { "Guitar", 5, 1, false, 6, 3, 7, 2, false, true, false, false, true, -1.0f },
            { "Piano", 5, 1, false, 6, 2, 7, 3, false, true, false, false, true, -1.0f },
            { "Keys", 5, 1, false, 5, 2, 7, 3, false, true, false, false, true, -1.0f },
            { "Male Vocal", 5, 1, false, 6, 2, 7, 3, false, true, false, false, true, -1.0f },
            { "Female Vocal", 5, 1, false, 6, 3, 8, 3, false, true, false, false, true, -1.0f }
        };
    }
}
