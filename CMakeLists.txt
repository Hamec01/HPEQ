cmake_minimum_required(VERSION 3.22)

project(HamloVerb VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# --- JUCE -------------------------------------------------------------------------------------
# The same JUCE version as HPEQ (the reverb came from there). A local checkout avoids a download.
set(HAMLO_VERB_JUCE_VERSION 8.0.12)
set(HAMLO_VERB_JUCE_DIR "" CACHE PATH "Local JUCE ${HAMLO_VERB_JUCE_VERSION} checkout (empty = FetchContent)")

if (HAMLO_VERB_JUCE_DIR)
    add_subdirectory("${HAMLO_VERB_JUCE_DIR}" "${CMAKE_BINARY_DIR}/JUCE")
    set(HAMLO_VERB_JUCE_SOURCE "${HAMLO_VERB_JUCE_DIR}")
else()
    include(FetchContent)
    FetchContent_Declare(JUCE
        GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
        GIT_TAG ${HAMLO_VERB_JUCE_VERSION}
        GIT_SHALLOW TRUE)
    FetchContent_MakeAvailable(JUCE)
    set(HAMLO_VERB_JUCE_SOURCE "${juce_SOURCE_DIR}")
endif()

# JUCE 8 creates a Direct2D / D3D11 device per Windows plug-in DLL; FL Studio may unload the plug-in while
# transport runs and some NVIDIA drivers hang tearing that device down. The patch adds compile switches
# (HPEQ_JUCE_DEFAULT_*, named after the project they came from) so this target can default to the GDI
# software renderer. A local JUCE must already carry the patch (HPEQ's fetched copy does).
if (WIN32 AND NOT HAMLO_VERB_JUCE_DIR)
    include(cmake/PatchJuceWindowsRenderer.cmake)
    hpeq_patch_juce_windows_renderer("${HAMLO_VERB_JUCE_SOURCE}")
endif()

enable_testing()

# --- Assets: HPEQ's Black Steel pictures (scratches, tubes, the hardware pack), embedded as HpeqAssets -------
file(GLOB HAMLO_VERB_SCRATCHES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/Assets/scratches/*.png)
file(GLOB_RECURSE HAMLO_VERB_HARDWARE CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/Assets/hardware_pack/*.png)
file(GLOB HAMLO_VERB_TUBES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/Assets/tubes/*.png)
juce_add_binary_data(HamloVerb_Assets NAMESPACE HpeqAssets SOURCES ${HAMLO_VERB_SCRATCHES} ${HAMLO_VERB_HARDWARE} ${HAMLO_VERB_TUBES})
set_target_properties(HamloVerb_Assets PROPERTIES POSITION_INDEPENDENT_CODE ON)

# --- Shared code: HPEQ's module-plug-in stack (Source/ is HPEQ's, verbatim; docs/SYNC_WITH_HPEQ.md) -----------
add_library(HamloVerb_Shared STATIC
    Source/Core/ModulePresets.cpp
    Source/DSP/Biquad.cpp
    Source/DSP/FxNodeProcessor.cpp
    Source/DSP/LayerEngine.cpp
    Source/DSP/PreEqProcessor.cpp
    Source/DSP/Processors.cpp
    Source/DSP/ReverbProcessor.cpp
    Source/DSP/HamloVerb.cpp
    Source/DSP/HamloEcho.cpp
    Source/DSP/HamloMag.cpp
    Source/DSP/ProcessorsM6.cpp
    Source/DSP/HamloRevolution.cpp
    Source/DSP/HamloRedWall.cpp
    Source/DSP/HamloUdarnik.cpp
    Source/DSP/HamloZastava.cpp
    Source/DSP/HamloShirota.cpp
    Source/DSP/CrossoverTree.cpp
    Source/DSP/EqDesign.cpp
    Source/UI/BlackSteel.cpp
    Source/UI/HamloCompPanel.cpp
    Source/UI/HamloVerbPanel.cpp
    Source/UI/HamloEchoPanel.cpp
    Source/UI/HamloMagPanel.cpp
    Source/UI/HamloRedWallPanel.cpp
    Source/UI/HamloUdarnikPanel.cpp
    Source/UI/HamloZastavaPanel.cpp
    Source/UI/HamloShirotaPanel.cpp
    Source/Standalone/ModuleVariant.cpp
    Source/Standalone/ModulePluginProcessor.cpp
    Source/Standalone/ModulePluginEditor.cpp
    Source/UI/PresetScreen.cpp)
target_include_directories(HamloVerb_Shared
    PUBLIC Source
    PRIVATE
        "${HAMLO_VERB_JUCE_SOURCE}/modules"
        "${CMAKE_CURRENT_BINARY_DIR}/juce_binarydata_HamloVerb_Assets/JuceLibraryCode")
target_compile_definitions(HamloVerb_Shared
    PUBLIC
        JUCE_GLOBAL_MODULE_SETTINGS_INCLUDED=1
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0)
add_dependencies(HamloVerb_Shared HamloVerb_Assets) # BinaryData.h first
target_link_libraries(HamloVerb_Shared
    PRIVATE
        juce::juce_recommended_config_flags
        juce::juce_recommended_warning_flags)

# --- The plug-in: identity = HPEQ's HamloVerb module VST3 (BBLB / HmVb / com.hamloprod.hamloverb, kind 1)
juce_add_plugin(HamloVerb
    COMPANY_NAME "HamloProd"
    BUNDLE_ID com.hamloprod.hamloverb
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE
    PLUGIN_MANUFACTURER_CODE BBLB
    PLUGIN_CODE HmVb
    VST3_CATEGORIES Fx
    FORMATS VST3
    PRODUCT_NAME "HamloVerb")
target_sources(HamloVerb PRIVATE Source/Standalone/ModulePluginEntry.cpp)
target_compile_definitions(HamloVerb PRIVATE
    HPEQ_MODULE_KIND=1
    HPEQ_JUCE_DEFAULT_IMAGE_TYPE=SoftwareImageType
    HPEQ_JUCE_DEFAULT_RENDERING_ENGINE=0
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0)
target_link_libraries(HamloVerb
    PRIVATE
        HamloVerb_Shared
        HamloVerb_Assets
        juce::juce_audio_utils
        juce::juce_dsp
        juce::juce_audio_formats
        juce::juce_cryptography
        juce::juce_recommended_config_flags
        juce::juce_recommended_warning_flags)

# --- Tests and the page snapshot ----------------------------------------------------------------------
foreach(tool Tests Snapshot)
    if (tool STREQUAL "Tests")
        add_executable(HamloVerb_Tests Tests/TestMain.cpp Tests/PluginTests.cpp)
    else()
        add_executable(HamloVerb_Snapshot Tests/SnapshotMain.cpp)
    endif()
    target_compile_definitions(HamloVerb_${tool} PRIVATE HPEQ_MODULE_KIND=1 JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_MODAL_LOOPS_PERMITTED=1)
    target_link_libraries(HamloVerb_${tool}
        PRIVATE
            HamloVerb_Shared
            HamloVerb_Assets
            juce::juce_audio_utils
            juce::juce_dsp
            juce::juce_audio_formats
            juce::juce_cryptography
            juce::juce_recommended_config_flags
            juce::juce_recommended_warning_flags)
endforeach()
add_test(NAME HamloVerb_Tests COMMAND HamloVerb_Tests)
