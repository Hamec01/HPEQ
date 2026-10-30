#include "DSP/ScoreEffects.h"
#include <algorithm>
#include <cmath>

namespace hamlo::echo
{
constexpr double pi=3.14159265358979323846;
void EchoSpace::prepare(double fs)
{
    rate=fs;
    for(auto& line:lines) line.data.assign(static_cast<size_t>(std::ceil(.4*fs))+16,0);
    reset();
}
void EchoSpace::reset() noexcept
{
    for(auto& line:lines) { line.pos=0;line.written=0; line.loss=0; }
    inputLow={};
}
void EchoSpace::configure(const ScoreSettings& s) noexcept
{
    constexpr double milliseconds[3][8] {
        {17.3,23.9,31.1,37.7,43.3,53.1,61.7,71.9},
        {7.1,11.3,13.7,17.9,23.3,29.9,37.1,43.7},
        {41.3,53.9,67.1,79.7,97.3,113.1,137.7,157.9}};
    type=std::clamp(s.spaceType,0,2);
    directGain={1,1};
    damping=1-std::exp(-2*pi*std::clamp(s.spaceDamping,100.0,.45*rate)/rate);
    for(size_t i=0;i<lines.size();++i)
    {
        auto& line=lines[i];
        line.length=std::clamp(static_cast<int>(std::lround(milliseconds[type][i]*.001*s.spaceSize*rate)),1,static_cast<int>(line.data.size())-1);
        line.gain=std::pow(10.,-3.*line.length/(rate*std::clamp(s.spaceDecay,.1,20.)));
        line.coupling=std::sqrt(1-line.gain*line.gain);directGain[i%2]*=-line.gain;
    }
}
std::array<double,2> EchoSpace::process(std::array<double,2> input) noexcept
{
    std::array<double,8> v;
    std::array<double,2> out=input;
    for(size_t i=0;i<8;++i)
    {
        auto& l=lines[i]; int at=l.pos-l.length; if(at<0)at+=static_cast<int>(l.data.size());
        const double y=l.written>=l.length?l.data[static_cast<size_t>(at)]:0;
        l.loss+=damping*(y-l.loss); v[i]=l.loss;
    }
    for(int step=1;step<8;step*=2)
        for(int start=0;start<8;start+=2*step)
            for(int j=0;j<step;++j) { const double a=v[static_cast<size_t>(start+j)], b=v[static_cast<size_t>(start+j+step)]; v[static_cast<size_t>(start+j)]=a+b; v[static_cast<size_t>(start+j+step)]=a-b; }
    for(size_t i=0;i<8;++i)
    {
        auto& l=lines[i];
        // Orthogonal two-port coupling plus an orthogonal Hadamard delay junction.
        // The external transfer is passive; removing its direct term and halving
        // retains a gain bound of one for safe placement inside the Echo loop.
        const double internal=v[i]*.3535533905932738,external=out[i%2];
        l.data[static_cast<size_t>(l.pos)]=l.gain*(l.gain*internal+l.coupling*external);
        out[i%2]=l.coupling*internal-l.gain*external;
        if(++l.pos==static_cast<int>(l.data.size()))l.pos=0;
        l.written=std::min(l.written+1,static_cast<int>(l.data.size()));
    }
    for(size_t ch=0;ch<2;++ch)out[ch]=.5*(out[ch]-directGain[ch]*input[ch]);
    return out;
}

void TextureGlitch::prepare(double fs)
{
    rate=fs; capacity=static_cast<int>(std::ceil(4.1*fs))+8;
    for(auto& ch:history)ch.assign(static_cast<size_t>(capacity),0);
    write=0;clock=epoch=0;
}
void TextureGlitch::reset() noexcept { write=0;clock=epoch=0; }
double TextureGlitch::at(int ch,int64_t sample) const noexcept
{
    if(sample<epoch||sample>=clock||sample<=clock-capacity)return 0;
    return history[static_cast<size_t>(ch)][static_cast<size_t>(sample%capacity)];
}
std::array<double,2> TextureGlitch::process(std::array<double,2> x,int mode,double ms,double amount) noexcept
{
    const int length=std::clamp(static_cast<int>(std::lround(ms*.001*rate)),64,static_cast<int>(2*rate));
    const int64_t age=(clock-epoch)%length, frame=clock-age;
    double position=static_cast<double>(frame-length+age);
    if(mode==1){const auto number=(clock-epoch)/length;position=number<1?-1:static_cast<double>(epoch+((number-1)/2)*2*length+age);}
    if(mode==2)position=static_cast<double>(frame-1-age);
    if(mode==3)position=static_cast<double>(frame-length+subdivisionAge(age,length,4));
    if(mode==4) {const double u=static_cast<double>(age)/length;position=frame-length+length*(u-.5*u*u);}
    const int64_t whole=static_cast<int64_t>(std::floor(position)); const double f=position-whole;
    const double edge=std::clamp(std::min(static_cast<double>(age),static_cast<double>(length-1-age))/32.,0.,1.);
    std::array<double,2> out{};
    for(int ch=0;ch<2;++ch)
    {
        double y=at(ch,whole)+f*(at(ch,whole+1)-at(ch,whole));
        if(mode==5)y=0;
        if(mode==0||amount==0)out[static_cast<size_t>(ch)]=x[static_cast<size_t>(ch)];
        else out[static_cast<size_t>(ch)]=x[static_cast<size_t>(ch)]*(1-amount)+amount*y*edge;
        history[static_cast<size_t>(ch)][static_cast<size_t>(write)]=static_cast<float>(x[static_cast<size_t>(ch)]);
    }
    if(++write==capacity)write=0; ++clock;
    return out;
}
}
