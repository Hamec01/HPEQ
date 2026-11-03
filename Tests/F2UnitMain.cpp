#include "DSP/FaderKernel.h"
#include "AllocationAudit.h"
#include "FaderF2Identity.h"
#include <array>
#include <algorithm>
#include <cmath>
#include <cstring>
#include <iostream>
#include <limits>

using namespace hamlo::fader;
namespace
{
int passed=0,failed=0;
void check(bool ok,const char* name){ok?++passed:++failed;if(!ok)std::cout<<"FAIL "<<name<<std::endl;}
void dc(Kernel& k,double seconds,float left=.04f,float right=.04f,int channels=2,double fs=48000)
{
    std::array<float,256> l{},r{};float* a[]{l.data(),r.data()};const int frames=static_cast<int>(std::lround(seconds*fs));
    for(int n=0;n<frames;n+=256){l.fill(left);r.fill(right);k.process(a,channels,std::min(256,frames-n));}
}
}
int main()
{
    for(double fs:{8000.,44100.,48000.,96000.,192000.})for(int channels:{1,2})
    {
        Kernel k;k.prepare(fs,channels);std::array<float,6> l{0.f,-0.f,1e-39f,-1e-39f,.125f,-.25f},r=l,original=l;float* a[]{l.data(),r.data()};k.process(a,channels,6);check(std::memcmp(l.data(),original.data(),sizeof(l))==0,"neutral signed-zero/subnormal");if(channels==2)check(std::memcmp(r.data(),original.data(),sizeof(r))==0,"neutral right");
    }
    {
        Kernel k;Settings s;s.mode=Mode::Match;s.peakGuard=false;k.set(s);k.prepare(48000,2);k.startLearn();dc(k,1,.1f,.1f);auto proposal=k.stopLearn();check(proposal.valid,"learn valid");check(std::abs(proposal.gainDb-2)<.00001,"RMS proposal known DC");check(k.settings().matchedDb==0,"Stop does not Apply");dc(k,.1);auto old=k.reading().output.rmsI;
        s.matchedDb=proposal.gainDb;k.set(s);dc(k,1,.1f,.1f);check(k.settings().matchedDb==proposal.gainDb,"Apply explicit settings");check(k.reading().output.rmsI>old,"applied gain affects output");k.reset();check(k.settings().matchedDb==proposal.gainDb&&!k.reading().learning,"reset preserves held match");
        k.startLearn();dc(k,.2);check(!k.stopLearn().valid,"short learning rejected");k.startLearn();dc(k,1,0,0);check(!k.stopLearn().valid,"silent learning rejected");
        s.peakGuard=true;s.matchCut=0;s.matchCeiling=-24;s.targetDb=0;k.set(s);k.startLearn();dc(k,1,.5f,.5f);auto impossible=k.stopLearn();check(!impossible.valid&&impossible.limited&&impossible.gainDb>=0,"conflicting headroom/range rejected");
        s.peakGuard=false;s.matchCut=24;s.targetDb=-18;s.bypass=false;k.set(s);k.startLearn();dc(k,1,.1f,.1f);s.bypass=true;k.set(s);check(!k.reading().learning,"bypass cancels learn");check(!k.stopLearn().valid,"cancelled learn cannot propose stale gain");
    }
    {
        Kernel k;Settings s;s.mode=Mode::Ride;s.targetDb=-26;s.profile=Profile::Natural;k.set(s);k.prepare(48000,1);dc(k,1,.04f,0,1);double previous=k.reading().correctionDb;bool crossed=false;
        for(int i=0;i<48000;++i){float x=.0001f;float* a[]{&x};k.process(a,1,1);const auto r=k.reading();if(!r.active&&!crossed){check(r.correctionDb<=previous,"idle starts at applied gain");crossed=true;}previous=r.correctionDb;}
        check(crossed&&std::abs(previous)<.1,"idle returns to neutral");
    }
    {
        Meter m;m.prepare(48000,2);for(int n=0;n<20000;++n)m.push(.1,.1);check(m.reading().readyM&&!m.reading().readyS,"meter window readiness");hamlo::test::allocation::begin();m.reset();auto allocations=hamlo::test::allocation::finish();check(allocations==0,"meter reset allocations");check(m.reading().samples==0&&!m.reading().readyM,"meter reset validity");for(int n=0;n<144000;++n)m.push(0,0);auto r=m.reading();check(r.rmsM==-120&&r.rmsS==-120&&r.rmsI==-120,"reset excludes stale powers");check(!r.readyI&&r.samplePeak==-120,"silent measurement gate and peak");
    }
    {
        // Test the documented finite integration capacity at the lowest rate.
        Meter m;m.prepare(8000,1);constexpr int limit=static_cast<int>(.4*8000)+72000*800;
        for(int n=0;n<limit;++n)m.push(n<4000?.1*std::sin(n*3.14159265358979323846/4):0);
        auto full=m.reading();check(full.full&&full.blocks==72000,"two-hour integration capacity");for(int n=0;n<8000;++n)m.push(.2*std::sin(n*3.14159265358979323846/4));auto later=m.reading();check(later.full&&later.lufsI==full.lufsI,"integration freezes at capacity");check(later.samples>full.samples&&later.readyS,"windows continue when full");m.reset();check(!m.reading().full&&m.reading().blocks==0,"reset releases integration capacity");
    }
    {
        Kernel k;Settings s;s.mode=Mode::Motion;s.sync=false;s.depthDb=24;s.shape=Shape::Sine;k.set(s);k.prepare(48000,2);std::array<float,256> l{},r{};float* a[]{l.data(),r.data()};
        for(int n=0;n<200;++n){l.fill(.1f);r.fill(.1f);Clock c;c.playing=false;k.process(a,2,256,c);}check(k.reading().motionRunning,"free clock runs while stopped");
        s.sync=true;k.set(s);for(int n=0;n<20;++n){l.fill(.1f);r.fill(.1f);Clock c;c.playing=false;k.process(a,2,256,c);}check(!k.reading().motionRunning&&l.back()==.1f,"stopped sync reaches unity");
        l.fill(.1f);r.fill(.1f);s.routing=Routing::RightBoth;s.leftDb=-6;s.polarity=3;s.rightDelayMs=20;
        hamlo::test::allocation::begin();k.set(s);k.startLearn();k.process(a,2,256);k.stopLearn();k.reset();const auto count=hamlo::test::allocation::finish();check(count==0,"all prepared actions allocate zero");
    }
    {
        Kernel k;k.prepare(192000,2);Settings s;s.inputDb=s.gainDb=s.outputDb=24;s.leftDb=s.rightDb=24;k.set(s);std::array<float,256> l{},r{};l.fill(std::numeric_limits<float>::max());r.fill(-std::numeric_limits<float>::max());float* a[]{l.data(),r.data()};k.process(a,2,256);bool finite=true;for(auto v:l)finite=finite&&std::isfinite(v);for(auto v:r)finite=finite&&std::isfinite(v);check(finite,"representational overflow fence");
    }
    {
        Kernel k;Settings s;s.mode=Mode::Motion;s.sync=false;s.shape=Shape::Sine;s.depthDb=24;s.rightPhase=.5;k.set(s);k.prepare(48000,2);
        float l=.1f,r=.1f;float* a[]{&l,&r};k.process(a,2,1);const double beforeL=l,beforeR=r;s.mode=Mode::Trim;k.set(s);l=r=.1f;k.process(a,2,1);
        check(std::abs(l-(beforeL+(.1-beforeL)/960))<1e-7&&std::abs(r-(beforeR+(.1-beforeR)/960))<1e-7,"mode fade preserves each channel's applied gain");
    }
    {
        Kernel k;Settings s;s.gainDb=6;k.set(s);k.prepare(48000,2);dc(k,.1,.1f,.2f);s.enabled=false;k.set(s);float l=.1f,r=.2f;float* a[]{&l,&r};k.process(a,2,1);const double wet=.1*std::pow(10.,6./20);check(std::abs(l-(wet+(.1-wet)/960))<1e-7,"enable first-sample fade");dc(k,.1,.1f,.2f);l=.1f;r=.2f;k.process(a,2,1);check(l==.1f&&r==.2f,"disabled reaches exact dry");
        s={};s.polarity=1;k.set(s);dc(k,.1,.1f,.2f);s.polarity=0;k.set(s);l=.1f;r=.2f;k.process(a,2,1);check(std::abs(l-(-.1+.2/480))<1e-7,"polarity first-sample fade");
        s={};k.set(s);dc(k,.1,.1f,.3f);s.routing=Routing::MonoSum;k.set(s);l=.1f;r=.3f;k.process(a,2,1);check(std::abs(l-(.1+.1/480))<1e-7&&std::abs(r-(.3-.1/480))<1e-7,"routing first-sample fade");
    }
    {
        Kernel a,b;Settings s;s.mode=Mode::Motion;s.sync=false;s.shape=Shape::Sine;s.depthDb=24;s.edgeMs=.1;a.set(s);b.set(s);a.prepare(48000,1);b.prepare(48000,1);float x=.1f,y=.1f;bool same=true;
        for(int n=0;n<=6000;++n){x=y=.1f;float* px[]{&x};float* py[]{&y};Clock stopped;stopped.playing=false;a.process(px,1,1,stopped);b.process(py,1,1);same=same&&x==y;}
        check(same,"free audio ignores stopped transport");check(std::abs(a.reading().phase*24000-6001)<1e-6,"free quarter-cycle sample timing");check(std::abs(20*std::log10(x/.1)+12)<.05,"sine gain at quarter cycle");
    }
    {
        Meter m;m.prepare(48000,1);for(int n=0;n<48000;++n)m.push(1e-4*std::sin(n*3.14159265358979323846/24));check(!m.reading().readyI&&m.reading().lufsI==-120,"absolute loudness gate");m.reset();for(int n=0;n<48000;++n)m.push(1e-3*std::sin(n*3.14159265358979323846/24));check(m.reading().readyI,"above-gate programme available");
    }
    for(auto profile:{Profile::Natural,Profile::Fast})
    {
        Kernel k;Settings s;s.mode=Mode::Ride;s.profile=profile;s.targetDb=-23;k.set(s);k.prepare(48000,1);double lo=100,hi=-100;
        for(int step=0;step<4;++step){const float level=static_cast<float>(.1*(step%2?std::pow(10.,-6./20):1));dc(k,2,level,0,1);const double got=k.reading().output.rmsM;lo=std::min(lo,got);hi=std::max(hi,got);}
        check(hi-lo<=3,"exact 6dB steps reduce variation by at least half");check(std::max(std::abs(lo+23),std::abs(hi+23))<=.2,"exact step levels settle to target");
        s.targetDb=20*std::log10(.04);k.set(s);dc(k,2,.04f,0,1);s.targetDb+=6;k.set(s);dc(k,.25,.04f,0,1);check(std::abs(k.reading().correctionDb-5.4)<.01,"rise reaches 90percent at configured time");dc(k,1,.04f,0,1);s.targetDb-=6;k.set(s);dc(k,.12,.04f,0,1);check(std::abs(k.reading().correctionDb-.6)<.01,"fall reaches 90percent at configured time");
    }
    std::cout<<"Source "<<FADER_F2_SOURCE_ID<<"\nF2Unit: "<<passed<<" passed, "<<failed<<" failed\n";return failed?1:0;
}
