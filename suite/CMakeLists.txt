cmake_minimum_required(VERSION 3.22)
project(LGNSuite VERSION 1.0.0 LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if(APPLE)
  set(CMAKE_OSX_DEPLOYMENT_TARGET "10.13" CACHE STRING "")
endif()

if(DEFINED JUCE_DIR)
  add_subdirectory(${JUCE_DIR} juce EXCLUDE_FROM_ALL)
else()
  include(FetchContent)
  FetchContent_Declare(juce GIT_REPOSITORY https://github.com/juce-framework/JUCE.git GIT_TAG 8.0.9 GIT_SHALLOW TRUE)
  FetchContent_MakeAvailable(juce)
endif()
include(${CMAKE_CURRENT_LIST_DIR}/../shared/station/station.cmake)

set(LGN_FORMATS VST3)
if(APPLE)
  list(APPEND LGN_FORMATS AU)
endif()

set(LGN_DEFS JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_VST3_CAN_REPLACE_VST2=0 JUCE_DISPLAY_SPLASH_SCREEN=0 JUCE_REPORT_APP_USAGE=0)
set(LGN_PLUGINS "")

# lgn_plugin(<target> <4-char code> <product name> <bundle suffix> <vst3 categories, | separated> <source>)
# Add INSTRUMENT after the source for a MIDI-driven instrument (no audio input).
function(lgn_plugin target code product bundle categories source)
  string(REPLACE "|" ";" cats "${categories}")
  set(is_synth FALSE)
  set(needs_midi FALSE)
  set(au_type kAudioUnitType_Effect)
  if("MIDI" IN_LIST ARGN)        # an effect that also listens to MIDI notes
    set(needs_midi TRUE)
    set(au_type kAudioUnitType_MusicEffect)
  endif()
  if("INSTRUMENT" IN_LIST ARGN)
    set(is_synth TRUE)
    set(needs_midi TRUE)
    set(au_type kAudioUnitType_MusicDevice)
  endif()
  juce_add_plugin(${target}
    COMPANY_NAME "LGN"
    BUNDLE_ID "com.lgn.${bundle}"
    PLUGIN_MANUFACTURER_CODE Lgnx
    PLUGIN_CODE ${code}
    FORMATS ${LGN_FORMATS}
    PRODUCT_NAME "${product}"
    IS_SYNTH ${is_synth}
    NEEDS_MIDI_INPUT ${needs_midi}
    NEEDS_MIDI_OUTPUT FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE
    VST3_CATEGORIES ${cats}
    AU_MAIN_TYPE ${au_type})
  target_sources(${target} PRIVATE common/Suite.cpp ${source})
  target_include_directories(${target} PRIVATE common dsp)
  station_use(${target})
  target_compile_definitions(${target} PUBLIC ${LGN_DEFS})
  target_link_libraries(${target} PRIVATE juce::juce_audio_utils juce::juce_dsp PUBLIC juce::juce_recommended_config_flags juce::juce_recommended_lto_flags)
  if(MSVC)
    target_compile_options(${target} PRIVATE /utf-8)
  endif()

  # Headless host test: loads the plugin, plays audio at several rates and block sizes, fuzzes every parameter,
  # round-trips state and renders the editor to PNG.
  juce_add_console_app(${target}_host PRODUCT_NAME "${target}_host")
  target_sources(${target}_host PRIVATE common/Suite.cpp ${source} tests/host_test.cpp)
  target_include_directories(${target}_host PRIVATE common dsp)
  target_compile_definitions(${target}_host PRIVATE ${LGN_DEFS} JucePlugin_Name="${product}")
  target_link_libraries(${target}_host PRIVATE juce::juce_audio_utils juce::juce_dsp juce::juce_recommended_config_flags)
  station_use(${target}_host)
  set_target_properties(${target}_host PROPERTIES EXCLUDE_FROM_ALL TRUE)

  set(LGN_PLUGINS ${LGN_PLUGINS} ${target} PARENT_SCOPE)
endfunction()

lgn_plugin(LGN_EQ         LgEq "LGN EQ"         eq        "Fx|EQ"          eq/EqPlugin.cpp)
lgn_plugin(LGN_Compressor LgCp "LGN Compressor" comp      "Fx|Dynamics"    comp/CompPlugin.cpp)
lgn_plugin(LGN_DeEsser    LgDs "LGN De-Esser"   deesser   "Fx|Dynamics"    deess/DeEssPlugin.cpp)
lgn_plugin(LGN_Saturator  LgSt "LGN Saturator"  saturator "Fx|Distortion"  saturate/SatPlugin.cpp)
lgn_plugin(LGN_Reverb     LgRv "LGN Reverb"     reverb    "Fx|Reverb"      verb/VerbPlugin.cpp)
lgn_plugin(LGN_Delay      LgDl "LGN Delay"      delay     "Fx|Delay"       delay/DelayPlugin.cpp)
lgn_plugin(LGN_Limiter    LgLm "LGN Limiter"    limiter   "Fx|Dynamics"    limit/LimitPlugin.cpp)
lgn_plugin(LGN_Gravity    LgGv "LGN Gravity"    gravity   "Fx|Pitch Shift" gravity/GravityPlugin.cpp)
lgn_plugin(LGN_Duck       Duck "LGN Duck"       duck      "Fx|Dynamics"    duck/DuckPlugin.cpp)
lgn_plugin(LGN_Fret       Fret "LGN Fret"       fret      "Instrument|Synth" fret/FretPlugin.cpp INSTRUMENT)
lgn_plugin(LGN_Haze       LgHz "LGN Haze"       haze      "Instrument|Synth" haze/HazePlugin.cpp INSTRUMENT)
lgn_plugin(LGN_Strike     LgSk "LGN Strike"     strike    "Instrument|Drum" strike/StrikePlugin.cpp INSTRUMENT)
lgn_plugin(LGN_Distance   LgDt "LGN Distance"   distance  "Fx|Spatial"     distance/DistancePlugin.cpp)
lgn_plugin(LGN_Era        LgEr "LGN Era"        era       "Fx|Distortion"  era/EraPlugin.cpp)
lgn_plugin(LGN_Swing      LgSw "LGN Swing"      swing     "Fx"             swing/SwingPlugin.cpp)
lgn_plugin(LGN_Mood       LgMd "LGN Mood"       mood      "Fx|Pitch Shift" mood/MoodPlugin.cpp)
lgn_plugin(LGN_Flipside   LgFs "LGN Flipside"   flipside  "Fx"             flipside/FlipsidePlugin.cpp MIDI)

# DSP unit tests (no JUCE).
add_executable(lgn_dsp_test EXCLUDE_FROM_ALL tests/dsp_test.cpp)
target_include_directories(lgn_dsp_test PRIVATE dsp)
