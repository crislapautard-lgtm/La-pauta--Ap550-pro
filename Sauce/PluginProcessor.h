#pragma once

#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_dsp/juce_dsp.h>
#include <array>
#include <atomic>
#include "Params.h"

class AP550Processor : public juce::AudioProcessor
{
public:
    AP550Processor();
    ~AP550Processor() override = default;

    // --- AudioProcessor
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout&) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "La Pauta AP550 Pro"; }
    bool acceptsMidi() const override  { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;

    // --- Presets
    void loadFactoryPreset (int index);
    void stepPreset (int direction);
    juce::String getPresetName() const;
    int getPresetIndex() const;

    juce::File getUserPresetDir() const;
    juce::Array<juce::File> listUserPresets() const;
    bool saveUserPreset (const juce::String& name);
    bool loadUserPreset (const juce::File& file);

    // --- A/B
    void toggleSlot();
    void copyToOtherSlot();
    int getActiveSlot() const { return activeSlot; }

    // --- Estado público (lo usa el editor)
    juce::UndoManager undo;                         // debe declararse antes que apvts
    juce::AudioProcessorValueTreeState apvts;

    std::atomic<float> vuL { -60.f }, vuR { -60.f }; // nivel RMS en dBFS

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();

    int  idx (const char* id) const;
    bool flag (const char* id) const;
    void setChoice (const char* id, int index);
    void setBool (const char* id, bool value);
    void updateFilters();

    using Filter = juce::dsp::IIR::Filter<float>;
    using Coeffs = juce::dsp::IIR::Coefficients<float>;

    std::array<Filter, 2> lf, mf, hf, lp;
    juce::dsp::Gain<float> outGain;

    double sampleRate = 44100.0;
    int lastKey = -1;
    std::array<float, 2> vuState { 0.f, 0.f };

    juce::ValueTree slotState[2];
    int activeSlot = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AP550Processor)
};
