#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
AP550Processor::AP550Processor()
    : AudioProcessor (BusesProperties()
                          .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, &undo, "PARAMS", createLayout())
{
    apvts.state.setProperty ("presetName", "Init", nullptr);
    apvts.state.setProperty ("presetIndex", 0, nullptr);
}

juce::AudioProcessorValueTreeState::ParameterLayout AP550Processor::createLayout()
{
    using namespace juce;
    AudioProcessorValueTreeState::ParameterLayout l;

    auto choice = [&] (const char* id, const String& name, const StringArray& items, int def)
    {
        l.add (std::make_unique<AudioParameterChoice> (ParameterID { id, 1 }, name, items, def));
    };
    auto toggle = [&] (const char* id, const String& name, bool def)
    {
        l.add (std::make_unique<AudioParameterBool> (ParameterID { id, 1 }, name, def));
    };

    choice (ap::kHfGain, "HF Gain", ap::gainNames(),    5);
    choice (ap::kHfFreq, "HF Freq", ap::hfFreqNames(),  2);
    toggle (ap::kHfBell, "HF Bell", false);

    choice (ap::kMfGain, "MF Gain", ap::gainNames(),    5);
    choice (ap::kMfFreq, "MF Freq", ap::mfFreqNames(),  2);

    choice (ap::kLfGain, "LF Gain", ap::gainNames(),    5);
    choice (ap::kLfFreq, "LF Freq", ap::lfFreqNames(),  1);
    toggle (ap::kLfBell, "LF Bell", false);

    toggle (ap::kEqIn,     "EQ In",    true);
    toggle (ap::kFilter,   "Filter",   false);
    toggle (ap::kPolarity, "Polarity", false);
    toggle (ap::kAnalog,   "Analog",   true);

    l.add (std::make_unique<AudioParameterFloat> (ParameterID { ap::kOutput, 1 }, "Output",
                                                  NormalisableRange<float> (-18.f, 18.f, 0.1f), 0.f,
                                                  AudioParameterFloatAttributes().withLabel ("dB")));
    return l;
}

//==============================================================================
int AP550Processor::idx (const char* id) const
{
    return (int) std::lround (apvts.getRawParameterValue (id)->load());
}

bool AP550Processor::flag (const char* id) const
{
    return apvts.getRawParameterValue (id)->load() > 0.5f;
}

void AP550Processor::setChoice (const char* id, int index)
{
    if (auto* p = apvts.getParameter (id))
    {
        p->beginChangeGesture();
        p->setValueNotifyingHost (p->convertTo0to1 ((float) index));
        p->endChangeGesture();
    }
}

void AP550Processor::setBool (const char* id, bool value)
{
    if (auto* p = apvts.getParameter (id))
    {
        p->beginChangeGesture();
        p->setValueNotifyingHost (value ? 1.0f : 0.0f);
        p->endChangeGesture();
    }
}

//==============================================================================
void AP550Processor::prepareToPlay (double sr, int samplesPerBlock)
{
    sampleRate = sr;
    const float fs = (float) sr;

    // Los filtros deben recibir coeficientes de 2º orden antes de resetear,
    // así el estado interno queda dimensionado correctamente.
    const float lpFreq = juce::jmin (16000.f, fs * 0.45f);
    for (size_t c = 0; c < 2; ++c)
    {
        lf[c].coefficients = Coeffs::makeLowShelf  (sr, 100.f,  0.6f, 1.0f);
        mf[c].coefficients = Coeffs::makePeakFilter (sr, 1500.f, 0.7f, 1.0f);
        hf[c].coefficients = Coeffs::makeHighShelf (sr, 10000.f, 0.6f, 1.0f);
        lp[c].coefficients = Coeffs::makeLowPass   (sr, lpFreq);
        lf[c].reset(); mf[c].reset(); hf[c].reset(); lp[c].reset();
    }

    juce::dsp::ProcessSpec spec { sr, (juce::uint32) samplesPerBlock, 2 };
    outGain.prepare (spec);
    outGain.setRampDurationSeconds (0.03);

    vuState = { 0.f, 0.f };
    lastKey = -1;
}

bool AP550Processor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto out = layouts.getMainOutputChannelSet();
    if (out != juce::AudioChannelSet::mono() && out != juce::AudioChannelSet::stereo())
        return false;
    return out == layouts.getMainInputChannelSet();
}

