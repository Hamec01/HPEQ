cmake_minimum_required(VERSION 3.22)

project(HamloShirota VERSION 0.2.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# --- JUCE -------------------------------------------------------------------------------------
# The same JUCE version as HPEQ (the stereo-processor came from there). A local checkout avoids a download.
set(HAMLO_SHIROTA_JUCE_VERSION 8.0.12)
set(HAMLO_SHIROTA_JUCE_DIR "" CACHE PATH "Local JUCE ${HAMLO_SHIROTA_JUCE_VERSION} checkout (empty = FetchContent)")

if (HAMLO_SHIROTA_JUCE_DIR)
    add_subdirectory("${HAMLO_SHIROTA_JUCE_DIR}" "${CMAKE_BINARY_DIR}/JUCE")
    set(HAMLO_SHIROTA_JUCE_SOURCE "${HAMLO_SHIROTA_JUCE_DIR}")
else()
    include(FetchContent)
    FetchContent_Declare(JUCE
        GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
        GIT_TAG ${HAMLO_SHIROTA_JUCE_VERSION}
        GIT_SHALLOW TRUE)
    FetchContent_MakeAvailable(JUCE)
    set(HAMLO_SHIROTA_JUCE_SOURCE "${juce_SOURCE_DIR}")
endif()

# JUCE 8 creates a Direct2D / D3D11 device per Windows plug-in DLL; FL Studio may unload the plug-in while
# transport runs and some NVIDIA drivers hang tearing that device down. The patch adds compile switches
# (HPEQ_JUCE_DEFAULT_*, named after the project they came from) so this target can default to the GDI
# software renderer. A local JUCE must already carry the patch (HPEQ's fetched copy does).
if (WIN32 AND NOT HAMLO_SHIROTA_JUCE_DIR)
    include(cmake/PatchJuceWindowsRenderer.cmake)
    hpeq_patch_juce_windows_renderer("${HAMLO_SHIROTA_JUCE_SOURCE}")
endif()

enable_testing()

# --- Assets: HPEQ's Black Steel pictures (scratches, tubes, the hardware pack), embedded as HpeqAssets -------
file(GLOB HAMLO_SHIROTA_SCRATCHES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/Assets/scratches/*.png)
file(GLOB_RECURSE HAMLO_SHIROTA_HARDWARE CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/Assets/hardware_pack/*.png)
file(GLOB HAMLO_SHIROTA_TUBES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/Assets/tubes/*.png)
juce_add_binary_data(HamloShirota_Assets NAMESPACE HpeqAssets SOURCES ${HAMLO_SHIROTA_SCRATCHES} ${HAMLO_SHIROTA_HARDWARE} ${HAMLO_SHIROTA_TUBES})
set_target_properties(HamloShirota_Assets PROPERTIES POSITION_INDEPENDENT_CODE ON)
juce_add_binary_data(HamloShirota_Hrtf NAMESPACE ShirotaHrtf HEADER_NAME ShirotaHrtf.h SOURCES Assets/Hrtf/production.bin)

# --- Shirota-owned processor; immutable M0 comparison: docs/HAMLOSHIROTA_V2.md -----------
add_library(HamloShirota_Shared STATIC
    Source/Core/ModulePresets.cpp
    Source/DSP/HamloShirota.cpp
    Source/DSP/LegacyStereoProcessor.cpp
    Source/DSP/LegacyShirotaCore.cpp
    Source/DSP/V2Spatial.cpp
    Source/DSP/V2Motion.cpp
    Source/DSP/V2Integrated.cpp
    Source/Standalone/V2Parameters.cpp
    Source/UI/BlackSteel.cpp
    Source/UI/HamloShirotaPanel.cpp
    Source/UI/ShirotaV2Panel.cpp
    Source/Standalone/GenrePresets.cpp
    Source/Standalone/ModuleVariant.cpp
    Source/Standalone/ModulePluginProcessor.cpp
    Source/Standalone/ModulePluginEditor.cpp
    Source/UI/PresetScreen.cpp)
target_include_directories(HamloShirota_Shared
    PUBLIC Source
    PRIVATE
        "${HAMLO_SHIROTA_JUCE_SOURCE}/modules"
        "${CMAKE_CURRENT_BINARY_DIR}/juce_binarydata_HamloShirota_Assets/JuceLibraryCode"
        "${CMAKE_CURRENT_BINARY_DIR}/juce_binarydata_HamloShirota_Hrtf/JuceLibraryCode")
target_compile_definitions(HamloShirota_Shared
    PUBLIC
        JUCE_GLOBAL_MODULE_SETTINGS_INCLUDED=1
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0)
add_dependencies(HamloShirota_Shared HamloShirota_Assets) # BinaryData.h first
add_dependencies(HamloShirota_Shared HamloShirota_Hrtf)
add_custom_target(HamloShirota_S4Identity COMMAND "${CMAKE_COMMAND}" "-DSOURCE_ROOT=${CMAKE_CURRENT_SOURCE_DIR}" "-DIDENTITY_OUTPUT=${CMAKE_CURRENT_BINARY_DIR}/S4Identity.h" -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/WriteS4Identity.cmake")
add_dependencies(HamloShirota_Shared HamloShirota_S4Identity)
target_include_directories(HamloShirota_Shared PUBLIC "${CMAKE_CURRENT_BINARY_DIR}")
target_link_libraries(HamloShirota_Shared PUBLIC HamloShirota_Hrtf)
target_link_libraries(HamloShirota_Shared
    PRIVATE
        juce::juce_recommended_config_flags
        juce::juce_recommended_warning_flags)

# --- The plug-in: identity = HPEQ's HamloShirota module VST3 (BBLB / HmSh / com.hamloprod.hamloshirota, kind 7)
juce_add_plugin(HamloShirota
    COMPANY_NAME "HamloProd"
    BUNDLE_ID com.hamloprod.hamloshirota
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE
    PLUGIN_MANUFACTURER_CODE BBLB
    PLUGIN_CODE HmSh
    VST3_CATEGORIES Fx
    FORMATS VST3
    PRODUCT_NAME "HamloShirota")
set(SHIROTA_RESOURCE_DIR "${CMAKE_CURRENT_BINARY_DIR}/HamloShirota_artefacts/$<CONFIG>/VST3/HamloShirota.vst3/Contents/Resources")
add_custom_command(TARGET HamloShirota_VST3 POST_BUILD
    COMMAND "${CMAKE_COMMAND}" -E make_directory "${SHIROTA_RESOURCE_DIR}"
    COMMAND "${CMAKE_COMMAND}" -E copy_if_different "${CMAKE_CURRENT_SOURCE_DIR}/Assets/Hrtf/LICENSE-SADIE-II.txt" "${SHIROTA_RESOURCE_DIR}/LICENSE-SADIE-II.txt"
    COMMAND "${CMAKE_COMMAND}" -E copy_if_different "${CMAKE_CURRENT_SOURCE_DIR}/Assets/Hrtf/NOTICE.md" "${SHIROTA_RESOURCE_DIR}/SADIE-II-NOTICE.md"
    COMMAND "${CMAKE_COMMAND}" -E copy_if_different "${CMAKE_CURRENT_SOURCE_DIR}/Assets/Hrtf/PRODUCTION-NOTICE.md" "${SHIROTA_RESOURCE_DIR}/PRODUCTION-NOTICE.md")
target_sources(HamloShirota PRIVATE Source/Standalone/ModulePluginEntry.cpp)
target_compile_definitions(HamloShirota PRIVATE
    HPEQ_MODULE_KIND=7
    HPEQ_JUCE_DEFAULT_IMAGE_TYPE=SoftwareImageType
    HPEQ_JUCE_DEFAULT_RENDERING_ENGINE=0
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0)
target_link_libraries(HamloShirota
    PRIVATE
        HamloShirota_Shared
        HamloShirota_Assets
        juce::juce_audio_utils
        juce::juce_dsp
        juce::juce_audio_formats
        juce::juce_cryptography
        juce::juce_recommended_config_flags
        juce::juce_recommended_warning_flags)

# --- Tests and the page snapshot ----------------------------------------------------------------------
foreach(tool Tests Snapshot Lab)
    if (tool STREQUAL "Tests")
        add_executable(HamloShirota_Tests Tests/TestMain.cpp Tests/PluginTests.cpp Tests/IsolationTests.cpp Tests/IntegrationTests.cpp Tests/AllocationAudit.cpp)
    elseif(tool STREQUAL "Lab")
        add_executable(HamloShirota_Lab Tests/ShirotaLabMain.cpp)
    else()
        add_executable(HamloShirota_Snapshot Tests/SnapshotMain.cpp)
    endif()
    target_compile_definitions(HamloShirota_${tool} PRIVATE HPEQ_MODULE_KIND=7 JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_MODAL_LOOPS_PERMITTED=1)
    target_link_libraries(HamloShirota_${tool}
        PRIVATE
            HamloShirota_Shared
            HamloShirota_Assets
            juce::juce_audio_utils
            juce::juce_dsp
            juce::juce_audio_formats
            juce::juce_cryptography
            juce::juce_recommended_config_flags
            juce::juce_recommended_warning_flags)
endforeach()
add_test(NAME HamloShirota_Tests COMMAND HamloShirota_Tests)
add_executable(HamloShirota_LegacyCompat Tests/LegacyCompatMain.cpp)
add_executable(HamloShirota_IntegrationLab Tests/IntegrationLabMain.cpp Tests/AllocationAudit.cpp)
add_executable(HamloShirota_IntegrationTiming Tests/IntegrationTimingMain.cpp)
add_executable(HamloShirota_EmbeddedBankFault EXCLUDE_FROM_ALL Tests/EmbeddedBankFaultMain.cpp)
target_compile_definitions(HamloShirota_EmbeddedBankFault PRIVATE HPEQ_MODULE_KIND=7 JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_MODAL_LOOPS_PERMITTED=1)
target_link_libraries(HamloShirota_EmbeddedBankFault PRIVATE HamloShirota_Shared HamloShirota_Assets juce::juce_audio_utils juce::juce_dsp juce::juce_audio_formats juce::juce_cryptography juce::juce_recommended_config_flags)
target_compile_definitions(HamloShirota_IntegrationTiming PRIVATE HPEQ_MODULE_KIND=7 JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_MODAL_LOOPS_PERMITTED=1)
target_link_libraries(HamloShirota_IntegrationTiming PRIVATE HamloShirota_Shared HamloShirota_Assets juce::juce_audio_utils juce::juce_dsp juce::juce_audio_formats juce::juce_cryptography juce::juce_recommended_config_flags)
target_compile_definitions(HamloShirota_IntegrationLab PRIVATE HPEQ_MODULE_KIND=7 JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_MODAL_LOOPS_PERMITTED=1)
target_link_libraries(HamloShirota_IntegrationLab PRIVATE HamloShirota_Shared HamloShirota_Assets juce::juce_audio_utils juce::juce_dsp juce::juce_audio_formats juce::juce_cryptography juce::juce_recommended_config_flags)
target_compile_definitions(HamloShirota_LegacyCompat PRIVATE HPEQ_MODULE_KIND=7 JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_MODAL_LOOPS_PERMITTED=1)
target_link_libraries(HamloShirota_LegacyCompat PRIVATE HamloShirota_Shared HamloShirota_Assets juce::juce_audio_utils juce::juce_dsp juce::juce_audio_formats juce::juce_cryptography juce::juce_recommended_config_flags)
option(HAMLO_SHIROTA_BUILD_S4_BASELINE "Build the frozen pre-integration facade" OFF)
if(HAMLO_SHIROTA_BUILD_S4_BASELINE)
    add_executable(HamloShirota_S4Baseline EXCLUDE_FROM_ALL Tests/IntegrationLabMain.cpp Tests/AllocationAudit.cpp
        out/s4/frozen/Source/Standalone/ModulePluginProcessor.cpp
        out/s4/frozen/Source/Standalone/ModulePluginEditor.cpp
        out/s4/frozen/Source/Standalone/ModuleVariant.cpp)
    target_include_directories(HamloShirota_S4Baseline BEFORE PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/out/s4/frozen/Source")
    target_compile_definitions(HamloShirota_S4Baseline PRIVATE HPEQ_MODULE_KIND=7 JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_MODAL_LOOPS_PERMITTED=1)
    target_link_libraries(HamloShirota_S4Baseline PRIVATE HamloShirota_Shared HamloShirota_Assets juce::juce_audio_utils juce::juce_dsp juce::juce_audio_formats juce::juce_cryptography juce::juce_recommended_config_flags)
endif()

# S2 offline prototype: deliberately not linked into the production plug-in.
add_executable(HamloShirota_SpatialLab Tests/SpatialLabMain.cpp Tests/AllocationAudit.cpp Source/DSP/SpatialPrototype.cpp)
add_custom_target(HamloShirota_SpatialIdentity
    COMMAND "${CMAKE_COMMAND}" "-DSOURCE_ROOT=${CMAKE_CURRENT_SOURCE_DIR}" "-DIDENTITY_OUTPUT=${CMAKE_CURRENT_BINARY_DIR}/SpatialIdentity.h" -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/WriteSpatialIdentity.cmake")
add_dependencies(HamloShirota_SpatialLab HamloShirota_SpatialIdentity)
target_include_directories(HamloShirota_SpatialLab PRIVATE Source Tests "${CMAKE_CURRENT_BINARY_DIR}")
target_link_libraries(HamloShirota_SpatialLab PRIVATE juce::juce_recommended_config_flags juce::juce_recommended_warning_flags)

add_executable(HamloShirota_SpatialDemo Tests/SpatialDemoMain.cpp Source/DSP/SpatialPrototype.cpp)
add_dependencies(HamloShirota_SpatialDemo HamloShirota_SpatialIdentity)
target_include_directories(HamloShirota_SpatialDemo PRIVATE Source "${CMAKE_CURRENT_BINARY_DIR}")
target_link_libraries(HamloShirota_SpatialDemo PRIVATE juce::juce_recommended_config_flags juce::juce_recommended_warning_flags)

# Fault probe links a deliberately stale DSP implementation against the real lab.
add_executable(HamloShirota_StaleBuildProbe EXCLUDE_FROM_ALL Tests/SpatialLabMain.cpp Tests/AllocationAudit.cpp Tests/StaleSpatialBuildProbe.cpp)
add_dependencies(HamloShirota_StaleBuildProbe HamloShirota_SpatialIdentity)
target_include_directories(HamloShirota_StaleBuildProbe PRIVATE Source Tests "${CMAKE_CURRENT_BINARY_DIR}")
target_link_libraries(HamloShirota_StaleBuildProbe PRIVATE juce::juce_recommended_config_flags juce::juce_recommended_warning_flags)

# S3 experiments remain separate from Legacy and the accepted S2 prototype.
add_custom_target(HamloShirota_MotionIdentity
    COMMAND "${CMAKE_COMMAND}" "-DSOURCE_ROOT=${CMAKE_CURRENT_SOURCE_DIR}" "-DIDENTITY_OUTPUT=${CMAKE_CURRENT_BINARY_DIR}/MotionIdentity.h" -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/WriteMotionIdentity.cmake")
add_executable(HamloShirota_MotionLab Tests/MotionLabMain.cpp Tests/AllocationAudit.cpp Source/DSP/MotionPrototype.cpp)
add_dependencies(HamloShirota_MotionLab HamloShirota_MotionIdentity)
target_include_directories(HamloShirota_MotionLab PRIVATE Source Tests "${CMAKE_CURRENT_BINARY_DIR}")
target_link_libraries(HamloShirota_MotionLab PRIVATE bcrypt juce::juce_recommended_config_flags juce::juce_recommended_warning_flags)
add_executable(HamloShirota_StaleMotionProbe EXCLUDE_FROM_ALL Tests/MotionLabMain.cpp Tests/AllocationAudit.cpp Tests/StaleMotionBuildProbe.cpp)
add_dependencies(HamloShirota_StaleMotionProbe HamloShirota_MotionIdentity)
target_include_directories(HamloShirota_StaleMotionProbe PRIVATE Source Tests "${CMAKE_CURRENT_BINARY_DIR}")
target_link_libraries(HamloShirota_StaleMotionProbe PRIVATE juce::juce_recommended_config_flags juce::juce_recommended_warning_flags)

# S5 native library/calibration runner; generated table is excluded from sound ID.
add_custom_target(HamloShirota_PresetIdentity
    COMMAND "${CMAKE_COMMAND}" "-DSOURCE_ROOT=${CMAKE_CURRENT_SOURCE_DIR}" "-DIDENTITY_OUTPUT=${CMAKE_CURRENT_BINARY_DIR}/S5Identity.h" -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/WriteS5Identity.cmake")
add_executable(HamloShirota_PresetLab Tests/PresetLabMain.cpp Tests/AllocationAudit.cpp)
add_dependencies(HamloShirota_PresetLab HamloShirota_PresetIdentity)
target_compile_definitions(HamloShirota_PresetLab PRIVATE HPEQ_MODULE_KIND=7 JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_MODAL_LOOPS_PERMITTED=1)
target_link_libraries(HamloShirota_PresetLab PRIVATE HamloShirota_Shared HamloShirota_Assets juce::juce_audio_utils juce::juce_dsp juce::juce_audio_formats juce::juce_cryptography juce::juce_recommended_config_flags)

add_executable(HamloShirota_V2Compat Tests/V2CompatMain.cpp)
target_compile_definitions(HamloShirota_V2Compat PRIVATE HPEQ_MODULE_KIND=7 JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_MODAL_LOOPS_PERMITTED=1)
target_link_libraries(HamloShirota_V2Compat PRIVATE HamloShirota_Shared HamloShirota_Assets juce::juce_audio_utils juce::juce_dsp juce::juce_audio_formats juce::juce_cryptography juce::juce_recommended_config_flags)
