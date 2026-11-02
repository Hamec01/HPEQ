#pragma once
#include "DSP/ReflectionPrototype.h"
#include "DSP/ScoreSettings.h"
#include "DSP/ScoreEffects.h"
#include "DSP/Ramp.h"
#include <atomic>

namespace hamlo::echo
{
class ScoreEngine
{
public:
    void prepare(double,int);
    void start(const ScoreSettings&) noexcept;
    void configure(const ScoreSettings&) noexcept;
    void process(float* const*,int,int,bool acceptInput=true,bool wetOnly=false) noexcept;
    void releaseFreeze() noexcept { if(settings.freeze)releaseRemaining=freezeLength;settings.freeze=false;freezeWeight.setTarget(0,steps); }
    double tailSeconds() const noexcept;
    static double estimateTail(const ScoreSettings&) noexcept;
    float outputDb() const noexcept { return meter.load(std::memory_order_relaxed); }
    static bool structuralChange(const ScoreSettings&,const ScoreSettings&) noexcept;
private:
    struct History
    {
        std::array<std::vector<float>,2> data;
        int capacity=1,pos=0;
        int64_t clock=0,epoch=0;
        void prepare(int);
        void reset() noexcept { pos=0;clock=epoch=0; }
        void put(std::array<double,2>) noexcept;
        double at(int,int64_t) const noexcept;
        double ago(int,double) const noexcept;
    };
    struct Filter
    {
        double s1=0,s2=0;
        double low(double x,double a) noexcept {s1+=a*(x-s1);s2+=a*(s1-s2);return s2;}
        double single(double x,double a) noexcept {s1+=a*(x-s1);return s1;}
    };
    struct TapState { std::array<std::array<Filter,2>,3> low,high,hp,lp; };
    struct TapTargets { double low=0,high=0; int length=4,repeats=4;std::array<int,3> delay{},frameDelay{};std::array<std::array<double,2>,3> gain{}; };
    ReflectionPrototype pitch, feedbackPitch;
    std::array<History,8> groups;
    std::array<History,3> feedback;
    History hold;
    EchoSpace space;
    TextureGlitch echoGlitch,tailGlitch;
    std::array<std::array<std::vector<float>,2>,8> groupScratch;
    std::array<std::vector<float>,2> sourceScratch;
    std::array<int,8> mapping{},intervals{};
    std::array<TapState,8> tapState;
    std::array<TapTargets,8> tapTargets;
    std::array<std::array<Filter,2>,3> feedbackLow,feedbackHigh;
    std::array<Filter,2> outputLow,outputHigh;
    std::array<double,2> colorState{};
    ScoreSettings settings;
    hpeq::dsp::LinearRamp dryWeight,wetWeight,outputGain,inputWeight,freezeWeight;
    std::array<hpeq::dsp::LinearRamp,3> bandWeight;
    double rate=48000,duckEnvelope=0,duckGain=1;
    double splitLowCoefficient=0,splitHighCoefficient=0,postLowCoefficient=0,postHighCoefficient=0,colorCoefficient=0,driveGain=1;
    std::array<double,3> loopDelays{};
    std::array<std::array<double,2>,3> bandReturnGain{};
    std::array<double,3> bandReturnBound{};
    int steps=1,blockSize=1,groupCount=0,feedbackShift=0,freezeLength=1;
    int releaseRemaining=0;
    int64_t clock=0,epoch=0;
    std::atomic<float> meter{-120};
    std::array<double,2> reflection(int,int) const noexcept;
    std::array<double,2> color(std::array<double,2>) noexcept;
};
}
