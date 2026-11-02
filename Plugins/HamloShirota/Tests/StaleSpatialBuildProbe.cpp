// Fault-injection DSP for testing the lab's mixed-build rejection. Never ship.
#include "DSP/SpatialPrototype.h"
namespace hamlo::shirota
{
struct SpatialPrototype::Impl{};
SpatialPrototype::SpatialPrototype()=default;
SpatialPrototype::~SpatialPrototype()=default;
void SpatialPrototype::prepare(double,int){}
void SpatialPrototype::setTargets(const SpatialSettings&) noexcept{}
void SpatialPrototype::process(float*,float*,int) noexcept{}
int SpatialPrototype::latency() const noexcept{return 64;}
double SpatialPrototype::guardDb() const noexcept{return 0;}
bool SpatialPrototype::implemented() noexcept{return true;}
const char* SpatialPrototype::compiledIdentity() noexcept{return "intentional-stale-dsp-fingerprint";}
}
