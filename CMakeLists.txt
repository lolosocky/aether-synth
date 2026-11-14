cmake_minimum_required(VERSION 3.22)
project(Aether VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if(APPLE)
  set(CMAKE_OSX_DEPLOYMENT_TARGET "10.13" CACHE STRING "")
endif()

# JUCE: use a local checkout if given (-DJUCE_DIR=...), otherwise fetch it.
if(DEFINED JUCE_DIR)
  add_subdirectory(${JUCE_DIR} juce EXCLUDE_FROM_ALL)
else()
  include(FetchContent)
  FetchContent_Declare(juce GIT_REPOSITORY https://github.com/juce-framework/JUCE.git GIT_TAG 8.0.9 GIT_SHALLOW TRUE)
  FetchContent_MakeAvailable(juce)
endif()

set(AETHER_FORMATS VST3)
if(APPLE)
  list(APPEND AETHER_FORMATS AU)
endif()
option(AETHER_STANDALONE "Also build a standalone app" ON)
if(AETHER_STANDALONE)
  list(APPEND AETHER_FORMATS Standalone)
endif()

juce_add_plugin(Aether
  COMPANY_NAME "LGN"
  BUNDLE_ID "com.lgn.aether"
  PLUGIN_MANUFACTURER_CODE Lgnx
  PLUGIN_CODE Aeth
  FORMATS ${AETHER_FORMATS}
  PRODUCT_NAME "Aether"
  IS_SYNTH TRUE
  NEEDS_MIDI_INPUT TRUE
  NEEDS_MIDI_OUTPUT FALSE
  IS_MIDI_EFFECT FALSE
  EDITOR_WANTS_KEYBOARD_FOCUS FALSE
  COPY_PLUGIN_AFTER_BUILD FALSE
  VST3_CATEGORIES Instrument Synth
  AU_MAIN_TYPE kAudioUnitType_MusicDevice)

juce_add_binary_data(AetherData SOURCES resources/presets.json.gz)

target_sources(Aether PRIVATE plugin/PluginProcessor.cpp plugin/PluginEditor.cpp)
target_include_directories(Aether PRIVATE src third_party)
target_compile_definitions(Aether PUBLIC
  JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_VST3_CAN_REPLACE_VST2=0
  JUCE_DISPLAY_SPLASH_SCREEN=0 JUCE_REPORT_APP_USAGE=0)
target_link_libraries(Aether PRIVATE AetherData
  juce::juce_audio_utils juce::juce_dsp
  PUBLIC juce::juce_recommended_config_flags juce::juce_recommended_lto_flags)
if(MSVC)
  target_compile_options(Aether PRIVATE /bigobj /utf-8)
endif()

# Engine port test (no JUCE): compares every preset against reference renders from the browser engine.
add_executable(aether_port_test EXCLUDE_FROM_ALL tests/port_test.cpp)
target_include_directories(aether_port_test PRIVATE src third_party)

# Headless test of the real plugin processor.
juce_add_console_app(aether_plugin_test)
target_sources(aether_plugin_test PRIVATE tests/plugin_test.cpp plugin/PluginProcessor.cpp plugin/PluginEditor.cpp)
target_include_directories(aether_plugin_test PRIVATE src third_party)
target_compile_definitions(aether_plugin_test PRIVATE JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JucePlugin_Name="Aether")
target_link_libraries(aether_plugin_test PRIVATE AetherData juce::juce_audio_utils juce::juce_dsp juce::juce_recommended_config_flags)
set_target_properties(aether_plugin_test PROPERTIES EXCLUDE_FROM_ALL TRUE)
