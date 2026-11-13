
#include "PluginProcessor.h"
#include "PluginEditor.h"

#include <algorithm>
#include <cmath>

namespace
{
    constexpr float kDbFloor = -60.0f;
    constexpr float kAnalogDrive = 1.35f;

    float safeDb (float value)
    {
        return juce::jlimit (-60.0f, 18.0f, value);
    }

    int safeIndex (int value, int maximum)
    {
        return juce::jlimit (0, maximum, value);
    }
}

// =============================================================================
// CONSTRUCTOR
// =============================================================================

AP550Processor::AP550Processor()
    : juce::AudioProcessor (
        BusesProperties()
            .withInput ("Input",
                        juce::AudioChannelSet::stereo(),
                        true)
            .withOutput ("Output",
                         juce::AudioChannelSet::stereo(),
                         true)),
      apvts (*this, &undo, "PARAMS", createLayout())
{
    slotState[0] = apvts.copyState();
    slotState[1] = apvts.copyState();
}

// =============================================================================
// PARÁMETROS
// =============================================================================

juce::AudioProcessorValueTreeState::ParameterLayout
AP550Processor::createLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    using Choice = juce::AudioParameterChoice;
    using Bool = juce::AudioParameterBool;
    using Float = juce::AudioParameterFloat;

    const juce::StringArray gains = ap::gainNames();

    layout.add (std::make_unique<Choice>(
        juce::ParameterID { ap::kLfGain, 1 },
        "LF Gain",
        gains,
        5));

    layout.add (std::make_unique<Choice>(
        juce::ParameterID { ap::kLfFreq, 1 },
        "LF Frequency",
        ap::lfFreqNames(),
        1));

    layout.add (std::make_unique<Bool>(
        juce::ParameterID { ap::kLfBell, 1 },
        "LF Bell",
        false));

    layout.add (std::make_unique<Choice>(
        juce::ParameterID { ap::kMfGain, 1 },
        "MF Gain",
        gains,
        5));

    layout.add (std::make_unique<Choice>(
        juce::ParameterID { ap::kMfFreq, 1 },
        "MF Frequency",
        ap::mfFreqNames(),
        2));

    layout.add (std::make_unique<Choice>(
        juce::ParameterID { ap::kHfGain, 1 },
        "HF Gain",
        gains,
        5));

    layout.add (std::make_unique<Choice>(
        juce::ParameterID { ap::kHfFreq, 1 },
        "HF Frequency",
        ap::hfFreqNames(),
        2));

    layout.add (std::make_unique<Bool>(
        juce::ParameterID { ap::kHfBell, 1 },
        "HF Bell",
        false));

    layout.add (std::make_unique<Bool>(
        juce::ParameterID { ap::kEqIn, 1 },
        "EQ In",
        true));

    layout.add (std::make_unique<Bool>(
        juce::ParameterID { ap::kFilter, 1 },
        "Filter",
        false));

    layout.add (std::make_unique<Bool>(
        juce::ParameterID { ap::kPolarity, 1 },
        "Polarity",
        false));

    layout.add (std::make_unique<Bool>(
        juce::ParameterID { ap::kAnalog, 1 },
        "Analog",
        true));

    layout.add (std::make_unique<Float>(
        juce::ParameterID { ap::kOutput, 1 },
        "Output",
        juce::NormalisableRange<float> (-18.0f, 18.0f, 0.1f),
        0.0f));

    return layout;
}

// =============================================================================
// LECTURA DE PARÁMETROS
// =============================================================================

int AP550Processor::idx (const char* id) const
{
    if (auto* value = apvts.getRawParameterValue (id))
        return static_cast<int> (std::round (value->load()));

    return 0;
}

bool AP550Processor::flag (const char* id) const
{
    if (auto* value = apvts.getRawParameterValue (id))
        return value->load() >= 0.5f;

    return false;
}

// =============================================================================
// ESCRITURA DE PARÁMETROS
// =============================================================================

void AP550Processor::setChoice (const char* id, int index)
{
    if (auto* parameter = apvts.getParameter (id))
    {
        const float normalized =
            parameter->convertTo0to1 (static_cast<float> (index));

        parameter->beginChangeGesture();
        parameter->setValueNotifyingHost (normalized);
        parameter->endChangeGesture();
    }
}

void AP550Processor::setBool (const char* id, bool value)
{
    if (auto* parameter = apvts.getParameter (id))
    {
        parameter->beginChangeGesture();
        parameter->setValueNotifyingHost (
            value ? 1.0f : 0.0f);
        parameter->endChangeGesture();
    }
}

// =============================================================================
// PREPARACIÓN DE AUDIO
// =============================================================================

