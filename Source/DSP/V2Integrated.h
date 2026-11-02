#pragma once
#include "DSP/V2Spatial.h"
#include "DSP/V2Motion.h"
#include "DSP/V2Settings.h"
#include "DSP/DelayLine.h"
#include "DSP/Ramp.h"
namespace hpeq::dsp
{
const char* s4SourceIdentity() noexcept;
class V2Integrated
{
public:
    void prepare(double fs,int block);
    void targets(const v2::Controls&,bool on) noexcept;
    void process(float* l,float* r,int samples,double bpm) noexcept;
    void clearHistory() noexcept {spatial.clearHistory();motion.clearHistory();dryDelay.reset();mix.reset(mix.targetValue());gain.reset(gain.targetValue());mono.reset(mono.targetValue());started=false;}
    double guardDb() const noexcept {return spatial.guardDb();}
    double azimuth() const noexcept {return motion.position().azimuth;}
private:
    hamlo::shirota::v2::SpatialProcessor spatial;
    hamlo::shirota::v2::MotionProcessor motion;
    hamlo::shirota::v2::MotionPath previousPath;
    v2::Controls previous{};
    bool started=false,hasPath=false;
    int steps=1;
    LinearRamp mix,gain,mono;
    AlignmentDelay dryDelay;
    std::array<std::vector<float>,2> dry;
};
}
