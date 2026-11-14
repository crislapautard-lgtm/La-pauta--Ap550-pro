#include "PluginEditor.h"
#include "EditorLayout.h"
#include "EditorDraw.h"

using namespace lpui;

//==============================================================================
//  Ratón
//==============================================================================
void AP550Editor::mouseDown (const juce::MouseEvent& e)
{
    const auto p = e.position / scale();
    proc.undo.beginNewTransaction();
    pressedTool = -1;
    dragKind = DragKind::none;

    const auto& tr = toolRects();
    for (int i = 0; i < 8; ++i)
        if (tr[(size_t) i].contains (p)) { pressedTool = i; repaint(); return; }

    if (kNameBar.contains (p)) { proc.toggleSlot(); return; }

    for (int b = 0; b < 3; ++b)
    {
        const auto& bd = bands()[(size_t) b];

        // frecuencia: clic en una de las etiquetas
        for (int f = 0; f < 5; ++f)
        {
            if (freqRect (b, f).contains (p))
            {
                if (auto* fp = proc.apvts.getParameter (bd.freqId))
                {
                    fp->beginChangeGesture();
                    setIdx (bd.freqId, f);
                    fp->endChangeGesture();
                }
                return;
            }
        }

        // ganancia: arrastrar la perilla
        if (p.getDistanceFrom ({ bd.cx, bd.cy }) < 46.f)
        {
            dragKind = DragKind::gain;
            dragBand = b;
            dragParam = proc.apvts.getParameter (bd.gainId);
            dragStartY = p.y;
            dragStartVal = (float) getIdx (bd.gainId);
            dragLastIdx = (int) dragStartVal;
            if (dragParam != nullptr) dragParam->beginChangeGesture();
            return;
        }
    }

    // botones
    if (p.getDistanceFrom (kBtnLF) < 19.f) { toggleBool (ap::kLfBell); return; }
    if (p.getDistanceFrom (kBtnIN) < 19.f) { toggleBool (ap::kEqIn);   return; }
    if (p.getDistanceFrom (kBtnHF) < 19.f) { toggleBool (ap::kHfBell); return; }
    if (p.getDistanceFrom (kPolC)  < 21.f) { toggleBool (ap::kPolarity); return; }

    if (juce::Rectangle<float> (60.f, 36.f).withCentre (kFilterC).contains (p)) { toggleBool (ap::kFilter); return; }
    if (juce::Rectangle<float> (44.f, 76.f).withCentre (kAnalogC).contains (p)) { toggleBool (ap::kAnalog); return; }

    // salida
    if (p.getDistanceFrom (kOutC) < 46.f)
    {
        dragKind = DragKind::output;
        dragParam = proc.apvts.getParameter (ap::kOutput);
        dragStartY = p.y;
        dragStartVal = getVal (ap::kOutput);
        if (dragParam != nullptr) dragParam->beginChangeGesture();
    }
}

void AP550Editor::mouseDrag (const juce::MouseEvent& e)
{
    if (dragKind == DragKind::none || dragParam == nullptr)
        return;

    const auto p = e.position / scale();

    if (dragKind == DragKind::gain)
    {
        const float steps = (dragStartY - p.y) / 14.f;
        const int idx = juce::jlimit (0, ap::kNumGainSteps - 1, (int) std::lround (dragStartVal + steps));
        if (idx != dragLastIdx)
        {
            dragLastIdx = idx;
            setIdx (bands()[(size_t) dragBand].gainId, idx);
        }
    }
    else
    {
        const float k = e.mods.isShiftDown() ? 0.04f : 0.15f;
        float v = juce::jlimit (-18.f, 18.f, dragStartVal + (dragStartY - p.y) * k);
        v = std::round (v * 10.f) / 10.f;
        setVal (ap::kOutput, v);
    }
}

void AP550Editor::mouseUp (const juce::MouseEvent& e)
{
    const auto p = e.position / scale();

    if (pressedTool >= 0)
    {
        const int t = pressedTool;
        pressedTool = -1;
        if (toolRects()[(size_t) t].contains (p))
            runTool (t);
        repaint();
        return;
    }

    if (dragKind != DragKind::none && dragParam != nullptr)
        dragParam->endChangeGesture();

    dragKind = DragKind::none;
    dragParam = nullptr;
}

void AP550Editor::mouseDoubleClick (const juce::MouseEvent& e)
{
    const auto p = e.position / scale();

    for (int b = 0; b < 3; ++b)
    {
        const auto& bd = bands()[(size_t) b];
        if (p.getDistanceFrom ({ bd.cx, bd.cy }) < 46.f)
        {
            setIdx (bd.gainId, 5);       // 0 dB
            return;
        }
    }

    if (p.getDistanceFrom (kOutC) < 46.f)
        setVal (ap::kOutput, 0.f);
}

