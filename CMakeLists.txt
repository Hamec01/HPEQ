cmake_minimum_required(VERSION 3.22)

project(HamloEcho VERSION 0.2.1 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# --- JUCE -------------------------------------------------------------------------------------
# The same JUCE version as HPEQ (the delay came from there). A local checkout avoids a download.
set(HAMLO_ECHO_JUCE_VERSION 8.0.12)
set(HAMLO_ECHO_JUCE_DIR "" CACHE PATH "Local JUCE ${HAMLO_ECHO_JUCE_VERSION} checkout (empty = FetchContent)")

if (HAMLO_ECHO_JUCE_DIR)
    add_subdirectory("${HAMLO_ECHO_JUCE_DIR}" "${CMAKE_BINARY_DIR}/JUCE")
    set(HAMLO_ECHO_JUCE_SOURCE "${HAMLO_ECHO_JUCE_DIR}")
else()
    include(FetchContent)
    FetchContent_Declare(JUCE
        GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
        GIT_TAG ${HAMLO_ECHO_JUCE_VERSION}
        GIT_SHALLOW TRUE)
    FetchContent_MakeAvailable(JUCE)
    set(HAMLO_ECHO_JUCE_SOURCE "${juce_SOURCE_DIR}")
endif()

# JUCE 8 creates a Direct2D / D3D11 device per Windows plug-in DLL; FL Studio may unload the plug-in while
# transport runs and some NVIDIA drivers hang tearing that device down. The patch adds compile switches
# (HPEQ_JUCE_DEFAULT_*, named after the project they came from) so this target can default to the GDI
# software renderer. A local JUCE must already carry the patch (HPEQ's fetched copy does).
if (WIN32 AND NOT HAMLO_ECHO_JUCE_DIR)
    include(cmake/PatchJuceWindowsRenderer.cmake)
    hpeq_patch_juce_windows_renderer("${HAMLO_ECHO_JUCE_SOURCE}")
endif()

enable_testing()

# Embed the exact production source identity in every laboratory executable.
set(SOURCE_ROOT "${CMAKE_CURRENT_SOURCE_DIR}")
set(SOURCE_VERSION "${PROJECT_VERSION}")
set(IDENTITY_OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/SourceIdentity.h")
include(cmake/WriteSourceIdentity.cmake)
add_custom_target(HamloEcho_SourceIdentity
    COMMAND "${CMAKE_COMMAND}" "-DSOURCE_ROOT=${CMAKE_CURRENT_SOURCE_DIR}" "-DSOURCE_VERSION=${PROJECT_VERSION}" "-DIDENTITY_OUTPUT=${CMAKE_CURRENT_BINARY_DIR}/SourceIdentity.h" -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/WriteSourceIdentity.cmake")

# --- Assets: HPEQ's Black Steel pictures (scratches, tubes, the hardware pack), embedded as HpeqAssets -------
file(GLOB HAMLO_ECHO_SCRATCHES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/Assets/scratches/*.png)
file(GLOB_RECURSE HAMLO_ECHO_HARDWARE CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/Assets/hardware_pack/*.png)
file(GLOB HAMLO_ECHO_TUBES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/Assets/tubes/*.png)
juce_add_binary_data(HamloEcho_Assets NAMESPACE HpeqAssets SOURCES ${HAMLO_ECHO_SCRATCHES} ${HAMLO_ECHO_HARDWARE} ${HAMLO_ECHO_TUBES})
set_target_properties(HamloEcho_Assets PROPERTIES POSITION_INDEPENDENT_CODE ON)

# --- Echo-only shared code; M0 provenance and bit-exact verification: docs/SYNC_WITH_HPEQ.md -----------
add_library(HamloEcho_Shared STATIC
    Source/Core/ModulePresets.cpp
    Source/DSP/HamloEcho.cpp
    Source/DSP/LegacyDelay.cpp
    Source/DSP/LegacyEchoCore.cpp
    Source/DSP/ReflectionPrototype.cpp
    Source/DSP/ScoreEngine.cpp
    Source/DSP/ScoreEffects.cpp
    Source/UI/BlackSteel.cpp
    Source/UI/HamloEchoPanel.cpp
    Source/UI/ScorePanel.cpp
    Source/Standalone/ModuleVariant.cpp
    Source/Standalone/ScoreParameters.cpp
    Source/Standalone/ModulePluginProcessor.cpp
    Source/Standalone/ModulePluginEditor.cpp
    Source/UI/PresetScreen.cpp)
target_include_directories(HamloEcho_Shared
    PUBLIC Source
    PRIVATE
        "${HAMLO_ECHO_JUCE_SOURCE}/modules"
        "${CMAKE_CURRENT_BINARY_DIR}/juce_binarydata_HamloEcho_Assets/JuceLibraryCode")
target_compile_definitions(HamloEcho_Shared
    PUBLIC
        JUCE_GLOBAL_MODULE_SETTINGS_INCLUDED=1
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0)
add_dependencies(HamloEcho_Shared HamloEcho_Assets) # BinaryData.h first
target_link_libraries(HamloEcho_Shared
    PRIVATE
        juce::juce_recommended_config_flags
        juce::juce_recommended_warning_flags)

# --- The plug-in: identity = HPEQ's HamloEcho module VST3 (BBLB / HmEc / com.hamloprod.hamloecho, kind 2)
juce_add_plugin(HamloEcho
    COMPANY_NAME "HamloProd"
    BUNDLE_ID com.hamloprod.hamloecho
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE
    PLUGIN_MANUFACTURER_CODE BBLB
    PLUGIN_CODE HmEc
    VST3_CATEGORIES Fx
    FORMATS VST3
    PRODUCT_NAME "HamloEcho")
target_sources(HamloEcho PRIVATE Source/Standalone/ModulePluginEntry.cpp)
target_compile_definitions(HamloEcho PRIVATE
    HPEQ_MODULE_KIND=2
    HPEQ_JUCE_DEFAULT_IMAGE_TYPE=SoftwareImageType
    HPEQ_JUCE_DEFAULT_RENDERING_ENGINE=0
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0)
target_link_libraries(HamloEcho
    PRIVATE
        HamloEcho_Shared
        HamloEcho_Assets
        juce::juce_audio_utils
        juce::juce_dsp
        juce::juce_audio_formats
        juce::juce_cryptography
        juce::juce_recommended_config_flags
        juce::juce_recommended_warning_flags)

# --- Tests and the page snapshot ----------------------------------------------------------------------
foreach(tool Tests Snapshot Lab ScorePresetLab)
    if (tool STREQUAL "Tests")
        add_executable(HamloEcho_Tests Tests/TestMain.cpp Tests/PluginTests.cpp Tests/LegacyStateTests.cpp Tests/ScoreIntegrationTests.cpp Tests/AllocationAudit.cpp)
    elseif(tool STREQUAL "ScorePresetLab")
        add_executable(HamloEcho_ScorePresetLab Tests/ScorePresetLabMain.cpp)
        target_include_directories(HamloEcho_ScorePresetLab PRIVATE "${CMAKE_CURRENT_BINARY_DIR}")
        add_dependencies(HamloEcho_ScorePresetLab HamloEcho_SourceIdentity)
    elseif(tool STREQUAL "Lab")
        add_executable(HamloEcho_Lab Tests/EchoLabMain.cpp)
    else()
        add_executable(HamloEcho_Snapshot Tests/SnapshotMain.cpp)
    endif()
    target_compile_definitions(HamloEcho_${tool} PRIVATE HPEQ_MODULE_KIND=2 JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_MODAL_LOOPS_PERMITTED=1)
    target_link_libraries(HamloEcho_${tool}
        PRIVATE
            HamloEcho_Shared
            HamloEcho_Assets
            juce::juce_audio_utils
            juce::juce_dsp
            juce::juce_audio_formats
            juce::juce_cryptography
            juce::juce_recommended_config_flags
            juce::juce_recommended_warning_flags)
endforeach()
add_test(NAME HamloEcho_Tests COMMAND HamloEcho_Tests)

# Research only: candidate note/reverse renderer, deliberately not linked into the plugin.
add_executable(HamloEcho_PrototypeLab Tests/PrototypeLabMain.cpp Source/DSP/ReflectionPrototype.cpp)
target_include_directories(HamloEcho_PrototypeLab PRIVATE Source)
target_compile_options(HamloEcho_PrototypeLab PRIVATE $<$<CXX_COMPILER_ID:MSVC>:/W4>)

# The same research harness against immutable 05a1445 DSP for strengthened before/after methods.
add_executable(HamloEcho_AttackBaselineLab Tests/PrototypeLabMain.cpp Tests/PrototypeReference/DSP/ReflectionPrototype.cpp)
target_include_directories(HamloEcho_AttackBaselineLab PRIVATE Tests/PrototypeReference)

add_executable(HamloEcho_ScoreLab Tests/ScoreLabMain.cpp Source/DSP/ScoreEngine.cpp Source/DSP/ScoreEffects.cpp Source/DSP/ReflectionPrototype.cpp)
target_include_directories(HamloEcho_ScoreLab PRIVATE Source "${CMAKE_CURRENT_BINARY_DIR}")
add_dependencies(HamloEcho_ScoreLab HamloEcho_SourceIdentity)
