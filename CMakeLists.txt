cmake_minimum_required(VERSION 3.22)

project(HPEQ VERSION 0.1.0 LANGUAGES C CXX)

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

# JUCE 8 creates a Direct2D/D3D11 device for every Windows plug-in DLL. FL Studio may unload an
# individual module while transport is running, and some NVIDIA drivers wait indefinitely while
# that per-DLL device is torn down. Add opt-in compile switches to the fetched JUCE sources; only
# the standalone module targets below enable them, leaving the original HPEQ target unchanged.
if (WIN32 AND NOT HPEQ_JUCE_DIR)
    include(cmake/PatchJuceWindowsRenderer.cmake)
    hpeq_patch_juce_windows_renderer("${juce_SOURCE_DIR}")
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
        # As the module plug-ins: GDI software rendering and software images. With JUCE 8's Direct2D
        # default, FL Studio hung when it closed / unloaded HPEQ (the per-DLL D3D11 device teardown waits
        # inside the GPU driver). The editor also switches its peer at runtime (FxPluginEditor).
        HPEQ_JUCE_DEFAULT_IMAGE_TYPE=SoftwareImageType
        HPEQ_JUCE_DEFAULT_RENDERING_ENGINE=0)

target_sources(HPEQ
    PRIVATE
        Source/Analysis/SpectrumAnalyzer.cpp
        Source/Analysis/LoudnessMeter.cpp
        Source/Core/FxProjectState.cpp
        Source/Core/Revolution.cpp
        Source/Core/ModulePresets.cpp
        Source/DSP/Biquad.cpp
        Source/DSP/FxNodeProcessor.cpp
        Source/DSP/LayerEngine.cpp
        Source/DSP/LayerSnapshot.cpp
        Source/DSP/PreEqProcessor.cpp
        Source/DSP/Processors.cpp
        Source/DSP/ReverbProcessor.cpp
        Source/DSP/HamloVerb.cpp
        Source/DSP/HamloEcho.cpp
        Source/DSP/HamloMag.cpp
        Source/DSP/ProcessorsM6.cpp
        Source/DSP/HamloRevolution.cpp
        Source/DSP/TubePreamp.cpp
        Source/DSP/HamloRedWall.cpp
        Source/DSP/HamloUdarnik.cpp
        Source/DSP/HamloZastava.cpp
        Source/DSP/HamloShirota.cpp
        Source/DSP/ProjectEngine.cpp
        Source/DSP/CrossoverTree.cpp
        Source/DSP/EqDesign.cpp
        Source/Plugin/FxPluginProcessor.cpp
        Source/Plugin/FxPluginEditor.cpp
        Source/Plugin/Presets.cpp
        Source/UI/AnalysisModel.cpp
        Source/UI/NodeInspector.cpp
        Source/UI/LayerControls.cpp
        Source/UI/HamloCompPanel.cpp
        Source/UI/BlackSteel.cpp
        Source/UI/HamloVerbPanel.cpp
        Source/UI/HamloEchoPanel.cpp
        Source/UI/HamloMagPanel.cpp
        Source/UI/HamloFaderPanel.cpp
        Source/UI/HamloRedWallPanel.cpp
        Source/UI/HamloUdarnikPanel.cpp
        Source/UI/HamloZastavaPanel.cpp
        Source/UI/ModuleHelp.cpp
        Source/UI/HamloShirotaPanel.cpp
        Source/UI/HamloRevolutionPanel.cpp
        Source/UI/HamloTochkaPanel.cpp
        Source/UI/SteelLookAndFeel.cpp
        Source/UI/ModuleTile.cpp
        Source/UI/TubeButton.cpp
        Source/UI/SpectrumWorkspace.cpp)

target_include_directories(HPEQ PUBLIC Source)

