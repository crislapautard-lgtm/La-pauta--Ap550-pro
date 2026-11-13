#include "PluginProcessor.h"
#include "PluginEditor.h"

AP550Processor::AP550Processor()
    : AudioProcessor (BusesProperties().withInput ("Input", juce::AudioChannelSet::stereo(), true)
                                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, &undo, "Parameters", createParameterLayout())
{
    slotStates[0] = apvts.copyState();
    slotStates[1] = apvts.copyState();
}

AP550Processor::~AP550Processor() {}

const juce::String AP550Processor::getName() const { return "La Pauta AP550 Pro"; }
bool AP550Processor::acceptsMidi() const { return false; }
bool AP550Processor::producesMidi() const { return false; }
bool AP550Processor::isMidiEffect() const { return false; }
double AP550Processor::getTailLengthSeconds() const { return 0.0; }
int AP550Processor::getNumPrograms() { return 1; }
int AP550Processor::getCurrentProgram() { return 0; }
void AP550Processor::setCurrentProgram (int) {}
const juce::String AP550Processor::getProgramName (int) { return {}; }
void AP550Processor::changeProgramName (int, const juce::String&) {}
bool AP550Processor::hasEditor() const { return true; }
juce::AudioProcessorEditor* AP550Processor::createEditor() { return new AP550Editor (*this); }

bool AP550Processor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
        && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
    return true;
}

juce::AudioProcessorValueTreeState::ParameterLayout AP550Processor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;
    auto gainChoices = []()
    {
        juce::StringArray s;
        s.add ("-12"); s.add ("-9"); s.add ("-6"); s.add ("-4"); s.add ("-2");
        s.add ("0"); s.add ("+2"); s.add ("+4"); s.add ("+6"); s.add ("+9"); s.add ("+12");
        return s;
    };
    auto freqLF = []() { juce::StringArray s; s.add ("50"); s.add ("100"); s.add ("200"); s.add ("300"); s.add ("400"); return s; };
    auto freqMF = []() { juce::StringArray s; s.add ("0.4"); s.add ("0.8"); s.add ("1.5"); s.add ("3"); s.add ("5"); return s; };
    auto freqHF = []() { juce::StringArray s; s.add ("5"); s.add ("7.5"); s.add ("10"); s.add ("12.5"); s.add ("15"); return s; };
    params.push_back (std::make_unique<juce::AudioParameterChoice> (juce::ParameterID (ap::kLfGain, 1), "LF Gain", gainChoices(), 5));
    params.push_back (std::make_unique<juce::AudioParameterChoice> (juce::ParameterID (ap::kLfFreq, 1), "LF Freq", freqLF(), 2));
    params.push_back (std::make_unique<juce::AudioParameterBool> (juce::ParameterID (ap::kLfBell, 1), "LF Bell", false));
    params.push_back (std::make_unique<juce::AudioParameterChoice> (juce::ParameterID (ap::kMfGain, 1), "MF Gain", gainChoices(), 5));
    params.push_back (std::make_unique<juce::AudioParameterChoice> (juce::ParameterID (ap::kMfFreq, 1), "MF Freq", freqMF(), 2));
    params.push_back (std::make_unique<juce::AudioParameterChoice> (juce::ParameterID (ap::kHfGain, 1), "HF Gain", gainChoices(), 5));
    params.push_back (std::make_unique<juce::AudioParameterChoice> (juce::ParameterID (ap::kHfFreq, 1), "HF Freq", freqHF(), 2));
    params.push_back (std::make_unique<juce::AudioParameterBool> (juce::ParameterID (ap::kHfBell, 1), "HF Bell", false));
    params.push_back (std::make_unique<juce::AudioParameterBool> (juce::ParameterID (ap::kEqIn, 1), "EQ In", true));
    params.push_back (std::make_unique<juce::AudioParameterBool> (juce::ParameterID (ap::kFilter, 1), "Filter", false));
    params.push_back (std::make_unique<juce::AudioParameterBool> (juce::ParameterID (ap::kPolarity, 1), "Polarity", false));
    params.push_back (std::make_unique<juce::AudioParameterBool> (juce::ParameterID (ap::kAnalog, 1), "Analog", false));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID (ap::kOutput, 1), "Output",
        juce::NormalisableRange<float> (-18.0f, 18.0f, 0.1f), 0.0f));
    return { params.begin(), params.end() };
}

