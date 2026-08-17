cmake_minimum_required(VERSION 3.22)

project(Parity VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(modules/JUCE)

juce_add_plugin(Parity
    COMPANY_NAME "Parity"
    COMPANY_WEBSITE "https://github.com/..."
    COMPANY_EMAIL "..."
    PLUGIN_MANUFACTURER_CODE "PtyL"
    PLUGIN_CODE "PtyP"
    FORMATS VST3 AU
    PRODUCT_NAME "Parity"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD TRUE
)

target_sources(Parity PRIVATE
    source/PluginProcessor.cpp
    source/PluginEditor.cpp
    source/Audio/LoudnessAnalyzer.cpp
    source/Audio/ReferencePlayer.cpp
    source/Audio/SpectrumAnalyzer.cpp
    source/UI/ABSwitch.cpp
    source/UI/ParityLookAndFeel.cpp
    source/UI/SpectrumView.cpp
)

target_include_directories(Parity PUBLIC source)

target_compile_definitions(Parity PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
)

target_link_libraries(Parity PRIVATE
    juce::juce_audio_utils
    juce::juce_dsp
    juce::juce_recommended_config_flags
    juce::juce_recommended_lto_flags
    juce::juce_recommended_warning_flags
)

juce_add_console_app(LoudnessSanityTest PRODUCT_NAME "LoudnessSanityTest")

target_sources(LoudnessSanityTest PRIVATE
    Tests/LoudnessSanityTest.cpp
    source/Audio/LoudnessAnalyzer.cpp
)

target_link_libraries(LoudnessSanityTest PRIVATE
    juce::juce_audio_basics
    juce::juce_recommended_config_flags
)
