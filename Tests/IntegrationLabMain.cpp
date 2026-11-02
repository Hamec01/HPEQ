// One instrument before/after production integration; M0 compatibility is separate.
#include "Standalone/ModulePluginProcessor.h"
#include "AllocationAudit.h"
#include "S4Identity.h"
#include <juce_cryptography/juce_cryptography.h>
#include <fstream>
#include <iostream>
#include <memory>
namespace hpeq::dsp {const char* s4SourceIdentity() noexcept;}
namespace
{
using hpeq::standalone::ModulePluginProcessor;
constexpr auto kind=hpeq::standalone::ModuleKind::Shirota;
struct LabPlayHead final:juce::AudioPlayHead
{double bpm=120;juce::Optional<PositionInfo> getPosition() const override {PositionInfo info;info.setBpm(bpm);info.setIsPlaying(true);return info;}};
struct Job {juce::String name;double fs=48000;int block=127,mode=1;bool events=false,neutral=false,bypass=false;int probe=0;};
std::vector<Job> jobs()
{
    std::vector<Job> v;
    for(double fs:{8000.,32000.,44100.,48000.,88200.,96000.,176400.,192000.})for(int mode:{1,2})v.push_back({"rate_"+juce::String(fs,0)+"_m"+juce::String(mode),fs,127,mode});
    for(int block:{1,64,127,512,4096})v.push_back({"events_"+juce::String(block),48000,block,1,true});
    for(int mode:{1,2}){v.push_back({"neutral_"+juce::String(mode),48000,127,mode,false,true});v.push_back({"bypass_"+juce::String(mode),48000,127,mode,false,false,true});}
    for(int i=1;i<=6;++i){Job j;j.name="timing_"+juce::String(i);j.probe=i;v.push_back(j);}
    return v;
}
void configure(ModulePluginProcessor& p,int mode,bool neutral)
{
    p.setParameterValue("v2Mode",mode);
    if(!neutral){p.setParameterValue("v2PositionOn",1);p.setParameterValue("v2Width",2.3);p.setParameterValue("v2Generate",.75);p.setParameterValue("v2Space",.5);p.setParameterValue("v2Motion",1);p.setParameterValue("v2Spread",60);}
}
juce::var object(){return new juce::DynamicObject;}
void set(juce::var& v,const char* name,juce::var value){v.getDynamicObject()->setProperty(name,value);}
}
int main(int argc,char** argv)
{
    juce::ScopedJuceInitialiser_GUI gui;
    try
    {
        if(argc!=2)throw std::runtime_error("IntegrationLab <new-directory>");const juce::File folder(argv[1]);if(folder.exists())throw std::runtime_error("Refusing existing run");folder.createDirectory();
        auto probe=std::make_unique<ModulePluginProcessor>(kind);const bool implemented=probe->parameter("v2Mode")!=nullptr;
        if(implemented&&juce::String(hpeq::dsp::s4SourceIdentity())!=SHIROTA_S4_SOURCE_ID)throw std::runtime_error("Mixed S4 lab/production identity");
        auto root=object();set(root,"implemented",implemented);set(root,"source_id",implemented?SHIROTA_S4_SOURCE_ID:"d1ef423 frozen facade");set(root,"method_id",SHIROTA_S4_METHOD_ID);set(root,"parameter_count",probe->getParameters().size());juce::Array<juce::var> rows;
        for(const auto& j:jobs())
        {
            LabPlayHead head;auto p=std::make_unique<ModulePluginProcessor>(kind);p->setPlayHead(&head);configure(*p,j.mode,j.neutral);if(j.bypass)p->setParameterValue("bypass",1);
            if(j.probe){for(auto value:std::initializer_list<std::pair<const char*,double>>{{"v2Width",1},{"v2Generate",0},{"v2Space",j.probe<4?1.:0.},{"v2Depth",1},{"v2Damping",18000},{"v2Motion",0},{"v2PositionOn",0},{"v2MonoBass",j.probe<4?0.:(j.probe==6?180.:80.)},{"v2Size",j.probe==3?2.:1.},{"v2Predelay",j.probe==3?37.:0.}})p->setParameterValue(value.first,value.second);}
            p->prepareToPlay(j.fs,256);
            const int n=static_cast<int>(j.fs*.5);juce::AudioBuffer<float> data(2,n);for(int i=0;i<n;++i){data.setSample(0,i,static_cast<float>(.1*std::sin(997*juce::MathConstants<double>::twoPi*i/j.fs)));data.setSample(1,i,static_cast<float>(.1*std::sin(1319*juce::MathConstants<double>::twoPi*i/j.fs)));}if(j.probe)for(int i=0;i<n;++i){const float x=static_cast<float>(.1*std::sin(997*juce::MathConstants<double>::twoPi*i/j.fs)+(j.probe>=4?.1*std::sin(60*juce::MathConstants<double>::twoPi*i/j.fs):0));data.setSample(0,i,x);data.setSample(1,i,j.probe>=4?-x:x);}const auto source=data;
            size_t allocations=0;juce::MidiBuffer midi;std::vector<int> points{0,n};if(j.events)points={0,3001,6007,10003,15001,n};else if(j.probe==1||j.probe==4)points={0,12001,n};
            for(int e=0;e<static_cast<int>(points.size())-1;++e)
            {
                if(j.events){head.bpm=e%2?174:90;p->setParameterValue("v2Unit",e%2);p->setParameterValue("v2Node0Angle",e%2?-135:45);p->setParameterValue("v2Mode",e==2?2:(e==3?0:1));p->setParameterValue("v2Position",e%2?143.37:0);p->setParameterValue("v2Predelay",e%2?37:0);p->setParameterValue("v2Size",e%2?2:.5);p->setParameterValue("v2MonoBass",e%2?180:80);p->setParameterValue("v2Motion",e%2);}
                if(e>0&&j.probe==1){p->setParameterValue("v2Predelay",37);p->setParameterValue("v2Size",2);}if(e>0&&j.probe==4)p->setParameterValue("v2MonoBass",180);
                const int begin=points[static_cast<size_t>(e)],end=points[static_cast<size_t>(e+1)];
                for(int pos=begin;pos<end;pos+=j.block)
                {juce::AudioBuffer<float> view(data.getArrayOfWritePointers(),2,pos,juce::jmin(j.block,end-pos));hamlo::test::allocation::begin();p->processBlock(view,midi);allocations+=hamlo::test::allocation::finish();}
            }
            juce::MemoryBlock bytes;bytes.setSize(static_cast<size_t>(n)*8);auto* output=static_cast<float*>(bytes.getData());bool finite=true;double peak=0;
            bool dry=true;const int delay=p->getLatencySamples();
            for(int i=0;i<n;++i)for(int ch=0;ch<2;++ch){const float x=data.getSample(ch,i);*output++=x;finite=finite&&std::isfinite(x);peak=std::max(peak,std::abs(static_cast<double>(x)));dry=dry&&x==(i<delay?0:source.getSample(ch,i-delay));}
            folder.getChildFile(j.name+".f32").replaceWithData(bytes.getData(),bytes.getSize());
            auto row=object();set(row,"name",j.name);set(row,"rate",j.fs);set(row,"latency",delay);set(row,"finite",finite);set(row,"peak",peak);set(row,"allocations",static_cast<int>(allocations));set(row,"dry_exact",dry);set(row,"sha256",juce::SHA256(bytes).toHexString());rows.add(row);
            std::cout<<j.name<<" finite="<<finite<<" allocations="<<allocations<<"\n";
        }
        set(root,"cases",rows);folder.getChildFile("manifest.json").replaceWithText(juce::JSON::toString(root));return 0;
    }
    catch(const std::exception& e){std::cerr<<e.what()<<"\n";return 2;}
}
