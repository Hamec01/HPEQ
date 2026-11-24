#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_cryptography/juce_cryptography.h>
#include "Core/FxProjectState.h"
#include "Rack/NativeApi.h"
#include <cmath>
#include <iostream>

namespace {
juce::var obj(){return new juce::DynamicObject();}
void put(juce::var& o,const char* k,const juce::var& v){o.getDynamicObject()->setProperty(k,v);}
juce::AudioProcessorParameter* parameter(juce::AudioProcessor& p,const juce::String& name){
    for(auto* v:p.getParameters())if(v->getName(128)==name)return v;return nullptr;
}
struct Result {juce::String hash;double energy=0;};
Result render(juce::AudioPluginInstance& p,bool key,bool silent,int block=127){
    const int length=96000, channels=p.getMainBusNumOutputChannels();
    juce::AudioBuffer<float> b(juce::jmax(p.getTotalNumInputChannels(),p.getTotalNumOutputChannels()),block);
    juce::MemoryOutputStream bytes;double energy=0;juce::MidiBuffer midi;
    p.prepareToPlay(48000,256);
    for(int at=0;at<length;at+=block){
        const int n=juce::jmin(block,length-at);b.clear();
        for(int i=0;i<n;++i){const float main=silent?0.f:.01f*std::sin(float(2*juce::MathConstants<double>::pi*997*(at+i)/48000));
            for(int ch=0;ch<channels;++ch)b.setSample(ch,i,main);
            if(key&&b.getNumChannels()>channels)for(int ch=channels;ch<b.getNumChannels();++ch)
                b.setSample(ch,i,.5f*std::sin(float(2*juce::MathConstants<double>::pi*83*(at+i)/48000)));
        }
        juce::AudioBuffer<float> view(b.getArrayOfWritePointers(),b.getNumChannels(),n);p.processBlock(view,midi);
        for(int ch=0;ch<channels;++ch){bytes.write(view.getReadPointer(ch),size_t(n)*sizeof(float));
            if(at>=48000)for(int i=0;i<n;++i)energy+=double(view.getSample(ch,i))*view.getSample(ch,i);}
    }
    p.releaseResources();return {juce::SHA256(bytes.getData(),bytes.getDataSize()).toHexString(),energy};
}
juce::ValueTree firstSlot(juce::ValueTree t){if(t.hasType("FxSlot"))return t;for(auto c:t){auto v=firstSlot(c);if(v.isValid())return v;}return {};}
juce::ValueTree componentTree(juce::AudioPluginInstance& plugin){
    juce::MemoryBlock host;plugin.getStateInformation(host);
    const auto envelope=juce::AudioProcessor::getXmlFromBinary(host.getData(),int(host.getSize()));
    if(!envelope||!envelope->hasTagName("VST3PluginState"))throw std::runtime_error("Missing JUCE host state envelope");
    const auto* component=envelope->getChildByName("IComponent");juce::MemoryBlock bytes;
    if(!component||!bytes.fromBase64Encoding(component->getAllSubText()))throw std::runtime_error("Invalid IComponent state");
    const auto xml=juce::AudioProcessor::getXmlFromBinary(bytes.getData(),int(bytes.getSize()));
    if(!xml||!xml->hasTagName("HPEQ"))throw std::runtime_error("Missing HamloRack component state");
    return juce::ValueTree::fromXml(*xml);
}
void setComponentTree(juce::AudioPluginInstance& plugin,juce::ValueTree root){
    juce::MemoryBlock bytes,wrapped;juce::AudioProcessor::copyXmlToBinary(*root.createXml(),bytes);
    juce::XmlElement host("VST3PluginState");host.createNewChildElement("IComponent")->addTextElement(bytes.toBase64Encoding());
    juce::AudioProcessor::copyXmlToBinary(host,wrapped);plugin.setStateInformation(wrapped.getData(),int(wrapped.getSize()));
}
}
int main(int argc,char** argv){
    juce::ScopedJuceInitialiser_GUI init;
    if(argc<3){std::cerr<<"HPEQ_ControlAudit <bundle> <new output directory> [require]\n";return 2;}
    const juce::File folder(argv[2]);if(folder.exists())return 2;folder.createDirectory();
    juce::AudioPluginFormatManager formats;formats.addFormat(std::make_unique<juce::VST3PluginFormat>());
    juce::OwnedArray<juce::PluginDescription> descriptions;formats.getFormat(0)->findAllTypesForFile(descriptions,argv[1]);
    if(descriptions.size()!=1)return 2;
    juce::var report=obj();put(report,"format","HamloRackControlAudit2");put(report,"plugin_version",descriptions[0]->version);
    int failures=0;juce::Array<juce::var> checks;
    const auto check=[&](const char* name,bool pass){auto r=obj();put(r,"name",name);put(r,"pass",pass);checks.add(r);if(!pass)++failures;};
    const auto create=[&](bool external,bool macro){juce::String error;
        auto p=formats.createPluginInstance(*descriptions[0],48000,256,error);if(!p)throw std::runtime_error(error.toStdString());
        auto layout=p->getBusesLayout();if(external&&layout.inputBuses.size()>1){layout.inputBuses.set(1,juce::AudioChannelSet::stereo());if(!p->setBusesLayout(layout))throw std::runtime_error("SC layout rejected");}
        hpeq::FxProjectState model;auto slot=hpeq::makeNativeModule(hpeq::FxType::Compressor);
        slot.parameters.set("thresholdDb",-24.);slot.parameters.set("ratio",macro?1.:8.);slot.parameters.set("kneeDb",0.);
        slot.parameters.set("attackMs",1.);slot.parameters.set("releaseMs",40.);slot.parameters.set("makeupDb",0.);
        slot.parameters.set("model",0);slot.parameters.set("source",0);slot.parameters.set("autoMakeup",0);
        model.layers[0].mixChain={slot};auto project=hpeq::toValueTree(model);auto module=firstSlot(project);
        if(external){module.setProperty("externalSidechain",true,nullptr);project.setProperty("schemaVersion",3,nullptr);}
        if(macro){juce::ValueTree binding("MacroBinding");binding.setProperty("macro",0,nullptr);binding.setProperty("parameter","makeupDb",nullptr);
            binding.setProperty("minimum",0.,nullptr);binding.setProperty("maximum",1.,nullptr);module.appendChild(binding,nullptr);project.setProperty("schemaVersion",3,nullptr);}
        auto root=componentTree(*p);
        root.removeChild(root.getChildWithName("HpeqProject"),nullptr);root.appendChild(project,nullptr);root.setProperty("schemaVersion",external||macro?3:2,nullptr);
        setComponentTree(*p,root);
        const auto restored=firstSlot(componentTree(*p).getChildWithName("HpeqProject"));
        const bool accepted=restored.isValid()&&int(restored["engineRevision"])==hpeq::native::kRevision
            &&(!external||bool(restored["externalSidechain"]))&&(!macro||restored.getChildWithName("MacroBinding").isValid());
        check(external?"external module state accepted":macro?"macro module state accepted":"internal module state accepted",accepted);
        if(!accepted)throw std::runtime_error("Requested component project was rejected");return p;
    };
    try {
        auto p=create(false,false);const bool macros=parameter(*p,"Macro 01")!=nullptr, trim=parameter(*p,"Final Trim")!=nullptr, sc=p->getBusCount(true)>1;
        put(report,"macros_available",macros);put(report,"sidechain_available",sc);put(report,"final_trim_available",trim);
        const auto baseline=render(*p,false,false);put(report,"internal_pcm_sha256",baseline.hash);
        if(argc>3){check("16 macros available",macros);check("optional SC bus available",sc);check("final trim available",trim);}
        if(macros){for(int i=1;i<=16;++i)check(("macro "+juce::String(i)).toRawUTF8(),parameter(*p,"Macro "+juce::String(i).paddedLeft('0',2))!=nullptr);
            auto m=create(false,true);float norm=0;for(const auto& spec:hpeq::native::layout(hpeq::FxType::Compressor))if(spec.id=="makeupDb")norm=spec.range.convertTo0to1(6.f);
            parameter(*m,"Macro 01")->setValue(norm);const auto result=render(*m,false,false);const double gain=10*std::log10(result.energy/baseline.energy);
            put(report,"macro_gain_db",gain);check("host macro drives native makeup +6 dB",std::abs(gain-6)<.03);}
        if(sc){auto active=create(true,false),quiet=create(true,false),leak=create(true,false);
            const auto duck=render(*active,true,false), noKey=render(*quiet,false,false), silence=render(*leak,true,true);
            const double reduction=10*std::log10(duck.energy/noKey.energy);put(report,"external_reduction_db",reduction);
            check("external key ducks quiet program",reduction<-6);check("external key never leaks into silent program",silence.energy==0);}
        if(trim){auto t=create(false,false);parameter(*t,"Final Trim")->setValue(18.f/36.f);const auto result=render(*t,false,false);
            const double gain=10*std::log10(result.energy/baseline.energy);put(report,"final_trim_db",gain);check("final trim -6 dB",std::abs(gain+6)<.02);}
    }catch(const std::exception& e){put(report,"error",e.what());++failures;}
    put(report,"checks",checks);put(report,"failures",failures);folder.getChildFile("controls.json").replaceWithText(juce::JSON::toString(report));
    std::cout<<juce::JSON::toString(report)<<std::endl;return failures?1:0;
}
