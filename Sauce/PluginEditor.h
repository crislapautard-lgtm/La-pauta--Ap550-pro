#pragma once

#include "PluginProcessor.h"

//==============================================================================
//  Interfaz de La Pauta AP550 Pro
//  Mismo diseño y tamaño del original (438 x 830), placa en gris plateado oscuro.
//  Todo se dibuja a mano: no usa imágenes externas.
//==============================================================================
class AP550Editor : public juce::AudioProcessorEditor,
                    private juce::Timer
{
public:
    explicit AP550Editor (AP550Processor&);
    ~AP550Editor() override;

    void paint (juce::Graphics&) override;

    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    void mouseDoubleClick (const juce::MouseEvent&) override;
    void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails&) override;

private:
    enum class DragKind { none, gain, output };

    void timerCallback() override;
    void buildBackground();
    float scale() const { return (float) getWidth() / (float) kW; }

    // Lectura / escritura de parámetros
    int   getIdx (const char* id) const;
    bool  getBool (const char* id) const;
    float getVal (const char* id) const;
    void  setIdx (const char* id, int index);
    void  setVal (const char* id, float value);
    void  toggleBool (const char* id);

    juce::String stateKey() const;

    // Dibujo
    void drawToolButton (juce::Graphics&, juce::Rectangle<float>, int kind, bool pressed) const;
    void drawMeterNeedle (juce::Graphics&, juce::Rectangle<float> face, float db) const;

    // Acciones
    void runTool (int tool);
    void showLoadMenu();
    void showSaveDialog();

    static constexpr int kW = 438;
    static constexpr int kH = 830;

    AP550Processor& proc;
    juce::Image bg;

    DragKind dragKind = DragKind::none;
    int dragBand = -1;
    float dragStartY = 0.f, dragStartVal = 0.f;
    int dragLastIdx = -1;
    juce::RangedAudioParameter* dragParam = nullptr;

    int pressedTool = -1;

    float needleL = -60.f, needleR = -60.f;
    juce::String lastKey;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AP550Editor)
};