void AP550Processor::prepareToPlay (
    double newSampleRate,
    int samplesPerBlock)
{
    sampleRate = newSampleRate;

    juce::dsp::ProcessSpec spec;

    spec.sampleRate = sampleRate;
    spec.maximumBlockSize =
        static_cast<juce::uint32> (
            juce::jmax (1, samplesPerBlock));

    spec.numChannels = 2;

    outGain.prepare (spec);
    outGain.setRampDurationSeconds (0.02);

    for (int channel = 0; channel < 2; ++channel)
    {
        lf[channel].reset();
        mf[channel].reset();
        hf[channel].reset();
        lp[channel].reset();
    }

    lastKey = -1;

    updateFilters();
}

// =============================================================================
// CONFIGURACIÓN DE CANALES
// =============================================================================

bool AP550Processor::isBusesLayoutSupported (
    const BusesLayout& layouts) const
{
    const auto input =
        layouts.getMainInputChannelSet();

    const auto output =
        layouts.getMainOutputChannelSet();

    if (input != output)
        return false;

    return input == juce::AudioChannelSet::mono()
        || input == juce::AudioChannelSet::stereo();
}

// =============================================================================
// ACTUALIZACIÓN DE FILTROS
// =============================================================================

void AP550Processor::updateFilters()
{
    const int lfGainIndex = safeIndex (idx (ap::kLfGain), 10);
    const int lfFreqIndex = safeIndex (idx (ap::kLfFreq), 4);

    const int mfGainIndex = safeIndex (idx (ap::kMfGain), 10);
    const int mfFreqIndex = safeIndex (idx (ap::kMfFreq), 4);

    const int hfGainIndex = safeIndex (idx (ap::kHfGain), 10);
    const int hfFreqIndex = safeIndex (idx (ap::kHfFreq), 4);

    const bool lfBell = flag (ap::kLfBell);
    const bool hfBell = flag (ap::kHfBell);

    const int key =
        lfGainIndex
        + lfFreqIndex * 20
        + mfGainIndex * 200
        + mfFreqIndex * 2000
        + hfGainIndex * 20000
        + hfFreqIndex * 200000
        + static_cast<int> (lfBell) * 2000000
        + static_cast<int> (hfBell) * 4000000;

    if (key == lastKey)
        return;

    lastKey = key;

    // IMPORTANTE:
    // Estos son arreglos definidos en Params.h.
    // No se utilizan paréntesis.

    const float lowFrequency =
        ap::lfFreqValues[lfFreqIndex];

    const float midFrequency =
        ap::mfFreqValues[mfFreqIndex];

    const float highFrequency =
        ap::hfFreqValues[hfFreqIndex];

    const float lowDb =
        ap::gainValues[lfGainIndex];

    const float midDb =
        ap::gainValues[mfGainIndex];

    const float highDb =
        ap::gainValues[hfGainIndex];

    const float nyquistLimit =
        static_cast<float> (sampleRate * 0.45);

    const float lowHz =
        juce::jlimit (20.0f, nyquistLimit, lowFrequency);

    const float midHz =
        juce::jlimit (20.0f, nyquistLimit, midFrequency);

    const float highHz =
        juce::jlimit (20.0f, nyquistLimit, highFrequency);

    const float lowPassHz =
        juce::jlimit (100.0f, nyquistLimit, 18000.0f);

    auto lowCoefficients = lfBell
        ? Coeffs::makePeakFilter (
            sampleRate,
            lowHz,
            0.8f,
            juce::Decibels::decibelsToGain (lowDb))
        : Coeffs::makeLowShelf (
            sampleRate,
            lowHz,
            0.707f,
            juce::Decibels::decibelsToGain (lowDb));

    auto midCoefficients =
        Coeffs::makePeakFilter (
            sampleRate,
            midHz,
            1.0f,
            juce::Decibels::decibelsToGain (midDb));

    auto highCoefficients = hfBell
        ? Coeffs::makePeakFilter (
            sampleRate,
            highHz,
            0.8f,
            juce::Decibels::decibelsToGain (highDb))
        : Coeffs::makeHighShelf (
            sampleRate,
            highHz,
            0.707f,
            juce::Decibels::decibelsToGain (highDb));

    auto lowPassCoefficients =
        Coeffs::makeLowPass (
            sampleRate,
            lowPassHz,
            0.707f);

    for (int channel = 0; channel < 2; ++channel)
    {
        lf[channel].coefficients = lowCoefficients;
        mf[channel].coefficients = midCoefficients;
        hf[channel].coefficients = highCoefficients;
        lp[channel].coefficients = lowPassCoefficients;
    }
}

// =============================================================================
// PROCESAMIENTO PRINCIPAL
// =============================================================================

