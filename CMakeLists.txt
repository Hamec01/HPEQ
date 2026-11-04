cmake_minimum_required(VERSION 3.22)

project(HamloFader VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# --- JUCE -------------------------------------------------------------------------------------
# The same JUCE version as HPEQ (the gain utility came from there). A local checkout avoids a download.
set(HAMLO_FADER_JUCE_VERSION 8.0.12)
set(HAMLO_FADER_JUCE_DIR "" CACHE PATH "Local JUCE ${HAMLO_FADER_JUCE_VERSION} checkout (empty = FetchContent)")

if (HAMLO_FADER_JUCE_DIR)
    add_subdirectory("${HAMLO_FADER_JUCE_DIR}" "${CMAKE_BINARY_DIR}/JUCE")
    set(HAMLO_FADER_JUCE_SOURCE "${HAMLO_FADER_JUCE_DIR}")
else()
    include(FetchContent)
    FetchContent_Declare(JUCE
        GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
        GIT_TAG ${HAMLO_FADER_JUCE_VERSION}
        GIT_SHALLOW TRUE)
    FetchContent_MakeAvailable(JUCE)
    set(HAMLO_FADER_JUCE_SOURCE "${juce_SOURCE_DIR}")
endif()

# JUCE 8 creates a Direct2D / D3D11 device per Windows plug-in DLL; FL Studio may unload the plug-in while
# transport runs and some NVIDIA drivers hang tearing that device down. The patch adds compile switches
# (HPEQ_JUCE_DEFAULT_*, named after the project they came from) so this target can default to the GDI
# software renderer. A local JUCE must already carry the patch (HPEQ's fetched copy does).
if (WIN32 AND NOT HAMLO_FADER_JUCE_DIR)
    include(cmake/PatchJuceWindowsRenderer.cmake)
    hpeq_patch_juce_windows_renderer("${HAMLO_FADER_JUCE_SOURCE}")
endif()

enable_testing()

# --- Assets: HPEQ's Black Steel pictures (scratches, tubes, the hardware pack), embedded as HpeqAssets -------
file(GLOB HAMLO_FADER_SCRATCHES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/Assets/scratches/*.png)
file(GLOB_RECURSE HAMLO_FADER_HARDWARE CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/Assets/hardware_pack/*.png)
file(GLOB HAMLO_FADER_TUBES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/Assets/tubes/*.png)
juce_add_binary_data(HamloFader_Assets NAMESPACE HpeqAssets SOURCES ${HAMLO_FADER_SCRATCHES} ${HAMLO_FADER_HARDWARE} ${HAMLO_FADER_TUBES})
set_target_properties(HamloFader_Assets PROPERTIES POSITION_INDEPENDENT_CODE ON)

# --- Shared code: HPEQ's module-plug-in stack (Source/ is HPEQ's, verbatim; docs/SYNC_WITH_HPEQ.md) -----------
add_library(HamloFader_Shared STATIC
    Source/Core/ModulePresets.cpp
    Source/DSP/FaderKernel.cpp
    Source/DSP/FaderMeter.cpp
    Source/Standalone/FaderHostControls.cpp
    Source/DSP/FaderGain.cpp
    Source/DSP/LegacyFaderCore.cpp
    Source/UI/BlackSteel.cpp
    Source/UI/HamloFaderPanel.cpp
    Source/UI/PresetScreen.cpp
    Source/Standalone/ModuleVariant.cpp
    Source/Standalone/ModulePluginProcessor.cpp
    Source/Standalone/ModulePluginEditor.cpp
    Source/UI/FaderV2Panel.cpp
)
target_include_directories(HamloFader_Shared
    PUBLIC Source
    PRIVATE
        "${HAMLO_FADER_JUCE_SOURCE}/modules"
        "${CMAKE_CURRENT_BINARY_DIR}/juce_binarydata_HamloFader_Assets/JuceLibraryCode")
target_compile_definitions(HamloFader_Shared
    PUBLIC
        JUCE_GLOBAL_MODULE_SETTINGS_INCLUDED=1
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0)
add_dependencies(HamloFader_Shared HamloFader_Assets) # BinaryData.h first
target_link_libraries(HamloFader_Shared
    PRIVATE
        juce::juce_recommended_config_flags
        juce::juce_recommended_warning_flags)

# --- The plug-in: HamloFader (BBLB / HmFd / com.hamloprod.hamlofader, kind 8 - added here: HPEQ has no Fader module VST3)
juce_add_plugin(HamloFader
    COMPANY_NAME "HamloProd"
    BUNDLE_ID com.hamloprod.hamlofader
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE
    PLUGIN_MANUFACTURER_CODE BBLB
    PLUGIN_CODE HmFd
    VST3_CATEGORIES Fx
    FORMATS VST3
    PRODUCT_NAME "HamloFader")
target_sources(HamloFader PRIVATE Source/Standalone/ModulePluginEntry.cpp)
target_compile_definitions(HamloFader PRIVATE
    HPEQ_MODULE_KIND=8
    HPEQ_JUCE_DEFAULT_IMAGE_TYPE=SoftwareImageType
    HPEQ_JUCE_DEFAULT_RENDERING_ENGINE=0
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0)
target_link_libraries(HamloFader
    PRIVATE
        HamloFader_Shared
        HamloFader_Assets
        juce::juce_audio_utils
        juce::juce_dsp
        juce::juce_audio_formats
        juce::juce_cryptography
        juce::juce_recommended_config_flags
        juce::juce_recommended_warning_flags)

# --- Tests and the page snapshot ----------------------------------------------------------------------
foreach(tool Tests Snapshot)
    if (tool STREQUAL "Tests")
        add_executable(HamloFader_Tests Tests/TestMain.cpp Tests/PluginTests.cpp Tests/F4UITests.cpp)
    else()
        add_executable(HamloFader_Snapshot Tests/SnapshotMain.cpp)
    endif()
    target_compile_definitions(HamloFader_${tool} PRIVATE HPEQ_MODULE_KIND=8 JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_MODAL_LOOPS_PERMITTED=1)
    target_link_libraries(HamloFader_${tool}
        PRIVATE
            HamloFader_Shared
            HamloFader_Assets
            juce::juce_audio_utils
            juce::juce_dsp
            juce::juce_audio_formats
            juce::juce_cryptography
            juce::juce_recommended_config_flags
            juce::juce_recommended_warning_flags)
endforeach()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/Source/DSP/FaderKernel.cpp")
    add_executable(HamloFader_F2Tests EXCLUDE_FROM_ALL Tests/F2UnitMain.cpp Tests/AllocationAudit.cpp Source/DSP/FaderKernel.cpp Source/DSP/FaderMeter.cpp)
    add_dependencies(HamloFader_F2Tests HamloFader_F2CurrentIdentity)
    target_include_directories(HamloFader_F2Tests PRIVATE Source Tests "${CMAKE_CURRENT_BINARY_DIR}/f2_Current")
    target_link_libraries(HamloFader_F2Tests PRIVATE juce::juce_core juce::juce_recommended_config_flags)
endif()
add_test(NAME HamloFader_Tests COMMAND HamloFader_Tests)

add_custom_target(HamloFader_F1Identity
    COMMAND "${CMAKE_COMMAND}" "-DSOURCE_ROOT=${CMAKE_CURRENT_SOURCE_DIR}" "-DIDENTITY_OUTPUT=${CMAKE_CURRENT_BINARY_DIR}/FaderF1Identity.h" -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/WriteF1Identity.cmake")
add_executable(HamloFader_F1Lab EXCLUDE_FROM_ALL Tests/F1LabMain.cpp Tests/AllocationAudit.cpp)
add_dependencies(HamloFader_F1Lab HamloFader_F1Identity)
target_include_directories(HamloFader_F1Lab PRIVATE "${CMAKE_CURRENT_BINARY_DIR}" Tests)
target_compile_definitions(HamloFader_F1Lab PRIVATE HPEQ_MODULE_KIND=8 JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_MODAL_LOOPS_PERMITTED=1)
target_link_libraries(HamloFader_F1Lab PRIVATE HamloFader_Shared HamloFader_Assets juce::juce_audio_utils juce::juce_dsp juce::juce_audio_formats juce::juce_cryptography juce::juce_recommended_config_flags)

# Offline F2 control/candidate; shipping code and old F1 laboratory stay unchanged.
option(HAMLO_FADER_BUILD_F2_REFERENCE "Build optional archived F1 control" OFF)
foreach(flavour Reference Current)
    if(flavour STREQUAL "Reference" AND NOT HAMLO_FADER_BUILD_F2_REFERENCE)
        continue()
    endif()
    if(flavour STREQUAL "Current" AND NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/Source/DSP/FaderKernel.cpp")
        continue()
    endif()
    set(target HamloFader_F2${flavour})
    if(flavour STREQUAL "Reference")
        set(reference 1)
    else()
        set(reference 0)
    endif()
    add_custom_target(${target}Identity
        COMMAND "${CMAKE_COMMAND}" -E make_directory "${CMAKE_CURRENT_BINARY_DIR}/f2_${flavour}"
        COMMAND "${CMAKE_COMMAND}" "-DSOURCE_ROOT=${CMAKE_CURRENT_SOURCE_DIR}" "-DREFERENCE_ROOT=${CMAKE_CURRENT_SOURCE_DIR}/out/f2/baseline" "-DREFERENCE=${reference}" "-DIDENTITY_OUTPUT=${CMAKE_CURRENT_BINARY_DIR}/f2_${flavour}/FaderF2Identity.h" -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/WriteF2Identity.cmake")
    add_executable(${target} EXCLUDE_FROM_ALL Tests/F2LabMain.cpp Tests/AllocationAudit.cpp)
    if(reference)
        target_sources(${target} PRIVATE out/f2/baseline/Source/DSP/FaderGain.cpp out/f2/baseline/Source/DSP/LegacyFaderCore.cpp)
        target_include_directories(${target} BEFORE PRIVATE out/f2/baseline/Source)
    else()
        target_sources(${target} PRIVATE Source/DSP/FaderKernel.cpp Source/DSP/FaderMeter.cpp)
    endif()
    add_dependencies(${target} ${target}Identity)
    target_include_directories(${target} PRIVATE Source Tests "${CMAKE_CURRENT_BINARY_DIR}/f2_${flavour}")
    target_compile_definitions(${target} PRIVATE FADER_F2_REFERENCE=${reference} JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0)
    target_link_libraries(${target} PRIVATE juce::juce_core juce::juce_cryptography juce::juce_recommended_config_flags)
endforeach()

# F3 paired host compatibility; frozen F1/F2 methods remain unchanged.
add_custom_target(HamloFader_F3Identity
    COMMAND "${CMAKE_COMMAND}" "-DSOURCE_ROOT=${CMAKE_CURRENT_SOURCE_DIR}" "-DIDENTITY_OUTPUT=${CMAKE_CURRENT_BINARY_DIR}/FaderF3Identity.h" -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/WriteF3Identity.cmake")
add_executable(HamloFader_F3Legacy EXCLUDE_FROM_ALL Tests/F3LegacyLab.cpp Tests/AllocationAudit.cpp)
add_dependencies(HamloFader_F3Legacy HamloFader_F3Identity)
target_include_directories(HamloFader_F3Legacy PRIVATE "${CMAKE_CURRENT_BINARY_DIR}" Tests)
target_compile_definitions(HamloFader_F3Legacy PRIVATE HPEQ_MODULE_KIND=8 JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_MODAL_LOOPS_PERMITTED=1)
target_link_libraries(HamloFader_F3Legacy PRIVATE HamloFader_Shared HamloFader_Assets juce::juce_audio_utils juce::juce_dsp juce::juce_audio_formats juce::juce_cryptography juce::juce_recommended_config_flags)

add_executable(HamloFader_F3Host EXCLUDE_FROM_ALL Tests/F3HostMain.cpp Tests/AllocationAudit.cpp)
add_dependencies(HamloFader_F3Host HamloFader_F3Identity)
target_include_directories(HamloFader_F3Host PRIVATE "${CMAKE_CURRENT_BINARY_DIR}" Tests)
target_compile_definitions(HamloFader_F3Host PRIVATE HPEQ_MODULE_KIND=8 JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_MODAL_LOOPS_PERMITTED=1)
target_link_libraries(HamloFader_F3Host PRIVATE HamloFader_Shared HamloFader_Assets juce::juce_audio_utils juce::juce_dsp juce::juce_audio_formats juce::juce_cryptography juce::juce_recommended_config_flags)

option(HAMLO_FADER_F3_REGRESSION "Build local frozen Motion regression probe" OFF)
if(HAMLO_FADER_F3_REGRESSION)
    add_executable(HamloFader_F3Regression EXCLUDE_FROM_ALL Tests/F3HostMain.cpp Tests/AllocationAudit.cpp out/f3/candidate_v2/Source/DSP/FaderKernel.cpp)
    add_dependencies(HamloFader_F3Regression HamloFader_F3Identity)
    target_include_directories(HamloFader_F3Regression PRIVATE "${CMAKE_CURRENT_BINARY_DIR}" Tests)
    target_compile_definitions(HamloFader_F3Regression PRIVATE FADER_F3_REGRESSION=1 JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_MODAL_LOOPS_PERMITTED=1)
    target_link_libraries(HamloFader_F3Regression PRIVATE HamloFader_Shared HamloFader_Assets juce::juce_audio_utils juce::juce_dsp juce::juce_audio_formats juce::juce_cryptography juce::juce_recommended_config_flags)
endif()