void AP550Processor::updateFilters()
{
    const int lfG = idx (ap::kLfGain), lfF = idx (ap::kLfFreq), lfB = flag (ap::kLfBell) ? 1 : 0;
    const int mfG = idx (ap::kMfGain), mfF = idx (ap::kMfFreq);
    const int hfG = idx (ap::kHfGain), hfF = idx (ap::kHfFreq), hfB = flag (ap::kHfBell) ? 1 : 0;

    const int key = (((((((lfG * 5 + lfF) * 2 + lfB) * 11 + mfG) * 5 + mfF) * 11 + hfG) * 5 + hfF) * 2 + hfB);
    if (key == lastKey)
        return;
    lastKey = key;

    const double sr = sampleRate;
    const float maxF = (float) sr * 0.45f;

    // Q proporcional: más estrecho cuanta más ganancia (como el 550A original)
    auto bell = [&] (float f, float gdb)
    {
        const float q = 0.6f + 0.09f * std::abs (gdb);
        return Coeffs::makePeakFilter (sr, juce::jmin (f, maxF), q, juce::Decibels::decibelsToGain (gdb));
    };

    const float lfGdb = ap::kGainSteps[juce::jlimit (0, 10, lfG)];
    const float mfGdb = ap::kGainSteps[juce::jlimit (0, 10, mfG)];
    const float hfGdb = ap::kGainSteps[juce::jlimit (0, 10, hfG)];
    const float lfFreq = ap::kLfFreqs[juce::jlimit (0, 4, lfF)];
    const float mfFreq = ap::kMfFreqs[juce::jlimit (0, 4, mfF)];
    const float hfFreq = ap::kHfFreqs[juce::jlimit (0, 4, hfF)];

    auto lfC = lfB ? bell (lfFreq, lfGdb)
                   : Coeffs::makeLowShelf (sr, lfFreq, 0.6f, juce::Decibels::decibelsToGain (lfGdb));
    auto mfC = bell (mfFreq, mfGdb);
    auto hfC = hfB ? bell (hfFreq, hfGdb)
                   : Coeffs::makeHighShelf (sr, juce::jmin (hfFreq, maxF), 0.6f, juce::Decibels::decibelsToGain (hfGdb));

    for (size_t c = 0; c < 2; ++c)
    {
        lf[c].coefficients = lfC;
        mf[c].coefficients = mfC;
        hf[c].coefficients = hfC;
    }
}

void AP550Processor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const int nCh = juce::jmin (buffer.getNumChannels(), 2);
    const int n   = buffer.getNumSamples();
    if (nCh == 0 || n == 0)
        return;

    updateFilters();

    const bool eqIn   = flag (ap::kEqIn);
    const bool filt   = flag (ap::kFilter);
    const bool analog = flag (ap::kAnalog);
    const bool pol    = flag (ap::kPolarity);

    constexpr float drive = 1.25f;   // saturación suave del modo ANALOG

    for (int ch = 0; ch < nCh; ++ch)
    {
        auto* d = buffer.getWritePointer (ch);
        auto& f1 = lf[(size_t) ch];
        auto& f2 = mf[(size_t) ch];
        auto& f3 = hf[(size_t) ch];
        auto& f4 = lp[(size_t) ch];

        for (int i = 0; i < n; ++i)
        {
            float x = d[i];

            if (eqIn)
            {
                x = f1.processSample (x);
                x = f2.processSample (x);
                x = f3.processSample (x);
                if (filt)
                    x = f4.processSample (x);
            }

            if (analog)
                x = std::tanh (x * drive) / drive;

            if (pol)
                x = -x;

            d[i] = x;
        }
    }

    outGain.setGainDecibels (apvts.getRawParameterValue (ap::kOutput)->load());
    juce::dsp::AudioBlock<float> block (buffer);
    juce::dsp::ProcessContextReplacing<float> ctx (block);
    outGain.process (ctx);

    // Medidores VU (RMS con ~300 ms de integración)
    const float alpha = 1.0f - std::exp (-(float) n / (float) (sampleRate * 0.3));
    for (int ch = 0; ch < 2; ++ch)
    {
        const int src = juce::jmin (ch, nCh - 1);
        const float rms = buffer.getRMSLevel (src, 0, n);
        vuState[(size_t) ch] += (rms * rms - vuState[(size_t) ch]) * alpha;
    }
    vuL.store (10.0f * std::log10 (vuState[0] + 1.0e-9f));
    vuR.store (10.0f * std::log10 (vuState[1] + 1.0e-9f));
}