void AP550Processor::processBlock (
    juce::AudioBuffer<float>& buffer,
    juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;

    juce::ignoreUnused (midiMessages);

    const int inputChannels =
        getTotalNumInputChannels();

    const int outputChannels =
        getTotalNumOutputChannels();

    const int numSamples =
        buffer.getNumSamples();

    // Limpiar canales de salida adicionales.

    for (int channel = inputChannels;
         channel < outputChannels;
         ++channel)
    {
        buffer.clear (channel, 0, numSamples);
    }

    if (numSamples <= 0 || inputChannels <= 0)
        return;

    updateFilters();

    const bool eqEnabled = flag (ap::kEqIn);
    const bool filterEnabled = flag (ap::kFilter);
    const bool polarityInverted = flag (ap::kPolarity);
    const bool analogEnabled = flag (ap::kAnalog);

    const float outputDb = safeDb (
        apvts.getRawParameterValue (ap::kOutput) != nullptr
            ? apvts.getRawParameterValue (ap::kOutput)->load()
            : 0.0f);

    outGain.setGainDecibels (outputDb);

    const int channelsToProcess =
        juce::jmin (inputChannels, 2);

    float peakSquareL = 0.0f;
    float peakSquareR = 0.0f;

    // EQ y procesamiento analógico.

    for (int channel = 0;
         channel < channelsToProcess;
         ++channel)
    {
        float* data = buffer.getWritePointer (channel);

        for (int sample = 0; sample < numSamples; ++sample)
        {
            float x = data[sample];

            if (eqEnabled)
            {
                x = lf[channel].processSample (x);
                x = mf[channel].processSample (x);
                x = hf[channel].processSample (x);
            }

            if (filterEnabled)
            {
                x = lp[channel].processSample (x);
            }

            if (analogEnabled)
            {
                const float driven =
                    std::tanh (x * kAnalogDrive);

                const float reference =
                    std::tanh (kAnalogDrive);

                x = reference > 0.0f
                    ? driven / reference
                    : x;
            }

            if (polarityInverted)
                x = -x;

            data[sample] = x;

            if (channel == 0)
                peakSquareL += x * x;
            else
                peakSquareR += x * x;
        }
    }

    // Ganancia de salida con rampa para evitar clics.

    juce::dsp::AudioBlock<float> audioBlock (buffer);

    juce::dsp::ProcessContextReplacing<float> context (
        audioBlock);

    outGain.process (context);

    // Medición RMS en dBFS.

    const float inverseSamples =
        1.0f / static_cast<float> (numSamples);

    const float rmsL =
        std::sqrt (peakSquareL * inverseSamples);

    const float rmsR =
        inputChannels > 1
            ? std::sqrt (peakSquareR * inverseSamples)
            : rmsL;

    const float dbL =
        juce::Decibels::gainToDecibels (
            rmsL,
            kDbFloor);

    const float dbR =
        juce::Decibels::gainToDecibels (
            rmsR,
            kDbFloor);

    vuL.store (dbL);
    vuR.store (dbR);
}

// =============================================================================
// EDITOR
// =============================================================================

juce::AudioProcessorEditor*
AP550Processor::createEditor()
{
    return new AP550Editor (*this);
}

// =============================================================================
// GUARDAR ESTADO
// =============================================================================

void AP550Processor::getStateInformation (
    juce::MemoryBlock& destinationData)
{
    const auto state = apvts.copyState();

    if (auto xml = state.createXml())
        copyXmlToBinary (*xml, destinationData);
}

// =============================================================================
// RESTAURAR ESTADO
// =============================================================================

void AP550Processor::setStateInformation (
    const void* data,
    int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
    {
        if (xml->hasTagName (apvts.state.getType()))
        {
            const auto restoredState =
                juce::ValueTree::fromXml (*xml);

            if (restoredState.isValid())
            {
                apvts.replaceState (restoredState);
                lastKey = -1;
            }
        }
    }
}

// =============================================================================
// PRESETS DE FÁBRICA
// =============================================================================

