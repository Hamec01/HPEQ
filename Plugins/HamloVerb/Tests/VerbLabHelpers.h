#pragma once
#include <algorithm>
#include <cmath>
#include <cstring>
#include <functional>
#include <memory>
#include <vector>
#include <juce_audio_utils/juce_audio_utils.h>
#include "Standalone/ModulePluginProcessor.h"

namespace hamlo::verblab
{
using Processor = hpeq::standalone::ModulePluginProcessor;
using Settings = std::vector<std::pair<juce::String, double>>;
struct Event { int at; juce::String id; double value; };
inline std::unique_ptr<Processor> processor(const Settings& settings, double fs = 48000, int channels = 2)
{
    auto p = std::make_unique<Processor>(hpeq::standalone::ModuleKind::Verb);
    for (const auto& v : settings) p->setParameterValue(v.first, v.second);
    auto layout = p->getBusesLayout();
    const auto set = channels == 1 ? juce::AudioChannelSet::mono() : juce::AudioChannelSet::stereo();
    layout.inputBuses.set(0, set); layout.outputBuses.set(0, set); p->setBusesLayout(layout);
    p->prepareToPlay(fs, 256);
    return p;
}
inline Settings neutral(int engine, double decay = 2, double predelay = 0)
{
    return { {"engine",engine}, {"routing",1}, {"mix",1}, {"space",3}, {"color",2},
        {"predelayMs",predelay}, {"predelaySync",0}, {"decaySeconds",decay}, {"size",1},
        {"attack",0}, {"distance",.5}, {"width",1}, {"earlyDiffusion",.7}, {"lateDiffusion",.7},
        {"modDepth",0}, {"bassMult",1}, {"hfDecayRatio",1}, {"lowCutHz",10}, {"highCutHz",20000},
        {"duck",0}, {"earlyDb",-60}, {"lateDb",0}, {"outputDb",0}, {"timeShape",0},
        {"shimmerAmount",0}, {"freeze",0} };
}
inline void set(Settings& settings, const juce::String& id, double value)
{
    for (auto& s : settings) if (s.first == id) { s.second = value; return; }
    settings.emplace_back(id, value);
}
inline juce::AudioBuffer<float> render(Processor& p, const juce::AudioBuffer<float>& input, int block = 256,
                                     const std::vector<Event>& events = {})
{
    juce::AudioBuffer<float> out(input);
    juce::MidiBuffer midi;
    size_t event = 0;
    for (int start = 0; start < out.getNumSamples();)
    {
        while (event < events.size() && events[event].at == start)
        { p.setParameterValue(events[event].id, events[event].value); ++event; }
        int n = std::min(block, out.getNumSamples() - start);
        if (event < events.size()) n = std::min(n, events[event].at - start);
        juce::AudioBuffer<float> view(out.getArrayOfWritePointers(), out.getNumChannels(), start, n);
        p.processBlock(view, midi); start += n;
    }
    return out;
}
inline void settle(Processor& p, double fs = 48000, double seconds = 1)
{
    juce::AudioBuffer<float> silence(p.getTotalNumInputChannels(), static_cast<int>(fs * seconds)); silence.clear();
    render(p, silence);
}
inline juce::AudioBuffer<float> impulse(const Settings& settings, double seconds, double fs = 48000)
{
    auto p = processor(settings, fs); settle(*p, fs);
    juce::AudioBuffer<float> input(2, static_cast<int>(seconds * fs)); input.clear();
    input.setSample(0, 0, .5f); input.setSample(1, 0, .5f);
    auto out = render(*p, input);
    if (p->parameter("engine") == nullptr || p->parameterValue("engine") == 0)
        for (int ch = 0; ch < 2; ++ch) out.setSample(ch, 0, out.getSample(ch, 0) - .5f);
    return out;
}
inline double energy(const juce::AudioBuffer<float>& x, int first = 0, int count = -1)
{
    const int end = count < 0 ? x.getNumSamples() : std::min(x.getNumSamples(), first + count);
    double sum = 0;
    for (int ch = 0; ch < x.getNumChannels(); ++ch) for (int i = first; i < end; ++i)
        sum += static_cast<double>(x.getSample(ch, i)) * x.getSample(ch, i);
    return sum;
}
inline bool finite(const juce::AudioBuffer<float>& x)
{
    for (int ch = 0; ch < x.getNumChannels(); ++ch) for (int i = 0; i < x.getNumSamples(); ++i)
        if (!std::isfinite(x.getSample(ch, i))) return false;
    return true;
}
inline bool exact(const juce::AudioBuffer<float>& a, const juce::AudioBuffer<float>& b)
{
    if (a.getNumChannels() != b.getNumChannels() || a.getNumSamples() != b.getNumSamples()) return false;
    for (int ch = 0; ch < a.getNumChannels(); ++ch)
        if (std::memcmp(a.getReadPointer(ch), b.getReadPointer(ch), static_cast<size_t>(a.getNumSamples()) * sizeof(float))) return false;
    return true;
}
inline juce::AudioBuffer<float> excitation(double seconds, double fs = 48000, int channels = 2)
{
    juce::AudioBuffer<float> x(channels, static_cast<int>(fs * seconds));
    std::uint32_t rng = 0x1234abcd;
    for (int i = 0; i < x.getNumSamples(); ++i)
    {
        rng = 1664525u * rng + 1013904223u;
        const double noise = (static_cast<double>(rng) / 4294967296.0 - .5) * .16;
        const double t = i / fs, env = std::exp(-std::fmod(t, .227) / .04);
        for (int ch = 0; ch < channels; ++ch)
            x.setSample(ch, i, static_cast<float>((.2 * std::sin(6.283185307179586 * (173 + 91 * ch) * t) + noise) * env));
    }
    return x;
}
struct DecayFit { double seconds = 0, r2 = 0; };
inline DecayFit decay(const juce::AudioBuffer<float>& x, double fs, double bottom = -35)
{
    std::vector<double> edc(static_cast<size_t>(x.getNumSamples()));
    double sum = 0;
    for (int i = x.getNumSamples() - 1; i >= 0; --i)
    {
        for (int ch = 0; ch < x.getNumChannels(); ++ch) sum += static_cast<double>(x.getSample(ch, i)) * x.getSample(ch, i);
        edc[static_cast<size_t>(i)] = sum;
    }
    if (sum < 1e-30) return {};
    double sx=0,sy=0,sxx=0,sxy=0,syy=0,n=0;
    for (int i = 0; i < x.getNumSamples(); ++i)
    {
        const double db = 10 * std::log10(std::max(1e-300, edc[static_cast<size_t>(i)] / sum));
        if (db > -5 || db < bottom) continue;
        const double t = i / fs;
        sx+=t;sy+=db;sxx+=t*t;sxy+=t*db;syy+=db*db;n+=1;
    }
    const double xx=n*sxx-sx*sx, yy=n*syy-sy*sy, xy=n*sxy-sx*sy;
    if (n<3 || xx<=0 || yy<=0 || xy>=0) return {};
    return { -60 * xx / xy, xy*xy/(xx*yy) };
}
inline int onset(const juce::AudioBuffer<float>& x)
{
    const double threshold = std::max(1e-12, static_cast<double>(x.getMagnitude(0,x.getNumSamples())) * 1e-4);
    for (int i=0;i<x.getNumSamples();++i)
        for(int ch=0;ch<x.getNumChannels();++ch) if(std::abs(x.getSample(ch,i))>=threshold) return i;
    return -1;
}
inline double densityTime(const juce::AudioBuffer<float>& x, double fs)
{
    const int first=onset(x), size=static_cast<int>(std::round(.025*fs)) | 1, half=size/2;
    if(first<0) return -1;
    std::vector<double> weights(static_cast<size_t>(size));double norm=0;
    for(int k=0;k<size;++k) norm += weights[static_cast<size_t>(k)] = .5-.5*std::cos(6.283185307179586*k/(size-1));
    for(auto& w:weights) w/=norm;
    for(int centre=first+half;centre+half<x.getNumSamples() && centre<first+static_cast<int>(.7*fs);centre+=std::max(1,static_cast<int>(fs*.001)))
    {
        double variance=0;
        for(int k=0;k<size;++k) {const double v=x.getSample(0,centre-half+k);variance+=weights[static_cast<size_t>(k)]*v*v;}
        if(variance<1e-30) continue;
        const double sigma=std::sqrt(variance);double outside=0;
        for(int k=0;k<size;++k) if(std::abs(x.getSample(0,centre-half+k))>sigma) outside+=weights[static_cast<size_t>(k)];
        if(outside / std::erfc(1/std::sqrt(2.0)) >= .9) return (centre-first)/fs;
    }
    return -1;
}
inline std::pair<double,double> stereo(const juce::AudioBuffer<float>& x)
{
    double ll=0,rr=0,lr=0,mm=0;
    for(int i=0;i<x.getNumSamples();++i){const double l=x.getSample(0,i),r=x.getSample(1,i);ll+=l*l;rr+=r*r;lr+=l*r;mm+=(l+r)*(l+r)*.25;}
    return {lr/std::sqrt(std::max(1e-300,ll*rr)),10*std::log10(std::max(1e-300,mm/(.5*(ll+rr)+1e-300)))};
}
}
