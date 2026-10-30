// Independent research executable: no plugin, JUCE audio wrapper or Legacy modifications.
#include "DSP/ReflectionPrototype.h"
#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <malloc.h>
#include <new>
#include <string>
#include <vector>
#ifdef _WIN32
#define NOMINMAX
#include <windows.h>
#endif

namespace audit
{
thread_local bool guarded = false;
thread_local size_t allocations = 0;
void* allocate(size_t n)
{
    if (guarded) ++allocations;
    if (auto* p = std::malloc(std::max(size_t(1), n))) return p;
    throw std::bad_alloc();
}
struct Guard { Guard() { guarded = true; } ~Guard() { guarded = false; } };
}
void* operator new(size_t n) { return audit::allocate(n); }
void* operator new[](size_t n) { return audit::allocate(n); }
void operator delete(void* p) noexcept { std::free(p); }
void operator delete[](void* p) noexcept { std::free(p); }
void operator delete(void* p, size_t) noexcept { std::free(p); }
void operator delete[](void* p, size_t) noexcept { std::free(p); }
void* operator new(size_t n, std::align_val_t align)
{
    if (audit::guarded) ++audit::allocations;
    if (auto* p = _aligned_malloc(std::max(size_t(1), n), static_cast<size_t>(align))) return p;
    throw std::bad_alloc();
}
void* operator new[](size_t n, std::align_val_t a) { return ::operator new(n, a); }
void operator delete(void* p, std::align_val_t) noexcept { _aligned_free(p); }
void operator delete[](void* p, std::align_val_t) noexcept { _aligned_free(p); }
void operator delete(void* p, size_t, std::align_val_t) noexcept { _aligned_free(p); }
void operator delete[](void* p, size_t, std::align_val_t) noexcept { _aligned_free(p); }

