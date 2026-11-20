# Namespace-isolated pinned native wrappers, generated outside source payloads.
find_package(Python3 REQUIRED COMPONENTS Interpreter)
set(HPEQ_NATIVE_GENERATED "${CMAKE_CURRENT_BINARY_DIR}/HamloNative")
execute_process(COMMAND "${Python3_EXECUTABLE}" -I -X utf8
    "${CMAKE_SOURCE_DIR}/scripts/hamlorack/generate_native.py" --out "${HPEQ_NATIVE_GENERATED}"
    WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}" RESULT_VARIABLE native_result
    OUTPUT_VARIABLE native_output ERROR_VARIABLE native_error)
if(NOT native_result EQUAL 0)
    message(FATAL_ERROR "Native isolation failed: ${native_output}\n${native_error}")
endif()
message(STATUS "${native_output}")
foreach(identity_spec "HamloShirota|WriteS4Identity.cmake|S4Identity.h"
    "HamloZastava|WriteS1Identity.cmake|ZastavaS1Identity.h"
    "HamloZastava|WriteS2Identity.cmake|ZastavaS2Identity.h")
    string(REPLACE "|" ";" identity_parts "${identity_spec}")
    list(GET identity_parts 0 identity_plugin)
    list(GET identity_parts 1 identity_script)
    list(GET identity_parts 2 identity_header)
    execute_process(COMMAND "${CMAKE_COMMAND}"
        "-DSOURCE_ROOT=${CMAKE_SOURCE_DIR}/Plugins/${identity_plugin}"
        "-DIDENTITY_OUTPUT=${HPEQ_NATIVE_GENERATED}/${identity_plugin}/${identity_header}"
        -P "${CMAKE_SOURCE_DIR}/Plugins/${identity_plugin}/cmake/${identity_script}"
        RESULT_VARIABLE identity_result)
    if(NOT identity_result EQUAL 0)
        message(FATAL_ERROR "Native identity generation failed for ${identity_plugin}")
    endif()
endforeach()
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS
    "${CMAKE_SOURCE_DIR}/scripts/hamlorack/generate_native.py"
    "${CMAKE_SOURCE_DIR}/Assets/Presets/HamloRack/Design/native-contract.json")
include("${HPEQ_NATIVE_GENERATED}/NativeTargets.cmake")
juce_add_binary_data(HPEQ_NativeHrtf NAMESPACE ShirotaHrtf HEADER_NAME ShirotaHrtf.h
    SOURCES "${CMAKE_SOURCE_DIR}/Plugins/HamloShirota/Assets/Hrtf/production.bin")
target_include_directories(HpeqNativeShirota PRIVATE
    "${CMAKE_CURRENT_BINARY_DIR}/juce_binarydata_HPEQ_NativeHrtf/JuceLibraryCode")
target_link_libraries(HpeqNativeShirota PUBLIC HPEQ_NativeHrtf)
add_dependencies(HpeqNativeShirota HPEQ_NativeHrtf)
add_library(HPEQ_NativeRuntime STATIC Source/Rack/NativeApi.cpp Source/Rack/NativeRegistry.cpp)
target_include_directories(HPEQ_NativeRuntime PUBLIC "${CMAKE_SOURCE_DIR}/Source"
    PRIVATE "${JUCE_MODULES_DIR}")
target_compile_definitions(HPEQ_NativeRuntime PRIVATE JUCE_GLOBAL_MODULE_SETTINGS_INCLUDED=1
    JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_VST3_CAN_REPLACE_VST2=0)
target_link_libraries(HPEQ_NativeRuntime PUBLIC ${HPEQ_NATIVE_LIBRARIES}
    PRIVATE juce::juce_recommended_config_flags juce::juce_recommended_warning_flags)
