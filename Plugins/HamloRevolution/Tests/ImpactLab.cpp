#include "Standalone/ModulePluginProcessor.h"
#include "TestSignals.h"
#include <juce_cryptography/juce_cryptography.h>
#include <iostream>
using P=hpeq::standalone::ModulePluginProcessor;
struct Head:juce::AudioPlayHead{double q=0;juce::Optional<PositionInfo> getPosition()const override{PositionInfo p;p.setBpm(120);p.setIsPlaying(true);p.setPpqPosition(q);return p;}};
int main(int argc,char**argv)
{
 juce::ScopedJuceInitialiser_GUI gui;if(argc!=2||juce::File(argv[1]).exists())return 2;
 juce::Array<juce::var> rows;
 for(int material=0;material<2;++material)for(int recipe=0;recipe<4;++recipe)
 {
  auto p=std::make_unique<P>(hpeq::standalone::ModuleKind::Revolution);
  if(recipe>0){if(recipe>=2)p->setParameterValue("simpleTarget",3);if(recipe==3){p->setParameterValue("simpleIntent",.95);p->setParameterValue("simpleAmount",1);p->setParameterValue("simpleVariation",.65);p->setParameterValue("simpleFill",1);}p->generateSimple(false);}
  auto x=material?hamlo::test::sine(16,80,-12):hamlo::test::drums(16);auto y=x;Head head;p->setPlayHead(&head);p->prepareToPlay(48000,256);juce::MidiBuffer m;
  for(int at=0;at<y.getNumSamples();at+=512){const int n=std::min(512,y.getNumSamples()-at);head.q=at/24000.;juce::AudioBuffer<float> b(y.getArrayOfWritePointers(),2,at,n);p->processBlock(b,m);}
  const int latency=p->getLatencySamples();double delta=0,dry=0;
  for(int c=0;c<2;++c)for(int i=latency;i<y.getNumSamples();++i){const auto a=x.getSample(c,i-latency);const auto d=y.getSample(c,i)-a;delta+=d*d;dry+=a*a;}
  juce::MemoryBlock pcm;for(int c=0;c<2;++c)pcm.append(y.getReadPointer(c),sizeof(float)*static_cast<size_t>(y.getNumSamples()));
  auto row=juce::var(new juce::DynamicObject);auto* o=row.getDynamicObject();o->setProperty("material",material?"80Hz bass":"drums");o->setProperty("recipe",recipe);o->setProperty("delta_db",10*std::log10(std::max(delta/dry,1e-30)));o->setProperty("pcm_sha256",juce::SHA256(pcm).toHexString());o->setProperty("latency",latency);
  std::vector<double> gain;double inputEnergy=0,outputEnergy=0;
  for(int at=48000;at+9600<y.getNumSamples();at+=9600){double a=0,b=0;for(int c=0;c<2;++c)for(int i=0;i<9600;++i){const auto d=x.getSample(c,at+i-latency),w=y.getSample(c,at+i);a+=d*d;b+=w*w;}gain.push_back(10*std::log10(std::max(b,1e-30)/std::max(a,1e-30)));inputEnergy+=a;outputEnergy+=b;}
  std::sort(gain.begin(),gain.end());o->setProperty("gain_spread_p95_p05_db",gain[gain.size()*95/100]-gain[gain.size()*5/100]);o->setProperty("output_gain_db",10*std::log10(outputEnergy/inputEnergy));rows.add(row);
  std::cout<<juce::JSON::toString(row,true)<<std::endl;p->setPlayHead(nullptr);
 }
 juce::File f(argv[1]);f.getParentDirectory().createDirectory();f.replaceWithText(juce::JSON::toString(rows));return 0;
}
