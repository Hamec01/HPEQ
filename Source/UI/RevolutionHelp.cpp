#include "UI/RevolutionHelp.h"

#include <map>

namespace hpeq::ui
{
juce::String revolutionHelp(const juce::String& key)
{
    static const std::map<juce::String, juce::String> texts {
        // ---- SIMPLE
        { "connect", "CONNECT TO HPDG: links to a running HPDG. Click: the first one found (or disconnect); right click: pick one. "
                     "Linked, every new HPDG pattern rewrites the effects from its hits, genre, key and groove (unless LOCK)." },
        { "simpleGenre", "GENRE: which genre's rules write the events. AUTO follows HPDG when linked, else guesses from the tempo." },
        { "simpleIntent", "INTENT: what kind of events - SUBTLE touches, GROOVE moves, FILLS at phrase ends, BREAKDOWN (half-time, "
                          "sweeps), CHAOS bursts." },
        { "simpleAmount", "AMOUNT: how many events and how deep. Turning it up keeps every event you already had and adds more." },
        { "simpleVariation", "VARIATION: 0 = every bar the same. Up: weaker events play by chance, GLITCH gets a second bar, slow "
                             "sweeps over several bars." },
        { "simpleGroove", "GROOVE: 0 = the straight grid. 100 % = the steps move to the groove of HPDG's pattern (linked) or of the "
                          "input heard when you pressed GENERATE." },
        { "mix", "MIX: dry / wet of the whole plug-in." },
        { "simpleFill", "FILL EVERY: where the bigger events go - the last beat of every 1, 2, 4 or 8 bars. OFF: no fills." },
        { "simpleTarget", "TARGET: what is processed - HATS (high band), SNARE (mid), TOPS (everything above the kick), FULL. "
                          "The kick and 808 stay clean unless FULL." },
        { "simpleLock", "LOCK: keep the current pattern - knob moves and new HPDG patterns no longer rewrite it." },
        { "simpleGenerate", "GENERATE: a new pattern (a new seed). Unlinked, it also takes the hits and timing the plug-in heard "
                            "from the input." },
        { "advancedPage", "ADVANCED: the full editor. It shows exactly what SIMPLE wrote, to change by hand." },
        { "simplePage", "SIMPLE: back to the five-knob page (it rewrites the pattern when you turn something there)." },
        { "preview", "What the generator wrote: TIME / GLITCH actions and the levels of the other lanes, with each lane's band." },
        // ---- ADVANCED: global
        { "generate", "GENERATE: a new pattern for the unlocked TIME / GLITCH / MOTION steps from the seed and CHARACTER." },
        { "freeMs", "RATE: the step length in FREE mode (milliseconds)." },
        { "steps", "STEPS: the pattern length in steps (lanes on FOLLOW use it)." },
        { "swing", "SWING: delays every second step (50 % = straight)." },
        { "humanize", "HUMAN: small seeded timing shifts per step, the same on every playback." },
        { "fadeMs", "FADE: the crossfade at every splice and step edge - shorter is snappier, longer is softer." },
        { "order", "ORDER: the order TIME, GLITCH and MOTION run in. SHAPE and THROW lanes always come after them." },
        { "character", "CHARACTER: how GENERATE writes - MINIMAL, MUSICAL, EXPERIMENTAL, DESTRUCTION." },
        { "grid", "GRID: the step length (1/4 .. 1/32)." },
        { "triplet", "TRIPLET: the grid in triplets." },
        { "free", "FREE: steps in milliseconds (RATE) instead of the host's tempo." },
        { "listen", "LISTEN: GENERATE follows the input - repeats on its hits, gaps between them." },
        { "timeStretch", "STRETCH: how TIME's HALF and 2X sound - TAPE (pitch moves), RHYTHM or CLEAN (pitch kept)." },
        { "scene", "SCENE: which of the 8 patterns plays (automatable). Click the status screen to switch, copy or clear." },
        { "bandLowHz", "LOW / MID: the crossover every lane on a band splits at." },
        { "bandHighHz", "MID / HIGH: the upper crossover every lane on a band splits at." },
        { "lanePlate", "The lane's menu: LENGTH (polymeter), RATE, CONDITION, BAND, steps 17 - 32, CURVE (SHAPE)." },
        { "laneCells", "The steps. TIME / GLITCH: click for the next action, drag to paint it over more steps, right click for the "
                       "action and its probability. Level lanes: drag up / down and across to draw." },
        { "laneGenerate", "GEN: a new pattern for this lane only." },
        { "laneLock", "LOCK: lock every step of this lane (GENERATE keeps them)." },
        // ---- lanes
        { "timeOn", "TIME on / off: half speed, reverse, freeze, tape stop, double speed." },
        { "timeMix", "TIME MIX: dry / wet of TIME." },
        { "timeDepth", "TIME DEPTH: how strong TIME's actions are." },
        { "glitchOn", "GLITCH on / off: repeat, stutter, ratchet, reverse, gap, jump, scratch." },
        { "glitchMix", "GLITCH MIX: dry / wet of GLITCH." },
        { "glitchDepth", "GLITCH DEPTH: how strong GLITCH's actions are." },
        { "motionOn", "MOTION on / off: a level per step moves the TARGET." },
        { "motionMix", "MOTION MIX: dry / wet of MOTION." },
        { "motionDepth", "MOTION DEPTH: how far level 15 moves the target." },
        { "motionSmooth", "MOTION SMOOTH: glides between the step levels." },
        { "motionTarget", "MOTION TARGET: volume, low pass, high pass, width or crush." },
        { "shapeAOn", "SHAPE A on / off: a level per step drives its TARGET." },
        { "shapeBOn", "SHAPE B on / off: a level per step drives its TARGET." },
        { "shapeAMix", "SHAPE A MIX: dry / wet." },
        { "shapeBMix", "SHAPE B MIX: dry / wet." },
        { "shapeADepth", "SHAPE A DEPTH: how far level 15 goes." },
        { "shapeBDepth", "SHAPE B DEPTH: how far level 15 goes." },
        { "shapeASmooth", "SHAPE A SMOOTH: ramps into the next step's level." },
        { "shapeBSmooth", "SHAPE B SMOOTH: ramps into the next step's level." },
        { "shapeARes", "SHAPE A RES: the filters' resonance." },
        { "shapeBRes", "SHAPE B RES: the filters' resonance." },
        { "shapeATarget", "SHAPE A TARGET: volume (gate, pump), low / high / band pass, pan, width, drive, crush." },
        { "shapeBTarget", "SHAPE B TARGET: volume (gate, pump), low / high / band pass, pan, width, drive, crush." },
        { "shapeACurve", "SHAPE A CURVE: a smooth curve through the step levels instead of steps." },
        { "shapeBCurve", "SHAPE B CURVE: a smooth curve through the step levels instead of steps." },
        { "throwAOn", "THROW A on / off: per-step throws into an effect." },
        { "throwBOn", "THROW B on / off: per-step throws into an effect." },
        { "throwAMix", "THROW A MIX: how loud the effect comes back." },
        { "throwBMix", "THROW B MIX: how loud the effect comes back." },
        { "throwADepth", "THROW A DEPTH: scales every step's send / intensity." },
        { "throwBDepth", "THROW B DEPTH: scales every step's send / intensity." },
        { "throwATail", "THROW A TAIL: delay feedback, reverb decay, phaser / flanger feedback, or VINYL's brake." },
        { "throwBTail", "THROW B TAIL: delay feedback, reverb decay, phaser / flanger feedback, or VINYL's brake." },
        { "throwATone", "THROW A TONE: damping, the phaser's centre, the flanger's sweep, or VINYL's crackle." },
        { "throwBTone", "THROW B TONE: damping, the phaser's centre, the flanger's sweep, or VINYL's crackle." },
        { "throwATarget", "THROW A TARGET: DELAY, TONAL (a delay tuned to NOTE), REVERB - their tails ring on; PHASER, FLANGER, "
                          "VINYL, DOWNSAMPLE - on the step only. Right click: TIME and NOTE." },
        { "throwBTarget", "THROW B TARGET: DELAY, TONAL (a delay tuned to NOTE), REVERB - their tails ring on; PHASER, FLANGER, "
                          "VINYL, DOWNSAMPLE - on the step only. Right click: TIME and NOTE." },
        { "throwATime", "THROW A TIME: the delay time, or the LFO period of PHASER / FLANGER / VINYL." },
        { "throwBTime", "THROW B TIME: the delay time, or the LFO period of PHASER / FLANGER / VINYL." },
        { "throwANote", "THROW A NOTE: TONAL's pitch (follows HPDG's key when linked)." },
        { "throwBNote", "THROW B NOTE: TONAL's pitch (follows HPDG's key when linked)." },
    };
    const auto it = texts.find(key);
    return it != texts.end() ? it->second : juce::String();
}
} // namespace hpeq::ui
