cmake_minimum_required(VERSION 3.22)
project(Beatwerk VERSION 1.1.0)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(JUCE)

juce_add_plugin(Beatwerk
    COMPANY_NAME "Beatwerk"
    PLUGIN_MANUFACTURER_CODE Btwk
    PLUGIN_CODE Btw1
    FORMATS AU VST3 Standalone
    PRODUCT_NAME "Beatwerk"
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS TRUE
    COPY_PLUGIN_AFTER_BUILD TRUE
    ICON_BIG "${CMAKE_CURRENT_SOURCE_DIR}/Resources/icon_big.png"
    ICON_SMALL "${CMAKE_CURRENT_SOURCE_DIR}/Resources/icon_small.png")

target_sources(Beatwerk
    PRIVATE
        Source/PluginProcessor.cpp
        Source/PluginEditor.cpp
        Source/MidiMapper.cpp
        Source/SampleEngine.cpp
        Source/AdgParser.cpp
        Source/DrumKitLibrary.cpp
        Source/PresetManager.cpp
        Source/PadComponent.cpp
        Source/PadMappingManager.cpp
        Source/PresetListComponent.cpp
        Source/LookAndFeel.cpp
        Source/AbletonImporter.cpp
        Source/SampleBrowserComponent.cpp)

target_compile_definitions(Beatwerk
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_DISPLAY_SPLASH_SCREEN=0)

target_link_libraries(Beatwerk
    PRIVATE
        juce::juce_audio_utils
        juce::juce_gui_extra
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)
