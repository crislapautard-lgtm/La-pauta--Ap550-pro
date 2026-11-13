#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

//==============================================================================
//  PluginEditor.h - Creado 100% en base a tu PluginEditor.cpp real
//  AP550 Pro - 438 x 830
//==============================================================================
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
    void mouseWheelMove (const juce::MouseEvent& e, const juce::MouseWheelDetails& w) override;

    void timerCallback() override;

private:
    // Estos dos los usa tu .cpp en el constructor y en paint()
    // Si no estan aqui, da error de kW / kH no declarado
    static constexpr int kW = 438;
    static constexpr int kH = 830;

    AP550Processor& proc;

    juce::Image bg;

    float needleL = -60.0f;
    float needleR = -60.0f;
    juce::String lastKey;
    int pressedTool = -1;

    enum class DragKind { none, gain, output };
    DragKind dragKind = DragKind::none;
    int dragBand = 0;
    juce::RangedAudioParameter* dragParam = nullptr;
    float dragStartY = 0.0f;
    float dragStartVal = 0.0f;
    int dragLastIdx = 0;

    float scale() const
    {
        return (float) getWidth() / (float) kW;
    }

    void buildBackground();

    int getIdx (const char* id) const;
    bool getBool (const char* id) const;
    float getVal (const char* id) const;
    void setIdx (const char* id, int index);
    void setVal (const char* id, float value);
    void toggleBool (const char* id);

    juce::String stateKey() const;

    // OJO: Aqui estaba el error.
    // En tu .cpp la funcion se llama drawToolButton, NO drawTool.
    // Por eso el compilador decia: 'drawTool' is not a member of 'AP550Editor'
    // Ahora los dos archivos dicen lo mismo: drawToolButton
    void drawToolButton (juce::Graphics& g, juce::Rectangle<float> r, int kind, bool pressed) const;
    void drawMeterNeedle (juce::Graphics& g, juce::Rectangle<float> face, float db) const;

    void runTool (int tool);
    void showLoadMenu();
    void showSaveDialog();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AP550Editor)
};
