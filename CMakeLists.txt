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

juce_add_binary_data(ParityAssets SOURCES
    assets/icons/audio-lines.svg
    assets/icons/check-dark.svg
    assets/icons/check-light.svg
    assets/icons/folder-open.svg
    assets/icons/pause.svg
    assets/icons/settings.svg
)

target_sources(Parity PRIVATE
    source/PluginProcessor.cpp
    source/PluginEditor.cpp
    source/Audio/LoudnessAnalyzer.cpp
    source/Audio/MasterChain.cpp
    source/Audio/ReferencePlayer.cpp
    source/Audio/SpectrumAnalyzer.cpp
    source/Audio/StereoAnalyzer.cpp
    source/UI/ABSwitch.cpp
    source/UI/ComparisonBar.cpp
    source/UI/ParityLookAndFeel.cpp
    source/UI/SpectrumView.cpp
    source/UI/VerticalFader.cpp
)

target_include_directories(Parity PUBLIC source)

target_compile_definitions(Parity PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
)

target_link_libraries(Parity PRIVATE
    ParityAssets
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

juce_add_console_app(MasterChainTest PRODUCT_NAME "MasterChainTest")

target_sources(MasterChainTest PRIVATE
    Tests/MasterChainTest.cpp
    source/Audio/MasterChain.cpp
)

target_link_libraries(MasterChainTest PRIVATE
    juce::juce_audio_basics
    juce::juce_recommended_config_flags
)

# Temporary harness for offscreen editor screenshots.
juce_add_console_app(UICaptureTest PRODUCT_NAME "UICaptureTest")

target_sources(UICaptureTest PRIVATE
    Tests/UICaptureTest.cpp
    source/PluginProcessor.cpp
    source/PluginEditor.cpp
    source/Audio/LoudnessAnalyzer.cpp
    source/Audio/MasterChain.cpp
    source/Audio/ReferencePlayer.cpp
    source/Audio/SpectrumAnalyzer.cpp
    source/Audio/StereoAnalyzer.cpp
    source/UI/ABSwitch.cpp
    source/UI/ComparisonBar.cpp
    source/UI/ParityLookAndFeel.cpp
    source/UI/SpectrumView.cpp
    source/UI/VerticalFader.cpp
)

target_include_directories(UICaptureTest PRIVATE source)

target_compile_definitions(UICaptureTest PRIVATE
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_MODAL_LOOPS_PERMITTED=1
    JucePlugin_Name="Parity"
    JucePlugin_Desc="Parity"
    JucePlugin_Manufacturer="Parity"
    JucePlugin_ManufacturerWebsite="https://github.com/..."
    JucePlugin_ManufacturerEmail="..."
    JucePlugin_ManufacturerCode=0x5074794c
    JucePlugin_PluginCode=0x50747950
    JucePlugin_IsSynth=0
    JucePlugin_WantsMidiInput=0
    JucePlugin_ProducesMidiOutput=0
    JucePlugin_IsMidiEffect=0
    JucePlugin_EditorRequiresKeyboardFocus=0
    JucePlugin_VSTNumMidiInputs=16
    JucePlugin_VSTNumMidiOutputs=16
    JucePlugin_Version=0.1.0
    JucePlugin_VersionCode=0x100
    JucePlugin_VersionString="0.1.0"
    JucePlugin_VSTUniqueID=JucePlugin_PluginCode
    JucePlugin_VSTCategory=kPlugCategEffect
    JucePlugin_Vst3Category="Fx"
    JucePlugin_AUMainType="'aufx'"
    JucePlugin_AUSubType=JucePlugin_PluginCode
    JucePlugin_AUExportPrefix=ParityAU
    JucePlugin_AUExportPrefixQuoted="ParityAU"
    JucePlugin_AUManufacturerCode=JucePlugin_ManufacturerCode
    JucePlugin_CFBundleIdentifier=com.parity.Parity
)

target_link_libraries(UICaptureTest PRIVATE
    ParityAssets
    juce::juce_audio_utils
    juce::juce_dsp
    juce::juce_recommended_config_flags
)