void AP550Editor::mouseWheelMove (const juce::MouseEvent& e, const juce::MouseWheelDetails& w)
{
    const auto p = e.position / scale();
    if (w.deltaY == 0.f)
        return;
    const int dir = w.deltaY > 0.f ? 1 : -1;

    for (int b = 0; b < 3; ++b)
    {
        const auto& bd = bands()[(size_t) b];
        if (juce::Rectangle<float> (0.f, bd.cy - 70.f, 217.f, 140.f).contains (p))
        {
            const char* id = e.mods.isShiftDown() ? bd.freqId : bd.gainId;
            const int maxIdx = e.mods.isShiftDown() ? 4 : ap::kNumGainSteps - 1;
            if (auto* par = proc.apvts.getParameter (id))
            {
                par->beginChangeGesture();
                setIdx (id, juce::jlimit (0, maxIdx, getIdx (id) + dir));
                par->endChangeGesture();
            }
            return;
        }
    }

    if (p.getDistanceFrom (kOutC) < 50.f)
    {
        if (auto* par = proc.apvts.getParameter (ap::kOutput))
        {
            const float step = e.mods.isShiftDown() ? 0.1f : 0.5f;
            par->beginChangeGesture();
            setVal (ap::kOutput, juce::jlimit (-18.f, 18.f, getVal (ap::kOutput) + dir * step));
            par->endChangeGesture();
        }
    }
}

//==============================================================================
//  Acciones de la barra superior
//==============================================================================
void AP550Editor::runTool (int tool)
{
    switch (tool)
    {
        case 0: proc.undo.undo(); break;
        case 1: proc.undo.redo(); break;
        case 2: proc.stepPreset (-1); break;
        case 3: proc.stepPreset (+1); break;
        case 4: proc.copyToOtherSlot(); break;
        case 5: showLoadMenu(); break;
        case 6: showSaveDialog(); break;
        default:
            juce::AlertWindow::showMessageBoxAsync (
                juce::MessageBoxIconType::InfoIcon,
                "La Pauta AP550 Pro",
                "Perilla: arrastra arriba/abajo para la ganancia (pasos fijos). Doble clic = 0 dB.\n"
                "Frecuencia: clic en el número azul. Rueda del ratón = ganancia, Shift + rueda = frecuencia.\n"
                "LF / HF: alternan shelf y campana.  IN: activa o saca el EQ.\n"
                "FLTR: filtro de agudos.  POL: invierte la fase.  ANALOG: saturación suave.\n"
                "Output: arrastra o usa la rueda; doble clic = 0 dB.\n"
                "Barra de preset: clic para alternar A/B.  A>B copia el ajuste actual al otro lado.",
                "OK", this);
            break;
    }
}

void AP550Editor::showLoadMenu()
{
    juce::PopupMenu menu;
    const auto& list = ap::factoryPresets();
    const int cur = proc.getPresetIndex();

    for (int i = 0; i < (int) list.size(); ++i)
        menu.addItem (1 + i, list[(size_t) i].name, true, i == cur);

    const auto users = proc.listUserPresets();
    if (users.size() > 0)
    {
        juce::PopupMenu sub;
        for (int i = 0; i < users.size(); ++i)
            sub.addItem (1000 + i, users[i].getFileNameWithoutExtension());
        menu.addSeparator();
        menu.addSubMenu ("User", sub);
    }

    const auto area = localAreaToGlobal ((toolRects()[5] * scale()).toNearestInt());
    juce::Component::SafePointer<AP550Editor> safe (this);

    menu.showMenuAsync (juce::PopupMenu::Options().withTargetScreenArea (area),
                        [safe, users] (int result)
                        {
                            if (safe == nullptr || result == 0)
                                return;
                            if (result >= 1000)
                                safe->proc.loadUserPreset (users[result - 1000]);
                            else
                                safe->proc.loadFactoryPreset (result - 1);
                        });
}

void AP550Editor::showSaveDialog()
{
    auto* w = new juce::AlertWindow ("Guardar preset", "Nombre del preset:",
                                     juce::MessageBoxIconType::NoIcon, this);
    w->addTextEditor ("name", proc.getPresetName());
    w->addButton ("Guardar",  1, juce::KeyPress (juce::KeyPress::returnKey));
    w->addButton ("Cancelar", 0, juce::KeyPress (juce::KeyPress::escapeKey));

    juce::Component::SafePointer<AP550Editor> safe (this);
    w->enterModalState (true,
                        juce::ModalCallbackFunction::create ([safe, w] (int result)
                        {
                            if (result == 1 && safe != nullptr)
                            {
                                const auto name = w->getTextEditorContents ("name").trim();
                                if (name.isNotEmpty())
                                    safe->proc.saveUserPreset (name);
                            }
                        }),
                        true);
}
