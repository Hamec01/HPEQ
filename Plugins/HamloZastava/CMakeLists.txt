cmake_minimum_required(VERSION 3.22)

project(HamloZastava VERSION 0.2.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# --- JUCE -------------------------------------------------------------------------------------
# The same JUCE version as HPEQ (the gate/expander came from there). A local checkout avoids a download.
set(HAMLO_ZASTAVA_JUCE_VERSION 8.0.12)
set(HAMLO_ZASTAVA_JUCE_DIR "" CACHE PATH "Local JUCE ${HAMLO_ZASTAVA_JUCE_VERSION} checkout (empty = FetchContent)")

if (HAMLO_ZASTAVA_JUCE_DIR)
    add_subdirectory("${HAMLO_ZASTAVA_JUCE_DIR}" "${CMAKE_BINARY_DIR}/JUCE")
    set(HAMLO_ZASTAVA_JUCE_SOURCE "${HAMLO_ZASTAVA_JUCE_DIR}")
else()
    include(FetchContent)
    FetchContent_Declare(JUCE
        GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
        GIT_TAG ${HAMLO_ZASTAVA_JUCE_VERSION}
        GIT_SHALLOW TRUE)
    FetchContent_MakeAvailable(JUCE)
    set(HAMLO_ZASTAVA_JUCE_SOURCE "${juce_SOURCE_DIR}")
endif()

# JUCE 8 creates a Direct2D / D3D11 device per Windows plug-in DLL; FL Studio may unload the plug-in while
# transport runs and some NVIDIA drivers hang tearing that device down. The patch adds compile switches
# (HPEQ_JUCE_DEFAULT_*, named after the project they came from) so this target can default to the GDI
# software renderer. A local JUCE must already carry the patch (HPEQ's fetched copy does).
if (WIN32 AND NOT HAMLO_ZASTAVA_JUCE_DIR)
    include(cmake/PatchJuceWindowsRenderer.cmake)
    hpeq_patch_juce_windows_renderer("${HAMLO_ZASTAVA_JUCE_SOURCE}")
endif()

enable_testing()

# --- Assets: HPEQ's Black Steel pictures (scratches, tubes, the hardware pack), embedded as HpeqAssets -------
file(GLOB HAMLO_ZASTAVA_SCRATCHES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/Assets/scratches/*.png)
file(GLOB_RECURSE HAMLO_ZASTAVA_HARDWARE CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/Assets/hardware_pack/*.png)
file(GLOB HAMLO_ZASTAVA_TUBES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/Assets/tubes/*.png)
juce_add_binary_data(HamloZastava_Assets NAMESPACE HpeqAssets SOURCES ${HAMLO_ZASTAVA_SCRATCHES} ${HAMLO_ZASTAVA_HARDWARE} ${HAMLO_ZASTAVA_TUBES})
set_target_properties(HamloZastava_Assets PROPERTIES POSITION_INDEPENDENT_CODE ON)

# --- Shared code: HPEQ's module-plug-in stack (Source/ is HPEQ's, verbatim; docs/SYNC_WITH_HPEQ.md) -----------
add_library(HamloZastava_Shared STATIC
    Source/Core/ModulePresets.cpp
    Source/Core/GenrePresets.cpp
    Source/DSP/HamloZastava.cpp
    Source/DSP/LegacyGateProcessor.cpp
    Source/DSP/LegacyZastavaCore.cpp
    Source/DSP/V2Gate.cpp
    Source/UI/BlackSteel.cpp
    Source/UI/HamloZastavaPanel.cpp
    Source/UI/ModernZastavaPanel.cpp
    Source/UI/PresetScreen.cpp
    Source/Standalone/ModuleVariant.cpp
    Source/Standalone/ModernParameters.cpp
    Source/Standalone/ModulePluginProcessor.cpp
    Source/Standalone/ModulePluginEditor.cpp
)
target_include_directories(HamloZastava_Shared
    PUBLIC Source
    PRIVATE
        "${HAMLO_ZASTAVA_JUCE_SOURCE}/modules"
        "${CMAKE_CURRENT_BINARY_DIR}/juce_binarydata_HamloZastava_Assets/JuceLibraryCode")
target_compile_definitions(HamloZastava_Shared
    PUBLIC
        JUCE_GLOBAL_MODULE_SETTINGS_INCLUDED=1
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0)
add_dependencies(HamloZastava_Shared HamloZastava_Assets) # BinaryData.h first
target_link_libraries(HamloZastava_Shared
    PRIVATE
        juce::juce_recommended_config_flags
        juce::juce_recommended_warning_flags)

# --- The plug-in: identity = HPEQ's HamloZastava module VST3 (BBLB / HmZa / com.hamloprod.hamlozastava, kind 6)
juce_add_plugin(HamloZastava
    COMPANY_NAME "HamloProd"
    BUNDLE_ID com.hamloprod.hamlozastava
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE
    PLUGIN_MANUFACTURER_CODE BBLB
    PLUGIN_CODE HmZa
    VST3_CATEGORIES Fx
    FORMATS VST3
    PRODUCT_NAME "HamloZastava")
target_sources(HamloZastava PRIVATE Source/Standalone/ModulePluginEntry.cpp)
target_compile_definitions(HamloZastava PRIVATE
    HPEQ_MODULE_KIND=6
    HPEQ_JUCE_DEFAULT_IMAGE_TYPE=SoftwareImageType
    HPEQ_JUCE_DEFAULT_RENDERING_ENGINE=0
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0)
target_link_libraries(HamloZastava
    PRIVATE
        HamloZastava_Shared
        HamloZastava_Assets
        juce::juce_audio_utils
        juce::juce_dsp
        juce::juce_audio_formats
        juce::juce_cryptography
        juce::juce_recommended_config_flags
        juce::juce_recommended_warning_flags)

# --- Tests and the page snapshot ----------------------------------------------------------------------
foreach(tool Tests Snapshot)
    if (tool STREQUAL "Tests")
        add_executable(HamloZastava_Tests Tests/TestMain.cpp Tests/PluginTests.cpp Tests/IsolationTests.cpp Tests/S3IntegrationTests.cpp Tests/S4Tests.cpp Tests/S5Tests.cpp Tests/AllocationAudit.cpp)
    else()
        add_executable(HamloZastava_Snapshot Tests/SnapshotMain.cpp)
    endif()
    target_compile_definitions(HamloZastava_${tool} PRIVATE HPEQ_MODULE_KIND=6 JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_MODAL_LOOPS_PERMITTED=1)
    target_link_libraries(HamloZastava_${tool}
        PRIVATE
            HamloZastava_Shared
            HamloZastava_Assets
            juce::juce_audio_utils
            juce::juce_dsp
            juce::juce_audio_formats
            juce::juce_cryptography
            juce::juce_recommended_config_flags
            juce::juce_recommended_warning_flags)
endforeach()

add_test(NAME HamloZastava_Tests COMMAND HamloZastava_Tests)

# Immutable S1 capture/verification laboratory.
add_custom_target(HamloZastava_S1Identity
    COMMAND "${CMAKE_COMMAND}" "-DSOURCE_ROOT=${CMAKE_CURRENT_SOURCE_DIR}" "-DIDENTITY_OUTPUT=${CMAKE_CURRENT_BINARY_DIR}/ZastavaS1Identity.h" -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/WriteS1Identity.cmake")
add_dependencies(HamloZastava_Shared HamloZastava_S1Identity)
add_dependencies(HamloZastava_Shared HamloZastava_S2Identity)
add_executable(HamloZastava_CompatLab Tests/ZastavaLabMain.cpp)
target_compile_definitions(HamloZastava_CompatLab PRIVATE HPEQ_MODULE_KIND=6 JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_MODAL_LOOPS_PERMITTED=1)
target_link_libraries(HamloZastava_CompatLab PRIVATE HamloZastava_Shared HamloZastava_Assets juce::juce_audio_utils juce::juce_dsp juce::juce_audio_formats juce::juce_cryptography juce::juce_recommended_config_flags)

target_include_directories(HamloZastava_Shared PUBLIC "${CMAKE_CURRENT_BINARY_DIR}")

option(HAMLO_ZASTAVA_BUILD_M0_REFERENCE "Build the frozen full M0 laboratory (requires captured sources)" OFF)
if(HAMLO_ZASTAVA_BUILD_M0_REFERENCE)
# Final paired method against the immutable full M0 source, never current headers.
set(ZASTAVA_M0_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/out/s1/original")
file(READ "${ZASTAVA_M0_ROOT}/CMakeLists.txt" m0_cmake)
string(REGEX MATCH "add_library\\(HamloZastava_Shared STATIC[^)]*" m0_sources "${m0_cmake}")
string(REGEX MATCHALL "Source/[A-Za-z0-9_/]+\\.cpp" m0_parts "${m0_sources}")
set(m0_units)
foreach(part IN LISTS m0_parts)
    list(APPEND m0_units "${ZASTAVA_M0_ROOT}/${part}")
endforeach()
add_library(HamloZastava_M0Shared STATIC EXCLUDE_FROM_ALL ${m0_units})
target_include_directories(HamloZastava_M0Shared PUBLIC "${ZASTAVA_M0_ROOT}/Source" PRIVATE "${HAMLO_ZASTAVA_JUCE_SOURCE}/modules" "${CMAKE_CURRENT_BINARY_DIR}/juce_binarydata_HamloZastava_Assets/JuceLibraryCode")
get_target_property(m0_defines HamloZastava_Shared INTERFACE_COMPILE_DEFINITIONS)
target_compile_definitions(HamloZastava_M0Shared PUBLIC ${m0_defines})
target_link_libraries(HamloZastava_M0Shared PRIVATE juce::juce_recommended_config_flags juce::juce_recommended_warning_flags)
add_dependencies(HamloZastava_M0Shared HamloZastava_Assets)
add_custom_target(HamloZastava_M0Identity
    COMMAND "${CMAKE_COMMAND}" -E make_directory "${CMAKE_CURRENT_BINARY_DIR}/m0"
    COMMAND "${CMAKE_COMMAND}" "-DSOURCE_ROOT=${ZASTAVA_M0_ROOT}" "-DMETHOD_ROOT=${CMAKE_CURRENT_SOURCE_DIR}" "-DIDENTITY_OUTPUT=${CMAKE_CURRENT_BINARY_DIR}/m0/ZastavaS1Identity.h" -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/WriteS1Identity.cmake")
add_executable(HamloZastava_M0CompatLab EXCLUDE_FROM_ALL Tests/ZastavaLabMain.cpp)
add_dependencies(HamloZastava_M0CompatLab HamloZastava_M0Identity)
target_include_directories(HamloZastava_M0CompatLab BEFORE PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/m0" "${ZASTAVA_M0_ROOT}/Source")
target_compile_definitions(HamloZastava_M0CompatLab PRIVATE HPEQ_MODULE_KIND=6 JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_MODAL_LOOPS_PERMITTED=1)
target_link_libraries(HamloZastava_M0CompatLab PRIVATE HamloZastava_M0Shared HamloZastava_Assets juce::juce_audio_utils juce::juce_dsp juce::juce_audio_formats juce::juce_cryptography juce::juce_recommended_config_flags)

endif()

# S2 kernel laboratory is independent of the shipping Legacy binary.
add_custom_target(HamloZastava_S2Identity
    COMMAND "${CMAKE_COMMAND}" "-DSOURCE_ROOT=${CMAKE_CURRENT_SOURCE_DIR}" "-DIDENTITY_OUTPUT=${CMAKE_CURRENT_BINARY_DIR}/ZastavaS2Identity.h" -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/WriteS2Identity.cmake")
add_executable(HamloZastava_V2Lab Tests/V2LabMain.cpp Tests/AllocationAudit.cpp Source/DSP/V2Gate.cpp)
add_dependencies(HamloZastava_V2Lab HamloZastava_S2Identity)
target_include_directories(HamloZastava_V2Lab PRIVATE Source Tests "${CMAKE_CURRENT_BINARY_DIR}")
add_executable(HamloZastava_V2Render Tests/V2RenderMain.cpp Tests/AllocationAudit.cpp Source/DSP/V2Gate.cpp)
add_dependencies(HamloZastava_V2Render HamloZastava_S2Identity)
target_include_directories(HamloZastava_V2Render PRIVATE Source Tests "${CMAKE_CURRENT_BINARY_DIR}")

# Optional, API-compatible pre-DSP control built with the identical current driver.
option(HAMLO_ZASTAVA_BUILD_S2_REFERENCE "Build archived S2 neutral control" OFF)
if(HAMLO_ZASTAVA_BUILD_S2_REFERENCE)
    set(ZASTAVA_S2_REFERENCE "${CMAKE_CURRENT_SOURCE_DIR}/out/s2/before_sources" CACHE PATH "Archived API-compatible S2 control or regression sources")
    add_custom_target(HamloZastava_S2ReferenceIdentity
        COMMAND "${CMAKE_COMMAND}" -E make_directory "${CMAKE_CURRENT_BINARY_DIR}/s2_reference"
        COMMAND "${CMAKE_COMMAND}" "-DSOURCE_ROOT=${ZASTAVA_S2_REFERENCE}" "-DMETHOD_ROOT=${CMAKE_CURRENT_SOURCE_DIR}" "-DIDENTITY_OUTPUT=${CMAKE_CURRENT_BINARY_DIR}/s2_reference/ZastavaS2Identity.h" -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/WriteS2Identity.cmake")
    add_executable(HamloZastava_V2Reference EXCLUDE_FROM_ALL Tests/V2LabMain.cpp Tests/AllocationAudit.cpp "${ZASTAVA_S2_REFERENCE}/Source/DSP/V2Gate.cpp")
    add_dependencies(HamloZastava_V2Reference HamloZastava_S2ReferenceIdentity)
    target_include_directories(HamloZastava_V2Reference BEFORE PRIVATE "${ZASTAVA_S2_REFERENCE}/Source" Tests "${CMAKE_CURRENT_BINARY_DIR}/s2_reference")
endif()

# S3 host integration uses the same test sources for archived M0 and current.
foreach(pair Host Legacy)
    if(pair STREQUAL "Host")
        set(driver Tests/S3HostLabMain.cpp)
    else()
        set(driver Tests/S3LegacyLabMain.cpp)
    endif()
    foreach(flavour Current Reference)
        if(flavour STREQUAL "Reference")
            if(NOT HAMLO_ZASTAVA_BUILD_M0_REFERENCE)
                continue()
            endif()
            set(source_root "${ZASTAVA_M0_ROOT}")
            set(shared HamloZastava_M0Shared)
        else()
            set(source_root "${CMAKE_CURRENT_SOURCE_DIR}")
            set(shared HamloZastava_Shared)
        endif()
        set(target HamloZastava_S3${pair}${flavour})
        set(identity_dir "${CMAKE_CURRENT_BINARY_DIR}/s3_${flavour}")
        add_custom_target(${target}Identity
            COMMAND "${CMAKE_COMMAND}" -E make_directory "${identity_dir}"
            COMMAND "${CMAKE_COMMAND}" "-DSOURCE_ROOT=${source_root}" "-DMETHOD_ROOT=${CMAKE_CURRENT_SOURCE_DIR}" "-DIDENTITY_OUTPUT=${identity_dir}/ZastavaS3Identity.h" -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/WriteS3Identity.cmake")
        add_executable(${target} EXCLUDE_FROM_ALL ${driver} Tests/AllocationAudit.cpp)
        if(pair STREQUAL "Host")
            target_sources(${target} PRIVATE Source/DSP/V2Gate.cpp)
            add_dependencies(${target} HamloZastava_S2Identity)
        endif()
        add_dependencies(${target} ${target}Identity)
        target_include_directories(${target} BEFORE PRIVATE "${source_root}/Source" "${identity_dir}" Source Tests "${CMAKE_CURRENT_BINARY_DIR}")
        target_compile_definitions(${target} PRIVATE HPEQ_MODULE_KIND=6 JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_MODAL_LOOPS_PERMITTED=1)
        target_link_libraries(${target} PRIVATE ${shared} HamloZastava_Assets juce::juce_audio_utils juce::juce_dsp juce::juce_audio_formats juce::juce_cryptography juce::juce_recommended_config_flags)
    endforeach()
endforeach()

# S5 reuses the identical original-prefix fixtures; new catalog tested separately.
add_custom_target(HamloZastava_PresetIdentity
    COMMAND "${CMAKE_COMMAND}" "-DSOURCE_ROOT=${CMAKE_CURRENT_SOURCE_DIR}" "-DIDENTITY_OUTPUT=${CMAKE_CURRENT_BINARY_DIR}/ZastavaPresetIdentity.h" -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/WritePresetIdentity.cmake")
add_executable(HamloZastava_PresetLab EXCLUDE_FROM_ALL Tests/PresetLabMain.cpp Tests/AllocationAudit.cpp)
add_dependencies(HamloZastava_PresetLab HamloZastava_PresetIdentity)
target_compile_definitions(HamloZastava_PresetLab PRIVATE HPEQ_MODULE_KIND=6 JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_MODAL_LOOPS_PERMITTED=1)
target_link_libraries(HamloZastava_PresetLab PRIVATE HamloZastava_Shared HamloZastava_Assets juce::juce_audio_utils juce::juce_dsp juce::juce_audio_formats juce::juce_cryptography juce::juce_recommended_config_flags)
foreach(pair Host Legacy)
    foreach(flavour Current Reference)
        if(flavour STREQUAL "Reference")
            if(NOT HAMLO_ZASTAVA_BUILD_M0_REFERENCE)
                continue()
            endif()
            set(s5_shared HamloZastava_M0Shared)
            set(s5_root "${ZASTAVA_M0_ROOT}")
        else()
            set(s5_shared HamloZastava_Shared)
            set(s5_root "${CMAKE_CURRENT_SOURCE_DIR}")
        endif()
        set(s5_target HamloZastava_S5${pair}${flavour})
        if(NOT TARGET HamloZastava_S5${flavour}Identity)
            add_custom_target(HamloZastava_S5${flavour}Identity
                COMMAND "${CMAKE_COMMAND}" -E make_directory "${CMAKE_CURRENT_BINARY_DIR}/s5_${flavour}"
                COMMAND "${CMAKE_COMMAND}" "-DSOURCE_ROOT=${s5_root}" "-DMETHOD_ROOT=${CMAKE_CURRENT_SOURCE_DIR}" "-DIDENTITY_OUTPUT=${CMAKE_CURRENT_BINARY_DIR}/s5_${flavour}/ZastavaS5Identity.h" -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/WriteS5CompatIdentity.cmake")
        endif()
        add_executable(${s5_target} EXCLUDE_FROM_ALL Tests/S5${pair}LabMain.cpp Tests/AllocationAudit.cpp)
        if(pair STREQUAL "Host")
            target_sources(${s5_target} PRIVATE Source/DSP/V2Gate.cpp)
            add_dependencies(${s5_target} HamloZastava_S2Identity)
        endif()
        add_dependencies(${s5_target} HamloZastava_S5${flavour}Identity)
        target_include_directories(${s5_target} BEFORE PRIVATE "${s5_root}/Source" "${CMAKE_CURRENT_BINARY_DIR}/s5_${flavour}" Source Tests "${CMAKE_CURRENT_BINARY_DIR}")
        target_compile_definitions(${s5_target} PRIVATE HPEQ_MODULE_KIND=6 JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_MODAL_LOOPS_PERMITTED=1)
        target_link_libraries(${s5_target} PRIVATE ${s5_shared} HamloZastava_Assets juce::juce_audio_utils juce::juce_dsp juce::juce_audio_formats juce::juce_cryptography juce::juce_recommended_config_flags)
    endforeach()
endforeach()