namespace
{
using hamlo::echo::ReflectionPrototype;
using hamlo::echo::Score;
constexpr double pi = 3.14159265358979323846;
using Audio = std::array<std::vector<float>, 2>;
double threadSeconds()
{
#ifdef _WIN32
    FILETIME created {}, exited {}, kernel {}, user {};
    if (GetThreadTimes(GetCurrentThread(), &created, &exited, &kernel, &user))
    {
        const uint64_t k = (static_cast<uint64_t>(kernel.dwHighDateTime) << 32) | kernel.dwLowDateTime;
        const uint64_t u = (static_cast<uint64_t>(user.dwHighDateTime) << 32) | user.dwLowDateTime;
        return static_cast<double>(k + u) * 1e-7;
    }
#endif
    return 0;
}
struct Case
{
    std::string name, type = "pitch";
    double rate = 48000, hz = 440, rightHz = 440, seconds = 2;
    int channels = 1, shift = 0, block = 256;
    bool events = false;
    bool dense = false;
    int onset = 0;
    int gap = 0;
    Score score;
};

Score one(int shift, double delayMs = 125)
{
    Score score;
    auto& tap = score.taps[0];
    tap.enabled = true; tap.targetNote = score.sourceNote + shift; tap.gain = 1; tap.timeMs = delayMs;
    return score;
}
Score eight(bool reverse = false)
{
    Score score;
    const int shifts[] { -24, -12, -7, 7, 12, 19, 24, 5 };
    for (int i = 0; i < 8; ++i)
    {
        auto& tap = score.taps[static_cast<size_t>(i)];
        tap.enabled = true; tap.targetNote = score.sourceNote + (reverse ? 0 : shifts[i]);
        tap.timeMs = 75 + 47 * i; tap.beats = .25 * (i + 1); tap.sync = (i % 2 == 1);
        tap.gain = .125; tap.pan = (i % 3 - 1) * .4;
        tap.reverse = reverse; tap.reverseMs = 50 + 25 * i;
    }
    return score;
}

Audio inputFor(const Case& c)
{
    const int frames = static_cast<int>(std::lround(c.seconds * c.rate));
    Audio input;
    for (auto& channel : input) channel.resize(static_cast<size_t>(frames));
    uint32_t seed = 0x484d4563;
    for (int i = 0; i < frames; ++i)
    {
        seed = seed * 1664525u + 1013904223u;
        for (int ch = 0; ch < c.channels; ++ch)
        {
            double x;
            if (c.dense)
            {
                const int age = i % static_cast<int>(std::lround(.008 * c.rate));
                uint32_t value = static_cast<uint32_t>(age + ch * 53) * 747796405u + 2891336453u;
                value = ((value >> ((value >> 28u) + 4u)) ^ value) * 277803737u;
                value = (value >> 22u) ^ value;
                x = age < static_cast<int>(std::lround(.006 * c.rate)) ? (static_cast<double>(value >> 8) / 8388608. - 1.) * .4 * std::exp(-age / (.0015 * c.rate)) : 0;
            }
            else if (c.type == "attack_pair") x = (i == c.onset ? .5 : 0) + (i == c.onset + c.gap ? -.35 : 0);
            else if (c.type == "attack_impulse") x = i == c.onset ? .5 : 0;
            else if (c.type == "attack_pulse")
            {
                const int age = i - c.onset, length = static_cast<int>(std::lround(.002 * c.rate));
                x = age >= 0 && age < length ? .4 * std::sin(2 * pi * 800 * age / c.rate) * (.5 - .5 * std::cos(2 * pi * age / length)) : 0;
            }
            else if (c.type == "attack_noise")
            {
                const int age = i - c.onset, length = static_cast<int>(std::lround(.006 * c.rate));
                uint32_t value = static_cast<uint32_t>(std::max(0, age)) * 747796405u + 2891336453u;
                value = ((value >> ((value >> 28u) + 4u)) ^ value) * 277803737u;
                value = (value >> 22u) ^ value;
                x = age >= 0 && age < length ? (static_cast<double>(value >> 8) / 8388608. - 1.) * .4 * std::exp(-age / (.0015 * c.rate)) : 0;
            }
            else if (c.type == "timing" || c.type == "transient") x = i == 0 ? .5 : 0;
            else if (c.type == "reverse") x = (((i * 37 + ch * 19) % 251) - 125) / 512.0;
            else
            {
                const double hz = ch == 0 ? c.hz : c.rightHz;
                x = .25 * std::sin(2 * pi * hz * i / c.rate);
                if (c.type == "determinism") x += (static_cast<double>(seed >> 8) / 8388608.0 - 1.0) * .03;
            }
            input[static_cast<size_t>(ch)][static_cast<size_t>(i)] = static_cast<float>(x);
        }
    }
    return input;
}

Audio render(const Case& c, const Audio& input, size_t& allocations)
{
    ReflectionPrototype renderer;
    renderer.setScore(c.score);
    renderer.prepare(c.rate, 256);
    Audio output;
    const int frames = static_cast<int>(input[0].size());
    for (auto& channel : output) channel.resize(static_cast<size_t>(frames));
    auto score = c.score;
    const size_t before = audit::allocations;
    for (int start = 0; start < frames;)
    {
        int count = std::min(c.block, frames - start);
        if (c.events)
        {
            for (int event : { 10001, 21003, 33007, 47009, 600007, 700003 })
                if (event > start) count = std::min(count, event - start);
            if (start == 10001 && !score.taps[0].reverse) score.taps[2].targetNote = score.sourceNote - 5;
            if (start == 21003) score.bpm = 174;
            if (start == 33007 && !score.taps[0].reverse) score.taps[5].targetNote = score.sourceNote + 3;
            if (start == 47009) score.bpm = 93;
            if (start == 600007) score.bpm = 153;
            if (start == 700003 && !score.taps[0].reverse) score.taps[0].targetNote = score.sourceNote + 19;
        }
        const float* in[] { input[0].data() + start, input[1].data() + start };
        float* out[] { output[0].data() + start, output[1].data() + start };
        {
            audit::Guard guard;
            renderer.setScore(score);
            renderer.process(in, out, c.channels, count);
        }
        start += count;
    }
    allocations = audit::allocations - before;
    return output;
}

void write(const std::filesystem::path& path, const Audio& audio, int channels)
{
    std::ofstream file(path, std::ios::binary);
    if (!file) throw std::runtime_error("Cannot create raw render");
    for (int ch = 0; ch < channels; ++ch)
        file.write(reinterpret_cast<const char*>(audio[static_cast<size_t>(ch)].data()), static_cast<std::streamsize>(audio[0].size() * sizeof(float)));
    if (!file) throw std::runtime_error("Cannot complete raw render");
}

std::vector<Case> cases()
{
    std::vector<Case> cases;
    for (double hz : { 110., 220., 440., 880. })
        for (int shift = -24; shift <= 24; ++shift)
        {
            const double target = hz * std::pow(2., shift / 12.);
            if (target < 55 - 1e-9 || target > 7040) continue;
            Case c; c.name = "pitch_" + std::to_string(static_cast<int>(hz)) + "_" + std::to_string(shift);
            c.hz = hz; c.shift = shift; c.score = one(shift); cases.push_back(c);
        }
    for (double rate : { 44100., 96000. })
        for (int shift : { -24, -12, 7, 12, 24 })
            for (bool identical : { false, true })
            {
                Case c; c.name = "stereo_" + std::to_string(static_cast<int>(rate)) + "_" + std::to_string(shift) + (identical ? "_same" : "_different");
                c.rate = rate; c.shift = shift; c.channels = 2; c.rightHz = identical ? 440 : 660;
                c.score = one(shift); cases.push_back(c);
            }
    for (double rate : { 44100., 48000., 96000. })
    {
        for (double ms : { 1., 125., 500., 4000. })
        {
            Case c; c.name = "timing_" + std::to_string(static_cast<int>(rate)) + "_" + std::to_string(static_cast<int>(ms));
            c.type = "timing"; c.rate = rate; c.channels = 2; c.seconds = ms / 1000. + .02;
            c.score = one(0, ms); cases.push_back(c);
        }
        for (double ms : { 50., 125., 500., 2000. })
        {
            Case c; c.name = "reverse_" + std::to_string(static_cast<int>(rate)) + "_" + std::to_string(static_cast<int>(ms));
            c.type = "reverse"; c.rate = rate; c.channels = 2; c.seconds = .125 + 3 * ms / 1000.;
            c.score = one(0); c.score.taps[0].reverse = true; c.score.taps[0].reverseMs = ms;
            cases.push_back(c);
        }
    }
    for (bool reverse : { false, true })
        for (int channels : { 1, 2 })
            for (int block : { 1, 64, 127, 512, 4096 })
            {
                Case c; c.name = std::string("blocks_") + (reverse ? "reverse_" : "pitch_") + std::to_string(channels) + "_" + std::to_string(block);
                c.type = "determinism"; c.channels = channels; c.seconds = 1.4; c.block = block;
                c.score = eight(reverse); c.events = true; cases.push_back(c);
            }
    for (int shift : { -24, -12, 7, 12, 24 })
    {
        Case c; c.name = "transient_" + std::to_string(shift); c.type = "transient";
        c.shift = shift; c.seconds = 1; c.score = one(shift); cases.push_back(c);
    }
    Case alias; alias.name = "out_of_band_10k_plus24"; alias.type = "alias"; alias.hz = 10000;
    alias.shift = 24; alias.score = one(24); cases.push_back(alias);
    return cases;
}

void renderCases(const std::filesystem::path& directory, const std::vector<Case>& selected)
{
    if (std::filesystem::exists(directory)) throw std::runtime_error("Refusing to overwrite a run directory");
    std::filesystem::create_directories(directory);
    std::ofstream manifest(directory / "manifest.tsv");
    manifest << "case\ttype\trate\tchannels\tframes\thz\tright_hz\tshift\tdelay\treverse_length\tpitch_bound\tallocations\n" << std::setprecision(17);
    for (const auto& c : selected)
    {
        const auto source = inputFor(c);
        size_t allocations = 0;
        const auto out = render(c, source, allocations);
        write(directory / (c.name + ".f32"), out, c.channels);
        ReflectionPrototype probe; probe.setScore(c.score); probe.prepare(c.rate, 256);
        const int delay = static_cast<int>(std::lround(c.score.taps[0].timeMs * c.rate / 1000.));
        manifest << c.name << '\t' << c.type << '\t' << c.rate << '\t' << c.channels << '\t' << source[0].size()
                 << '\t' << c.hz << '\t' << c.rightHz << '\t' << c.shift << '\t' << delay << '\t' << probe.reverseAccumulationSamples(0)
                 << '\t' << probe.pitchAccumulationSamples(c.shift) << '\t' << allocations << '\n';
        manifest.flush();
        std::cout << "RENDER " << c.name << " allocations=" << allocations << std::endl;
    }
    std::cout << "COMPLETE " << selected.size() << " cases\n";
}

void wrapStress(const std::filesystem::path& directory)
{
    std::vector<Case> selected;
    for (bool reverse : { false, true })
        for (int block : { 64, 4096 })
        {
            Case c; c.name = std::string("wrap_") + (reverse ? "reverse_" : "pitch_") + std::to_string(block);
            c.type = "determinism"; c.rate = 96000; c.channels = 2; c.seconds = 12;
            c.block = block; c.events = true; c.score = eight(reverse);
            selected.push_back(c);
        }
    for (int block : { 64, 4096 })
    {
        Case c; c.name = "wrap_dense_" + std::to_string(block); c.type = "determinism";
        c.rate = 96000; c.channels = 2; c.seconds = 12; c.block = block;
        c.events = true; c.dense = true; c.score = eight();
        c.score.taps[0].timeMs = 4000; c.score.taps[0].sync = false;
        selected.push_back(c);
    }
    renderCases(directory, selected);
}

void attackMatrix(const std::filesystem::path& directory, bool edges = false)
{
    if (std::filesystem::exists(directory)) throw std::runtime_error("Refusing to overwrite attack run");
    std::filesystem::create_directories(directory);
    std::ofstream manifest(directory / "attacks.tsv");
    manifest << "case\ttype\trate\tchannels\tshift\tonset\tdelay\tstart\tframes\tallocations\tgap\n";
    std::vector<Case> selected;
    if (!edges) for (int shift = -24; shift <= 24; ++shift)
        for (int phase = 0; phase < 16; ++phase)
            for (const char* type : { "attack_impulse", "attack_pulse", "attack_noise" })
            {
                Case c; c.type = type; c.shift = shift; c.onset = 16000 + 239 * phase;
                c.name = c.type + "_" + std::to_string(shift) + "_" + std::to_string(phase);
                c.seconds = 1; c.score = one(shift);
                selected.push_back(c);
            }
    if (edges)
    {
        for (double rate : { 44100., 96000. })
            for (int shift : { -24, -12, 0, 7, 24 })
                for (double delayMs : { 1., 4000. })
                    for (const char* type : { "attack_impulse", "attack_pulse", "attack_noise" })
                    {
                        Case c; c.type = type; c.shift = shift; c.rate = rate; c.channels = 2;
                        c.onset = static_cast<int>(std::lround(rate * .2)) + 113;
                        c.name = c.type + "_edge_" + std::to_string(static_cast<int>(rate)) + "_" + std::to_string(shift) + "_" + std::to_string(static_cast<int>(delayMs));
                        c.score = one(shift, delayMs);
                        c.seconds = c.onset / rate + std::max(.08, delayMs / 1000.) + .32;
                        selected.push_back(c);
                    }
        for (int shift : { -24, -12, 0, 7, 24 })
            for (double gap : { .008, .016, .03 })
            {
                Case c; c.type = "attack_pair"; c.shift = shift; c.onset = 16019;
                c.gap = static_cast<int>(std::lround(gap * c.rate));
                c.name = "pair_" + std::to_string(shift) + "_" + std::to_string(c.gap);
                c.score = one(shift); c.seconds = 1;
                selected.push_back(c);
            }
    }
    for (const auto& c : selected)
    {
                const auto input = inputFor(c);
                size_t allocations = 0;
                auto output = render(c, input, allocations);
                const double delayMs = c.shift == 0 ? c.score.taps[0].timeMs : std::max(80.0, c.score.taps[0].timeMs);
                const int delay = static_cast<int>(std::lround(delayMs * c.rate / 1000.));
                const int start = c.onset + delay - static_cast<int>(std::lround(.032 * c.rate));
                const int length = static_cast<int>(std::lround(.332 * c.rate));
                Audio cropped;
                for (int ch = 0; ch < c.channels; ++ch)
                    cropped[static_cast<size_t>(ch)].assign(output[static_cast<size_t>(ch)].begin() + start, output[static_cast<size_t>(ch)].begin() + start + length);
                write(directory / (c.name + ".f32"), cropped, c.channels);
                manifest << c.name << '\t' << c.type << '\t' << c.rate << '\t' << c.channels << '\t' << c.shift << '\t' << c.onset
                         << '\t' << delay << '\t' << start << '\t' << length << '\t' << allocations << '\t' << c.gap << '\n';
    }
    std::cout << "COMPLETE " << selected.size() << " attack cases\n";
}

void benchmark(const std::filesystem::path& report, bool reverse, bool dense = false)
{
    std::ofstream file(report);
    file << "instances\treverse\tmean_percent\tp99_percent\tworst_percent\tallocations\thistory_bytes_per_instance\tshared_table_bytes\tthread_mean_percent\n" << std::setprecision(12);
    for (int instances : { 1, 2 })
    {
        std::array<ReflectionPrototype, 2> processors;
        const auto score = eight(reverse);
        for (auto& p : processors) { p.setScore(score); p.prepare(48000, 256); }
        Audio input, output;
        for (auto* group : { &input, &output }) for (auto& ch : *group) ch.resize(256);
        std::vector<double> percentages;
        percentages.reserve(1000);
        const size_t before = audit::allocations;
        int64_t position = 0;
        double chargedCpu = 0;
        for (int block = 0; block < 1125; ++block)
        {
            for (int i = 0; i < 256; ++i, ++position)
            {
                input[0][static_cast<size_t>(i)] = static_cast<float>(.15 * std::sin(2 * pi * 173 * position / 48000.) + .05 * std::sin(2 * pi * 1009 * position / 48000.));
                input[1][static_cast<size_t>(i)] = static_cast<float>(.13 * std::sin(2 * pi * 251 * position / 48000.) + .04 * std::sin(2 * pi * 1877 * position / 48000.));
                if (dense)
                {
                    const int age = static_cast<int>(position % 384);
                    for (int ch = 0; ch < 2; ++ch)
                    {
                        uint32_t value = static_cast<uint32_t>(age + ch * 53) * 747796405u + 2891336453u;
                        value = ((value >> ((value >> 28u) + 4u)) ^ value) * 277803737u;
                        value = (value >> 22u) ^ value;
                        input[static_cast<size_t>(ch)][static_cast<size_t>(i)] = age < 288 ? static_cast<float>((static_cast<double>(value >> 8) / 8388608. - 1.) * .4 * std::exp(-age / 72.)) : 0.0f;
                    }
                }
            }
            const float* in[] { input[0].data(), input[1].data() };
            float* out[] { output[0].data(), output[1].data() };
            const double cpuBefore = threadSeconds();
            const auto start = std::chrono::steady_clock::now();
            {
                audit::Guard guard;
                for (int p = 0; p < instances; ++p)
                {
                    processors[static_cast<size_t>(p)].setScore(score);
                    processors[static_cast<size_t>(p)].process(in, out, 2, 256);
                }
            }
            const auto end = std::chrono::steady_clock::now();
            const double cpuAfter = threadSeconds();
            if (block >= 188) chargedCpu += cpuAfter - cpuBefore;
            if (block >= 188) percentages.push_back(std::chrono::duration<double>(end - start).count() / (256. / 48000.) * 100.);
        }
        double sum = 0; for (double percent : percentages) sum += percent;
        std::sort(percentages.begin(), percentages.end());
        const double mean = sum / percentages.size();
        file << instances << '\t' << reverse << '\t' << mean << '\t' << percentages[static_cast<size_t>(.99 * (percentages.size() - 1))]
             << '\t' << percentages.back() << '\t' << audit::allocations - before << '\t' << processors[0].historyBytes()
             << '\t' << ReflectionPrototype::sharedTableBytes() << '\t' << chargedCpu / (percentages.size() * 256. / 48000.) * 100 << '\n';
        std::cout << "CPU " << instances << " reverse=" << reverse << " mean=" << mean << '%' << std::endl;
    }
}

void material(const std::filesystem::path& inputPath, const std::filesystem::path& outputPath, int channels, bool reverse)
{
    const auto bytes = std::filesystem::file_size(inputPath);
    const auto frames = static_cast<size_t>(bytes / (sizeof(float) * channels));
    Audio input;
    std::ifstream source(inputPath, std::ios::binary);
    for (int ch = 0; ch < channels; ++ch)
    {
        input[static_cast<size_t>(ch)].resize(frames);
        source.read(reinterpret_cast<char*>(input[static_cast<size_t>(ch)].data()), static_cast<std::streamsize>(frames * sizeof(float)));
    }
    if (!source) throw std::runtime_error("Cannot read material");
    Case c; c.channels = channels; c.score = eight(reverse);
    size_t allocations = 0;
    write(outputPath, render(c, input, allocations), channels);
    std::cout << "MATERIAL frames=" << frames << " allocations=" << allocations << '\n';
}

void noteExample(const std::filesystem::path& directory)
{
    std::filesystem::create_directories(directory);
    Case c; c.type = "pitch"; c.channels = 2; c.seconds = 4.5;
    c.hz = c.rightHz = 440 * std::pow(2., (72 - 69) / 12.); // C5 (MIDI 72), not C6.
    const int notes[] { 60, 67, 72, 48 }; // C4, G4, C5, C3.
    for (int i = 0; i < 4; ++i)
    {
        auto& tap = c.score.taps[static_cast<size_t>(i)];
        tap.enabled = true; tap.targetNote = notes[i]; tap.timeMs = 500 + 1000 * i; tap.gain = 1;
    }
    auto input = inputFor(c);
    for (size_t i = 0; i < input[0].size(); ++i)
    {
        const double t = static_cast<double>(i) / c.rate;
        double envelope = 0;
        if (t < .01) envelope = .5 - .5 * std::cos(pi * t / .01);
        else if (t < .31) envelope = 1;
        else if (t < .35) envelope = .5 + .5 * std::cos(pi * (t - .31) / .04);
        for (auto& ch : input) ch[i] = static_cast<float>(ch[i] * envelope);
    }
    size_t allocations = 0;
    write(directory / "C5_source.f32", input, 2);
    write(directory / "C4_G4_C5_C3_wet.f32", render(c, input, allocations), 2);
    std::cout << "NOTES C5(MIDI72) -> C4(60),G4(67),C5(72),C3(48); allocations=" << allocations << '\n';
}
}

