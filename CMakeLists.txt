cmake_minimum_required(VERSION 3.22)

project(STALLVERKET VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Windows: bygg in C++-runtime i pluginet, så att köparen inte behöver installera något extra
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

# JUCE hämtas automatiskt vid bygget
include(FetchContent)
FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 8.0.4
    GIT_SHALLOW ON)
FetchContent_MakeAvailable(JUCE)

if(APPLE)
    set(STV_FORMATS VST3 AU)
else()
    set(STV_FORMATS VST3)
endif()
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    list(APPEND STV_FORMATS LV2)
endif()

# STÄLLVERKET är en effekt: den läggs på varje kanal som ska vara med i låten.
juce_add_plugin(Stallverket
    COMPANY_NAME "Phunkshunz"
    PRODUCT_NAME "STALLVERKET"
    BUNDLE_ID "com.phunkshunz.stallverket"
    PLUGIN_MANUFACTURER_CODE Phnk
    PLUGIN_CODE Stlv
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT TRUE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE
    VST3_CATEGORIES Fx Tools
    AU_MAIN_TYPE kAudioUnitType_Effect
    LV2URI "https://phunkshunz.github.io/lv2/Stallverket"
    FORMATS ${STV_FORMATS})

target_sources(Stallverket
    PRIVATE
        src/PluginProcessor.cpp
        src/PluginEditor.cpp)

target_include_directories(Stallverket PRIVATE src)

target_compile_definitions(Stallverket
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_DISPLAY_SPLASH_SCREEN=0)

target_link_libraries(Stallverket
    PRIVATE
        juce::juce_audio_utils
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)

# MSVC: läs källkoden som UTF-8, så att å, ä och ö blir rätt
if(MSVC)
    target_compile_options(Stallverket PRIVATE /utf-8 /bigobj)
endif()

# Test: cmake -B build -DSTV_TEST=ON && cmake --build build --target StallverketTest
option(STV_TEST "Bygg testprogrammet" OFF)
if(STV_TEST)
    juce_add_console_app(StallverketTest PRODUCT_NAME "StallverketTest")
    target_sources(StallverketTest PRIVATE tests/stv_test.cpp src/PluginProcessor.cpp src/PluginEditor.cpp)
    target_include_directories(StallverketTest PRIVATE src)
    target_compile_definitions(StallverketTest PRIVATE JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_STANDALONE_APPLICATION=0 JucePlugin_Name="STALLVERKET" STALLVERKET_TEST_UNLOCKED=1)
    target_link_libraries(StallverketTest PRIVATE juce::juce_audio_utils juce::juce_dsp PUBLIC juce::juce_recommended_config_flags)
endif()
