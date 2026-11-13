
#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

class AP550Editor : public juce::AudioProcessorEditor,
                    public juce::Timer
{
public:
    AP550Editor (AP550Processor& p);
    ~AP550Editor() override;

    void paint (juce::Graphics& g) override;

    void mouseDown (const juce::MouseEvent& e) override;
    void mouseDrag (const juce::MouseEvent& e) override;
    void mouseUp (const juce::MouseEvent& e) override;
    void mouseDoubleClick (const juce::MouseEvent& e) override;
    void mouseWheelMove (const juce::MouseEvent& e,
                         const juce::MouseWheelDetails& w) override;

    void timerCallback() override;

private:
    static constexpr int kW = 438;
    static constexpr int kH = 830;

    AP550Processor& proc;

    juce::Image bg;

    float needleL = -60.0f;
    float needleR = -60.0f;

    juce::String lastKey;

    int pressedTool = -1;

    enum class DragKind
    {
        none,
        gain,
        output
    };

    DragKind dragKind = DragKind::none;

    int dragBand = 0;

    juce::RangedAudioParameter* dragParam = nullptr;

    float dragStartY = 0.0f;
    float dragStartVal = 0.0f;

    int dragLastIdx = 0;

    float scale() const
    {
        return static_cast<float> (getWidth())
             / static_cast<float> (kW);
    }

    void buildBackground();

    int getIdx (const char*) const;
    bool getBool (const char*) const;
    float getVal (const char*) const;

    void setIdx (const char*, int);
    void setVal (const char*, float);
    void toggleBool (const char*);

    juce::String stateKey() const;

    void drawToolButton (
        juce::Graphics&,
        juce::Rectangle<float>,
        int kind,
        bool pressed) const;

    void drawMeterNeedle (
        juce::Graphics&,
        juce::Rectangle<float>,
        float db) const;

    void runTool (int tool);
    void showLoadMenu();
    void showSaveDialog();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AP550Editor)
};
