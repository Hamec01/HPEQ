cmake_minimum_required(VERSION 3.22)

project(HamloUdarnik VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# --- JUCE -------------------------------------------------------------------------------------
# The same JUCE version as HPEQ (the transient shaper came from there). A local checkout avoids a download.
set(HAMLOUDARNIK_JUCE_VERSION 8.0.12)
set(HAMLOUDARNIK_JUCE_DIR "" CACHE PATH "Local JUCE ${HAMLOUDARNIK_JUCE_VERSION} checkout (empty = FetchContent)")

if (HAMLOUDARNIK_JUCE_DIR)
    add_subdirectory("${HAMLOUDARNIK_JUCE_DIR}" "${CMAKE_BINARY_DIR}/JUCE")
    set(HAMLOUDARNIK_JUCE_SOURCE "${HAMLOUDARNIK_JUCE_DIR}")
else()
    include(FetchContent)
    FetchContent_Declare(JUCE
        GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
        GIT_TAG ${HAMLOUDARNIK_JUCE_VERSION}
        GIT_SHALLOW TRUE)
    FetchContent_MakeAvailable(JUCE)
    set(HAMLOUDARNIK_JUCE_SOURCE "${juce_SOURCE_DIR}")
endif()

# JUCE 8 creates a Direct2D / D3D11 device per Windows plug-in DLL; FL Studio may unload the plug-in while
# transport runs and some NVIDIA drivers hang tearing that device down. The patch adds compile switches
# (HPEQ_JUCE_DEFAULT_*, named after the project they came from) so this target can default to the GDI
# software renderer. A local JUCE must already carry the patch (HPEQ's fetched copy does).
if (WIN32 AND NOT HAMLOUDARNIK_JUCE_DIR)
    include(cmake/PatchJuceWindowsRenderer.cmake)
    hpeq_patch_juce_windows_renderer("${HAMLOUDARNIK_JUCE_SOURCE}")
endif()

enable_testing()

# --- Assets: the owner's Black Steel hardware and scratched pictures, embedded in the binary ----------
file(GLOB HAMLOUDARNIK_PICTURES CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/Assets/scratches/*.png
     ${CMAKE_CURRENT_SOURCE_DIR}/Assets/hardware/*.png)
juce_add_binary_data(HamloUdarnik_Assets NAMESPACE HamloUdarnikAssets HEADER_NAME BinaryData.h SOURCES ${HAMLOUDARNIK_PICTURES})
set_target_properties(HamloUdarnik_Assets PROPERTIES POSITION_INDEPENDENT_CODE ON)

# --- Shared code: DSP, page, processor (the plug-in and the tests link the same objects) ----------------
# Only JUCE's headers here: the JUCE module code is compiled once into each executable / plug-in below.
add_library(HamloUdarnik_Shared STATIC
    Source/DSP/HamloUdarnik.cpp
    Source/DSP/TransientShaper.cpp
    Source/DSP/UdarnikV2.cpp
    Source/UI/BlackSteel.cpp
    Source/UI/HamloUdarnikPanel.cpp
    Source/UI/ModuleHelp.cpp
    Source/Plugin/Presets.cpp
    Source/Plugin/HamloUdarnikProcessor.cpp
    Source/Plugin/HamloUdarnikEditor.cpp)
target_include_directories(HamloUdarnik_Shared
    PUBLIC Source
    PRIVATE
        "${HAMLOUDARNIK_JUCE_SOURCE}/modules"
        "${CMAKE_CURRENT_BINARY_DIR}/juce_binarydata_HamloUdarnik_Assets/JuceLibraryCode")
target_compile_definitions(HamloUdarnik_Shared
    PUBLIC
        JUCE_GLOBAL_MODULE_SETTINGS_INCLUDED=1
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0)
target_link_libraries(HamloUdarnik_Shared
    PRIVATE
        juce::juce_recommended_config_flags
        juce::juce_recommended_warning_flags)

# --- The plug-in --------------------------------------------------------------------------------
# Identity = HPEQ's HamloUdarnik module VST3 (manufacturer BBLB, code HmUd, bundle id), so projects saved with
# that plug-in open this one.
juce_add_plugin(HamloUdarnik
    COMPANY_NAME "HamloProd"
    BUNDLE_ID com.hamloprod.hamloudarnik
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE
    PLUGIN_MANUFACTURER_CODE BBLB
    PLUGIN_CODE HmUd
    VST3_CATEGORIES Fx Dynamics
    FORMATS VST3
    PRODUCT_NAME "HamloUdarnik")
target_sources(HamloUdarnik PRIVATE Source/Plugin/PluginEntry.cpp)
target_compile_definitions(HamloUdarnik PRIVATE
    HPEQ_JUCE_DEFAULT_IMAGE_TYPE=SoftwareImageType
    HPEQ_JUCE_DEFAULT_RENDERING_ENGINE=0
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0)
target_link_libraries(HamloUdarnik
    PRIVATE
        HamloUdarnik_Shared
        HamloUdarnik_Assets
        juce::juce_audio_utils
        juce::juce_recommended_config_flags
        juce::juce_recommended_warning_flags)

# --- Tests ----------------------------------------------------------------------------------------
add_executable(HamloUdarnik_Tests
    Tests/TestMain.cpp
    Tests/ShaperTests.cpp
    Tests/PluginTests.cpp
    Tests/V2Tests.cpp)
target_compile_definitions(HamloUdarnik_Tests PRIVATE JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_MODAL_LOOPS_PERMITTED=1)
target_link_libraries(HamloUdarnik_Tests
    PRIVATE
        HamloUdarnik_Shared
        HamloUdarnik_Assets
        juce::juce_audio_utils
        juce::juce_recommended_config_flags
        juce::juce_recommended_warning_flags)
add_test(NAME HamloUdarnik_Tests COMMAND HamloUdarnik_Tests)

# Reference check: hosts a reference HamloUdarnik.vst3 (HPEQ's module build) and this processor side by side on
# the same signals and settings, and reports the difference (docs/reports).
add_executable(HamloUdarnik_Reference Tests/ReferenceMain.cpp)
target_compile_definitions(HamloUdarnik_Reference PRIVATE JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_PLUGINHOST_VST3=1 JUCE_MODAL_LOOPS_PERMITTED=1)
target_link_libraries(HamloUdarnik_Reference
    PRIVATE
        HamloUdarnik_Shared
        HamloUdarnik_Assets
        juce::juce_audio_utils
        juce::juce_recommended_config_flags
        juce::juce_recommended_warning_flags)

# The page as a PNG (docs and visual checks).
add_executable(HamloUdarnik_Snapshot Tests/SnapshotMain.cpp)
target_compile_definitions(HamloUdarnik_Snapshot PRIVATE JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0)
target_link_libraries(HamloUdarnik_Snapshot
    PRIVATE
        HamloUdarnik_Shared
        HamloUdarnik_Assets
        juce::juce_gui_basics
        juce::juce_recommended_config_flags
        juce::juce_recommended_warning_flags)


# Measurements against docs/UDARNIK_V2.md (the "before" and "after" runs use this tool unchanged).
add_executable(HamloUdarnik_Lab Tests/LabMain.cpp)
target_compile_definitions(HamloUdarnik_Lab PRIVATE JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0)
target_link_libraries(HamloUdarnik_Lab
    PRIVATE
        HamloUdarnik_Shared
        HamloUdarnik_Assets
        juce::juce_audio_utils
        juce::juce_audio_formats
        juce::juce_recommended_config_flags
        juce::juce_recommended_warning_flags)
