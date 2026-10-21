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

# --- Measurement harness (shared by the M0 tool and the unit tests) ----------------------------
set(HPEQ_HARNESS_SOURCES
    Tests/Harness/TestSignalFactory.cpp
    Tests/Harness/AudioMetrics.cpp
    Tests/Harness/AudioFileIO.cpp
    Tests/Harness/RtAllocationProbe.cpp
    Tests/Harness/MachineInfo.cpp)

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