void AP550Processor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    juce::ignoreUnused (samplesPerBlock);
    lastSampleRate = sampleRate;
    juce::dsp::ProcessSpec spec { sampleRate, (juce::uint32) samplesPerBlock, 2 };
    for (int ch = 0; ch < 2; ++ch)
    {
        lf[ch].prepare (spec);
        mf[ch].prepare (spec);
        hf[ch].prepare (spec);
        lp[ch].prepare (spec);
        lf[ch].reset();
        mf[ch].reset();
        hf[ch].reset();
        lp[ch].reset();
    }
    updateFilters();
}

void AP550Processor::releaseResources() {}

void AP550Processor::updateFilters()
{
    auto getIdx = [this] (const char* id) { return (int) std::lround (apvts.getRawParameterValue (id)->load()); };
    auto getBool = [this] (const char* id) { return apvts.getRawParameterValue (id)->load() > 0.5f; };
    static const float gainDb[11] = { -12, -9, -6, -4, -2, 0, 2, 4, 6, 9, 12 };
    static const float lfFreq[5] = { 50, 100, 200, 300, 400 };
    static const float mfFreq[5] = { 400, 800, 1500, 3000, 5000 };
    static const float hfFreq[5] = { 5000, 7500, 10000, 12500, 15000 };
    const float lfG = gainDb[getIdx (ap::kLfGain)];
    const float mfG = gainDb[getIdx (ap::kMfGain)];
    const float hfG = gainDb[getIdx (ap::kHfGain)];
    const bool lfBell = getBool (ap::kLfBell);
    const bool hfBell = getBool (ap::kHfBell);
    for (int ch = 0; ch < 2; ++ch)
    {
        if (lfBell)
            lf[ch].coefficients = juce::dsp::IIR::Coefficients<float>::makePeakFilter (lastSampleRate, lfFreq[getIdx (ap::kLfFreq)], 1.0f, juce::Decibels::decibelsToGain (lfG));
        else
            lf[ch].coefficients = juce::dsp::IIR::Coefficients<float>::makeLowShelf (lastSampleRate, lfFreq[getIdx (ap::kLfFreq)], 0.7f, juce::Decibels::decibelsToGain (lfG));
        mf[ch].coefficients = juce::dsp::IIR::Coefficients<float>::makePeakFilter (lastSampleRate, mfFreq[getIdx (ap::kMfFreq)], 1.2f, juce::Decibels::decibelsToGain (mfG));
        if (hfBell)
            hf[ch].coefficients = juce::dsp::IIR::Coefficients<float>::makePeakFilter (lastSampleRate, hfFreq[getIdx (ap::kHfFreq)], 1.0f, juce::Decibels::decibelsToGain (hfG));
        else
            hf[ch].coefficients = juce::dsp::IIR::Coefficients<float>::makeHighShelf (lastSampleRate, hfFreq[getIdx (ap::kHfFreq)], 0.7f, juce::Decibels::decibelsToGain (hfG));
        lp[ch].coefficients = juce::dsp::IIR::Coefficients<float>::makeLowPass (lastSampleRate, 18000.0f, 0.7f);
    }
}

void AP550Processor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());
    updateFilters();
    const bool eqIn = apvts.getRawParameterValue (ap::kEqIn)->load() > 0.5f;
    const bool filterOn = apvts.getRawParameterValue (ap::kFilter)->load() > 0.5f;
    const bool polarity = apvts.getRawParameterValue (ap::kPolarity)->load() > 0.5f;
    const bool analog = apvts.getRawParameterValue (ap::kAnalog)->load() > 0.5f;
    const float outGain = juce::Decibels::decibelsToGain (apvts.getRawParameterValue (ap::kOutput)->load());
    float peakL = 0.0f, peakR = 0.0f;
    for (int channel = 0; channel < totalNumInputChannels; ++channel)
    {
        auto* data = buffer.getWritePointer (channel);
        for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
        {
            float x = data[sample];
            if (polarity) x = -x;
            if (eqIn)
            {
                x = lf[channel].processSample (x);
                x = mf[channel].processSample (x);
                x = hf[channel].processSample (x);
            }
            if (filterOn)
                x = lp[channel].processSample (x);
            if (analog)
                x = std::tanh (x * 1.1f);
            x *= outGain;
            data[sample] = x;
            float a = std::abs (x);
            if (channel == 0) peakL = juce::jmax (peakL, a);
            else peakR = juce::jmax (peakR, a);
        }
    }
    auto toDb = [] (float p) { return p > 0.00001f ? juce::jlimit (-60.0f, 3.0f, juce::Decibels::gainToDecibels (p)) : -60.0f; };
    vuL.store (toDb (peakL));
    vuR.store (toDb (peakR));
}

