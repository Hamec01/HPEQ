cmake_minimum_required(VERSION 3.22)

project(HamloComp VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# --- JUCE -------------------------------------------------------------------------------------
# The same JUCE version as HPEQ (the compressor came from there). A local checkout avoids a download.
set(HAMLOCOMP_JUCE_VERSION 8.0.12)
set(HAMLOCOMP_JUCE_DIR "" CACHE PATH "Local JUCE ${HAMLOCOMP_JUCE_VERSION} checkout (empty = FetchContent)")

if (HAMLOCOMP_JUCE_DIR)
    add_subdirectory("${HAMLOCOMP_JUCE_DIR}" "${CMAKE_BINARY_DIR}/JUCE")
    set(HAMLOCOMP_JUCE_SOURCE "${HAMLOCOMP_JUCE_DIR}")
else()
    include(FetchContent)
    FetchContent_Declare(JUCE
        GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
        GIT_TAG ${HAMLOCOMP_JUCE_VERSION}
        GIT_SHALLOW TRUE)
    FetchContent_MakeAvailable(JUCE)
    set(HAMLOCOMP_JUCE_SOURCE "${juce_SOURCE_DIR}")
endif()

# JUCE 8 creates a Direct2D / D3D11 device per Windows plug-in DLL; FL Studio may unload the plug-in while
# transport runs and some NVIDIA drivers hang tearing that device down. The patch adds compile switches
# (HPEQ_JUCE_DEFAULT_*, named after the project they came from) so this target can default to the GDI
# software renderer. A local JUCE must already carry the patch (HPEQ's fetched copy does).
if (WIN32 AND NOT HAMLOCOMP_JUCE_DIR)
    include(cmake/PatchJuceWindowsRenderer.cmake)
    hpeq_patch_juce_windows_renderer("${HAMLOCOMP_JUCE_SOURCE}")
endif()

enable_testing()

# --- Assets: the owner's Black Steel hardware and scratched pictures, embedded in the binary ----------
file(GLOB HAMLOCOMP_PICTURES CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/Assets/scratches/*.png
     ${CMAKE_CURRENT_SOURCE_DIR}/Assets/hardware/*.png)
juce_add_binary_data(HamloComp_Assets NAMESPACE HamloCompAssets HEADER_NAME BinaryData.h SOURCES ${HAMLOCOMP_PICTURES})
set_target_properties(HamloComp_Assets PROPERTIES POSITION_INDEPENDENT_CODE ON)

# --- Shared code: DSP, page, processor (the plug-in and the tests link the same objects) ----------------
# Only JUCE's headers here: the JUCE module code is compiled once into each executable / plug-in below.
add_library(HamloComp_Shared STATIC
    Source/DSP/Compressor.cpp
    Source/UI/BlackSteel.cpp
    Source/UI/HamloCompPanel.cpp
    Source/UI/ModuleHelp.cpp
    Source/Plugin/Presets.cpp
    Source/Plugin/HamloCompProcessor.cpp
    Source/Plugin/HamloCompEditor.cpp)
target_include_directories(HamloComp_Shared
    PUBLIC Source
    PRIVATE
        "${HAMLOCOMP_JUCE_SOURCE}/modules"
        "${CMAKE_CURRENT_BINARY_DIR}/juce_binarydata_HamloComp_Assets/JuceLibraryCode")
target_compile_definitions(HamloComp_Shared
    PUBLIC
        JUCE_GLOBAL_MODULE_SETTINGS_INCLUDED=1
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0)
target_link_libraries(HamloComp_Shared
    PRIVATE
        juce::juce_recommended_config_flags
        juce::juce_recommended_warning_flags)

# --- The plug-in --------------------------------------------------------------------------------
# Identity = HPEQ's HamloComp module VST3 (manufacturer BBLB, code HmCp, bundle id), so projects saved with
# that plug-in open this one.
juce_add_plugin(HamloComp
    COMPANY_NAME "HamloProd"
    BUNDLE_ID com.hamloprod.hamlocomp
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE
    PLUGIN_MANUFACTURER_CODE BBLB
    PLUGIN_CODE HmCp
    VST3_CATEGORIES Fx Dynamics
    FORMATS VST3
    PRODUCT_NAME "HamloComp")
target_sources(HamloComp PRIVATE Source/Plugin/PluginEntry.cpp)
target_compile_definitions(HamloComp PRIVATE
    HPEQ_JUCE_DEFAULT_IMAGE_TYPE=SoftwareImageType
    HPEQ_JUCE_DEFAULT_RENDERING_ENGINE=0
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0)
target_link_libraries(HamloComp
    PRIVATE
        HamloComp_Shared
        HamloComp_Assets
        juce::juce_audio_utils
        juce::juce_recommended_config_flags
        juce::juce_recommended_warning_flags)

# --- Tests ----------------------------------------------------------------------------------------
add_executable(HamloComp_Tests
    Tests/TestMain.cpp
    Tests/CompressorTests.cpp
    Tests/PluginTests.cpp)
target_compile_definitions(HamloComp_Tests PRIVATE JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_MODAL_LOOPS_PERMITTED=1)
target_link_libraries(HamloComp_Tests
    PRIVATE
        HamloComp_Shared
        HamloComp_Assets
        juce::juce_audio_utils
        juce::juce_recommended_config_flags
        juce::juce_recommended_warning_flags)
add_test(NAME HamloComp_Tests COMMAND HamloComp_Tests)

# Reference check: hosts a reference HamloComp.vst3 (HPEQ's module build) and this processor side by side on
# the same signals and settings, and reports the difference (docs/reports).
add_executable(HamloComp_Reference Tests/ReferenceMain.cpp)
target_compile_definitions(HamloComp_Reference PRIVATE JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_PLUGINHOST_VST3=1 JUCE_MODAL_LOOPS_PERMITTED=1)
target_link_libraries(HamloComp_Reference
    PRIVATE
        HamloComp_Shared
        HamloComp_Assets
        juce::juce_audio_utils
        juce::juce_recommended_config_flags
        juce::juce_recommended_warning_flags)
