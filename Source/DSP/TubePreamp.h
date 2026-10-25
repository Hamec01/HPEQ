#pragma once

#include <array>
#include <memory>
#include <vector>

namespace juce::dsp
{
template <typename T>
class Oversampling;
}

namespace hpeq::dsp
{
// BRING LENIN BACK / BRING STALIN BACK (owner, 2026-10-26; research SOVIET_TUBE_PREAMP_RESEARCH_RU.md):
// two Soviet tube preamplifiers as a colour for the whole HPEQ output, each switched by its button.
//
//  LENIN  = LOMO UP-25 (Cherkasov, "Spravochnik kinomekhanika", 1979, pp. 201-204, tables 24 / 25):
//           6Zh32P pentode -> 6N3P triode (plate-to-grid feedback, R6 / R7) -> 6N3P triode -> 6N3P pair in
//           series (totem pole, R16 / R25 = 220 ohm, feedback R19 / R28). Operating points from table 25.
//           Capacitor roles from table 24: C1 50 uF / 15 V and C9 10 uF are cathode bypasses (L1, L2 left); the
//           4 uF parts, listed without a voltage, are taken as supply decoupling; 0.033 uF ones as coupling.
//  STALIN = LOMO UP-27 (forum measurements, no factory schematic found): input transformer -> three 6Zh32P
//           pentode stages -> 6N3P output stage. Values are a hypothesis of a typical layout.
//
// Each tube is a Koren model fitted to the datasheet points (6N3P at 150 V / 240 ohm: 8.5 mA, 5.8 mA/V,
// mu 36; 6Zh32P at 250 / 140 V, -2 V: 3 mA, 1.8 mA/V). Each stage's static behaviour along its AC load
// line (with its feedback) is solved once into a table; the dynamics around it run per sample: the
// coupling capacitor into the grid leak, grid current when the grid goes positive (it charges the
// coupling capacitor: blocking after an overload), the cathode bypass capacitor (the bias moves with
// the signal), the transformer core's flux (STALIN: the core saturates first at low frequencies).
// Circuit-informed, not hardware-calibrated: no real UP-25 / UP-27 was measured.
namespace tube
{
struct Triode
{
    double mu, ex, kg1, kp, kvb;
    double plateCurrent(double ug, double ua) const noexcept;
};
struct Pentode
{
    double mu, ex, kg1, kp, kvb, kg2;
    double plateCurrent(double ug1, double ug2, double ua) const noexcept;
    double screenCurrent(double ug1, double ug2) const noexcept;
};
extern const Triode k6N3P;
extern const Pentode k6Zh32P;

// One common-cathode stage.
struct StageSpec
{
    bool pentode = false;
    double supply = 250.0;  // B+ (V)
    double plateR = 100e3;  // DC plate load (ohm)
    double cathodeR = 1e3;  // cathode resistor (ohm): sets the bias
    double cathodeC = 0.0;  // bypass capacitor (F); 0 = unbypassed (local feedback)
    double screenV = 0.0;   // pentode: screen voltage (bypassed)
    double acLoad = 100e3;  // plate load for the signal (plate resistor || next grid leak)
    double feedback = 0.0;  // plate-to-grid feedback ratio
    double couplingC = 0.0; // capacitor into this grid (F); 0 = direct
    double gridLeak = 1e6;  // grid leak (ohm)
    double sourceR = 1e3;   // what drives the grid (the previous plate's AC load)
};

class Stage
{
public:
    void prepare(const StageSpec&, double sampleRate);
    void reset() noexcept;
    double process(double input) noexcept; // grid volts in (AC), plate volts out (AC, inverted)
    double staticOutput(double grid) const noexcept; // the table alone, at the resting bias (calibration)
    double restingBias() const noexcept { return restingCathode; }
    double gridShift() const noexcept { return couplingV; }

private:
    double lookup(double s, double& plateCurrent) const noexcept;

    StageSpec spec;
    double dt = 1.0 / 192000.0;
    static constexpr int kTable = 4096;
    double tableMin = -20.0, tableStep = 0.01;
    std::vector<double> plateSwing, current; // per s
    double restingCurrent = 0.0, restingCathode = 0.0, restingPlate = 0.0;
    double restingSwing = 0.0; // the table at rest (one table step from 0): taken off, so no signal = no swing
    double cathodeV = 0.0, couplingV = 0.0;
};

// LOMO UP-27's input transformer: primary resistance, magnetising inductance with a saturating core
// (the flux is the integral of the voltage, so low frequencies saturate first), leakage resonance.
class Transformer
{
public:
    void prepare(double sampleRate, double saturationVolts);
    void reset() noexcept;
    double process(double input) noexcept;

private:
    double dt = 1.0 / 192000.0, flux = 0.0, fluxSat = 1.0, inductance = 6.4, resistance = 600.0;
    double b0 = 1.0, b1 = 0.0, b2 = 0.0, a1 = 0.0, a2 = 0.0, x1 = 0.0, x2 = 0.0, y1 = 0.0, y2 = 0.0;
};

enum class Model { Lenin, Stalin };

// One channel of one preamp, from the plug-in's sample values to the plug-in's sample values: small
// signals pass at unity gain (calibrated at prepare), loud ones go into the tubes.
class Preamp
{
public:
    void prepare(Model, double oversampledRate);
    void reset() noexcept;
    double process(double x) noexcept;
    double inputScale() const noexcept { return inScale; }

private:
    double staticChain(double volts) const noexcept;
    Model model = Model::Lenin;
    std::vector<Stage> stages;
    Transformer transformer;
    double inScale = 1.0, outScale = 1.0, dcBlockR = 0.9999, dcX = 0.0, dcY = 0.0;
};
} // namespace tube

// The colour stage of the whole plug-in: LENIN, STALIN or both (LENIN into STALIN), 4x oversampled, 20 ms
// fades when a button changes. With both off and the fades done it does nothing (bit-exact, no latency).
class TubeColour
{
public:
    TubeColour();
    ~TubeColour();
    void prepare(double sampleRate, int maxBlockSize);
    void reset() noexcept;
    void setTargets(bool lenin, bool stalin, double leninMix = 1.0, double stalinMix = 1.0) noexcept;
    void process(float* const* channels, int numChannels, int numSamples) noexcept;
    bool isRunning() const noexcept;
    int latencySamples() const noexcept { return latency; } // while a button is on

private:
    std::unique_ptr<juce::dsp::Oversampling<float>> oversampling;
    std::array<tube::Preamp, 2> lenin, stalin;
    double rate = 48000.0;
    int latency = 0;
    bool wantLenin = false, wantStalin = false;
    double targetLenin = 0.0, targetStalin = 0.0; // the mix each fades to (0 when its button is off)
    double mixLenin = 0.0, mixStalin = 0.0, fadeStep = 0.001;
};
} // namespace hpeq::dsp
