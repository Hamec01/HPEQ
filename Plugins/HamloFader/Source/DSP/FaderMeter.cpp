#include "DSP/FaderMeter.h"
#include <array>
#include <vector>
#include <algorithm>
#include <cmath>

namespace hamlo::fader
{
namespace
{
double decibels(double power) noexcept{return power>1e-24?10*std::log10(power):-120;}
// ITU-R BS.1770-5 Annex2, rows are successive original-rate samples.
constexpr double fir[12][4]{
{.001708984375,-.0291748046875,-.0189208984375,-.00830078125},
{.010986328125,.029296875,.0330810546875,.014892578125},
{-.0196533203125,-.0517578125,-.0582275390625,-.026611328125},
{.033203125,.089111328125,.1015625,.047607421875},
{-.0594482421875,-.16650390625,-.2003173828125,-.102294921875},
{.1373291015625,.465087890625,.77978515625,.97216796875},
{.97216796875,.77978515625,.465087890625,.1373291015625},
{-.102294921875,-.2003173828125,-.16650390625,-.0594482421875},
{.047607421875,.1015625,.089111328125,.033203125},
{-.026611328125,-.0582275390625,-.0517578125,-.0196533203125},
{.014892578125,.0330810546875,.029296875,.010986328125},
{-.00830078125,-.0189208984375,-.0291748046875,.001708984375}};
struct Peak
{
    double history[2][12]{};int pos=0;double sample=0,interpolated=0;
    void push(double l,double r,int channels) noexcept
    {
        const double x[]{l,r};
        for(int c=0;c<channels;++c)
        {
            sample=std::max(sample,std::abs(x[c]));history[c][pos]=x[c];
            double sums[4]{};
            for(int tap=0;tap<12;++tap){int i=pos-tap;if(i<0)i+=12;const double v=history[c][i];for(int phase=0;phase<4;++phase)sums[phase]+=v*fir[tap][phase];}
            for(double v:sums)interpolated=std::max(interpolated,std::abs(v));
        }
        if(++pos==12)pos=0;
    }
};
struct Filter
{
    double b[3]{},a[3]{1,0,0},z1=0,z2=0;
    double push(double x) noexcept
    {const double y=b[0]*x+z1;z1=b[1]*x-a[1]*y+z2;z2=b[2]*x-a[2]*y;if(std::abs(z1)<1e-30)z1=0;if(std::abs(z2)<1e-30)z2=0;return y;}
    void design(const double* num,const double* den,double fs)
    {
        if(fs==48000){std::copy(num,num+3,b);std::copy(den,den+3,a);return;}
        // Inverse bilinear at48k, then forward bilinear at the requested rate.
        const auto analog=[](const double* p,int order){const double k=96000.;return order==2?p[0]-p[1]+p[2]:order==1?2*k*(p[0]-p[2]):k*k*(p[0]+p[1]+p[2]);};
        const double k=2*fs,A2=analog(den,2),A1=analog(den,1),A0=analog(den,0),B2=analog(num,2),B1=analog(num,1),B0=analog(num,0),d=A2*k*k+A1*k+A0;
        b[0]=(B2*k*k+B1*k+B0)/d;b[1]=2*(B0-B2*k*k)/d;b[2]=(B2*k*k-B1*k+B0)/d;
        a[0]=1;a[1]=2*(A0-A2*k*k)/d;a[2]=(A2*k*k-A1*k+A0)/d;
    }
};
}
struct Meter::Impl
{
    int channels=2,m=19200,s=144000,hop=4800,pos=0,used=0;
    std::uint64_t count=0;
    std::vector<double> raw,weighted,blocks;
    double sumM=0,sumS=0,kM=0,kS=0,total=0,integrated=-120;
    Filter shelf[2],highpass[2];Peak peak;
    bool full=false;
    void reset() noexcept
    {
        // Windows are invalid until rewritten after reset; no large real-time clear.
        pos=used=0;count=0;sumM=sumS=kM=kS=total=0;integrated=-120;peak={};full=false;
        for(auto* group:{shelf,highpass})for(int c=0;c<2;++c)group[c].z1=group[c].z2=0;
    }
    void updateIntegrated() noexcept
    {
        if(used==static_cast<int>(blocks.size())){full=true;return;}
        blocks[static_cast<size_t>(used++)]=std::max(0.,kM)/m;
        constexpr double absolute=1.172465304582298e-7; // -70 LUFS with -0.691 offset.
        double sum=0;int n=0;
        for(int i=0;i<used;++i)if(blocks[static_cast<size_t>(i)]>absolute){sum+=blocks[static_cast<size_t>(i)];++n;}
        if(n==0){integrated=-120;return;}
        const double threshold=std::max(absolute,.1*sum/n);sum=0;n=0;
        for(int i=0;i<used;++i)if(blocks[static_cast<size_t>(i)]>threshold){sum+=blocks[static_cast<size_t>(i)];++n;}
        integrated=n?-.691+10*std::log10(sum/n):-120;
    }
};
Meter::Meter():p(std::make_unique<Impl>()){}Meter::~Meter()=default;
void Meter::prepare(double rate,int channels)
{
    rate=std::isfinite(rate)?std::clamp(rate,8000.,192000.):48000;p->channels=std::clamp(channels,1,2);p->m=static_cast<int>(std::lround(rate*.4));p->s=static_cast<int>(std::lround(rate*3));p->hop=static_cast<int>(std::lround(rate*.1));p->raw.assign(static_cast<size_t>(p->s),0);p->weighted.assign(static_cast<size_t>(p->s),0);p->blocks.assign(72000,0);
    constexpr double b1[]{1.53512485958697,-2.69169618940638,1.19839281085285},a1[]{1,-1.69065929318241,.73248077421585},b2[]{1,-2,1},a2[]{1,-1.99004745483398,.99007225036621};
    for(int c=0;c<2;++c){p->shelf[c].design(b1,a1,rate);p->highpass[c].design(b2,a2,rate);}p->reset();
}
void Meter::reset() noexcept{p->reset();}
void Meter::push(double l,double r) noexcept
{
    if(p->raw.empty())return;
    if(!std::isfinite(l))l=0;if(!std::isfinite(r))r=0;
    const double samples[]{l,r};double raw=0,weighted=0;
    for(int c=0;c<p->channels;++c){raw+=samples[c]*samples[c];const double k=p->highpass[c].push(p->shelf[c].push(samples[c]));weighted+=k*k;}raw/=p->channels;
    int old=p->pos-p->m;if(old<0)old+=p->s;
    const bool haveM=p->count>=static_cast<std::uint64_t>(p->m),haveS=p->count>=static_cast<std::uint64_t>(p->s);
    p->sumM+=raw-(haveM?p->raw[static_cast<size_t>(old)]:0);p->kM+=weighted-(haveM?p->weighted[static_cast<size_t>(old)]:0);p->sumS+=raw-(haveS?p->raw[static_cast<size_t>(p->pos)]:0);p->kS+=weighted-(haveS?p->weighted[static_cast<size_t>(p->pos)]:0);
    p->raw[static_cast<size_t>(p->pos)]=raw;p->weighted[static_cast<size_t>(p->pos)]=weighted;p->total+=raw;++p->count;if(++p->pos==p->s)p->pos=0;p->peak.push(l,r,p->channels);
    if(p->count>=static_cast<std::uint64_t>(p->m)&&(p->count-p->m)%p->hop==0&&!p->full)p->updateIntegrated();
}
void Meter::finishPeak() noexcept
{auto drained=p->peak;for(int i=0;i<12;++i)drained.push(0,0,p->channels);p->peak.interpolated=std::max(p->peak.interpolated,drained.interpolated);}
MeterReading Meter::reading() const noexcept
{
    MeterReading r;r.samples=p->count;r.blocks=static_cast<std::uint64_t>(p->used);r.readyM=p->count>=static_cast<std::uint64_t>(p->m);r.readyS=p->count>=static_cast<std::uint64_t>(p->s);r.readyI=p->integrated>-120;r.full=p->full;
    const auto m=std::max<std::uint64_t>(1,std::min<std::uint64_t>(p->count,static_cast<std::uint64_t>(p->m))),s=std::max<std::uint64_t>(1,std::min<std::uint64_t>(p->count,static_cast<std::uint64_t>(p->s)));
    r.rmsM=decibels(std::max(0.,p->sumM)/m);r.rmsS=decibels(std::max(0.,p->sumS)/s);r.rmsI=decibels(p->total/std::max<std::uint64_t>(1,p->count));r.lufsM=decibels(std::max(0.,p->kM)/m)-.691;r.lufsS=decibels(std::max(0.,p->kS)/s)-.691;r.lufsI=p->integrated;r.samplePeak=decibels(p->peak.sample*p->peak.sample);const auto tp=std::max(p->peak.sample,p->peak.interpolated);r.truePeak=decibels(tp*tp);return r;
}
}
