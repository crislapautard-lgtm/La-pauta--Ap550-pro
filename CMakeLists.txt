cmake_minimum_required(VERSION 3.22)

project(LaPautaAP550Pro VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)
FetchContent_Declare(JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG        8.0.6
    GIT_SHALLOW    ON)
FetchContent_MakeAvailable(JUCE)

set(LP_COPY TRUE)
if(DEFINED ENV{CI})
    set(LP_COPY FALSE)
endif()

juce_add_plugin(LaPautaAP550Pro
    COMPANY_NAME               "La Pauta"
    BUNDLE_ID                  com.lapauta.ap550pro
    PLUGIN_MANUFACTURER_CODE   LPau
    PLUGIN_CODE                Ap55
    FORMATS                    VST3 Standalone
    PRODUCT_NAME               "La Pauta AP550 Pro"
    IS_SYNTH                   FALSE
    NEEDS_MIDI_INPUT           FALSE
    NEEDS_MIDI_OUTPUT          FALSE
    COPY_PLUGIN_AFTER_BUILD    ${LP_COPY}
    VST3_CATEGORIES            Fx EQ)

target_sources(LaPautaAP550Pro PRIVATE
    Sauce/Params.h
    Sauce/PluginProcessor.h
    Sauce/PluginProcessor.cpp
    Sauce/PluginEditor.h
    Sauce/PluginEditor.cpp
    Sauce/PluginEditorPaint.cpp
    Sauce/PluginEditorMouse.cpp
    Sauce/EditorLayout.h
    Sauce/EditorDraw.h)

target_compile_definitions(LaPautaAP550Pro PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0)

target_link_libraries(LaPautaAP550Pro
    PRIVATE
        juce::juce_audio_utils
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_warning_flags)
