set(hpeq_source "${PROJECT_SOURCE_DIR}/Source")
set(preset_source "${PROJECT_SOURCE_DIR}/Plugins/HamloMag/Source")

juce_add_plugin(HamloTochka
    VERSION 0.1.0
    COMPANY_NAME "HamloProd"
    BUNDLE_ID com.hamloprod.hamlotochka
    PLUGIN_MANUFACTURER_CODE BBLB
    PLUGIN_CODE HmTk
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE
    VST3_CATEGORIES Fx EQ
    FORMATS VST3
    PRODUCT_NAME "HamloTochka")

target_sources(HamloTochka PRIVATE
    Source/TochkaProcessor.cpp
    Source/TochkaEditor.cpp
    Source/PresetScreen.cpp
    "${hpeq_source}/Analysis/LoudnessMeter.cpp"
    "${hpeq_source}/Analysis/SpectrumAnalyzer.cpp"
    "${hpeq_source}/DSP/Biquad.cpp"
    "${hpeq_source}/DSP/EqDesign.cpp"
    "${hpeq_source}/DSP/PreEqProcessor.cpp"
    "${hpeq_source}/UI/AnalysisModel.cpp"
    "${hpeq_source}/UI/BlackSteel.cpp"
    "${hpeq_source}/UI/HamloTochkaPanel.cpp"
    "${hpeq_source}/UI/SteelLookAndFeel.cpp")
target_include_directories(HamloTochka PRIVATE "${hpeq_source}" "${preset_source}")
target_compile_definitions(HamloTochka PRIVATE
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    HPEQ_JUCE_DEFAULT_IMAGE_TYPE=SoftwareImageType
    HPEQ_JUCE_DEFAULT_RENDERING_ENGINE=0)
target_link_libraries(HamloTochka PRIVATE
    HPEQ_Assets
    juce::juce_audio_processors
    juce::juce_dsp
    juce::juce_recommended_config_flags)

add_executable(HamloTochka_HostCheck Tests/HostCheck.cpp
    "${hpeq_source}/DSP/Biquad.cpp"
    "${hpeq_source}/DSP/EqDesign.cpp"
    "${hpeq_source}/DSP/PreEqProcessor.cpp"
    "${hpeq_source}/Analysis/LoudnessMeter.cpp")
target_include_directories(HamloTochka_HostCheck PRIVATE "${hpeq_source}")
target_compile_definitions(HamloTochka_HostCheck PRIVATE
    JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_PLUGINHOST_VST3=1)
target_link_libraries(HamloTochka_HostCheck PRIVATE
    juce::juce_audio_processors juce::juce_recommended_config_flags)
add_dependencies(HamloTochka_HostCheck HamloTochka_VST3)
add_test(NAME HamloTochka_HostCheck COMMAND HamloTochka_HostCheck
    "$<TARGET_FILE:HamloTochka_VST3>")

add_executable(HamloTochka_LocalCheck Tests/LocalCheck.cpp)
target_include_directories(HamloTochka_LocalCheck PRIVATE
    Source "${hpeq_source}" "$<TARGET_PROPERTY:juce::juce_audio_processors,INTERFACE_INCLUDE_DIRECTORIES>")
target_compile_definitions(HamloTochka_LocalCheck PRIVATE
    JUCE_GLOBAL_MODULE_SETTINGS_INCLUDED=1 JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0)
target_link_libraries(HamloTochka_LocalCheck PRIVATE HamloTochka)
add_test(NAME HamloTochka_LocalCheck COMMAND HamloTochka_LocalCheck)
