#pragma once
#include <juce_audio_formats/juce_audio_formats.h>
#include <array>
#include <atomic>
#include <deque>
namespace hpeq::standalone
{
class RevolutionSample final:private juce::Thread
{
public:
 struct Status{juce::File source,rendered;juce::String message;bool busy=false,loaded=false,playing=false,loop=true,ready=false;int tailSeconds=2;double seconds=0,progress=0,peakDb=-120;std::array<float,128> wave{};};
 struct Clock{bool active=false;std::uint64_t frame=0;};
 RevolutionSample();~RevolutionSample() override;
 bool load(const juce::File&);
 void useInput();
 void play(bool on) noexcept;
 bool isPlaying() const noexcept{return playing.load();}
 void setLoop(bool on) noexcept{loop.store(on);}
 void prepare(double rate) noexcept;
 Clock fill(juce::AudioBuffer<float>&,int channels) noexcept;
 Status status(std::uint64_t revision) const;
 bool render(const juce::File&,const juce::MemoryBlock& state,std::uint64_t revision,double bpm,int tailSeconds,int channels=2);
private:
 struct Slot{juce::AudioBuffer<float> audio;double rate=48000;std::atomic<int> readers{0};};
 struct Job{bool isLoad=false;juce::File file;juce::MemoryBlock state;std::uint64_t ticket=0,source=0,revision=0;double bpm=120;int tail=2,channels=2;};
 std::array<Slot,3> slots;
 std::atomic<int> active{-1};
 std::atomic<bool> playing{false},loop{true};
 std::atomic<double> rate{48000},progress{0};
 std::atomic<std::uint64_t> restart{1},ticket{0},sourceSerial{0};
 std::uint64_t audioRestart=0,audioFrame=0;int audioPosition=0;
 mutable juce::CriticalSection mutex;
 std::deque<Job> jobs;
 Status info;
 std::uint64_t renderRevision=0,renderSource=0;
 juce::AudioBuffer<float> original;
 double originalRate=0;
 int acquire() noexcept;
 void release(int index) noexcept;
 bool enqueue(Job);
 void run() override;
 void doLoad(const Job&);
 void doRender(const Job&);
 bool publish(const juce::AudioBuffer<float>&,double);
 void message(const juce::String&);
};
}
