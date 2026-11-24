#pragma once
#include <array>
#include <cstdint>
namespace hpeq::control {
inline constexpr int kMacros=16, kInterval=16, kMaxBindings=64;
// Pointers borrow the current plug-in chunk. Offsets follow every nested split.
struct Frame {
    std::array<const float*,kMacros> macros {};
    std::array<const float*,2> key {};
    int keyChannels=0, offset=0, latency=0;
    std::uint64_t position=0;
};
}