//==============================================================================
juce::AudioProcessorEditor* AP550Processor::createEditor()
{
    return new AP550Editor (*this);
}

void AP550Processor::getStateInformation (juce::MemoryBlock& dest)
{
    if (auto xml = apvts.copyState().createXml())
        copyXmlToBinary (*xml, dest);
}

void AP550Processor::setStateInformation (const void* data, int size)
{
    if (auto xml = getXmlFromBinary (data, size))
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

//==============================================================================
//  Presets
//==============================================================================
juce::String AP550Processor::getPresetName() const
{
    return apvts.state.getProperty ("presetName", "Init").toString();
}

int AP550Processor::getPresetIndex() const
{
    return (int) apvts.state.getProperty ("presetIndex", -1);
}

void AP550Processor::loadFactoryPreset (int index)
{
    const auto& list = ap::factoryPresets();
    if (index < 0 || index >= (int) list.size())
        return;

    const auto& p = list[(size_t) index];
    undo.beginNewTransaction ("Preset");

    setChoice (ap::kLfGain, ap::gainToIndex (p.lfG));
    setChoice (ap::kLfFreq, p.lfF);
    setBool   (ap::kLfBell, p.lfBell);

    setChoice (ap::kMfGain, ap::gainToIndex (p.mfG));
    setChoice (ap::kMfFreq, p.mfF);

    setChoice (ap::kHfGain, ap::gainToIndex (p.hfG));
    setChoice (ap::kHfFreq, p.hfF);
    setBool   (ap::kHfBell, p.hfBell);

    setBool (ap::kFilter, p.filter);
    setBool (ap::kEqIn, true);

    apvts.state.setProperty ("presetName", p.name, nullptr);
    apvts.state.setProperty ("presetIndex", index, nullptr);
}

void AP550Processor::stepPreset (int direction)
{
    const int n = (int) ap::factoryPresets().size();
    const int cur = getPresetIndex();
    const int next = cur < 0 ? (direction > 0 ? 0 : n - 1)
                             : (cur + direction + n) % n;
    loadFactoryPreset (next);
}

juce::File AP550Processor::getUserPresetDir() const
{
    auto base = juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory);
   #if JUCE_MAC
    base = base.getChildFile ("Application Support");
   #endif
    return base.getChildFile ("La Pauta").getChildFile ("AP550 Pro").getChildFile ("Presets");
}

juce::Array<juce::File> AP550Processor::listUserPresets() const
{
    auto files = getUserPresetDir().findChildFiles (juce::File::findFiles, false, "*.lpap");
    std::sort (files.begin(), files.end(), [] (const juce::File& a, const juce::File& b)
    {
        return a.getFileName().compareIgnoreCase (b.getFileName()) < 0;
    });
    return files;
}

bool AP550Processor::saveUserPreset (const juce::String& name)
{
    auto dir = getUserPresetDir();
    if (! dir.createDirectory().wasOk())
        return false;

    auto file = dir.getChildFile (juce::File::createLegalFileName (name)).withFileExtension ("lpap");

    apvts.state.setProperty ("presetName", name, nullptr);
    apvts.state.setProperty ("presetIndex", -1, nullptr);

    if (auto xml = apvts.copyState().createXml())
        return xml->writeTo (file);

    return false;
}

bool AP550Processor::loadUserPreset (const juce::File& file)
{
    if (auto xml = juce::XmlDocument::parse (file))
    {
        if (xml->hasTagName (apvts.state.getType()))
        {
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
            apvts.state.setProperty ("presetName", file.getFileNameWithoutExtension(), nullptr);
            apvts.state.setProperty ("presetIndex", -1, nullptr);
            return true;
        }
    }
    return false;
}

//==============================================================================
//  A/B
//==============================================================================
void AP550Processor::toggleSlot()
{
    slotState[activeSlot] = apvts.copyState();
    activeSlot = 1 - activeSlot;

    if (slotState[activeSlot].isValid())
        apvts.replaceState (slotState[activeSlot].createCopy());
}

void AP550Processor::copyToOtherSlot()
{
    slotState[1 - activeSlot] = apvts.copyState();
}

//==============================================================================
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new AP550Processor();
}
