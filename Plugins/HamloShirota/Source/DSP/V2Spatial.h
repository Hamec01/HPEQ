#pragma once
#include <array>
#include <memory>

namespace hamlo::shirota::v2
{
enum class Generator { Phase,Micro,Diffuse };
enum class Space { Tight,Open,Scatter };
struct SpatialSettings
{
    bool enabled=false,guard=false;
    double width=1;
    std::array<double,3> bandWidth{1,1,1};
    double splitLow=300,splitHigh=3000,monoBassHz=0;
    Generator generator=Generator::Phase;
    double generate=0,space=0,size=1,depth=.5,damping=9000,predelayMs=0;
    Space character=Space::Tight;
    double correlationFloor=0,mix=1,outputDb=0;
};
// Experimental API; deliberately excluded from the production VST3 target.
class SpatialProcessor
{
public:
    SpatialProcessor();
    ~SpatialProcessor();
    void prepare(double sampleRate,int maximumBlock=256);
    void setTargets(const SpatialSettings&) noexcept;
    void process(float* left,float* right,int samples) noexcept;
    void clearHistory() noexcept;
    int latency() const noexcept;
    double guardDb() const noexcept;
    static bool implemented() noexcept;
    static const char* compiledIdentity() noexcept;
private:
    struct Impl;
    std::unique_ptr<Impl> impl;
};
}
