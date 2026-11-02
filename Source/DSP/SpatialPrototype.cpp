#include "DSP/SpatialPrototype.h"
#include "SpatialIdentity.h"
#include <algorithm>
#include <cmath>
#include <vector>

namespace hamlo::shirota
{
namespace
{
constexpr double pi=3.14159265358979323846;
double bounded(double x,double lo,double hi,double fallback) noexcept {return std::isfinite(x)?std::clamp(x,lo,hi):fallback;}
double zap(double x) noexcept {return std::abs(x)<1e-30?0:x;}
struct Smooth
{
    double value=0,target=0,step=0;int remaining=0;
    void reset(double x) noexcept {value=target=x;remaining=0;}
    void set(double x,int steps) noexcept {if(x==target)return;target=x;remaining=steps;step=(target-value)/steps;}
    double next() noexcept {if(remaining>0)value=--remaining==0?target:value+step;return value;}
};
struct Delay
{
    std::vector<double> data;int pos=0;
    void prepare(int n){data.assign(static_cast<size_t>(n),0);pos=0;}
    void write(double x) noexcept {data[static_cast<size_t>(pos)]=zap(x);}
    void advance() noexcept {if(++pos==static_cast<int>(data.size()))pos=0;}
    double read(double age) const noexcept
    {
        age=std::clamp(age,0.,static_cast<double>(data.size()-2));const int whole=static_cast<int>(age);const double part=age-whole;
        int a=pos-whole;if(a<0)a+=static_cast<int>(data.size());int b=a-1;if(b<0)b+=static_cast<int>(data.size());
        return data[static_cast<size_t>(a)]*(1-part)+data[static_cast<size_t>(b)]*part;
    }
};
struct Allpass
{
    Delay line;
    double process(double x,double samples,double g=.62) noexcept
    {const double y=line.read(samples)-g*x;line.write(x+g*y);line.advance();return zap(y);}
};
struct Highpass
{
    double b0=1,b1=0,b2=0,a1=0,a2=0,z1=0,z2=0;
    void design(double fs,double hz,double q) noexcept
    {
        const double w=2*pi*hz/fs,c=std::cos(w),a=std::sin(w)/(2*q),den=1+a;
        b0=(1+c)*.5/den;b1=-(1+c)/den;b2=b0;a1=-2*c/den;a2=(1-a)/den;
    }
    double process(double x) noexcept {const double y=b0*x+z1;z1=zap(b1*x-a1*y+z2);z2=zap(b2*x-a2*y);return y;}
};
}
struct SpatialPrototype::Impl
{
    double fs=48000;int d=64,pos=0,hilbertPos=0,steps=1440;
    std::vector<float> dryLeft,dryRight;
    std::vector<double> rawMid;
    std::vector<std::pair<int,double>> hilbert;
    Delay midHistory,sideHistory;
    std::array<Allpass,4> generatorAllpass;
    std::array<std::array<Allpass,4>,2> spaceAllpass;
    Smooth width,generate,space,size,depth,predelay,output,mix,monoBlend,lowPole,highPole,dampingPole;
    std::array<Smooth,3> bands,generatorWeight,spaceWeight;
    std::array<double,3> generatorLow{},wetLowLeft{},wetLowRight{};
    std::array<Highpass,2> monoHighpass;
    double low=0,mid=0,midEnergy=0,sideEnergy=0,guardGain=1,energyPole=.999,attack=.99,release=.999,genPole=.98;
    bool guard=false,started=false;double budget=1;
    void initialise(double rate)
    {
        fs=rate;d=2*static_cast<int>(std::lround(32*fs/48000));steps=std::max(1,static_cast<int>(std::ceil(.03*fs)));
        dryLeft.assign(static_cast<size_t>(d+1),0);dryRight=dryLeft;rawMid.assign(static_cast<size_t>(2*d+1),0);
        for(int lag=1;lag<2*d;lag+=2)
        {const int k=lag-d;const double window=.42-.5*std::cos(2*pi*lag/(2*d))+.08*std::cos(4*pi*lag/(2*d));hilbert.push_back({lag,2*window/(pi*k)});}
        midHistory.prepare(static_cast<int>(fs*.5)+8);sideHistory.prepare(static_cast<int>(fs*.5)+8);
        for(auto& a:generatorAllpass)a.line.prepare(static_cast<int>(fs*.05)+8);
        for(auto& group:spaceAllpass)for(auto& a:group)a.line.prepare(static_cast<int>(fs*.12)+8);
        width.reset(1);generate.reset(0);space.reset(0);size.reset(1);depth.reset(.5);predelay.reset(0);output.reset(1);mix.reset(1);monoBlend.reset(0);
        for(int i=0;i<3;++i){bands[static_cast<size_t>(i)].reset(1);generatorWeight[static_cast<size_t>(i)].reset(i==0?1:0);spaceWeight[static_cast<size_t>(i)].reset(i==0?1:0);}
        lowPole.reset(std::exp(-2*pi*300/fs));highPole.reset(std::exp(-2*pi*3000/fs));dampingPole.reset(std::exp(-2*pi*4500/fs));
        genPole=std::exp(-2*pi*130/fs);energyPole=std::exp(-1/(.05*fs));attack=std::exp(-1/(.003*fs));release=std::exp(-1/(.08*fs));
        monoHighpass[0].design(fs,150,.541196100146197);monoHighpass[1].design(fs,150,1.306562964876377);
    }
    void targets(const SpatialSettings& requested) noexcept
    {
        SpatialSettings p=requested;
        if(!p.enabled){p=SpatialSettings{};p.mix=0;}
        width.set(bounded(p.width,0,4,1),steps);
        for(int i=0;i<3;++i)
        {
            bands[static_cast<size_t>(i)].set(bounded(p.bandWidth[static_cast<size_t>(i)],0,4,1),steps);
            generatorWeight[static_cast<size_t>(i)].set(i==std::clamp(static_cast<int>(p.generator),0,2)?1:0,steps);
            spaceWeight[static_cast<size_t>(i)].set(i==std::clamp(static_cast<int>(p.character),0,2)?1:0,steps);
        }
        generate.set(bounded(p.generate,0,1.5,0),steps);space.set(bounded(p.space,0,1,0),steps);size.set(bounded(p.size,.5,2,1),steps);
        const double z=bounded(p.depth,0,1,.5);depth.set(z,steps);predelay.set(bounded(p.predelayMs,0,60,0)*fs/1000,steps);
        output.set(std::pow(10.,bounded(p.outputDb,-24,12,0)/20),steps);mix.set(bounded(p.mix,0,1,1),steps);
        lowPole.set(std::exp(-2*pi*bounded(p.splitLow,60,1500,300)/fs),steps);
        highPole.set(std::exp(-2*pi*bounded(p.splitHigh,1500,12000,3000)/fs),steps);
        dampingPole.set(std::exp(-2*pi*bounded(p.damping/(1+2*z),500,std::min(18000.,fs*.4),4500)/fs),steps);
        const double hz=bounded(p.monoBassHz,0,500,0);monoBlend.set(hz>=20?1:0,steps);
        if(hz>=20){monoHighpass[0].design(fs,hz,.541196100146197);monoHighpass[1].design(fs,hz,1.306562964876377);}
        guard=p.guard;const double floor=bounded(p.correlationFloor,0,.95,0);budget=(1-floor)/(1+floor);
        // A prepared but unplayed instance has no previous audible setting.
        // Apply its initial state exactly; retain sample ramps for live edits.
        if(!started)
        {
            for(auto* v:{&width,&generate,&space,&size,&depth,&predelay,&output,&mix,&monoBlend,&lowPole,&highPole,&dampingPole})v->reset(v->target);
            for(auto* group:{&bands,&generatorWeight,&spaceWeight})for(auto& v:*group)v.reset(v.target);
        }
    }
    std::pair<double,double> early(int character,double scale,double pre,double damping) noexcept
    {
        constexpr double tight[]{6.7,11.3,17.9,23.9},tightGain[]{.4,.28,.19,.13};
        constexpr double open[]{13.1,19.3,29.9,41.3,53.9,67.1,79.7,93.1},openGain[]{.24,.2,.16,.13,.10,.075,.055,.04};
        constexpr double scatter[]{9.7,17.3,31.1,47.9},scatterGain[]{.32,.28,.23,.17};
        const auto* times=character==0?tight:character==1?open:scatter;const auto* gains=character==0?tightGain:character==1?openGain:scatterGain;
        const int count=character==1?8:4;double l=0,r=0;
        for(int i=0;i<count;++i)
        {
            const double a=pre+times[i]*scale*fs/1000,b=pre+(times[i]*1.13+1.7)*scale*fs/1000;
            l+=gains[i]*(midHistory.read(a)+.6*sideHistory.read(a));r+=gains[i]*(midHistory.read(b)-.6*sideHistory.read(b));
        }
        if(character==2)
        {
            constexpr double ages[]{5.9,8.3,13.1,19.7};
            for(int i=0;i<4;++i){l=spaceAllpass[0][static_cast<size_t>(i)].process(l,ages[i]*scale*fs/1000);r=spaceAllpass[1][static_cast<size_t>(i)].process(r,ages[i]*scale*1.11*fs/1000);}
        }
        auto& zl=wetLowLeft[static_cast<size_t>(character)];auto& zr=wetLowRight[static_cast<size_t>(character)];
        zl=zap(damping*zl+(1-damping)*l);zr=zap(damping*zr+(1-damping)*r);return {zl,zr};
    }
    void sample(float& left,float& right) noexcept
    {
        started=true;
        const double inputL=std::isfinite(left)?left:0,inputR=std::isfinite(right)?right:0;
        dryLeft[static_cast<size_t>(pos)]=static_cast<float>(inputL);dryRight[static_cast<size_t>(pos)]=static_cast<float>(inputR);
        const int read=(pos+1)%(d+1);const float dryL=dryLeft[static_cast<size_t>(read)],dryR=dryRight[static_cast<size_t>(read)];
        const double m=.5*(static_cast<double>(dryL)+dryR),originalS=.5*(static_cast<double>(dryL)-dryR);
        rawMid[static_cast<size_t>(hilbertPos)]=.5*(inputL+inputR);midHistory.write(m);
        const double w=width.next(),g=generate.next(),sp=space.next(),scale=size.next(),z=depth.next(),pre=predelay.next(),volume=output.next(),balance=mix.next(),hp=monoBlend.next();
        const double lo=lowPole.next(),hi=highPole.next(),damp=dampingPole.next();
        const double b0=bands[0].next(),b1=bands[1].next(),b2=bands[2].next();
        std::array<double,3> gw{},sw{};for(int i=0;i<3;++i){gw[static_cast<size_t>(i)]=generatorWeight[static_cast<size_t>(i)].next();sw[static_cast<size_t>(i)]=spaceWeight[static_cast<size_t>(i)].next();}
        double phase=0;
        if(g!=0&&gw[0]!=0)for(const auto& coefficient:hilbert)
        {int idx=hilbertPos-coefficient.first;if(idx<0)idx+=static_cast<int>(rawMid.size());phase+=coefficient.second*rawMid[static_cast<size_t>(idx)];}
        const double micro=.5*(midHistory.read(.004*fs)-midHistory.read(.009*fs));
        double diffuse=m;constexpr double ages[]{3.7,5.3,8.3,11.3};for(int i=0;i<4;++i)diffuse=generatorAllpass[static_cast<size_t>(i)].process(diffuse,ages[i]*fs/1000);
        const double candidates[]{phase,micro,diffuse};double generated=0;
        for(int i=0;i<3;++i)
        {auto& lowState=generatorLow[static_cast<size_t>(i)];lowState=zap(genPole*lowState+(1-genPole)*candidates[i]);generated+=gw[static_cast<size_t>(i)]*(candidates[i]-lowState);}
        // Width and bass controls govern the complete Side, including generation.
        const double side=originalS+g*generated;
        low=zap(lo*low+(1-lo)*side);const double upper=side-low;mid=zap(hi*mid+(1-hi)*upper);const double top=upper-mid;
        double s=(b0==1&&b1==1&&b2==1?side:b0*low+b1*mid+b2*top)*w;
        double filtered=s;for(auto& h:monoHighpass)filtered=h.process(filtered);
        s+=hp*(filtered-s);
        midEnergy=zap(energyPole*midEnergy+(1-energyPole)*m*m);sideEnergy=zap(energyPole*sideEnergy+(1-energyPole)*s*s);
        const double wanted=guard&&sideEnergy>1e-28?std::min(1.,std::sqrt(budget*midEnergy/sideEnergy)):1;
        const double speed=wanted<guardGain?attack:release;guardGain=wanted+speed*(guardGain-wanted);if(std::abs(guardGain-1)<1e-12)guardGain=1;
        s*=guardGain;sideHistory.write(s);
        double wetL=0,wetR=0;
        for(int c=0;c<3;++c){const auto wet=early(c,scale,pre,damp);wetL+=sw[static_cast<size_t>(c)]*wet.first;wetR+=sw[static_cast<size_t>(c)]*wet.second;}
        const double direct=1-.45*sp*z,wetGain=sp*(.35+.35*z);
        const double effectL=direct*(m+s)+wetGain*wetL,effectR=direct*(m-s)+wetGain*wetR;
        const bool neutral=w==1&&b0==1&&b1==1&&b2==1&&g==0&&sp==0&&hp==0&&guardGain==1;
        if((neutral||balance==0)&&volume==1){left=dryL;right=dryR;}
        else {left=static_cast<float>((dryL+balance*(effectL-dryL))*volume);right=static_cast<float>((dryR+balance*(effectR-dryR))*volume);}
        midHistory.advance();sideHistory.advance();pos=read;if(++hilbertPos==static_cast<int>(rawMid.size()))hilbertPos=0;
    }
};
SpatialPrototype::SpatialPrototype()=default;
SpatialPrototype::~SpatialPrototype()=default;
void SpatialPrototype::prepare(double fs,int)
{impl=std::make_unique<Impl>();impl->initialise(bounded(fs,8000,192000,48000));}
void SpatialPrototype::setTargets(const SpatialSettings& p) noexcept {impl->targets(p);}
void SpatialPrototype::process(float* left,float* right,int samples) noexcept {for(int i=0;i<samples;++i)impl->sample(left[i],right[i]);}
int SpatialPrototype::latency() const noexcept {return impl->d;}
double SpatialPrototype::guardDb() const noexcept {return 20*std::log10(std::max(impl->guardGain,1e-30));}
bool SpatialPrototype::implemented() noexcept {return true;}
const char* SpatialPrototype::compiledIdentity() noexcept {return SHIROTA_SPATIAL_SOURCE_ID;}
}
