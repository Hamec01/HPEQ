#pragma once
#include <array>
#include <memory>
#include <string>

namespace hamlo::shirota::v2
{
enum class MotionUnit { Milliseconds, Beats };
enum class MotionArc { Shortest, Clockwise, Counterclockwise };
enum class MotionCurve { Linear, Smooth };
struct MotionNode
{
    double azimuth=0,hold=125,travel=375;
    MotionArc arc=MotionArc::Shortest;
    MotionCurve curve=MotionCurve::Smooth;
};
struct MotionPath
{
    std::array<MotionNode,16> nodes{};
    int count=2;
    bool loop=true;
    MotionUnit unit=MotionUnit::Milliseconds;
};
struct MotionPosition { double azimuth=0;int node=0;bool travelling=false;double progress=0; };
class MotionClock
{
public:
    void prepare(double rate) noexcept;
    void setPath(const MotionPath&) noexcept;
    void setTempo(double bpm) noexcept;
    void setRate(double rate) noexcept;
    void reset() noexcept;
    MotionPosition next() noexcept;
private:
    MotionPath path;
    double fs=48000,bpm=120,phase=0,speed=1;
    int node=0;
    bool travelling=false,stationary=false,finished=false;
};
enum class MotionRenderer { Stereo, Binaural };
enum class MotionSource { Spread, Point };
struct MotionSettings
{
    bool enabled=false,usePath=false;
    MotionRenderer renderer=MotionRenderer::Stereo;
    MotionSource source=MotionSource::Spread;
    double center=0,spread=180,balance=0;
    int profile=0;
};
// Offline experiment; not linked into the production plug-in.
class MotionProcessor
{
public:
    MotionProcessor();
    ~MotionProcessor();
    void prepare(double rate);
    void setTargets(const MotionSettings&) noexcept;
    void setPath(const MotionPath&) noexcept;
    void setTempo(double bpm) noexcept;
    void setRate(double rate) noexcept;
    void resetPath() noexcept;
    void process(float* left,float* right,int frames,MotionPosition* positions=nullptr) noexcept;
    void clearHistory() noexcept;
    int latency() const noexcept;
    MotionPosition position() const noexcept;
    static bool implemented() noexcept;
    static const char* compiledIdentity() noexcept;
private:
    struct Impl;
    std::unique_ptr<Impl> impl;
};
}
