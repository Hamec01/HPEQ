#pragma once

#include <array>

namespace hamlo::dsp
{
constexpr int kMaxChannels = 2;

// The saturator's settings in HPEQ's slot layout (Source/DSP/LayerSnapshot.h: FxSlotParams, slot::kDriveDb ..
// kSatNoiseFollow, indices 0 .. 21), so the code copied from HPEQ reads `p.values[slot::k...]` unchanged.
enum class FxType { Saturator };

struct FxSlotParams
{
    bool active = false;
    FxType type = FxType::Saturator;
    float mix = 1.0f;
    std::array<float, 48> values {};
};

namespace slot
{
// Saturator: drive dB 0..48, curve 0 tanh / 1 tube / 2 hard (CLASSIC), bias -0.9..0.9 (CLASSIC), trim dB,
// oversampling 0 1x / 1 2x / 2 4x, auto gain, then the HamloMag controls
constexpr int kDriveDb = 0, kCurve = 1, kBias = 2, kTrimDb = 3, kOversampling = 4, kAutoGain = 5,
              kSatModel = 6, kSatPunish = 7, kSatTone = 8, kSatLowCutHz = 9, kSatHighCutHz = 10, kSatThump = 11, kSatSteep = 12,
              kSatHarmonics = 13, kSatEvenOdd = 14, kSatTapeBias = 15, kSatSpeed = 16, kSatWow = 17, kSatHiss = 18, kSatHum = 19,
              kSatCrackle = 20, kSatNoiseFollow = 21,
              // HamloMag v2 only (docs/MAG_V2.md): SAMPLER 0 off / 1 SP-1200 / 2 MPC60 / 3 S950, PHONE 0..1,
              // SUB Hz (0 = off, 30..300), DYNAMICS -1..1; oversampling index 3 = 8x
              kSatSampler = 22, kSatPhone = 23, kSatSubHz = 24, kSatDynamics = 25;
} // namespace slot
} // namespace hamlo::dsp
