cmake_minimum_required(VERSION 3.22)
project(BLACKBOX VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(MSVC)
    add_compile_options(/W4)
else()
    add_compile_options(-Wall -Wextra)
endif()

# ---------------------------------------------------------------------------
# The DSP core is deliberately header-only and free of JUCE types. It is what
# the offline renderer compiles against, and it keeps the licensing options
# open if this ever stops being a GPL release.
# ---------------------------------------------------------------------------
add_library(bb_dsp INTERFACE)
target_include_directories(bb_dsp INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/src)

# Offline renderer - no JUCE, no audio device, no host. Builds anywhere.
add_executable(bb_render tools/render.cpp)
target_link_libraries(bb_render PRIVATE bb_dsp)

# ---------------------------------------------------------------------------
# Plugin. Skipped if JUCE is unavailable so that bb_render still builds on a
# machine with no GUI dependencies installed.
# ---------------------------------------------------------------------------
option(BLACKBOX_BUILD_PLUGIN "Build the VST3 / standalone plugin targets" ON)

if(BLACKBOX_BUILD_PLUGIN)
    include(FetchContent)
    FetchContent_Declare(JUCE
        GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
        GIT_TAG        8.0.15
        GIT_SHALLOW    TRUE)
    FetchContent_MakeAvailable(JUCE)

    # Animated backdrop frames, compiled in so the plugin stays a single file.
    file(GLOB BLACKBOX_BG_FRAMES CONFIGURE_DEPENDS
         ${CMAKE_CURRENT_SOURCE_DIR}/resources/bg/*.png)

    juce_add_binary_data(BlackboxAssets SOURCES ${BLACKBOX_BG_FRAMES})

    # The plugin is a shared library, so the static data it links must be PIC.
    set_target_properties(BlackboxAssets PROPERTIES POSITION_INDEPENDENT_CODE TRUE)

    juce_add_plugin(BLACKBOX
        COMPANY_NAME            "Blackbox Audio"
        PLUGIN_MANUFACTURER_CODE Bbdk
        PLUGIN_CODE              Bbx1
        FORMATS                  VST3 Standalone
        IS_SYNTH                 TRUE
        NEEDS_MIDI_INPUT         TRUE
        NEEDS_MIDI_OUTPUT        FALSE
        IS_MIDI_EFFECT           FALSE
        EDITOR_WANTS_KEYBOARD_FOCUS TRUE
        COPY_PLUGIN_AFTER_BUILD  FALSE
        PRODUCT_NAME             "BLACKBOX")

    target_sources(BLACKBOX PRIVATE
        src/plugin/BackgroundFilm.cpp
        src/plugin/PluginProcessor.cpp
        src/plugin/PluginEditor.cpp
        src/plugin/ParameterLayout.cpp
        src/plugin/VintageLookAndFeel.cpp)

    target_compile_definitions(BLACKBOX PUBLIC
        JUCE_WEB_BROWSER=0        # trims libwebkit2gtk off the Linux dep list
        JUCE_USE_CURL=0           # and libcurl
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_DISPLAY_SPLASH_SCREEN=0)

    target_link_libraries(BLACKBOX PRIVATE
        BlackboxAssets
        bb_dsp
        juce::juce_audio_utils
        juce::juce_dsp
        PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)
endif()