# Owner's scratched USSR pictures (alpha masks), engraved into the Black Steel pages.
file(GLOB HPEQ_SCRATCH_PICTURES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/Assets/scratches/*.png)
# The complete owner-supplied hardware pack is embedded in the same binary-data library, so the
# plugin never depends on files next to the installed .vst3 bundle.
file(GLOB_RECURSE HPEQ_HARDWARE_PICTURES CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/docs/fx-addon/assets/soviet_hardware_pack/*.png)
# BRING LENIN / STALIN BACK: the owner's portraits, dark (off) and red (on)
file(GLOB HPEQ_TUBE_PICTURES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/Assets/tubes/*.png)
# Presets pass: the factory preset library (saved states), with an index of where each file sits
# (pack / category / file); the index is rewritten only when the library changes.
file(GLOB_RECURSE HPEQ_FACTORY_PRESETS CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/Assets/Presets/Factory/*.hpeq)
set(HPEQ_FACTORY_INDEX_TEXT "")
foreach(preset ${HPEQ_FACTORY_PRESETS})
    file(RELATIVE_PATH relative ${CMAKE_CURRENT_SOURCE_DIR}/Assets/Presets/Factory ${preset})
    string(APPEND HPEQ_FACTORY_INDEX_TEXT "${relative}
")
endforeach()
file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/hpeq_factory_presets.txt.new "${HPEQ_FACTORY_INDEX_TEXT}")
configure_file(${CMAKE_CURRENT_BINARY_DIR}/hpeq_factory_presets.txt.new ${CMAKE_CURRENT_BINARY_DIR}/hpeq_factory_presets.txt COPYONLY)
juce_add_binary_data(HPEQ_Assets NAMESPACE HpeqAssets
                     SOURCES ${HPEQ_SCRATCH_PICTURES} ${HPEQ_HARDWARE_PICTURES} ${HPEQ_TUBE_PICTURES}
                             ${HPEQ_FACTORY_PRESETS} ${CMAKE_CURRENT_BINARY_DIR}/hpeq_factory_presets.txt)

target_link_libraries(HPEQ
    PUBLIC
        HPEQ_Assets
        # Test executables link the plugin's shared code; these modules are compiled once, here.
        juce::juce_audio_utils
        juce::juce_dsp
        juce::juce_audio_formats
        juce::juce_cryptography
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)

# --- Standalone module plug-ins ---------------------------------------------------------------
# These targets are additive: HPEQ above keeps its processor, editor, identity and source list.
# Common module code is compiled once and linked into eight small variant targets.
add_library(HPEQ_ModuleShared STATIC
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
    Source/Standalone/ModulePluginEditor.cpp)
target_include_directories(HPEQ_ModuleShared
    PUBLIC Source
    PRIVATE
        "${juce_SOURCE_DIR}/modules"
        "${CMAKE_CURRENT_BINARY_DIR}/juce_binarydata_HPEQ_Assets/JuceLibraryCode")
target_compile_definitions(HPEQ_ModuleShared
    PUBLIC
        JUCE_GLOBAL_MODULE_SETTINGS_INCLUDED=1
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0)
target_link_libraries(HPEQ_ModuleShared
    PRIVATE
        juce::juce_recommended_config_flags
        juce::juce_recommended_warning_flags)

function(hpeq_add_module_plugin target product bundle_id plugin_code kind)
    juce_add_plugin(${target}
        COMPANY_NAME "HamloProd"
        BUNDLE_ID ${bundle_id}
        IS_SYNTH FALSE
        NEEDS_MIDI_INPUT FALSE
        NEEDS_MIDI_OUTPUT FALSE
        IS_MIDI_EFFECT FALSE
        EDITOR_WANTS_KEYBOARD_FOCUS FALSE
        COPY_PLUGIN_AFTER_BUILD TRUE
        VST3_COPY_DIR "${CMAKE_BINARY_DIR}/HamloModules/$<CONFIG>"
        PLUGIN_MANUFACTURER_CODE BBLB
        PLUGIN_CODE ${plugin_code}
        VST3_CATEGORIES Fx
        FORMATS VST3
        PRODUCT_NAME ${product})
    target_sources(${target} PRIVATE Source/Standalone/ModulePluginEntry.cpp)
    target_include_directories(${target} PRIVATE Source)
    target_compile_definitions(${target} PRIVATE
        HPEQ_MODULE_KIND=${kind}
        HPEQ_JUCE_DEFAULT_IMAGE_TYPE=SoftwareImageType
        HPEQ_JUCE_DEFAULT_RENDERING_ENGINE=0
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0)
    target_link_libraries(${target}
        PRIVATE
            HPEQ_ModuleShared
            HPEQ_Assets
            juce::juce_audio_utils
            juce::juce_dsp
            juce::juce_audio_formats
            juce::juce_cryptography
            juce::juce_recommended_config_flags
            juce::juce_recommended_warning_flags)
    set_target_properties(${target} PROPERTIES FOLDER "Standalone Modules")
    set_target_properties(${target}_VST3 PROPERTIES FOLDER "Standalone Modules")
endfunction()

hpeq_add_module_plugin(HamloComp HamloComp com.hamloprod.hamlocomp HmCp 0)
hpeq_add_module_plugin(HamloVerb HamloVerb com.hamloprod.hamloverb HmVb 1)
hpeq_add_module_plugin(HamloEcho HamloEcho com.hamloprod.hamloecho HmEc 2)
hpeq_add_module_plugin(HamloMag HamloMag com.hamloprod.hamlomag HmMg 3)
hpeq_add_module_plugin(HamloRedWall HamloRedWall com.hamloprod.hamloredwall HmRw 4)
hpeq_add_module_plugin(HamloUdarnik HamloUdarnik com.hamloprod.hamloudarnik HmUd 5)
hpeq_add_module_plugin(HamloZastava HamloZastava com.hamloprod.hamlozastava HmZa 6)
hpeq_add_module_plugin(HamloShirota HamloShirota com.hamloprod.hamloshirota HmSh 7)

add_custom_target(HPEQ_ModuleVSTs
    DEPENDS HamloComp_VST3 HamloVerb_VST3 HamloEcho_VST3 HamloMag_VST3
            HamloRedWall_VST3 HamloUdarnik_VST3 HamloZastava_VST3 HamloShirota_VST3)
set_target_properties(HPEQ_ModuleVSTs PROPERTIES FOLDER "Standalone Modules")

add_executable(HPEQ_ModuleTests
    Tests/Unit/ModuleTestMain.cpp
    Tests/Unit/ModulePluginTests.cpp)
target_link_libraries(HPEQ_ModuleTests
    PRIVATE
        HPEQ_ModuleShared
        HPEQ_Assets
        juce::juce_audio_utils
        juce::juce_dsp
        juce::juce_audio_formats
        juce::juce_cryptography
        juce::juce_recommended_config_flags
        juce::juce_recommended_warning_flags)
add_test(NAME HPEQ_ModuleTests COMMAND HPEQ_ModuleTests)

add_executable(HPEQ_ModuleHostSmoke Tests/Tools/ModuleHostSmokeMain.cpp)
target_link_libraries(HPEQ_ModuleHostSmoke
    PRIVATE
        juce::juce_audio_processors
        juce::juce_audio_formats
        juce::juce_cryptography
        juce::juce_dsp
        juce::juce_gui_basics
        juce::juce_recommended_config_flags
        juce::juce_recommended_warning_flags)
target_compile_definitions(HPEQ_ModuleHostSmoke PRIVATE
    JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_PLUGINHOST_VST3=1 JUCE_MODAL_LOOPS_PERMITTED=1)

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
    Tests/Unit/M8Tests.cpp
    Tests/Unit/D6Tests.cpp
    Tests/Unit/CompressorPassTests.cpp
    Tests/Unit/LimiterPassTests.cpp
    Tests/Unit/TransientPassTests.cpp
    Tests/Unit/GatePassTests.cpp
    Tests/Unit/StereoPassTests.cpp
    Tests/Unit/OwnerRoundTests.cpp
    Tests/Unit/ReverbPassTests.cpp
    Tests/Unit/DelayPassTests.cpp
    Tests/Unit/SaturatorPassTests.cpp
    Tests/Unit/MixKnobTests.cpp
    Tests/Unit/FaderPassTests.cpp
    Tests/Unit/RevolutionPassTests.cpp
    Tests/Unit/EqPassTests.cpp
    Tests/Unit/TubePassTests.cpp
    Tests/Unit/PresetPassTests.cpp
    Tests/Unit/RevolutionPresetTests.cpp
    Tests/Unit/UiAnalysisTests.cpp)
target_include_directories(HPEQ_Tests PRIVATE Tests)
target_compile_definitions(HPEQ_Tests PRIVATE HPEQ_TEST_FIXTURES_DIR="${CMAKE_CURRENT_SOURCE_DIR}/Tests/Fixtures")
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
    Tests/Tools/LabT11.cpp
    Tests/Tools/LabBench.cpp
    Tests/Tools/LabPresets.cpp
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
