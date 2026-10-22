cmake_minimum_required(VERSION 3.22)

project(HPEQ VERSION 0.0.1 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# --- JUCE -------------------------------------------------------------------------------------
# A local checkout (e.g. the one inside HPDG) avoids a download; otherwise the pinned version is
# fetched. Both must be the same JUCE version HPDG ships with, so the two plugins stay comparable.
set(HPEQ_JUCE_VERSION 8.0.12)
set(HPEQ_JUCE_DIR "" CACHE PATH "Local JUCE ${HPEQ_JUCE_VERSION} checkout (empty = FetchContent)")

if (HPEQ_JUCE_DIR)
    add_subdirectory("${HPEQ_JUCE_DIR}" "${CMAKE_BINARY_DIR}/JUCE")
else()
    include(FetchContent)
    FetchContent_Declare(JUCE
        GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
        GIT_TAG ${HPEQ_JUCE_VERSION}
        GIT_SHALLOW TRUE)
    FetchContent_MakeAvailable(JUCE)
endif()

# Read-only HPDG sources: the M0 harness compiles a few HPDG DSP blocks from here to measure them.
# Nothing is copied into this repository and nothing in HPDG is modified.
set(HPEQ_HPDG_SOURCE_DIR "" CACHE PATH "HPDG Source/ directory for M0 measurements (optional)")

enable_testing()

# --- Plugin -----------------------------------------------------------------------------------
juce_add_plugin(HPEQ
    COMPANY_NAME "HamloProd"
    BUNDLE_ID com.hamloprod.hpeq
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE
    # Same vendor code as HPDG (BBLB), different plugin code: HPDG is BBG1.
    PLUGIN_MANUFACTURER_CODE BBLB
    PLUGIN_CODE HpEq
    VST3_CATEGORIES Fx EQ
    FORMATS VST3
    PRODUCT_NAME "HPEQ")

target_compile_definitions(HPEQ
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_DISPLAY_SPLASH_SCREEN=0)

target_sources(HPEQ
    PRIVATE
        Source/Analysis/SpectrumAnalyzer.cpp
        Source/Core/FxProjectState.cpp
        Source/DSP/Biquad.cpp
        Source/DSP/FxNodeProcessor.cpp
        Source/DSP/LayerEngine.cpp
        Source/DSP/LayerSnapshot.cpp
        Source/DSP/PreEqProcessor.cpp
        Source/DSP/Processors.cpp
        Source/DSP/ReverbProcessor.cpp
        Source/DSP/ProcessorsM6.cpp
        Source/DSP/ProjectEngine.cpp
        Source/DSP/CrossoverTree.cpp
        Source/DSP/EqDesign.cpp
        Source/Plugin/FxPluginProcessor.cpp
        Source/Plugin/FxPluginEditor.cpp
        Source/UI/AnalysisModel.cpp
        Source/UI/NodeInspector.cpp
        Source/UI/LayerControls.cpp
        Source/UI/SpectrumWorkspace.cpp)

target_include_directories(HPEQ PUBLIC Source)

target_link_libraries(HPEQ
    PUBLIC
        # Test executables link the plugin's shared code; these modules are compiled once, here.
        juce::juce_audio_utils
        juce::juce_dsp
        juce::juce_audio_formats
        juce::juce_cryptography
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)

# --- Measurement harness (shared by the M0 tool and the unit tests) ----------------------------
set(HPEQ_HARNESS_SOURCES
    Tests/Harness/TestSignalFactory.cpp
    Tests/Harness/AudioMetrics.cpp
    Tests/Harness/AudioFileIO.cpp
    Tests/Harness/RtAllocationProbe.cpp
    Tests/Harness/MachineInfo.cpp
    Tests/Harness/SvgPlot.cpp)

# Unit tests: harness self-checks, plugin contract (M1), state schema.
add_executable(HPEQ_Tests
    ${HPEQ_HARNESS_SOURCES}
    Tests/Unit/TestMain.cpp
    Tests/Unit/HarnessTests.cpp
    Tests/Unit/PluginContractTests.cpp
    Tests/Unit/StateTests.cpp
    Tests/Unit/DspTests.cpp
    Tests/Unit/EqTests.cpp
    Tests/Unit/DynamicsReverbTests.cpp
    Tests/Unit/M6Tests.cpp
    Tests/Unit/M7Tests.cpp
    Tests/Unit/UiAnalysisTests.cpp)
target_include_directories(HPEQ_Tests PRIVATE Tests)
target_link_libraries(HPEQ_Tests PRIVATE HPEQ)
add_test(NAME HPEQ_Tests COMMAND HPEQ_Tests)

# M2 lab: H1 vs H2 frequency-core experiment on the real engine (JSON, SVG, WAV).
add_executable(HPEQ_Lab
    ${HPEQ_HARNESS_SOURCES}
    Tests/Tools/LabMain.cpp
    Tests/Tools/LabM3.cpp
    Tests/Tools/LabM4.cpp
    Tests/Tools/LabM6.cpp
    Tests/Tools/LabM7.cpp
    Tests/Tools/LabBench.cpp
    Tests/Tools/LabUi.cpp)
target_include_directories(HPEQ_Lab PRIVATE Tests)
target_link_libraries(HPEQ_Lab PRIVATE HPEQ)

# M0 tool: fixtures, baseline metrics, HPDG block audit.
add_executable(HPEQ_Harness
    ${HPEQ_HARNESS_SOURCES}
    Tests/Tools/HarnessMain.cpp
    Tests/Tools/HpdgBlockAudit.cpp)
target_include_directories(HPEQ_Harness PRIVATE Tests)
target_link_libraries(HPEQ_Harness
    PRIVATE
        juce::juce_audio_formats
        juce::juce_cryptography
        juce::juce_dsp
        juce::juce_recommended_config_flags
        juce::juce_recommended_warning_flags)
target_compile_definitions(HPEQ_Harness PRIVATE JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0)

if (HPEQ_HPDG_SOURCE_DIR)
    target_sources(HPEQ_Harness PRIVATE
        "${HPEQ_HPDG_SOURCE_DIR}/Audio/DrumCompressorBlock.cpp"
        "${HPEQ_HPDG_SOURCE_DIR}/Audio/MonstaFxBlock.cpp")
    target_include_directories(HPEQ_Harness PRIVATE "${HPEQ_HPDG_SOURCE_DIR}")
    target_compile_definitions(HPEQ_Harness PRIVATE HPEQ_WITH_HPDG_BLOCKS=1)
endif()

add_test(NAME HPEQ_Harness_SelfCheck COMMAND HPEQ_Harness selfcheck)

# M1 host smoke: loads built .vst3 binaries through JUCE's VST3 host, as a DAW scan would.
add_executable(HPEQ_HostSmoke
    ${HPEQ_HARNESS_SOURCES}
    Tests/Tools/HostSmokeMain.cpp)
target_include_directories(HPEQ_HostSmoke PRIVATE Tests)
target_link_libraries(HPEQ_HostSmoke
    PRIVATE
        juce::juce_audio_processors
        juce::juce_audio_formats
        juce::juce_cryptography
        juce::juce_dsp
        juce::juce_gui_basics
        juce::juce_recommended_config_flags
        juce::juce_recommended_warning_flags)
target_compile_definitions(HPEQ_HostSmoke PRIVATE
    JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_PLUGINHOST_VST3=1 JUCE_MODAL_LOOPS_PERMITTED=1)
