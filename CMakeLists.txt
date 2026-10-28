cmake_minimum_required(VERSION 3.22)

project(HamloMag VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# --- JUCE -------------------------------------------------------------------------------------
# The same JUCE version as HPEQ (the saturator came from there). A local checkout avoids a download.
set(HAMLOMAG_JUCE_VERSION 8.0.12)
set(HAMLOMAG_JUCE_DIR "" CACHE PATH "Local JUCE ${HAMLOMAG_JUCE_VERSION} checkout (empty = FetchContent)")

if (HAMLOMAG_JUCE_DIR)
    add_subdirectory("${HAMLOMAG_JUCE_DIR}" "${CMAKE_BINARY_DIR}/JUCE")
    set(HAMLOMAG_JUCE_SOURCE "${HAMLOMAG_JUCE_DIR}")
else()
    include(FetchContent)
    FetchContent_Declare(JUCE
        GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
        GIT_TAG ${HAMLOMAG_JUCE_VERSION}
        GIT_SHALLOW TRUE)
    FetchContent_MakeAvailable(JUCE)
    set(HAMLOMAG_JUCE_SOURCE "${juce_SOURCE_DIR}")
endif()

# JUCE 8 creates a Direct2D / D3D11 device per Windows plug-in DLL; FL Studio may unload the plug-in while
# transport runs and some NVIDIA drivers hang tearing that device down. The patch adds compile switches
# (HPEQ_JUCE_DEFAULT_*, named after the project they came from) so this target can default to the GDI
# software renderer. A local JUCE must already carry the patch (HPEQ's fetched copy does).
if (WIN32 AND NOT HAMLOMAG_JUCE_DIR)
    include(cmake/PatchJuceWindowsRenderer.cmake)
    hpeq_patch_juce_windows_renderer("${HAMLOMAG_JUCE_SOURCE}")
endif()

enable_testing()

# --- Assets: the owner's Black Steel hardware and scratched pictures, embedded in the binary ----------
file(GLOB HAMLOMAG_PICTURES CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/Assets/scratches/*.png
     ${CMAKE_CURRENT_SOURCE_DIR}/Assets/hardware/*.png)
juce_add_binary_data(HamloMag_Assets NAMESPACE HamloMagAssets HEADER_NAME BinaryData.h SOURCES ${HAMLOMAG_PICTURES})
set_target_properties(HamloMag_Assets PROPERTIES POSITION_INDEPENDENT_CODE ON)

# --- Shared code: DSP, page, processor (the plug-in and the tests link the same objects) ----------------
# Only JUCE's headers here: the JUCE module code is compiled once into each executable / plug-in below.
add_library(HamloMag_Shared STATIC
    Source/DSP/HamloMag.cpp
    Source/DSP/Saturator.cpp
    Source/UI/BlackSteel.cpp
    Source/UI/HamloMagPanel.cpp
    Source/UI/ModuleHelp.cpp
    Source/UI/PresetScreen.cpp
    Source/Plugin/Presets.cpp
    Source/Plugin/HamloMagProcessor.cpp
    Source/Plugin/HamloMagEditor.cpp)
target_include_directories(HamloMag_Shared
    PUBLIC Source
    PRIVATE
        "${HAMLOMAG_JUCE_SOURCE}/modules"
        "${CMAKE_CURRENT_BINARY_DIR}/juce_binarydata_HamloMag_Assets/JuceLibraryCode")
target_compile_definitions(HamloMag_Shared
    PUBLIC
        JUCE_GLOBAL_MODULE_SETTINGS_INCLUDED=1
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0)
target_link_libraries(HamloMag_Shared
    PRIVATE
        juce::juce_recommended_config_flags
        juce::juce_recommended_warning_flags)

# --- The plug-in --------------------------------------------------------------------------------
# Identity = HPEQ's HamloMag module VST3 (manufacturer BBLB, code HmRw, bundle id), so projects saved with
# that plug-in open this one.
juce_add_plugin(HamloMag
    COMPANY_NAME "HamloProd"
    BUNDLE_ID com.hamloprod.hamlomag
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE
    PLUGIN_MANUFACTURER_CODE BBLB
    PLUGIN_CODE HmMg
    VST3_CATEGORIES Fx Distortion
    FORMATS VST3
    PRODUCT_NAME "HamloMag")
target_sources(HamloMag PRIVATE Source/Plugin/PluginEntry.cpp)
target_compile_definitions(HamloMag PRIVATE
    HPEQ_JUCE_DEFAULT_IMAGE_TYPE=SoftwareImageType
    HPEQ_JUCE_DEFAULT_RENDERING_ENGINE=0
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0)
target_link_libraries(HamloMag
    PRIVATE
        HamloMag_Shared
        HamloMag_Assets
        juce::juce_audio_utils
        juce::juce_dsp
        juce::juce_recommended_config_flags
        juce::juce_recommended_warning_flags)

# --- Tests ----------------------------------------------------------------------------------------
add_executable(HamloMag_Tests
    Tests/TestMain.cpp
    Tests/SaturatorTests.cpp
    Tests/PluginTests.cpp)
target_compile_definitions(HamloMag_Tests PRIVATE JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_MODAL_LOOPS_PERMITTED=1)
target_link_libraries(HamloMag_Tests
    PRIVATE
        HamloMag_Shared
        HamloMag_Assets
        juce::juce_audio_utils
        juce::juce_dsp
        juce::juce_recommended_config_flags
        juce::juce_recommended_warning_flags)
add_test(NAME HamloMag_Tests COMMAND HamloMag_Tests)

# The page as a PNG (docs and visual checks).
add_executable(HamloMag_Snapshot Tests/SnapshotMain.cpp)
target_compile_definitions(HamloMag_Snapshot PRIVATE JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0)
target_link_libraries(HamloMag_Snapshot
    PRIVATE
        HamloMag_Shared
        HamloMag_Assets
        juce::juce_gui_basics
        juce::juce_audio_utils
        juce::juce_dsp
        juce::juce_recommended_config_flags
        juce::juce_recommended_warning_flags)


