#pragma once

#include <JuceHeader.h>

namespace ap
{
    inline constexpr const char* kLfGain = "lfGain";
    inline constexpr const char* kLfFreq = "lfFreq";
    inline constexpr const char* kLfBell = "lfBell";
    inline constexpr const char* kMfGain = "mfGain";
    inline constexpr const char* kMfFreq = "mfFreq";
    inline constexpr const char* kHfGain = "hfGain";
    inline constexpr const char* kHfFreq = "hfFreq";
    inline constexpr const char* kHfBell = "hfBell";
    inline constexpr const char* kEqIn = "eqIn";
    inline constexpr const char* kFilter = "filter";
    inline constexpr const char* kPolarity = "polarity";
    inline constexpr const char* kAnalog = "analog";
    inline constexpr const char* kOutput = "output";
    inline constexpr int kNumGainSteps = 11;

    struct Preset
    {
        juce::String name;
        std::array<float, 13> values {};
    };

    inline const std::vector<Preset>& factoryPresets()
    {
        static const std::vector<Preset> presets =
        {
            { "Default",      { 5, 2, 0, 5, 2, 5, 2, 0, 1, 0, 0, 0, 0 } },
            { "Vocal Air",    { 5, 2, 0, 5, 3, 6, 3, 1, 1, 0, 0, 0, 0 } },
            { "Bass Punch",   { 7, 1, 0, 5, 2, 5, 2, 0, 1, 0, 0, 0, 0 } },
            { "Drum Room",    { 6, 2, 1, 6, 2, 6, 3, 1, 1, 0, 0, 0, 0 } },
            { "La Pauta Mix", { 5, 2, 0, 6, 3, 6, 4, 0, 1, 0, 0, 1, 0 } },
        };
        return presets;
    }
}

class AP550Processor : public juce::AudioProcessor
{
public:
    AP550Processor();
    ~AP550Processor() override;
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;
    const juce::String getName() const override;
    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
    juce::AudioProcessorValueTreeState apvts;
    juce::UndoManager undo;
    std::atomic<float> vuL { -60.0f };
    std::atomic<float> vuR { -60.0f };
    juce::String getPresetName() const;
    int getPresetIndex() const;
    int getActiveSlot() const;
    void toggleSlot();
    void stepPreset (int dir);
    void copyToOtherSlot();
    void loadFactoryPreset (int index);
    void loadUserPreset (const juce::File& file);
    void saveUserPreset (const juce::String& name);
    juce::Array<juce::File> listUserPresets() const;
    juce::File getUserPresetFolder() const;
private:
    double lastSampleRate = 44100.0;
    int activeSlot = 0;
    int presetIndex = 0;
    juce::String presetName = "Default";
    std::array<juce::ValueTree, 2> slotStates;
    juce::dsp::IIR::Filter<float> lf[2];
    juce::dsp::IIR::Filter<float> mf[2];
    juce::dsp::IIR::Filter<float> hf[2];
    juce::dsp::IIR::Filter<float> lp[2];
    void updateFilters();
    void applyPresetValues (const ap::Preset& p);
    void saveSlot (int slot);
    void loadSlot (int slot);
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AP550Processor)
};