void AP550Processor::loadFactoryPreset (int index)
{
    const auto presets = ap::factoryPresets();

    if (presets.empty())
        return;

    index = safeIndex (
        index,
        static_cast<int> (presets.size()) - 1);

    const auto& preset =
        presets[static_cast<size_t> (index)];

    setChoice (ap::kLfGain, preset.lfGain);
    setChoice (ap::kLfFreq, preset.lfFreq);
    setBool (ap::kLfBell, preset.lfBell);

    setChoice (ap::kMfGain, preset.mfGain);
    setChoice (ap::kMfFreq, preset.mfFreq);

    setChoice (ap::kHfGain, preset.hfGain);
    setChoice (ap::kHfFreq, preset.hfFreq);
    setBool (ap::kHfBell, preset.hfBell);

    setBool (ap::kEqIn, preset.eqIn);
    setBool (ap::kFilter, preset.filter);
    setBool (ap::kPolarity, preset.polarity);
    setBool (ap::kAnalog, preset.analog);

    if (auto* parameter = apvts.getParameter (ap::kOutput))
    {
        const float normalized =
            parameter->convertTo0to1 (preset.output);

        parameter->beginChangeGesture();
        parameter->setValueNotifyingHost (normalized);
        parameter->endChangeGesture();
    }

    lastKey = -1;
}

// =============================================================================
// IDENTIFICAR PRESET ACTUAL
// =============================================================================

int AP550Processor::getPresetIndex() const
{
    const auto presets = ap::factoryPresets();

    for (int i = 0;
         i < static_cast<int> (presets.size());
         ++i)
    {
        const auto& preset =
            presets[static_cast<size_t> (i)];

        const float currentOutput =
            apvts.getRawParameterValue (ap::kOutput) != nullptr
                ? apvts.getRawParameterValue (ap::kOutput)->load()
                : 0.0f;

        if (idx (ap::kLfGain) == preset.lfGain
            && idx (ap::kLfFreq) == preset.lfFreq
            && flag (ap::kLfBell) == preset.lfBell
            && idx (ap::kMfGain) == preset.mfGain
            && idx (ap::kMfFreq) == preset.mfFreq
            && idx (ap::kHfGain) == preset.hfGain
            && idx (ap::kHfFreq) == preset.hfFreq
            && flag (ap::kHfBell) == preset.hfBell
            && flag (ap::kEqIn) == preset.eqIn
            && flag (ap::kFilter) == preset.filter
            && flag (ap::kPolarity) == preset.polarity
            && flag (ap::kAnalog) == preset.analog
            && std::abs (currentOutput - preset.output) < 0.06f)
        {
            return i;
        }
    }

    return -1;
}

juce::String AP550Processor::getPresetName() const
{
    const int index = getPresetIndex();

    const auto presets = ap::factoryPresets();

    if (index >= 0
        && index < static_cast<int> (presets.size()))
    {
        return presets[static_cast<size_t> (index)].name;
    }

    return "Custom";
}

// =============================================================================
// SIGUIENTE / ANTERIOR PRESET
// =============================================================================

void AP550Processor::stepPreset (int direction)
{
    const auto presets = ap::factoryPresets();

    if (presets.empty())
        return;

    int index = getPresetIndex();

    if (index < 0)
        index = 0;
    else
        index += direction;

    const int count =
        static_cast<int> (presets.size());

    index = (index % count + count) % count;

    loadFactoryPreset (index);
}

// =============================================================================
// CARPETA DE PRESETS DEL USUARIO
// =============================================================================

juce::File AP550Processor::getUserPresetDir() const
{
    auto directory =
        juce::File::getSpecialLocation (
            juce::File::userApplicationDataDirectory);

    directory = directory.getChildFile ("La Pauta")
                         .getChildFile ("AP550 Pro")
                         .getChildFile ("Presets");

    return directory;
}

// =============================================================================
// LISTAR PRESETS DEL USUARIO
// =============================================================================

juce::Array<juce::File>
AP550Processor::listUserPresets() const
{
    juce::Array<juce::File> files;

    const auto directory = getUserPresetDir();

    if (!directory.isDirectory())
        return files;

    directory.findChildFiles (
        files,
        juce::File::findFiles,
        false,
        "*.xml");

    return files;
}

// =============================================================================
// GUARDAR PRESET DEL USUARIO
// =============================================================================

bool AP550Processor::saveUserPreset (
    const juce::String& name)
{
    const juce::String cleanName =
        name.trim()
            .replaceCharacters ("\\/:*?\"<>|", "________");

    if (cleanName.isEmpty())
        return false;

    const auto directory = getUserPresetDir();

    if (!directory.createDirectory())
        return false;

    const auto file =
        directory.getChildFile (cleanName + ".xml");

    auto state = apvts.copyState();

    state.setProperty (
        "userPresetName",
        cleanName,
        nullptr);

    if (auto xml = state.createXml())
        return xml->writeTo (file);

    return false;
}

// =============================================================================
// CARGAR PRESET DEL USUARIO
// =============================================================================

bool AP550Processor::loadUserPreset (
    const juce::File& file)
{
    if (!file.existsAsFile())
        return false;

    const auto xml = juce::XmlDocument::parse (file);

    if (xml == nullptr)
        return false;

    if (!xml->hasTag