void AP550Processor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    copyXmlToBinary (*xml, destData);
}

void AP550Processor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xml (getXmlFromBinary (data, sizeInBytes));
    if (xml == nullptr || ! xml->hasTagName ("LaPautaAP550Pro"))
    {
        if (xml != nullptr)
        {
            auto tree = juce::ValueTree::fromXml (*xml);
            if (tree.isValid())
                apvts.replaceState (tree);
        }
        return;
    }
    auto tree = juce::ValueTree::fromXml (*xml);
    if (tree.isValid())
        apvts.replaceState (tree);
}

juce::String AP550Processor::getPresetName() const { return presetName; }
int AP550Processor::getPresetIndex() const { return presetIndex; }
int AP550Processor::getActiveSlot() const { return activeSlot; }

void AP550Processor::saveSlot (int slot)
{
    if (slot >= 0 && slot < 2)
        slotStates[(size_t) slot] = apvts.copyState();
}
void AP550Processor::loadSlot (int slot)
{
    if (slot >= 0 && slot < 2)
        apvts.replaceState (slotStates[(size_t) slot]);
}
void AP550Processor::toggleSlot()
{
    saveSlot (activeSlot);
    activeSlot = (activeSlot == 0 ? 1 : 0);
    loadSlot (activeSlot);
}
void AP550Processor::copyToOtherSlot()
{
    int other = (activeSlot == 0 ? 1 : 0);
    slotStates[(size_t) other] = apvts.copyState();
}
void AP550Processor::applyPresetValues (const ap::Preset& p)
{
    auto setChoice = [this] (const char* id, float idx)
    {
        if (auto* par = apvts.getParameter (id))
            par->setValueNotifyingHost (par->convertTo0to1 (idx));
    };
    auto setBool = [this] (const char* id, float v)
    {
        if (auto* par = apvts.getParameter (id))
            par->setValueNotifyingHost (v > 0.5f ? 1.0f : 0.0f);
    };
    auto setFloat = [this] (const char* id, float v)
    {
        if (auto* par = apvts.getParameter (id))
            par->setValueNotifyingHost (par->convertTo0to1 (v));
    };
    setChoice (ap::kLfGain, p.values[0]); setChoice (ap::kLfFreq, p.values[1]); setBool (ap::kLfBell, p.values[2]);
    setChoice (ap::kMfGain, p.values[3]); setChoice (ap::kMfFreq, p.values[4]);
    setChoice (ap::kHfGain, p.values[5]); setChoice (ap::kHfFreq, p.values[6]); setBool (ap::kHfBell, p.values[7]);
    setBool (ap::kEqIn, p.values[8]); setBool (ap::kFilter, p.values[9]); setBool (ap::kPolarity, p.values[10]);
    setBool (ap::kAnalog, p.values[11]); setFloat (ap::kOutput, p.values[12]);
    presetName = p.name;
}
void AP550Processor::loadFactoryPreset (int index)
{
    const auto& list = ap::factoryPresets();
    if (index < 0 || index >= (int) list.size())
        return;
    presetIndex = index;
    applyPresetValues (list[(size_t) index]);
}
void AP550Processor::stepPreset (int dir)
{
    const auto& list = ap::factoryPresets();
    int next = presetIndex + dir;
    if (next < 0) next = (int) list.size() - 1;
    if (next >= (int) list.size()) next = 0;
    loadFactoryPreset (next);
}
juce::File AP550Processor::getUserPresetFolder() const
{
    auto folder = juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory)
        .getChildFile ("La Pauta").getChildFile ("AP550 Pro").getChildFile ("Presets");
    folder.createDirectory();
    return folder;
}
juce::Array<juce::File> AP550Processor::listUserPresets() const
{
    return getUserPresetFolder().findChildFiles (juce::File::findFiles, false, "*.xml");
}
void AP550Processor::saveUserPreset (const juce::String& name)
{
    auto file = getUserPresetFolder().getChildFile (name + ".xml");
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    if (xml != nullptr)
        xml->writeTo (file);
    presetName = name;
}
void AP550Processor::loadUserPreset (const juce::File& file)
{
    std::unique_ptr<juce::XmlElement> xml (juce::XmlDocument::parse (file));
    if (xml == nullptr)
        return;
    auto tree = juce::ValueTree::fromXml (*xml);
    if (tree.isValid())
    {
        apvts.replaceState (tree);
        presetName = file.getFileNameWithoutExtension();
        presetIndex = -1;
    }
}