int main(int argc, char** argv)
{
    try
    {
        if (argc == 3 && std::string(argv[1]) == "render") renderCases(argv[2], cases());
        else if (argc == 3 && std::string(argv[1]) == "stress") wrapStress(argv[2]);
        else if (argc == 3 && std::string(argv[1]) == "attacks") attackMatrix(argv[2]);
        else if (argc == 3 && std::string(argv[1]) == "attackedges") attackMatrix(argv[2], true);
        else if (argc == 3 && std::string(argv[1]) == "notes") noteExample(argv[2]);
        else if (argc == 4 && std::string(argv[1]) == "bench") benchmark(argv[2], std::atoi(argv[3]) != 0);
        else if (argc == 5 && std::string(argv[1]) == "bench") benchmark(argv[2], std::atoi(argv[3]) != 0, std::string(argv[4]) == "dense");
        else if (argc == 6 && std::string(argv[1]) == "material") material(argv[2], argv[3], std::clamp(std::atoi(argv[4]), 1, 2), std::atoi(argv[5]) != 0);
        else { std::cerr << "Usage: PrototypeLab render|stress <new-dir> | notes <dir> | bench <report.tsv> <reverse:0|1> | material <planar-input.f32> <output.f32> <channels> <reverse:0|1>\n"; return 2; }
        return 0;
    }
    catch (const std::exception& e) { std::cerr << e.what() << '\n'; return 1; }
}
