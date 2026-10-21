#include "Harness/MachineInfo.h"

#include "Harness/RtAllocationProbe.h"

namespace hpeq::test
{
juce::String buildConfiguration()
{
   #if defined(_DEBUG) || JUCE_DEBUG
    return "Debug";
   #else
    return "Release";
   #endif
}

juce::var describeMachine()
{
    auto* info = new juce::DynamicObject();
    info->setProperty("os", juce::SystemStats::getOperatingSystemName());
    info->setProperty("cpu_vendor", juce::SystemStats::getCpuVendor());
    info->setProperty("cpu_model", juce::SystemStats::getCpuModel());
    info->setProperty("cpu_mhz", juce::SystemStats::getCpuSpeedInMegahertz());
    info->setProperty("logical_cpus", juce::SystemStats::getNumCpus());
    info->setProperty("physical_cpus", juce::SystemStats::getNumPhysicalCpus());
    info->setProperty("memory_mb", juce::SystemStats::getMemorySizeInMegabytes());
    info->setProperty("juce_version", juce::SystemStats::getJUCEVersion());
   #if defined(_MSC_FULL_VER)
    info->setProperty("compiler", "MSVC " + juce::String(_MSC_FULL_VER));
   #elif defined(__clang__)
    info->setProperty("compiler", "clang " + juce::String(__clang_version__));
   #else
    info->setProperty("compiler", "unknown");
   #endif
    info->setProperty("build", buildConfiguration());
    info->setProperty("allocation_probe", allocationCoverageName());
    info->setProperty("utc", juce::Time::getCurrentTime().toISO8601(true));
    return juce::var(info);
}
} // namespace hpeq::test
