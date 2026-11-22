cmake_minimum_required(VERSION 3.22)
project(NoirnuitReverse VERSION 1.0.0 LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# First configure needs Internet to get the open-source JUCE framework.
# For an offline build, place JUCE in ./JUCE and it will be used automatically.
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/JUCE/CMakeLists.txt")
    add_subdirectory(JUCE)
else()
    include(FetchContent)
    FetchContent_Declare(JUCE
        GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
        GIT_TAG 8.0.13
        GIT_SHALLOW TRUE)
    FetchContent_MakeAvailable(JUCE)
endif()

juce_add_plugin(NoirnuitReverse
    COMPANY_NAME "Noirnuit Audio"
    BUNDLE_ID "audio.noirnuit.reverse"
    PLUGIN_MANUFACTURER_CODE NnAu
    PLUGIN_CODE NRev
    FORMATS VST3 Standalone
    PRODUCT_NAME "NOIRNUIT REVERSE"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    VST3_CATEGORIES Fx Reverb
    COPY_PLUGIN_AFTER_BUILD FALSE)

juce_generate_juce_header(NoirnuitReverse)
target_sources(NoirnuitReverse PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginProcessor.h
    Source/PluginEditor.cpp
    Source/PluginEditor.h
    Source/ReverseEngine.cpp
    Source/ReverseEngine.h)

target_compile_definitions(NoirnuitReverse PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0)

target_link_libraries(NoirnuitReverse PRIVATE
    juce::juce_audio_utils
    juce::juce_audio_processors
    juce::juce_dsp
    juce::juce_gui_extra
    PUBLIC juce::juce_recommended_config_flags
           juce::juce_recommended_lto_flags
           juce::juce_recommended_warning_flags)

# A zero-dependency CLI that uses exactly the same reverse DSP engine:
add_executable(noirnuit-render
    Tools/OfflineRenderer.cpp Source/ReverseEngine.cpp)
target_compile_features(noirnuit-render PRIVATE cxx_std_17)

include(CTest)
if(BUILD_TESTING)
    add_executable(reverse-engine-tests Tests/test_engine.cpp Source/ReverseEngine.cpp)
    add_test(NAME reverse-engine-tests COMMAND reverse-engine-tests)
endif()
