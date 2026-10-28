cmake_minimum_required(VERSION 3.22)

project(HamloRedWall VERSION 0.3.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# --- JUCE -------------------------------------------------------------------------------------
# The same JUCE version as HPEQ (the limiter came from there). A local checkout avoids a download.
set(HAMLOREDWALL_JUCE_VERSION 8.0.12)
set(HAMLOREDWALL_JUCE_DIR "" CACHE PATH "Local JUCE ${HAMLOREDWALL_JUCE_VERSION} checkout (empty = FetchContent)")

if (HAMLOREDWALL_JUCE_DIR)
    add_subdirectory("${HAMLOREDWALL_JUCE_DIR}" "${CMAKE_BINARY_DIR}/JUCE")
    set(HAMLOREDWALL_JUCE_SOURCE "${HAMLOREDWALL_JUCE_DIR}")
else()
    include(FetchContent)
    FetchContent_Declare(JUCE
        GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
        GIT_TAG ${HAMLOREDWALL_JUCE_VERSION}
        GIT_SHALLOW TRUE)
    FetchContent_MakeAvailable(JUCE)
    set(HAMLOREDWALL_JUCE_SOURCE "${juce_SOURCE_DIR}")
endif()

# JUCE 8 creates a Direct2D / D3D11 device per Windows plug-in DLL; FL Studio may unload the plug-in while
# transport runs and some NVIDIA drivers hang tearing that device down. The patch adds compile switches
# (HPEQ_JUCE_DEFAULT_*, named after the project they came from) so this target can default to the GDI
# software renderer. A local JUCE must already carry the patch (HPEQ's fetched copy does).
if (WIN32 AND NOT HAMLOREDWALL_JUCE_DIR)
    include(cmake/PatchJuceWindowsRenderer.cmake)
    hpeq_patch_juce_windows_renderer("${HAMLOREDWALL_JUCE_SOURCE}")
endif()

enable_testing()

# --- Assets: the owner's Black Steel hardware and scratched pictures, embedded in the binary ----------
file(GLOB HAMLOREDWALL_PICTURES CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/Assets/scratches/*.png
     ${CMAKE_CURRENT_SOURCE_DIR}/Assets/hardware/*.png)
juce_add_binary_data(HamloRedWall_Assets NAMESPACE HamloRedWallAssets HEADER_NAME BinaryData.h SOURCES ${HAMLOREDWALL_PICTURES})
set_target_properties(HamloRedWall_Assets PROPERTIES POSITION_INDEPENDENT_CODE ON)

# --- Shared code: DSP, page, processor (the plug-in and the tests link the same objects) ----------------
# Only JUCE's headers here: the JUCE module code is compiled once into each executable / plug-in below.
add_library(HamloRedWall_Shared STATIC
    Source/DSP/HamloRedWall.cpp
    Source/DSP/Limiter.cpp
    Source/DSP/RedWallColour.cpp
    Source/UI/BlackSteel.cpp
    Source/UI/HamloRedWallPanel.cpp
    Source/UI/ModuleHelp.cpp
    Source/UI/PresetScreen.cpp
    Source/Plugin/Presets.cpp
    Source/Plugin/HamloRedWallProcessor.cpp
    Source/Plugin/HamloRedWallEditor.cpp)
target_include_directories(HamloRedWall_Shared
    PUBLIC Source
    PRIVATE
        "${HAMLOREDWALL_JUCE_SOURCE}/modules"
        "${CMAKE_CURRENT_BINARY_DIR}/juce_binarydata_HamloRedWall_Assets/JuceLibraryCode")
target_compile_definitions(HamloRedWall_Shared
    PUBLIC
        JUCE_GLOBAL_MODULE_SETTINGS_INCLUDED=1
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0)
target_link_libraries(HamloRedWall_Shared
    PRIVATE
        juce::juce_recommended_config_flags
        juce::juce_recommended_warning_flags)

# --- The plug-in --------------------------------------------------------------------------------
# Identity = HPEQ's HamloRedWall module VST3 (manufacturer BBLB, code HmRw, bundle id), so projects saved with
# that plug-in open this one.
juce_add_plugin(HamloRedWall
    COMPANY_NAME "HamloProd"
    BUNDLE_ID com.hamloprod.hamloredwall
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE
    PLUGIN_MANUFACTURER_CODE BBLB
    PLUGIN_CODE HmRw
    VST3_CATEGORIES Fx Dynamics
    FORMATS VST3
    PRODUCT_NAME "HamloRedWall")
target_sources(HamloRedWall PRIVATE Source/Plugin/PluginEntry.cpp)
target_compile_definitions(HamloRedWall PRIVATE
    HPEQ_JUCE_DEFAULT_IMAGE_TYPE=SoftwareImageType
    HPEQ_JUCE_DEFAULT_RENDERING_ENGINE=0
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0)
target_link_libraries(HamloRedWall
    PRIVATE
        HamloRedWall_Shared
        HamloRedWall_Assets
        juce::juce_audio_utils
        juce::juce_dsp
        juce::juce_recommended_config_flags
        juce::juce_recommended_warning_flags)

# --- Tests ----------------------------------------------------------------------------------------
add_executable(HamloRedWall_Tests
    Tests/TestMain.cpp
    Tests/LimiterTests.cpp
    Tests/PluginTests.cpp)
target_compile_definitions(HamloRedWall_Tests PRIVATE JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_MODAL_LOOPS_PERMITTED=1)
target_link_libraries(HamloRedWall_Tests
    PRIVATE
        HamloRedWall_Shared
        HamloRedWall_Assets
        juce::juce_audio_utils
        juce::juce_dsp
        juce::juce_recommended_config_flags
        juce::juce_recommended_warning_flags)
add_test(NAME HamloRedWall_Tests COMMAND HamloRedWall_Tests)

# Reference check: hosts a reference HamloRedWall.vst3 (HPEQ's module build) and this processor side by side on
# the same signals and settings, and reports the difference (docs/reports).
add_executable(HamloRedWall_Reference Tests/ReferenceMain.cpp)
target_compile_definitions(HamloRedWall_Reference PRIVATE JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_PLUGINHOST_VST3=1 JUCE_MODAL_LOOPS_PERMITTED=1)
target_link_libraries(HamloRedWall_Reference
    PRIVATE
        HamloRedWall_Shared
        HamloRedWall_Assets
        juce::juce_audio_utils
        juce::juce_dsp
        juce::juce_recommended_config_flags
        juce::juce_recommended_warning_flags)

# The page as a PNG (docs and visual checks).
add_executable(HamloRedWall_Snapshot Tests/SnapshotMain.cpp)
target_compile_definitions(HamloRedWall_Snapshot PRIVATE JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0)
target_link_libraries(HamloRedWall_Snapshot
    PRIVATE
        HamloRedWall_Shared
        HamloRedWall_Assets
        juce::juce_gui_basics
        juce::juce_audio_utils
        juce::juce_dsp
        juce::juce_recommended_config_flags
        juce::juce_recommended_warning_flags)


# Measurements against docs/REDWALL_V2.md (the "before" and "after" runs use this tool unchanged).
add_executable(HamloRedWall_Lab Tests/LabMain.cpp)
target_compile_definitions(HamloRedWall_Lab PRIVATE JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0)
target_link_libraries(HamloRedWall_Lab
    PRIVATE
        HamloRedWall_Shared
        HamloRedWall_Assets
        juce::juce_audio_utils
        juce::juce_dsp
        juce::juce_audio_formats
        juce::juce_recommended_config_flags
        juce::juce_recommended_warning_flags)

# Genre presets on their material: LOUDNESS calibration and the check (docs/GENRE_PRESETS.md).
add_executable(HamloRedWall_PresetLab Tests/PresetLabMain.cpp)
target_compile_definitions(HamloRedWall_PresetLab PRIVATE JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0)
target_link_libraries(HamloRedWall_PresetLab
    PRIVATE
        HamloRedWall_Shared
        HamloRedWall_Assets
        juce::juce_audio_utils
        juce::juce_dsp
        juce::juce_audio_formats
        juce::juce_recommended_config_flags
        juce::juce_recommended_warning_flags)
