# HamloRack: 250 authored preset designs

These are the immutable raw authoring settings. Executable integration and measured calibration are documented in [RACK_COMPLETE](RACK_COMPLETE.md) and [RACK_BANK_REPORT](RACK_BANK_REPORT.md). Original RACK0 status strings in the design JSON record its authoring stage; runtime calibration overrides live separately in `Assets/Presets/HamloRack/calibration.json`.

Every chain below is ordered. EQ precedes frequency regions; the full-range chain follows. Amount is the rack insert blend or wet-send level; native MIX stays unity. Full exact settings and explicit Revolution scenes are in `Assets/Presets/HamloRack/Design/recipes.json`.

## Boom Bap

### HR001 - BB Classic Front Knock

**Kick / 90 BPM.** Source: isolated kick hits including soft and hard strikes

Keep the first knock forward and remove cardboard behind it.

EQ: LowCut 28 Hz / +0 dB / Q 0.707; Bell 280 Hz / -2 dB / Q 1.2

Full range:

- HamloUdarnik (amount 1): attackDb=2, sustainDb=-1, speed=0.6, style=PRAVDA (0), engine=V2 (1), protect=On (1), lookahead=Vostok (1), listen=Full (0), rise=1, decay=1, sensitivity=0.8, preserve=1, mix=1, outputDb=0
- HamloMag (amount 0.45): model=Iron (3), driveDb=5, mix=1, trimDb=0, oversampling=8x (3), autoGain=On (1), harmonics=0.35, tone=0, evenOdd=0, tapeBias=0.5, speed=1 (0), wow=0, hiss=0, hum=0, crackle=0, noiseFollow=Follow (1), punish=Off (0), thump=Off (0), steep=Off (0), sampler=Off (0), phoneBass=0, subProtectHz=0, dynamics=0, match=Off (0), delta=Off (0)

Reject/revise if: The 50-90 Hz body falls or the leading hit softens.

Intent check: Keep the first knock forward and remove cardboard behind it. Assess against: The 50-90 Hz body falls or the leading hit softens.. Original design-stage calibration status: **not_run**; final numerical results and individual owner-audition status are in RACK_BANK_REPORT.md.

### HR002 - BB Dusty Short Foot

**Kick / 90 BPM.** Source: isolated kick hits including soft and hard strikes

Shorten a long dusty kick while retaining its low fundamental.

EQ: LowCut 26 Hz / +0 dB / Q 0.707; HighShelf 6500 Hz / -2 dB / Q 0.707

Full range:

- HamloZastava (amount 1): v2_engine=Drum (2), v2_enabled=On (1), v2_threshold=-42, v2_range=12, v2_hold=45, v2_release=80, v2_attack=0.3, v2_hysteresis=4, v2_knee=6, v2_external=Main (0), v2_expand=Expand (1), v2_ratio=2, v2_link=1, v2_mix=1, v2_output=0, v2_body=45, v2_tail=80, v2_lookahead=2, v2_hp=0, v2_lp=0, v2_listen=Off (0)
- HamloMag (amount 0.5): model=Tape (1), driveDb=7, mix=1, trimDb=0, oversampling=8x (3), autoGain=On (1), harmonics=0.35, tone=0, evenOdd=0, tapeBias=0.5, speed=1 (0), wow=0, hiss=0, hum=0, crackle=0, noiseFollow=Follow (1), punish=Off (0), thump=Off (0), steep=Off (0), sampler=MPC60 (2), phoneBass=0, subProtectHz=0, dynamics=0, match=Off (0), delta=Off (0)

Reject/revise if: The tail masks the next bass note or the fundamental becomes thin.

Intent check: Shorten a long dusty kick while retaining its low fundamental. Assess against: The tail masks the next bass note or the fundamental becomes thin.. Original design-stage calibration status: **not_run**; final numerical results and individual owner-audition status are in RACK_BANK_REPORT.md.

### HR003 - BB Jazzy Felt Weight

**Kick / 90 BPM.** Source: isolated kick hits including soft and hard strikes

Round the top of a soft acoustic kick without blunting its low pulse.

EQ: LowShelf 85 Hz / +1 dB / Q 0.707; Bell 3000 Hz / -2 dB / Q 1

Full range:

- HamloUdarnik (amount 1): attackDb=-1, sustainDb=1, speed=0.4, style=PRAVDA (0), engine=V2 (1), protect=On (1), lookahead=Vostok (1), listen=Full (0), rise=1, decay=1, sensitivity=0.8, preserve=1, mix=1, outputDb=0
- HamloComp (amount 1): thresholdDb=-23, ratio=2, attackMs=25, releaseMs=167, model=Opto (3), source=Drums (1), kneeDb=6, detector=RMS (1), character=Clean (0), autoMakeup=Off (0), makeupDb=0, mix=1

Reject/revise if: The felt click becomes brighter or 60-120 Hz loses weight.

Intent check: Round the top of a soft acoustic kick without blunting its low pulse. Assess against: The felt click becomes brighter or 60-120 Hz loses weight.. Original design-stage calibration status: **not_run**; final numerical results and individual owner-audition status are in RACK_BANK_REPORT.md.

### HR004 - BB Gold Mid Knock

**Kick / 90 BPM.** Source: isolated kick hits including soft and hard strikes

Add mid harmonics to a sub-heavy kick for small-speaker audibility.

EQ: LowCut 25 Hz / +0 dB / Q 0.707

Full range:

- HamloMag (amount 0.4): model=Tube (2), driveDb=8, mix=1, trimDb=0, oversampling=8x (3), autoGain=On (1), harmonics=0.35, tone=0, evenOdd=0, tapeBias=0.5, speed=1 (0), wow=0, hiss=0, hum=0, crackle=0, noiseFollow=Follow (1), punish=Off (0), thump=Off (0), steep=Off (0), sampler=Off (0), phoneBass=0.2, subProtectHz=65, dynamics=0, match=Off (0), delta=Off (0)
- HamloComp (amount 1): thresholdDb=-20, ratio=2, attackMs=18, releaseMs=145, model=VCA (1), source=Drums (1), kneeDb=6, detector=RMS (1), character=Clean (0), autoMakeup=Off (0), makeupDb=0, mix=1

Reject/revise if: The new midrange buzz overwhelms the dry sub.

Intent check: Add mid harmonics to a sub-heavy kick for small-speaker audibility. Assess against: The new midrange buzz overwhelms the dry sub.. Original design-stage calibration status: **not_run**; final numerical results and individual owner-audition status are in RACK_BANK_REPORT.md.

### HR005 - BB Underground Parallel Heel

**Kick / 90 BPM.** Source: isolated kick hits including soft and hard strikes

Blend a crushed mid-band under the original attack.

EQ: Bell 420 Hz / -1.5 dB / Q 1

Full range:

- HamloComp (amount 0.25): thresholdDb=-31, ratio=7, attackMs=8, releaseMs=125, model=FET (2), source=Drums (1), kneeDb=6, detector=RMS (1), character=Clean (0), autoMakeup=Off (0), makeupDb=0, mix=1

Band 95-1800 Hz:

- HamloMag (amount 0.35): model=Iron (3), driveDb=11, mix=1, trimDb=0, oversampling=8x (3), autoGain=On (1), harmonics=0.35, tone=0, evenOdd=0, tapeBias=0.5, speed=1 (0), wow=0, hiss=0, hum=0, crackle=0, noiseFollow=Follow (1), punish=Off (0), thump=Off (0), steep=Off (0), sampler=Off (0), phoneBass=0, subProtectHz=0, dynamics=0, match=Off (0), delta=Off (0)

Reject/revise if: The dry transient is quieter after loudness matching.

Intent check: Blend a crushed mid-band under the original attack. Assess against: The dry transient is quieter after loudness matching.. Original design-stage calibration status: **not_run**; final numerical results and individual owner-audition status are in RACK_BANK_REPORT.md.

### HR006 - BB Lofi Cassette Pillow

**Kick / 90 BPM.** Source: isolated kick hits including soft and hard strikes

Soften a brittle sampled kick with a slower dark tape response.

EQ: LowCut 30 Hz / +0 dB / Q 0.707; HighCut 9000 Hz / +0 dB / Q 0.707

Full range:

- HamloMag (amount 0.65): model=Tape (1), driveDb=6, mix=1, trimDb=0, oversampling=8x (3), autoGain=On (1), harmonics=0.35, tone=-0.25, evenOdd=0, tapeBias=0.5, speed=3 (2), wow=0, hiss=0, hum=0, crackle=0, noiseFollow=Follow (1), punish=Off (0), thump=Off (0), steep=Off (0), sampler=Off (0), phoneBass=0, subProtectHz=0, dynamics=0, match=Off (0), delta=Off (0)
- HamloUdarnik (amount 1): attackDb=-1, sustainDb=0.5, speed=0.45, style=PRAVDA (0), engine=V2 (1), protect=On (1), lookahead=Vostok (1), listen=Full (0), rise=1, decay=1, sensitivity=0.8, preserve=1, mix=1, outputDb=0

Reject/revise if: Wow bends the bass pitch or the kick becomes a flat thud.

Intent check: Soften a brittle sampled kick with a slower dark tape response. Assess against: Wow bends the bass pitch or the kick becomes a flat thud.. Original design-stage calibration status: **not_run**; final numerical results and individual owner-audition status are in RACK_BANK_REPORT.md.

### HR007 - BB Classic Wooden Crack

**Snare / 90 BPM.** Source: snare or clap hits including ghost notes and a flam

Recover stick attack above a short wooden body.

EQ: LowCut 110 Hz / +0 dB / Q 0.707; Bell 850 Hz / -1.5 dB / Q 1

Full range:

- HamloUdarnik (amount 1): attackDb=2.5, sustainDb=-1, speed=0.65, style=PRAVDA (0), engine=V2 (1), protect=On (1), lookahead=Vostok (1), listen=Full (0), rise=1, decay=1, sensitivity=0.8, preserve=1, mix=1, outputDb=0
- HamloMag (amount 0.3): model=Iron (3), driveDb=5, mix=1, trimDb=0, oversampling=8x (3), autoGain=On (1), harmonics=0.35, tone=0, evenOdd=0, tapeBias=0.5, speed=1 (0), wow=0, hiss=0, hum=0, crackle=0, noiseFollow=Follow (1), punish=Off (0), thump=Off (0), steep=Off (0), sampler=Off (0), phoneBass=0, subProtectHz=0, dynamics=0, match=Off (0), delta=Off (0)

Reject/revise if: The 2-4 kHz crack becomes painful or ghost notes disappear.

Intent check: Recover stick attack above a short wooden body. Assess against: The 2-4 kHz crack becomes painful or ghost notes disappear.. Original design-stage calibration status: **not_run**; final numerical results and individual owner-audition status are in RACK_BANK_REPORT.md.

### HR008 - BB Dusty Sampler Rim

**Snare / 90 BPM.** Source: snare or clap hits including ghost notes and a flam

Give a clean rim a restrained sampled edge and dark room.

EQ: LowCut 140 Hz / +0 dB / Q 0.707; HighShelf 9000 Hz / -2 dB / Q 0.707

Full range:

- HamloMag (amount 0.4): model=Tape (1), driveDb=5, mix=1, trimDb=0, oversampling=8x (3), autoGain=On (1), harmonics=0.35, tone=0, evenOdd=0, tapeBias=0.5, speed=1 (0), wow=0, hiss=0, hum=0, crackle=0, noiseFollow=Follow (1), punish=Off (0), thump=Off (0), steep=Off (0), sampler=SP-1200 (1), phoneBass=0, subProtectHz=0, dynamics=0, match=Off (0), delta=Off (0)
- HamloVerb (amount 0.09): space=Room (1), engine=Warm (2), decaySeconds=0.35, predelayMs=5, predelaySync=Free (0), lowCutHz=300, highCutHz=5500, bassMult=0.75, hfDecayRatio=0.5, width=1.1, modDepth=0.1, duck=0.15, routing=Send (1), mix=1, keepDry=Off (0), wetDb=0, outputDb=0, earlyDb=0, lateDb=0, timeShape=Off (0), shimmerAmount=0, freeze=Off (0)

Reject/revise if: Aliased grit dominates the rim pitch or the room reaches the next backbeat.

Intent check: Give a clean rim a restrained sampled edge and dark room. Assess against: Aliased grit dominates the rim pitch or the room reaches the next backbeat.. Original design-stage calibration status: **not_run**; final numerical results and individual owner-audition status are in RACK_BANK_REPORT.md.

### HR009 - BB Jazzy Ghost Keeper

**Snare / 90 BPM.** Source: snare or clap hits including ghost notes and a flam

Control accents gently while leaving ghost notes readable.

EQ: LowCut 95 Hz / +0 dB / Q 0.707

Full range:

- HamloComp (amount 1): thresholdDb=-24, ratio=1.8, attackMs=20, releaseMs=180, model=Opto (3), source=Drums (1), kneeDb=12, detector=RMS (1), character=Clean (0), autoMakeup=Off (0), makeupDb=0, mix=1
- HamloVerb (amount 0.08): space=Chamber (2), engine=Velvet (3), decaySeconds=0.55, predelayMs=8, predelaySync=Free (0), lowCutHz=180, highCutHz=8000, bassMult=0.75, hfDecayRatio=0.5, width=1.1, modDepth=0.1, duck=0.15, routing=Send (1), mix=1, keepDry=Off (0), wetDb=0, outputDb=0, earlyDb=0, lateDb=0, timeShape=Off (0), shimmerAmount=0, freeze=Off (0)

Reject/revise if: Ghost-note/accent contrast collapses or the brush tail pumps.

Intent check: Control accents gently while leaving ghost notes readable. Assess against: Ghost-note/accent contrast collapses or the brush tail pumps.. Original design-stage calibration status: **not_run**; final numerical results and individual owner-audition status are in RACK_BANK_REPORT.md.

### HR010 - BB Gold Plate Halo

**Snare / 90 BPM.** Source: snare or clap hits including ghost notes and a flam

Put a short plate behind a dry forward backbeat.

EQ: LowCut 110 Hz / +0 dB / Q 0.707; Bell 210 Hz / +1 dB / Q 1

Full range:

- HamloComp (amount 1): thresholdDb=-22, ratio=2.5, attackMs=16, releaseMs=140, model=VCA (1), source=Drums (1), kneeDb=6, detector=RMS (1), character=Clean (0), autoMakeup=Off (0), makeupDb=0, mix=1
- HamloVerb (amount 0.13): space=Plate (4), engine=Warm (2), decaySeconds=0.7, predelayMs=24, predelaySync=Free (0), lowCutHz=350, highCutHz=6500, bassMult=0.75, hfDecayRatio=0.5, width=1.1, modDepth=0.1, duck=0.15, routing=Send (1), mix=1, keepDry=Off (0), wetDb=0, outputDb=0, earlyDb=0, lateDb=0, timeShape=Off (0), shimmerAmount=0, freeze=Off (0)

Reject/revise if: The plate hides the first 20 ms or adds low-mid mud.

Intent check: Put a short plate behind a dry forward backbeat. Assess against: The plate hides the first 20 ms or adds low-mid mud.. Original design-stage calibration status: **not_run**; final numerical results and individual owner-audition status are in RACK_BANK_REPORT.md.

### HR011 - BB Basement Parallel Bark

**Snare / 90 BPM.** Source: snare or clap hits including ghost notes and a flam

Add a dirty parallel body with the clean crack intact.

EQ: Bell 600 Hz / -2 dB / Q 1.3

Full range:

- HamloMag (amount 0.2): model=Germanium (4), driveDb=12, mix=1, trimDb=0, oversampling=8x (3), autoGain=On (1), harmonics=0.35, tone=0, evenOdd=0, tapeBias=0.5, speed=1 (0), wow=0, hiss=0, hum=0, crackle=0, noiseFollow=Follow (1), punish=Off (0), thump=Off (0), steep=Off (0), sampler=Off (0), phoneBass=0, subProtectHz=0, dynamics=0, match=Off (0), delta=Off (0)
- HamloComp (amount 0.3): thresholdDb=-30, ratio=8, attackMs=12, releaseMs=110, model=FET (2), source=Drums (1), kneeDb=6, detector=RMS (1), character=Clean (0), autoMakeup=Off (0), makeupDb=0, mix=1

Reject/revise if: The matched snare loses its crest or the bark rings at 600 Hz.

Intent check: Add a dirty parallel body with the clean crack intact. Assess against: The matched snare loses its crest or the bark rings at 600 Hz.. Original design-stage calibration status: **not_run**; final numerical results and individual owner-audition status are in RACK_BANK_REPORT.md.

### HR012 - BB Lofi Brush Room

**Snare / 90 BPM.** Source: snare or clap hits including ghost notes and a flam

Make a long brush sample softer and more intimate.

EQ: LowCut 140 Hz / +0 dB / Q 0.707; HighCut 10000 Hz / +0 dB / Q 0.707

Full range:

- HamloUdarnik (amount 1): attackDb=-2, sustainDb=-1, speed=0.35, style=PRAVDA (0), engine=V2 (1), protect=On (1), lookahead=Vostok (1), listen=Full (0), rise=1, decay=1, sensitivity=0.8, preserve=1, mix=1, outputDb=0
- HamloVerb (amount 0.12): space=Room (1), engine=Velvet (3), decaySeconds=0.42, predelayMs=0, predelaySync=Free (0), lowCutHz=250, highCutHz=5000, bassMult=0.75, hfDecayRatio=0.5, width=1.1, modDepth=0.1, duck=0.15, routing=Send (1), mix=1, keepDry=Off (0), wetDb=0, outputDb=0, earlyDb=0, lateDb=0, timeShape=Off (0), shimmerAmount=0, freeze=Off (0)

Reject/revise if: The brush sweep turns into gated chatter.

Intent check: Make a long brush sample softer and more intimate. Assess against: The brush sweep turns into gated chatter.. Original design-stage calibration status: **not_run**; final numerical results and individual owner-audition status are in RACK_BANK_REPORT.md.

### HR013 - BB Silk Closed Hats

**Hats / 90 BPM.** Source: closed/open hats, an accent and a dense sixteenth roll

Remove abrasive upper peaks while keeping the timekeeping edge.

EQ: LowCut 500 Hz / +0 dB / Q 0.707; Bell 7200 Hz / +0 dB / Q 1.3 (dynamic -2.5 dB)

Full range:

- HamloMag (amount 0.35): model=Tape (1), driveDb=3, mix=1, trimDb=0, oversampling=8x (3), autoGain=On (1), harmonics=0.35, tone=-0.1, evenOdd=0, tapeBias=0.5, speed=1 (0), wow=0, hiss=0, hum=0, crackle=0, noiseFollow=Follow (1), punish=Off (0), thump=Off (0), steep=Off (0), sampler=Off (0), phoneBass=0, subProtectHz=0, dynamics=0, match=Off (0), delta=Off (0)

Reject/revise if: The 5-8 kHz accents still spit or every tick becomes dull.

Intent check: Remove abrasive upper peaks while keeping the timekeeping edge. Assess against: The 5-8 kHz accents still spit or every tick becomes dull.. Original design-stage calibration status: **not_run**; final numerical results and individual owner-audition status are in RACK_BANK_REPORT.md.

### HR014 - BB Dusty Twelve Bit Hats

**Hats / 90 BPM.** Source: closed/open hats, an accent and a dense sixteenth roll

Add sampled grain above the kick without broadband noise.

EQ: LowCut 700 Hz / +0 dB / Q 0.707; HighCut 12000 Hz / +0 dB / Q 0.707

Full range:

- HamloMag (amount 0.35): model=Tape (1), driveDb=5, mix=1, trimDb=0, oversampling=8x (3), autoGain=On (1), harmonics=0.35, tone=0, evenOdd=0, tapeBias=0.5, speed=1 (0), wow=0, hiss=0, hum=0, crackle=0, noiseFollow=Follow (1), punish=Off (0), thump=Off (0), steep=Off (0), sampler=SP-1200 (1), phoneBass=0, subProtectHz=0, dynamics=0, match=Off (0), delta=Off (0)

Reject/revise if: Idle gaps gain hiss or the top becomes a continuous hash.

Intent check: Add sampled grain above the kick without broadband noise. Assess against: Idle gaps gain hiss or the top becomes a continuous hash.. Original design-stage calibration status: **not_run**; final numerical results and individual owner-audition status are in RACK_BANK_REPORT.md.

### HR015 - BB Jazzy Open Hat Air

**Hats / 90 BPM.** Source: closed/open hats, an accent and a dense sixteenth roll

Let open hats breathe in a small stereo room.

EQ: LowCut 600 Hz / +0 dB / Q 0.707; HighShelf 11000 Hz / +1 dB / Q 0.707

Full range:

- HamloVerb (amount 0.08): space=Ambience (6), engine=Clean (1), decaySeconds=0.45, predelayMs=4, predelaySync=Free (0), lowCutHz=900, highCutHz=10000, bassMult=0.75, hfDecayRatio=0.5, width=1.1, modDepth=0.1, duck=0.15, routing=Send (1), mix=1, keepDry=Off (0), wetDb=0, outputDb=0, earlyDb=0, lateDb=0, timeShape=Off (0), shimmerAmount=0, freeze=Off (0)
- HamloShirota (amount 1): v2Mode=Stereo / Space (1), v2Width=1, v2LowWidth=0, v2MidWidth=1, v2HighWidth=1.15, v2LowSplit=300, v2HighSplit=3000, v2MonoBass=180, v2Generate=0, v2Guard=On (1), v2Floor=0.15, v2Mix=1, v2Output=0, v2Motion=Off (0), v2PositionOn=Off (0), v2Mono=Stereo (0)

Reject/revise if: The mono fold loses the tick or tails smear the swung gaps.

Intent check: Let open hats breathe in a small stereo room. Assess against: The mono fold loses the tick or tails smear the swung gaps.. Original design-stage calibration status: **not_run**; final numerical results and individual owner-audition status are in RACK_BANK_REPORT.md.

### HR016 - BB Swing Echo Tick

**Hats / 90 BPM.** Source: closed/open hats, an accent and a dense sixteenth roll

Add one quiet swung eighth response after the hat.

EQ: LowCut 850 Hz / +0 dB / Q 0.707

Full range:

- HamloEcho (amount 0.12): echoMode=Score (1), quickEnabled=Off (0), scoreRouting=Send (1), scoreMix=1, scoreOutput=0, scoreSource=60, scoreFeedback=0.12, scoreFeedbackSync=1/8 (5), scoreFeedbackMs=500, scoreFeedbackPitch=0, scoreColor=Tape (2), scoreDrive=0, scoreLowCut=1000, scoreHighCut=6000, scoreWidth=1, scoreDuck=0.2, scoreFreeze=Off (0), scoreSpaceMix=0, scoreGlitch=Off (0), scoreGlitchAmount=0, scoreBands=Off (0), scoreSwing=0.18, tap1On=1, tap2On=0, tap3On=0, tap4On=0, tap5On=0, tap6On=0, tap7On=0, tap8On=0, tap1Sync=1/8 (5), tap1Time=250, tap1Note=60, tap1Gain=-6, tap1Pan=0, tap1Shape=Forward (0), tap1Fragment=125, tap1Repeats=3, tap1Low=1000, tap1High=6000, tap1SpaceSend=0

Reject/revise if: A repeat lands on the following main tick and hides the pocket.

Intent check: Add one quiet swung eighth response after the hat. Assess against: A repeat lands on the following main tick and hides the pocket.. Original design-stage calibration status: **not_run**; final numerical results and individual owner-audition status are in RACK_BANK_REPORT.md.

### HR017 - BB Soft Offbeat Tuck

**Hats / 90 BPM.** Source: closed/open hats, an accent and a dense sixteenth roll

Reduce front-edge aggression and push hats behind the snare.

EQ: LowCut 650 Hz / +0 dB / Q 0.707; HighShelf 8500 Hz / -1.5 dB / Q 0.707

Full range:

- HamloUdarnik (amount 1): attackDb=-2, sustainDb=0.5, speed=0.5, style=PRAVDA (0), engine=V2 (1), protect=On (1), lookahead=Vostok (1), listen=Full (0), rise=1, decay=1, sensitivity=0.8, preserve=1, mix=1, outputDb=0
- HamloFader (amount 1): engine=Modern (1), enabled=On (1), mode=Trim (0), gainDb=-1, targetDb=-18, mix=1, polarity=Off (0), mute=Off (0), solo=Off (0), inputDb=0, outputDb=0, leftDb=0, rightDb=0, matchedDb=0, peakGuard=On (1), matchCeiling=-1, strength=0.55, maxBoost=3, maxCut=6, floorDb=-55, holdMs=100, riseMs=250, fallMs=140, depthDb=0, rightPhase=0

Reject/revise if: Offbeats become inaudible at the matched level.

Intent check: Reduce front-edge aggression and push hats behind the snare. Assess against: Offbeats become inaudible at the matched level.. Original design-stage calibration status: **not_run**; final numerical results and individual owner-audition status are in RACK_BANK_REPORT.md.

### HR018 - BB Conga Palm Body

**Percussion / 90 BPM.** Source: a sparse percussion loop with velocity variation

Emphasize hand tone and trim hard slap peaks.

EQ: LowCut 100 Hz / +0 dB / Q 0.707; Bell 230 Hz / +1.5 dB / Q 1

Full range:

- HamloUdarnik (amount 1): attackDb=-1, sustainDb=1.5, speed=0.45, style=PRAVDA (0), engine=V2 (1), protect=On (1), lookahead=Vostok (1), listen=Full (0), rise=1, decay=1, sensitivity=0.8, preserve=1, mix=1, outputDb=0
- HamloMag (amount 0.3): model=Tube (2), driveDb=4, mix=1, trimDb=0, oversampling=8x (3), autoGain=On (1), harmonics=0.35, tone=0, evenOdd=0, tapeBias=0.5, speed=1 (0), wow=0, hiss=0, hum=0, crackle=0, noiseFollow=Follow (1), punish=Off (0), thump=Off (0), steep=Off (0), sampler=Off (0), phoneBass=0, subProtectHz=0, dynamics=0, match=Off (0), delta=Off (0)

Reject/revise if: The low hand pitch disappears or slaps get louder.

Intent check: Emphasize hand tone and trim hard slap peaks. Assess against: The low hand pitch disappears or slaps get louder.. Original design-stage calibration status: **not_run**; final numerical results and individual owner-audition status are in RACK_BANK_REPORT.md.

### HR019 - BB Shaker Back Wall

**Percussion / 90 BPM.** Source: a sparse percussion loop with velocity variation

Place a sharp shaker behind the beat in a dark room.

EQ: LowCut 950 Hz / +0 dB / Q 0.707; HighShelf 10000 Hz / -2 dB / Q 0.707

Full range:

- HamloVerb (amount 0.14): space=Room (1), engine=Velvet (3), decaySeconds=0.38, predelayMs=0, predelaySync=Free (0), lowCutHz=1000, highCutHz=5500, bassMult=0.75, hfDecayRatio=0.5, width=1.1, modDepth=0.1, duck=0.15, routing=Send (1), mix=1, keepDry=Off (0), wetDb=0, outputDb=0, earlyDb=0, lateDb=0, timeShape=Off (0), shimmerAmount=0, freeze=Off (0)
- HamloShirota (amount 1): v2Mode=Stereo / Space (1), v2Width=1, v2LowWidth=0, v2MidWidth=1, v2HighWidth=1.1, v2LowSplit=300, v2HighSplit=3000, v2MonoBass=200, v2Generate=0, v2Guard=On (1), v2Floor=0.15, v2Mix=1, v2Output=0, v2Motion=Off (0), v2PositionOn=Off (0), v2Mono=Stereo (0)

Reject/revise if: The shaker sounds closer or the room sustains through every gap.

Intent check: Place a sharp shaker behind the beat in a dark room. Assess against: The shaker sounds closer or the room sustains through every gap.. Original design-stage calibration status: **not_run**; final numerical results and individual owner-audition status are in RACK_BANK_REPORT.md.

### HR020 - BB Rim Dub Answer

**Percussion / 90 BPM.** Source: a sparse percussion loop with velocity variation

Answer a dry rim with a narrow filtered quarter delay.

EQ: LowCut 180 Hz / +0 dB / Q 0.707

Full range:

- HamloEcho (amount 0.18): echoMode=Score (1), quickEnabled=Off (0), scoreRouting=Send (1), scoreMix=1, scoreOutput=0, scoreSource=60, scoreFeedback=0.28, scoreFeedbackSync=1/4 (8), scoreFeedbackMs=500, scoreFeedbackPitch=0, scoreColor=BBD (3), scoreDrive=0, scoreLowCut=600, scoreHighCut=4200, scoreWidth=1, scoreDuck=0.2, scoreFreeze=Off (0), scoreSpaceMix=0, scoreGlitch=Off (0), scoreGlitchAmount=0, scoreBands=Off (0), scoreSwing=0, tap1On=1, tap2On=0, tap3On=0, tap4On=0, tap5On=0, tap6On=0, tap7On=0, tap8On=0, tap1Sync=1/4 (8), tap1Time=250, tap1Note=60, tap1Gain=-6, tap1Pan=0, tap1Shape=Forward (0), tap1Fragment=125, tap1Repeats=3, tap1Low=600, tap1High=4200, tap1SpaceSend=0

Reject/revise if: The delay carries sub energy or competes with the next snare.

Intent check: Answer a dry rim with a narrow filtered quarter delay. Assess against: The delay carries sub energy or competes with the next snare.. Original design-stage calibration status: **not_run**; final numerical results and individual owner-audition status are in RACK_BANK_REPORT.md.

### HR021 - BB Last Step Shuffle

**Percussion / 90 BPM.** Source: a sparse percussion loop with velocity variation

Change only the final two sixteenths of a percussion bar.

EQ: LowCut 180 Hz / +0 dB / Q 0.707

Full range:

- HamloRevolution (amount 0.3): mix=1, free=Sync (0), grid=1/16 (2), triplet=Off (0), swing=0, humanize=0, fadeMs=3, order=T-G-M (0), scene=1 (0), listen=Off (0), timeOn=Off (0), glitchOn=On (1), motionOn=Off (0), timeMix=1, timeDepth=1, glitchMix=1, glitchDepth=0.5, motionMix=1, motionDepth=0.35, motionSmooth=0.5, motionTarget=Low Pass (1), timeStretch=Rhythm (1), glitchEngine=Block (1), shapeAOn=Off (0), shapeBOn=Off (0), throwAOn=Off (0), throwBOn=Off (0)
  - BLOCK GLITCH (steps 1-16): 15: Shuffle; 16: Repeat
- HamloMag (amount 0.25): model=Tape (1), driveDb=4, mix=1, trimDb=0, oversampling=8x (3), autoGain=On (1), harmonics=0.35, tone=0, evenOdd=0, tapeBias=0.5, speed=1 (0), wow=0, hiss=0, hum=0, crackle=0, noiseFollow=Follow (1), punish=Off (0), thump=Off (0), steep=Off (0), sampler=Off (0), phoneBass=0, subProtectHz=0, dynamics=0, match=Off (0), delta=Off (0)

Reject/revise if: The downbeat or main backbeat changes.

Intent check: Change only the final two sixteenths of a percussion bar. Assess against: The downbeat or main backbeat changes.. Original design-stage calibration status: **not_run**; final numerical results and individual owner-audition status are in RACK_BANK_REPORT.md.

### HR022 - BB Classic Bus Cement

**Drum Bus / 90 BPM.** Source: a stereo unprocessed drum loop with kick, snare, hats and ghost notes

Join the drum body with slow-attack VCA reduction.

EQ: LowCut 25 Hz / +0 dB / Q 0.707

Full range:

- HamloComp (amount 1): thresholdDb=-22, ratio=2, attackMs=30, releaseMs=167, model=VCA (1), source=Drums (1), kneeDb=6, detector=RMS (1), character=Clean (0), autoMakeup=Off (0), makeupDb=0, mix=1
- HamloMag (amount 0.35): model=Tape (1), driveDb=3, mix=1, trimDb=0, oversampling=8x (3), autoGain=On (1), harmonics=0.35, tone=0, evenOdd=0, tapeBias=0.5, speed=1 (0), wow=0, hiss=0, hum=0, crackle=0, noiseFollow=Follow (1), punish=Off (0), thump=Off (0), steep=Off (0), sampler=Off (0), phoneBass=0, subProtectHz=0, dynamics=0, match=Off (0), delta=Off (0)

Reject/revise if: The matched kick/snare attacks lose more contrast than the body.

Intent check: Join the drum body with slow-attack VCA reduction. Assess against: The matched kick/snare attacks lose more contrast than the body.. Original design-stage calibration status: **not_run**; final numerical results and individual owner-audition status are in RACK_BANK_REPORT.md.

### HR023 - BB Dusty Parallel Basement

**Drum Bus / 90 BPM.** Source: a stereo unprocessed drum loop with kick, snare, hats and ghost notes

Layer dense sampler crunch under a clean drum loop.

EQ: Bell 350 Hz / -1 dB / Q 1

Full range:

- HamloMag (amount 0.35): model=Tape (1), driveDb=8, mix=1, trimDb=0, oversampling=8x (3), autoGain=On (1), harmonics=0.35, tone=0, evenOdd=0, tapeBias=0.5, speed=1 (0), wow=0, hiss=0, hum=0, crackle=0, noiseFollow=Follow (1), punish=Off (0), thump=Off (0), steep=Off (0), sampler=SP-1200 (1), phoneBass=0, subProtectHz=0, dynamics=0, match=Off (0), delta=Off (0)
- HamloComp (amount 0.25): thresholdDb=-30, ratio=6, attackMs=12, releaseMs=180, model=FET (2), source=Drums (1), kneeDb=6, detector=RMS (1), character=Clean (0), autoMakeup=Off (0), makeupDb=0, mix=1

Reject/revise if: Ghost notes vanish or the parallel return overtakes the clean snare.

Intent check: Layer dense sampler crunch under a clean drum loop. Assess against: Ghost notes vanish or the parallel return overtakes the clean snare.. Original design-stage calibration status: **not_run**; final numerical results and individual owner-audition status are in RACK_BANK_REPORT.md.

### HR024 - BB Jazzy Room Glue

**Drum Bus / 90 BPM.** Source: a stereo unprocessed drum loop with kick, snare, hats and ghost notes

Give a sparse jazz break a low-density shared room.

EQ: LowCut 27 Hz / +0 dB / Q 0.707

Full range:

- HamloComp (amount 1): thresholdDb=-24, ratio=1.5, attackMs=35, releaseMs=200, model=Opto (3), source=Drums (1), kneeDb=6, detector=RMS (1), character=Clean (0), autoMakeup=Off (0), makeupDb=0, mix=1
- HamloVerb (amount 0.07): space=Chamber (2), engine=Velvet (3), decaySeconds=0.6, predelayMs=15, predelaySync=Free (0), lowCutHz=350, highCutHz=6500, bassMult=0.75, hfDecayRatio=0.5, width=1.1, modDepth=0.1, duck=0.15, routing=Send (1), mix=1, keepDry=Off (0), wetDb=0, outputDb=0, earlyDb=0, lateDb=0, timeShape=Off (0), shimmerAmount=0, freeze=Off (0)

Reject/revise if: Ride cymbals turn splashy or kick low end grows in the room.

Intent check: Give a sparse jazz break a low-density shared room. Assess against: Ride cymbals turn splashy or kick low end grows in the room.. Original design-stage calibration status: **not_run**; final numerical results and individual owner-audition status are in RACK_BANK_REPORT.md.

### HR025 - BB Gold Snare Shoulder

**Drum Bus / 90 BPM.** Source: a stereo unprocessed drum loop with kick, snare, hats and ghost notes

Add body only around the snare register and leave the sub alone.

EQ: Bell 470 Hz / -1.5 dB / Q 1

Full range:

- HamloComp (amount 1): thresholdDb=-21, ratio=2, attackMs=25, releaseMs=155, model=Tube (4), source=Drums (1), kneeDb=6, detector=RMS (1), character=Clean (0), autoMakeup=Off (0), makeupDb=0, mix=1

Band 160-2500 Hz:

- HamloUdarnik (amount 0.6): attackDb=1, sustainDb=1, speed=0.55, style=PRAVDA (0), engine=V2 (1), protect=On (1), lookahead=Vostok (1), listen=Full (0), rise=1, decay=1, sensitivity=0.8, preserve=1, mix=1, outputDb=0

Reject/revise if: Kick sub grows or the snare becomes boxier after matching.

Intent check: Add body only around the snare register and leave the sub alone. Assess against: Kick sub grows or the snare becomes boxier after matching.. Original design-stage calibration status: **not_run**; final numerical results and individual owner-audition status are in RACK_BANK_REPORT.md.

### HR026 - BB Underground Punch Return

**Drum Bus / 90 BPM.** Source: a stereo unprocessed drum loop with kick, snare, hats and ghost notes

Bring short gritty sustain under intact transients.

EQ: HighShelf 9500 Hz / -0.5 dB / Q 0.707

Full range:

- HamloComp (amount 0.22): thresholdDb=-32, ratio=8, attackMs=15, releaseMs=130, model=FET (2), source=Drums (1), kneeDb=6, detector=RMS (1), character=Clean (0), autoMakeup=Off (0), makeupDb=0, mix=1
- HamloMag (amount 0.3): model=Iron (3), driveDb=4, mix=1, trimDb=0, oversampling=8x (3), autoGain=On (1), harmonics=0.35, tone=0, evenOdd=0, tapeBias=0.5, speed=1 (0), wow=0, hiss=0, hum=0, crackle=0, noiseFollow=Follow (1), punish=Off (0), thump=Off (0), steep=Off (0), sampler=Off (0), phoneBass=0, subProtectHz=0, dynamics=0, match=Off (0), delta=Off (0)

Reject/revise if: The matched loop is merely louder or hats dominate the return.

Intent check: Bring short gritty sustain under intact transients. Assess against: The matched loop is merely louder or hats dominate the return.. Original design-stage calibration status: **not_run**; final numerical results and individual owner-audition status are in RACK_BANK_REPORT.md.

### HR027 - BB Lofi Worn Break

**Drum Bus / 90 BPM.** Source: a stereo unprocessed drum loop with kick, snare, hats and ghost notes

Darken a bright break and add restrained tape instability.

EQ: LowCut 30 Hz / +0 dB / Q 0.707; HighCut 12000 Hz / +0 dB / Q 0.707

Full range:

- HamloMag (amount 0.55): model=Tape (1), driveDb=6, mix=1, trimDb=0, oversampling=8x (3), autoGain=On (1), harmonics=0.35, tone=-0.2, evenOdd=0, tapeBias=0.5, speed=1 (0), wow=0.04, hiss=0, hum=0, crackle=0, noiseFollow=Follow (1), punish=Off (0), thump=Off (0), steep=Off (0), sampler=Off (0), phoneBass=0, subProtectHz=0, dynamics=0, match=Off (0), delta=Off (0)
- HamloUdarnik (amount 1): attackDb=-0.8, sustainDb=0.5, speed=0.45, style=PRAVDA (0), engine=V2 (1), protect=On (1), lookahead=Vostok (1), listen=Full (0), rise=1, decay=1, sensitivity=0.8, preserve=1, mix=1, outputDb=0

Reject/revise if: The kick pitch wanders audibly or swing timing becomes unclear.

Intent check: Darken a bright break and add restrained tape instability. Assess against: The kick pitch wanders audibly or swing timing becomes unclear.. Original design-stage calibration status: **not_run**; final numerical results and individual owner-audition status are in RACK_BANK_REPORT.md.

### HR028 - BB Bar End Turn

**Drum Bus / 90 BPM.** Source: a stereo unprocessed drum loop with kick, snare, hats and ghost notes

Keep the groove and make a restrained end-of-bar reply on the tops.

EQ: LowCut 25 Hz / +0 dB / Q 0.707

Full range:

- HamloComp (amount 1): thresholdDb=-23, ratio=1.8, attackMs=28, releaseMs=170, model=VCA (1), source=Drums (1), kneeDb=6, detector=RMS (1), character=Clean (0), autoMakeup=Off (0), makeupDb=0, mix=1

Band 1200-16000 Hz:

- HamloRevolution (amount 0.22): mix=1, free=Sync (0), grid=1/16 (2), triplet=Off (0), swing=0, humanize=0, fadeMs=3, order=T-G-M (0), scene=1 (0), listen=Off (0), timeOn=Off (0), glitchOn=On (1), motionOn=Off (0), timeMix=1, timeDepth=1, glitchMix=1, glitchDepth=0.5, motionMix=1, motionDepth=0.35, motionSmooth=0.5, motionTarget=Low Pass (1), timeStretch=Rhythm (1), glitchEngine=Block (1), shapeAOn=Off (0), shapeBOn=Off (0), throwAOn=Off (0), throwBOn=Off (0)
  - BLOCK GLITCH (steps 1-16): 15: Repeat; 16: Reverse

Reject/revise if: Any low kick note changes or early-bar groove is chopped.

Intent check: Keep the groove and make a restrained end-of-bar reply on the tops. Assess against: Any low kick note changes or early-bar groove is chopped.. Original design-stage calibration status: **not_run**; final numerical results and individual owner-audition status are in RACK_BANK_REPORT.md.

### HR029 - BB Upright Round Fingers

**Bass / 90 BPM.** Source: a monophonic bass phrase with low and high notes and sustained tails

Level an upright line while retaining finger articulation.

EQ: LowCut 28 Hz / +0 dB / Q 0.707; Bell 280 Hz / -1.5 dB / Q 1

Full range:

- HamloComp (amount 1): thresholdDb=-25, ratio=2.5, attackMs=28, releaseMs=220, model=Opto (3), source=All (0), kneeDb=6, detector=RMS (1), character=Clean (0), autoMakeup=Off (0), makeupDb=0, mix=1
- HamloMag (amount 0.3): model=Tube (2), driveDb=3, mix=1, trimDb=0, oversampling=8x (3), autoGain=On (1), harmonics=0.35, tone=0, evenOdd=0, tapeBias=0.5, speed=1 (0), wow=0, hiss=0, hum=0, crackle=0, noiseFollow=Follow (1), punish=Off (0), thump=Off (0), steep=Off (0), sampler=Off (0), phoneBass=0, subProtectHz=0, dynamics=0, match=Off (0), delta=Off (0)

Reject/revise if: The note tails pump or the finger noise becomes brittle.

Intent check: Level an upright line while retaining finger articulation. Assess against: The note tails pump or the finger noise becomes brittle.. Original design-stage calibration status: **not_run**; final numerical results and individual owner-audition status are in RACK_BANK_REPORT.md.

### HR030 - BB Electric Pocket Bass

**Bass / 90 BPM.** Source: a monophonic bass phrase with low and high notes and sustained tails

Tighten an electric bass into the kick pocket with modest harmonics.

EQ: LowCut 26 Hz / +0 dB / Q 0.707; Bell 800 Hz / +1 dB / Q 1

Full range:

- HamloComp (amount 1): thresholdDb=-24, ratio=3, attackMs=20, releaseMs=167, model=VCA (1), source=All (0), kneeDb=6, detector=RMS (1), character=Clean (0), autoMakeup=Off (0), makeupDb=0, mix=1
- HamloMag (amount 0.35): model=Iron (3), driveDb=5, mix=1, trimDb=0, oversampling=8x (3), autoGain=On (1), harmonics=0.35, tone=0, evenOdd=0, tapeBias=0.5, speed=1 (0), wow=0, hiss=0, hum=0, crackle=0, noiseFollow=Follow (1), punish=Off (0), thump=Off (0), steep=Off (0), sampler=Off (0), phoneBass=0, subProtectHz=75, dynamics=0, match=Off (0), delta=Off (0)

Reject/revise if: The low note sustains into the next kick or pick attack vanishes.

Intent check: Tighten an electric bass into the kick pocket with modest harmonics. Assess against: The low note sustains into the next kick or pick attack vanishes.. Original design-stage calibration status: **not_run**; final numerical results and individual owner-audition status are in RACK_BANK_REPORT.md.

### HR031 - BB Sub On Small Speakers

**Bass / 90 BPM.** Source: a monophonic bass phrase with low and high notes and sustained tails

Add upper harmonics while keeping the sub centered.

EQ: LowCut 24 Hz / +0 dB / Q 0.707

Full range:

- HamloMag (amount 0.35): model=Tube (2), driveDb=6, mix=1, trimDb=0, oversampling=8x (3), autoGain=On (1), harmonics=0.35, tone=0, evenOdd=0, tapeBias=0.5, speed=1 (0), wow=0, hiss=0, hum=0, crackle=0, noiseFollow=Follow (1), punish=Off (0), thump=Off (0), steep=Off (0), sampler=Off (0), phoneBass=0.3, subProtectHz=90, dynamics=0, match=Off (0), delta=Off (0)
- HamloShirota (amount 1): v2Mode=Stereo / Space (1), v2Width=1, v2LowWidth=0, v2MidWidth=0.9, v2HighWidth=1, v2LowSplit=300, v2HighSplit=3000, v2MonoBass=160, v2Generate=0, v2Guard=On (1), v2Floor=0.15, v2Mix=1, v2Output=0, v2Motion=Off (0), v2PositionOn=Off (0), v2Mono=Stereo (0)

Reject/revise if: The 40-80 Hz fundamental changes pitch or the mono fold cancels it.

Intent check: Add upper harmonics while keeping the sub centered. Assess against: The 40-80 Hz fundamental changes pitch or the mono fold cancels it.. Original design-stage calibration status: **not_run**; final numerical results and individual owner-audition status are in RACK_BANK_REPORT.md.

### HR032 - BB Tape Note Sustain

**Bass / 90 BPM.** Source: a monophonic bass phrase with low and high notes and sustained tails

Support short bass-note tails with gentle tube compression.

EQ: LowCut 25 Hz / +0 dB / Q 0.707; HighCut 7000 Hz / +0 dB / Q 0.707

Full range:

- HamloComp (amount 1): thresholdDb=-26, ratio=2.2, attackMs=30, releaseMs=240, model=Tube (4), source=All (0), kneeDb=6, detector=RMS (1), character=Clean (0), autoMakeup=Off (0), makeupDb=0, mix=1
- HamloMag (amount 0.4): model=Tape (1), driveDb=4, mix=1, trimDb=0, oversampling=8x (3), autoGain=On (1), harmonics=0.35, tone=0, evenOdd=0, tapeBias=0.5, speed=1 (0), wow=0, hiss=0, hum=0, crackle=0, noiseFollow=Follow (1), punish=Off (0), thump=Off (0), steep=Off (0), sampler=Off (0), phoneBass=0, subProtectHz=0, dynamics=0, match=Off (0), delta=Off (0)

Reject/revise if: Noise rises between notes or the next attack ducks for too long.

Intent check: Support short bass-note tails with gentle tube compression. Assess against: Noise rises between notes or the next attack ducks for too long.. Original design-stage calibration status: **not_run**; final numerical results and individual owner-audition status are in RACK_BANK_REPORT.md.

### HR033 - BB Muted Sample Bass

**Bass / 90 BPM.** Source: a monophonic bass phrase with low and high notes and sustained tails

Remove upper spill from a sampled bass phrase and contain its tail.

EQ: LowCut 30 Hz / +0 dB / Q 0.707; HighCut 2400 Hz / +0 dB / Q 0.707

Full range:

- HamloZastava (amount 1): v2_engine=Clean (1), v2_enabled=On (1), v2_threshold=-48, v2_range=9, v2_hold=80, v2_release=180, v2_attack=0.3, v2_hysteresis=4, v2_knee=6, v2_external=Main (0), v2_expand=Expand (1), v2_ratio=2, v2_link=1, v2_mix=1, v2_output=0, v2_body=80, v2_tail=180, v2_lookahead=2, v2_hp=30, v2_lp=350, v2_listen=Off (0)
- HamloComp (amount 1): thresholdDb=-23, ratio=2, attackMs=25, releaseMs=190, model=Opto (3), source=All (0), kneeDb=6, detector=RMS (1), character=Clean (0), autoMakeup=Off (0), makeupDb=0, mix=1

Reject/revise if: The line loses pitch definition or the expander chatters on held notes.

Intent check: Remove upper spill from a sampled bass phrase and contain its tail. Assess against: The line loses pitch definition or the expander chatters on held notes.. Original design-stage calibration status: **not_run**; final numerical results and individual owner-audition status are in RACK_BANK_REPORT.md.

### HR034 - BB Dry Front Rap

**Vocal / 90 BPM.** Source: a dry lead phrase with plosives, sibilance and a quiet ending

Keep consonants close with FET control and little ambience.

EQ: LowCut 80 Hz / +0 dB / Q 0.707; Bell 320 Hz / -1.5 dB / Q 1; Bell 6500 Hz / +0 dB / Q 1 (dynamic -3 dB)

Full range:

- HamloComp (amount 1): thresholdDb=-25, ratio=3, attackMs=8, releaseMs=115, model=FET (2), source=Voc (3), kneeDb=6, detector=RMS (1), character=Clean (0), autoMakeup=Off (0), makeupDb=0, mix=1
- HamloVerb (amount 0.045): space=Ambience (6), engine=Warm (2), decaySeconds=0.35, predelayMs=18, predelaySync=Free (0), lowCutHz=300, highCutHz=6000, bassMult=0.75, hfDecayRatio=0.5, width=1.1, modDepth=0.1, duck=0.15, routing=Send (1), mix=1, keepDry=Off (0), wetDb=0, outputDb=0, earlyDb=0, lateDb=0, timeShape=Off (0), shimmerAmount=0, freeze=Off (0)

Reject/revise if: Sibilance rises or endings are pinned down after loudness matching.

Intent check: Keep consonants close with FET control and little ambience. Assess against: Sibilance rises or endings are pinned down after loudness matching.. Original design-stage calibration status: **not_run**; final numerical results and individual owner-audition status are in RACK_BANK_REPORT.md.

### HR035 - BB Jazzy Intimate Verse

**Vocal / 90 BPM.** Source: a dry lead phrase with plosives, sibilance and a quiet ending

Ride quiet syllables gently before soft opto leveling.

EQ: LowCut 75 Hz / +0 dB / Q 0.707; HighShelf 11000 Hz / +0.7 dB / Q 0.707

Full range:

- HamloFader (amount 1): engine=Modern (1), enabled=On (1), mode=Ride (2), gainDb=0, targetDb=-21, mix=1, polarity=Off (0), mute=Off (0), solo=Off (0), inputDb=0, outputDb=0, leftDb=0, rightDb=0, matchedDb=0, peakGuard=On (1), matchCeiling=-1, strength=0.35, maxBoost=2, maxCut=4, floorDb=-55, holdMs=100, riseMs=250, fallMs=140, depthDb=0, rightPhase=0
- HamloComp (amount 1): thresholdDb=-23, ratio=2, attackMs=18, releaseMs=210, model=Opto (3), source=Voc (3), kneeDb=6, detector=RMS (1), character=Clean (0), autoMakeup=Off (0), makeupDb=0, mix=1

Reject/revise if: Breaths swell in gaps or phrase dynamics disappear.

Intent check: Ride quiet syllables gently before soft opto leveling. Assess against: Breaths swell in gaps or phrase dynamics disappear.. Original design-stage calibration status: **not_run**; final numerical results and individual owner-audition status are in RACK_BANK_REPORT.md.

### HR036 - BB Tape Slap Story

**Vocal / 90 BPM.** Source: a dry lead phrase with plosives, sibilance and a quiet ending

Add a quiet free-time slap under a warm spoken lead.

EQ: LowCut 85 Hz / +0 dB / Q 0.707; Bell 400 Hz / -1 dB / Q 1

Full range:

- HamloMag (amount 0.25): model=Tube (2), driveDb=3, mix=1, trimDb=0, oversampling=8x (3), autoGain=On (1), harmonics=0.35, tone=0, evenOdd=0, tapeBias=0.5, speed=1 (0), wow=0, hiss=0, hum=0, crackle=0, noiseFollow=Follow (1), punish=Off (0), thump=Off (0), steep=Off (0), sampler=Off (0), phoneBass=0, subProtectHz=0, dynamics=0, match=Off (0), delta=Off (0)
- HamloEcho (amount 0.15): echoMode=Score (1), quickEnabled=Off (0), scoreRouting=Send (1), scoreMix=1, scoreOutput=0, scoreSource=60, scoreFeedback=0, scoreFeedbackSync=Free (0), scoreFeedbackMs=105, scoreFeedbackPitch=0, scoreColor=Tape (2), scoreDrive=0, scoreLowCut=350, scoreHighCut=4800, scoreWidth=1, scoreDuck=0.2, scoreFreeze=Off (0), scoreSpaceMix=0, scoreGlitch=Off (0), scoreGlitchAmount=0, scoreBands=Off (0), scoreSwing=0, tap1On=1, tap2On=0, tap3On=0, tap4On=0, tap5On=0, tap6On=0, tap7On=0, tap8On=0, tap1Sync=Free (0), tap1Time=105, tap1Note=60, tap1Gain=-6, tap1Pan=0, tap1Shape=Forward (0), tap1Fragment=125, tap1Repeats=3, tap1Low=350, tap1High=4800, tap1SpaceSend=0

Reject/revise if: The slap is heard as a second speaker or obscures words.

Intent check: Add a quiet free-time slap under a warm spoken lead. Assess against: The slap is heard as a second speaker or obscures words.. Original design-stage calibration status: **not_run**; final numerical results and individual owner-audition status are in RACK_BANK_REPORT.md.

### HR037 - BB Backroom Hook

**Vocal / 90 BPM.** Source: a dry lead phrase with plosives, sibilance and a quiet ending

Place a doubled hook behind the lead with a dark chamber.

EQ: LowCut 150 Hz / +0 dB / Q 0.707; HighShelf 8000 Hz / -1 dB / Q 0.707

Full range:

- HamloComp (amount 1): thresholdDb=-26, ratio=3, attackMs=12, releaseMs=170, model=Opto (3), source=Voc (3), kneeDb=6, detector=RMS (1), character=Clean (0), autoMakeup=Off (0), makeupDb=0, mix=1
- HamloShirota (amount 1): v2Mode=Stereo / Space (1), v2Width=1, v2LowWidth=0, v2MidWidth=1.15, v2HighWidth=1.25, v2LowSplit=300, v2HighSplit=3000, v2MonoBass=180, v2Generate=0, v2Guard=On (1), v2Floor=0.15, v2Mix=1, v2Output=0, v2Motion=Off (0), v2PositionOn=Off (0), v2Mono=Stereo (0)
- HamloVerb (amount 0.15): space=Chamber (2), engine=Warm (2), decaySeconds=0.9, predelayMs=10, predelaySync=Free (0), lowCutHz=350, highCutHz=6000, bassMult=0.75, hfDecayRatio=0.5, width=1.1, modDepth=0.1, duck=0.15, routing=Send (1), mix=1, keepDry=Off (0), wetDb=0, outputDb=0, earlyDb=0, lateDb=0, timeShape=Off (0), shimmerAmount=0, freeze=Off (0)

Reject/revise if: The hook moves ahead of the lead or mono consonants blur.

Intent check: Place a doubled hook behind the lead with a dark chamber. Assess against: The hook moves ahead of the lead or mono consonants blur.. Original design-stage calibration status: **not_run**; final numerical results and individual owner-audition status are in RACK_BANK_REPORT.md.

### HR038 - BB Phrase Tail Answer

**Vocal / 90 BPM.** Source: a dry lead phrase with plosives, sibilance and a quiet ending

Let a ducked quarter echo emerge after the rap phrase.

EQ: LowCut 90 Hz / +0 dB / Q 0.707; Bell 6700 Hz / +0 dB / Q 1.1 (dynamic -2.5 dB)

Full range:

- HamloComp (amount 1): thresholdDb=-24, ratio=2.5, attackMs=10, releaseMs=135, model=FET (2), source=Voc (3), kneeDb=6, detector=RMS (1), character=Clean (0), autoMakeup=Off (0), makeupDb=0, mix=1
- HamloEcho (amount 0.18): echoMode=Score (1), quickEnabled=Off (0), scoreRouting=Send (1), scoreMix=1, scoreOutput=0, scoreSource=60, scoreFeedback=0.25, scoreFeedbackSync=1/4 (8), scoreFeedbackMs=500, scoreFeedbackPitch=0, scoreColor=Tape (2), scoreDrive=0, scoreLowCut=450, scoreHighCut=5000, scoreWidth=1, scoreDuck=0.55, scoreFreeze=Off (0), scoreSpaceMix=0, scoreGlitch=Off (0), scoreGlitchAmount=0, scoreBands=Off (0), scoreSwing=0, tap1On=1, tap2On=0, tap3On=0, tap4On=0, tap5On=0, tap6On=0, tap7On=0, tap8On=0, tap1Sync=1/4 (8), tap1Time=250, tap1Note=60, tap1Gain=-6, tap1Pan=0, tap1Shape=Forward (0), tap1Fragment=125, tap1Repeats=3, tap1Low=450, tap1High=5000, tap1SpaceSend=0

Reject/revise if: Repeats compete with active words or accumulate across several lines.

Intent check: Let a ducked quarter echo emerge after the rap phrase. Assess against: Repeats compete with active words or accumulate across several lines.. Original design-stage calibration status: **not_run**; final numerical results and individual owner-audition status are in RACK_BANK_REPORT.md.

### HR039 - BB Felt Chord Bed

**Keys / 90 BPM.** Source: a dry chord phrase with staccato and held notes

Round sharp piano hammers and give chords a small shared chamber.

EQ: LowCut 110 Hz / +0 dB / Q 0.707; Bell 330 Hz / -1.5 dB / Q 1

Full range:

- HamloUdarnik (amount 1): attackDb=-1, sustainDb=1, speed=0.4, style=PRAVDA (0), engine=V2 (1), protect=On (1), lookahead=Vostok (1), listen=Full (0), rise=1, decay=1, sensitivity=0.8, preserve=1, mix=1, outputDb=0
- HamloMag (amount 0.3): model=Tape (1), driveDb=3, mix=1, trimDb=0, oversampling=8x (3), autoGain=On (1), harmonics=0.35, tone=0, evenOdd=0, tapeBias=0.5, speed=1 (0), wow=0, hiss=0, hum=0, crackle=0, noiseFollow=Follow (1), punish=Off (0), thump=Off (0), steep=Off (0), sampler=Off (0), phoneBass=0, subProtectHz=0, dynamics=0, match=Off (0), delta=Off (0)
- HamloVerb (amount 0.1): space=Chamber (2), engine=Velvet (3), decaySeconds=0.8, predelayMs=12, predelaySync=Free (0), lowCutHz=300, highCutHz=6500, bassMult=0.75, hfDecayRatio=0.5, width=1.1, modDepth=0.1, duck=0.15, routing=Send (1), mix=1, keepDry=Off (0), wetDb=0, outputDb=0, earlyDb=0, lateDb=0, timeShape=Off (0), shimmerAmount=0, freeze=Off (0)

Reject/revise if: Hammer definition vanishes or low-mid sustain masks the bass.

Intent check: Round sharp piano hammers and give chords a small shared chamber. Assess against: Hammer definition vanishes or low-mid sustain masks the bass.. Original design-stage calibration status: **not_run**; final numerical results and individual owner-audition status are in RACK_BANK_REPORT.md.

### HR040 - BB Rhodes Tube Cushion

**Keys / 90 BPM.** Source: a dry chord phrase with staccato and held notes

Add soft even harmonics under a dry electric piano.

EQ: LowCut 90 Hz / +0 dB / Q 0.707; HighShelf 7000 Hz / -1 dB / Q 0.707

Full range:

- HamloMag (amount 0.45): model=Tube (2), driveDb=6, mix=1, trimDb=0, oversampling=8x (3), autoGain=On (1), harmonics=0.35, tone=0, evenOdd=0.35, tapeBias=0.5, speed=1 (0), wow=0, hiss=0, hum=0, crackle=0, noiseFollow=Follow (1), punish=Off (0), thump=Off (0), steep=Off (0), sampler=Off (0), phoneBass=0, subProtectHz=0, dynamics=0, match=Off (0), delta=Off (0)
- HamloComp (amount 1): thresholdDb=-25, ratio=2, attackMs=25, releaseMs=240, model=Opto (3), source=Samples (2), kneeDb=6, detector=RMS (1), character=Clean (0), autoMakeup=Off (0), makeupDb=0, mix=1

Reject/revise if: Chord peaks fuzz or tremolo depth changes unintentionally.

Intent check: Add soft even harmonics under a dry electric piano. Assess against: Chord peaks fuzz or tremolo depth changes unintentionally.. Original design-stage calibration status: **not_run**; final numerical results and individual owner-audition status are in RACK_BANK_REPORT.md.

### HR041 - BB Sampler Piano Slice

**Keys / 90 BPM.** Source: a dry chord phrase with staccato and held notes

Give chopped piano a restrained sampled texture without extending gaps.

EQ: LowCut 120 Hz / +0 dB / Q 0.707; HighCut 10500 Hz / +0 dB / Q 0.707

Full range:

- HamloMag (amount 0.4): model=Tape (1), driveDb=5, mix=1, trimDb=0, oversampling=8x (3), autoGain=On (1), harmonics=0.35, tone=0, evenOdd=0, tapeBias=0.5, speed=1 (0), wow=0, hiss=0, hum=0, crackle=0, noiseFollow=Follow (1), punish=Off (0), thump=Off (0), steep=Off (0), sampler=MPC60 (2), phoneBass=0, subProtectHz=0, dynamics=0, match=Off (0), delta=Off (0)
- HamloUdarnik (amount 1): attackDb=1, sustainDb=-1, speed=0.55, style=PRAVDA (0), engine=V2 (1), protect=On (1), lookahead=Vostok (1), listen=Full (0), rise=1, decay=1, sensitivity=0.8, preserve=1, mix=1, outputDb=0

Reject/revise if: The slice edge clicks or the silent spaces fill with residue.

Intent check: Give chopped piano a restrained sampled texture without extending gaps. Assess against: The slice edge clicks or the silent spaces fill with residue.. Original design-stage calibration status: **not_run**; final numerical results and individual owner-audition status are in RACK_BANK_REPORT.md.

### HR042 - BB Wide Chord Reply

**Keys / 90 BPM.** Source: a dry chord phrase with staccato and held notes

Send a filtered eighth answer wide while the original chords remain centered.

EQ: LowCut 140 Hz / +0 dB / Q 0.707

Full range:

- HamloEcho (amount 0.18): echoMode=Score (1), quickEnabled=Off (0), scoreRouting=Send (1), scoreMix=1, scoreOutput=0, scoreSource=60, scoreFeedback=0.18, scoreFeedbackSync=1/8 (5), scoreFeedbackMs=500, scoreFeedbackPitch=0, scoreColor=BBD (3), scoreDrive=0, scoreLowCut=180, scoreHighCut=8000, scoreWidth=1, scoreDuck=0.2, scoreFreeze=Off (0), scoreSpaceMix=0, scoreGlitch=Off (0), scoreGlitchAmount=0, scoreBands=Off (0), scoreSwing=0, tap1On=1, tap2On=1, tap3On=0, tap4On=0, tap5On=0, tap6On=0, tap7On=0, tap8On=0, tap1Sync=1/8 (5), tap1Time=250, tap1Note=60, tap1Gain=-7, tap1Pan=-0.6, tap1Shape=Forward (0), tap1Fragment=125, tap1Repeats=3, tap1Low=180, tap1High=8000, tap1SpaceSend=0, tap2Sync=1/4 (8), tap2Time=500, tap2Note=60, tap2Gain=-12, tap2Pan=0.6, tap2Shape=Forward (0), tap2Fragment=125, tap2Repeats=3, tap2Low=180, tap2High=8000, tap2SpaceSend=0

Reject/revise if: The dry piano widens too far or the mono chord loses its root.

Intent check: Send a filtered eighth answer wide while the original chords remain centered. Assess against: The dry piano widens too far or the mono chord loses its root.. Original design-stage calibration status: **not_run**; final numerical results and individual owner-audition status are in RACK_BANK_REPORT.md.

### HR043 - BB Dusty Soul Frame

**Samples / 90 BPM.** Source: a musical sample loop with gaps and a clear phrase ending

Darken a bright soul loop and retain the chopped envelope.

EQ: LowCut 65 Hz / +0 dB / Q 0.707; HighCut 12500 Hz / +0 dB / Q 0.707

Full range:

- HamloMag (amount 0.55): model=Tape (1), driveDb=5, mix=1, trimDb=0, oversampling=8x (3), autoGain=On (1), harmonics=0.35, tone=-0.12, evenOdd=0, tapeBias=0.5, speed=1 (0), wow=0, hiss=0, hum=0, crackle=0, noiseFollow=Follow (1), punish=Off (0), thump=Off (0), steep=Off (0), sampler=SP-1200 (1), phoneBass=0, subProtectHz=0, dynamics=0, match=Off (0), delta=Off (0)
- HamloComp (amount 1): thresholdDb=-24, ratio=1.8, attackMs=30, releaseMs=190, model=Opto (3), source=Samples (2), kneeDb=6, detector=RMS (1), character=Clean (0), autoMakeup=Off (0), makeupDb=0, mix=1

Reject/revise if: The bass becomes unstable or crackle fills the sample gaps.

Intent check: Darken a bright soul loop and retain the chopped envelope. Assess against: The bass becomes unstable or crackle fills the sample gaps.. Original design-stage calibration status: **not_run**; final numerical results and individual owner-audition status are in RACK_BANK_REPORT.md.

### HR044 - BB Jazzy Horn Seat

**Samples / 90 BPM.** Source: a musical sample loop with gaps and a clear phrase ending

Tame a nasal horn loop and seat it in a short room.

EQ: LowCut 150 Hz / +0 dB / Q 0.707; Bell 1600 Hz / -2 dB / Q 1.4

Full range:

- HamloComp (amount 1): thresholdDb=-22, ratio=2, attackMs=18, releaseMs=165, model=Opto (3), source=Samples (2), kneeDb=6, detector=RMS (1), character=Clean (0), autoMakeup=Off (0), makeupDb=0, mix=1
- HamloVerb (amount 0.1): space=Room (1), engine=Warm (2), decaySeconds=0.5, predelayMs=8, predelaySync=Free (0), lowCutHz=400, highCutHz=6500, bassMult=0.75, hfDecayRatio=0.5, width=1.1, modDepth=0.1, duck=0.15, routing=Send (1), mix=1, keepDry=Off (0), wetDb=0, outputDb=0, earlyDb=0, lateDb=0, timeShape=Off (0), shimmerAmount=0, freeze=Off (0)

Reject/revise if: The brass loses its entrance or the room enlarges the low end.

Intent check: Tame a nasal horn loop and seat it in a short room. Assess against: The brass loses its entrance or the room enlarges the low end.. Original design-stage calibration status: **not_run**; final numerical results and individual owner-audition status are in RACK_BANK_REPORT.md.

### HR045 - BB Last Word Reverse

**Samples / 90 BPM.** Source: a musical sample loop with gaps and a clear phrase ending

Reverse only the final eighth of the sample phrase.

EQ: LowCut 100 Hz / +0 dB / Q 0.707

Full range:

- HamloRevolution (amount 0.45): mix=1, free=Sync (0), grid=1/16 (2), triplet=Off (0), swing=0, humanize=0, fadeMs=3, order=T-G-M (0), scene=1 (0), listen=Off (0), timeOn=On (1), glitchOn=Off (0), motionOn=Off (0), timeMix=1, timeDepth=1, glitchMix=1, glitchDepth=0.5, motionMix=1, motionDepth=0.35, motionSmooth=0.5, motionTarget=Low Pass (1), timeStretch=Rhythm (1), glitchEngine=Block (1), shapeAOn=Off (0), shapeBOn=Off (0), throwAOn=Off (0), throwBOn=Off (0)
  - TIME (steps 1-16): 15: Reverse; 16: Reverse
- HamloMag (amount 0.25): model=Tape (1), driveDb=3, mix=1, trimDb=0, oversampling=8x (3), autoGain=On (1), harmonics=0.35, tone=0, evenOdd=0, tapeBias=0.5, speed=1 (0), wow=0, hiss=0, hum=0, crackle=0, noiseFollow=Follow (1), punish=Off (0), thump=Off (0), steep=Off (0), sampler=Off (0), phoneBass=0, subProtectHz=0, dynamics=0, match=Off (0), delta=Off (0)

Reject/revise if: The start of the next bar is reversed or cut off.

Intent check: Reverse only the final eighth of the sample phrase. Assess against: The start of the next bar is reversed or cut off.. Original design-stage calibration status: **not_run**; final numerical results and individual owner-audition status are in RACK_BANK_REPORT.md.

### HR046 - BB Filtered Pan Conversation

**Samples / 90 BPM.** Source: a musical sample loop with gaps and a clear phrase ending

Move the mids across the phrase while leaving its low anchor steady.

EQ: LowCut 70 Hz / +0 dB / Q 0.707

Full range:

- HamloMag (amount 0.3): model=Tape (1), driveDb=4, mix=1, trimDb=0, oversampling=8x (3), autoGain=On (1), harmonics=0.35, tone=0, evenOdd=0, tapeBias=0.5, speed=1 (0), wow=0, hiss=0, hum=0, crackle=0, noiseFollow=Follow (1), punish=Off (0), thump=Off (0), steep=Off (0), sampler=Off (0), phoneBass=0, subProtectHz=0, dynamics=0, match=Off (0), delta=Off (0)

Band 450-9000 Hz:

- HamloRevolution (amount 0.35): mix=1, free=Sync (0), grid=1/16 (2), triplet=Off (0), swing=0, humanize=0, fadeMs=3, order=T-G-M (0), scene=1 (0), listen=Off (0), timeOn=Off (0), glitchOn=Off (0), motionOn=Off (0), timeMix=1, timeDepth=1, glitchMix=1, glitchDepth=0.5, motionMix=1, motionDepth=0.35, motionSmooth=0.5, motionTarget=Low Pass (1), timeStretch=Rhythm (1), glitchEngine=Block (1), shapeAOn=On (1), shapeBOn=Off (0), throwAOn=Off (0), throwBOn=Off (0), shapeATarget=Pan (4), shapeADepth=0.4, shapeASmooth=0.6
  - SHAPE A (steps 1-16): 3: 3; 4: 5; 5: 8; 6: 11; 7: 13; 8: 15; 9: 15; 10: 13; 11: 11; 12: 8; 13: 5; 14: 3

Reject/revise if: The stereo motion steals the low fundamental or folds down badly.

Intent check: Move the mids across the phrase while leaving its low anchor steady. Assess against: The stereo motion steals the low fundamental or folds down badly.. Original design-stage calibration status: **not_run**; final numerical results and individual owner-audition status are in RACK_BANK_REPORT.md.

### HR047 - BB Classic Open Master

**Master / 90 BPM.** Source: an unmastered stereo mix with headroom and a busy chorus

Finish a warm boom-bap premaster with minimal reduction and intact snare.

EQ: LowCut 23 Hz / +0 dB / Q 0.707; Bell 320 Hz / -0.6 dB / Q 1

Full range:

- HamloMag (amount 0.2): model=Tape (1), driveDb=2, mix=1, trimDb=0, oversampling=8x (3), autoGain=On (1), harmonics=0.35, tone=0, evenOdd=0, tapeBias=0.5, speed=1 (0), wow=0, hiss=0, hum=0, crackle=0, noiseFollow=Follow (1), punish=Off (0), thump=Off (0), steep=Off (0), sampler=Off (0), phoneBass=0, subProtectHz=0, dynamics=0, match=Off (0), delta=Off (0)
- HamloComp (amount 1): thresholdDb=-19, ratio=1.5, attackMs=40, releaseMs=180, model=VCA (1), source=All (0), kneeDb=6, detector=RMS (1), character=Clean (0), autoMakeup=Off (0), makeupDb=0, mix=1
- HamloRedWall (amount 1): engine=V2 (1), thresholdDb=-2, ceilingDb=-1, releaseMs=130, autoRelease=Manual (0), lookaheadMs=2, link=1, truePeak=On (1), style=GLASNOST (0), outputDb=0, drive=0, tube=0, monitor=Normal (0), mix=1

Reject/revise if: Final 8x true peak exceeds -1 dBTP or limiter work exceeds 3 dB.

Intent check: Finish a warm boom-bap premaster with minimal reduction and intact snare. Assess against: Final 8x true peak exceeds -1 dBTP or limiter work exceeds 3 dB.. Original design-stage calibration status: **not_run**; final numerical results and individual owner-audition status are in RACK_BANK_REPORT.md.

### HR048 - BB Dusty Album Cohesion

**Master / 90 BPM.** Source: an unmastered stereo mix with headroom and a busy chorus

Join a dusty mix gently without raising noise or narrowing the sample bed.

EQ: LowCut 25 Hz / +0 dB / Q 0.707; HighShelf 11000 Hz / -0.4 dB / Q 0.707

Full range:

- HamloComp (amount 1): thresholdDb=-21, ratio=1.6, attackMs=35, releaseMs=200, model=Opto (3), source=All (0), kneeDb=6, detector=RMS (1), character=Clean (0), autoMakeup=Off (0), makeupDb=0, mix=1
- HamloMag (amount 0.15): model=Iron (3), driveDb=2, mix=1, trimDb=0, oversampling=8x (3), autoGain=On (1), harmonics=0.35, tone=0, evenOdd=0, tapeBias=0.5, speed=1 (0), wow=0, hiss=0, hum=0, crackle=0, noiseFollow=Follow (1), punish=Off (0), thump=Off (0), steep=Off (0), sampler=Off (0), phoneBass=0, subProtectHz=0, dynamics=0, match=Off (0), delta=Off (0)
- HamloRedWall (amount 1): engine=V2 (1), thresholdDb=-2.5, ceilingDb=-1, releaseMs=145, autoRelease=Manual (0), lookaheadMs=2, link=1, truePeak=On (1), style=GLASNOST (0), outputDb=0, drive=0, tube=0, monitor=Normal (0), mix=1

Reject/revise if: Idle noise grows or the matched chorus loses snare contrast.

Intent check: Join a dusty mix gently without raising noise or narrowing the sample bed. Assess against: Idle noise grows or the matched chorus loses snare contrast.. Original design-stage calibration status: **not_run**; final numerical results and individual owner-audition status are in RACK_BANK_REPORT.md.

### HR049 - BB Jazzy Headroom Master

**Master / 90 BPM.** Source: an unmastered stereo mix with headroom and a busy chorus

Preserve spacious live dynamics with a conservative final ceiling.

EQ: LowCut 22 Hz / +0 dB / Q 0.707

Full range:

- HamloComp (amount 1): thresholdDb=-18, ratio=1.3, attackMs=45, releaseMs=230, model=Opto (3), source=All (0), kneeDb=6, detector=RMS (1), character=Clean (0), autoMakeup=Off (0), makeupDb=0, mix=1
- HamloShirota (amount 1): v2Mode=Stereo / Space (1), v2Width=1, v2LowWidth=0.95, v2MidWidth=1, v2HighWidth=1.05, v2LowSplit=300, v2HighSplit=3000, v2MonoBass=100, v2Generate=0, v2Guard=On (1), v2Floor=0.15, v2Mix=1, v2Output=0, v2Motion=Off (0), v2PositionOn=Off (0), v2Mono=Stereo (0)
- HamloRedWall (amount 1): engine=V2 (1), thresholdDb=-1, ceilingDb=-1, releaseMs=180, autoRelease=Manual (0), lookaheadMs=2, link=1, truePeak=On (1), style=GLASNOST (0), outputDb=0, drive=0, tube=0, monitor=Normal (0), mix=1

Reject/revise if: Limiter work exceeds 2 dB or the room pulls inward on kick hits.

Intent check: Preserve spacious live dynamics with a conservative final ceiling. Assess against: Limiter work exceeds 2 dB or the room pulls inward on kick hits.. Original design-stage calibration status: **not_run**; final numerical results and individual owner-audition status are in RACK_BANK_REPORT.md.

### HR050 - BB Gold Low End Frame

**Master / 90 BPM.** Source: an unmastered stereo mix with headroom and a busy chorus

Center only the low end and trim a crowded low-mid band before limiting.

EQ: LowCut 24 Hz / +0 dB / Q 0.707; Bell 260 Hz / -0.8 dB / Q 0.8

Full range:

- HamloShirota (amount 1): v2Mode=Stereo / Space (1), v2Width=1, v2LowWidth=0, v2MidWidth=1, v2HighWidth=1.08, v2LowSplit=300, v2HighSplit=3000, v2MonoBass=110, v2Generate=0, v2Guard=On (1), v2Floor=0.15, v2Mix=1, v2Output=0, v2Motion=Off (0), v2PositionOn=Off (0), v2Mono=Stereo (0)
- HamloComp (amount 1): thresholdDb=-20, ratio=1.5, attackMs=35, releaseMs=167, model=VCA (1), source=All (0), kneeDb=6, detector=RMS (1), character=Clean (0), autoMakeup=Off (0), makeupDb=0, mix=1
- HamloRedWall (amount 1): engine=V2 (1), thresholdDb=-2.2, ceilingDb=-1, releaseMs=120, autoRelease=Manual (0), lookaheadMs=2, link=1, truePeak=On (1), style=GLASNOST (0), outputDb=0, drive=0, tube=0, monitor=Normal (0), mix=1

Reject/revise if: Bass fold-down changes notes or the kick loses low-end authority.

Intent check: Center only the low end and trim a crowded low-mid band before limiting. Assess against: Bass fold-down changes notes or the kick loses low-end authority.. Original design-stage calibration status: **not_run**; final numerical results and individual owner-audition status are in RACK_BANK_REPORT.md.

## Trap

### HR051 - Trap ATL Click Over Eight

**Kick / 140 BPM.** Source: isolated kick hits including soft and hard strikes

Create a compact upper click that separates the kick from an 808.

EQ: LowCut 35 Hz / +0 dB / Q 0.707; Bell 380 Hz / -2 dB / Q 1; Bell 3200 Hz / +1.5 dB / Q 1

Full range:

- HamloUdarnik (amount 1): attackDb=2, sustainDb=-1.5, speed=0.75, style=PRAVDA (0), engine=V2 (1), protect=On (1), lookahead=Vostok (1), listen=Full (0), rise=1, decay=1, sensitivity=0.8, preserve=1, mix=1, outputDb=0
- HamloComp (amount 1): thresholdDb=-21, ratio=2.5, attackMs=14, releaseMs=85, model=VCA (1), source=Drums (1), kneeDb=6, detector=RMS (1), character=Clean (0), autoMakeup=Off (0), makeupDb=0, mix=1

Reject/revise if: The boost adds sub masking or click becomes a sharp spit.

Intent check: Create a compact upper click that separates the kick from an 808. Assess against: The boost adds sub masking or click becomes a sharp spit.. Original design-stage calibration status: **not_run**; final numerical results and individual owner-audition status are in RACK_BANK_REPORT.md.

### HR052 - Trap Dark Low Hammer

**Kick / 140 BPM.** Source: isolated kick hits including soft and hard strikes

Keep a dark low kick weighty and shorten its overlap with the bass.

EQ: LowCut 26 Hz / +0 dB / Q 0.707; HighShelf 6500 Hz / -1.5 dB / Q 0.707

Full range:

- HamloZastava (amount 1): v2_engine=Drum (2), v2_enabled=On (1), v2_threshold=-40, v2_range=14, v2_hold=35, v2_release=55, v2_attack=0.3, v2_hysteresis=4, v2_knee=6, v2_external=Main (0), v2_expand=Expand (1), v2_ratio=2, v2_link=1, v2_mix=1, v2_output=0, v2_body=35, v2_tail=55, v2_lookahead=2, v2_hp=0, v2_lp=0, v2_listen=Off (0)
- HamloMag (amount 0.35): model=Iron (3), driveDb=6, mix=1, trimDb=0, oversampling=8x (3), autoGain=On (1), harmonics=0.35, tone=0, evenOdd=0, tapeBias=0.5, speed=1 (0), wow=0, hiss=0, hum=0, crackle=0, noiseFollow=Follow (1), punish=Off (0), thump=Off (0), steep=Off (0), sampler=Off (0), phoneBass=0, subProtectHz=55, dynamics=0, match=Off (0), delta=Off (0)

Reject/revise if: The low note rings beyond a sixteenth or loses its initial weight.

Intent check: Keep a dark low kick weighty and shorten its overlap with the bass. Assess against: The low note rings beyond a sixteenth or loses its initial weight.. Original design-stage calibration status: **not_run**; final numerical results and individual owner-audition status are in RACK_BANK_REPORT.md.

### HR053 - Trap Cloud Soft Knock

**Kick / 140 BPM.** Source: isolated kick hits including soft and hard strikes

Make a hard kick gentler for a floating melodic beat.

EQ: LowCut 30 Hz / +0 dB / Q 0.707; Bell 2800 Hz / -1.5 dB / Q 1

Full range:

- HamloUdarnik (amount 1): attackDb=-1.5, sustainDb=-0.5, speed=0.55, style=PRAVDA (0), engine=V2 (1), protect=On (1), lookahead=Vostok (1), listen=Full (0), rise=1, decay=1, sensitivity=0.8, preserve=1, mix=1, outputDb=0
- HamloMag (amount 0.25): model=Tube (2), driveDb=3, mix=1, trimDb=0, oversampling=8x (3), autoGain=On (1), harmonics=0.35, tone=0, evenOdd=0, tapeBias=0.5, speed=1 (0), wow=0, hiss=0, hum=0, crackle=0, noiseFollow=Follow (1), punish=Off (0), thump=Off (0), steep=Off (0), sampler=Off (0), phoneBass=0, subProtectHz=0, dynamics=0, match=Off (0), delta=Off (0)

Reject/revise if: The kick disappears under the pad or gains extra low-tail sustain.

Intent check: Make a hard kick gentler for a floating melodic beat. Assess against: The kick disappears under the pad or gains extra low-tail sustain.. Original design-stage calibration status: **not_run**; final numerical results and individual owner-audition status are in RACK_BANK_REPORT.md.

### HR054 - Trap Rage Mid Bite

**Kick / 140 BPM.** Source: isolated kick hits including soft and hard strikes

Add controlled midrange edge while holding the kick sub clean.

EQ: LowCut 28 Hz / +0 dB / Q 0.707; Bell 450 Hz / -2 dB / Q 1

Full range:

- HamloMag (amount 0.25): model=Germanium (4), driveDb=8, mix=1, trimDb=0, oversampling=8x (3), autoGain=On (1), harmonics=0.35, tone=0, evenOdd=0, tapeBias=0.5, speed=1 (0), wow=0, hiss=0, hum=0, crackle=0, noiseFollow=Follow (1), punish=Off (0), thump=Off (0), steep=Off (0), sampler=Off (0), phoneBass=0, subProtectHz=90, dynamics=0, match=Off (0), delta=Off (0)
- HamloUdarnik (amount 1): attackDb=1.5, sustainDb=-1, speed=0.8, style=PRAVDA (0), engine=V2 (1), protect=On (1), lookahead=Vostok (1), listen=Full (0), rise=1, decay=1, sensitivity=0.8, preserve=1, mix=1, outputDb=0

Reject/revise if: The dry sub distorts or the added edge resembles broadband noise.

Intent check: Add controlled midrange edge while holding the kick sub clean. Assess against: The dry sub distorts or the added edge resembles broadband noise.. Original design-stage calibration status: **not_run**; final numerical results and individual owner-audition status are in RACK_BANK_REPORT.md.

### HR055 - Trap Memphis Short Crunch

**Kick / 140 BPM.** Source: isolated kick hits including soft and hard strikes

Make a short kick feel sampled and compressed without a long release.

EQ: LowCut 32 Hz / +0 dB / Q 0.707; HighCut 11000 Hz / +0 dB / Q 0.707

Full range:

- HamloMag (amount 0.35): model=Tape (1), driveDb=5, mix=1, trimDb=0, oversampling=8x (3), autoGain=On (1), harmonics=0.35, tone=0, evenOdd=0, tapeBias=0.5, speed=1 (0), wow=0, hiss=0, hum=0, crackle=0, noiseFollow=Follow (1), punish=Off (0), thump=Off (0), steep=Off (0), sampler=S950 (3), phoneBass=0, subProtectHz=0, dynamics=0, match=Off (0), delta=Off (0)
- HamloComp (amount 0.45): thresholdDb=-25, ratio=4, attackMs=7, releaseMs=65, model=FET (2), source=Drums (1), kneeDb=6, detector=RMS (1), character=Clean (0), autoMakeup=Off (0), makeupDb=0, mix=1

Reject/revise if: The next hat ducks or the sampled edge dominates the attack.

Intent check: Make a short kick feel sampled and compressed without a long release. Assess against: The next hat ducks or the sampled edge dominates the attack.. Original design-stage calibration status: **not_run**; final numerical results and individual owner-audition status are in RACK_BANK_REPORT.md.

### HR056 - Trap Luxury Clean Heel

**Kick / 140 BPM.** Source: isolated kick hits including soft and hard strikes

Tidy the kick low mids with a clean high-crest transient.

EQ: LowCut 27 Hz / +0 dB / Q 0.707; Bell 240 Hz / -1.8 dB / Q 1; LowShelf 65 Hz / +0.7 dB / Q 0.707

Full range:

- HamloUdarnik (amount 1): attackDb=1, sustainDb=-1, speed=0.65, style=PRAVDA (0), engine=V2 (1), protect=On (1), lookahead=Vostok (1), listen=Full (0), rise=1, decay=1, sensitivity=0.8, preserve=1, mix=1, outputDb=0
- HamloComp (amount 1): thresholdDb=-20, ratio=1.8, attackMs=22, releaseMs=105, model=VCA (1), source=Drums (1), kneeDb=6, detector=RMS (1), character=Clean (0), autoMakeup=Off (0), makeupDb=0, mix=1

Reject/revise if: The first 10 ms lose contrast or the low shelf overfills the 808.

Intent check: Tidy the kick low mids with a clean high-crest transient. Assess against: The first 10 ms lose contrast or the low shelf overfills the 808.. Original design-stage calibration status: **not_run**; final numerical results and individual owner-audition status are in RACK_BANK_REPORT.md.

### HR057 - Trap ATL Clap Stack

**Snare / 140 BPM.** Source: snare or clap hits including ghost notes and a flam

Bring clap onset forward while reducing long upper tails.

EQ: LowCut 180 Hz / +0 dB / Q 0.707; Bell 1000 Hz / -1 dB / Q 1

Full range:

- HamloUdarnik (amount 1): attackDb=2, sustainDb=-2, speed=0.8, style=PRAVDA (0), engine=V2 (1), protect=On (1), lookahead=Vostok (1), listen=Full (0), rise=1, decay=1, sensitivity=0.8, preserve=1, mix=1, outputDb=0
- HamloMag (amount 0.25): model=Iron (3), driveDb=4, mix=1, trimDb=0, oversampling=8x (3), autoGain=On (1), harmonics=0.35, tone=0, evenOdd=0, tapeBias=0.5, speed=1 (0), wow=0, hiss=0, hum=0, crackle=0, noiseFollow=Follow (1), punish=Off (0), thump=Off (0), steep=Off (0), sampler=Off (0), phoneBass=0, subProtectHz=0, dynamics=0, match=Off (0), delta=Off (0)

Reject/revise if: The stack turns into a click or flam spacing becomes unclear.

Intent check: Bring clap onset forward while reducing long upper tails. Assess against: The stack turns into a click or flam spacing becomes unclear.. Original design-stage calibration status: **not_run**; final numerical results and individual owner-audition status are in RACK_BANK_REPORT.md.

### HR058 - Trap Dark Rim Needle

**Snare / 140 BPM.** Source: snare or clap hits including ghost notes and a flam

Focus a narrow rim without exaggerating its nasal ring.

EQ: LowCut 200 Hz / +0 dB / Q 0.707; Bell 1250 Hz / -2.5 dB / Q 1.8

Full range:

- HamloComp (amount 1): thresholdDb=-23, ratio=2.5, attackMs=5, releaseMs=70, model=FET (2), source=Drums (1), kneeDb=6, detector=RMS (1), character=Clean (0), autoMakeup=Off (0), makeupDb=0, mix=1
- HamloVerb (amount 0.055): space=Ambience (6), engine=Crisp (4), decaySeconds=0.28, predelayMs=8, predelaySync=Free (0), lowCutHz=600, highCutHz=5500, bassMult=0.75, hfDecayRatio=0.5, width=1.1, modDepth=0.1, duck=0.15, routing=Send (1), mix=1, keepDry=Off (0), wetDb=0, outputDb=0, earlyDb=0, lateDb=0, timeShape=Off (0), shimmerAmount=0, freeze=Off (0)

Reject/revise if: The 900-1500 Hz ring persists or rim attack becomes harsh.

Intent check: Focus a narrow rim without exaggerating its nasal ring. Assess against: The 900-1500 Hz ring persists or rim attack becomes harsh.. Original design-stage calibration status: **not_run**; final numerical results and individual owner-audition status are in RACK_BANK_REPORT.md.

### HR059 - Trap Cloud Air Clap

**Snare / 140 BPM.** Source: snare or clap hits including ghost notes and a flam

Spread the airy clap tail around a dry center.

EQ: LowCut 220 Hz / +0 dB / Q 0.707; Bell 7800 Hz / +0 dB / Q 1 (dynamic -2 dB)

Full range:

- HamloVerb (amount 0.12): space=Plate (4), engine=Velvet (3), decaySeconds=0.65, predelayMs=18, predelaySync=Free (0), lowCutHz=600, highCutHz=10000, bassMult=0.75, hfDecayRatio=0.5, width=1.1, modDepth=0.1, duck=0.15, routing=Send (1), mix=1, keepDry=Off (0), wetDb=0, outputDb=0, earlyDb=0, lateDb=0, timeShape=Off (0), shimmerAmount=0, freeze=Off (0)
- HamloShirota (amount 1): v2Mode=Stereo / Space (1), v2Width=1, v2LowWidth=0, v2MidWidth=1.1, v2HighWidth=1.35, v2LowSplit=300, v2HighSplit=3000, v2MonoBass=250, v2Generate=0, v2Guard=On (1), v2Floor=0.15, v2Mix=1, v2Output=0, v2Motion=Off (0), v2PositionOn=Off (0), v2Mono=Stereo (0)

Reject/revise if: The mono clap loses its center or air hangs into the next snare.

Intent check: Spread the airy clap tail around a dry center. Assess against: The mono clap loses its center or air hangs into the next snare.. Original design-stage calibration status: **not_run**; final numerical results and individual owner-audition status are in RACK_BANK_REPORT.md.

### HR060 - Trap Rage Distorted Backbeat

**Snare / 140 BPM.** Source: snare or clap hits including ghost notes and a flam

Blend a rough upper snare layer while keeping body clean.

EQ: LowCut 120 Hz / +0 dB / Q 0.707

Full range:

- HamloUdarnik (amount 1): attackDb=1.5, sustainDb=-0.5, speed=0.8, style=PRAVDA (0), engine=V2 (1), protect=On (1), lookahead=Vostok (1), listen=Full (0), rise=1, decay=1, sensitivity=0.8, preserve=1, mix=1, outputDb=0

Band 900-11000 Hz:

- HamloMag (amount 0.25): model=Germanium (4), driveDb=12, mix=1, trimDb=0, oversampling=8x (3), autoGain=On (1), harmonics=0.35, tone=0, evenOdd=0, tapeBias=0.5, speed=1 (0), wow=0, hiss=0, hum=0, crackle=0, noiseFollow=Follow (1), punish=Off (0), thump=Off (0), steep=Off (0), sampler=Off (0), phoneBass=0, subProtectHz=0, dynamics=0, match=Off (0), delta=Off (0)
- HamloComp (amount 0.4): thresholdDb=-29, ratio=6, attackMs=4, releaseMs=60, model=FET (2), source=Drums (1), kneeDb=6, detector=RMS (1), character=Clean (0), autoMakeup=Off (0), makeupDb=0, mix=1

Reject/revise if: Low body flattens or distortion raises the matched peak excessively.

Intent check: Blend a rough upper snare layer while keeping body clean. Assess against: Low body flattens or distortion raises the matched peak excessively.. Original design-stage calibration status: **not_run**; final numerical results and individual owner-audition status are in RACK_BANK_REPORT.md.

### HR061 - Trap Memphis Gated Plate

**Snare / 140 BPM.** Source: snare or clap hits including ghost notes and a flam

End a synthetic plate sharply after the clap.

EQ: LowCut 160 Hz / +0 dB / Q 0.707

Full range:

- HamloMag (amount 0.25): model=Tape (1), driveDb=4, mix=1, trimDb=0, oversampling=8x (3), autoGain=On (1), harmonics=0.35, tone=0, evenOdd=0, tapeBias=0.5, speed=1 (0), wow=0, hiss=0, hum=0, crackle=0, noiseFollow=Follow (1), punish=Off (0), thump=Off (0), steep=Off (0), sampler=Off (0), phoneBass=0, subProtectHz=0, dynamics=0, match=Off (0), delta=Off (0)
- HamloVerb (amount 0.16): space=Plate (4), engine=Warm (2), decaySeconds=0.8, predelayMs=10, predelaySync=Free (0), lowCutHz=500, highCutHz=6500, bassMult=0.75, hfDecayRatio=0.5, width=1.1, modDepth=0.1, duck=0.15, routing=Send (1), mix=1, keepDry=Off (0), wetDb=0, outputDb=0, earlyDb=0, lateDb=0, timeShape=Gated (1), shimmerAmount=0, freeze=Off (0), shapeTimeMs=150, gateReleaseMs=25

Reject/revise if: Plate opening swallows the dry clap or the gate snaps audibly.

Intent check: End a synthetic plate sharply after the clap. Assess against: Plate opening swallows the dry clap or the gate snaps audibly.. Original design-stage calibration status: **not_run**; final numerical results and individual owner-audition status are in RACK_BANK_REPORT.md.

### HR062 - Trap Luxury Snap Body

**Snare / 140 BPM.** Source: snare or clap hits including ghost notes and a flam

Add a little body below a bright clean snap.

EQ: LowCut 130 Hz / +0 dB / Q 0.707; Bell 220 Hz / +1.2 dB / Q 1; Bell 4300 Hz / -1 dB / Q 1

Full range:

- HamloUdarnik (amount 1): attackDb=1, sustainDb=1, speed=0.65, style=PRAVDA (0), engine=V2 (1), protect=On (1), lookahead=Vostok (1), listen=Full (0), rise=1, decay=1, sensitivity=0.8, preserve=1, mix=1, outputDb=0
- HamloComp (amount 1): thresholdDb=-22, ratio=2, attackMs=16, releaseMs=90, model=VCA (1), source=Drums (1), kneeDb=6, detector=RMS (1), character=Clean (0), autoMakeup=Off (0), makeupDb=0, mix=1

Reject/revise if: The lower body masks vocal low mids or snap loses height.

Intent check: Add a little body below a bright clean snap. Assess against: The lower body masks vocal low mids or snap loses height.. Original design-stage calibration status: **not_run**; final numerical results and individual owner-audition status are in RACK_BANK_REPORT.md.

### HR063 - Trap ATL Roll Definition

**Hats / 140 BPM.** Source: closed/open hats, an accent and a dense sixteenth roll

Keep rapid ticks separate by shortening bodies rather than boosting treble.

EQ: LowCut 950 Hz / +0 dB / Q 0.707; Bell 8500 Hz / +0 dB / Q 1.2 (dynamic -2 dB)

Full range:

- HamloUdarnik (amount 1): attackDb=0.7, sustainDb=-2, speed=0.85, style=PRAVDA (0), engine=V2 (1), protect=On (1), lookahead=Vostok (1), listen=Full (0), rise=1, decay=1, sensitivity=0.8, preserve=1, mix=1, outputDb=0

Reject/revise if: Sixteenth rolls fuse or the hats become brighter after matching.

Intent check: Keep rapid ticks separate by shortening bodies rather than boosting treble. Assess against: Sixteenth rolls fuse or the hats become brighter after matching.. Original design-stage calibration status: **not_run**; final numerical results and individual owner-audition status are in RACK_BANK_REPORT.md.

### HR064 - Trap Dark Metal Tame

**Hats / 140 BPM.** Source: closed/open hats, an accent and a dense sixteenth roll

Suppress sharp metal resonances while retaining quiet roll notes.

EQ: LowCut 700 Hz / +0 dB / Q 0.707; Bell 6500 Hz / +0 dB / Q 2 (dynamic -3.5 dB)

Full range:

- HamloComp (amount 1): thresholdDb=-26, ratio=2, attackMs=2, releaseMs=45, model=VCA (1), source=All (0), kneeDb=10, detector=RMS (1), character=Clean (0), autoMakeup=Off (0), makeupDb=0, mix=1

Reject/revise if: Accent resonance still rings or low-velocity notes are gated out.

Intent check: Suppress sharp metal resonances while retaining quiet roll notes. Assess against: Accent resonance still rings or low-velocity notes are gated out.. Original design-stage calibration status: **not_run**; final numerical results and individual owner-audition status are in RACK_BANK_REPORT.md.

### HR065 - Trap Cloud Wide Tick

**Hats / 140 BPM.** Source: closed/open hats, an accent and a dense sixteenth roll

Place only high hat energy wider with a stable dry center.

EQ: LowCut 1100 Hz / +0 dB / Q 0.707

Full range:

- HamloShirota (amount 1): v2Mode=Stereo / Space (1), v2Width=1, v2LowWidth=0, v2MidWidth=1, v2HighWidth=1.3, v2LowSplit=300, v2HighSplit=3000, v2MonoBass=300, v2Generate=0.12, v2Guard=On (1), v2Floor=0.15, v2Mix=1, v2Output=0, v2Motion=Off (0), v2PositionOn=Off (0), v2Mono=Stereo (0), v2Generator=Diffuse (2)
- HamloVerb (amount 0.045): space=Ambience (6), engine=Velvet (3), decaySeconds=0.3, predelayMs=0, predelaySync=Free (0), lowCutHz=1000, highCutHz=9000, bassMult=0.75, hfDecayRatio=0.5, width=1.1, modDepth=0.1, duck=0.15, routing=Send (1), mix=1, keepDry=Off (0), wetDb=0, outputDb=0, earlyDb=0, lateDb=0, timeShape=Off (0), shimmerAmount=0, freeze=Off (0)

Reject/revise if: Mono fold loses the roll or the center moves between hits.

Intent check: Place only high hat energy wider with a stable dry center. Assess against: Mono fold loses the roll or the center moves between hits.. Original design-stage calibration status: **not_run**; final numerical results and individual owner-audition status are in RACK_BANK_REPORT.md.

### HR066 - Trap Rage End Roll

**Hats / 140 BPM.** Source: closed/open hats, an accent and a dense sixteenth roll

Add one short BLOCK roll at the last sixteenth.

EQ: LowCut 1000 Hz / +0 dB / Q 0.707

Full range:

- HamloRevolution (amount 0.35): mix=1, free=Sync (0), grid=1/16 (2), triplet=Off (0), swing=0, humanize=0, fadeMs=3, order=T-G-M (0), scene=1 (0), listen=Off (0), timeOn=Off (0), glitchOn=On (1), motionOn=Off (0), timeMix=1, timeDepth=1, glitchMix=1, glitchDepth=0.35, motionMix=1, motionDepth=0.35, motionSmooth=0.5, motionTarget=Low Pass (1), timeStretch=Rhythm (1), glitchEngine=Block (1), shapeAOn=Off (0), shapeBOn=Off (0), throwAOn=Off (0), throwBOn=Off (0)
  - BLOCK GLITCH (steps 1-16): 16: Roll

Reject/revise if: The roll fires on an earlier beat or cuts the next downbeat.

Intent check: Add one short BLOCK roll at the last sixteenth. Assess against: The roll fires on an earlier beat or cuts the next downbeat.. Original design-stage calibration status: **not_run**; final numerical results and individual owner-audition status are in RACK_BANK_REPORT.md.

### HR067 - Trap Triplet Echo Ghost

**Hats / 140 BPM.** Source: closed/open hats, an accent and a dense sixteenth roll

Put a quiet triplet response around straight hats.

EQ: LowCut 1200 Hz / +0 dB / Q 0.707

Full range:

- HamloEcho (amount 0.1): echoMode=Score (1), quickEnabled=Off (0), scoreRouting=Send (1), scoreMix=1, scoreOutput=0, scoreSource=60, scoreFeedback=0.1, scoreFeedbackSync=1/8 T (4), scoreFeedbackMs=500, scoreFeedbackPitch=0, scoreColor=Clean (0), scoreDrive=0, scoreLowCut=1400, scoreHighCut=8500, scoreWidth=1, scoreDuck=0.2, scoreFreeze=Off (0), scoreSpaceMix=0, scoreGlitch=Off (0), scoreGlitchAmount=0, scoreBands=Off (0), scoreSwing=0, tap1On=1, tap2On=0, tap3On=0, tap4On=0, tap5On=0, tap6On=0, tap7On=0, tap8On=0, tap1Sync=1/8 T (4), tap1Time=250, tap1Note=60, tap1Gain=-6, tap1Pan=0, tap1Shape=Forward (0), tap1Fragment=125, tap1Repeats=3, tap1Low=1400, tap1High=8500, tap1SpaceSend=0

Reject/revise if: The triplet becomes louder than the original or timing sounds unsynced.

Intent check: Put a quiet triplet response around straight hats. Assess against: The triplet becomes louder than the original or timing sounds unsynced.. Original design-stage calibration status: **not_run**; final numerical results and individual owner-audition status are in RACK_BANK_REPORT.md.

### HR068 - Trap Bell Ping Seat

**Percussion / 140 BPM.** Source: a sparse percussion loop with velocity variation

Seat a hard bell with a damped offset reply.

EQ: LowCut 350 Hz / +0 dB / Q 0.707; Bell 2400 Hz / -1.5 dB / Q 1

Full range:

- HamloMag (amount 0.2): model=Tube (2), driveDb=3, mix=1, trimDb=0, oversampling=8x (3), autoGain=On (1), harmonics=0.35, tone=0, evenOdd=0, tapeBias=0.5, speed=1 (0), wow=0, hiss=0, hum=0, crackle=0, noiseFollow=Follow (1), punish=Off (0), thump=Off (0), steep=Off (0), sampler=Off (0), phoneBass=0, subProtectHz=0, dynamics=0, match=Off (0), delta=Off (0)
- HamloEcho (amount 0.12): echoMode=Score (1), quickEnabled=Off (0), scoreRouting=Send (1), scoreMix=1, scoreOutput=0, scoreSource=60, scoreFeedback=0.18, scoreFeedbackSync=1/8 D (6), scoreFeedbackMs=500, scoreFeedbackPitch=0, scoreColor=Warm (1), scoreDrive=0, scoreLowCut=600, scoreHighCut=5500, scoreWidth=1, scoreDuck=0.2, scoreFreeze=Off (0), scoreSpaceMix=0, scoreGlitch=Off (0), scoreGlitchAmount=0, scoreBands=Off (0), scoreSwing=0, tap1On=1, tap2On=0, tap3On=0, tap4On=0, tap5On=0, tap6On=0, tap7On=0, tap8On=0, tap1Sync=1/8 D (6), tap1Time=250, tap1Note=60, tap1Gain=-6, tap1Pan=0, tap1Shape=Forward (0), tap1Fragment=125, tap1Repeats=3, tap1Low=600, tap1High=5500, tap1SpaceSend=0

Reject/revise if: The reply clashes with the bell pitch or overtakes the next strike.

Intent check: Seat a hard bell with a damped offset reply. Assess against: The reply clashes with the bell pitch or overtakes the next strike.. Original design-stage calibration status: **not_run**; final numerical results and individual owner-audition status are in RACK_BANK_REPORT.md.

### HR069 - Trap Memphis Cowbell Grit

**Percussion / 140 BPM.** Source: a sparse percussion loop with velocity variation

Add a sampled rough edge with its fundamental held steady.

EQ: LowCut 250 Hz / +0 dB / Q 0.707; HighCut 10000 Hz / +0 dB / Q 0.707

Full range:

- HamloMag (amount 0.35): model=Iron (3), driveDb=7, mix=1, trimDb=0, oversampling=8x (3), autoGain=On (1), harmonics=0.35, tone=0, evenOdd=0, tapeBias=0.5, speed=1 (0), wow=0, hiss=0, hum=0, crackle=0, noiseFollow=Follow (1), punish=Off (0), thump=Off (0), steep=Off (0), sampler=MPC60 (2), phoneBass=0, subProtectHz=0, dynamics=0, match=Off (0), delta=Off (0)
- HamloUdarnik (amount 1): attackDb=0.5, sustainDb=-1, speed=0.75, style=PRAVDA (0), engine=V2 (1), protect=On (1), lookahead=Vostok (1), listen=Full (0), rise=1, decay=1, sensitivity=0.8, preserve=1, mix=1, outputDb=0

Reject/revise if: The cowbell pitch wavers or upper aliasing dominates.

Intent check: Add a sampled rough edge with its fundamental held steady. Assess against: The cowbell pitch wavers or upper aliasing dominates.. Original design-stage calibration status: **not_run**; final numerical results and individual owner-audition status are in RACK_BANK_REPORT.md.

### HR070 - Trap Texture Stereo Dots

**Percussion / 140 BPM.** Source: a sparse percussion loop with velocity variation

Separate dry percussion from small left-right echoes.

EQ: LowCut 500 Hz / +0 dB / Q 0.707

Full range:

- HamloEcho (amount 0.15): echoMode=Score (1), quickEnabled=Off (0), scoreRouting=Send (1), scoreMix=1, scoreOutput=0, scoreSource=60, scoreFeedback=0.1, scoreFeedbackSync=1/8 (5), scoreFeedbackMs=500, scoreFeedbackPitch=0, scoreColor=Clean (0), scoreDrive=0, scoreLowCut=180, scoreHighCut=8000, scoreWidth=1, scoreDuck=0.2, scoreFreeze=Off (0), scoreSpaceMix=0, scoreGlitch=Off (0), scoreGlitchAmount=0, scoreBands=Off (0), scoreSwing=0, tap1On=1, tap2On=1, tap3On=0, tap4On=0, tap5On=0, tap6On=0, tap7On=0, tap8On=0, tap1Sync=1/8 (5), tap1Time=250, tap1Note=60, tap1Gain=-9, tap1Pan=-0.7, tap1Shape=Forward (0), tap1Fragment=125, tap1Repeats=3, tap1Low=180, tap1High=8000, tap1SpaceSend=0, tap2Sync=1/4 (8), tap2Time=500, tap2Note=60, tap2Gain=-14, tap2Pan=0.7, tap2Shape=Forward (0), tap2Fragment=125, tap2Repeats=3, tap2Low=180, tap2High=8000, tap2SpaceSend=0

Reject/revise if: The echo shifts the dry event or mono loses its attack.

Intent check: Separate dry percussion from small left-right echoes. Assess against: The echo shifts the dry event or mono loses its attack.. Original design-stage calibration status: **not_run**; final numerical results and individual owner-audition status are in RACK_BANK_REPORT.md.

### HR071 - Trap Short Reverse Accent

**Percussion / 140 BPM.** Source: a sparse percussion loop with velocity variation

Reverse the final small percussion gesture only.

EQ: LowCut 300 Hz / +0 dB / Q 0.707

Full range:

- HamloRevolution (amount 0.4): mix=1, free=Sync (0), grid=1/16 (2), triplet=Off (0), swing=0, humanize=0, fadeMs=3, order=T-G-M (0), scene=1 (0), listen=Off (0), timeOn=On (1), glitchOn=Off (0), motionOn=Off (0), timeMix=0.7, timeDepth=1, glitchMix=1, glitchDepth=0.5, motionMix=1, motionDepth=0.35, motionSmooth=0.5, motionTarget=Low Pass (1), timeStretch=Rhythm (1), glitchEngine=Block (1), shapeAOn=Off (0), shapeBOn=Off (0), throwAOn=Off (0), throwBOn=Off (0)
  - TIME (steps 1-16): 16: Reverse
- HamloVerb (amount 0.05): space=Ambience (6), engine=Crisp (4), decaySeconds=0.25, predelayMs=0, predelaySync=Free (0), lowCutHz=800, highCutHz=6500, bassMult=0.75, hfDecayRatio=0.5, width=1.1, modDepth=0.1, duck=0.15, routing=Send (1), mix=1, keepDry=Off (0), wetDb=0, outputDb=0, earlyDb=0, lateDb=0, timeShape=Off (0), shimmerAmount=0, freeze=Off (0)

Reject/revise if: A full bar reverses or the following bar starts late.

Intent check: Reverse the final small percussion gesture only. Assess against: A full bar reverses or the following bar starts late.. Original design-stage calibration status: **not_run**; final numerical results and individual owner-audition status are in RACK_BANK_REPORT.md.

### HR072 - Trap ATL Dry Glue

**Drum Bus / 140 BPM.** Source: a stereo unprocessed drum loop with kick, snare, hats and ghost notes

Bind kick and snare with a fast recovery while preserving the front edge.

EQ: LowCut 25 Hz / +0 dB / Q 0.707; Bell 350 Hz / -0.7 dB / Q 1

Full range:

- HamloComp (amount 1): thresholdDb=-22, ratio=2, attackMs=22, releaseMs=100, model=VCA (1), source=Drums (1), kneeDb=6, detector=RMS (1), character=Clean (0), autoMakeup=Off (0), makeupDb=0, mix=1
- HamloMag (amount 0.2): model=Iron (3), driveDb=3, mix=1, trimDb=0, oversampling=8x (3), autoGain=On (1), harmonics=0.35, tone=0, evenOdd=0, tapeBias=0.5, speed=1 (0), wow=0, hiss=0, hum=0, crackle=0, noiseFollow=Follow (1), punish=Off (0), thump=Off (0), steep=Off (0), sampler=Off (0), phoneBass=0, subProtectHz=0, dynamics=0, match=Off (0), delta=Off (0)

Reject/revise if: The 808 slot in the groove feels smaller or hats pump on kicks.

Intent check: Bind kick and snare with a fast recovery while preserving the front edge. Assess against: The 808 slot in the groove feels smaller or hats pump on kicks.. Original design-stage calibration status: **not_run**; final numerical results and individual owner-audition status are in RACK_BANK_REPORT.md.

### HR073 - Trap Dark Parallel Pressure

**Drum Bus / 140 BPM.** Source: a stereo unprocessed drum loop with kick, snare, hats and ghost notes

Add compressed mid-body under clean drum peaks.

EQ: Bell 500 Hz / -1 dB / Q 1

Full range:

- HamloComp (amount 0.25): thresholdDb=-30, ratio=6, attackMs=10, releaseMs=85, model=FET (2), source=Drums (1), kneeDb=6, detector=RMS (1), character=Clean (0), autoMakeup=Off (0), makeupDb=0, mix=1

Band 120-6500 Hz:

- HamloMag (amount 0.25): model=Tube (2), driveDb=7, mix=1, trimDb=0, oversampling=8x (3), autoGain=On (1), harmonics=0.35, tone=0, evenOdd=0, tapeBias=0.5, speed=1 (0), wow=0, hiss=0, hum=0, crackle=0, noiseFollow=Follow (1), punish=Off (0), thump=Off (0), steep=Off (0), sampler=Off (0), phoneBass=0, subProtectHz=0, dynamics=0, match=Off (0), delta=Off (0)

Reject/revise if: The dry crest drops or the parallel path raises cymbal wash.

Intent check: Add compressed mid-body under clean drum peaks. Assess against: The dry crest drops or the parallel path raises cymbal wash.. Original design-stage calibration status: **not_run**; final numerical results and individual owner-audition status are in RACK_BANK_REPORT.md.

### HR074 - Trap Cloud Open Tops

**Drum Bus / 140 BPM.** Source: a stereo unprocessed drum loop with kick, snare, hats and ghost notes

Keep low drums centered and give upper percussion modest air.

EQ: LowCut 28 Hz / +0 dB / Q 0.707; HighShelf 10500 Hz / +0.5 dB / Q 0.707

Full range:

- HamloComp (amount 1): thresholdDb=-21, ratio=1.5, attackMs=30, releaseMs=125, model=Opto (3), source=Drums (1), kneeDb=6, detector=RMS (1), character=Clean (0), autoMakeup=Off (0), makeupDb=0, mix=1
- HamloShirota (amount 1): v2Mode=Stereo / Space (1), v2Width=1, v2LowWidth=0, v2MidWidth=1.05, v2HighWidth=1.2, v2LowSplit=300, v2HighSplit=3000, v2MonoBass=150, v2Generate=0, v2Guard=On (1), v2Floor=0.15, v2Mix=1, v2Output=0, v2Motion=Off (0), v2PositionOn=Off (0), v2Mono=Stereo (0)

Reject/revise if: The kick changes in mono or the top overshadows the vocal.

Intent check: Keep low drums centered and give upper percussion modest air. Assess against: The kick changes in mono or the top overshadows the vocal.. Original design-stage calibration status: **not_run**; final numerical results and individual owner-audition status are in RACK_BANK_REPORT.md.

### HR075 - Trap Rage Hit Contrast

**Drum Bus / 140 BPM.** Source: a stereo unprocessed drum loop with kick, snare, hats and ghost notes

Increase hit/body contrast without adding full-range clipping.

EQ: LowCut 30 Hz / +0 dB / Q 0.707

Full range:

- HamloUdarnik (amount 0.65): attackDb=1.5, sustainDb=-1, speed=0.8, style=UDARNIK (1), engine=V2 (1), protect=On (1), lookahead=Vostok (1), listen=Full (0), rise=1, decay=1, sensitivity=0.8, preserve=1, mix=1, outputDb=0
- HamloMag (amount 0.25): model=Iron (3), driveDb=4, mix=1, trimDb=0, oversampling=8x (3), autoGain=On (1), harmonics=0.35, tone=0, evenOdd=0, tapeBias=0.5, speed=1 (0), wow=0, hiss=0, hum=0, crackle=0, noiseFollow=Follow (1), punish=Off (0), thump=Off (0), steep=Off (0), sampler=Off (0), phoneBass=0, subProtectHz=100, dynamics=0, match=Off (0), delta=Off (0)

Reject/revise if: The loudest snare clips or quieter ghost hits vanish.

Intent check: Increase hit/body contrast without adding full-range clipping. Assess against: The loudest snare clips or quieter ghost hits vanish.. Original design-stage calibration status: **not_run**; final numerical results and individual owner-audition status are in RACK_BANK_REPORT.md.

### HR076 - Trap Memphis Sample Bus

**Drum Bus / 140 BPM.** Source: a stereo unprocessed drum loop with kick, snare, hats and ghost notes

Make a clean drum bus cohere around a restrained sampled midrange.

EQ: LowCut 28 Hz / +0 dB / Q 0.707; HighCut 14000 Hz / +0 dB / Q 0.707

Full range:

- HamloMag (amount 0.35): model=Tape (1), driveDb=5, mix=1, trimDb=0, oversampling=8x (3), autoGain=On (1), harmonics=0.35, tone=0, evenOdd=0, tapeBias=0.5, speed=1 (0), wow=0, hiss=0, hum=0, crackle=0, noiseFollow=Follow (1), punish=Off (0), thump=Off (0), steep=Off (0), sampler=S950 (3), phoneBass=0, subProtectHz=0, dynamics=0, match=Off (0), delta=Off (0)
- HamloComp (amount 1): thresholdDb=-23, ratio=2.5, attackMs=18, releaseMs=95, model=VCA (1), source=Drums (1), kneeDb=6, detector=RMS (1), character=Clean (0), autoMakeup=Off (0), makeupDb=0, mix=1

Reject/revise if: The entire beat loses high-frequency separation.

Intent check: Make a clean drum bus cohere around a restrained sampled midrange. Assess against: The entire beat loses high-frequency separation.. Original design-stage calibration status: **not_run**; final numerical results and individual owner-audition status are in RACK_BANK_REPORT.md.

### HR077 - Trap Last Beat Chop

**Drum Bus / 140 BPM.** Source: a stereo unprocessed drum loop with kick, snare, hats and ghost notes

Chop only the upper drum band at the last two subdivisions.

EQ: LowCut 25 Hz / +0 dB / Q 0.707

Full range:

- HamloComp (amount 1): thresholdDb=-21, ratio=1.7, attackMs=25, releaseMs=100, model=VCA (1), source=Drums (1), kneeDb=6, detector=RMS (1), character=Clean (0), autoMakeup=Off (0), makeupDb=0, mix=1

Band 800-16000 Hz:

- HamloRevolution (amount 0.3): mix=1, free=Sync (0), grid=1/16 (2), triplet=Off (0), swing=0, humanize=0, fadeMs=3, order=T-G-M (0), scene=1 (0), listen=Off (0), timeOn=Off (0), glitchOn=On (1), motionOn=Off (0), timeMix=1, timeDepth=1, glitchMix=1, glitchDepth=0.4, motionMix=1, motionDepth=0.35, motionSmooth=0.5, motionTarget=Low Pass (1), timeStretch=Rhythm (1), glitchEngine=Block (1), shapeAOn=Off (0), shapeBOn=Off (0), throwAOn=Off (0), throwBOn=Off (0)
  - BLOCK GLITCH (steps 1-16): 15: Chop; 16: Roll

Reject/revise if: The low kick is gated or the chop bleeds into the next bar.

Intent check: Chop only the upper drum band at the last two subdivisions. Assess against: The low kick is gated or the chop bleeds into the next bar.. Original design-stage calibration status: **not_run**; final numerical results and individual owner-audition status are in RACK_BANK_REPORT.md.

### HR078 - Trap Luxury Peak Discipline

**Drum Bus / 140 BPM.** Source: a stereo unprocessed drum loop with kick, snare, hats and ghost notes

Contain occasional snare spikes with short limiter recovery.

EQ: LowCut 26 Hz / +0 dB / Q 0.707

Full range:

- HamloComp (amount 1): thresholdDb=-20, ratio=1.6, attackMs=28, releaseMs=110, model=VCA (1), source=Drums (1), kneeDb=6, detector=RMS (1), character=Clean (0), autoMakeup=Off (0), makeupDb=0, mix=1
- HamloRedWall (amount 1): engine=V2 (1), thresholdDb=-1, ceilingDb=-1, releaseMs=22, autoRelease=Manual (0), lookaheadMs=2, link=1, truePeak=On (1), style=GLASNOST (0), outputDb=0, drive=0, tube=0, monitor=Normal (0), mix=1

Reject/revise if: Every kick is limited or the next hit remains attenuated.

Intent check: Contain occasional snare spikes with short limiter recovery. Assess against: Every kick is limited or the next hit remains attenuated.. Original design-stage calibration status: **not_run**; final numerical results and individual owner-audition status are in RACK_BANK_REPORT.md.

### HR079 - Trap ATL Eight Translation

**Bass / 140 BPM.** Source: a monophonic bass phrase with low and high notes and sustained tails

Generate harmonics above a clean 808 fundamental.

EQ: LowCut 24 Hz / +0 dB / Q 0.707

Full range:

- HamloMag (amount 0.4): model=Tube (2), driveDb=6, mix=1, trimDb=0, oversampling=8x (3), autoGain=On (1), harmonics=0.35, tone=0, evenOdd=0, tapeBias=0.5, speed=1 (0), wow=0, hiss=0, hum=0, crackle=0, noiseFollow=Follow (1), punish=Off (0), thump=Off (0), steep=Off (0), sampler=Off (0), phoneBass=0.35, subProtectHz=100, dynamics=0, match=Off (0), delta=Off (0)
- HamloShirota (amount 1): v2Mode=Stereo / Space (1), v2Width=1, v2LowWidth=0, v2MidWidth=0.95, v2HighWidth=1, v2LowSplit=300, v2HighSplit=3000, v2MonoBass=180, v2Generate=0, v2Guard=On (1), v2Floor=0.15, v2Mix=1, v2Output=0, v2Motion=Off (0), v2PositionOn=Off (0), v2Mono=Stereo (0)

Reject/revise if: Glides become rough below 100 Hz or the bass loses mono stability.

Intent check: Generate harmonics above a clean 808 fundamental. Assess against: Glides become rough below 100 Hz or the bass loses mono stability.. Original design-stage calibration status: **not_run**; final numerical results and individual owner-audition status are in RACK_BANK_REPORT.md.

### HR080 - Trap Dark Long Eight

**Bass / 140 BPM.** Source: a monophonic bass phrase with low and high notes and sustained tails

Level long 808 notes without compressing away the initial drop.

EQ: LowCut 22 Hz / +0 dB / Q 0.707; Bell 240 Hz / -1 dB / Q 1

Full range:

- HamloComp (amount 1): thresholdDb=-25, ratio=2.5, attackMs=35, releaseMs=180, model=Opto (3), source=All (0), kneeDb=6, detector=RMS (1), character=Clean (0), autoMakeup=Off (0), makeupDb=0, mix=1
- HamloMag (amount 0.3): model=Iron (3), driveDb=4, mix=1, trimDb=0, oversampling=8x (3), autoGain=On (1), harmonics=0.35, tone=0, evenOdd=0, tapeBias=0.5, speed=1 (0), wow=0, hiss=0, hum=0, crackle=0, noiseFollow=Follow (1), punish=Off (0), thump=Off (0), steep=Off (0), sampler=Off (0), phoneBass=0, subProtectHz=70, dynamics=0, match=Off (0), delta=Off (0)

Reject/revise if: The note breathes audibly or the glide tail is cut short.

Intent check: Level long 808 notes without compressing away the initial drop. Assess against: The note breathes audibly or the glide tail is cut short.. Original design-stage calibration status: **not_run**; final numerical results and individual owner-audition status are in RACK_BANK_REPORT.md.

### HR081 - Trap Cloud Soft Sub

**Bass / 140 BPM.** Source: a monophonic bass phrase with low and high notes and sustained tails

Soften upper buzz on a melodic 808 while maintaining pitch.

EQ: LowCut 24 Hz / +0 dB / Q 0.707; HighCut 6500 Hz / +0 dB / Q 0.707

Full range:

- HamloMag (amount 0.3): model=Tape (1), driveDb=3, mix=1, trimDb=0, oversampling=8x (3), autoGain=On (1), harmonics=0.35, tone=-0.2, evenOdd=0, tapeBias=0.5, speed=1 (0), wow=0, hiss=0, hum=0, crackle=0, noiseFollow=Follow (1), punish=Off (0), thump=Off (0), steep=Off (0), sampler=Off (0), phoneBass=0.12, subProtectHz=85, dynamics=0, match=Off (0), delta=Off (0)
- HamloComp (amount 1): thresholdDb=-23, ratio=1.7, attackMs=30, releaseMs=210, model=Opto (3), source=All (0), kneeDb=6, detector=RMS (1), character=Clean (0), autoMakeup=Off (0), makeupDb=0, mix=1

Reject/revise if: The bass vanishes on a small speaker or low notes become quieter.

Intent check: Soften upper buzz on a melodic 808 while maintaining pitch. Assess against: The bass vanishes on a small speaker or low notes become quieter.. Original design-stage calibration status: **not_run**; final numerical results and individual owner-audition status are in RACK_BANK_REPORT.md.

### HR082 - Trap Rage Upper Growl

**Bass / 140 BPM.** Source: a monophonic bass phrase with low and high notes and sustained tails

Distort the bass upper band while leaving the sub undriven.

EQ: LowCut 25 Hz / +0 dB / Q 0.707

Full range:

- HamloShirota (amount 1): v2Mode=Stereo / Space (1), v2Width=1, v2LowWidth=0, v2MidWidth=1, v2HighWidth=1.05, v2LowSplit=300, v2HighSplit=3000, v2MonoBass=170, v2Generate=0, v2Guard=On (1), v2Floor=0.15, v2Mix=1, v2Output=0, v2Motion=Off (0), v2PositionOn=Off (0), v2Mono=Stereo (0)

Band 150-5000 Hz:

- HamloMag (amount 0.4): model=Germanium (4), driveDb=13, mix=1, trimDb=0, oversampling=8x (3), autoGain=On (1), harmonics=0.35, tone=0, evenOdd=0, tapeBias=0.5, speed=1 (0), wow=0, hiss=0, hum=0, crackle=0, noiseFollow=Follow (1), punish=Off (0), thump=Off (0), steep=Off (0), sampler=Off (0), phoneBass=0, subProtectHz=0, dynamics=0, match=Off (0), delta=Off (0)
- HamloComp (amount 1): thresholdDb=-28, ratio=4, attackMs=8, releaseMs=80, model=FET (2), source=All (0), kneeDb=6, detector=RMS (1), character=Clean (0), autoMakeup=Off (0), makeupDb=0, mix=1

Reject/revise if: The fundamental clips or the growl covers the kick click.

Intent check: Distort the bass upper band while leaving the sub undriven. Assess against: The fundamental clips or the growl covers the kick click.. Original design-stage calibration status: **not_run**; final numerical results and individual owner-audition status are in RACK_BANK_REPORT.md.

### HR083 - Trap Memphis Short Eight

**Bass / 140 BPM.** Source: a monophonic bass phrase with low and high notes and sustained tails

Contain an 808 tail and add compact sampled midrange.

EQ: LowCut 25 Hz / +0 dB / Q 0.707; HighCut 9000 Hz / +0 dB / Q 0.707

Full range:

- HamloZastava (amount 1): v2_engine=Clean (1), v2_enabled=On (1), v2_threshold=-46, v2_range=12, v2_hold=90, v2_release=130, v2_attack=0.3, v2_hysteresis=4, v2_knee=6, v2_external=Main (0), v2_expand=Expand (1), v2_ratio=2, v2_link=1, v2_mix=1, v2_output=0, v2_body=90, v2_tail=130, v2_lookahead=2, v2_hp=25, v2_lp=400, v2_listen=Off (0)
- HamloMag (amount 0.3): model=Iron (3), driveDb=5, mix=1, trimDb=0, oversampling=8x (3), autoGain=On (1), harmonics=0.35, tone=0, evenOdd=0, tapeBias=0.5, speed=1 (0), wow=0, hiss=0, hum=0, crackle=0, noiseFollow=Follow (1), punish=Off (0), thump=Off (0), steep=Off (0), sampler=MPC60 (2), phoneBass=0, subProtectHz=65, dynamics=0, match=Off (0), delta=Off (0)

Reject/revise if: The envelope clicks on low notes or ends before pitch is established.

Intent check: Contain an 808 tail and add compact sampled midrange. Assess against: The envelope clicks on low notes or ends before pitch is established.. Original design-stage calibration status: **not_run**; final numerical results and individual owner-audition status are in RACK_BANK_REPORT.md.

### HR084 - Trap ATL Front Lead

**Vocal / 140 BPM.** Source: a dry lead phrase with plosives, sibilance and a quiet ending

Hold a dry rap vocal forward with fast FET control and restrained air.

EQ: LowCut 85 Hz / +0 dB / Q 0.707; Bell 350 Hz / -1.5 dB / Q 1; Bell 7200 Hz / +0 dB / Q 1.1 (dynamic -3 dB)

Full range:

- HamloFader (amount 1): engine=Modern (1), enabled=On (1), mode=Ride (2), gainDb=0, targetDb=-18, mix=1, polarity=Off (0), mute=Off (0), solo=Off (0), inputDb=0, outputDb=0, leftDb=0, rightDb=0, matchedDb=0, peakGuard=On (1), matchCeiling=-1, strength=0.45, maxBoost=2, maxCut=6, floorDb=-55, holdMs=100, riseMs=250, fallMs=140, depthDb=0, rightPhase=0
- HamloComp (amount 1): thresholdDb=-25, ratio=3.5, attackMs=6, releaseMs=90, model=FET (2), source=Voc (3), kneeDb=6, detector=RMS (1), character=Clean (0), autoMakeup=Off (0), makeupDb=0, mix=1
- HamloMag (amount 0.15): model=Tube (2), driveDb=2, mix=1, trimDb=0, oversampling=8x (3), autoGain=On (1), harmonics=0.35, tone=0, evenOdd=0, tapeBias=0.5, speed=1 (0), wow=0, hiss=0, hum=0, crackle=0, noiseFollow=Follow (1), punish=Off (0), thump=Off (0), steep=Off (0), sampler=Off (0), phoneBass=0, subProtectHz=0, dynamics=0, match=Off (0), delta=Off (0)

Reject/revise if: Sibilants rise or the next word ducks after a shout.

Intent check: Hold a dry rap vocal forward with fast FET control and restrained air. Assess against: Sibilants rise or the next word ducks after a shout.. Original design-stage calibration status: **not_run**; final numerical results and individual owner-audition status are in RACK_BANK_REPORT.md.

### HR085 - Trap Cloud Floating Hook

**Vocal / 140 BPM.** Source: a dry lead phrase with plosives, sibilance and a quiet ending

Let a ducked bright hall bloom after the hook words.

EQ: LowCut 110 Hz / +0 dB / Q 0.707; Bell 6500 Hz / +0 dB / Q 1 (dynamic -2.5 dB)

Full range:

- HamloComp (amount 1): thresholdDb=-24, ratio=2.2, attackMs=14, releaseMs=170, model=Opto (3), source=Voc (3), kneeDb=6, detector=RMS (1), character=Clean (0), autoMakeup=Off (0), makeupDb=0, mix=1
- HamloVerb (amount 0.18): space=Hall (3), engine=Velvet (3), decaySeconds=1.6, predelayMs=38, predelaySync=Free (0), lowCutHz=450, highCutHz=10000, bassMult=0.75, hfDecayRatio=0.5, width=1.1, modDepth=0.1, duck=0.5, routing=Send (1), mix=1, keepDry=Off (0), wetDb=0, outputDb=0, earlyDb=0, lateDb=0, timeShape=Off (0), shimmerAmount=0, freeze=Off (0)

Reject/revise if: Hall covers active consonants or low reverb overlaps the 808.

Intent check: Let a ducked bright hall bloom after the hook words. Assess against: Hall covers active consonants or low reverb overlaps the 808.. Original design-stage calibration status: **not_run**; final numerical results and individual owner-audition status are in RACK_BANK_REPORT.md.

### HR086 - Trap Dark Telephone Adlib

**Vocal / 140 BPM.** Source: a dry lead phrase with plosives, sibilance and a quiet ending

Band-limit an adlib and place a gritty echo behind it.

EQ: LowCut 420 Hz / +0 dB / Q 0.707; HighCut 4200 Hz / +0 dB / Q 0.707; Bell 1500 Hz / +1 dB / Q 1

Full range:

- HamloMag (amount 0.25): model=Germanium (4), driveDb=7, mix=1, trimDb=0, oversampling=8x (3), autoGain=On (1), harmonics=0.35, tone=0, evenOdd=0, tapeBias=0.5, speed=1 (0), wow=0, hiss=0, hum=0, crackle=0, noiseFollow=Follow (1), punish=Off (0), thump=Off (0), steep=Off (0), sampler=Off (0), phoneBass=0, subProtectHz=0, dynamics=0, match=Off (0), delta=Off (0)
- HamloEcho (amount 0.18): echoMode=Score (1), quickEnabled=Off (0), scoreRouting=Send (1), scoreMix=1, scoreOutput=0, scoreSource=60, scoreFeedback=0.25, scoreFeedbackSync=1/8 D (6), scoreFeedbackMs=500, scoreFeedbackPitch=0, scoreColor=BBD (3), scoreDrive=0, scoreLowCut=650, scoreHighCut=3500, scoreWidth=1, scoreDuck=0.2, scoreFreeze=Off (0), scoreSpaceMix=0, scoreGlitch=Off (0), scoreGlitchAmount=0, scoreBands=Off (0), scoreSwing=0, tap1On=1, tap2On=0, tap3On=0, tap4On=0, tap5On=0, tap6On=0, tap7On=0, tap8On=0, tap1Sync=1/8 D (6), tap1Time=250, tap1Note=60, tap1Gain=-6, tap1Pan=0, tap1Shape=Forward (0), tap1Fragment=125, tap1Repeats=3, tap1Low=650, tap1High=3500, tap1SpaceSend=0

Reject/revise if: Words lose intelligibility or the echo runs indefinitely.

Intent check: Band-limit an adlib and place a gritty echo behind it. Assess against: Words lose intelligibility or the echo runs indefinitely.. Original design-stage calibration status: **not_run**; final numerical results and individual owner-audition status are in RACK_BANK_REPORT.md.

### HR087 - Trap Rage Parallel Shout

**Vocal / 140 BPM.** Source: a dry lead phrase with plosives, sibilance and a quiet ending

Add rough density under a controlled shout without boosting ess sounds.

EQ: LowCut 100 Hz / +0 dB / Q 0.707; Bell 7000 Hz / +0 dB / Q 1 (dynamic -4 dB)

Full range:

- HamloComp (amount 0.5): thresholdDb=-27, ratio=5, attackMs=4, releaseMs=70, model=FET (2), source=Voc (3), kneeDb=6, detector=RMS (1), character=Clean (0), autoMakeup=Off (0), makeupDb=0, mix=1
- HamloMag (amount 0.2): model=Germanium (4), driveDb=9, mix=1, trimDb=0, oversampling=8x (3), autoGain=On (1), harmonics=0.35, tone=0, evenOdd=0, tapeBias=0.5, speed=1 (0), wow=0, hiss=0, hum=0, crackle=0, noiseFollow=Follow (1), punish=Off (0), thump=Off (0), steep=Off (0), sampler=Off (0), phoneBass=0, subProtectHz=0, dynamics=0, match=Off (0), delta=Off (0)
- HamloVerb (amount 0.05): space=Ambience (6), engine=Crisp (4), decaySeconds=0.3, predelayMs=12, predelaySync=Free (0), lowCutHz=500, highCutHz=6000, bassMult=0.75, hfDecayRatio=0.5, width=1.1, modDepth=0.1, duck=0.15, routing=Send (1), mix=1, keepDry=Off (0), wetDb=0, outputDb=0, earlyDb=0, lateDb=0, timeShape=Off (0), shimmerAmount=0, freeze=Off (0)

Reject/revise if: The shout sounds louder only because of unmatched level or its esses tear.

Intent check: Add rough density under a controlled shout without boosting ess sounds. Assess against: The shout sounds louder only because of unmatched level or its esses tear.. Original design-stage calibration status: **not_run**; final numerical results and individual owner-audition status are in RACK_BANK_REPORT.md.

### HR088 - Trap Luxury Pitch Replies

**Vocal / 140 BPM.** Source: a dry lead phrase with plosives, sibilance and a quiet ending

Place a quiet fifth-up reply after the dry phrase.

EQ: LowCut 95 Hz / +0 dB / Q 0.707; Bell 6800 Hz / +0 dB / Q 1 (dynamic -2.5 dB)

Full range:

- HamloComp (amount 1): thresholdDb=-23, ratio=2.5, attackMs=12, releaseMs=120, model=Opto (3), source=Voc (3), kneeDb=6, detector=RMS (1), character=Clean (0), autoMakeup=Off (0), makeupDb=0, mix=1
- HamloEcho (amount 0.12): echoMode=Score (1), quickEnabled=Off (0), scoreRouting=Send (1), scoreMix=1, scoreOutput=0, scoreSource=60, scoreFeedback=0.18, scoreFeedbackSync=1/8 D (6), scoreFeedbackMs=500, scoreFeedbackPitch=0, scoreColor=Clean (0), scoreDrive=0, scoreLowCut=180, scoreHighCut=8000, scoreWidth=1, scoreDuck=0.6, scoreFreeze=Off (0), scoreSpaceMix=0, scoreGlitch=Off (0), scoreGlitchAmount=0, scoreBands=Off (0), scoreSwing=0, tap1On=1, tap2On=0, tap3On=0, tap4On=0, tap5On=0, tap6On=0, tap7On=0, tap8On=0, tap1Sync=1/8 D (6), tap1Time=250, tap1Note=67, tap1Gain=-12, tap1Pan=-0.35, tap1Shape=Forward (0), tap1Fragment=125, tap1Repeats=3, tap1Low=180, tap1High=8000, tap1SpaceSend=0

Reject/revise if: The pitched reflection sounds like a doubled lead during words.

Intent check: Place a quiet fifth-up reply after the dry phrase. Assess against: The pitched reflection sounds like a doubled lead during words.. Original design-stage calibration status: **not_run**; final numerical results and individual owner-audition status are in RACK_BANK_REPORT.md.

### HR089 - Trap Cloud Bell Bed

**Keys / 140 BPM.** Source: a dry chord phrase with staccato and held notes

Keep bell attack clear above a dark softly spreading tail.

EQ: LowCut 180 Hz / +0 dB / Q 0.707; Bell 4000 Hz / -1 dB / Q 1

Full range:

- HamloUdarnik (amount 1): attackDb=-0.5, sustainDb=0.5, speed=0.6, style=PRAVDA (0), engine=V2 (1), protect=On (1), lookahead=Vostok (1), listen=Full (0), rise=1, decay=1, sensitivity=0.8, preserve=1, mix=1, outputDb=0
- HamloVerb (amount 0.16): space=Hall (3), engine=Velvet (3), decaySeconds=1.3, predelayMs=30, predelaySync=Free (0), lowCutHz=550, highCutHz=8500, bassMult=0.75, hfDecayRatio=0.5, width=1.1, modDepth=0.1, duck=0.15, routing=Send (1), mix=1, keepDry=Off (0), wetDb=0, outputDb=0, earlyDb=0, lateDb=0, timeShape=Off (0), shimmerAmount=0, freeze=Off (0)
- HamloShirota (amount 1): v2Mode=Stereo / Space (1), v2Width=1, v2LowWidth=0, v2MidWidth=1.1, v2HighWidth=1.25, v2LowSplit=300, v2HighSplit=3000, v2MonoBass=200, v2Generate=0, v2Guard=On (1), v2Floor=0.15, v2Mix=1, v2Output=0, v2Motion=Off (0), v2PositionOn=Off (0), v2Mono=Stereo (0)

Reject/revise if: The bell loses pitch or ambience hides the next arpeggio note.

Intent check: Keep bell attack clear above a dark softly spreading tail. Assess against: The bell loses pitch or ambience hides the next arpeggio note.. Original design-stage calibration status: **not_run**; final numerical results and individual owner-audition status are in RACK_BANK_REPORT.md.

### HR090 - Trap Dark Piano Space

**Keys / 140 BPM.** Source: a dry chord phrase with staccato and held notes

Remove muddy piano sustain and add a dark chamber.

EQ: LowCut 150 Hz / +0 dB / Q 0.707; Bell 350 Hz / -2 dB / Q 1; HighShelf 10000 Hz / -1 dB / Q 0.707

Full range:

- HamloComp (amount 1): thresholdDb=-24, ratio=2, attackMs=22, releaseMs=180, model=Opto (3), source=Samples (2), kneeDb=6, detector=RMS (1), character=Clean (0), autoMakeup=Off (0), makeupDb=0, mix=1
- HamloVerb (amount 0.15): space=Chamber (2), engine=Deep (5), decaySeconds=1.1, predelayMs=24, predelaySync=Free (0), lowCutHz=400, highCutHz=5500, bassMult=0.75, hfDecayRatio=0.5, width=1.1, modDepth=0.1, duck=0.15, routing=Send (1), mix=1, keepDry=Off (0), wetDb=0, outputDb=0, earlyDb=0, lateDb=0, timeShape=Off (0), shimmerAmount=0, freeze=Off (0)

Reject/revise if: The piano bottom covers the 808 or the chamber sounds detached.

Intent check: Remove muddy piano sustain and add a dark chamber. Assess against: The piano bottom covers the 808 or the chamber sounds detached.. Original design-stage calibration status: **not_run**; final numerical results and individual owner-audition status are in RACK_BANK_REPORT.md.

### HR091 - Trap Rage Pluck Edge

**Keys / 140 BPM.** Source: a dry chord phrase with staccato and held notes

Add controlled iron harmonics to a hard synth-key pluck.

EQ: LowCut 180 Hz / +0 dB / Q 0.707; Bell 2800 Hz / -1 dB / Q 1

Full range:

- HamloMag (amount 0.35): model=Iron (3), driveDb=7, mix=1, trimDb=0, oversampling=8x (3), autoGain=On (1), harmonics=0.35, tone=0, evenOdd=0, tapeBias=0.5, speed=1 (0), wow=0, hiss=0, hum=0, crackle=0, noiseFollow=Follow (1), punish=Off (0), thump=Off (0), steep=Off (0), sampler=Off (0), phoneBass=0, subProtectHz=0, dynamics=0.3, match=Off (0), delta=Off (0)
- HamloUdarnik (amount 1): attackDb=1, sustainDb=-1, speed=0.8, style=PRAVDA (0), engine=V2 (1), protect=On (1), lookahead=Vostok (1), listen=Full (0), rise=1, decay=1, sensitivity=0.8, preserve=1, mix=1, outputDb=0

Reject/revise if: The pluck becomes broadband distortion or masks consonants.

Intent check: Add controlled iron harmonics to a hard synth-key pluck. Assess against: The pluck becomes broadband distortion or masks consonants.. Original design-stage calibration status: **not_run**; final numerical results and individual owner-audition status are in RACK_BANK_REPORT.md.

### HR092 - Trap Luxury Eighth Sparkle

**Keys / 140 BPM.** Source: a dry chord phrase with staccato and held notes

Place light dotted-eighth answers around electric keys.

EQ: LowCut 140 Hz / +0 dB / Q 0.707

Full range:

- HamloMag (amount 0.2): model=Tube (2), driveDb=3, mix=1, trimDb=0, oversampling=8x (3), autoGain=On (1), harmonics=0.35, tone=0, evenOdd=0, tapeBias=0.5, speed=1 (0), wow=0, hiss=0, hum=0, crackle=0, noiseFollow=Follow (1), punish=Off (0), thump=Off (0), steep=Off (0), sampler=Off (0), phoneBass=0, subProtectHz=0, dynamics=0, match=Off (0), delta=Off (0)
- HamloEcho (amount 0.13): echoMode=Score (1), quickEnabled=Off (0), scoreRouting=Send (1), scoreMix=1, scoreOutput=0, scoreSource=60, scoreFeedback=0.18, scoreFeedbackSync=1/8 D (6), scoreFeedbackMs=500, scoreFeedbackPitch=0, scoreColor=Clean (0), scoreDrive=0, scoreLowCut=700, scoreHighCut=9500, scoreWidth=1, scoreDuck=0.2, scoreFreeze=Off (0), scoreSpaceMix=0, scoreGlitch=Off (0), scoreGlitchAmount=0, scoreBands=Off (0), scoreSwing=0, tap1On=1, tap2On=0, tap3On=0, tap4On=0, tap5On=0, tap6On=0, tap7On=0, tap8On=0, tap1Sync=1/8 D (6), tap1Time=250, tap1Note=60, tap1Gain=-6, tap1Pan=0, tap1Shape=Forward (0), tap1Fragment=125, tap1Repeats=3, tap1Low=700, tap1High=9500, tap1SpaceSend=0
- HamloShirota (amount 1): v2Mode=Stereo / Space (1), v2Width=1, v2LowWidth=0, v2MidWidth=1.05, v2HighWidth=1.2, v2LowSplit=300, v2HighSplit=3000, v2MonoBass=200, v2Generate=0, v2Guard=On (1), v2Floor=0.15, v2Mix=1, v2Output=0, v2Motion=Off (0), v2PositionOn=Off (0), v2Mono=Stereo (0)

Reject/revise if: The echo shifts the chord root or dominates the original pattern.

Intent check: Place light dotted-eighth answers around electric keys. Assess against: The echo shifts the chord root or dominates the original pattern.. Original design-stage calibration status: **not_run**; final numerical results and individual owner-audition status are in RACK_BANK_REPORT.md.

### HR093 - Trap Memphis Tape Scene

**Samples / 140 BPM.** Source: a musical sample loop with gaps and a clear phrase ending

Make a sample darker and more compact without adding continuous noise.

EQ: LowCut 120 Hz / +0 dB / Q 0.707; HighCut 11000 Hz / +0 dB / Q 0.707

Full range:

- HamloMag (amount 0.45): model=Tape (1), driveDb=6, mix=1, trimDb=0, oversampling=8x (3), autoGain=On (1), harmonics=0.35, tone=0, evenOdd=0, tapeBias=0.5, speed=1 (0), wow=0, hiss=0, hum=0, crackle=0, noiseFollow=Follow (1), punish=Off (0), thump=Off (0), steep=Off (0), sampler=SP-1200 (1), phoneBass=0, subProtectHz=0, dynamics=0, match=Off (0), delta=Off (0)
- HamloComp (amount 1): thresholdDb=-24, ratio=2, attackMs=20, releaseMs=120, model=Opto (3), source=Samples (2), kneeDb=6, detector=RMS (1), character=Clean (0), autoMakeup=Off (0), makeupDb=0, mix=1

Reject/revise if: Idle gaps gain hum or the chopped rhythm stops feeling clear.

Intent check: Make a sample darker and more compact without adding continuous noise. Assess against: Idle gaps gain hum or the chopped rhythm stops feeling clear.. Original design-stage calibration status: **not_run**; final numerical results and individual owner-audition status are in RACK_BANK_REPORT.md.

### HR094 - Trap Cloud Octave Mist

**Samples / 140 BPM.** Source: a musical sample loop with gaps and a clear phrase ending

Add sparse octave-up echo with a quiet soft space tail.

EQ: LowCut 160 Hz / +0 dB / Q 0.707

Full range:

- HamloEcho (amount 0.14): echoMode=Score (1), quickEnabled=Off (0), scoreRouting=Send (1), scoreMix=1, scoreOutput=0, scoreSource=60, scoreFeedback=0.16, scoreFeedbackSync=1/4 (8), scoreFeedbackMs=500, scoreFeedbackPitch=0, scoreColor=Clean (0), scoreDrive=0, scoreLowCut=180, scoreHighCut=8000, scoreWidth=1, scoreDuck=0.2, scoreFreeze=Off (0), scoreSpaceMix=0.18, scoreGlitch=Off (0), scoreGlitchAmount=0, scoreBands=Off (0), scoreSwing=0, tap1On=1, tap2On=0, tap3On=0, tap4On=0, tap5On=0, tap6On=0, tap7On=0, tap8On=0, tap1Sync=1/4 (8), tap1Time=250, tap1Note=72, tap1Gain=-14, tap1Pan=0.4, tap1Shape=Forward (0), tap1Fragment=125, tap1Repeats=3, tap1Low=180, tap1High=8000, tap1SpaceSend=0.5, scoreSpaceDecay=1.8

Reject/revise if: The octave becomes the dominant melody or feedback changes the root repeatedly.

Intent check: Add sparse octave-up echo with a quiet soft space tail. Assess against: The octave becomes the dominant melody or feedback changes the root repeatedly.. Original design-stage calibration status: **not_run**; final numerical results and individual owner-audition status are in RACK_BANK_REPORT.md.

### HR095 - Trap Last Word Tape Stop

**Samples / 140 BPM.** Source: a musical sample loop with gaps and a clear phrase ending

Stop the final sample subdivision and resume cleanly on the downbeat.

EQ: LowCut 90 Hz / +0 dB / Q 0.707

Full range:

- HamloRevolution (amount 0.65): mix=1, free=Sync (0), grid=1/16 (2), triplet=Off (0), swing=0, humanize=0, fadeMs=3, order=T-G-M (0), scene=1 (0), listen=Off (0), timeOn=On (1), glitchOn=Off (0), motionOn=Off (0), timeMix=1, timeDepth=1, glitchMix=1, glitchDepth=0.5, motionMix=1, motionDepth=0.35, motionSmooth=0.5, motionTarget=Low Pass (1), timeStretch=Tape (0), glitchEngine=Block (1), shapeAOn=Off (0), shapeBOn=Off (0), throwAOn=Off (0), throwBOn=Off (0)
  - TIME (steps 1-16): 16: Tape Stop

Reject/revise if: The next bar starts pitched down or has missing samples.

Intent check: Stop the final sample subdivision and resume cleanly on the downbeat. Assess against: The next bar starts pitched down or has missing samples.. Original design-stage calibration status: **not_run**; final numerical results and individual owner-audition status are in RACK_BANK_REPORT.md.

### HR096 - Trap Gated Melody Pocket

**Samples / 140 BPM.** Source: a musical sample loop with gaps and a clear phrase ending

Carve a syncopated melody envelope around the kick and snare.

EQ: LowCut 180 Hz / +0 dB / Q 0.707

Full range:

- HamloZastava (amount 0.65): v2_engine=Rhythm (4), v2_enabled=On (1), v2_threshold=-40, v2_range=24, v2_hold=0, v2_release=8, v2_attack=2, v2_hysteresis=4, v2_knee=6, v2_external=Main (0), v2_expand=Expand (1), v2_ratio=2, v2_link=1, v2_mix=1, v2_output=0, v2_body=0, v2_tail=8, v2_lookahead=2, v2_hp=0, v2_lp=0, v2_listen=Off (0), v2_steps=16, v2_beats=0.25, v2_sync=Tempo (1), v2_step1=1, v2_step2=0.6, v2_step3=0, v2_step4=0.4, v2_step5=1, v2_step6=0, v2_step7=0.6, v2_step8=0, v2_step9=1, v2_step10=0.5, v2_step11=0, v2_step12=0.7, v2_step13=1, v2_step14=0, v2_step15=0.5, v2_step16=0, v2_step17=1, v2_step18=0.6, v2_step19=0, v2_step20=0.4, v2_step21=1, v2_step22=0, v2_step23=0.6, v2_step24=0, v2_step25=1, v2_step26=0.5, v2_step27=0, v2_step28=0.7, v2_step29=1, v2_step30=0, v2_step31=0.5, v2_step32=0
- HamloVerb (amount 0.06): space=Ambience (6), engine=Velvet (3), decaySeconds=0.35, predelayMs=0, predelaySync=Free (0), lowCutHz=500, highCutHz=8000, bassMult=0.75, hfDecayRatio=0.5, width=1.1, modDepth=0.1, duck=0.15, routing=Send (1), mix=1, keepDry=Off (0), wetDb=0, outputDb=0, earlyDb=0, lateDb=0, timeShape=Off (0), shimmerAmount=0, freeze=Off (0)

Reject/revise if: Gate edges click or the rhythm obscures the melody's main accents.

Intent check: Carve a syncopated melody envelope around the kick and snare. Assess against: Gate edges click or the rhythm obscures the melody's main accents.. Original design-stage calibration status: **not_run**; final numerical results and individual owner-audition status are in RACK_BANK_REPORT.md.

### HR097 - Trap ATL Sub Anchor

**Master / 140 BPM.** Source: an unmastered stereo mix with headroom and a busy chorus

Keep the sub mono and let kick and vocal peaks survive gentle limiting.

EQ: LowCut 22 Hz / +0 dB / Q 0.707; Bell 280 Hz / -0.6 dB / Q 1

Full range:

- HamloShirota (amount 1): v2Mode=Stereo / Space (1), v2Width=1, v2LowWidth=0, v2MidWidth=1, v2HighWidth=1.06, v2LowSplit=300, v2HighSplit=3000, v2MonoBass=130, v2Generate=0, v2Guard=On (1), v2Floor=0.15, v2Mix=1, v2Output=0, v2Motion=Off (0), v2PositionOn=Off (0), v2Mono=Stereo (0)
- HamloComp (amount 1): thresholdDb=-18, ratio=1.4, attackMs=35, releaseMs=107, model=VCA (1), source=All (0), kneeDb=6, detector=RMS (1), character=Clean (0), autoMakeup=Off (0), makeupDb=0, mix=1
- HamloRedWall (amount 1): engine=V2 (1), thresholdDb=-2.2, ceilingDb=-1, releaseMs=90, autoRelease=Manual (0), lookaheadMs=2, link=1, truePeak=On (1), style=GLASNOST (0), outputDb=0, drive=0, tube=0, monitor=Normal (0), mix=1

Reject/revise if: Final 8x true peak exceeds -1 dBTP or bass distorts before 3 dB limiter work.

Intent check: Keep the sub mono and let kick and vocal peaks survive gentle limiting. Assess against: Final 8x true peak exceeds -1 dBTP or bass distorts before 3 dB limiter work.. Original design-stage calibration status: **not_run**; final numerical results and individual owner-audition status are in RACK_BANK_REPORT.md.

### HR098 - Trap Cloud Air Master

**Master / 140 BPM.** Source: an unmastered stereo mix with headroom and a busy chorus

Open a soft hook mix while containing upper harshness dynamically.

EQ: LowCut 23 Hz / +0 dB / Q 0.707; HighShelf 11500 Hz / +0.5 dB / Q 0.707; Bell 7200 Hz / +0 dB / Q 1 (dynamic -1.5 dB)

Full range:

- HamloComp (amount 1): thresholdDb=-19, ratio=1.4, attackMs=40, releaseMs=160, model=Opto (3), source=All (0), kneeDb=6, detector=RMS (1), character=Clean (0), autoMakeup=Off (0), makeupDb=0, mix=1
- HamloShirota (amount 1): v2Mode=Stereo / Space (1), v2Width=1, v2LowWidth=0.9, v2MidWidth=1.03, v2HighWidth=1.1, v2LowSplit=300, v2HighSplit=3000, v2MonoBass=120, v2Generate=0, v2Guard=On (1), v2Floor=0.15, v2Mix=1, v2Output=0, v2Motion=Off (0), v2PositionOn=Off (0), v2Mono=Stereo (0)
- HamloRedWall (amount 1): engine=V2 (1), thresholdDb=-1.8, ceilingDb=-1, releaseMs=125, autoRelease=Manual (0), lookaheadMs=2, link=1, truePeak=On (1), style=GLASNOST (0), outputDb=0, drive=0, tube=0, monitor=Normal (0), mix=1

Reject/revise if: The airy band hisses or the matched vocal becomes recessed.

Intent check: Open a soft hook mix while containing upper harshness dynamically. Assess against: The airy band hisses or the matched vocal becomes recessed.. Original design-stage calibration status: **not_run**; final numerical results and individual owner-audition status are in RACK_BANK_REPORT.md.

### HR099 - Trap Dark Density Master

**Master / 140 BPM.** Source: an unmastered stereo mix with headroom and a busy chorus

Support midrange density without saturating the deepest 808.

EQ: LowCut 22 Hz / +0 dB / Q 0.707; Bell 420 Hz / -0.6 dB / Q 1

Full range:

- HamloMag (amount 0.18): model=Iron (3), driveDb=3, mix=1, trimDb=0, oversampling=8x (3), autoGain=On (1), harmonics=0.35, tone=0, evenOdd=0, tapeBias=0.5, speed=1 (0), wow=0, hiss=0, hum=0, crackle=0, noiseFollow=Follow (1), punish=Off (0), thump=Off (0), steep=Off (0), sampler=Off (0), phoneBass=0, subProtectHz=110, dynamics=0, match=Off (0), delta=Off (0)
- HamloComp (amount 1): thresholdDb=-20, ratio=1.6, attackMs=30, releaseMs=110, model=VCA (1), source=All (0), kneeDb=6, detector=RMS (1), character=Clean (0), autoMakeup=Off (0), makeupDb=0, mix=1
- HamloRedWall (amount 1): engine=V2 (1), thresholdDb=-2.5, ceilingDb=-1, releaseMs=95, autoRelease=Manual (0), lookaheadMs=2, link=1, truePeak=On (1), style=GLASNOST (0), outputDb=0, drive=0, tube=0, monitor=Normal (0), mix=1

Reject/revise if: Sub harmonics buzz audibly or limiter work exceeds 3 dB.

Intent check: Support midrange density without saturating the deepest 808. Assess against: Sub harmonics buzz audibly or limiter work exceeds 3 dB.. Original design-stage calibration status: **not_run**; final numerical results and individual owner-audition status are in RACK_BANK_REPORT.md.

### HR100 - Trap Luxury Clean Ceiling

**Master / 140 BPM.** Source: an unmastered stereo mix with headroom and a busy chorus

Finish a balanced clean mix with little colour and bounded stereo width.

EQ: LowCut 24 Hz / +0 dB / Q 0.707

Full range:

- HamloShirota (amount 1): v2Mode=Stereo / Space (1), v2Width=1, v2LowWidth=0, v2MidWidth=1.02, v2HighWidth=1.07, v2LowSplit=300, v2HighSplit=3000, v2MonoBass=140, v2Generate=0, v2Guard=On (1), v2Floor=0.15, v2Mix=1, v2Output=0, v2Motion=Off (0), v2PositionOn=Off (0), v2Mono=Stereo (0)
- HamloComp (amount 1): thresholdDb=-17, ratio=1.3, attackMs=45, releaseMs=140, model=VCA (1), source=All (0), kneeDb=6, detector=RMS (1), character=Clean (0), autoMakeup=Off (0), makeupDb=0, mix=1
- HamloRedWall (amount 1): engine=V2 (1), thresholdDb=-1.5, ceilingDb=-1, releaseMs=105, autoRelease=Manual (0), lookaheadMs=2, link=1, truePeak=On (1), style=GLASNOST (0), outputDb=0, drive=0, tube=0, monitor=Normal (0), mix=1

Reject/revise if: Mono fold loses the hook or limiting changes the 808 envelope.

Intent check: Finish a balanced clean mix with little colour and bounded stereo width. Assess against: Mono fold loses the hook or limiting changes the 808 envelope.. Original design-stage calibration status: **not_run**; final numerical results and individual owner-audition status are in RACK_BANK_REPORT.md.

## Techno

### HR101 - Techno Peak Front Pillar

**Kick / 132 BPM.** Source: isolated kick hits including soft and hard strikes

Keep a four-on-floor kick firm with low-mid space and an intact transient.

EQ: LowCut 26 Hz / +0 dB / Q 0.707; Bell 300 Hz / -2 dB / Q 1

Full range:

- HamloUdarnik (amount 1): attackDb=1.5, sustainDb=-1, speed=0.75, style=PRAVDA (0), engine=V2 (1), protect=On (1), lookahead=Vostok (1), listen=Full (0), rise=1, decay=1, sensitivity=0.8, preserve=1, mix=1, outputDb=0
- HamloComp (amount 1): thresholdDb=-22, ratio=2.5, attackMs=20, releaseMs=114, model=VCA (1), source=Drums (1), kneeDb=6, detector=RMS (1), character=Clean (0), autoMakeup=Off (0), makeupDb=0, mix=1

Reject/revise if: The next quarter loses impact or low-mid buildup remains.

Intent check: Keep a four-on-floor kick firm with low-mid space and an intact transient. Assess against: The next quarter loses impact or low-mid buildup remains.. Original design-stage calibration status: **not_run**; final numerical results and individual owner-audition status are in RACK_BANK_REPORT.md.

### HR102 - Techno Hypnotic Round Pulse

**Kick / 132 BPM.** Source: isolated kick hits including soft and hard strikes

Round a sharp kick and let its low body carry the pulse.

EQ: LowCut 25 Hz / +0 dB / Q 0.707; Bell 4200 Hz / -2 dB / Q 1

Full range:

- HamloMag (amount 0.4): model=Tape (1), driveDb=5, mix=1, trimDb=0, oversampling=8x (3), autoGain=On (1), harmonics=0.35, tone=0, evenOdd=0, tapeBias=0.5, speed=1 (0), wow=0, hiss=0, hum=0, crackle=0, noiseFollow=Follow (1), punish=Off (0), thump=Off (0), steep=Off (0), sampler=Off (0), phoneBass=0, subProtectHz=55, dynamics=0, match=Off (0), delta=Off (0)
- HamloUdarnik (amount 1): attackDb=-0.8, sustainDb=-0.5, speed=0.6, style=PRAVDA (0), engine=V2 (1), protect=On (1), lookahead=Vostok (1), listen=Full (0), rise=1, decay=1, sensitivity=0.8, preserve=1, mix=1, outputDb=0

Reject/revise if: The upper click still dominates or low body blooms past the next beat.

Intent check: Round a sharp kick and let its low body carry the pulse. Assess against: The upper click still dominates or low body blooms past the next beat.. Original design-stage calibration status: **not_run**; final numerical results and individual owner-audition status are in RACK_BANK_REPORT.md.

### HR103 - Techno Minimal Dry Foot

**Kick / 132 BPM.** Source: isolated kick hits including soft and hard strikes

Remove excess tail and leave room for a sparse bass pattern.

EQ: LowCut 30 Hz / +0 dB / Q 0.707; Bell 420 Hz / -1.5 dB / Q 1

Full range:

- HamloZastava (amount 1): v2_engine=Drum (2), v2_enabled=On (1), v2_threshold=-42, v2_range=16, v2_hold=35, v2_release=60, v2_attack=0.3, v2_hysteresis=4, v2_knee=6, v2_external=Main (0), v2_expand=Expand (1), v2_ratio=2, v2_link=1, v2_mix=1, v2_output=0, v2_body=35, v2_tail=60, v2_lookahead=2, v2_hp=0, v2_lp=0, v2_listen=Off (0)
- HamloUdarnik (amount 1): attackDb=0.7, sustainDb=-1.5, speed=0.8, style=PRAVDA (0), engine=V2 (1), protect=On (1), lookahead=Vostok (1), listen=Full (0), rise=1, decay=1, sensitivity=0.8, preserve=1, mix=1, outputDb=0

Reject/revise if: The shortened envelope clicks or the kick becomes hollow.

Intent check: Remove excess tail and leave room for a sparse bass pattern. Assess against: The shortened envelope clicks or the kick becomes hollow.. Original design-stage calibration status: **not_run**; final numerical results and individual owner-audition status are in RACK_BANK_REPORT.md.

### HR104 - Techno Detroit Iron Center

**Kick / 132 BPM.** Source: isolated kick hits including soft and hard strikes

Give a clean electronic kick restrained iron midrange.

EQ: LowCut 25 Hz / +0 dB / Q 0.707; LowShelf 70 Hz / +0.7 dB / Q 0.707

Full range:

- HamloMag (amount 0.4): model=Iron (3), driveDb=7, mix=1, trimDb=0, oversampling=8x (3), autoGain=On (1), harmonics=0.35, tone=0, evenOdd=0, tapeBias=0.5, speed=1 (0), wow=0, hiss=0, hum=0, crackle=0, noiseFollow=Follow (1), punish=Off (0), thump=Off (0), steep=Off (0), sampler=Off (0), phoneBass=0, subProtectHz=80, dynamics=0, match=Off (0), delta=Off (0)
- HamloComp (amount 1): thresholdDb=-21, ratio=2, attackMs=24, releaseMs=125, model=VCA (1), source=Drums (1), kneeDb=6, detector=RMS (1), character=Clean (0), autoMakeup=Off (0), makeupDb=0, mix=1

Reject/revise if: The fundamental buzzes or harmonics bury the hi-hat.

Intent check: Give a clean electronic kick restrained iron midrange. Assess against: The fundamental buzzes or harmonics bury the hi-hat.. Original design-stage calibration status: **not_run**; final numerical results and individual owner-audition status are in RACK_BANK_REPORT.md.

### HR105 - Techno Dub Dark Rumble Seed

**Kick / 132 BPM.** Source: isolated kick hits including soft and hard strikes

Derive a filtered room bloom from the kick without a long bright tail.

EQ: LowCut 28 Hz / +0 dB / Q 0.707

Full range:

- HamloMag (amount 0.3): model=Tape (1), driveDb=4, mix=1, trimDb=0, oversampling=8x (3), autoGain=On (1), harmonics=0.35, tone=0, evenOdd=0, tapeBias=0.5, speed=1 (0), wow=0, hiss=0, hum=0, crackle=0, noiseFollow=Follow (1), punish=Off (0), thump=Off (0), steep=Off (0), sampler=Off (0), phoneBass=0, subProtectHz=0, dynamics=0, match=Off (0), delta=Off (0)

Band 70-420 Hz:

- HamloVerb (amount 0.14): space=Room (1), engine=Deep (5), decaySeconds=0.6, predelayMs=20, predelaySync=Free (0), lowCutHz=80, highCutHz=1800, bassMult=0.75, hfDecayRatio=0.5, width=1.1, modDepth=0.1, duck=0.15, routing=Send (1), mix=1, keepDry=Off (0), wetDb=0, outputDb=0, earlyDb=0, lateDb=0, timeShape=Off (0), shimmerAmount=0, freeze=Off (0)

Reject/revise if: The bloom builds across bars or destabilizes the dry kick pitch.

Intent check: Derive a filtered room bloom from the kick without a long bright tail. Assess against: The bloom builds across bars or destabilizes the dry kick pitch.. Original design-stage calibration status: **not_run**; final numerical results and individual owner-audition status are in RACK_BANK_REPORT.md.

### HR106 - Techno Hard Parallel Hammer

**Kick / 132 BPM.** Source: isolated kick hits including soft and hard strikes

Blend aggressive upper kick colour below a clean low anchor.

EQ: LowCut 26 Hz / +0 dB / Q 0.707; Bell 550 Hz / -2 dB / Q 1

Full range:

- HamloUdarnik (amount 1): attackDb=1, sustainDb=-1, speed=0.8, style=PRAVDA (0), engine=V2 (1), protect=On (1), lookahead=Vostok (1), listen=Full (0), rise=1, decay=1, sensitivity=0.8, preserve=1, mix=1, outputDb=0

Band 130-4500 Hz:

- HamloMag (amount 0.35): model=Germanium (4), driveDb=15, mix=1, trimDb=0, oversampling=8x (3), autoGain=On (1), harmonics=0.35, tone=0, evenOdd=0, tapeBias=0.5, speed=1 (0), wow=0, hiss=0, hum=0, crackle=0, noiseFollow=Follow (1), punish=Off (0), thump=Off (0), steep=Off (0), sampler=Off (0), phoneBass=0, subProtectHz=0, dynamics=0, match=Off (0), delta=Off (0)
- HamloComp (amount 0.45): thresholdDb=-29, ratio=5, attackMs=10, releaseMs=75, model=FET (2), source=Drums (1), kneeDb=6, detector=RMS (1), character=Clean (0), autoMakeup=Off (0), makeupDb=0, mix=1

Reject/revise if: The sub is distorted or the matched transient loses punch.

Intent check: Blend aggressive upper kick colour below a clean low anchor. Assess against: The sub is distorted or the matched transient loses punch.. Original design-stage calibration status: **not_run**; final numerical results and individual owner-audition status are in RACK_BANK_REPORT.md.

### HR107 - Techno Peak Clap Hit

**Snare / 132 BPM.** Source: snare or clap hits including ghost notes and a flam

Emphasize clap attack and pull its tail off the next hat.

EQ: LowCut 150 Hz / +0 dB / Q 0.707; Bell 600 Hz / -1.5 dB / Q 1

Full range:

- HamloUdarnik (amount 1): attackDb=2, sustainDb=-1.5, speed=0.8, style=PRAVDA (0), engine=V2 (1), protect=On (1), lookahead=Vostok (1), listen=Full (0), rise=1, decay=1, sensitivity=0.8, preserve=1, mix=1, outputDb=0
- HamloComp (amount 1): thresholdDb=-22, ratio=2.5, attackMs=12, releaseMs=95, model=VCA (1), source=Drums (1), kneeDb=6, detector=RMS (1), character=Clean (0), autoMakeup=Off (0), makeupDb=0, mix=1

Reject/revise if: The clap turns papery or the hat space remains crowded.

Intent check: Emphasize clap attack and pull its tail off the next hat. Assess against: The clap turns papery or the hat space remains crowded.. Original design-stage calibration status: **not_run**; final numerical results and individual owner-audition status are in RACK_BANK_REPORT.md.

### HR108 - Techno Hypnotic Rim Chamber

**Snare / 132 BPM.** Source: snare or clap hits including ghost notes and a flam

Place a dry rim in a small dark chamber with a clear center.

EQ: LowCut 200 Hz / +0 dB / Q 0.707; Bell 1600 Hz / -1 dB / Q 1

Full range:

- HamloMag (amount 0.25): model=Tube (2), driveDb=4, mix=1, trimDb=0, oversampling=8x (3), autoGain=On (1), harmonics=0.35, tone=0, evenOdd=0, tapeBias=0.5, speed=1 (0), wow=0, hiss=0, hum=0, crackle=0, noiseFollow=Follow (1), punish=Off (0), thump=Off (0), steep=Off (0), sampler=Off (0), phoneBass=0, subProtectHz=0, dynamics=0, match=Off (0), delta=Off (0)
- HamloVerb (amount 0.12): space=Chamber (2), engine=Warm (2), decaySeconds=0.55, predelayMs=12, predelaySync=Free (0), lowCutHz=650, highCutHz=5000, bassMult=0.75, hfDecayRatio=0.5, width=1.1, modDepth=0.1, duck=0.15, routing=Send (1), mix=1, keepDry=Off (0), wetDb=0, outputDb=0, earlyDb=0, lateDb=0, timeShape=Off (0), shimmerAmount=0, freeze=Off (0)

Reject/revise if: The chamber becomes a second rhythmic event or dulls the rim.

Intent check: Place a dry rim in a small dark chamber with a clear center. Assess against: The chamber becomes a second rhythmic event or dulls the rim.. Original design-stage calibration status: **not_run**; final numerical results and individual owner-audition status are in RACK_BANK_REPORT.md.

### HR109 - Techno Minimal Tight Snap

**Snare / 132 BPM.** Source: snare or clap hits including ghost notes and a flam

Contain a synthetic snap with a fast drum envelope.

EQ: LowCut 220 Hz / +0 dB / Q 0.707; Bell 8000 Hz / +0 dB / Q 1.2 (dynamic -2 dB)

Full range:

- HamloZastava (amount 1): v2_engine=Drum (2), v2_enabled=On (1), v2_threshold=-43, v2_range=12, v2_hold=20, v2_release=45, v2_attack=0.3, v2_hysteresis=4, v2_knee=6, v2_external=Main (0), v2_expand=Expand (1), v2_ratio=2, v2_link=1, v2_mix=1, v2_output=0, v2_body=20, v2_tail=45, v2_lookahead=2, v2_hp=0, v2_lp=0, v2_listen=Off (0)
- HamloUdarnik (amount 1): attackDb=1, sustainDb=-1, speed=0.85, style=PRAVDA (0), engine=V2 (1), protect=On (1), lookahead=Vostok (1), listen=Full (0), rise=1, decay=1, sensitivity=0.8, preserve=1, mix=1, outputDb=0

Reject/revise if: Quiet hits disappear or the release adds chatter.

Intent check: Contain a synthetic snap with a fast drum envelope. Assess against: Quiet hits disappear or the release adds chatter.. Original design-stage calibration status: **not_run**; final numerical results and individual owner-audition status are in RACK_BANK_REPORT.md.

### HR110 - Techno Detroit Short Plate

**Snare / 132 BPM.** Source: snare or clap hits including ghost notes and a flam

Give a machine snare a metallic but bounded plate tail.

EQ: LowCut 140 Hz / +0 dB / Q 0.707; Bell 280 Hz / +1 dB / Q 1

Full range:

- HamloMag (amount 0.25): model=Iron (3), driveDb=4, mix=1, trimDb=0, oversampling=8x (3), autoGain=On (1), harmonics=0.35, tone=0, evenOdd=0, tapeBias=0.5, speed=1 (0), wow=0, hiss=0, hum=0, crackle=0, noiseFollow=Follow (1), punish=Off (0), thump=Off (0), steep=Off (0), sampler=Off (0), phoneBass=0, subProtectHz=0, dynamics=0, match=Off (0), delta=Off (0)
- HamloVerb (amount 0.14): space=Plate (4), engine=Crisp (4), decaySeconds=0.65, predelayMs=15, predelaySync=Free (0), lowCutHz=500, highCutHz=7500, bassMult=0.75, hfDecayRatio=0.5, width=1.1, modDepth=0.1, duck=0.15, routing=Send (1), mix=1, keepDry=Off (0), wetDb=0, outputDb=0, earlyDb=0, lateDb=0, timeShape=Off (0), shimmerAmount=0, freeze=Off (0)

Reject/revise if: The plate obscures the clap grid or low mids ring.

Intent check: Give a machine snare a metallic but bounded plate tail. Assess against: The plate obscures the clap grid or low mids ring.. Original design-stage calibration status: **not_run**; final numerical results and individual owner-audition status are in RACK_BANK_REPORT.md.

### HR111 - Techno Dub Snare Throw

**Snare / 132 BPM.** Source: snare or clap hits including ghost notes and a flam

Send a damped offbeat repeat behind a dry snare.

EQ: LowCut 160 Hz / +0 dB / Q 0.707

Full range:

- HamloEcho (amount 0.18): echoMode=Score (1), quickEnabled=Off (0), scoreRouting=Send (1), scoreMix=1, scoreOutput=0, scoreSource=60, scoreFeedback=0.38, scoreFeedbackSync=1/8 D (6), scoreFeedbackMs=500, scoreFeedbackPitch=0, scoreColor=BBD (3), scoreDrive=0, scoreLowCut=700, scoreHighCut=4000, scoreWidth=1, scoreDuck=0.35, scoreFreeze=Off (0), scoreSpaceMix=0, scoreGlitch=Off (0), scoreGlitchAmount=0, scoreBands=Off (0), scoreSwing=0, tap1On=1, tap2On=0, tap3On=0, tap4On=0, tap5On=0, tap6On=0, tap7On=0, tap8On=0, tap1Sync=1/8 D (6), tap1Time=250, tap1Note=60, tap1Gain=-6, tap1Pan=0, tap1Shape=Forward (0), tap1Fragment=125, tap1Repeats=3, tap1Low=700, tap1High=4000, tap1SpaceSend=0

Reject/revise if: The repeat makes the next snare late or feedback grows each bar.

Intent check: Send a damped offbeat repeat behind a dry snare. Assess against: The repeat makes the next snare late or feedback grows each bar.. Original design-stage calibration status: **not_run**; final numerical results and individual owner-audition status are in RACK_BANK_REPORT.md.

### HR112 - Techno Hard Gate Burst

**Snare / 132 BPM.** Source: snare or clap hits including ghost notes and a flam

Add a short dense room burst behind a hard snare.

EQ: LowCut 130 Hz / +0 dB / Q 0.707; Bell 450 Hz / -1.5 dB / Q 1

Full range:

- HamloComp (amount 0.6): thresholdDb=-25, ratio=4, attackMs=8, releaseMs=80, model=FET (2), source=Drums (1), kneeDb=6, detector=RMS (1), character=Clean (0), autoMakeup=Off (0), makeupDb=0, mix=1
- HamloVerb (amount 0.14): space=Room (1), engine=Crisp (4), decaySeconds=0.9, predelayMs=10, predelaySync=Free (0), lowCutHz=500, highCutHz=6500, bassMult=0.75, hfDecayRatio=0.5, width=1.1, modDepth=0.1, duck=0.15, routing=Send (1), mix=1, keepDry=Off (0), wetDb=0, outputDb=0, earlyDb=0, lateDb=0, timeShape=Gated (1), shimmerAmount=0, freeze=Off (0), shapeTimeMs=110, gateReleaseMs=18

Reject/revise if: The gate clicks or its burst masks the initial crack.

Intent check: Add a short dense room burst behind a hard snare. Assess against: The gate clicks or its burst masks the initial crack.. Original design-stage calibration status: **not_run**; final numerical results and individual owner-audition status are in RACK_BANK_REPORT.md.

### HR113 - Techno Peak Open Hat Seat

**Hats / 132 BPM.** Source: closed/open hats, an accent and a dense sixteenth roll

Shorten overlong offbeat hats while keeping their high lift.

EQ: LowCut 650 Hz / +0 dB / Q 0.707; Bell 8000 Hz / +0 dB / Q 1 (dynamic -2 dB)

Full range:

- HamloUdarnik (amount 1): attackDb=0.5, sustainDb=-2, speed=0.75, style=PRAVDA (0), engine=V2 (1), protect=On (1), lookahead=Vostok (1), listen=Full (0), rise=1, decay=1, sensitivity=0.8, preserve=1, mix=1, outputDb=0
- HamloMag (amount 0.2): model=Tape (1), driveDb=3, mix=1, trimDb=0, oversampling=8x (3), autoGain=On (1), harmonics=0.35, tone=0, evenOdd=0, tapeBias=0.5, speed=1 (0), wow=0, hiss=0, hum=0, crackle=0, noiseFollow=Follow (1), punish=Off (0), thump=Off (0), steep=Off (0), sampler=Off (0), phoneBass=0, subProtectHz=0, dynamics=0, match=Off (0), delta=Off (0)

Reject/revise if: The open hat loses its offbeat accent or still masks the next kick.

Intent check: Shorten overlong offbeat hats while keeping their high lift. Assess against: The open hat loses its offbeat accent or still masks the next kick.. Original design-stage calibration status: **not_run**; final numerical results and individual owner-audition status are in RACK_BANK_REPORT.md.

### HR114 - Techno Hypnotic Metallic Dots

**Hats / 132 BPM.** Source: closed/open hats, an accent and a dense sixteenth roll

Keep a repetitive closed hat focused with subtle dark echoes.

EQ: LowCut 1100 Hz / +0 dB / Q 0.707; HighShelf 12500 Hz / -1 dB / Q 0.707

Full range:

- HamloEcho (amount 0.08): echoMode=Score (1), quickEnabled=Off (0), scoreRouting=Send (1), scoreMix=1, scoreOutput=0, scoreSource=60, scoreFeedback=0.08, scoreFeedbackSync=1/16 (2), scoreFeedbackMs=500, scoreFeedbackPitch=0, scoreColor=BBD (3), scoreDrive=0, scoreLowCut=1400, scoreHighCut=5000, scoreWidth=1, scoreDuck=0.2, scoreFreeze=Off (0), scoreSpaceMix=0, scoreGlitch=Off (0), scoreGlitchAmount=0, scoreBands=Off (0), scoreSwing=0, tap1On=1, tap2On=0, tap3On=0, tap4On=0, tap5On=0, tap6On=0, tap7On=0, tap8On=0, tap1Sync=1/16 (2), tap1Time=250, tap1Note=60, tap1Gain=-6, tap1Pan=0, tap1Shape=Forward (0), tap1Fragment=125, tap1Repeats=3, tap1Low=1400, tap1High=5000, tap1SpaceSend=0

Reject/revise if: Echoes replace the main tick or create a constant top layer.

Intent check: Keep a repetitive closed hat focused with subtle dark echoes. Assess against: Echoes replace the main tick or create a constant top layer.. Original design-stage calibration status: **not_run**; final numerical results and individual owner-audition status are in RACK_BANK_REPORT.md.

### HR115 - Techno Minimal Tick Transient

**Hats / 132 BPM.** Source: closed/open hats, an accent and a dense sixteenth roll

Remove body from a small machine tick without expanding stereo width.

EQ: LowCut 1200 Hz / +0 dB / Q 0.707; HighCut 14000 Hz / +0 dB / Q 0.707

Full range:

- HamloUdarnik (amount 1): attackDb=1, sustainDb=-2.5, speed=0.85, style=PRAVDA (0), engine=V2 (1), protect=On (1), lookahead=Vostok (1), listen=Full (0), rise=1, decay=1, sensitivity=0.8, preserve=1, mix=1, outputDb=0
- HamloFader (amount 1): engine=Modern (1), enabled=On (1), mode=Trim (0), gainDb=-0.5, targetDb=-18, mix=1, polarity=Off (0), mute=Off (0), solo=Off (0), inputDb=0, outputDb=0, leftDb=0, rightDb=0, matchedDb=0, peakGuard=On (1), matchCeiling=-1, strength=0.55, maxBoost=3, maxCut=6, floorDb=-55, holdMs=100, riseMs=250, fallMs=140, depthDb=0, rightPhase=0

Reject/revise if: The tick becomes a click artifact or vanishes on quiet passages.

Intent check: Remove body from a small machine tick without expanding stereo width. Assess against: The tick becomes a click artifact or vanishes on quiet passages.. Original design-stage calibration status: **not_run**; final numerical results and individual owner-audition status are in RACK_BANK_REPORT.md.

### HR116 - Techno Detroit Tape Air

**Hats / 132 BPM.** Source: closed/open hats, an accent and a dense sixteenth roll

Round brittle hats and keep a modest high-band stereo spread.

EQ: LowCut 800 Hz / +0 dB / Q 0.707

Full range:

- HamloMag (amount 0.3): model=Tape (1), driveDb=4, mix=1, trimDb=0, oversampling=8x (3), autoGain=On (1), harmonics=0.35, tone=-0.1, evenOdd=0, tapeBias=0.5, speed=1 (0), wow=0, hiss=0, hum=0, crackle=0, noiseFollow=Follow (1), punish=Off (0), thump=Off (0), steep=Off (0), sampler=Off (0), phoneBass=0, subProtectHz=0, dynamics=0, match=Off (0), delta=Off (0)
- HamloShirota (amount 1): v2Mode=Stereo / Space (1), v2Width=1, v2LowWidth=0, v2MidWidth=1, v2HighWidth=1.2, v2LowSplit=300, v2HighSplit=3000, v2MonoBass=300, v2Generate=0, v2Guard=On (1), v2Floor=0.15, v2Mix=1, v2Output=0, v2Motion=Off (0), v2PositionOn=Off (0), v2Mono=Stereo (0)

Reject/revise if: Mono loses timekeeping or tape tone dulls every accent.

Intent check: Round brittle hats and keep a modest high-band stereo spread. Assess against: Mono loses timekeeping or tape tone dulls every accent.. Original design-stage calibration status: **not_run**; final numerical results and individual owner-audition status are in RACK_BANK_REPORT.md.

### HR117 - Techno Hard Last Step Rush

**Hats / 132 BPM.** Source: closed/open hats, an accent and a dense sixteenth roll

Add one rising BLOCK burst at the phrase edge.

EQ: LowCut 1000 Hz / +0 dB / Q 0.707

Full range:

- HamloRevolution (amount 0.3): mix=1, free=Sync (0), grid=1/16 (2), triplet=Off (0), swing=0, humanize=0, fadeMs=3, order=T-G-M (0), scene=1 (0), listen=Off (0), timeOn=Off (0), glitchOn=On (1), motionOn=Off (0), timeMix=1, timeDepth=1, glitchMix=1, glitchDepth=0.3, motionMix=1, motionDepth=0.35, motionSmooth=0.5, motionTarget=Low Pass (1), timeStretch=Rhythm (1), glitchEngine=Block (1), shapeAOn=Off (0), shapeBOn=Off (0), throwAOn=Off (0), throwBOn=Off (0)
  - BLOCK GLITCH (steps 1-16): 15: Rise; 16: Roll

Reject/revise if: The rise repeats every early beat or the next downbeat is cut.

Intent check: Add one rising BLOCK burst at the phrase edge. Assess against: The rise repeats every early beat or the next downbeat is cut.. Original design-stage calibration status: **not_run**; final numerical results and individual owner-audition status are in RACK_BANK_REPORT.md.

### HR118 - Techno Hypnotic Tom Focus

**Percussion / 132 BPM.** Source: a sparse percussion loop with velocity variation

Keep low tom notes distinct and soften excessive stick bite.

EQ: LowCut 65 Hz / +0 dB / Q 0.707; Bell 1800 Hz / -1.5 dB / Q 1

Full range:

- HamloUdarnik (amount 1): attackDb=-0.5, sustainDb=-1.5, speed=0.65, style=PRAVDA (0), engine=V2 (1), protect=On (1), lookahead=Vostok (1), listen=Full (0), rise=1, decay=1, sensitivity=0.8, preserve=1, mix=1, outputDb=0
- HamloComp (amount 1): thresholdDb=-24, ratio=2, attackMs=22, releaseMs=114, model=VCA (1), source=All (0), kneeDb=6, detector=RMS (1), character=Clean (0), autoMakeup=Off (0), makeupDb=0, mix=1

Reject/revise if: The tom tail overlaps the next note or fundamental shifts.

Intent check: Keep low tom notes distinct and soften excessive stick bite. Assess against: The tom tail overlaps the next note or fundamental shifts.. Original design-stage calibration status: **not_run**; final numerical results and individual owner-audition status are in RACK_BANK_REPORT.md.

### HR119 - Techno Minimal Wood Reply

**Percussion / 132 BPM.** Source: a sparse percussion loop with velocity variation

Add a quiet short delay around a dry wooden hit.

EQ: LowCut 300 Hz / +0 dB / Q 0.707; Bell 900 Hz / +1 dB / Q 1

Full range:

- HamloEcho (amount 0.1): echoMode=Score (1), quickEnabled=Off (0), scoreRouting=Send (1), scoreMix=1, scoreOutput=0, scoreSource=60, scoreFeedback=0.12, scoreFeedbackSync=1/16 D (3), scoreFeedbackMs=500, scoreFeedbackPitch=0, scoreColor=Warm (1), scoreDrive=0, scoreLowCut=600, scoreHighCut=6000, scoreWidth=1, scoreDuck=0.2, scoreFreeze=Off (0), scoreSpaceMix=0, scoreGlitch=Off (0), scoreGlitchAmount=0, scoreBands=Off (0), scoreSwing=0, tap1On=1, tap2On=0, tap3On=0, tap4On=0, tap5On=0, tap6On=0, tap7On=0, tap8On=0, tap1Sync=1/16 D (3), tap1Time=250, tap1Note=60, tap1Gain=-6, tap1Pan=0, tap1Shape=Forward (0), tap1Fragment=125, tap1Repeats=3, tap1Low=600, tap1High=6000, tap1SpaceSend=0

Reject/revise if: The reply sounds like a flam on the original transient.

Intent check: Add a quiet short delay around a dry wooden hit. Assess against: The reply sounds like a flam on the original transient.. Original design-stage calibration status: **not_run**; final numerical results and individual owner-audition status are in RACK_BANK_REPORT.md.

### HR120 - Techno Dub Chord Perc Space

**Percussion / 132 BPM.** Source: a sparse percussion loop with velocity variation

Create a filtered ping-pong response with a stable dry hit.

EQ: LowCut 250 Hz / +0 dB / Q 0.707

Full range:

- HamloEcho (amount 0.2): echoMode=Score (1), quickEnabled=Off (0), scoreRouting=Send (1), scoreMix=1, scoreOutput=0, scoreSource=60, scoreFeedback=0.4, scoreFeedbackSync=1/8 D (6), scoreFeedbackMs=500, scoreFeedbackPitch=0, scoreColor=BBD (3), scoreDrive=0, scoreLowCut=500, scoreHighCut=4200, scoreWidth=1, scoreDuck=0.2, scoreFreeze=Off (0), scoreSpaceMix=0, scoreGlitch=Off (0), scoreGlitchAmount=0, scoreBands=Off (0), scoreSwing=0, tap1On=1, tap2On=1, tap3On=0, tap4On=0, tap5On=0, tap6On=0, tap7On=0, tap8On=0, tap1Sync=1/8 D (6), tap1Time=250, tap1Note=60, tap1Gain=-8, tap1Pan=-0.6, tap1Shape=Forward (0), tap1Fragment=125, tap1Repeats=3, tap1Low=500, tap1High=4200, tap1SpaceSend=0, tap2Sync=1/4 (8), tap2Time=500, tap2Note=60, tap2Gain=-13, tap2Pan=0.6, tap2Shape=Forward (0), tap2Fragment=125, tap2Repeats=3, tap2Low=500, tap2High=4200, tap2SpaceSend=0

Reject/revise if: The feedback creates low buildup or the dry center shifts.

Intent check: Create a filtered ping-pong response with a stable dry hit. Assess against: The feedback creates low buildup or the dry center shifts.. Original design-stage calibration status: **not_run**; final numerical results and individual owner-audition status are in RACK_BANK_REPORT.md.

### HR121 - Techno Industrial Mid Scrape

**Percussion / 132 BPM.** Source: a sparse percussion loop with velocity variation

Add parallel hard colour to metallic percussion without brittle treble.

EQ: LowCut 350 Hz / +0 dB / Q 0.707; HighCut 10000 Hz / +0 dB / Q 0.707

Full range:

- HamloMag (amount 0.25): model=Germanium (4), driveDb=14, mix=1, trimDb=0, oversampling=8x (3), autoGain=On (1), harmonics=0.35, tone=0, evenOdd=0, tapeBias=0.5, speed=1 (0), wow=0, hiss=0, hum=0, crackle=0, noiseFollow=Follow (1), punish=Off (0), thump=Off (0), steep=Off (0), sampler=Off (0), phoneBass=0, subProtectHz=0, dynamics=0, match=Off (0), delta=Off (0)
- HamloComp (amount 0.4): thresholdDb=-27, ratio=4, attackMs=6, releaseMs=65, model=FET (2), source=Drums (1), kneeDb=6, detector=RMS (1), character=Clean (0), autoMakeup=Off (0), makeupDb=0, mix=1

Reject/revise if: The upper band turns into hiss or the clean metal pitch disappears.

Intent check: Add parallel hard colour to metallic percussion without brittle treble. Assess against: The upper band turns into hiss or the clean metal pitch disappears.. Original design-stage calibration status: **not_run**; final numerical results and individual owner-audition status are in RACK_BANK_REPORT.md.

### HR122 - Techno Peak Quarter Glue

**Drum Bus / 132 BPM.** Source: a stereo unprocessed drum loop with kick, snare, hats and ghost notes

Let the compressor recover inside a quarter while keeping the kick front.

EQ: LowCut 25 Hz / +0 dB / Q 0.707; Bell 320 Hz / -0.7 dB / Q 1

Full range:

- HamloComp (amount 1): thresholdDb=-22, ratio=2, attackMs=28, releaseMs=114, model=VCA (1), source=Drums (1), kneeDb=6, detector=RMS (1), character=Clean (0), autoMakeup=Off (0), makeupDb=0, mix=1
- HamloMag (amount 0.2): model=Iron (3), driveDb=3, mix=1, trimDb=0, oversampling=8x (3), autoGain=On (1), harmonics=0.35, tone=0, evenOdd=0, tapeBias=0.5, speed=1 (0), wow=0, hiss=0, hum=0, crackle=0, noiseFollow=Follow (1), punish=Off (0), thump=Off (0), steep=Off (0), sampler=Off (0), phoneBass=0, subProtectHz=0, dynamics=0, match=Off (0), delta=Off (0)

Reject/revise if: The detector pumps the hats excessively or all kicks flatten.

Intent check: Let the compressor recover inside a quarter while keeping the kick front. Assess against: The detector pumps the hats excessively or all kicks flatten.. Original design-stage calibration status: **not_run**; final numerical results and individual owner-audition status are in RACK_BANK_REPORT.md.

### HR123 - Techno Hypnotic Tape Bed

**Drum Bus / 132 BPM.** Source: a stereo unprocessed drum loop with kick, snare, hats and ghost notes

Bind a rolling machine beat with modest tape density.

EQ: LowCut 26 Hz / +0 dB / Q 0.707; HighShelf 12000 Hz / -0.5 dB / Q 0.707

Full range:

- HamloMag (amount 0.35): model=Tape (1), driveDb=4, mix=1, trimDb=0, oversampling=8x (3), autoGain=On (1), harmonics=0.35, tone=0, evenOdd=0, tapeBias=0.5, speed=1 (0), wow=0, hiss=0, hum=0, crackle=0, noiseFollow=Follow (1), punish=Off (0), thump=Off (0), steep=Off (0), sampler=Off (0), phoneBass=0, subProtectHz=0, dynamics=0, match=Off (0), delta=Off (0)
- HamloComp (amount 1): thresholdDb=-23, ratio=1.8, attackMs=30, releaseMs=170, model=Opto (3), source=Drums (1), kneeDb=6, detector=RMS (1), character=Clean (0), autoMakeup=Off (0), makeupDb=0, mix=1

Reject/revise if: Loop accents lose their hierarchy or low end turns woolly.

Intent check: Bind a rolling machine beat with modest tape density. Assess against: Loop accents lose their hierarchy or low end turns woolly.. Original design-stage calibration status: **not_run**; final numerical results and individual owner-audition status are in RACK_BANK_REPORT.md.

### HR124 - Techno Minimal Clean Space

**Drum Bus / 132 BPM.** Source: a stereo unprocessed drum loop with kick, snare, hats and ghost notes

Control the envelope lightly while preserving sparse gaps.

EQ: LowCut 28 Hz / +0 dB / Q 0.707

Full range:

- HamloComp (amount 1): thresholdDb=-20, ratio=1.5, attackMs=35, releaseMs=130, model=VCA (1), source=Drums (1), kneeDb=6, detector=RMS (1), character=Clean (0), autoMakeup=Off (0), makeupDb=0, mix=1
- HamloVerb (amount 0.035): space=Ambience (6), engine=Clean (1), decaySeconds=0.25, predelayMs=3, predelaySync=Free (0), lowCutHz=900, highCutHz=6500, bassMult=0.75, hfDecayRatio=0.5, width=1.1, modDepth=0.1, duck=0.15, routing=Send (1), mix=1, keepDry=Off (0), wetDb=0, outputDb=0, earlyDb=0, lateDb=0, timeShape=Off (0), shimmerAmount=0, freeze=Off (0)

Reject/revise if: Gate noise replaces the gaps or the hats sound closer than the clap.

Intent check: Control the envelope lightly while preserving sparse gaps. Assess against: Gate noise replaces the gaps or the hats sound closer than the clap.. Original design-stage calibration status: **not_run**; final numerical results and individual owner-audition status are in RACK_BANK_REPORT.md.

### HR125 - Techno Detroit Parallel Circuit

**Drum Bus / 132 BPM.** Source: a stereo unprocessed drum loop with kick, snare, hats and ghost notes

Add an energetic mid-band parallel layer beneath clean machine transients.

EQ: LowCut 26 Hz / +0 dB / Q 0.707

Full range:

- HamloComp (amount 0.25): thresholdDb=-29, ratio=6, attackMs=15, releaseMs=100, model=FET (2), source=Drums (1), kneeDb=6, detector=RMS (1), character=Clean (0), autoMakeup=Off (0), makeupDb=0, mix=1

Band 170-6000 Hz:

- HamloMag (amount 0.35): model=Iron (3), driveDb=8, mix=1, trimDb=0, oversampling=8x (3), autoGain=On (1), harmonics=0.35, tone=0, evenOdd=0, tapeBias=0.5, speed=1 (0), wow=0, hiss=0, hum=0, crackle=0, noiseFollow=Follow (1), punish=Off (0), thump=Off (0), steep=Off (0), sampler=Off (0), phoneBass=0, subProtectHz=0, dynamics=0, match=Off (0), delta=Off (0)

Reject/revise if: The snare crest falls or the layer muddies the kick-bass pocket.

Intent check: Add an energetic mid-band parallel layer beneath clean machine transients. Assess against: The snare crest falls or the layer muddies the kick-bass pocket.. Original design-stage calibration status: **not_run**; final numerical results and individual owner-audition status are in RACK_BANK_REPORT.md.

### HR126 - Techno Dub Tops Feedback

**Drum Bus / 132 BPM.** Source: a stereo unprocessed drum loop with kick, snare, hats and ghost notes

Create a dark upper-band delay bed while the low rhythm stays dry.

EQ: LowCut 25 Hz / +0 dB / Q 0.707

Full range:

- HamloComp (amount 1): thresholdDb=-22, ratio=1.7, attackMs=30, releaseMs=150, model=VCA (1), source=Drums (1), kneeDb=6, detector=RMS (1), character=Clean (0), autoMakeup=Off (0), makeupDb=0, mix=1

Band 700-12000 Hz:

- HamloEcho (amount 0.12): echoMode=Score (1), quickEnabled=Off (0), scoreRouting=Send (1), scoreMix=1, scoreOutput=0, scoreSource=60, scoreFeedback=0.32, scoreFeedbackSync=1/8 D (6), scoreFeedbackMs=500, scoreFeedbackPitch=0, scoreColor=BBD (3), scoreDrive=0, scoreLowCut=800, scoreHighCut=4000, scoreWidth=1, scoreDuck=0.2, scoreFreeze=Off (0), scoreSpaceMix=0, scoreGlitch=Off (0), scoreGlitchAmount=0, scoreBands=Off (0), scoreSwing=0, tap1On=1, tap2On=0, tap3On=0, tap4On=0, tap5On=0, tap6On=0, tap7On=0, tap8On=0, tap1Sync=1/8 D (6), tap1Time=250, tap1Note=60, tap1Gain=-6, tap1Pan=0, tap1Shape=Forward (0), tap1Fragment=125, tap1Repeats=3, tap1Low=800, tap1High=4000, tap1SpaceSend=0

Reject/revise if: The low kick is echoed or feedback outlasts the intended phrase gaps.

Intent check: Create a dark upper-band delay bed while the low rhythm stays dry. Assess against: The low kick is echoed or feedback outlasts the intended phrase gaps.. Original design-stage calibration status: **not_run**; final numerical results and individual owner-audition status are in RACK_BANK_REPORT.md.

### HR127 - Techno Hard Controlled Density

**Drum Bus / 132 BPM.** Source: a stereo unprocessed drum loop with kick, snare, hats and ghost notes

Increase parallel drum density with protected transient contrast.

EQ: LowCut 28 Hz / +0 dB / Q 0.707; Bell 400 Hz / -1 dB / Q 1

Full range:

- HamloUdarnik (amount 0.6): attackDb=1.2, sustainDb=-0.5, speed=0.8, style=UDARNIK (1), engine=V2 (1), protect=On (1), lookahead=Vostok (1), listen=Full (0), rise=1, decay=1, sensitivity=0.8, preserve=1, mix=1, outputDb=0
- HamloMag (amount 0.18): model=Germanium (4), driveDb=8, mix=1, trimDb=0, oversampling=8x (3), autoGain=On (1), harmonics=0.35, tone=0, evenOdd=0, tapeBias=0.5, speed=1 (0), wow=0, hiss=0, hum=0, crackle=0, noiseFollow=Follow (1), punish=Off (0), thump=Off (0), steep=Off (0), sampler=Off (0), phoneBass=0, subProtectHz=0, dynamics=0, match=Off (0), delta=Off (0)
- HamloComp (amount 0.3): thresholdDb=-27, ratio=4, attackMs=15, releaseMs=90, model=FET (2), source=Drums (1), kneeDb=6, detector=RMS (1), character=Clean (0), autoMakeup=Off (0), makeupDb=0, mix=1

Reject/revise if: The matched beat loses attack/body contrast or master peaks jump above plan.

Intent check: Increase parallel drum density with protected transient contrast. Assess against: The matched beat loses attack/body contrast or master peaks jump above plan.. Original design-stage calibration status: **not_run**; final numerical results and individual owner-audition status are in RACK_BANK_REPORT.md.

### HR128 - Techno Phrase Tops Shuffle

**Drum Bus / 132 BPM.** Source: a stereo unprocessed drum loop with kick, snare, hats and ghost notes

Shuffle the final upper-drum fragments while preserving the four-on-floor.

EQ: LowCut 25 Hz / +0 dB / Q 0.707

Full range:

- HamloComp (amount 1): thresholdDb=-22, ratio=1.8, attackMs=25, releaseMs=115, model=VCA (1), source=Drums (1), kneeDb=6, detector=RMS (1), character=Clean (0), autoMakeup=Off (0), makeupDb=0, mix=1

Band 900-16000 Hz:

- HamloRevolution (amount 0.35): mix=1, free=Sync (0), grid=1/16 (2), triplet=Off (0), swing=0, humanize=0, fadeMs=3, order=T-G-M (0), scene=1 (0), listen=Off (0), timeOn=Off (0), glitchOn=On (1), motionOn=Off (0), timeMix=1, timeDepth=1, glitchMix=1, glitchDepth=0.5, motionMix=1, motionDepth=0.35, motionSmooth=0.5, motionTarget=Low Pass (1), timeStretch=Rhythm (1), glitchEngine=Block (1), shapeAOn=Off (0), shapeBOn=Off (0), throwAOn=Off (0), throwBOn=Off (0)
  - BLOCK GLITCH (steps 1-16): 13: Shuffle; 15: Repeat; 16: Roll

Reject/revise if: The kick timing changes or the next bar lacks its first hat.

Intent check: Shuffle the final upper-drum fragments while preserving the four-on-floor. Assess against: The kick timing changes or the next bar lacks its first hat.. Original design-stage calibration status: **not_run**; final numerical results and individual owner-audition status are in RACK_BANK_REPORT.md.

### HR129 - Techno Peak Rolling Bass

**Bass / 132 BPM.** Source: a monophonic bass phrase with low and high notes and sustained tails

Level a rolling line without blurring its sixteenth-note spaces.

EQ: LowCut 28 Hz / +0 dB / Q 0.707; Bell 280 Hz / -1.5 dB / Q 1

Full range:

- HamloComp (amount 1): thresholdDb=-24, ratio=3, attackMs=18, releaseMs=90, model=VCA (1), source=All (0), kneeDb=6, detector=RMS (1), character=Clean (0), autoMakeup=Off (0), makeupDb=0, mix=1
- HamloMag (amount 0.3): model=Iron (3), driveDb=4, mix=1, trimDb=0, oversampling=8x (3), autoGain=On (1), harmonics=0.35, tone=0, evenOdd=0, tapeBias=0.5, speed=1 (0), wow=0, hiss=0, hum=0, crackle=0, noiseFollow=Follow (1), punish=Off (0), thump=Off (0), steep=Off (0), sampler=Off (0), phoneBass=0, subProtectHz=80, dynamics=0, match=Off (0), delta=Off (0)

Reject/revise if: The bass sustains into the kick or loudness matching reveals less articulation.

Intent check: Level a rolling line without blurring its sixteenth-note spaces. Assess against: The bass sustains into the kick or loudness matching reveals less articulation.. Original design-stage calibration status: **not_run**; final numerical results and individual owner-audition status are in RACK_BANK_REPORT.md.

### HR130 - Techno Hypnotic Tempo Dip

**Bass / 132 BPM.** Source: a monophonic bass phrase with low and high notes and sustained tails

Cut a gentle tempo-shaped pocket at the start of each quarter.

EQ: LowCut 25 Hz / +0 dB / Q 0.707

Full range:

- HamloFader (amount 1): engine=Modern (1), enabled=On (1), mode=Motion (3), gainDb=0, targetDb=-18, mix=1, polarity=Off (0), mute=Off (0), solo=Off (0), inputDb=0, outputDb=0, leftDb=0, rightDb=0, matchedDb=0, peakGuard=On (1), matchCeiling=-1, strength=0.55, maxBoost=3, maxCut=6, floorDb=-55, holdMs=100, riseMs=250, fallMs=140, depthDb=6, rightPhase=0, shape=Custom (0), points=4, sync=Sync (1), beats=1, edgeMs=3, smooth=Smooth (1), level00=0, level01=0.35, level02=0.85, level03=1
- HamloMag (amount 0.25): model=Tape (1), driveDb=4, mix=1, trimDb=0, oversampling=8x (3), autoGain=On (1), harmonics=0.35, tone=0, evenOdd=0, tapeBias=0.5, speed=1 (0), wow=0, hiss=0, hum=0, crackle=0, noiseFollow=Follow (1), punish=Off (0), thump=Off (0), steep=Off (0), sampler=Off (0), phoneBass=0, subProtectHz=70, dynamics=0, match=Off (0), delta=Off (0)

Reject/revise if: The dip misses the grid or the bass fails to recover before the offbeat.

Intent check: Cut a gentle tempo-shaped pocket at the start of each quarter. Assess against: The dip misses the grid or the bass fails to recover before the offbeat.. Original design-stage calibration status: **not_run**; final numerical results and individual owner-audition status are in RACK_BANK_REPORT.md.

### HR131 - Techno Acid Upper Bite

**Bass / 132 BPM.** Source: a monophonic bass phrase with low and high notes and sustained tails

Add controlled resonance-band grit above an undriven sub.

EQ: LowCut 30 Hz / +0 dB / Q 0.707; Bell 3200 Hz / +0 dB / Q 1.8 (dynamic -2 dB)

Full range:

- HamloShirota (amount 1): v2Mode=Stereo / Space (1), v2Width=1, v2LowWidth=0, v2MidWidth=1, v2HighWidth=1, v2LowSplit=300, v2HighSplit=3000, v2MonoBass=140, v2Generate=0, v2Guard=On (1), v2Floor=0.15, v2Mix=1, v2Output=0, v2Motion=Off (0), v2PositionOn=Off (0), v2Mono=Stereo (0)

Band 180-7000 Hz:

- HamloMag (amount 0.35): model=Germanium (4), driveDb=9, mix=1, trimDb=0, oversampling=8x (3), autoGain=On (1), harmonics=0.35, tone=0, evenOdd=0, tapeBias=0.5, speed=1 (0), wow=0, hiss=0, hum=0, crackle=0, noiseFollow=Follow (1), punish=Off (0), thump=Off (0), steep=Off (0), sampler=Off (0), phoneBass=0, subProtectHz=0, dynamics=0, match=Off (0), delta=Off (0)
- HamloComp (amount 1): thresholdDb=-25, ratio=3, attackMs=6, releaseMs=85, model=FET (2), source=All (0), kneeDb=6, detector=RMS (1), character=Clean (0), autoMakeup=Off (0), makeupDb=0, mix=1

Reject/revise if: The filter peak tears or the low fundamental is saturated.

Intent check: Add controlled resonance-band grit above an undriven sub. Assess against: The filter peak tears or the low fundamental is saturated.. Original design-stage calibration status: **not_run**; final numerical results and individual owner-audition status are in RACK_BANK_REPORT.md.

### HR132 - Techno Dub Round Sustain

**Bass / 132 BPM.** Source: a monophonic bass phrase with low and high notes and sustained tails

Smooth a soft dub-bass line without generating extra low tails.

EQ: LowCut 24 Hz / +0 dB / Q 0.707; HighCut 4000 Hz / +0 dB / Q 0.707

Full range:

- HamloComp (amount 1): thresholdDb=-25, ratio=2.5, attackMs=30, releaseMs=210, model=Opto (3), source=All (0), kneeDb=6, detector=RMS (1), character=Clean (0), autoMakeup=Off (0), makeupDb=0, mix=1
- HamloMag (amount 0.3): model=Tube (2), driveDb=4, mix=1, trimDb=0, oversampling=8x (3), autoGain=On (1), harmonics=0.35, tone=0, evenOdd=0, tapeBias=0.5, speed=1 (0), wow=0, hiss=0, hum=0, crackle=0, noiseFollow=Follow (1), punish=Off (0), thump=Off (0), steep=Off (0), sampler=Off (0), phoneBass=0, subProtectHz=65, dynamics=0, match=Off (0), delta=Off (0)

Reject/revise if: Notes swell after their attack or low sub level becomes inconsistent.

Intent check: Smooth a soft dub-bass line without generating extra low tails. Assess against: Notes swell after their attack or low sub level becomes inconsistent.. Original design-stage calibration status: **not_run**; final numerical results and individual owner-audition status are in RACK_BANK_REPORT.md.

### HR133 - Techno Hard Mono Grind

**Bass / 132 BPM.** Source: a monophonic bass phrase with low and high notes and sustained tails

Add a compact mid grind with mono low end and short releases.

EQ: LowCut 28 Hz / +0 dB / Q 0.707

Full range:

- HamloMag (amount 0.4): model=Iron (3), driveDb=9, mix=1, trimDb=0, oversampling=8x (3), autoGain=On (1), harmonics=0.35, tone=0, evenOdd=0, tapeBias=0.5, speed=1 (0), wow=0, hiss=0, hum=0, crackle=0, noiseFollow=Follow (1), punish=Off (0), thump=Off (0), steep=Off (0), sampler=Off (0), phoneBass=0.1, subProtectHz=100, dynamics=0, match=Off (0), delta=Off (0)
- HamloComp (amount 1): thresholdDb=-24, ratio=3.5, attackMs=15, releaseMs=85, model=VCA (1), source=All (0), kneeDb=6, detector=RMS (1), character=Clean (0), autoMakeup=Off (0), makeupDb=0, mix=1
- HamloShirota (amount 1): v2Mode=Stereo / Space (1), v2Width=1, v2LowWidth=0, v2MidWidth=1, v2HighWidth=1.05, v2LowSplit=300, v2HighSplit=3000, v2MonoBass=160, v2Generate=0, v2Guard=On (1), v2Floor=0.15, v2Mix=1, v2Output=0, v2Motion=Off (0), v2PositionOn=Off (0), v2Mono=Stereo (0)

Reject/revise if: Stereo artifacts appear below 150 Hz or the bass overshadows the kick click.

Intent check: Add a compact mid grind with mono low end and short releases. Assess against: Stereo artifacts appear below 150 Hz or the bass overshadows the kick click.. Original design-stage calibration status: **not_run**; final numerical results and individual owner-audition status are in RACK_BANK_REPORT.md.

### HR134 - Techno Spoken Close Command

**Vocal / 132 BPM.** Source: a dry lead phrase with plosives, sibilance and a quiet ending

Keep a spoken phrase dry, focused and stable in front of a loud beat.

EQ: LowCut 130 Hz / +0 dB / Q 0.707; Bell 450 Hz / -2 dB / Q 1; Bell 6500 Hz / +0 dB / Q 1 (dynamic -2.5 dB)

Full range:

- HamloFader (amount 1): engine=Modern (1), enabled=On (1), mode=Ride (2), gainDb=0, targetDb=-18, mix=1, polarity=Off (0), mute=Off (0), solo=Off (0), inputDb=0, outputDb=0, leftDb=0, rightDb=0, matchedDb=0, peakGuard=On (1), matchCeiling=-1, strength=0.4, maxBoost=2, maxCut=6, floorDb=-55, holdMs=100, riseMs=250, fallMs=140, depthDb=0, rightPhase=0
- HamloComp (amount 1): thresholdDb=-25, ratio=3, attackMs=7, releaseMs=100, model=FET (2), source=Voc (3), kneeDb=6, detector=RMS (1), character=Clean (0), autoMakeup=Off (0), makeupDb=0, mix=1

Reject/revise if: Breaths surge between commands or the bandpass destroys intelligibility.

Intent check: Keep a spoken phrase dry, focused and stable in front of a loud beat. Assess against: Breaths surge between commands or the bandpass destroys intelligibility.. Original design-stage calibration status: **not_run**; final numerical results and individual owner-audition status are in RACK_BANK_REPORT.md.

### HR135 - Techno Dub Word Echo

**Vocal / 132 BPM.** Source: a dry lead phrase with plosives, sibilance and a quiet ending

Let a damped dotted echo emerge between spoken words.

EQ: LowCut 180 Hz / +0 dB / Q 0.707; HighCut 10000 Hz / +0 dB / Q 0.707

Full range:

- HamloComp (amount 1): thresholdDb=-23, ratio=2.5, attackMs=10, releaseMs=130, model=VCA (1), source=Voc (3), kneeDb=6, detector=RMS (1), character=Clean (0), autoMakeup=Off (0), makeupDb=0, mix=1
- HamloEcho (amount 0.2): echoMode=Score (1), quickEnabled=Off (0), scoreRouting=Send (1), scoreMix=1, scoreOutput=0, scoreSource=60, scoreFeedback=0.38, scoreFeedbackSync=1/8 D (6), scoreFeedbackMs=500, scoreFeedbackPitch=0, scoreColor=BBD (3), scoreDrive=0, scoreLowCut=650, scoreHighCut=4500, scoreWidth=1, scoreDuck=0.6, scoreFreeze=Off (0), scoreSpaceMix=0, scoreGlitch=Off (0), scoreGlitchAmount=0, scoreBands=Off (0), scoreSwing=0, tap1On=1, tap2On=0, tap3On=0, tap4On=0, tap5On=0, tap6On=0, tap7On=0, tap8On=0, tap1Sync=1/8 D (6), tap1Time=250, tap1Note=60, tap1Gain=-6, tap1Pan=0, tap1Shape=Forward (0), tap1Fragment=125, tap1Repeats=3, tap1Low=650, tap1High=4500, tap1SpaceSend=0

Reject/revise if: Repeats obscure commands or build endlessly at phrase boundaries.

Intent check: Let a damped dotted echo emerge between spoken words. Assess against: Repeats obscure commands or build endlessly at phrase boundaries.. Original design-stage calibration status: **not_run**; final numerical results and individual owner-audition status are in RACK_BANK_REPORT.md.

### HR136 - Techno Gated Chant

**Vocal / 132 BPM.** Source: a dry lead phrase with plosives, sibilance and a quiet ending

Shape a held chant into a repeating syncopated figure.

EQ: LowCut 160 Hz / +0 dB / Q 0.707; Bell 7000 Hz / +0 dB / Q 1 (dynamic -3 dB)

Full range:

- HamloZastava (amount 0.75): v2_engine=Rhythm (4), v2_enabled=On (1), v2_threshold=-40, v2_range=24, v2_hold=0, v2_release=8, v2_attack=2, v2_hysteresis=4, v2_knee=6, v2_external=Main (0), v2_expand=Expand (1), v2_ratio=2, v2_link=1, v2_mix=1, v2_output=0, v2_body=0, v2_tail=8, v2_lookahead=2, v2_hp=0, v2_lp=0, v2_listen=Off (0), v2_steps=16, v2_beats=0.25, v2_sync=Tempo (1), v2_step1=1, v2_step2=1, v2_step3=0, v2_step4=0.4, v2_step5=1, v2_step6=0, v2_step7=0.6, v2_step8=0, v2_step9=1, v2_step10=0.7, v2_step11=0, v2_step12=0, v2_step13=1, v2_step14=0, v2_step15=0.7, v2_step16=0, v2_step17=1, v2_step18=1, v2_step19=0, v2_step20=0.4, v2_step21=1, v2_step22=0, v2_step23=0.6, v2_step24=0, v2_step25=1, v2_step26=0.7, v2_step27=0, v2_step28=0, v2_step29=1, v2_step30=0, v2_step31=0.7, v2_step32=0
- HamloVerb (amount 0.07): space=Room (1), engine=Crisp (4), decaySeconds=0.4, predelayMs=8, predelaySync=Free (0), lowCutHz=600, highCutHz=5500, bassMult=0.75, hfDecayRatio=0.5, width=1.1, modDepth=0.1, duck=0.15, routing=Send (1), mix=1, keepDry=Off (0), wetDb=0, outputDb=0, earlyDb=0, lateDb=0, timeShape=Off (0), shimmerAmount=0, freeze=Off (0)

Reject/revise if: Gate edges click or consonants become impossible to identify.

Intent check: Shape a held chant into a repeating syncopated figure. Assess against: Gate edges click or consonants become impossible to identify.. Original design-stage calibration status: **not_run**; final numerical results and individual owner-audition status are in RACK_BANK_REPORT.md.

### HR137 - Techno Warehouse Call

**Vocal / 132 BPM.** Source: a dry lead phrase with plosives, sibilance and a quiet ending

Put a clear dry call ahead of a ducked dark warehouse tail.

EQ: LowCut 140 Hz / +0 dB / Q 0.707; Bell 300 Hz / -1.5 dB / Q 1

Full range:

- HamloComp (amount 1): thresholdDb=-25, ratio=3, attackMs=10, releaseMs=115, model=FET (2), source=Voc (3), kneeDb=6, detector=RMS (1), character=Clean (0), autoMakeup=Off (0), makeupDb=0, mix=1
- HamloVerb (amount 0.19): space=Hall (3), engine=Deep (5), decaySeconds=1.8, predelayMs=45, predelaySync=Free (0), lowCutHz=550, highCutHz=5000, bassMult=0.75, hfDecayRatio=0.5, width=1.1, modDepth=0.1, duck=0.55, routing=Send (1), mix=1, keepDry=Off (0), wetDb=0, outputDb=0, earlyDb=0, lateDb=0, timeShape=Off (0), shimmerAmount=0, freeze=Off (0)

Reject/revise if: The tail masks the next call or adds low-mid boom.

Intent check: Put a clear dry call ahead of a ducked dark warehouse tail. Assess against: The tail masks the next call or adds low-mid boom.. Original design-stage calibration status: **not_run**; final numerical results and individual owner-audition status are in RACK_BANK_REPORT.md.

### HR138 - Techno Last Word Throw Lane

**Vocal / 132 BPM.** Source: a dry lead phrase with plosives, sibilance and a quiet ending

Send only the final two steps to a Revolution echo throw.

EQ: LowCut 150 Hz / +0 dB / Q 0.707; Bell 6000 Hz / +0 dB / Q 1 (dynamic -2 dB)

Full range:

- HamloRevolution (amount 0.4): mix=1, free=Sync (0), grid=1/16 (2), triplet=Off (0), swing=0, humanize=0, fadeMs=3, order=T-G-M (0), scene=1 (0), listen=Off (0), timeOn=Off (0), glitchOn=Off (0), motionOn=Off (0), timeMix=1, timeDepth=1, glitchMix=1, glitchDepth=0.5, motionMix=1, motionDepth=0.35, motionSmooth=0.5, motionTarget=Low Pass (1), timeStretch=Rhythm (1), glitchEngine=Block (1), shapeAOn=Off (0), shapeBOn=Off (0), throwAOn=On (1), throwBOn=Off (0), throwATarget=Delay (0), throwAMix=0.3, throwATail=0.25, throwATime=1/8 (5)
  - THROW A (steps 1-16): 15: 10; 16: 15

Reject/revise if: The whole phrase is repeated or the throw has a pitched leading artifact.

Intent check: Send only the final two steps to a Revolution echo throw. Assess against: The whole phrase is repeated or the throw has a pitched leading artifact.. Original design-stage calibration status: **not_run**; final numerical results and individual owner-audition status are in RACK_BANK_REPORT.md.

### HR139 - Techno Detroit Chord Punch

**Keys / 132 BPM.** Source: a dry chord phrase with staccato and held notes

Keep a chord stab short, warm and distinct from the kick.

EQ: LowCut 180 Hz / +0 dB / Q 0.707; Bell 450 Hz / -1.5 dB / Q 1

Full range:

- HamloUdarnik (amount 1): attackDb=1, sustainDb=-1.5, speed=0.7, style=PRAVDA (0), engine=V2 (1), protect=On (1), lookahead=Vostok (1), listen=Full (0), rise=1, decay=1, sensitivity=0.8, preserve=1, mix=1, outputDb=0
- HamloMag (amount 0.3): model=Iron (3), driveDb=5, mix=1, trimDb=0, oversampling=8x (3), autoGain=On (1), harmonics=0.35, tone=0, evenOdd=0, tapeBias=0.5, speed=1 (0), wow=0, hiss=0, hum=0, crackle=0, noiseFollow=Follow (1), punish=Off (0), thump=Off (0), steep=Off (0), sampler=Off (0), phoneBass=0, subProtectHz=0, dynamics=0, match=Off (0), delta=Off (0)
- HamloComp (amount 1): thresholdDb=-24, ratio=2, attackMs=18, releaseMs=115, model=VCA (1), source=Samples (2), kneeDb=6, detector=RMS (1), character=Clean (0), autoMakeup=Off (0), makeupDb=0, mix=1

Reject/revise if: Chord body spills into the next stab or the top becomes fuzzy.

Intent check: Keep a chord stab short, warm and distinct from the kick. Assess against: Chord body spills into the next stab or the top becomes fuzzy.. Original design-stage calibration status: **not_run**; final numerical results and individual owner-audition status are in RACK_BANK_REPORT.md.

### HR140 - Techno Dub Chord Tunnel

**Keys / 132 BPM.** Source: a dry chord phrase with staccato and held notes

Create a damped syncopated chord echo with a small chamber return.

EQ: LowCut 220 Hz / +0 dB / Q 0.707

Full range:

- HamloEcho (amount 0.2): echoMode=Score (1), quickEnabled=Off (0), scoreRouting=Send (1), scoreMix=1, scoreOutput=0, scoreSource=60, scoreFeedback=0.42, scoreFeedbackSync=1/8 D (6), scoreFeedbackMs=500, scoreFeedbackPitch=0, scoreColor=BBD (3), scoreDrive=0, scoreLowCut=450, scoreHighCut=4000, scoreWidth=1, scoreDuck=0.2, scoreFreeze=Off (0), scoreSpaceMix=0, scoreGlitch=Off (0), scoreGlitchAmount=0, scoreBands=Off (0), scoreSwing=0, tap1On=1, tap2On=0, tap3On=0, tap4On=0, tap5On=0, tap6On=0, tap7On=0, tap8On=0, tap1Sync=1/8 D (6), tap1Time=250, tap1Note=60, tap1Gain=-6, tap1Pan=0, tap1Shape=Forward (0), tap1Fragment=125, tap1Repeats=3, tap1Low=450, tap1High=4000, tap1SpaceSend=0
- HamloVerb (amount 0.08): space=Chamber (2), engine=Warm (2), decaySeconds=0.9, predelayMs=16, predelaySync=Free (0), lowCutHz=600, highCutHz=5000, bassMult=0.75, hfDecayRatio=0.5, width=1.1, modDepth=0.1, duck=0.15, routing=Send (1), mix=1, keepDry=Off (0), wetDb=0, outputDb=0, earlyDb=0, lateDb=0, timeShape=Off (0), shimmerAmount=0, freeze=Off (0)

Reject/revise if: Low repeats accumulate or the dry chord loses its placement.

Intent check: Create a damped syncopated chord echo with a small chamber return. Assess against: Low repeats accumulate or the dry chord loses its placement.. Original design-stage calibration status: **not_run**; final numerical results and individual owner-audition status are in RACK_BANK_REPORT.md.

### HR141 - Techno Minimal Pluck Seat

**Keys / 132 BPM.** Source: a dry chord phrase with staccato and held notes

Tuck bright plucks behind the rhythm with restrained early reflections.

EQ: LowCut 200 Hz / +0 dB / Q 0.707; Bell 5000 Hz / +0 dB / Q 1.4 (dynamic -2 dB)

Full range:

- HamloUdarnik (amount 1): attackDb=-0.5, sustainDb=-0.5, speed=0.7, style=PRAVDA (0), engine=V2 (1), protect=On (1), lookahead=Vostok (1), listen=Full (0), rise=1, decay=1, sensitivity=0.8, preserve=1, mix=1, outputDb=0
- HamloVerb (amount 0.06): space=Ambience (6), engine=Clean (1), decaySeconds=0.3, predelayMs=5, predelaySync=Free (0), lowCutHz=800, highCutHz=7000, bassMult=0.75, hfDecayRatio=0.5, width=1.1, modDepth=0.1, duck=0.15, routing=Send (1), mix=1, keepDry=Off (0), wetDb=0, outputDb=0, earlyDb=0, lateDb=0, timeShape=Off (0), shimmerAmount=0, freeze=Off (0)

Reject/revise if: The plucks become diffuse or sharp accents still dominate.

Intent check: Tuck bright plucks behind the rhythm with restrained early reflections. Assess against: The plucks become diffuse or sharp accents still dominate.. Original design-stage calibration status: **not_run**; final numerical results and individual owner-audition status are in RACK_BANK_REPORT.md.

### HR142 - Techno Peak Chord Breathing

**Keys / 132 BPM.** Source: a dry chord phrase with staccato and held notes

Give a sustained chord a gentle beat-linked dip and recovery.

EQ: LowCut 160 Hz / +0 dB / Q 0.707; Bell 350 Hz / -1.5 dB / Q 1

Full range:

- HamloFader (amount 1): engine=Modern (1), enabled=On (1), mode=Motion (3), gainDb=0, targetDb=-18, mix=1, polarity=Off (0), mute=Off (0), solo=Off (0), inputDb=0, outputDb=0, leftDb=0, rightDb=0, matchedDb=0, peakGuard=On (1), matchCeiling=-1, strength=0.55, maxBoost=3, maxCut=6, floorDb=-55, holdMs=100, riseMs=250, fallMs=140, depthDb=5, rightPhase=0, shape=Custom (0), points=4, sync=Sync (1), beats=1, edgeMs=3, smooth=Smooth (1), level00=0, level01=0.55, level02=1, level03=1
- HamloShirota (amount 1): v2Mode=Stereo / Space (1), v2Width=1, v2LowWidth=0, v2MidWidth=1.1, v2HighWidth=1.2, v2LowSplit=300, v2HighSplit=3000, v2MonoBass=200, v2Generate=0, v2Guard=On (1), v2Floor=0.15, v2Mix=1, v2Output=0, v2Motion=Off (0), v2PositionOn=Off (0), v2Mono=Stereo (0)
- HamloVerb (amount 0.08): space=Hall (3), engine=Velvet (3), decaySeconds=1.2, predelayMs=25, predelaySync=Free (0), lowCutHz=500, highCutHz=8000, bassMult=0.75, hfDecayRatio=0.5, width=1.1, modDepth=0.1, duck=0.15, routing=Send (1), mix=1, keepDry=Off (0), wetDb=0, outputDb=0, earlyDb=0, lateDb=0, timeShape=Off (0), shimmerAmount=0, freeze=Off (0)

Reject/revise if: The dip sounds like an external-key promise or makes the chord disappear.

Intent check: Give a sustained chord a gentle beat-linked dip and recovery. Assess against: The dip sounds like an external-key promise or makes the chord disappear.. Original design-stage calibration status: **not_run**; final numerical results and individual owner-audition status are in RACK_BANK_REPORT.md.

### HR143 - Techno Hypnotic Filter Step

**Samples / 132 BPM.** Source: a musical sample loop with gaps and a clear phrase ending

Move only the sample upper mids through a repeating low-pass gesture.

EQ: LowCut 100 Hz / +0 dB / Q 0.707

Full range:

- HamloMag (amount 0.2): model=Tape (1), driveDb=3, mix=1, trimDb=0, oversampling=8x (3), autoGain=On (1), harmonics=0.35, tone=0, evenOdd=0, tapeBias=0.5, speed=1 (0), wow=0, hiss=0, hum=0, crackle=0, noiseFollow=Follow (1), punish=Off (0), thump=Off (0), steep=Off (0), sampler=Off (0), phoneBass=0, subProtectHz=0, dynamics=0, match=Off (0), delta=Off (0)

Band 600-14000 Hz:

- HamloRevolution (amount 0.45): mix=1, free=Sync (0), grid=1/16 (2), triplet=Off (0), swing=0, humanize=0, fadeMs=3, order=T-G-M (0), scene=1 (0), listen=Off (0), timeOn=Off (0), glitchOn=Off (0), motionOn=On (1), timeMix=1, timeDepth=1, glitchMix=1, glitchDepth=0.5, motionMix=1, motionDepth=0.35, motionSmooth=0.5, motionTarget=Low Pass (1), timeStretch=Rhythm (1), glitchEngine=Block (1), shapeAOn=Off (0), shapeBOn=Off (0), throwAOn=Off (0), throwBOn=Off (0)
  - MOTION (steps 1-16): 3: 2; 4: 4; 5: 6; 6: 8; 7: 10; 8: 12; 9: 12; 10: 10; 11: 8; 12: 6; 13: 4; 14: 2

Reject/revise if: Its low pulse moves or filter steps click audibly.

Intent check: Move only the sample upper mids through a repeating low-pass gesture. Assess against: Its low pulse moves or filter steps click audibly.. Original design-stage calibration status: **not_run**; final numerical results and individual owner-audition status are in RACK_BANK_REPORT.md.

### HR144 - Techno Industrial Broken Loop

**Samples / 132 BPM.** Source: a musical sample loop with gaps and a clear phrase ending

Add two explicit BLOCK chops near the end of an industrial loop.

EQ: LowCut 160 Hz / +0 dB / Q 0.707; HighCut 13000 Hz / +0 dB / Q 0.707

Full range:

- HamloMag (amount 0.2): model=Germanium (4), driveDb=8, mix=1, trimDb=0, oversampling=8x (3), autoGain=On (1), harmonics=0.35, tone=0, evenOdd=0, tapeBias=0.5, speed=1 (0), wow=0, hiss=0, hum=0, crackle=0, noiseFollow=Follow (1), punish=Off (0), thump=Off (0), steep=Off (0), sampler=Off (0), phoneBass=0, subProtectHz=0, dynamics=0, match=Off (0), delta=Off (0)
- HamloRevolution (amount 0.35): mix=1, free=Sync (0), grid=1/16 (2), triplet=Off (0), swing=0, humanize=0, fadeMs=3, order=T-G-M (0), scene=1 (0), listen=Off (0), timeOn=Off (0), glitchOn=On (1), motionOn=Off (0), timeMix=1, timeDepth=1, glitchMix=1, glitchDepth=0.5, motionMix=1, motionDepth=0.35, motionSmooth=0.5, motionTarget=Low Pass (1), timeStretch=Rhythm (1), glitchEngine=Block (1), shapeAOn=Off (0), shapeBOn=Off (0), throwAOn=Off (0), throwBOn=Off (0)
  - BLOCK GLITCH (steps 1-16): 11: Chop; 15: Repeat; 16: Gap

Reject/revise if: The chop density obscures the loop or carries into the following bar.

Intent check: Add two explicit BLOCK chops near the end of an industrial loop. Assess against: The chop density obscures the loop or carries into the following bar.. Original design-stage calibration status: **not_run**; final numerical results and individual owner-audition status are in RACK_BANK_REPORT.md.

### HR145 - Techno Dub Stereo Field

**Samples / 132 BPM.** Source: a musical sample loop with gaps and a clear phrase ending

Give an ambient texture width and a slow beat-linked pan journey.

EQ: LowCut 180 Hz / +0 dB / Q 0.707

Full range:

- HamloShirota (amount 1): v2Mode=Stereo / Space (1), v2Width=1, v2LowWidth=0, v2MidWidth=1.15, v2HighWidth=1.3, v2LowSplit=300, v2HighSplit=3000, v2MonoBass=220, v2Generate=0, v2Guard=On (1), v2Floor=0.15, v2Mix=1, v2Output=0, v2Motion=On (1), v2PositionOn=On (1), v2Mono=Stereo (0), v2Unit=Quarter-note beats (1), v2Nodes=2, v2Node0Angle=-35, v2Node1Angle=35, v2Node0HoldBeat=1, v2Node1HoldBeat=1, v2Node0TravelBeat=3, v2Node1TravelBeat=3

Reject/revise if: The mono fold loses the texture or the low anchor shifts sides.

Intent check: Give an ambient texture width and a slow beat-linked pan journey. Assess against: The mono fold loses the texture or the low anchor shifts sides.. Original design-stage calibration status: **not_run**; final numerical results and individual owner-audition status are in RACK_BANK_REPORT.md.

### HR146 - Techno One Beat Reverse Swell

**Samples / 132 BPM.** Source: a musical sample loop with gaps and a clear phrase ending

Add a quiet reverse-shaped reverb approaching the next sample hit.

EQ: LowCut 200 Hz / +0 dB / Q 0.707

Full range:

- HamloVerb (amount 0.2): space=Hall (3), engine=Bloom (6), decaySeconds=1.2, predelayMs=0, predelaySync=Free (0), lowCutHz=700, highCutHz=6500, bassMult=0.75, hfDecayRatio=0.5, width=1.1, modDepth=0.1, duck=0.15, routing=Send (1), mix=1, keepDry=Off (0), wetDb=0, outputDb=0, earlyDb=0, lateDb=0, timeShape=Reverse Swell (2), shimmerAmount=0, freeze=Off (0), shapeSync=1/4 (10), gateReleaseMs=30

Reject/revise if: The effect changes the dry timing or arrives after its target hit.

Intent check: Add a quiet reverse-shaped reverb approaching the next sample hit. Assess against: The effect changes the dry timing or arrives after its target hit.. Original design-stage calibration status: **not_run**; final numerical results and individual owner-audition status are in RACK_BANK_REPORT.md.

### HR147 - Techno Peak Clean Pressure

**Master / 132 BPM.** Source: an unmastered stereo mix with headroom and a busy chorus

Finish a club premaster with a stable low center and controlled limiter work.

EQ: LowCut 24 Hz / +0 dB / Q 0.707; Bell 330 Hz / -0.7 dB / Q 1

Full range:

- HamloComp (amount 1): thresholdDb=-19, ratio=1.5, attackMs=40, releaseMs=114, model=VCA (1), source=All (0), kneeDb=6, detector=RMS (1), character=Clean (0), autoMakeup=Off (0), makeupDb=0, mix=1
- HamloShirota (amount 1): v2Mode=Stereo / Space (1), v2Width=1, v2LowWidth=0, v2MidWidth=1, v2HighWidth=1.06, v2LowSplit=300, v2HighSplit=3000, v2MonoBass=110, v2Generate=0, v2Guard=On (1), v2Floor=0.15, v2Mix=1, v2Output=0, v2Motion=Off (0), v2PositionOn=Off (0), v2Mono=Stereo (0)
- HamloRedWall (amount 1): engine=V2 (1), thresholdDb=-2.4, ceilingDb=-1, releaseMs=100, autoRelease=Manual (0), lookaheadMs=2, link=1, truePeak=On (1), style=GLASNOST (0), outputDb=0, drive=0, tube=0, monitor=Normal (0), mix=1

Reject/revise if: Final 8x true peak exceeds -1 dBTP or kick flattening begins before 3 dB work.

Intent check: Finish a club premaster with a stable low center and controlled limiter work. Assess against: Final 8x true peak exceeds -1 dBTP or kick flattening begins before 3 dB work.. Original design-stage calibration status: **not_run**; final numerical results and individual owner-audition status are in RACK_BANK_REPORT.md.

### HR148 - Techno Hypnotic Longform

**Master / 132 BPM.** Source: an unmastered stereo mix with headroom and a busy chorus

Preserve small evolving accents while adding gentle tape cohesion.

EQ: LowCut 23 Hz / +0 dB / Q 0.707; HighShelf 13000 Hz / -0.3 dB / Q 0.707

Full range:

- HamloMag (amount 0.15): model=Tape (1), driveDb=2, mix=1, trimDb=0, oversampling=8x (3), autoGain=On (1), harmonics=0.35, tone=0, evenOdd=0, tapeBias=0.5, speed=1 (0), wow=0, hiss=0, hum=0, crackle=0, noiseFollow=Follow (1), punish=Off (0), thump=Off (0), steep=Off (0), sampler=Off (0), phoneBass=0, subProtectHz=0, dynamics=0, match=Off (0), delta=Off (0)
- HamloComp (amount 1): thresholdDb=-20, ratio=1.4, attackMs=45, releaseMs=190, model=Opto (3), source=All (0), kneeDb=6, detector=RMS (1), character=Clean (0), autoMakeup=Off (0), makeupDb=0, mix=1
- HamloRedWall (amount 1): engine=V2 (1), thresholdDb=-1.8, ceilingDb=-1, releaseMs=145, autoRelease=Manual (0), lookaheadMs=2, link=1, truePeak=On (1), style=GLASNOST (0), outputDb=0, drive=0, tube=0, monitor=Normal (0), mix=1

Reject/revise if: Long compression causes audible hat pumping or subtle accents disappear.

Intent check: Preserve small evolving accents while adding gentle tape cohesion. Assess against: Long compression causes audible hat pumping or subtle accents disappear.. Original design-stage calibration status: **not_run**; final numerical results and individual owner-audition status are in RACK_BANK_REPORT.md.

### HR149 - Techno Dub Depth Master

**Master / 132 BPM.** Source: an unmastered stereo mix with headroom and a busy chorus

Keep dub space deep and protect low-mid headroom.

EQ: LowCut 25 Hz / +0 dB / Q 0.707; Bell 220 Hz / -0.8 dB / Q 0.8

Full range:

- HamloShirota (amount 1): v2Mode=Stereo / Space (1), v2Width=1, v2LowWidth=0.9, v2MidWidth=1.02, v2HighWidth=1.08, v2LowSplit=300, v2HighSplit=3000, v2MonoBass=120, v2Generate=0, v2Guard=On (1), v2Floor=0.15, v2Mix=1, v2Output=0, v2Motion=Off (0), v2PositionOn=Off (0), v2Mono=Stereo (0)
- HamloComp (amount 1): thresholdDb=-19, ratio=1.4, attackMs=40, releaseMs=170, model=VCA (1), source=All (0), kneeDb=6, detector=RMS (1), character=Clean (0), autoMakeup=Off (0), makeupDb=0, mix=1
- HamloRedWall (amount 1): engine=V2 (1), thresholdDb=-1.6, ceilingDb=-1, releaseMs=150, autoRelease=Manual (0), lookaheadMs=2, link=1, truePeak=On (1), style=GLASNOST (0), outputDb=0, drive=0, tube=0, monitor=Normal (0), mix=1

Reject/revise if: The room folds inward on kicks or low delay returns dominate the mix.

Intent check: Keep dub space deep and protect low-mid headroom. Assess against: The room folds inward on kicks or low delay returns dominate the mix.. Original design-stage calibration status: **not_run**; final numerical results and individual owner-audition status are in RACK_BANK_REPORT.md.

### HR150 - Techno Hard Transient Reserve

**Master / 132 BPM.** Source: an unmastered stereo mix with headroom and a busy chorus

Keep aggressive midrange while reserving clean space for kick peaks.

EQ: LowCut 26 Hz / +0 dB / Q 0.707; Bell 4000 Hz / +0 dB / Q 1.2 (dynamic -1.5 dB)

Full range:

- HamloComp (amount 1): thresholdDb=-18, ratio=1.5, attackMs=35, releaseMs=95, model=VCA (1), source=All (0), kneeDb=6, detector=RMS (1), character=Clean (0), autoMakeup=Off (0), makeupDb=0, mix=1
- HamloMag (amount 0.12): model=Iron (3), driveDb=2, mix=1, trimDb=0, oversampling=8x (3), autoGain=On (1), harmonics=0.35, tone=0, evenOdd=0, tapeBias=0.5, speed=1 (0), wow=0, hiss=0, hum=0, crackle=0, noiseFollow=Follow (1), punish=Off (0), thump=Off (0), steep=Off (0), sampler=Off (0), phoneBass=0, subProtectHz=120, dynamics=0, match=Off (0), delta=Off (0)
- HamloRedWall (amount 1): engine=V2 (1), thresholdDb=-2.8, ceilingDb=-1, releaseMs=85, autoRelease=Manual (0), lookaheadMs=2, link=1, truePeak=On (1), style=GLASNOST (0), outputDb=0, drive=0, tube=0, monitor=Normal (0), mix=1

Reject/revise if: The hard character vanishes or limiting adds low-note distortion.

Intent check: Keep aggressive midrange while reserving clean space for kick peaks. Assess against: The hard character vanishes or limiting adds low-note distortion.. Original design-stage calibration status: **not_run**; final numerical results and individual owner-audition status are in RACK_BANK_REPORT.md.

## Drum and Bass

### HR151 - DnB Modern Fast Punch

**Kick / 174 BPM.** Source: isolated kick hits including soft and hard strikes

Keep a short kick attack distinct inside a fast break.

EQ: LowCut 30 Hz / +0 dB / Q 0.707; Bell 300 Hz / -2 dB / Q 1; Bell 2700 Hz / +1 dB / Q 1

Full range:

- HamloUdarnik (amount 1): attackDb=2, sustainDb=-1.5, speed=0.9, style=PRAVDA (0), engine=V2 (1), protect=On (1), lookahead=Vostok (1), listen=Full (0), rise=1, decay=1, sensitivity=0.8, preserve=1, mix=1, outputDb=0
- HamloComp (amount 1): thresholdDb=-22, ratio=2.5, attackMs=14, releaseMs=65, model=VCA (1), source=Drums (1), kneeDb=6, detector=RMS (1), character=Clean (0), autoMakeup=Off (0), makeupDb=0, mix=1

Reject/revise if: The body masks the next ghost hit or click loses definition.

Intent check: Keep a short kick attack distinct inside a fast break. Assess against: The body masks the next ghost hit or click loses definition.. Original design-stage calibration status: **not_run**; final numerical results and individual owner-audition status are in RACK_BANK_REPORT.md.

### HR152 - DnB Roller Low Pocket

**Kick / 174 BPM.** Source: isolated kick hits including soft and hard strikes

Tighten a rolling kick tail without stripping its low-note weight.

EQ: LowCut 25 Hz / +0 dB / Q 0.707; Bell 420 Hz / -1.5 dB / Q 1

Full range:

- HamloZastava (amount 1): v2_engine=Drum (2), v2_enabled=On (1), v2_threshold=-42, v2_range=12, v2_hold=32, v2_release=45, v2_attack=0.3, v2_hysteresis=4, v2_knee=6, v2_external=Main (0), v2_expand=Expand (1), v2_ratio=2, v2_link=1, v2_mix=1, v2_output=0, v2_body=32, v2_tail=45, v2_lookahead=2, v2_hp=0, v2_lp=0, v2_listen=Off (0)
- HamloMag (amount 0.25): model=Iron (3), driveDb=4, mix=1, trimDb=0, oversampling=8x (3), autoGain=On (1), harmonics=0.35, tone=0, evenOdd=0, tapeBias=0.5, speed=1 (0), wow=0, hiss=0, hum=0, crackle=0, noiseFollow=Follow (1), punish=Off (0), thump=Off (0), steep=Off (0), sampler=Off (0), phoneBass=0, subProtectHz=65, dynamics=0, match=Off (0), delta=Off (0)

Reject/revise if: The low note is cut before a full cycle or the tail overlaps the bass.

Intent check: Tighten a rolling kick tail without stripping its low-note weight. Assess against: The low note is cut before a full cycle or the tail overlaps the bass.. Original design-stage calibration status: **not_run**; final numerical results and individual owner-audition status are in RACK_BANK_REPORT.md.

### HR153 - DnB Liquid Soft Front

**Kick / 174 BPM.** Source: isolated kick hits including soft and hard strikes

Soften an aggressive kick for an open liquid arrangement.

EQ: LowCut 28 Hz / +0 dB / Q 0.707; Bell 3400 Hz / -1 dB / Q 1

Full range:

- HamloUdarnik (amount 1): attackDb=-1.2, sustainDb=-0.5, speed=0.6, style=PRAVDA (0), engine=V2 (1), protect=On (1), lookahead=Vostok (1), listen=Full (0), rise=1, decay=1, sensitivity=0.8, preserve=1, mix=1, outputDb=0
- HamloComp (amount 1): thresholdDb=-21, ratio=1.8, attackMs=24, releaseMs=100, model=Opto (3), source=Drums (1), kneeDb=6, detector=RMS (1), character=Clean (0), autoMakeup=Off (0), makeupDb=0, mix=1

Reject/revise if: The softened kick disappears under bass or gains excess sustain.

Intent check: Soften an aggressive kick for an open liquid arrangement. Assess against: The softened kick disappears under bass or gains excess sustain.. Original design-stage calibration status: **not_run**; final numerical results and individual owner-audition status are in RACK_BANK_REPORT.md.

### HR154 - DnB Neuro Mid Click

**Kick / 174 BPM.** Source: isolated kick hits including soft and hard strikes

Add a compact mid click above clean low-end energy.

EQ: LowCut 28 Hz / +0 dB / Q 0.707

Full range:

- HamloUdarnik (amount 1): attackDb=1, sustainDb=-1, speed=0.85, style=PRAVDA (0), engine=V2 (1), protect=On (1), lookahead=Vostok (1), listen=Full (0), rise=1, decay=1, sensitivity=0.8, preserve=1, mix=1, outputDb=0

Band 500-6000 Hz:

- HamloMag (amount 0.3): model=Iron (3), driveDb=7, mix=1, trimDb=0, oversampling=8x (3), autoGain=On (1), harmonics=0.35, tone=0, evenOdd=0, tapeBias=0.5, speed=1 (0), wow=0, hiss=0, hum=0, crackle=0, noiseFollow=Follow (1), punish=Off (0), thump=Off (0), steep=Off (0), sampler=Off (0), phoneBass=0, subProtectHz=0, dynamics=0, match=Off (0), delta=Off (0)
- HamloComp (amount 1): thresholdDb=-24, ratio=3, attackMs=5, releaseMs=55, model=FET (2), source=All (0), kneeDb=6, detector=RMS (1), character=Clean (0), autoMakeup=Off (0), makeupDb=0, mix=1

Reject/revise if: Upper distortion masks the snare or the sub becomes gritty.

Intent check: Add a compact mid click above clean low-end energy. Assess against: Upper distortion masks the snare or the sub becomes gritty.. Original design-stage calibration status: **not_run**; final numerical results and individual owner-audition status are in RACK_BANK_REPORT.md.

### HR155 - DnB Jump Up Solid Knock

**Kick / 174 BPM.** Source: isolated kick hits including soft and hard strikes

Emphasize the audible low-mid punch without bloated sub sustain.

EQ: LowCut 32 Hz / +0 dB / Q 0.707; Bell 130 Hz / +1 dB / Q 1; Bell 400 Hz / -2 dB / Q 1

Full range:

- HamloUdarnik (amount 1): attackDb=1.5, sustainDb=-2, speed=0.85, style=PRAVDA (0), engine=V2 (1), protect=On (1), lookahead=Vostok (1), listen=Full (0), rise=1, decay=1, sensitivity=0.8, preserve=1, mix=1, outputDb=0
- HamloMag (amount 0.3): model=Tube (2), driveDb=5, mix=1, trimDb=0, oversampling=8x (3), autoGain=On (1), harmonics=0.35, tone=0, evenOdd=0, tapeBias=0.5, speed=1 (0), wow=0, hiss=0, hum=0, crackle=0, noiseFollow=Follow (1), punish=Off (0), thump=Off (0), steep=Off (0), sampler=Off (0), phoneBass=0, subProtectHz=0, dynamics=0, match=Off (0), delta=Off (0)

Reject/revise if: The 100-180 Hz area clouds the bass line or the front edge clips.

Intent check: Emphasize the audible low-mid punch without bloated sub sustain. Assess against: The 100-180 Hz area clouds the bass line or the front edge clips.. Original design-stage calibration status: **not_run**; final numerical results and individual owner-audition status are in RACK_BANK_REPORT.md.

### HR156 - DnB Break Resampled Foot

**Kick / 174 BPM.** Source: isolated kick hits including soft and hard strikes

Blend sampled texture into an acoustic kick with fast release.

EQ: LowCut 30 Hz / +0 dB / Q 0.707; HighCut 12000 Hz / +0 dB / Q 0.707

Full range:

- HamloMag (amount 0.25): model=Tape (1), driveDb=4, mix=1, trimDb=0, oversampling=8x (3), autoGain=On (1), harmonics=0.35, tone=0, evenOdd=0, tapeBias=0.5, speed=1 (0), wow=0, hiss=0, hum=0, crackle=0, noiseFollow=Follow (1), punish=Off (0), thump=Off (0), steep=Off (0), sampler=S950 (3), phoneBass=0, subProtectHz=0, dynamics=0, match=Off (0), delta=Off (0)
- HamloComp (amount 0.5): thresholdDb=-24, ratio=3, attackMs=10, releaseMs=60, model=VCA (1), source=Drums (1), kneeDb=6, detector=RMS (1), character=Clean (0), autoMakeup=Off (0), makeupDb=0, mix=1

Reject/revise if: Texture erases low-velocity hits or the kick sounds continuously crushed.

Intent check: Blend sampled texture into an acoustic kick with fast release. Assess against: Texture erases low-velocity hits or the kick sounds continuously crushed.. Original design-stage calibration status: **not_run**; final numerical results and individual owner-audition status are in RACK_BANK_REPORT.md.

### HR157 - DnB Modern Snare Whip

**Snare / 174 BPM.** Source: snare or clap hits including ghost notes and a flam

Increase attack/body contrast without making the snare louder by default.

EQ: LowCut 125 Hz / +0 dB / Q 0.707; Bell 700 Hz / -1.5 dB / Q 1

Full range:

- HamloUdarnik (amount 0.75): attackDb=2.5, sustainDb=-1.5, speed=0.9, style=UDARNIK (1), engine=V2 (1), protect=On (1), lookahead=Vostok (1), listen=Full (0), rise=1, decay=1, sensitivity=0.8, preserve=1, mix=1, outputDb=0
- HamloMag (amount 0.25): model=Iron (3), driveDb=4, mix=1, trimDb=0, oversampling=8x (3), autoGain=On (1), harmonics=0.35, tone=0, evenOdd=0, tapeBias=0.5, speed=1 (0), wow=0, hiss=0, hum=0, crackle=0, noiseFollow=Follow (1), punish=Off (0), thump=Off (0), steep=Off (0), sampler=Off (0), phoneBass=0, subProtectHz=0, dynamics=0, match=Off (0), delta=Off (0)

Reject/revise if: The matched hit lacks crack or the upper snap becomes painful.

Intent check: Increase attack/body contrast without making the snare louder by default. Assess against: The matched hit lacks crack or the upper snap becomes painful.. Original design-stage calibration status: **not_run**; final numerical results and individual owner-audition status are in RACK_BANK_REPORT.md.

### HR158 - DnB Roller Dry Backbeat

**Snare / 174 BPM.** Source: snare or clap hits including ghost notes and a flam

Trim a lingering snare tail while preserving ghost notes.

EQ: LowCut 130 Hz / +0 dB / Q 0.707; Bell 240 Hz / +1 dB / Q 1

Full range:

- HamloZastava (amount 1): v2_engine=Drum (2), v2_enabled=On (1), v2_threshold=-47, v2_range=10, v2_hold=30, v2_release=55, v2_attack=0.3, v2_hysteresis=4, v2_knee=6, v2_external=Main (0), v2_expand=Expand (1), v2_ratio=2, v2_link=1, v2_mix=1, v2_output=0, v2_body=30, v2_tail=55, v2_lookahead=2, v2_hp=0, v2_lp=0, v2_listen=Off (0)
- HamloComp (amount 1): thresholdDb=-22, ratio=2.2, attackMs=14, releaseMs=70, model=VCA (1), source=Drums (1), kneeDb=6, detector=RMS (1), character=Clean (0), autoMakeup=Off (0), makeupDb=0, mix=1

Reject/revise if: The gate removes ghost notes or the snare tail still covers the next hat.

Intent check: Trim a lingering snare tail while preserving ghost notes. Assess against: The gate removes ghost notes or the snare tail still covers the next hat.. Original design-stage calibration status: **not_run**; final numerical results and individual owner-audition status are in RACK_BANK_REPORT.md.

### HR159 - DnB Liquid Chamber Lift

**Snare / 174 BPM.** Source: snare or clap hits including ghost notes and a flam

Put a small dark chamber behind a clean light snare.

EQ: LowCut 150 Hz / +0 dB / Q 0.707; HighShelf 11500 Hz / +0.5 dB / Q 0.707

Full range:

- HamloUdarnik (amount 1): attackDb=1, sustainDb=0.5, speed=0.7, style=PRAVDA (0), engine=V2 (1), protect=On (1), lookahead=Vostok (1), listen=Full (0), rise=1, decay=1, sensitivity=0.8, preserve=1, mix=1, outputDb=0
- HamloVerb (amount 0.11): space=Chamber (2), engine=Velvet (3), decaySeconds=0.55, predelayMs=18, predelaySync=Free (0), lowCutHz=500, highCutHz=7500, bassMult=0.75, hfDecayRatio=0.5, width=1.1, modDepth=0.1, duck=0.15, routing=Send (1), mix=1, keepDry=Off (0), wetDb=0, outputDb=0, earlyDb=0, lateDb=0, timeShape=Off (0), shimmerAmount=0, freeze=Off (0)

Reject/revise if: The room overtakes ghosts or moves the backbeat behind the groove.

Intent check: Put a small dark chamber behind a clean light snare. Assess against: The room overtakes ghosts or moves the backbeat behind the groove.. Original design-stage calibration status: **not_run**; final numerical results and individual owner-audition status are in RACK_BANK_REPORT.md.

### HR160 - DnB Neuro Parallel Snap

**Snare / 174 BPM.** Source: snare or clap hits including ghost notes and a flam

Add a narrow rough snare edge under the original dry hit.

EQ: LowCut 120 Hz / +0 dB / Q 0.707; Bell 4800 Hz / +0 dB / Q 1.4 (dynamic -2.5 dB)

Full range:

- HamloComp (amount 0.3): thresholdDb=-29, ratio=6, attackMs=6, releaseMs=55, model=FET (2), source=Drums (1), kneeDb=6, detector=RMS (1), character=Clean (0), autoMakeup=Off (0), makeupDb=0, mix=1
- HamloMag (amount 0.18): model=Germanium (4), driveDb=9, mix=1, trimDb=0, oversampling=8x (3), autoGain=On (1), harmonics=0.35, tone=0, evenOdd=0, tapeBias=0.5, speed=1 (0), wow=0, hiss=0, hum=0, crackle=0, noiseFollow=Follow (1), punish=Off (0), thump=Off (0), steep=Off (0), sampler=Off (0), phoneBass=0, subProtectHz=0, dynamics=0, match=Off (0), delta=Off (0)

Reject/revise if: The dry hit flattens or 3-6 kHz roughness dominates.

Intent check: Add a narrow rough snare edge under the original dry hit. Assess against: The dry hit flattens or 3-6 kHz roughness dominates.. Original design-stage calibration status: **not_run**; final numerical results and individual owner-audition status are in RACK_BANK_REPORT.md.

### HR161 - DnB Jump Up Short Plate

**Snare / 174 BPM.** Source: snare or clap hits including ghost notes and a flam

Give a machine backbeat size with a quickly gated plate.

EQ: LowCut 140 Hz / +0 dB / Q 0.707; Bell 260 Hz / +1.2 dB / Q 1

Full range:

- HamloUdarnik (amount 1): attackDb=1, sustainDb=-1, speed=0.85, style=PRAVDA (0), engine=V2 (1), protect=On (1), lookahead=Vostok (1), listen=Full (0), rise=1, decay=1, sensitivity=0.8, preserve=1, mix=1, outputDb=0
- HamloVerb (amount 0.14): space=Plate (4), engine=Crisp (4), decaySeconds=0.65, predelayMs=12, predelaySync=Free (0), lowCutHz=650, highCutHz=6500, bassMult=0.75, hfDecayRatio=0.5, width=1.1, modDepth=0.1, duck=0.15, routing=Send (1), mix=1, keepDry=Off (0), wetDb=0, outputDb=0, earlyDb=0, lateDb=0, timeShape=Gated (1), shimmerAmount=0, freeze=Off (0), shapeTimeMs=95, gateReleaseMs=15

Reject/revise if: Plate persists into the next sixteenth or sounds like an abrupt click.

Intent check: Give a machine backbeat size with a quickly gated plate. Assess against: Plate persists into the next sixteenth or sounds like an abrupt click.. Original design-stage calibration status: **not_run**; final numerical results and individual owner-audition status are in RACK_BANK_REPORT.md.

### HR162 - DnB Break Ghost Detail

**Snare / 174 BPM.** Source: snare or clap hits including ghost notes and a flam

Level hard accents gently enough to expose break ghost articulation.

EQ: LowCut 110 Hz / +0 dB / Q 0.707; Bell 900 Hz / -1 dB / Q 1

Full range:

- HamloComp (amount 1): thresholdDb=-25, ratio=2, attackMs=18, releaseMs=86, model=Opto (3), source=Drums (1), kneeDb=12, detector=RMS (1), character=Clean (0), autoMakeup=Off (0), makeupDb=0, mix=1
- HamloMag (amount 0.2): model=Tape (1), driveDb=3, mix=1, trimDb=0, oversampling=8x (3), autoGain=On (1), harmonics=0.35, tone=0, evenOdd=0, tapeBias=0.5, speed=1 (0), wow=0, hiss=0, hum=0, crackle=0, noiseFollow=Follow (1), punish=Off (0), thump=Off (0), steep=Off (0), sampler=Off (0), phoneBass=0, subProtectHz=0, dynamics=0, match=Off (0), delta=Off (0)

Reject/revise if: Accent/ghost ratio collapses or ghosts start pumping.

Intent check: Level hard accents gently enough to expose break ghost articulation. Assess against: Accent/ghost ratio collapses or ghosts start pumping.. Original design-stage calibration status: **not_run**; final numerical results and individual owner-audition status are in RACK_BANK_REPORT.md.

### HR163 - DnB Modern Fast Silk

**Hats / 174 BPM.** Source: closed/open hats, an accent and a dense sixteenth roll

Reduce upper bite while keeping dense rolls legible.

EQ: LowCut 950 Hz / +0 dB / Q 0.707; Bell 8500 Hz / +0 dB / Q 1.2 (dynamic -2.5 dB)

Full range:

- HamloUdarnik (amount 1): attackDb=0.5, sustainDb=-1.5, speed=0.95, style=PRAVDA (0), engine=V2 (1), protect=On (1), lookahead=Vostok (1), listen=Full (0), rise=1, decay=1, sensitivity=0.8, preserve=1, mix=1, outputDb=0

Reject/revise if: Roll notes merge or the transient becomes duller than the body.

Intent check: Reduce upper bite while keeping dense rolls legible. Assess against: Roll notes merge or the transient becomes duller than the body.. Original design-stage calibration status: **not_run**; final numerical results and individual owner-audition status are in RACK_BANK_REPORT.md.

### HR164 - DnB Roller Shuffled Tops

**Hats / 174 BPM.** Source: closed/open hats, an accent and a dense sixteenth roll

Add a quiet eighth-triplet ghost around a straight top loop.

EQ: LowCut 1100 Hz / +0 dB / Q 0.707

Full range:

- HamloEcho (amount 0.075): echoMode=Score (1), quickEnabled=Off (0), scoreRouting=Send (1), scoreMix=1, scoreOutput=0, scoreSource=60, scoreFeedback=0.08, scoreFeedbackSync=1/8 T (4), scoreFeedbackMs=500, scoreFeedbackPitch=0, scoreColor=Warm (1), scoreDrive=0, scoreLowCut=1600, scoreHighCut=6500, scoreWidth=1, scoreDuck=0.2, scoreFreeze=Off (0), scoreSpaceMix=0, scoreGlitch=Off (0), scoreGlitchAmount=0, scoreBands=Off (0), scoreSwing=0, tap1On=1, tap2On=0, tap3On=0, tap4On=0, tap5On=0, tap6On=0, tap7On=0, tap8On=0, tap1Sync=1/8 T (4), tap1Time=250, tap1Note=60, tap1Gain=-6, tap1Pan=0, tap1Shape=Forward (0), tap1Fragment=125, tap1Repeats=3, tap1Low=1600, tap1High=6500, tap1SpaceSend=0

Reject/revise if: The ghost crowds the snare or sounds like free-time drift.

Intent check: Add a quiet eighth-triplet ghost around a straight top loop. Assess against: The ghost crowds the snare or sounds like free-time drift.. Original design-stage calibration status: **not_run**; final numerical results and individual owner-audition status are in RACK_BANK_REPORT.md.

### HR165 - DnB Liquid Wide Air

**Hats / 174 BPM.** Source: closed/open hats, an accent and a dense sixteenth roll

Widen soft cymbal air while keeping the low-mid stereo anchor.

EQ: LowCut 800 Hz / +0 dB / Q 0.707; HighShelf 12000 Hz / +0.6 dB / Q 0.707

Full range:

- HamloShirota (amount 1): v2Mode=Stereo / Space (1), v2Width=1, v2LowWidth=0, v2MidWidth=1, v2HighWidth=1.25, v2LowSplit=300, v2HighSplit=3000, v2MonoBass=300, v2Generate=0, v2Guard=On (1), v2Floor=0.15, v2Mix=1, v2Output=0, v2Motion=Off (0), v2PositionOn=Off (0), v2Mono=Stereo (0)
- HamloVerb (amount 0.045): space=Ambience (6), engine=Velvet (3), decaySeconds=0.28, predelayMs=2, predelaySync=Free (0), lowCutHz=1000, highCutHz=11000, bassMult=0.75, hfDecayRatio=0.5, width=1.1, modDepth=0.1, duck=0.15, routing=Send (1), mix=1, keepDry=Off (0), wetDb=0, outputDb=0, earlyDb=0, lateDb=0, timeShape=Off (0), shimmerAmount=0, freeze=Off (0)

Reject/revise if: The mono fold loses the hat pulse or the room smears fast notes.

Intent check: Widen soft cymbal air while keeping the low-mid stereo anchor. Assess against: The mono fold loses the hat pulse or the room smears fast notes.. Original design-stage calibration status: **not_run**; final numerical results and individual owner-audition status are in RACK_BANK_REPORT.md.

### HR166 - DnB Neuro Metallic Tuck

**Hats / 174 BPM.** Source: closed/open hats, an accent and a dense sixteenth roll

Control a hard metallic top resonance with short envelope action.

EQ: LowCut 1000 Hz / +0 dB / Q 0.707; Bell 6400 Hz / +0 dB / Q 2 (dynamic -3 dB)

Full range:

- HamloComp (amount 1): thresholdDb=-25, ratio=2.5, attackMs=1, releaseMs=35, model=VCA (1), source=All (0), kneeDb=8, detector=RMS (1), character=Clean (0), autoMakeup=Off (0), makeupDb=0, mix=1
- HamloUdarnik (amount 1): attackDb=-0.5, sustainDb=-0.5, speed=0.9, style=PRAVDA (0), engine=V2 (1), protect=On (1), lookahead=Vostok (1), listen=Full (0), rise=1, decay=1, sensitivity=0.8, preserve=1, mix=1, outputDb=0

Reject/revise if: The resonance rings past the next subdivision or ticks vanish.

Intent check: Control a hard metallic top resonance with short envelope action. Assess against: The resonance rings past the next subdivision or ticks vanish.. Original design-stage calibration status: **not_run**; final numerical results and individual owner-audition status are in RACK_BANK_REPORT.md.

### HR167 - DnB Break End Ratchet

**Hats / 174 BPM.** Source: closed/open hats, an accent and a dense sixteenth roll

Put a single BLOCK roll at the end of the top phrase.

EQ: LowCut 1100 Hz / +0 dB / Q 0.707

Full range:

- HamloRevolution (amount 0.28): mix=1, free=Sync (0), grid=1/16 (2), triplet=Off (0), swing=0, humanize=0, fadeMs=3, order=T-G-M (0), scene=1 (0), listen=Off (0), timeOn=Off (0), glitchOn=On (1), motionOn=Off (0), timeMix=1, timeDepth=1, glitchMix=1, glitchDepth=0.25, motionMix=1, motionDepth=0.35, motionSmooth=0.5, motionTarget=Low Pass (1), timeStretch=Rhythm (1), glitchEngine=Block (1), shapeAOn=Off (0), shapeBOn=Off (0), throwAOn=Off (0), throwBOn=Off (0)
  - BLOCK GLITCH (steps 1-16): 16: Roll

Reject/revise if: The roll affects the low kick or repeats across the following bar.

Intent check: Put a single BLOCK roll at the end of the top phrase. Assess against: The roll affects the low kick or repeats across the following bar.. Original design-stage calibration status: **not_run**; final numerical results and individual owner-audition status are in RACK_BANK_REPORT.md.

### HR168 - DnB Modern Tom Separation

**Percussion / 174 BPM.** Source: a sparse percussion loop with velocity variation

Shorten tom tails and expose their attack inside a busy fill.

EQ: LowCut 70 Hz / +0 dB / Q 0.707; Bell 500 Hz / -1.5 dB / Q 1

Full range:

- HamloUdarnik (amount 1): attackDb=1.5, sustainDb=-2, speed=0.85, style=PRAVDA (0), engine=V2 (1), protect=On (1), lookahead=Vostok (1), listen=Full (0), rise=1, decay=1, sensitivity=0.8, preserve=1, mix=1, outputDb=0
- HamloZastava (amount 1): v2_engine=Drum (2), v2_enabled=On (1), v2_threshold=-46, v2_range=10, v2_hold=30, v2_release=50, v2_attack=0.3, v2_hysteresis=4, v2_knee=6, v2_external=Main (0), v2_expand=Expand (1), v2_ratio=2, v2_link=1, v2_mix=1, v2_output=0, v2_body=30, v2_tail=50, v2_lookahead=2, v2_hp=0, v2_lp=0, v2_listen=Off (0)

Reject/revise if: The tom pitch becomes too short to identify or the fill loses velocity detail.

Intent check: Shorten tom tails and expose their attack inside a busy fill. Assess against: The tom pitch becomes too short to identify or the fill loses velocity detail.. Original design-stage calibration status: **not_run**; final numerical results and individual owner-audition status are in RACK_BANK_REPORT.md.

### HR169 - DnB Roller Wood Groove

**Percussion / 174 BPM.** Source: a sparse percussion loop with velocity variation

Keep woody syncopations compact with a modest iron body.

EQ: LowCut 250 Hz / +0 dB / Q 0.707; Bell 1300 Hz / -1 dB / Q 1

Full range:

- HamloMag (amount 0.3): model=Iron (3), driveDb=4, mix=1, trimDb=0, oversampling=8x (3), autoGain=On (1), harmonics=0.35, tone=0, evenOdd=0, tapeBias=0.5, speed=1 (0), wow=0, hiss=0, hum=0, crackle=0, noiseFollow=Follow (1), punish=Off (0), thump=Off (0), steep=Off (0), sampler=Off (0), phoneBass=0, subProtectHz=0, dynamics=0, match=Off (0), delta=Off (0)
- HamloComp (amount 1): thresholdDb=-24, ratio=2, attackMs=12, releaseMs=75, model=VCA (1), source=Drums (1), kneeDb=6, detector=RMS (1), character=Clean (0), autoMakeup=Off (0), makeupDb=0, mix=1

Reject/revise if: The loop loses swing articulation or low-mid body clouds the snare.

Intent check: Keep woody syncopations compact with a modest iron body. Assess against: The loop loses swing articulation or low-mid body clouds the snare.. Original design-stage calibration status: **not_run**; final numerical results and individual owner-audition status are in RACK_BANK_REPORT.md.

### HR170 - DnB Liquid Bell Glints

**Percussion / 174 BPM.** Source: a sparse percussion loop with velocity variation

Place two quiet stereo bell responses around a dry event.

EQ: LowCut 400 Hz / +0 dB / Q 0.707

Full range:

- HamloEcho (amount 0.1): echoMode=Score (1), quickEnabled=Off (0), scoreRouting=Send (1), scoreMix=1, scoreOutput=0, scoreSource=60, scoreFeedback=0.12, scoreFeedbackSync=1/8 (5), scoreFeedbackMs=500, scoreFeedbackPitch=0, scoreColor=Clean (0), scoreDrive=0, scoreLowCut=700, scoreHighCut=9000, scoreWidth=1, scoreDuck=0.2, scoreFreeze=Off (0), scoreSpaceMix=0, scoreGlitch=Off (0), scoreGlitchAmount=0, scoreBands=Off (0), scoreSwing=0, tap1On=1, tap2On=1, tap3On=0, tap4On=0, tap5On=0, tap6On=0, tap7On=0, tap8On=0, tap1Sync=1/8 (5), tap1Time=250, tap1Note=60, tap1Gain=-11, tap1Pan=-0.5, tap1Shape=Forward (0), tap1Fragment=125, tap1Repeats=3, tap1Low=700, tap1High=9000, tap1SpaceSend=0, tap2Sync=1/4 (8), tap2Time=500, tap2Note=60, tap2Gain=-15, tap2Pan=0.5, tap2Shape=Forward (0), tap2Fragment=125, tap2Repeats=3, tap2Low=700, tap2High=9000, tap2SpaceSend=0

Reject/revise if: Pitched bell identity changes or mono loses the original attack.

Intent check: Place two quiet stereo bell responses around a dry event. Assess against: Pitched bell identity changes or mono loses the original attack.. Original design-stage calibration status: **not_run**; final numerical results and individual owner-audition status are in RACK_BANK_REPORT.md.

### HR171 - DnB Neuro Fragment Answer

**Percussion / 174 BPM.** Source: a sparse percussion loop with velocity variation

Repeat the final percussion fragment once with a restrained wet throw.

EQ: LowCut 350 Hz / +0 dB / Q 0.707

Full range:

- HamloRevolution (amount 0.35): mix=1, free=Sync (0), grid=1/16 (2), triplet=Off (0), swing=0, humanize=0, fadeMs=3, order=T-G-M (0), scene=1 (0), listen=Off (0), timeOn=Off (0), glitchOn=On (1), motionOn=Off (0), timeMix=1, timeDepth=1, glitchMix=1, glitchDepth=0.5, motionMix=1, motionDepth=0.35, motionSmooth=0.5, motionTarget=Low Pass (1), timeStretch=Rhythm (1), glitchEngine=Block (1), shapeAOn=Off (0), shapeBOn=Off (0), throwAOn=On (1), throwBOn=Off (0), throwATarget=Delay (0), throwAMix=0.3, throwATail=0.25, throwATime=1/8 (5)
  - BLOCK GLITCH (steps 1-16): 15: Repeat
  - THROW A (steps 1-16): 16: 9

Reject/revise if: The phrase becomes a continuous glitch or the next downbeat is obscured.

Intent check: Repeat the final percussion fragment once with a restrained wet throw. Assess against: The phrase becomes a continuous glitch or the next downbeat is obscured.. Original design-stage calibration status: **not_run**; final numerical results and individual owner-audition status are in RACK_BANK_REPORT.md.

### HR172 - DnB Modern Crest Glue

**Drum Bus / 174 BPM.** Source: a stereo unprocessed drum loop with kick, snare, hats and ghost notes

Glue a fast break with release short enough to recover before the next hit.

EQ: LowCut 26 Hz / +0 dB / Q 0.707; Bell 350 Hz / -0.6 dB / Q 1

Full range:

- HamloComp (amount 1): thresholdDb=-22, ratio=2, attackMs=24, releaseMs=70, model=VCA (1), source=Drums (1), kneeDb=6, detector=RMS (1), character=Clean (0), autoMakeup=Off (0), makeupDb=0, mix=1
- HamloUdarnik (amount 0.6): attackDb=0.8, sustainDb=-0.5, speed=0.8, style=PRAVDA (0), engine=V2 (1), protect=On (1), lookahead=Vostok (1), listen=Full (0), rise=1, decay=1, sensitivity=0.8, preserve=1, mix=1, outputDb=0

Reject/revise if: Matched crest shrinks too far or the compressor stays down between hits.

Intent check: Glue a fast break with release short enough to recover before the next hit. Assess against: Matched crest shrinks too far or the compressor stays down between hits.. Original design-stage calibration status: **not_run**; final numerical results and individual owner-audition status are in RACK_BANK_REPORT.md.

### HR173 - DnB Roller Parallel Weight

**Drum Bus / 174 BPM.** Source: a stereo unprocessed drum loop with kick, snare, hats and ghost notes

Add mid-body weight under a clean rolling break.

EQ: LowCut 25 Hz / +0 dB / Q 0.707

Full range:

- HamloComp (amount 0.25): thresholdDb=-29, ratio=6, attackMs=12, releaseMs=65, model=FET (2), source=Drums (1), kneeDb=6, detector=RMS (1), character=Clean (0), autoMakeup=Off (0), makeupDb=0, mix=1

Band 130-5500 Hz:

- HamloMag (amount 0.3): model=Iron (3), driveDb=6, mix=1, trimDb=0, oversampling=8x (3), autoGain=On (1), harmonics=0.35, tone=0, evenOdd=0, tapeBias=0.5, speed=1 (0), wow=0, hiss=0, hum=0, crackle=0, noiseFollow=Follow (1), punish=Off (0), thump=Off (0), steep=Off (0), sampler=Off (0), phoneBass=0, subProtectHz=0, dynamics=0, match=Off (0), delta=Off (0)

Reject/revise if: Kick/snare attacks flatten or the parallel path lifts hat wash.

Intent check: Add mid-body weight under a clean rolling break. Assess against: Kick/snare attacks flatten or the parallel path lifts hat wash.. Original design-stage calibration status: **not_run**; final numerical results and individual owner-audition status are in RACK_BANK_REPORT.md.

### HR174 - DnB Liquid Open Break

**Drum Bus / 174 BPM.** Source: a stereo unprocessed drum loop with kick, snare, hats and ghost notes

Retain room dynamics and add only a small high-band spread.

EQ: LowCut 28 Hz / +0 dB / Q 0.707; Bell 430 Hz / -0.7 dB / Q 1

Full range:

- HamloComp (amount 1): thresholdDb=-21, ratio=1.6, attackMs=30, releaseMs=105, model=Opto (3), source=Drums (1), kneeDb=6, detector=RMS (1), character=Clean (0), autoMakeup=Off (0), makeupDb=0, mix=1
- HamloShirota (amount 1): v2Mode=Stereo / Space (1), v2Width=1, v2LowWidth=0, v2MidWidth=1.03, v2HighWidth=1.16, v2LowSplit=300, v2HighSplit=3000, v2MonoBass=150, v2Generate=0, v2Guard=On (1), v2Floor=0.15, v2Mix=1, v2Output=0, v2Motion=Off (0), v2PositionOn=Off (0), v2Mono=Stereo (0)
- HamloVerb (amount 0.035): space=Ambience (6), engine=Velvet (3), decaySeconds=0.25, predelayMs=5, predelaySync=Free (0), lowCutHz=1000, highCutHz=9500, bassMult=0.75, hfDecayRatio=0.5, width=1.1, modDepth=0.1, duck=0.15, routing=Send (1), mix=1, keepDry=Off (0), wetDb=0, outputDb=0, earlyDb=0, lateDb=0, timeShape=Off (0), shimmerAmount=0, freeze=Off (0)

Reject/revise if: The snare loses its location or mono cymbals thin out.

Intent check: Retain room dynamics and add only a small high-band spread. Assess against: The snare loses its location or mono cymbals thin out.. Original design-stage calibration status: **not_run**; final numerical results and individual owner-audition status are in RACK_BANK_REPORT.md.

### HR175 - DnB Neuro Tight Drum Wall

**Drum Bus / 174 BPM.** Source: a stereo unprocessed drum loop with kick, snare, hats and ghost notes

Shorten drum bodies before a restrained parallel hard-colour layer.

EQ: LowCut 28 Hz / +0 dB / Q 0.707; Bell 4800 Hz / +0 dB / Q 1 (dynamic -1.5 dB)

Full range:

- HamloUdarnik (amount 0.65): attackDb=1, sustainDb=-1.5, speed=0.9, style=UDARNIK (1), engine=V2 (1), protect=On (1), lookahead=Vostok (1), listen=Full (0), rise=1, decay=1, sensitivity=0.8, preserve=1, mix=1, outputDb=0
- HamloMag (amount 0.16): model=Germanium (4), driveDb=7, mix=1, trimDb=0, oversampling=8x (3), autoGain=On (1), harmonics=0.35, tone=0, evenOdd=0, tapeBias=0.5, speed=1 (0), wow=0, hiss=0, hum=0, crackle=0, noiseFollow=Follow (1), punish=Off (0), thump=Off (0), steep=Off (0), sampler=Off (0), phoneBass=0, subProtectHz=0, dynamics=0, match=Off (0), delta=Off (0)
- HamloComp (amount 0.4): thresholdDb=-26, ratio=3.5, attackMs=14, releaseMs=60, model=VCA (1), source=Drums (1), kneeDb=6, detector=RMS (1), character=Clean (0), autoMakeup=Off (0), makeupDb=0, mix=1

Reject/revise if: Ghost notes vanish or sharp edges overshoot the peak-growth plan.

Intent check: Shorten drum bodies before a restrained parallel hard-colour layer. Assess against: Ghost notes vanish or sharp edges overshoot the peak-growth plan.. Original design-stage calibration status: **not_run**; final numerical results and individual owner-audition status are in RACK_BANK_REPORT.md.

### HR176 - DnB Jump Up Front Back

**Drum Bus / 174 BPM.** Source: a stereo unprocessed drum loop with kick, snare, hats and ghost notes

Favor kick/snare front edges without increasing cymbal sustain.

EQ: LowCut 30 Hz / +0 dB / Q 0.707; Bell 280 Hz / -0.7 dB / Q 1

Full range:

- HamloUdarnik (amount 1): attackDb=1.5, sustainDb=-1, speed=0.9, style=PRAVDA (0), engine=V2 (1), protect=On (1), lookahead=Vostok (1), listen=Full (0), rise=1, decay=1, sensitivity=0.8, preserve=1, mix=1, outputDb=0
- HamloMag (amount 0.25): model=Iron (3), driveDb=4, mix=1, trimDb=0, oversampling=8x (3), autoGain=On (1), harmonics=0.35, tone=0, evenOdd=0, tapeBias=0.5, speed=1 (0), wow=0, hiss=0, hum=0, crackle=0, noiseFollow=Follow (1), punish=Off (0), thump=Off (0), steep=Off (0), sampler=Off (0), phoneBass=0, subProtectHz=0, dynamics=0, match=Off (0), delta=Off (0)
- HamloComp (amount 1): thresholdDb=-22, ratio=2, attackMs=20, releaseMs=80, model=VCA (1), source=Drums (1), kneeDb=6, detector=RMS (1), character=Clean (0), autoMakeup=Off (0), makeupDb=0, mix=1

Reject/revise if: Cymbal wash rises or the snare becomes thin after matching.

Intent check: Favor kick/snare front edges without increasing cymbal sustain. Assess against: Cymbal wash rises or the snare becomes thin after matching.. Original design-stage calibration status: **not_run**; final numerical results and individual owner-audition status are in RACK_BANK_REPORT.md.

### HR177 - DnB Break Sampler Cohesion

**Drum Bus / 174 BPM.** Source: a stereo unprocessed drum loop with kick, snare, hats and ghost notes

Bring a chopped acoustic break together with restrained sampler colour.

EQ: LowCut 28 Hz / +0 dB / Q 0.707; HighCut 14000 Hz / +0 dB / Q 0.707

Full range:

- HamloMag (amount 0.3): model=Tape (1), driveDb=4, mix=1, trimDb=0, oversampling=8x (3), autoGain=On (1), harmonics=0.35, tone=0, evenOdd=0, tapeBias=0.5, speed=1 (0), wow=0, hiss=0, hum=0, crackle=0, noiseFollow=Follow (1), punish=Off (0), thump=Off (0), steep=Off (0), sampler=MPC60 (2), phoneBass=0, subProtectHz=0, dynamics=0, match=Off (0), delta=Off (0)
- HamloComp (amount 1): thresholdDb=-23, ratio=2.2, attackMs=22, releaseMs=86, model=VCA (1), source=Drums (1), kneeDb=6, detector=RMS (1), character=Clean (0), autoMakeup=Off (0), makeupDb=0, mix=1

Reject/revise if: The resampled top aliases excessively or original ghost timing changes.

Intent check: Bring a chopped acoustic break together with restrained sampler colour. Assess against: The resampled top aliases excessively or original ghost timing changes.. Original design-stage calibration status: **not_run**; final numerical results and individual owner-audition status are in RACK_BANK_REPORT.md.

### HR178 - DnB Last Beat Break Edit

**Drum Bus / 174 BPM.** Source: a stereo unprocessed drum loop with kick, snare, hats and ghost notes

Use explicit upper-band reverse and repeat steps at the phrase end.

EQ: LowCut 25 Hz / +0 dB / Q 0.707

Full range:

- HamloComp (amount 1): thresholdDb=-22, ratio=1.8, attackMs=26, releaseMs=75, model=VCA (1), source=Drums (1), kneeDb=6, detector=RMS (1), character=Clean (0), autoMakeup=Off (0), makeupDb=0, mix=1

Band 1000-16000 Hz:

- HamloRevolution (amount 0.32): mix=1, free=Sync (0), grid=1/16 (2), triplet=Off (0), swing=0, humanize=0, fadeMs=3, order=T-G-M (0), scene=1 (0), listen=Off (0), timeOn=Off (0), glitchOn=On (1), motionOn=Off (0), timeMix=1, timeDepth=1, glitchMix=1, glitchDepth=0.5, motionMix=1, motionDepth=0.35, motionSmooth=0.5, motionTarget=Low Pass (1), timeStretch=Rhythm (1), glitchEngine=Block (1), shapeAOn=Off (0), shapeBOn=Off (0), throwAOn=Off (0), throwBOn=Off (0)
  - BLOCK GLITCH (steps 1-16): 14: Reverse; 16: Repeat

Reject/revise if: Low kick timing moves or the first beat of the next phrase is missing.

Intent check: Use explicit upper-band reverse and repeat steps at the phrase end. Assess against: Low kick timing moves or the first beat of the next phrase is missing.. Original design-stage calibration status: **not_run**; final numerical results and individual owner-audition status are in RACK_BANK_REPORT.md.

### HR179 - DnB Modern Sub Level

**Bass / 174 BPM.** Source: a monophonic bass phrase with low and high notes and sustained tails

Hold a sine-like sub steady with no stereo motion below the crossover.

EQ: LowCut 24 Hz / +0 dB / Q 0.707; HighCut 5000 Hz / +0 dB / Q 0.707

Full range:

- HamloComp (amount 1): thresholdDb=-25, ratio=2.5, attackMs=30, releaseMs=130, model=Opto (3), source=All (0), kneeDb=6, detector=RMS (1), character=Clean (0), autoMakeup=Off (0), makeupDb=0, mix=1
- HamloShirota (amount 1): v2Mode=Stereo / Space (1), v2Width=1, v2LowWidth=0, v2MidWidth=1, v2HighWidth=1, v2LowSplit=300, v2HighSplit=3000, v2MonoBass=180, v2Generate=0, v2Guard=On (1), v2Floor=0.15, v2Mix=1, v2Output=0, v2Motion=Off (0), v2PositionOn=Off (0), v2Mono=Stereo (0)

Reject/revise if: The compressor modulates pitch or low stereo energy remains.

Intent check: Hold a sine-like sub steady with no stereo motion below the crossover. Assess against: The compressor modulates pitch or low stereo energy remains.. Original design-stage calibration status: **not_run**; final numerical results and individual owner-audition status are in RACK_BANK_REPORT.md.

### HR180 - DnB Roller Mid Translation

**Bass / 174 BPM.** Source: a monophonic bass phrase with low and high notes and sustained tails

Add restrained audible harmonics above an undriven low sub.

EQ: LowCut 26 Hz / +0 dB / Q 0.707

Full range:

- HamloMag (amount 0.35): model=Iron (3), driveDb=6, mix=1, trimDb=0, oversampling=8x (3), autoGain=On (1), harmonics=0.35, tone=0, evenOdd=0, tapeBias=0.5, speed=1 (0), wow=0, hiss=0, hum=0, crackle=0, noiseFollow=Follow (1), punish=Off (0), thump=Off (0), steep=Off (0), sampler=Off (0), phoneBass=0.2, subProtectHz=95, dynamics=0, match=Off (0), delta=Off (0)
- HamloComp (amount 1): thresholdDb=-24, ratio=2.5, attackMs=20, releaseMs=86, model=VCA (1), source=All (0), kneeDb=6, detector=RMS (1), character=Clean (0), autoMakeup=Off (0), makeupDb=0, mix=1

Reject/revise if: The added mids hide the snare or the deepest note distorts.

Intent check: Add restrained audible harmonics above an undriven low sub. Assess against: The added mids hide the snare or the deepest note distorts.. Original design-stage calibration status: **not_run**; final numerical results and individual owner-audition status are in RACK_BANK_REPORT.md.

### HR181 - DnB Liquid Warm Sustain

**Bass / 174 BPM.** Source: a monophonic bass phrase with low and high notes and sustained tails

Level a soft bass phrase with smooth opto recovery and tape warmth.

EQ: LowCut 25 Hz / +0 dB / Q 0.707; Bell 300 Hz / -1 dB / Q 1

Full range:

- HamloComp (amount 1): thresholdDb=-25, ratio=2.2, attackMs=35, releaseMs=170, model=Opto (3), source=All (0), kneeDb=6, detector=RMS (1), character=Clean (0), autoMakeup=Off (0), makeupDb=0, mix=1
- HamloMag (amount 0.25): model=Tape (1), driveDb=3, mix=1, trimDb=0, oversampling=8x (3), autoGain=On (1), harmonics=0.35, tone=0, evenOdd=0, tapeBias=0.5, speed=1 (0), wow=0, hiss=0, hum=0, crackle=0, noiseFollow=Follow (1), punish=Off (0), thump=Off (0), steep=Off (0), sampler=Off (0), phoneBass=0, subProtectHz=75, dynamics=0, match=Off (0), delta=Off (0)

Reject/revise if: The next note ducks noticeably or low sustain becomes woolly.

Intent check: Level a soft bass phrase with smooth opto recovery and tape warmth. Assess against: The next note ducks noticeably or low sustain becomes woolly.. Original design-stage calibration status: **not_run**; final numerical results and individual owner-audition status are in RACK_BANK_REPORT.md.

### HR182 - DnB Neuro Clean Sub Dirty Top

**Bass / 174 BPM.** Source: a monophonic bass phrase with low and high notes and sustained tails

Separate aggressive growl treatment from a mono sub anchor.

EQ: LowCut 25 Hz / +0 dB / Q 0.707

Full range:

- HamloShirota (amount 1): v2Mode=Stereo / Space (1), v2Width=1, v2LowWidth=0, v2MidWidth=1.08, v2HighWidth=1.15, v2LowSplit=300, v2HighSplit=3000, v2MonoBass=170, v2Generate=0, v2Guard=On (1), v2Floor=0.15, v2Mix=1, v2Output=0, v2Motion=Off (0), v2PositionOn=Off (0), v2Mono=Stereo (0)

Band 180-8000 Hz:

- HamloMag (amount 0.4): model=Germanium (4), driveDb=14, mix=1, trimDb=0, oversampling=8x (3), autoGain=On (1), harmonics=0.35, tone=0, evenOdd=0, tapeBias=0.5, speed=1 (0), wow=0, hiss=0, hum=0, crackle=0, noiseFollow=Follow (1), punish=Off (0), thump=Off (0), steep=Off (0), sampler=Off (0), phoneBass=0, subProtectHz=0, dynamics=0, match=Off (0), delta=Off (0)
- HamloComp (amount 1): thresholdDb=-28, ratio=4, attackMs=8, releaseMs=65, model=FET (2), source=All (0), kneeDb=6, detector=RMS (1), character=Clean (0), autoMakeup=Off (0), makeupDb=0, mix=1

Reject/revise if: Distortion leaks into the low fundamental or growl width cancels in mono.

Intent check: Separate aggressive growl treatment from a mono sub anchor. Assess against: Distortion leaks into the low fundamental or growl width cancels in mono.. Original design-stage calibration status: **not_run**; final numerical results and individual owner-audition status are in RACK_BANK_REPORT.md.

### HR183 - DnB Jump Up Tempo Pocket

**Bass / 174 BPM.** Source: a monophonic bass phrase with low and high notes and sustained tails

Dip the bass envelope on quarter-note starts without requiring a key bus.

EQ: LowCut 28 Hz / +0 dB / Q 0.707

Full range:

- HamloFader (amount 1): engine=Modern (1), enabled=On (1), mode=Motion (3), gainDb=0, targetDb=-18, mix=1, polarity=Off (0), mute=Off (0), solo=Off (0), inputDb=0, outputDb=0, leftDb=0, rightDb=0, matchedDb=0, peakGuard=On (1), matchCeiling=-1, strength=0.55, maxBoost=3, maxCut=6, floorDb=-55, holdMs=100, riseMs=250, fallMs=140, depthDb=5.5, rightPhase=0, shape=Custom (0), points=4, sync=Sync (1), beats=1, edgeMs=3, smooth=Smooth (1), level00=0, level01=0.5, level02=1, level03=1
- HamloMag (amount 0.3): model=Tube (2), driveDb=5, mix=1, trimDb=0, oversampling=8x (3), autoGain=On (1), harmonics=0.35, tone=0, evenOdd=0, tapeBias=0.5, speed=1 (0), wow=0, hiss=0, hum=0, crackle=0, noiseFollow=Follow (1), punish=Off (0), thump=Off (0), steep=Off (0), sampler=Off (0), phoneBass=0, subProtectHz=85, dynamics=0, match=Off (0), delta=Off (0)
- HamloShirota (amount 1): v2Mode=Stereo / Space (1), v2Width=1, v2LowWidth=0, v2MidWidth=1, v2HighWidth=1.05, v2LowSplit=300, v2HighSplit=3000, v2MonoBass=180, v2Generate=0, v2Guard=On (1), v2Floor=0.15, v2Mix=1, v2Output=0, v2Motion=Off (0), v2PositionOn=Off (0), v2Mono=Stereo (0)

Reject/revise if: The dip drifts with BPM or the offbeat never returns to full level.

Intent check: Dip the bass envelope on quarter-note starts without requiring a key bus. Assess against: The dip drifts with BPM or the offbeat never returns to full level.. Original design-stage calibration status: **not_run**; final numerical results and individual owner-audition status are in RACK_BANK_REPORT.md.

### HR184 - DnB Modern Topline Seat

**Vocal / 174 BPM.** Source: a dry lead phrase with plosives, sibilance and a quiet ending

Control a sung lead with fast recovery and bounded sibilance.

EQ: LowCut 100 Hz / +0 dB / Q 0.707; Bell 380 Hz / -1.5 dB / Q 1; Bell 7000 Hz / +0 dB / Q 1 (dynamic -3 dB)

Full range:

- HamloFader (amount 1): engine=Modern (1), enabled=On (1), mode=Ride (2), gainDb=0, targetDb=-18, mix=1, polarity=Off (0), mute=Off (0), solo=Off (0), inputDb=0, outputDb=0, leftDb=0, rightDb=0, matchedDb=0, peakGuard=On (1), matchCeiling=-1, strength=0.45, maxBoost=2.5, maxCut=6, floorDb=-55, holdMs=100, riseMs=250, fallMs=140, depthDb=0, rightPhase=0
- HamloComp (amount 1): thresholdDb=-24, ratio=2.8, attackMs=9, releaseMs=90, model=FET (2), source=Voc (3), kneeDb=6, detector=RMS (1), character=Clean (0), autoMakeup=Off (0), makeupDb=0, mix=1

Reject/revise if: The topline pumps at drum hits or sibilants become piercing.

Intent check: Control a sung lead with fast recovery and bounded sibilance. Assess against: The topline pumps at drum hits or sibilants become piercing.. Original design-stage calibration status: **not_run**; final numerical results and individual owner-audition status are in RACK_BANK_REPORT.md.

### HR185 - DnB Liquid Hall Lift

**Vocal / 174 BPM.** Source: a dry lead phrase with plosives, sibilance and a quiet ending

Give a melodic lead a ducked hall that blooms in phrase gaps.

EQ: LowCut 110 Hz / +0 dB / Q 0.707; Bell 6500 Hz / +0 dB / Q 1 (dynamic -2.5 dB)

Full range:

- HamloComp (amount 1): thresholdDb=-24, ratio=2.3, attackMs=16, releaseMs=150, model=Opto (3), source=Voc (3), kneeDb=6, detector=RMS (1), character=Clean (0), autoMakeup=Off (0), makeupDb=0, mix=1
- HamloVerb (amount 0.17): space=Hall (3), engine=Velvet (3), decaySeconds=1.5, predelayMs=35, predelaySync=Free (0), lowCutHz=450, highCutHz=9000, bassMult=0.75, hfDecayRatio=0.5, width=1.1, modDepth=0.1, duck=0.5, routing=Send (1), mix=1, keepDry=Off (0), wetDb=0, outputDb=0, earlyDb=0, lateDb=0, timeShape=Off (0), shimmerAmount=0, freeze=Off (0)

Reject/revise if: The hall hides fast words or low tail covers the bass.

Intent check: Give a melodic lead a ducked hall that blooms in phrase gaps. Assess against: The hall hides fast words or low tail covers the bass.. Original design-stage calibration status: **not_run**; final numerical results and individual owner-audition status are in RACK_BANK_REPORT.md.

### HR186 - DnB Roller MC Close

**Vocal / 174 BPM.** Source: a dry lead phrase with plosives, sibilance and a quiet ending

Keep a fast MC phrase dry and intelligible with limited leveling boost.

EQ: LowCut 105 Hz / +0 dB / Q 0.707; Bell 300 Hz / -1.5 dB / Q 1; Bell 7200 Hz / +0 dB / Q 1.2 (dynamic -3.5 dB)

Full range:

- HamloZastava (amount 1): v2_engine=Clean (1), v2_enabled=On (1), v2_threshold=-50, v2_range=8, v2_hold=50, v2_release=80, v2_attack=1, v2_hysteresis=4, v2_knee=6, v2_external=Main (0), v2_expand=Expand (1), v2_ratio=2, v2_link=1, v2_mix=1, v2_output=0, v2_body=50, v2_tail=80, v2_lookahead=2, v2_hp=0, v2_lp=0, v2_listen=Off (0)
- HamloComp (amount 1): thresholdDb=-25, ratio=3, attackMs=5, releaseMs=65, model=FET (2), source=Voc (3), kneeDb=6, detector=RMS (1), character=Clean (0), autoMakeup=Off (0), makeupDb=0, mix=1
- HamloMag (amount 0.15): model=Tube (2), driveDb=2, mix=1, trimDb=0, oversampling=8x (3), autoGain=On (1), harmonics=0.35, tone=0, evenOdd=0, tapeBias=0.5, speed=1 (0), wow=0, hiss=0, hum=0, crackle=0, noiseFollow=Follow (1), punish=Off (0), thump=Off (0), steep=Off (0), sampler=Off (0), phoneBass=0, subProtectHz=0, dynamics=0, match=Off (0), delta=Off (0)

Reject/revise if: Quiet word endings disappear or breaths are raised into the groove.

Intent check: Keep a fast MC phrase dry and intelligible with limited leveling boost. Assess against: Quiet word endings disappear or breaths are raised into the groove.. Original design-stage calibration status: **not_run**; final numerical results and individual owner-audition status are in RACK_BANK_REPORT.md.

### HR187 - DnB Neuro Short Radio Call

**Vocal / 174 BPM.** Source: a dry lead phrase with plosives, sibilance and a quiet ending

Make a compact gritty band-limited call with a single sync reply.

EQ: LowCut 450 Hz / +0 dB / Q 0.707; HighCut 4800 Hz / +0 dB / Q 0.707

Full range:

- HamloMag (amount 0.22): model=Germanium (4), driveDb=7, mix=1, trimDb=0, oversampling=8x (3), autoGain=On (1), harmonics=0.35, tone=0, evenOdd=0, tapeBias=0.5, speed=1 (0), wow=0, hiss=0, hum=0, crackle=0, noiseFollow=Follow (1), punish=Off (0), thump=Off (0), steep=Off (0), sampler=Off (0), phoneBass=0, subProtectHz=0, dynamics=0, match=Off (0), delta=Off (0)
- HamloEcho (amount 0.12): echoMode=Score (1), quickEnabled=Off (0), scoreRouting=Send (1), scoreMix=1, scoreOutput=0, scoreSource=60, scoreFeedback=0.1, scoreFeedbackSync=1/8 (5), scoreFeedbackMs=500, scoreFeedbackPitch=0, scoreColor=BBD (3), scoreDrive=0, scoreLowCut=700, scoreHighCut=4000, scoreWidth=1, scoreDuck=0.2, scoreFreeze=Off (0), scoreSpaceMix=0, scoreGlitch=Off (0), scoreGlitchAmount=0, scoreBands=Off (0), scoreSwing=0, tap1On=1, tap2On=0, tap3On=0, tap4On=0, tap5On=0, tap6On=0, tap7On=0, tap8On=0, tap1Sync=1/8 (5), tap1Time=250, tap1Note=60, tap1Gain=-6, tap1Pan=0, tap1Shape=Forward (0), tap1Fragment=125, tap1Repeats=3, tap1Low=700, tap1High=4000, tap1SpaceSend=0

Reject/revise if: The call becomes unintelligible or repeats obscure the next snare.

Intent check: Make a compact gritty band-limited call with a single sync reply. Assess against: The call becomes unintelligible or repeats obscure the next snare.. Original design-stage calibration status: **not_run**; final numerical results and individual owner-audition status are in RACK_BANK_REPORT.md.

### HR188 - DnB Break Reversed Tail

**Vocal / 174 BPM.** Source: a dry lead phrase with plosives, sibilance and a quiet ending

Reverse a quiet end-of-phrase response without reversing dry words.

EQ: LowCut 140 Hz / +0 dB / Q 0.707; Bell 6800 Hz / +0 dB / Q 1 (dynamic -2.5 dB)

Full range:

- HamloComp (amount 1): thresholdDb=-23, ratio=2.2, attackMs=12, releaseMs=100, model=Opto (3), source=Voc (3), kneeDb=6, detector=RMS (1), character=Clean (0), autoMakeup=Off (0), makeupDb=0, mix=1
- HamloEcho (amount 0.13): echoMode=Score (1), quickEnabled=Off (0), scoreRouting=Send (1), scoreMix=1, scoreOutput=0, scoreSource=60, scoreFeedback=0.08, scoreFeedbackSync=1/4 (8), scoreFeedbackMs=500, scoreFeedbackPitch=0, scoreColor=Clean (0), scoreDrive=0, scoreLowCut=180, scoreHighCut=8000, scoreWidth=1, scoreDuck=0.5, scoreFreeze=Off (0), scoreSpaceMix=0, scoreGlitch=Off (0), scoreGlitchAmount=0, scoreBands=Off (0), scoreSwing=0, tap1On=1, tap2On=0, tap3On=0, tap4On=0, tap5On=0, tap6On=0, tap7On=0, tap8On=0, tap1Sync=1/4 (8), tap1Time=250, tap1Note=60, tap1Gain=-12, tap1Pan=0.2, tap1Shape=Reverse (1), tap1Fragment=125, tap1Repeats=3, tap1Low=180, tap1High=8000, tap1SpaceSend=0

Reject/revise if: Dry consonants reverse or the response arrives before its capture is available.

Intent check: Reverse a quiet end-of-phrase response without reversing dry words. Assess against: Dry consonants reverse or the response arrives before its capture is available.. Original design-stage calibration status: **not_run**; final numerical results and individual owner-audition status are in RACK_BANK_REPORT.md.

### HR189 - DnB Liquid Piano Bloom

**Keys / 174 BPM.** Source: a dry chord phrase with staccato and held notes

Keep piano hammers clean while a filtered hall supports the chords.

EQ: LowCut 140 Hz / +0 dB / Q 0.707; Bell 330 Hz / -1.3 dB / Q 1

Full range:

- HamloComp (amount 1): thresholdDb=-23, ratio=1.8, attackMs=28, releaseMs=170, model=Opto (3), source=Samples (2), kneeDb=6, detector=RMS (1), character=Clean (0), autoMakeup=Off (0), makeupDb=0, mix=1
- HamloVerb (amount 0.12): space=Hall (3), engine=Velvet (3), decaySeconds=1.4, predelayMs=26, predelaySync=Free (0), lowCutHz=400, highCutHz=8500, bassMult=0.75, hfDecayRatio=0.5, width=1.1, modDepth=0.1, duck=0.15, routing=Send (1), mix=1, keepDry=Off (0), wetDb=0, outputDb=0, earlyDb=0, lateDb=0, timeShape=Off (0), shimmerAmount=0, freeze=Off (0)

Reject/revise if: Hall masks new chord attacks or low-mid sustain competes with bass.

Intent check: Keep piano hammers clean while a filtered hall supports the chords. Assess against: Hall masks new chord attacks or low-mid sustain competes with bass.. Original design-stage calibration status: **not_run**; final numerical results and individual owner-audition status are in RACK_BANK_REPORT.md.

### HR190 - DnB Roller Stab Cut

**Keys / 174 BPM.** Source: a dry chord phrase with staccato and held notes

Make chord stabs short enough for a fast rolling arrangement.

EQ: LowCut 200 Hz / +0 dB / Q 0.707; Bell 550 Hz / -1.5 dB / Q 1

Full range:

- HamloUdarnik (amount 1): attackDb=1, sustainDb=-2, speed=0.85, style=PRAVDA (0), engine=V2 (1), protect=On (1), lookahead=Vostok (1), listen=Full (0), rise=1, decay=1, sensitivity=0.8, preserve=1, mix=1, outputDb=0
- HamloMag (amount 0.25): model=Iron (3), driveDb=4, mix=1, trimDb=0, oversampling=8x (3), autoGain=On (1), harmonics=0.35, tone=0, evenOdd=0, tapeBias=0.5, speed=1 (0), wow=0, hiss=0, hum=0, crackle=0, noiseFollow=Follow (1), punish=Off (0), thump=Off (0), steep=Off (0), sampler=Off (0), phoneBass=0, subProtectHz=0, dynamics=0, match=Off (0), delta=Off (0)
- HamloComp (amount 1): thresholdDb=-23, ratio=2, attackMs=14, releaseMs=70, model=VCA (1), source=Samples (2), kneeDb=6, detector=RMS (1), character=Clean (0), autoMakeup=Off (0), makeupDb=0, mix=1

Reject/revise if: Stab body disappears or release pumps on the next sixteenth.

Intent check: Make chord stabs short enough for a fast rolling arrangement. Assess against: Stab body disappears or release pumps on the next sixteenth.. Original design-stage calibration status: **not_run**; final numerical results and individual owner-audition status are in RACK_BANK_REPORT.md.

### HR191 - DnB Neuro Pluck Focus

**Keys / 174 BPM.** Source: a dry chord phrase with staccato and held notes

Constrain a bright synth pluck before adding mid-band grit.

EQ: LowCut 180 Hz / +0 dB / Q 0.707; Bell 5200 Hz / +0 dB / Q 1.4 (dynamic -2.5 dB)

Full range:

- HamloMag (amount 0.25): model=Germanium (4), driveDb=8, mix=1, trimDb=0, oversampling=8x (3), autoGain=On (1), harmonics=0.35, tone=0, evenOdd=0, tapeBias=0.5, speed=1 (0), wow=0, hiss=0, hum=0, crackle=0, noiseFollow=Follow (1), punish=Off (0), thump=Off (0), steep=Off (0), sampler=Off (0), phoneBass=0, subProtectHz=180, dynamics=0, match=Off (0), delta=Off (0)
- HamloUdarnik (amount 1): attackDb=0.8, sustainDb=-1, speed=0.85, style=PRAVDA (0), engine=V2 (1), protect=On (1), lookahead=Vostok (1), listen=Full (0), rise=1, decay=1, sensitivity=0.8, preserve=1, mix=1, outputDb=0

Reject/revise if: Hard harmonics dominate the pluck pitch or its attack disappears.

Intent check: Constrain a bright synth pluck before adding mid-band grit. Assess against: Hard harmonics dominate the pluck pitch or its attack disappears.. Original design-stage calibration status: **not_run**; final numerical results and individual owner-audition status are in RACK_BANK_REPORT.md.

### HR192 - DnB Liquid Rhodes Drifts

**Keys / 174 BPM.** Source: a dry chord phrase with staccato and held notes

Add quiet alternating eighth echoes behind a warm Rhodes chord.

EQ: LowCut 150 Hz / +0 dB / Q 0.707

Full range:

- HamloMag (amount 0.2): model=Tube (2), driveDb=3, mix=1, trimDb=0, oversampling=8x (3), autoGain=On (1), harmonics=0.35, tone=0, evenOdd=0, tapeBias=0.5, speed=1 (0), wow=0, hiss=0, hum=0, crackle=0, noiseFollow=Follow (1), punish=Off (0), thump=Off (0), steep=Off (0), sampler=Off (0), phoneBass=0, subProtectHz=0, dynamics=0, match=Off (0), delta=Off (0)
- HamloEcho (amount 0.1): echoMode=Score (1), quickEnabled=Off (0), scoreRouting=Send (1), scoreMix=1, scoreOutput=0, scoreSource=60, scoreFeedback=0.16, scoreFeedbackSync=1/8 (5), scoreFeedbackMs=500, scoreFeedbackPitch=0, scoreColor=Warm (1), scoreDrive=0, scoreLowCut=600, scoreHighCut=7000, scoreWidth=1, scoreDuck=0.2, scoreFreeze=Off (0), scoreSpaceMix=0, scoreGlitch=Off (0), scoreGlitchAmount=0, scoreBands=Off (0), scoreSwing=0, tap1On=1, tap2On=1, tap3On=0, tap4On=0, tap5On=0, tap6On=0, tap7On=0, tap8On=0, tap1Sync=1/8 (5), tap1Time=250, tap1Note=60, tap1Gain=-11, tap1Pan=-0.5, tap1Shape=Forward (0), tap1Fragment=125, tap1Repeats=3, tap1Low=600, tap1High=7000, tap1SpaceSend=0, tap2Sync=1/4 (8), tap2Time=500, tap2Note=60, tap2Gain=-15, tap2Pan=0.5, tap2Shape=Forward (0), tap2Fragment=125, tap2Repeats=3, tap2Low=600, tap2High=7000, tap2SpaceSend=0

Reject/revise if: Echoes shift chord pitch or mask the following harmony.

Intent check: Add quiet alternating eighth echoes behind a warm Rhodes chord. Assess against: Echoes shift chord pitch or mask the following harmony.. Original design-stage calibration status: **not_run**; final numerical results and individual owner-audition status are in RACK_BANK_REPORT.md.

### HR193 - DnB Break Ghost Preservation

**Samples / 174 BPM.** Source: a musical sample loop with gaps and a clear phrase ending

Glue a vintage break sample while preserving low-level ghost articulation.

EQ: LowCut 35 Hz / +0 dB / Q 0.707; Bell 450 Hz / -1 dB / Q 1

Full range:

- HamloMag (amount 0.25): model=Tape (1), driveDb=3, mix=1, trimDb=0, oversampling=8x (3), autoGain=On (1), harmonics=0.35, tone=0, evenOdd=0, tapeBias=0.5, speed=1 (0), wow=0, hiss=0, hum=0, crackle=0, noiseFollow=Follow (1), punish=Off (0), thump=Off (0), steep=Off (0), sampler=MPC60 (2), phoneBass=0, subProtectHz=0, dynamics=0, match=Off (0), delta=Off (0)
- HamloComp (amount 1): thresholdDb=-25, ratio=1.8, attackMs=24, releaseMs=95, model=Opto (3), source=Samples (2), kneeDb=12, detector=RMS (1), character=Clean (0), autoMakeup=Off (0), makeupDb=0, mix=1

Reject/revise if: The sampler erases soft notes or accents lose their natural hierarchy.

Intent check: Glue a vintage break sample while preserving low-level ghost articulation. Assess against: The sampler erases soft notes or accents lose their natural hierarchy.. Original design-stage calibration status: **not_run**; final numerical results and individual owner-audition status are in RACK_BANK_REPORT.md.

### HR194 - DnB Neuro Last Eighth Flip

**Samples / 174 BPM.** Source: a musical sample loop with gaps and a clear phrase ending

Reverse only the final eighth of a melodic sample.

EQ: LowCut 120 Hz / +0 dB / Q 0.707

Full range:

- HamloRevolution (amount 0.5): mix=1, free=Sync (0), grid=1/16 (2), triplet=Off (0), swing=0, humanize=0, fadeMs=3, order=T-G-M (0), scene=1 (0), listen=Off (0), timeOn=On (1), glitchOn=Off (0), motionOn=Off (0), timeMix=1, timeDepth=1, glitchMix=1, glitchDepth=0.5, motionMix=1, motionDepth=0.35, motionSmooth=0.5, motionTarget=Low Pass (1), timeStretch=Clean (2), glitchEngine=Block (1), shapeAOn=Off (0), shapeBOn=Off (0), throwAOn=Off (0), throwBOn=Off (0)
  - TIME (steps 1-16): 15: Reverse; 16: Reverse

Reject/revise if: Reversal changes the next phrase start or low notes phase-cancel.

Intent check: Reverse only the final eighth of a melodic sample. Assess against: Reversal changes the next phrase start or low notes phase-cancel.. Original design-stage calibration status: **not_run**; final numerical results and individual owner-audition status are in RACK_BANK_REPORT.md.

### HR195 - DnB Liquid Skyline Texture

**Samples / 174 BPM.** Source: a musical sample loop with gaps and a clear phrase ending

Make a quiet atmospheric sample broad above a clean low cut.

EQ: LowCut 250 Hz / +0 dB / Q 0.707; HighShelf 11000 Hz / +0.5 dB / Q 0.707

Full range:

- HamloShirota (amount 1): v2Mode=Stereo / Space (1), v2Width=1, v2LowWidth=0, v2MidWidth=1.2, v2HighWidth=1.35, v2LowSplit=300, v2HighSplit=3000, v2MonoBass=280, v2Generate=0.08, v2Guard=On (1), v2Floor=0.15, v2Mix=1, v2Output=0, v2Motion=Off (0), v2PositionOn=Off (0), v2Mono=Stereo (0), v2Generator=Diffuse (2)
- HamloVerb (amount 0.16): space=Hall (3), engine=Bloom (6), decaySeconds=2, predelayMs=32, predelaySync=Free (0), lowCutHz=600, highCutHz=9000, bassMult=0.75, hfDecayRatio=0.5, width=1.1, modDepth=0.1, duck=0.15, routing=Send (1), mix=1, keepDry=Off (0), wetDb=0, outputDb=0, earlyDb=0, lateDb=0, timeShape=Off (0), shimmerAmount=0, freeze=Off (0)

Reject/revise if: Mono fold loses the main texture or reverb overwhelms transients.

Intent check: Make a quiet atmospheric sample broad above a clean low cut. Assess against: Mono fold loses the main texture or reverb overwhelms transients.. Original design-stage calibration status: **not_run**; final numerical results and individual owner-audition status are in RACK_BANK_REPORT.md.

### HR196 - DnB Roller Rhythmic Fragments

**Samples / 174 BPM.** Source: a musical sample loop with gaps and a clear phrase ending

Cut selected off-grid sample fragments into an explicit 16-step response.

EQ: LowCut 180 Hz / +0 dB / Q 0.707

Full range:

- HamloRevolution (amount 0.4): mix=1, free=Sync (0), grid=1/16 (2), triplet=Off (0), swing=0, humanize=0, fadeMs=3, order=T-G-M (0), scene=1 (0), listen=Off (0), timeOn=Off (0), glitchOn=On (1), motionOn=Off (0), timeMix=1, timeDepth=1, glitchMix=1, glitchDepth=0.3, motionMix=1, motionDepth=0.35, motionSmooth=0.5, motionTarget=Low Pass (1), timeStretch=Rhythm (1), glitchEngine=Block (1), shapeAOn=Off (0), shapeBOn=Off (0), throwAOn=Off (0), throwBOn=Off (0)
  - BLOCK GLITCH (steps 1-16): 4: Repeat; 8: Chop; 12: Repeat; 16: Roll
- HamloVerb (amount 0.035): space=Ambience (6), engine=Crisp (4), decaySeconds=0.24, predelayMs=0, predelaySync=Free (0), lowCutHz=800, highCutHz=6000, bassMult=0.75, hfDecayRatio=0.5, width=1.1, modDepth=0.1, duck=0.15, routing=Send (1), mix=1, keepDry=Off (0), wetDb=0, outputDb=0, earlyDb=0, lateDb=0, timeShape=Off (0), shimmerAmount=0, freeze=Off (0)

Reject/revise if: The fragment pattern sounds randomly dense or loses downbeat identity.

Intent check: Cut selected off-grid sample fragments into an explicit 16-step response. Assess against: The fragment pattern sounds randomly dense or loses downbeat identity.. Original design-stage calibration status: **not_run**; final numerical results and individual owner-audition status are in RACK_BANK_REPORT.md.

### HR197 - DnB Modern Drum Headroom

**Master / 174 BPM.** Source: an unmastered stereo mix with headroom and a busy chorus

Finish a fast full mix with gentle bus control and restrained final limiting.

EQ: LowCut 24 Hz / +0 dB / Q 0.707; Bell 330 Hz / -0.6 dB / Q 1

Full range:

- HamloComp (amount 1): thresholdDb=-18, ratio=1.4, attackMs=35, releaseMs=86, model=VCA (1), source=All (0), kneeDb=6, detector=RMS (1), character=Clean (0), autoMakeup=Off (0), makeupDb=0, mix=1
- HamloShirota (amount 1): v2Mode=Stereo / Space (1), v2Width=1, v2LowWidth=0, v2MidWidth=1, v2HighWidth=1.05, v2LowSplit=300, v2HighSplit=3000, v2MonoBass=120, v2Generate=0, v2Guard=On (1), v2Floor=0.15, v2Mix=1, v2Output=0, v2Motion=Off (0), v2PositionOn=Off (0), v2Mono=Stereo (0)
- HamloRedWall (amount 1): engine=V2 (1), thresholdDb=-2.2, ceilingDb=-1, releaseMs=75, autoRelease=Manual (0), lookaheadMs=2, link=1, truePeak=On (1), style=GLASNOST (0), outputDb=0, drive=0, tube=0, monitor=Normal (0), mix=1

Reject/revise if: Final 8x true peak exceeds -1 dBTP or snare contrast drops before 3 dB work.

Intent check: Finish a fast full mix with gentle bus control and restrained final limiting. Assess against: Final 8x true peak exceeds -1 dBTP or snare contrast drops before 3 dB work.. Original design-stage calibration status: **not_run**; final numerical results and individual owner-audition status are in RACK_BANK_REPORT.md.

### HR198 - DnB Liquid Open Finish

**Master / 174 BPM.** Source: an unmastered stereo mix with headroom and a busy chorus

Retain the open piano and vocal field with a stable low center.

EQ: LowCut 23 Hz / +0 dB / Q 0.707; HighShelf 11500 Hz / +0.4 dB / Q 0.707

Full range:

- HamloComp (amount 1): thresholdDb=-19, ratio=1.4, attackMs=40, releaseMs=130, model=Opto (3), source=All (0), kneeDb=6, detector=RMS (1), character=Clean (0), autoMakeup=Off (0), makeupDb=0, mix=1
- HamloShirota (amount 1): v2Mode=Stereo / Space (1), v2Width=1, v2LowWidth=0.9, v2MidWidth=1.02, v2HighWidth=1.1, v2LowSplit=300, v2HighSplit=3000, v2MonoBass=110, v2Generate=0, v2Guard=On (1), v2Floor=0.15, v2Mix=1, v2Output=0, v2Motion=Off (0), v2PositionOn=Off (0), v2Mono=Stereo (0)
- HamloRedWall (amount 1): engine=V2 (1), thresholdDb=-1.7, ceilingDb=-1, releaseMs=110, autoRelease=Manual (0), lookaheadMs=2, link=1, truePeak=On (1), style=GLASNOST (0), outputDb=0, drive=0, tube=0, monitor=Normal (0), mix=1

Reject/revise if: Mono narrows the hook excessively or the hall pumps on each snare.

Intent check: Retain the open piano and vocal field with a stable low center. Assess against: Mono narrows the hook excessively or the hall pumps on each snare.. Original design-stage calibration status: **not_run**; final numerical results and individual owner-audition status are in RACK_BANK_REPORT.md.

### HR199 - DnB Neuro Controlled Edge

**Master / 174 BPM.** Source: an unmastered stereo mix with headroom and a busy chorus

Tame upper-mid density before a conservative peak ceiling.

EQ: LowCut 25 Hz / +0 dB / Q 0.707; Bell 3600 Hz / +0 dB / Q 1 (dynamic -1.5 dB)

Full range:

- HamloMag (amount 0.12): model=Iron (3), driveDb=2, mix=1, trimDb=0, oversampling=8x (3), autoGain=On (1), harmonics=0.35, tone=0, evenOdd=0, tapeBias=0.5, speed=1 (0), wow=0, hiss=0, hum=0, crackle=0, noiseFollow=Follow (1), punish=Off (0), thump=Off (0), steep=Off (0), sampler=Off (0), phoneBass=0, subProtectHz=130, dynamics=0, match=Off (0), delta=Off (0)
- HamloComp (amount 1): thresholdDb=-19, ratio=1.5, attackMs=32, releaseMs=86, model=VCA (1), source=All (0), kneeDb=6, detector=RMS (1), character=Clean (0), autoMakeup=Off (0), makeupDb=0, mix=1
- HamloRedWall (amount 1): engine=V2 (1), thresholdDb=-2.3, ceilingDb=-1, releaseMs=70, autoRelease=Manual (0), lookaheadMs=2, link=1, truePeak=On (1), style=GLASNOST (0), outputDb=0, drive=0, tube=0, monitor=Normal (0), mix=1

Reject/revise if: The growl loses identity or limiter work exceeds 3 dB to hit target loudness.

Intent check: Tame upper-mid density before a conservative peak ceiling. Assess against: The growl loses identity or limiter work exceeds 3 dB to hit target loudness.. Original design-stage calibration status: **not_run**; final numerical results and individual owner-audition status are in RACK_BANK_REPORT.md.

### HR200 - DnB Roller Low End Finish

**Master / 174 BPM.** Source: an unmastered stereo mix with headroom and a busy chorus

Keep the sub mono and prevent low-mid buildup from dominating the limiter.

EQ: LowCut 24 Hz / +0 dB / Q 0.707; Bell 240 Hz / -0.8 dB / Q 0.85

Full range:

- HamloShirota (amount 1): v2Mode=Stereo / Space (1), v2Width=1, v2LowWidth=0, v2MidWidth=1, v2HighWidth=1.04, v2LowSplit=300, v2HighSplit=3000, v2MonoBass=140, v2Generate=0, v2Guard=On (1), v2Floor=0.15, v2Mix=1, v2Output=0, v2Motion=Off (0), v2PositionOn=Off (0), v2Mono=Stereo (0)
- HamloComp (amount 1): thresholdDb=-20, ratio=1.6, attackMs=30, releaseMs=95, model=VCA (1), source=All (0), kneeDb=6, detector=RMS (1), character=Clean (0), autoMakeup=Off (0), makeupDb=0, mix=1
- HamloRedWall (amount 1): engine=V2 (1), thresholdDb=-2, ceilingDb=-1, releaseMs=85, autoRelease=Manual (0), lookaheadMs=2, link=1, truePeak=On (1), style=GLASNOST (0), outputDb=0, drive=0, tube=0, monitor=Normal (0), mix=1

Reject/revise if: Bass note balance changes in mono or kicks become softer after matching.

Intent check: Keep the sub mono and prevent low-mid buildup from dominating the limiter. Assess against: Bass note balance changes in mono or kicks become softer after matching.. Original design-stage calibration status: **not_run**; final numerical results and individual owner-audition status are in RACK_BANK_REPORT.md.

## Studio

### HR201 - Studio Transparent Lead

**Lead Vocal / 120 BPM.** Source: a dry sung/spoken lead phrase with quiet and loud words

Stabilize phrase level gently before clean vocal compression.

EQ: LowCut 75 Hz / +0 dB / Q 0.707; Bell 300 Hz / -1 dB / Q 1; Bell 7000 Hz / +0 dB / Q 1 (dynamic -2.5 dB)

Full range:

- HamloFader (amount 1): engine=Modern (1), enabled=On (1), mode=Ride (2), gainDb=0, targetDb=-18, mix=1, polarity=Off (0), mute=Off (0), solo=Off (0), inputDb=0, outputDb=0, leftDb=0, rightDb=0, matchedDb=0, peakGuard=On (1), matchCeiling=-1, strength=0.35, maxBoost=2, maxCut=4, floorDb=-55, holdMs=100, riseMs=250, fallMs=140, depthDb=0, rightPhase=0
- HamloComp (amount 1): thresholdDb=-23, ratio=2, attackMs=15, releaseMs=150, model=VCA (1), source=Voc (3), kneeDb=6, detector=RMS (1), character=Clean (0), autoMakeup=Off (0), makeupDb=0, mix=1

Reject/revise if: Quiet breaths rise between words or loud notes lose expression.

Intent check: Stabilize phrase level gently before clean vocal compression. Assess against: Quiet breaths rise between words or loud notes lose expression.. Original design-stage calibration status: **not_run**; final numerical results and individual owner-audition status are in RACK_BANK_REPORT.md.

### HR202 - Studio Intimate Soul

**Lead Vocal / 120 BPM.** Source: a dry sung/spoken lead phrase with quiet and loud words

Smooth close-miked soul singing with soft harmonics and a tiny chamber.

EQ: LowCut 65 Hz / +0 dB / Q 0.707; Bell 240 Hz / -1 dB / Q 1; HighShelf 11000 Hz / +0.5 dB / Q 0.707

Full range:

- HamloComp (amount 1): thresholdDb=-24, ratio=2.3, attackMs=22, releaseMs=230, model=Opto (3), source=Voc (3), kneeDb=6, detector=RMS (1), character=Clean (0), autoMakeup=Off (0), makeupDb=0, mix=1
- HamloMag (amount 0.2): model=Tube (2), driveDb=3, mix=1, trimDb=0, oversampling=8x (3), autoGain=On (1), harmonics=0.35, tone=0, evenOdd=0, tapeBias=0.5, speed=1 (0), wow=0, hiss=0, hum=0, crackle=0, noiseFollow=Follow (1), punish=Off (0), thump=Off (0), steep=Off (0), sampler=Off (0), phoneBass=0, subProtectHz=0, dynamics=0, match=Off (0), delta=Off (0)
- HamloVerb (amount 0.055): space=Chamber (2), engine=Velvet (3), decaySeconds=0.55, predelayMs=20, predelaySync=Free (0), lowCutHz=350, highCutHz=6500, bassMult=0.75, hfDecayRatio=0.5, width=1.1, modDepth=0.1, duck=0.15, routing=Send (1), mix=1, keepDry=Off (0), wetDb=0, outputDb=0, earlyDb=0, lateDb=0, timeShape=Off (0), shimmerAmount=0, freeze=Off (0)

Reject/revise if: Chest body becomes woolly or the vocal sounds farther from the mic.

Intent check: Smooth close-miked soul singing with soft harmonics and a tiny chamber. Assess against: Chest body becomes woolly or the vocal sounds farther from the mic.. Original design-stage calibration status: **not_run**; final numerical results and individual owner-audition status are in RACK_BANK_REPORT.md.

### HR203 - Studio Rock Front Edge

**Lead Vocal / 120 BPM.** Source: a dry sung/spoken lead phrase with quiet and loud words

Blend FET density into a rock lead while keeping strong consonants.

EQ: LowCut 90 Hz / +0 dB / Q 0.707; Bell 450 Hz / -1.5 dB / Q 1; Bell 7500 Hz / +0 dB / Q 1 (dynamic -3 dB)

Full range:

- HamloComp (amount 0.6): thresholdDb=-27, ratio=4, attackMs=8, releaseMs=100, model=FET (2), source=Voc (3), kneeDb=6, detector=RMS (1), character=Clean (0), autoMakeup=Off (0), makeupDb=0, mix=1
- HamloMag (amount 0.2): model=Iron (3), driveDb=5, mix=1, trimDb=0, oversampling=8x (3), autoGain=On (1), harmonics=0.35, tone=0, evenOdd=0, tapeBias=0.5, speed=1 (0), wow=0, hiss=0, hum=0, crackle=0, noiseFollow=Follow (1), punish=Off (0), thump=Off (0), steep=Off (0), sampler=Off (0), phoneBass=0, subProtectHz=0, dynamics=0, match=Off (0), delta=Off (0)
- HamloVerb (amount 0.08): space=Plate (4), engine=Warm (2), decaySeconds=0.75, predelayMs=28, predelaySync=Free (0), lowCutHz=500, highCutHz=6500, bassMult=0.75, hfDecayRatio=0.5, width=1.1, modDepth=0.1, duck=0.15, routing=Send (1), mix=1, keepDry=Off (0), wetDb=0, outputDb=0, earlyDb=0, lateDb=0, timeShape=Off (0), shimmerAmount=0, freeze=Off (0)

Reject/revise if: Shouts flatten or the 3 kHz area becomes painful at matched level.

Intent check: Blend FET density into a rock lead while keeping strong consonants. Assess against: Shouts flatten or the 3 kHz area becomes painful at matched level.. Original design-stage calibration status: **not_run**; final numerical results and individual owner-audition status are in RACK_BANK_REPORT.md.

### HR204 - Studio Spoken Warmth

**Lead Vocal / 120 BPM.** Source: a dry sung/spoken lead phrase with quiet and loud words

Reduce room noise gently and level spoken phrases without a radio crush.

EQ: LowCut 70 Hz / +0 dB / Q 0.707; Bell 220 Hz / -1 dB / Q 1; Bell 6200 Hz / +0 dB / Q 1 (dynamic -2 dB)

Full range:

- HamloZastava (amount 1): v2_engine=Clean (1), v2_enabled=On (1), v2_threshold=-52, v2_range=8, v2_hold=90, v2_release=150, v2_attack=2, v2_hysteresis=4, v2_knee=6, v2_external=Main (0), v2_expand=Expand (1), v2_ratio=2, v2_link=1, v2_mix=1, v2_output=0, v2_body=90, v2_tail=150, v2_lookahead=2, v2_hp=0, v2_lp=0, v2_listen=Off (0)
- HamloFader (amount 1): engine=Modern (1), enabled=On (1), mode=Ride (2), gainDb=0, targetDb=-21, mix=1, polarity=Off (0), mute=Off (0), solo=Off (0), inputDb=0, outputDb=0, leftDb=0, rightDb=0, matchedDb=0, peakGuard=On (1), matchCeiling=-1, strength=0.4, maxBoost=2, maxCut=6, floorDb=-55, holdMs=100, riseMs=250, fallMs=140, depthDb=0, rightPhase=0
- HamloComp (amount 1): thresholdDb=-23, ratio=2, attackMs=18, releaseMs=180, model=Opto (3), source=Voc (3), kneeDb=6, detector=RMS (1), character=Clean (0), autoMakeup=Off (0), makeupDb=0, mix=1

Reject/revise if: The gate removes word endings or breaths audibly swell.

Intent check: Reduce room noise gently and level spoken phrases without a radio crush. Assess against: The gate removes word endings or breaths audibly swell.. Original design-stage calibration status: **not_run**; final numerical results and individual owner-audition status are in RACK_BANK_REPORT.md.

### HR205 - Studio Dry Fast Verse

**Lead Vocal / 120 BPM.** Source: a dry sung/spoken lead phrase with quiet and loud words

Make a fast dry rap verse readable with quick release and no added space.

EQ: LowCut 85 Hz / +0 dB / Q 0.707; Bell 350 Hz / -1.8 dB / Q 1; Bell 7100 Hz / +0 dB / Q 1.2 (dynamic -3.5 dB)

Full range:

- HamloComp (amount 1): thresholdDb=-25, ratio=3.2, attackMs=6, releaseMs=80, model=FET (2), source=Voc (3), kneeDb=6, detector=RMS (1), character=Clean (0), autoMakeup=Off (0), makeupDb=0, mix=1
- HamloMag (amount 0.12): model=Tube (2), driveDb=2, mix=1, trimDb=0, oversampling=8x (3), autoGain=On (1), harmonics=0.35, tone=0, evenOdd=0, tapeBias=0.5, speed=1 (0), wow=0, hiss=0, hum=0, crackle=0, noiseFollow=Follow (1), punish=Off (0), thump=Off (0), steep=Off (0), sampler=Off (0), phoneBass=0, subProtectHz=0, dynamics=0, match=Off (0), delta=Off (0)

Reject/revise if: Fast consonants smear or the final word ducks after an accent.

Intent check: Make a fast dry rap verse readable with quick release and no added space. Assess against: Fast consonants smear or the final word ducks after an accent.. Original design-stage calibration status: **not_run**; final numerical results and individual owner-audition status are in RACK_BANK_REPORT.md.

### HR206 - Studio Ballad Open Hall

**Lead Vocal / 120 BPM.** Source: a dry sung/spoken lead phrase with quiet and loud words

Keep the lead ahead of a wide hall that opens in sustained gaps.

EQ: LowCut 80 Hz / +0 dB / Q 0.707; Bell 6800 Hz / +0 dB / Q 1 (dynamic -2.5 dB)

Full range:

- HamloComp (amount 1): thresholdDb=-24, ratio=2, attackMs=25, releaseMs=240, model=Opto (3), source=Voc (3), kneeDb=6, detector=RMS (1), character=Clean (0), autoMakeup=Off (0), makeupDb=0, mix=1
- HamloVerb (amount 0.18): space=Hall (3), engine=Bloom (6), decaySeconds=2.2, predelayMs=55, predelaySync=Free (0), lowCutHz=450, highCutHz=9000, bassMult=0.75, hfDecayRatio=0.5, width=1.1, modDepth=0.1, duck=0.55, routing=Send (1), mix=1, keepDry=Off (0), wetDb=0, outputDb=0, earlyDb=0, lateDb=0, timeShape=Off (0), shimmerAmount=0, freeze=Off (0)

Reject/revise if: Consonants sink into the hall or the tail clouds the next line.

Intent check: Keep the lead ahead of a wide hall that opens in sustained gaps. Assess against: Consonants sink into the hall or the tail clouds the next line.. Original design-stage calibration status: **not_run**; final numerical results and individual owner-audition status are in RACK_BANK_REPORT.md.

### HR207 - Studio Whisper Support

**Lead Vocal / 120 BPM.** Source: a dry sung/spoken lead phrase with quiet and loud words

Support quiet whisper syllables with a bounded rider and soft air control.

EQ: LowCut 110 Hz / +0 dB / Q 0.707; Bell 8500 Hz / +0 dB / Q 1.2 (dynamic -3 dB)

Full range:

- HamloFader (amount 1): engine=Modern (1), enabled=On (1), mode=Ride (2), gainDb=0, targetDb=-22, mix=1, polarity=Off (0), mute=Off (0), solo=Off (0), inputDb=0, outputDb=0, leftDb=0, rightDb=0, matchedDb=0, peakGuard=On (1), matchCeiling=-1, strength=0.3, maxBoost=2, maxCut=3, floorDb=-48, holdMs=100, riseMs=350, fallMs=220, depthDb=0, rightPhase=0
- HamloComp (amount 1): thresholdDb=-26, ratio=1.8, attackMs=18, releaseMs=200, model=Opto (3), source=Voc (3), kneeDb=6, detector=RMS (1), character=Clean (0), autoMakeup=Off (0), makeupDb=0, mix=1

Reject/revise if: Mouth noise rises or the rider hunts during silence.

Intent check: Support quiet whisper syllables with a bounded rider and soft air control. Assess against: Mouth noise rises or the rider hunts during silence.. Original design-stage calibration status: **not_run**; final numerical results and individual owner-audition status are in RACK_BANK_REPORT.md.

### HR208 - Studio Small Room Truth

**Lead Vocal / 120 BPM.** Source: a dry sung/spoken lead phrase with quiet and loud words

Put a dry vocal in a believable short room without washing out detail.

EQ: LowCut 85 Hz / +0 dB / Q 0.707; Bell 350 Hz / -1 dB / Q 1

Full range:

- HamloComp (amount 1): thresholdDb=-23, ratio=2, attackMs=16, releaseMs=160, model=VCA (1), source=Voc (3), kneeDb=6, detector=RMS (1), character=Clean (0), autoMakeup=Off (0), makeupDb=0, mix=1
- HamloVerb (amount 0.075): space=Room (1), engine=Clean (1), decaySeconds=0.38, predelayMs=8, predelaySync=Free (0), lowCutHz=350, highCutHz=8000, bassMult=0.75, hfDecayRatio=0.5, width=1.1, modDepth=0.1, duck=0.15, routing=Send (1), mix=1, keepDry=Off (0), wetDb=0, outputDb=0, earlyDb=1, lateDb=-4, timeShape=Off (0), shimmerAmount=0, freeze=Off (0)

Reject/revise if: Room sounds like slapback or obscures word onsets.

Intent check: Put a dry vocal in a believable short room without washing out detail. Assess against: Room sounds like slapback or obscures word onsets.. Original design-stage calibration status: **not_run**; final numerical results and individual owner-audition status are in RACK_BANK_REPORT.md.

### HR209 - Studio Telephone Bridge

**Lead Vocal / 120 BPM.** Source: a dry sung/spoken lead phrase with quiet and loud words

Make an intentional band-limited bridge voice with controlled drive.

EQ: LowCut 350 Hz / +0 dB / Q 0.707; HighCut 3700 Hz / +0 dB / Q 0.707; Bell 1600 Hz / +1 dB / Q 1; Bell 2600 Hz / -1 dB / Q 1

Full range:

- HamloComp (amount 1): thresholdDb=-26, ratio=3.5, attackMs=5, releaseMs=90, model=FET (2), source=Voc (3), kneeDb=6, detector=RMS (1), character=Clean (0), autoMakeup=Off (0), makeupDb=0, mix=1
- HamloMag (amount 0.2): model=Germanium (4), driveDb=8, mix=1, trimDb=0, oversampling=8x (3), autoGain=On (1), harmonics=0.35, tone=0, evenOdd=0, tapeBias=0.5, speed=1 (0), wow=0, hiss=0, hum=0, crackle=0, noiseFollow=Follow (1), punish=Off (0), thump=Off (0), steep=Off (0), sampler=Off (0), phoneBass=0, subProtectHz=0, dynamics=0, match=Off (0), delta=Off (0)

Reject/revise if: The bridge words stop being intelligible or resonant mids hurt.

Intent check: Make an intentional band-limited bridge voice with controlled drive. Assess against: The bridge words stop being intelligible or resonant mids hurt.. Original design-stage calibration status: **not_run**; final numerical results and individual owner-audition status are in RACK_BANK_REPORT.md.

### HR210 - Studio Fifth Answer

**Lead Vocal / 120 BPM.** Source: a dry sung/spoken lead phrase with quiet and loud words

Answer the end of a phrase with a quiet fifth-up sync reflection.

EQ: LowCut 100 Hz / +0 dB / Q 0.707; Bell 7000 Hz / +0 dB / Q 1 (dynamic -2.5 dB)

Full range:

- HamloComp (amount 1): thresholdDb=-24, ratio=2.5, attackMs=12, releaseMs=135, model=Opto (3), source=Voc (3), kneeDb=6, detector=RMS (1), character=Clean (0), autoMakeup=Off (0), makeupDb=0, mix=1
- HamloEcho (amount 0.12): echoMode=Score (1), quickEnabled=Off (0), scoreRouting=Send (1), scoreMix=1, scoreOutput=0, scoreSource=60, scoreFeedback=0.12, scoreFeedbackSync=1/4 (8), scoreFeedbackMs=500, scoreFeedbackPitch=0, scoreColor=Clean (0), scoreDrive=0, scoreLowCut=180, scoreHighCut=8000, scoreWidth=1, scoreDuck=0.6, scoreFreeze=Off (0), scoreSpaceMix=0, scoreGlitch=Off (0), scoreGlitchAmount=0, scoreBands=Off (0), scoreSwing=0, tap1On=1, tap2On=0, tap3On=0, tap4On=0, tap5On=0, tap6On=0, tap7On=0, tap8On=0, tap1Sync=1/4 (8), tap1Time=250, tap1Note=67, tap1Gain=-13, tap1Pan=0.25, tap1Shape=Forward (0), tap1Fragment=125, tap1Repeats=3, tap1Low=180, tap1High=8000, tap1SpaceSend=0

Reject/revise if: Pitch-shifted words compete with the dry line or create dissonant continuous feedback.

Intent check: Answer the end of a phrase with a quiet fifth-up sync reflection. Assess against: Pitch-shifted words compete with the dry line or create dissonant continuous feedback.. Original design-stage calibration status: **not_run**; final numerical results and individual owner-audition status are in RACK_BANK_REPORT.md.

### HR211 - Studio Short Double Width

**Lead Vocal / 120 BPM.** Source: a dry sung/spoken lead phrase with quiet and loud words

Use quiet offset unpitched reflections as a restrained vocal double.

EQ: LowCut 95 Hz / +0 dB / Q 0.707; Bell 6700 Hz / +0 dB / Q 1 (dynamic -2.5 dB)

Full range:

- HamloComp (amount 1): thresholdDb=-24, ratio=2.3, attackMs=14, releaseMs=140, model=VCA (1), source=Voc (3), kneeDb=6, detector=RMS (1), character=Clean (0), autoMakeup=Off (0), makeupDb=0, mix=1
- HamloEcho (amount 0.1): echoMode=Score (1), quickEnabled=Off (0), scoreRouting=Send (1), scoreMix=1, scoreOutput=0, scoreSource=60, scoreFeedback=0, scoreFeedbackSync=Free (0), scoreFeedbackMs=500, scoreFeedbackPitch=0, scoreColor=Clean (0), scoreDrive=0, scoreLowCut=400, scoreHighCut=7000, scoreWidth=1, scoreDuck=0.2, scoreFreeze=Off (0), scoreSpaceMix=0, scoreGlitch=Off (0), scoreGlitchAmount=0, scoreBands=Off (0), scoreSwing=0, tap1On=1, tap2On=1, tap3On=0, tap4On=0, tap5On=0, tap6On=0, tap7On=0, tap8On=0, tap1Sync=Free (0), tap1Time=24, tap1Note=60, tap1Gain=-12, tap1Pan=-0.65, tap1Shape=Forward (0), tap1Fragment=125, tap1Repeats=3, tap1Low=400, tap1High=7000, tap1SpaceSend=0, tap2Sync=Free (0), tap2Time=37, tap2Note=60, tap2Gain=-14, tap2Pan=0.65, tap2Shape=Forward (0), tap2Fragment=125, tap2Repeats=3, tap2Low=400, tap2High=7000, tap2SpaceSend=0

Reject/revise if: The lead moves sideways or mono loses consonant intelligibility.

Intent check: Use quiet offset unpitched reflections as a restrained vocal double. Assess against: The lead moves sideways or mono loses consonant intelligibility.. Original design-stage calibration status: **not_run**; final numerical results and individual owner-audition status are in RACK_BANK_REPORT.md.

### HR212 - Studio Parallel Shout Bed

**Lead Vocal / 120 BPM.** Source: a dry sung/spoken lead phrase with quiet and loud words

Add a rough low-level density bed while retaining the dry shout crest.

EQ: LowCut 100 Hz / +0 dB / Q 0.707; Bell 7200 Hz / +0 dB / Q 1 (dynamic -4 dB)

Full range:

- HamloComp (amount 0.25): thresholdDb=-30, ratio=7, attackMs=5, releaseMs=85, model=FET (2), source=Voc (3), kneeDb=6, detector=RMS (1), character=Clean (0), autoMakeup=Off (0), makeupDb=0, mix=1
- HamloMag (amount 0.16): model=Germanium (4), driveDb=10, mix=1, trimDb=0, oversampling=8x (3), autoGain=On (1), harmonics=0.35, tone=0, evenOdd=0, tapeBias=0.5, speed=1 (0), wow=0, hiss=0, hum=0, crackle=0, noiseFollow=Follow (1), punish=Off (0), thump=Off (0), steep=Off (0), sampler=Off (0), phoneBass=0, subProtectHz=0, dynamics=0, match=Off (0), delta=Off (0)
- HamloVerb (amount 0.045): space=Ambience (6), engine=Crisp (4), decaySeconds=0.3, predelayMs=14, predelaySync=Free (0), lowCutHz=700, highCutHz=5500, bassMult=0.75, hfDecayRatio=0.5, width=1.1, modDepth=0.1, duck=0.15, routing=Send (1), mix=1, keepDry=Off (0), wetDb=0, outputDb=0, earlyDb=0, lateDb=0, timeShape=Off (0), shimmerAmount=0, freeze=Off (0)

Reject/revise if: The matched dry shout feels smaller or saturated esses dominate.

Intent check: Add a rough low-level density bed while retaining the dry shout crest. Assess against: The matched dry shout feels smaller or saturated esses dominate.. Original design-stage calibration status: **not_run**; final numerical results and individual owner-audition status are in RACK_BANK_REPORT.md.

### HR213 - Studio Backing Soft Seat

**Backing Vocal / 120 BPM.** Source: a dry backing stack with consonant unisons and held vowels

Place a backing stack behind the lead with controlled upper attack.

EQ: LowCut 160 Hz / +0 dB / Q 0.707; HighShelf 9000 Hz / -1.5 dB / Q 0.707

Full range:

- HamloComp (amount 1): thresholdDb=-26, ratio=3, attackMs=15, releaseMs=180, model=Opto (3), source=Voc (3), kneeDb=6, detector=RMS (1), character=Clean (0), autoMakeup=Off (0), makeupDb=0, mix=1
- HamloUdarnik (amount 1): attackDb=-1, sustainDb=0.5, speed=0.5, style=PRAVDA (0), engine=V2 (1), protect=On (1), lookahead=Vostok (1), listen=Full (0), rise=1, decay=1, sensitivity=0.8, preserve=1, mix=1, outputDb=0
- HamloVerb (amount 0.12): space=Chamber (2), engine=Velvet (3), decaySeconds=0.85, predelayMs=12, predelaySync=Free (0), lowCutHz=450, highCutHz=7000, bassMult=0.75, hfDecayRatio=0.5, width=1.1, modDepth=0.1, duck=0.15, routing=Send (1), mix=1, keepDry=Off (0), wetDb=0, outputDb=0, earlyDb=0, lateDb=0, timeShape=Off (0), shimmerAmount=0, freeze=Off (0)

Reject/revise if: The backing consonants become more forward than the lead.

Intent check: Place a backing stack behind the lead with controlled upper attack. Assess against: The backing consonants become more forward than the lead.. Original design-stage calibration status: **not_run**; final numerical results and individual owner-audition status are in RACK_BANK_REPORT.md.

### HR214 - Studio Chorus Side Halo

**Backing Vocal / 120 BPM.** Source: a dry backing stack with consonant unisons and held vowels

Make the high backing field wider while preserving a centered vowel body.

EQ: LowCut 170 Hz / +0 dB / Q 0.707; Bell 500 Hz / -1 dB / Q 1

Full range:

- HamloComp (amount 1): thresholdDb=-25, ratio=2.5, attackMs=20, releaseMs=200, model=Opto (3), source=Voc (3), kneeDb=6, detector=RMS (1), character=Clean (0), autoMakeup=Off (0), makeupDb=0, mix=1
- HamloShirota (amount 1): v2Mode=Stereo / Space (1), v2Width=1, v2LowWidth=0, v2MidWidth=1.1, v2HighWidth=1.35, v2LowSplit=300, v2HighSplit=3000, v2MonoBass=250, v2Generate=0, v2Guard=On (1), v2Floor=0.15, v2Mix=1, v2Output=0, v2Motion=Off (0), v2PositionOn=Off (0), v2Mono=Stereo (0)
- HamloVerb (amount 0.1): space=Plate (4), engine=Velvet (3), decaySeconds=1.1, predelayMs=22, predelaySync=Free (0), lowCutHz=600, highCutHz=8500, bassMult=0.75, hfDecayRatio=0.5, width=1.1, modDepth=0.1, duck=0.15, routing=Send (1), mix=1, keepDry=Off (0), wetDb=0, outputDb=0, earlyDb=0, lateDb=0, timeShape=Off (0), shimmerAmount=0, freeze=Off (0)

Reject/revise if: The mono chorus loses the vowel or center shifts on consonants.

Intent check: Make the high backing field wider while preserving a centered vowel body. Assess against: The mono chorus loses the vowel or center shifts on consonants.. Original design-stage calibration status: **not_run**; final numerical results and individual owner-audition status are in RACK_BANK_REPORT.md.

### HR215 - Studio Dark Gang Stack

**Backing Vocal / 120 BPM.** Source: a dry backing stack with consonant unisons and held vowels

Keep a gang shout compact, rough and slightly behind the lead.

EQ: LowCut 190 Hz / +0 dB / Q 0.707; HighCut 9000 Hz / +0 dB / Q 0.707; Bell 800 Hz / -1.5 dB / Q 1

Full range:

- HamloComp (amount 0.65): thresholdDb=-28, ratio=4, attackMs=10, releaseMs=130, model=FET (2), source=Voc (3), kneeDb=6, detector=RMS (1), character=Clean (0), autoMakeup=Off (0), makeupDb=0, mix=1
- HamloMag (amount 0.25): model=Iron (3), driveDb=6, mix=1, trimDb=0, oversampling=8x (3), autoGain=On (1), harmonics=0.35, tone=0, evenOdd=0, tapeBias=0.5, speed=1 (0), wow=0, hiss=0, hum=0, crackle=0, noiseFollow=Follow (1), punish=Off (0), thump=Off (0), steep=Off (0), sampler=Off (0), phoneBass=0, subProtectHz=0, dynamics=0, match=Off (0), delta=Off (0)
- HamloVerb (amount 0.1): space=Room (1), engine=Warm (2), decaySeconds=0.45, predelayMs=5, predelaySync=Free (0), lowCutHz=600, highCutHz=5500, bassMult=0.75, hfDecayRatio=0.5, width=1.1, modDepth=0.1, duck=0.15, routing=Send (1), mix=1, keepDry=Off (0), wetDb=0, outputDb=0, earlyDb=0, lateDb=0, timeShape=Off (0), shimmerAmount=0, freeze=Off (0)

Reject/revise if: The stack turns into flat distortion or low mids crowd the lead.

Intent check: Keep a gang shout compact, rough and slightly behind the lead. Assess against: The stack turns into flat distortion or low mids crowd the lead.. Original design-stage calibration status: **not_run**; final numerical results and individual owner-audition status are in RACK_BANK_REPORT.md.

### HR216 - Studio Air Choir Bloom

**Backing Vocal / 120 BPM.** Source: a dry backing stack with consonant unisons and held vowels

Add a quiet octave shimmer only to the filtered reverb return.

EQ: LowCut 230 Hz / +0 dB / Q 0.707; Bell 8000 Hz / +0 dB / Q 1 (dynamic -2 dB)

Full range:

- HamloComp (amount 1): thresholdDb=-25, ratio=2, attackMs=22, releaseMs=230, model=Opto (3), source=Voc (3), kneeDb=6, detector=RMS (1), character=Clean (0), autoMakeup=Off (0), makeupDb=0, mix=1
- HamloVerb (amount 0.18): space=Hall (3), engine=Bloom (6), decaySeconds=2.4, predelayMs=40, predelaySync=Free (0), lowCutHz=700, highCutHz=10000, bassMult=0.75, hfDecayRatio=0.5, width=1.1, modDepth=0.1, duck=0.35, routing=Send (1), mix=1, keepDry=Off (0), wetDb=0, outputDb=0, earlyDb=0, lateDb=0, timeShape=Off (0), shimmerAmount=0.18, freeze=Off (0), shimmerPitch=12, shimmerFeedback=0.2

Reject/revise if: A pitched dry lead appears or the shimmer builds across several chords.

Intent check: Add a quiet octave shimmer only to the filtered reverb return. Assess against: A pitched dry lead appears or the shimmer builds across several chords.. Original design-stage calibration status: **not_run**; final numerical results and individual owner-audition status are in RACK_BANK_REPORT.md.

### HR217 - Studio Reverse Choir Swell

**Backing Vocal / 120 BPM.** Source: a dry backing stack with consonant unisons and held vowels

Create a quiet reverse-shaped reverb rise behind held backing vowels.

EQ: LowCut 200 Hz / +0 dB / Q 0.707

Full range:

- HamloVerb (amount 0.16): space=Hall (3), engine=Bloom (6), decaySeconds=1.8, predelayMs=0, predelaySync=Free (0), lowCutHz=600, highCutHz=8500, bassMult=0.75, hfDecayRatio=0.5, width=1.1, modDepth=0.1, duck=0.15, routing=Send (1), mix=1, keepDry=Off (0), wetDb=0, outputDb=0, earlyDb=0, lateDb=0, timeShape=Reverse Swell (2), shimmerAmount=0, freeze=Off (0), shapeSync=1/4 (10), shapeTimeMs=500, gateReleaseMs=40
- HamloShirota (amount 1): v2Mode=Stereo / Space (1), v2Width=1, v2LowWidth=0, v2MidWidth=1.1, v2HighWidth=1.25, v2LowSplit=300, v2HighSplit=3000, v2MonoBass=250, v2Generate=0, v2Guard=On (1), v2Floor=0.15, v2Mix=1, v2Output=0, v2Motion=Off (0), v2PositionOn=Off (0), v2Mono=Stereo (0)

Reject/revise if: The dry consonants reverse or the swell arrives after the intended chord change.

Intent check: Create a quiet reverse-shaped reverb rise behind held backing vowels. Assess against: The dry consonants reverse or the swell arrives after the intended chord change.. Original design-stage calibration status: **not_run**; final numerical results and individual owner-audition status are in RACK_BANK_REPORT.md.

### HR218 - Studio Rhythmic Hook Gate

**Backing Vocal / 120 BPM.** Source: a dry backing stack with consonant unisons and held vowels

Shape a backing hook into explicit soft sixteenth-note openings.

EQ: LowCut 180 Hz / +0 dB / Q 0.707; HighShelf 9500 Hz / -1 dB / Q 0.707

Full range:

- HamloZastava (amount 0.6): v2_engine=Rhythm (4), v2_enabled=On (1), v2_threshold=-40, v2_range=24, v2_hold=0, v2_release=8, v2_attack=2, v2_hysteresis=4, v2_knee=6, v2_external=Main (0), v2_expand=Expand (1), v2_ratio=2, v2_link=1, v2_mix=1, v2_output=0, v2_body=0, v2_tail=8, v2_lookahead=2, v2_hp=0, v2_lp=0, v2_listen=Off (0), v2_steps=16, v2_beats=0.25, v2_sync=Tempo (1), v2_step1=1, v2_step2=1, v2_step3=0, v2_step4=0.5, v2_step5=1, v2_step6=0, v2_step7=0.5, v2_step8=0, v2_step9=1, v2_step10=1, v2_step11=0, v2_step12=0.5, v2_step13=1, v2_step14=0, v2_step15=0.7, v2_step16=0, v2_step17=1, v2_step18=1, v2_step19=0, v2_step20=0.5, v2_step21=1, v2_step22=0, v2_step23=0.5, v2_step24=0, v2_step25=1, v2_step26=1, v2_step27=0, v2_step28=0.5, v2_step29=1, v2_step30=0, v2_step31=0.7, v2_step32=0
- HamloVerb (amount 0.07): space=Ambience (6), engine=Velvet (3), decaySeconds=0.4, predelayMs=8, predelaySync=Free (0), lowCutHz=700, highCutHz=7000, bassMult=0.75, hfDecayRatio=0.5, width=1.1, modDepth=0.1, duck=0.15, routing=Send (1), mix=1, keepDry=Off (0), wetDb=0, outputDb=0, earlyDb=0, lateDb=0, timeShape=Off (0), shimmerAmount=0, freeze=Off (0)

Reject/revise if: Gate clicks appear or key lyrical accents are removed.

Intent check: Shape a backing hook into explicit soft sixteenth-note openings. Assess against: Gate clicks appear or key lyrical accents are removed.. Original design-stage calibration status: **not_run**; final numerical results and individual owner-audition status are in RACK_BANK_REPORT.md.

### HR219 - Studio Acoustic Pick Balance

**Guitar / 120 BPM.** Source: a dry guitar phrase with pick attacks, ringing notes and a stop

Reduce pick bite and stabilize acoustic strums without flattening them.

EQ: LowCut 80 Hz / +0 dB / Q 0.707; Bell 220 Hz / -1.5 dB / Q 1; Bell 3800 Hz / +0 dB / Q 1.5 (dynamic -2 dB)

Full range:

- HamloComp (amount 1): thresholdDb=-23, ratio=2, attackMs=25, releaseMs=180, model=Opto (3), source=Samples (2), kneeDb=6, detector=RMS (1), character=Clean (0), autoMakeup=Off (0), makeupDb=0, mix=1
- HamloVerb (amount 0.07): space=Room (1), engine=Clean (1), decaySeconds=0.45, predelayMs=12, predelaySync=Free (0), lowCutHz=350, highCutHz=8500, bassMult=0.75, hfDecayRatio=0.5, width=1.1, modDepth=0.1, duck=0.15, routing=Send (1), mix=1, keepDry=Off (0), wetDb=0, outputDb=0, earlyDb=0, lateDb=0, timeShape=Off (0), shimmerAmount=0, freeze=Off (0)

Reject/revise if: String accents lose movement or the guitar gains low-mid boom.

Intent check: Reduce pick bite and stabilize acoustic strums without flattening them. Assess against: String accents lose movement or the guitar gains low-mid boom.. Original design-stage calibration status: **not_run**; final numerical results and individual owner-audition status are in RACK_BANK_REPORT.md.

### HR220 - Studio Fingerstyle Detail

**Guitar / 120 BPM.** Source: a dry guitar phrase with pick attacks, ringing notes and a stop

Retain finger transients while supporting quiet ringing notes.

EQ: LowCut 70 Hz / +0 dB / Q 0.707; Bell 300 Hz / -1 dB / Q 1

Full range:

- HamloFader (amount 1): engine=Modern (1), enabled=On (1), mode=Ride (2), gainDb=0, targetDb=-21, mix=1, polarity=Off (0), mute=Off (0), solo=Off (0), inputDb=0, outputDb=0, leftDb=0, rightDb=0, matchedDb=0, peakGuard=On (1), matchCeiling=-1, strength=0.25, maxBoost=1.5, maxCut=3, floorDb=-55, holdMs=100, riseMs=250, fallMs=140, depthDb=0, rightPhase=0
- HamloComp (amount 1): thresholdDb=-24, ratio=1.8, attackMs=32, releaseMs=240, model=Opto (3), source=Samples (2), kneeDb=6, detector=RMS (1), character=Clean (0), autoMakeup=Off (0), makeupDb=0, mix=1

Reject/revise if: Fret noise rises between notes or the sustained body pumps.

Intent check: Retain finger transients while supporting quiet ringing notes. Assess against: Fret noise rises between notes or the sustained body pumps.. Original design-stage calibration status: **not_run**; final numerical results and individual owner-audition status are in RACK_BANK_REPORT.md.

### HR221 - Studio Funk Short Chank

**Guitar / 120 BPM.** Source: a dry guitar phrase with pick attacks, ringing notes and a stop

Keep muted electric chords tight with a little iron midrange.

EQ: LowCut 140 Hz / +0 dB / Q 0.707; Bell 450 Hz / -1.5 dB / Q 1

Full range:

- HamloUdarnik (amount 1): attackDb=1, sustainDb=-2, speed=0.75, style=PRAVDA (0), engine=V2 (1), protect=On (1), lookahead=Vostok (1), listen=Full (0), rise=1, decay=1, sensitivity=0.8, preserve=1, mix=1, outputDb=0
- HamloMag (amount 0.25): model=Iron (3), driveDb=4, mix=1, trimDb=0, oversampling=8x (3), autoGain=On (1), harmonics=0.35, tone=0, evenOdd=0, tapeBias=0.5, speed=1 (0), wow=0, hiss=0, hum=0, crackle=0, noiseFollow=Follow (1), punish=Off (0), thump=Off (0), steep=Off (0), sampler=Off (0), phoneBass=0, subProtectHz=0, dynamics=0, match=Off (0), delta=Off (0)
- HamloComp (amount 1): thresholdDb=-23, ratio=2.5, attackMs=18, releaseMs=110, model=VCA (1), source=Samples (2), kneeDb=6, detector=RMS (1), character=Clean (0), autoMakeup=Off (0), makeupDb=0, mix=1

Reject/revise if: Chord stops click or low-mid sustain covers the groove.

Intent check: Keep muted electric chords tight with a little iron midrange. Assess against: Chord stops click or low-mid sustain covers the groove.. Original design-stage calibration status: **not_run**; final numerical results and individual owner-audition status are in RACK_BANK_REPORT.md.

### HR222 - Studio Clean Amp Slap

**Guitar / 120 BPM.** Source: a dry guitar phrase with pick attacks, ringing notes and a stop

Put a filtered short slap behind a clean electric lead.

EQ: LowCut 100 Hz / +0 dB / Q 0.707; Bell 280 Hz / -1 dB / Q 1

Full range:

- HamloMag (amount 0.2): model=Tube (2), driveDb=3, mix=1, trimDb=0, oversampling=8x (3), autoGain=On (1), harmonics=0.35, tone=0, evenOdd=0, tapeBias=0.5, speed=1 (0), wow=0, hiss=0, hum=0, crackle=0, noiseFollow=Follow (1), punish=Off (0), thump=Off (0), steep=Off (0), sampler=Off (0), phoneBass=0, subProtectHz=0, dynamics=0, match=Off (0), delta=Off (0)
- HamloEcho (amount 0.12): echoMode=Score (1), quickEnabled=Off (0), scoreRouting=Send (1), scoreMix=1, scoreOutput=0, scoreSource=60, scoreFeedback=0, scoreFeedbackSync=Free (0), scoreFeedbackMs=115, scoreFeedbackPitch=0, scoreColor=Tape (2), scoreDrive=0, scoreLowCut=450, scoreHighCut=5500, scoreWidth=1, scoreDuck=0.2, scoreFreeze=Off (0), scoreSpaceMix=0, scoreGlitch=Off (0), scoreGlitchAmount=0, scoreBands=Off (0), scoreSwing=0, tap1On=1, tap2On=0, tap3On=0, tap4On=0, tap5On=0, tap6On=0, tap7On=0, tap8On=0, tap1Sync=Free (0), tap1Time=115, tap1Note=60, tap1Gain=-6, tap1Pan=0, tap1Shape=Forward (0), tap1Fragment=125, tap1Repeats=3, tap1Low=450, tap1High=5500, tap1SpaceSend=0

Reject/revise if: The slap becomes a second distinct melody or combs out the mono lead.

Intent check: Put a filtered short slap behind a clean electric lead. Assess against: The slap becomes a second distinct melody or combs out the mono lead.. Original design-stage calibration status: **not_run**; final numerical results and individual owner-audition status are in RACK_BANK_REPORT.md.

### HR223 - Studio Grit Lead Sustain

**Guitar / 120 BPM.** Source: a dry guitar phrase with pick attacks, ringing notes and a stop

Add a parallel rough edge and smooth sustain under a guitar lead.

EQ: LowCut 90 Hz / +0 dB / Q 0.707; Bell 3200 Hz / +0 dB / Q 1 (dynamic -2 dB)

Full range:

- HamloComp (amount 0.55): thresholdDb=-27, ratio=4, attackMs=12, releaseMs=190, model=FET (2), source=Samples (2), kneeDb=6, detector=RMS (1), character=Clean (0), autoMakeup=Off (0), makeupDb=0, mix=1
- HamloMag (amount 0.2): model=Germanium (4), driveDb=8, mix=1, trimDb=0, oversampling=8x (3), autoGain=On (1), harmonics=0.35, tone=0, evenOdd=0, tapeBias=0.5, speed=1 (0), wow=0, hiss=0, hum=0, crackle=0, noiseFollow=Follow (1), punish=Off (0), thump=Off (0), steep=Off (0), sampler=Off (0), phoneBass=0, subProtectHz=0, dynamics=0, match=Off (0), delta=Off (0)
- HamloVerb (amount 0.08): space=Plate (4), engine=Warm (2), decaySeconds=0.85, predelayMs=25, predelaySync=Free (0), lowCutHz=500, highCutHz=6500, bassMult=0.75, hfDecayRatio=0.5, width=1.1, modDepth=0.1, duck=0.15, routing=Send (1), mix=1, keepDry=Off (0), wetDb=0, outputDb=0, earlyDb=0, lateDb=0, timeShape=Off (0), shimmerAmount=0, freeze=Off (0)

Reject/revise if: The dry pick flattens or distortion hides bent-note pitch.

Intent check: Add a parallel rough edge and smooth sustain under a guitar lead. Assess against: The dry pick flattens or distortion hides bent-note pitch.. Original design-stage calibration status: **not_run**; final numerical results and individual owner-audition status are in RACK_BANK_REPORT.md.

### HR224 - Studio Wide Rhythm Pair

**Guitar / 120 BPM.** Source: a dry guitar phrase with pick attacks, ringing notes and a stop

Widen the high part of a recorded guitar pair without inventing low stereo.

EQ: LowCut 120 Hz / +0 dB / Q 0.707; Bell 350 Hz / -1.5 dB / Q 1

Full range:

- HamloComp (amount 1): thresholdDb=-22, ratio=1.8, attackMs=28, releaseMs=160, model=VCA (1), source=Samples (2), kneeDb=6, detector=RMS (1), character=Clean (0), autoMakeup=Off (0), makeupDb=0, mix=1
- HamloShirota (amount 1): v2Mode=Stereo / Space (1), v2Width=1, v2LowWidth=0, v2MidWidth=1.1, v2HighWidth=1.25, v2LowSplit=300, v2HighSplit=3000, v2MonoBass=220, v2Generate=0, v2Guard=On (1), v2Floor=0.15, v2Mix=1, v2Output=0, v2Motion=Off (0), v2PositionOn=Off (0), v2Mono=Stereo (0)

Reject/revise if: Mono loses riff body or the pair balance drifts.

Intent check: Widen the high part of a recorded guitar pair without inventing low stereo. Assess against: Mono loses riff body or the pair balance drifts.. Original design-stage calibration status: **not_run**; final numerical results and individual owner-audition status are in RACK_BANK_REPORT.md.

### HR225 - Studio Ambient Guitar Trail

**Guitar / 120 BPM.** Source: a dry guitar phrase with pick attacks, ringing notes and a stop

Let a filtered delay feed a quiet long hall after clean guitar notes.

EQ: LowCut 140 Hz / +0 dB / Q 0.707

Full range:

- HamloEcho (amount 0.15): echoMode=Score (1), quickEnabled=Off (0), scoreRouting=Send (1), scoreMix=1, scoreOutput=0, scoreSource=60, scoreFeedback=0.28, scoreFeedbackSync=1/8 D (6), scoreFeedbackMs=500, scoreFeedbackPitch=0, scoreColor=Warm (1), scoreDrive=0, scoreLowCut=500, scoreHighCut=6500, scoreWidth=1, scoreDuck=0.2, scoreFreeze=Off (0), scoreSpaceMix=0, scoreGlitch=Off (0), scoreGlitchAmount=0, scoreBands=Off (0), scoreSwing=0, tap1On=1, tap2On=0, tap3On=0, tap4On=0, tap5On=0, tap6On=0, tap7On=0, tap8On=0, tap1Sync=1/8 D (6), tap1Time=250, tap1Note=60, tap1Gain=-6, tap1Pan=0, tap1Shape=Forward (0), tap1Fragment=125, tap1Repeats=3, tap1Low=500, tap1High=6500, tap1SpaceSend=0
- HamloVerb (amount 0.13): space=Hall (3), engine=Bloom (6), decaySeconds=2.8, predelayMs=38, predelaySync=Free (0), lowCutHz=650, highCutHz=8000, bassMult=0.75, hfDecayRatio=0.5, width=1.1, modDepth=0.1, duck=0.4, routing=Send (1), mix=1, keepDry=Off (0), wetDb=0, outputDb=0, earlyDb=0, lateDb=0, timeShape=Off (0), shimmerAmount=0, freeze=Off (0)

Reject/revise if: Feedback muddies the next harmony or the dry pick is buried.

Intent check: Let a filtered delay feed a quiet long hall after clean guitar notes. Assess against: Feedback muddies the next harmony or the dry pick is buried.. Original design-stage calibration status: **not_run**; final numerical results and individual owner-audition status are in RACK_BANK_REPORT.md.

### HR226 - Studio Garage Sample Edge

**Guitar / 120 BPM.** Source: a dry guitar phrase with pick attacks, ringing notes and a stop

Give a dry guitar sample a compact sampled edge and a small room.

EQ: LowCut 140 Hz / +0 dB / Q 0.707; HighCut 11000 Hz / +0 dB / Q 0.707

Full range:

- HamloMag (amount 0.35): model=Tape (1), driveDb=5, mix=1, trimDb=0, oversampling=8x (3), autoGain=On (1), harmonics=0.35, tone=0, evenOdd=0, tapeBias=0.5, speed=1 (0), wow=0, hiss=0, hum=0, crackle=0, noiseFollow=Follow (1), punish=Off (0), thump=Off (0), steep=Off (0), sampler=MPC60 (2), phoneBass=0, subProtectHz=0, dynamics=0, match=Off (0), delta=Off (0)
- HamloComp (amount 0.5): thresholdDb=-25, ratio=3, attackMs=18, releaseMs=135, model=FET (2), source=Samples (2), kneeDb=6, detector=RMS (1), character=Clean (0), autoMakeup=Off (0), makeupDb=0, mix=1
- HamloVerb (amount 0.06): space=Room (1), engine=Warm (2), decaySeconds=0.35, predelayMs=5, predelaySync=Free (0), lowCutHz=500, highCutHz=5500, bassMult=0.75, hfDecayRatio=0.5, width=1.1, modDepth=0.1, duck=0.15, routing=Send (1), mix=1, keepDry=Off (0), wetDb=0, outputDb=0, earlyDb=0, lateDb=0, timeShape=Off (0), shimmerAmount=0, freeze=Off (0)

Reject/revise if: The sample loses chord identity or noise fills the stops.

Intent check: Give a dry guitar sample a compact sampled edge and a small room. Assess against: The sample loses chord identity or noise fills the stops.. Original design-stage calibration status: **not_run**; final numerical results and individual owner-audition status are in RACK_BANK_REPORT.md.

### HR227 - Studio Grand Clear Body

**Keys / 120 BPM.** Source: a dry chord phrase with staccato and held notes

Trim piano low-mid congestion while leaving natural hammer dynamics.

EQ: LowCut 55 Hz / +0 dB / Q 0.707; Bell 280 Hz / -1.5 dB / Q 1; HighShelf 10000 Hz / +0.5 dB / Q 0.707

Full range:

- HamloComp (amount 1): thresholdDb=-22, ratio=1.6, attackMs=35, releaseMs=260, model=Opto (3), source=Samples (2), kneeDb=6, detector=RMS (1), character=Clean (0), autoMakeup=Off (0), makeupDb=0, mix=1
- HamloVerb (amount 0.08): space=Chamber (2), engine=Clean (1), decaySeconds=0.9, predelayMs=20, predelaySync=Free (0), lowCutHz=300, highCutHz=10000, bassMult=0.75, hfDecayRatio=0.5, width=1.1, modDepth=0.1, duck=0.15, routing=Send (1), mix=1, keepDry=Off (0), wetDb=0, outputDb=0, earlyDb=0, lateDb=0, timeShape=Off (0), shimmerAmount=0, freeze=Off (0)

Reject/revise if: Piano sounds scooped or sustain becomes obviously compressed.

Intent check: Trim piano low-mid congestion while leaving natural hammer dynamics. Assess against: Piano sounds scooped or sustain becomes obviously compressed.. Original design-stage calibration status: **not_run**; final numerical results and individual owner-audition status are in RACK_BANK_REPORT.md.

### HR228 - Studio Felt Close Piano

**Keys / 120 BPM.** Source: a dry chord phrase with staccato and held notes

Soften felt-piano mechanical bite and bring the room closer.

EQ: LowCut 75 Hz / +0 dB / Q 0.707; Bell 320 Hz / -1 dB / Q 1; HighShelf 8000 Hz / -1 dB / Q 0.707

Full range:

- HamloUdarnik (amount 1): attackDb=-1, sustainDb=0.5, speed=0.4, style=PRAVDA (0), engine=V2 (1), protect=On (1), lookahead=Vostok (1), listen=Full (0), rise=1, decay=1, sensitivity=0.8, preserve=1, mix=1, outputDb=0
- HamloMag (amount 0.2): model=Tube (2), driveDb=3, mix=1, trimDb=0, oversampling=8x (3), autoGain=On (1), harmonics=0.35, tone=0, evenOdd=0, tapeBias=0.5, speed=1 (0), wow=0, hiss=0, hum=0, crackle=0, noiseFollow=Follow (1), punish=Off (0), thump=Off (0), steep=Off (0), sampler=Off (0), phoneBass=0, subProtectHz=0, dynamics=0, match=Off (0), delta=Off (0)
- HamloVerb (amount 0.09): space=Room (1), engine=Velvet (3), decaySeconds=0.4, predelayMs=5, predelaySync=Free (0), lowCutHz=350, highCutHz=6000, bassMult=0.75, hfDecayRatio=0.5, width=1.1, modDepth=0.1, duck=0.15, routing=Send (1), mix=1, keepDry=Off (0), wetDb=0, outputDb=0, earlyDb=0, lateDb=0, timeShape=Off (0), shimmerAmount=0, freeze=Off (0)

Reject/revise if: The felt attack disappears or quiet key noise rises audibly.

Intent check: Soften felt-piano mechanical bite and bring the room closer. Assess against: The felt attack disappears or quiet key noise rises audibly.. Original design-stage calibration status: **not_run**; final numerical results and individual owner-audition status are in RACK_BANK_REPORT.md.

### HR229 - Studio Rhodes Velvet

**Keys / 120 BPM.** Source: a dry chord phrase with staccato and held notes

Smooth electric-piano chords with restrained even harmonics.

EQ: LowCut 85 Hz / +0 dB / Q 0.707; Bell 400 Hz / -1.3 dB / Q 1

Full range:

- HamloMag (amount 0.35): model=Tube (2), driveDb=5, mix=1, trimDb=0, oversampling=8x (3), autoGain=On (1), harmonics=0.35, tone=0, evenOdd=0.25, tapeBias=0.5, speed=1 (0), wow=0, hiss=0, hum=0, crackle=0, noiseFollow=Follow (1), punish=Off (0), thump=Off (0), steep=Off (0), sampler=Off (0), phoneBass=0, subProtectHz=0, dynamics=0, match=Off (0), delta=Off (0)
- HamloComp (amount 1): thresholdDb=-24, ratio=2, attackMs=28, releaseMs=260, model=Opto (3), source=Samples (2), kneeDb=6, detector=RMS (1), character=Clean (0), autoMakeup=Off (0), makeupDb=0, mix=1

Reject/revise if: The loud chords fuzz or tremolo gets an unintended pumping envelope.

Intent check: Smooth electric-piano chords with restrained even harmonics. Assess against: The loud chords fuzz or tremolo gets an unintended pumping envelope.. Original design-stage calibration status: **not_run**; final numerical results and individual owner-audition status are in RACK_BANK_REPORT.md.

### HR230 - Studio Organ Iron Seat

**Keys / 120 BPM.** Source: a dry chord phrase with staccato and held notes

Give a steady organ soft iron density and a bounded upper field.

EQ: LowCut 100 Hz / +0 dB / Q 0.707; Bell 1800 Hz / -1 dB / Q 1

Full range:

- HamloMag (amount 0.3): model=Iron (3), driveDb=5, mix=1, trimDb=0, oversampling=8x (3), autoGain=On (1), harmonics=0.35, tone=0, evenOdd=0, tapeBias=0.5, speed=1 (0), wow=0, hiss=0, hum=0, crackle=0, noiseFollow=Follow (1), punish=Off (0), thump=Off (0), steep=Off (0), sampler=Off (0), phoneBass=0, subProtectHz=0, dynamics=0, match=Off (0), delta=Off (0)
- HamloShirota (amount 1): v2Mode=Stereo / Space (1), v2Width=1, v2LowWidth=0, v2MidWidth=1.08, v2HighWidth=1.2, v2LowSplit=300, v2HighSplit=3000, v2MonoBass=200, v2Generate=0, v2Guard=On (1), v2Floor=0.15, v2Mix=1, v2Output=0, v2Motion=Off (0), v2PositionOn=Off (0), v2Mono=Stereo (0)
- HamloVerb (amount 0.08): space=Chamber (2), engine=Warm (2), decaySeconds=0.7, predelayMs=10, predelaySync=Free (0), lowCutHz=450, highCutHz=7500, bassMult=0.75, hfDecayRatio=0.5, width=1.1, modDepth=0.1, duck=0.15, routing=Send (1), mix=1, keepDry=Off (0), wetDb=0, outputDb=0, earlyDb=0, lateDb=0, timeShape=Off (0), shimmerAmount=0, freeze=Off (0)

Reject/revise if: The organ becomes a static wall or mono loses the held chord.

Intent check: Give a steady organ soft iron density and a bounded upper field. Assess against: The organ becomes a static wall or mono loses the held chord.. Original design-stage calibration status: **not_run**; final numerical results and individual owner-audition status are in RACK_BANK_REPORT.md.

### HR231 - Studio Wide Piano Early Room

**Keys / 120 BPM.** Source: a dry chord phrase with staccato and held notes

Use early room reflections to widen piano above its low body.

EQ: LowCut 60 Hz / +0 dB / Q 0.707; Bell 300 Hz / -0.8 dB / Q 1

Full range:

- HamloVerb (amount 0.1): space=Room (1), engine=Clean (1), decaySeconds=0.5, predelayMs=8, predelaySync=Free (0), lowCutHz=350, highCutHz=10000, bassMult=0.75, hfDecayRatio=0.5, width=1.1, modDepth=0.1, duck=0.15, routing=Send (1), mix=1, keepDry=Off (0), wetDb=0, outputDb=0, earlyDb=2, lateDb=-6, timeShape=Off (0), shimmerAmount=0, freeze=Off (0)
- HamloShirota (amount 1): v2Mode=Stereo / Space (1), v2Width=1, v2LowWidth=0.8, v2MidWidth=1.05, v2HighWidth=1.15, v2LowSplit=300, v2HighSplit=3000, v2MonoBass=140, v2Generate=0, v2Guard=On (1), v2Floor=0.15, v2Mix=1, v2Output=0, v2Motion=Off (0), v2PositionOn=Off (0), v2Mono=Stereo (0)

Reject/revise if: The mono hammer cancels or low piano notes lean to one side.

Intent check: Use early room reflections to widen piano above its low body. Assess against: The mono hammer cancels or low piano notes lean to one side.. Original design-stage calibration status: **not_run**; final numerical results and individual owner-audition status are in RACK_BANK_REPORT.md.

### HR232 - Studio Tape Keys Drift

**Keys / 120 BPM.** Source: a dry chord phrase with staccato and held notes

Add mild tape instability to a sustained key texture.

EQ: LowCut 120 Hz / +0 dB / Q 0.707; HighCut 12500 Hz / +0 dB / Q 0.707

Full range:

- HamloMag (amount 0.45): model=Tape (1), driveDb=4, mix=1, trimDb=0, oversampling=8x (3), autoGain=On (1), harmonics=0.35, tone=-0.08, evenOdd=0, tapeBias=0.5, speed=1 (0), wow=0.05, hiss=0, hum=0, crackle=0, noiseFollow=Follow (1), punish=Off (0), thump=Off (0), steep=Off (0), sampler=Off (0), phoneBass=0, subProtectHz=0, dynamics=0, match=Off (0), delta=Off (0)
- HamloVerb (amount 0.07): space=Chamber (2), engine=Warm (2), decaySeconds=0.75, predelayMs=16, predelaySync=Free (0), lowCutHz=450, highCutHz=7000, bassMult=0.75, hfDecayRatio=0.5, width=1.1, modDepth=0.1, duck=0.15, routing=Send (1), mix=1, keepDry=Off (0), wetDb=0, outputDb=0, earlyDb=0, lateDb=0, timeShape=Off (0), shimmerAmount=0, freeze=Off (0)

Reject/revise if: Chord pitch becomes seasick or dry gaps gain noise.

Intent check: Add mild tape instability to a sustained key texture. Assess against: Chord pitch becomes seasick or dry gaps gain noise.. Original design-stage calibration status: **not_run**; final numerical results and individual owner-audition status are in RACK_BANK_REPORT.md.

### HR233 - Studio Piano Reverse Reply

**Keys / 120 BPM.** Source: a dry chord phrase with staccato and held notes

Add a quiet reversed echo after the piano chord attack.

EQ: LowCut 130 Hz / +0 dB / Q 0.707

Full range:

- HamloEcho (amount 0.11): echoMode=Score (1), quickEnabled=Off (0), scoreRouting=Send (1), scoreMix=1, scoreOutput=0, scoreSource=60, scoreFeedback=0.08, scoreFeedbackSync=1/4 (8), scoreFeedbackMs=500, scoreFeedbackPitch=0, scoreColor=Clean (0), scoreDrive=0, scoreLowCut=500, scoreHighCut=7500, scoreWidth=1, scoreDuck=0.2, scoreFreeze=Off (0), scoreSpaceMix=0, scoreGlitch=Off (0), scoreGlitchAmount=0, scoreBands=Off (0), scoreSwing=0, tap1On=1, tap2On=0, tap3On=0, tap4On=0, tap5On=0, tap6On=0, tap7On=0, tap8On=0, tap1Sync=1/4 (8), tap1Time=250, tap1Note=60, tap1Gain=-13, tap1Pan=-0.2, tap1Shape=Reverse (1), tap1Fragment=125, tap1Repeats=3, tap1Low=500, tap1High=7500, tap1SpaceSend=0
- HamloVerb (amount 0.04): space=Ambience (6), engine=Velvet (3), decaySeconds=0.4, predelayMs=0, predelaySync=Free (0), lowCutHz=700, highCutHz=7000, bassMult=0.75, hfDecayRatio=0.5, width=1.1, modDepth=0.1, duck=0.15, routing=Send (1), mix=1, keepDry=Off (0), wetDb=0, outputDb=0, earlyDb=0, lateDb=0, timeShape=Off (0), shimmerAmount=0, freeze=Off (0)

Reject/revise if: The dry hammer reverses or the reply smears the next chord change.

Intent check: Add a quiet reversed echo after the piano chord attack. Assess against: The dry hammer reverses or the reply smears the next chord change.. Original design-stage calibration status: **not_run**; final numerical results and individual owner-audition status are in RACK_BANK_REPORT.md.

### HR234 - Studio Chord Soft Pulse

**Keys / 120 BPM.** Source: a dry chord phrase with staccato and held notes

Make a sustained chord breathe gently over a two-beat custom curve.

EQ: LowCut 140 Hz / +0 dB / Q 0.707; Bell 420 Hz / -1 dB / Q 1

Full range:

- HamloFader (amount 1): engine=Modern (1), enabled=On (1), mode=Motion (3), gainDb=0, targetDb=-18, mix=1, polarity=Off (0), mute=Off (0), solo=Off (0), inputDb=0, outputDb=0, leftDb=0, rightDb=0, matchedDb=0, peakGuard=On (1), matchCeiling=-1, strength=0.55, maxBoost=3, maxCut=6, floorDb=-55, holdMs=100, riseMs=250, fallMs=140, depthDb=3, rightPhase=0, shape=Custom (0), points=4, sync=Sync (1), beats=2, edgeMs=3, smooth=Smooth (1), level00=0.3, level01=0.6, level02=1, level03=0.8
- HamloShirota (amount 1): v2Mode=Stereo / Space (1), v2Width=1, v2LowWidth=0, v2MidWidth=1.08, v2HighWidth=1.2, v2LowSplit=300, v2HighSplit=3000, v2MonoBass=200, v2Generate=0, v2Guard=On (1), v2Floor=0.15, v2Mix=1, v2Output=0, v2Motion=Off (0), v2PositionOn=Off (0), v2Mono=Stereo (0)

Reject/revise if: The chord hard-gates or pulse phase drifts with transport.

Intent check: Make a sustained chord breathe gently over a two-beat custom curve. Assess against: The chord hard-gates or pulse phase drifts with transport.. Original design-stage calibration status: **not_run**; final numerical results and individual owner-audition status are in RACK_BANK_REPORT.md.

### HR235 - Studio Lead Focus Cut

**Synth / 120 BPM.** Source: a dry synth phrase with a held note and a transient passage

Tame upper-mid lead resonance and add a small filtered sync response.

EQ: LowCut 120 Hz / +0 dB / Q 0.707; Bell 3400 Hz / +0 dB / Q 1.6 (dynamic -2.5 dB)

Full range:

- HamloComp (amount 1): thresholdDb=-23, ratio=2, attackMs=12, releaseMs=120, model=VCA (1), source=Samples (2), kneeDb=6, detector=RMS (1), character=Clean (0), autoMakeup=Off (0), makeupDb=0, mix=1
- HamloEcho (amount 0.09): echoMode=Score (1), quickEnabled=Off (0), scoreRouting=Send (1), scoreMix=1, scoreOutput=0, scoreSource=60, scoreFeedback=0.15, scoreFeedbackSync=1/8 D (6), scoreFeedbackMs=500, scoreFeedbackPitch=0, scoreColor=Clean (0), scoreDrive=0, scoreLowCut=600, scoreHighCut=6500, scoreWidth=1, scoreDuck=0.2, scoreFreeze=Off (0), scoreSpaceMix=0, scoreGlitch=Off (0), scoreGlitchAmount=0, scoreBands=Off (0), scoreSwing=0, tap1On=1, tap2On=0, tap3On=0, tap4On=0, tap5On=0, tap6On=0, tap7On=0, tap8On=0, tap1Sync=1/8 D (6), tap1Time=250, tap1Note=60, tap1Gain=-6, tap1Pan=0, tap1Shape=Forward (0), tap1Fragment=125, tap1Repeats=3, tap1Low=600, tap1High=6500, tap1SpaceSend=0

Reject/revise if: The lead loses pitch focus or echoes occupy the next melody note.

Intent check: Tame upper-mid lead resonance and add a small filtered sync response. Assess against: The lead loses pitch focus or echoes occupy the next melody note.. Original design-stage calibration status: **not_run**; final numerical results and individual owner-audition status are in RACK_BANK_REPORT.md.

### HR236 - Studio Pad Wide Calm

**Synth / 120 BPM.** Source: a dry synth phrase with a held note and a transient passage

Make a pad wide above a mono low anchor with a slow spatial journey.

EQ: LowCut 170 Hz / +0 dB / Q 0.707

Full range:

- HamloShirota (amount 1): v2Mode=Stereo / Space (1), v2Width=1, v2LowWidth=0, v2MidWidth=1.15, v2HighWidth=1.35, v2LowSplit=300, v2HighSplit=3000, v2MonoBass=220, v2Generate=0, v2Guard=On (1), v2Floor=0.15, v2Mix=1, v2Output=0, v2Motion=On (1), v2PositionOn=On (1), v2Mono=Stereo (0), v2Unit=Quarter-note beats (1), v2Nodes=2, v2Node0Angle=-25, v2Node1Angle=25, v2Node0TravelBeat=6, v2Node1TravelBeat=6
- HamloVerb (amount 0.1): space=Hall (3), engine=Velvet (3), decaySeconds=2, predelayMs=30, predelaySync=Free (0), lowCutHz=600, highCutHz=8500, bassMult=0.75, hfDecayRatio=0.5, width=1.1, modDepth=0.1, duck=0.15, routing=Send (1), mix=1, keepDry=Off (0), wetDb=0, outputDb=0, earlyDb=0, lateDb=0, timeShape=Off (0), shimmerAmount=0, freeze=Off (0)

Reject/revise if: The mono fold loses chord identity or motion makes the bass move.

Intent check: Make a pad wide above a mono low anchor with a slow spatial journey. Assess against: The mono fold loses chord identity or motion makes the bass move.. Original design-stage calibration status: **not_run**; final numerical results and individual owner-audition status are in RACK_BANK_REPORT.md.

### HR237 - Studio Pluck Falling Notes

**Synth / 120 BPM.** Source: a dry synth phrase with a held note and a transient passage

Answer a pluck with sparse descending interval reflections.

EQ: LowCut 180 Hz / +0 dB / Q 0.707

Full range:

- HamloEcho (amount 0.14): echoMode=Score (1), quickEnabled=Off (0), scoreRouting=Send (1), scoreMix=1, scoreOutput=0, scoreSource=60, scoreFeedback=0.12, scoreFeedbackSync=1/8 (5), scoreFeedbackMs=500, scoreFeedbackPitch=0, scoreColor=Clean (0), scoreDrive=0, scoreLowCut=700, scoreHighCut=8000, scoreWidth=1, scoreDuck=0.2, scoreFreeze=Off (0), scoreSpaceMix=0, scoreGlitch=Off (0), scoreGlitchAmount=0, scoreBands=Off (0), scoreSwing=0, tap1On=1, tap2On=1, tap3On=0, tap4On=0, tap5On=0, tap6On=0, tap7On=0, tap8On=0, tap1Sync=1/8 (5), tap1Time=250, tap1Note=53, tap1Gain=-13, tap1Pan=-0.4, tap1Shape=Forward (0), tap1Fragment=125, tap1Repeats=3, tap1Low=700, tap1High=8000, tap1SpaceSend=0, tap2Sync=1/4 (8), tap2Time=500, tap2Note=48, tap2Gain=-17, tap2Pan=0.4, tap2Shape=Forward (0), tap2Fragment=125, tap2Repeats=3, tap2Low=700, tap2High=8000, tap2SpaceSend=0

Reject/revise if: The echoes replace the main melody or descend repeatedly in uncontrolled feedback.

Intent check: Answer a pluck with sparse descending interval reflections. Assess against: The echoes replace the main melody or descend repeatedly in uncontrolled feedback.. Original design-stage calibration status: **not_run**; final numerical results and individual owner-audition status are in RACK_BANK_REPORT.md.

### HR238 - Studio Synth Texture Stair

**Synth / 120 BPM.** Source: a dry synth phrase with a held note and a transient passage

Use explicit SHAPE drive steps on the upper band of a synth phrase.

EQ: LowCut 100 Hz / +0 dB / Q 0.707

Full range:

- HamloMag (amount 0.15): model=Tape (1), driveDb=2, mix=1, trimDb=0, oversampling=8x (3), autoGain=On (1), harmonics=0.35, tone=0, evenOdd=0, tapeBias=0.5, speed=1 (0), wow=0, hiss=0, hum=0, crackle=0, noiseFollow=Follow (1), punish=Off (0), thump=Off (0), steep=Off (0), sampler=Off (0), phoneBass=0, subProtectHz=0, dynamics=0, match=Off (0), delta=Off (0)

Band 600-11000 Hz:

- HamloRevolution (amount 0.3): mix=1, free=Sync (0), grid=1/16 (2), triplet=Off (0), swing=0, humanize=0, fadeMs=3, order=T-G-M (0), scene=1 (0), listen=Off (0), timeOn=Off (0), glitchOn=Off (0), motionOn=Off (0), timeMix=1, timeDepth=1, glitchMix=1, glitchDepth=0.5, motionMix=1, motionDepth=0.35, motionSmooth=0.5, motionTarget=Low Pass (1), timeStretch=Rhythm (1), glitchEngine=Block (1), shapeAOn=On (1), shapeBOn=Off (0), throwAOn=Off (0), throwBOn=Off (0), shapeATarget=Drive (6), shapeADepth=0.4, shapeASmooth=0.6
  - SHAPE A (steps 1-16): 3: 2; 4: 2; 5: 4; 6: 4; 7: 6; 8: 6; 9: 8; 10: 8; 11: 6; 12: 6; 13: 4; 14: 4; 15: 2

Reject/revise if: The sub distorts or drive transitions create clicks.

Intent check: Use explicit SHAPE drive steps on the upper band of a synth phrase. Assess against: The sub distorts or drive transitions create clicks.. Original design-stage calibration status: **not_run**; final numerical results and individual owner-audition status are in RACK_BANK_REPORT.md.

### HR239 - Studio Short Stereo Spark

**Synth / 120 BPM.** Source: a dry synth phrase with a held note and a transient passage

Add small diffuse high-band stereo energy to a mono pluck.

EQ: LowCut 160 Hz / +0 dB / Q 0.707

Full range:

- HamloShirota (amount 1): v2Mode=Stereo / Space (1), v2Width=1, v2LowWidth=0, v2MidWidth=1.05, v2HighWidth=1.3, v2LowSplit=300, v2HighSplit=3000, v2MonoBass=250, v2Generate=0.18, v2Guard=On (1), v2Floor=0.15, v2Mix=1, v2Output=0, v2Motion=Off (0), v2PositionOn=Off (0), v2Mono=Stereo (0), v2Generator=Diffuse (2)
- HamloVerb (amount 0.05): space=Ambience (6), engine=Crisp (4), decaySeconds=0.28, predelayMs=3, predelaySync=Free (0), lowCutHz=1000, highCutHz=10000, bassMult=0.75, hfDecayRatio=0.5, width=1.1, modDepth=0.1, duck=0.15, routing=Send (1), mix=1, keepDry=Off (0), wetDb=0, outputDb=0, earlyDb=0, lateDb=0, timeShape=Off (0), shimmerAmount=0, freeze=Off (0)

Reject/revise if: The mono pluck loses its central attack or width sounds like a hard echo.

Intent check: Add small diffuse high-band stereo energy to a mono pluck. Assess against: The mono pluck loses its central attack or width sounds like a hard echo.. Original design-stage calibration status: **not_run**; final numerical results and individual owner-audition status are in RACK_BANK_REPORT.md.

### HR240 - Studio End Phrase Half Time

**Synth / 120 BPM.** Source: a dry synth phrase with a held note and a transient passage

Apply half-time only on the final two subdivisions of a synth phrase.

EQ: LowCut 120 Hz / +0 dB / Q 0.707

Full range:

- HamloRevolution (amount 0.5): mix=1, free=Sync (0), grid=1/16 (2), triplet=Off (0), swing=0, humanize=0, fadeMs=3, order=T-G-M (0), scene=1 (0), listen=Off (0), timeOn=On (1), glitchOn=Off (0), motionOn=Off (0), timeMix=1, timeDepth=1, glitchMix=1, glitchDepth=0.5, motionMix=1, motionDepth=0.35, motionSmooth=0.5, motionTarget=Low Pass (1), timeStretch=Clean (2), glitchEngine=Block (1), shapeAOn=Off (0), shapeBOn=Off (0), throwAOn=Off (0), throwBOn=Off (0)
  - TIME (steps 1-16): 15: Half; 16: Half
- HamloEcho (amount 0.06): echoMode=Score (1), quickEnabled=Off (0), scoreRouting=Send (1), scoreMix=1, scoreOutput=0, scoreSource=60, scoreFeedback=0.08, scoreFeedbackSync=1/8 (5), scoreFeedbackMs=500, scoreFeedbackPitch=0, scoreColor=Clean (0), scoreDrive=0, scoreLowCut=700, scoreHighCut=7000, scoreWidth=1, scoreDuck=0.2, scoreFreeze=Off (0), scoreSpaceMix=0, scoreGlitch=Off (0), scoreGlitchAmount=0, scoreBands=Off (0), scoreSwing=0, tap1On=1, tap2On=0, tap3On=0, tap4On=0, tap5On=0, tap6On=0, tap7On=0, tap8On=0, tap1Sync=1/8 (5), tap1Time=250, tap1Note=60, tap1Gain=-6, tap1Pan=0, tap1Shape=Forward (0), tap1Fragment=125, tap1Repeats=3, tap1Low=700, tap1High=7000, tap1SpaceSend=0

Reject/revise if: The next bar stays half-speed or pitch drops despite Clean stretch.

Intent check: Apply half-time only on the final two subdivisions of a synth phrase. Assess against: The next bar stays half-speed or pitch drops despite Clean stretch.. Original design-stage calibration status: **not_run**; final numerical results and individual owner-audition status are in RACK_BANK_REPORT.md.

### HR241 - Studio Mix Gentle Glue

**Mix Bus / 120 BPM.** Source: a balanced stereo premaster with kick, bass and lead vocal

Bind a balanced premaster with minimal reduction and no forced loudness.

EQ: LowCut 22 Hz / +0 dB / Q 0.707; Bell 300 Hz / -0.4 dB / Q 1

Full range:

- HamloComp (amount 1): thresholdDb=-19, ratio=1.5, attackMs=40, releaseMs=180, model=VCA (1), source=All (0), kneeDb=6, detector=RMS (1), character=Clean (0), autoMakeup=Off (0), makeupDb=0, mix=1
- HamloMag (amount 0.12): model=Iron (3), driveDb=2, mix=1, trimDb=0, oversampling=8x (3), autoGain=On (1), harmonics=0.35, tone=0, evenOdd=0, tapeBias=0.5, speed=1 (0), wow=0, hiss=0, hum=0, crackle=0, noiseFollow=Follow (1), punish=Off (0), thump=Off (0), steep=Off (0), sampler=Off (0), phoneBass=0, subProtectHz=0, dynamics=0, match=Off (0), delta=Off (0)

Reject/revise if: Matched kick/snare contrast falls or the vocal ducks on bass notes.

Intent check: Bind a balanced premaster with minimal reduction and no forced loudness. Assess against: Matched kick/snare contrast falls or the vocal ducks on bass notes.. Original design-stage calibration status: **not_run**; final numerical results and individual owner-audition status are in RACK_BANK_REPORT.md.

### HR242 - Studio Mix Tape Cohesion

**Mix Bus / 120 BPM.** Source: a balanced stereo premaster with kick, bass and lead vocal

Add restrained tape density while holding the deepest bass clean.

EQ: LowCut 24 Hz / +0 dB / Q 0.707

Full range:

- HamloMag (amount 0.22): model=Tape (1), driveDb=3, mix=1, trimDb=0, oversampling=8x (3), autoGain=On (1), harmonics=0.35, tone=0, evenOdd=0, tapeBias=0.5, speed=1 (0), wow=0, hiss=0, hum=0, crackle=0, noiseFollow=Follow (1), punish=Off (0), thump=Off (0), steep=Off (0), sampler=Off (0), phoneBass=0, subProtectHz=90, dynamics=0, match=Off (0), delta=Off (0)
- HamloComp (amount 1): thresholdDb=-20, ratio=1.4, attackMs=35, releaseMs=210, model=Opto (3), source=All (0), kneeDb=6, detector=RMS (1), character=Clean (0), autoMakeup=Off (0), makeupDb=0, mix=1

Reject/revise if: Bass becomes woolly or the top loses separation after matching.

Intent check: Add restrained tape density while holding the deepest bass clean. Assess against: Bass becomes woolly or the top loses separation after matching.. Original design-stage calibration status: **not_run**; final numerical results and individual owner-audition status are in RACK_BANK_REPORT.md.

### HR243 - Studio Mix Low Center

**Mix Bus / 120 BPM.** Source: a balanced stereo premaster with kick, bass and lead vocal

Center only low-frequency mix energy and gently open the top field.

EQ: LowCut 23 Hz / +0 dB / Q 0.707

Full range:

- HamloShirota (amount 1): v2Mode=Stereo / Space (1), v2Width=1, v2LowWidth=0, v2MidWidth=1, v2HighWidth=1.08, v2LowSplit=300, v2HighSplit=3000, v2MonoBass=120, v2Generate=0, v2Guard=On (1), v2Floor=0.15, v2Mix=1, v2Output=0, v2Motion=Off (0), v2PositionOn=Off (0), v2Mono=Stereo (0)
- HamloComp (amount 1): thresholdDb=-18, ratio=1.3, attackMs=45, releaseMs=200, model=VCA (1), source=All (0), kneeDb=6, detector=RMS (1), character=Clean (0), autoMakeup=Off (0), makeupDb=0, mix=1

Reject/revise if: The mono lead loses body or kick/sub notes change in the fold.

Intent check: Center only low-frequency mix energy and gently open the top field. Assess against: The mono lead loses body or kick/sub notes change in the fold.. Original design-stage calibration status: **not_run**; final numerical results and individual owner-audition status are in RACK_BANK_REPORT.md.

### HR244 - Studio Mix Dynamic Mud

**Mix Bus / 120 BPM.** Source: a balanced stereo premaster with kick, bass and lead vocal

Let the low mids duck only when dense chords build up.

EQ: LowCut 22 Hz / +0 dB / Q 0.707; Bell 280 Hz / +0 dB / Q 0.9 (dynamic -1.8 dB)

Full range:

- HamloComp (amount 1): thresholdDb=-19, ratio=1.4, attackMs=38, releaseMs=170, model=VCA (1), source=All (0), kneeDb=6, detector=RMS (1), character=Clean (0), autoMakeup=Off (0), makeupDb=0, mix=1

Reject/revise if: Sparse passages sound scooped or bass-note body is removed continuously.

Intent check: Let the low mids duck only when dense chords build up. Assess against: Sparse passages sound scooped or bass-note body is removed continuously.. Original design-stage calibration status: **not_run**; final numerical results and individual owner-audition status are in RACK_BANK_REPORT.md.

### HR245 - Studio Mix Soft Top Control

**Mix Bus / 120 BPM.** Source: a balanced stereo premaster with kick, bass and lead vocal

Control a brittle upper mix band while leaving the rest open.

EQ: LowCut 23 Hz / +0 dB / Q 0.707

Full range:

- HamloComp (amount 1): thresholdDb=-18, ratio=1.3, attackMs=42, releaseMs=190, model=Opto (3), source=All (0), kneeDb=6, detector=RMS (1), character=Clean (0), autoMakeup=Off (0), makeupDb=0, mix=1

Band 5000-16000 Hz:

- HamloComp (amount 1): thresholdDb=-25, ratio=1.8, attackMs=3, releaseMs=75, model=VCA (1), source=All (0), kneeDb=10, detector=RMS (1), character=Clean (0), autoMakeup=Off (0), makeupDb=0, mix=1

Reject/revise if: The lead loses presence or cymbals trigger broadband pumping.

Intent check: Control a brittle upper mix band while leaving the rest open. Assess against: The lead loses presence or cymbals trigger broadband pumping.. Original design-stage calibration status: **not_run**; final numerical results and individual owner-audition status are in RACK_BANK_REPORT.md.

### HR246 - Studio Mix Parallel Body

**Mix Bus / 120 BPM.** Source: a balanced stereo premaster with kick, bass and lead vocal

Blend a little clean parallel density under the full dry premaster.

EQ: LowCut 24 Hz / +0 dB / Q 0.707; Bell 380 Hz / -0.5 dB / Q 1

Full range:

- HamloComp (amount 0.18): thresholdDb=-28, ratio=4, attackMs=25, releaseMs=160, model=VCA (1), source=All (0), kneeDb=6, detector=RMS (1), character=Clean (0), autoMakeup=Off (0), makeupDb=0, mix=1
- HamloMag (amount 0.1): model=Tube (2), driveDb=2, mix=1, trimDb=0, oversampling=8x (3), autoGain=On (1), harmonics=0.35, tone=0, evenOdd=0, tapeBias=0.5, speed=1 (0), wow=0, hiss=0, hum=0, crackle=0, noiseFollow=Follow (1), punish=Off (0), thump=Off (0), steep=Off (0), sampler=Off (0), phoneBass=0, subProtectHz=0, dynamics=0, match=Off (0), delta=Off (0)

Reject/revise if: The dry crest shrinks or the kick-bass pocket clouds at matched loudness.

Intent check: Blend a little clean parallel density under the full dry premaster. Assess against: The dry crest shrinks or the kick-bass pocket clouds at matched loudness.. Original design-stage calibration status: **not_run**; final numerical results and individual owner-audition status are in RACK_BANK_REPORT.md.

### HR247 - Studio Capture Match Minus Eighteen

**Utility / 120 BPM.** Source: an impulse, tone burst and representative stereo musical phrase

Prepare a bounded RMS match capture for comparing source clips at -18 dBFS.

EQ: flat

Full range:

- HamloFader (amount 1): engine=Modern (1), enabled=On (1), mode=Match (1), gainDb=0, targetDb=-18, mix=1, polarity=Off (0), mute=Off (0), solo=Off (0), inputDb=0, outputDb=0, leftDb=0, rightDb=0, matchedDb=0, peakGuard=On (1), matchCeiling=-1, strength=0.55, maxBoost=3, maxCut=6, floorDb=-55, holdMs=100, riseMs=250, fallMs=140, depthDb=0, rightPhase=0, measure=RMS (0), matchBoost=6, matchCut=18

Reject/revise if: Match runs as an unbounded live rider or raises a silent clip.

Intent check: Prepare a bounded RMS match capture for comparing source clips at -18 dBFS. Assess against: Match runs as an unbounded live rider or raises a silent clip.. Original design-stage calibration status: **not_run**; final numerical results and individual owner-audition status are in RACK_BANK_REPORT.md.

Required native action: Capture native Fader Match on an active clip window; persist measured matchedDb; silence must not produce a boost.

### HR248 - Studio Dual Mono Safe Fold

**Utility / 120 BPM.** Source: an impulse, tone burst and representative stereo musical phrase

Fold a stereo source to mono for a deliberate center-only utility.

EQ: flat

Full range:

- HamloFader (amount 1): engine=Modern (1), enabled=On (1), mode=Trim (0), gainDb=-3, targetDb=-18, mix=1, polarity=Off (0), mute=Off (0), solo=Off (0), inputDb=0, outputDb=0, leftDb=0, rightDb=0, matchedDb=0, peakGuard=On (1), matchCeiling=-1, strength=0.55, maxBoost=3, maxCut=6, floorDb=-55, holdMs=100, riseMs=250, fallMs=140, depthDb=0, rightPhase=0, routing=Mono Sum (1)

Reject/revise if: The fold clips or the result retains a left-right difference.

Intent check: Fold a stereo source to mono for a deliberate center-only utility. Assess against: The fold clips or the result retains a left-right difference.. Original design-stage calibration status: **not_run**; final numerical results and individual owner-audition status are in RACK_BANK_REPORT.md.

### HR249 - Studio One Beat Breathing

**Utility / 120 BPM.** Source: an impulse, tone burst and representative stereo musical phrase

Apply a clear quarter-note dip and recovery to a sustained input.

EQ: flat

Full range:

- HamloFader (amount 1): engine=Modern (1), enabled=On (1), mode=Motion (3), gainDb=0, targetDb=-18, mix=1, polarity=Off (0), mute=Off (0), solo=Off (0), inputDb=0, outputDb=0, leftDb=0, rightDb=0, matchedDb=0, peakGuard=On (1), matchCeiling=-1, strength=0.55, maxBoost=3, maxCut=6, floorDb=-55, holdMs=100, riseMs=250, fallMs=140, depthDb=9, rightPhase=0, shape=Custom (0), points=4, sync=Sync (1), beats=1, edgeMs=5, smooth=Smooth (1), level00=0, level01=0.2, level02=0.75, level03=1

Reject/revise if: The envelope clicks at the grid edge or free-runs after transport reset.

Intent check: Apply a clear quarter-note dip and recovery to a sustained input. Assess against: The envelope clicks at the grid edge or free-runs after transport reset.. Original design-stage calibration status: **not_run**; final numerical results and individual owner-audition status are in RACK_BANK_REPORT.md.

### HR250 - Studio Filtered Space Return

**Utility / 120 BPM.** Source: an impulse, tone burst and representative stereo musical phrase

Specify a pure wet-only filtered chamber return for an aux bus.

EQ: flat

Full range:

- HamloVerb (amount 1): space=Chamber (2), engine=Warm (2), decaySeconds=1.25, predelayMs=18, predelaySync=Free (0), lowCutHz=450, highCutHz=7500, bassMult=0.75, hfDecayRatio=0.5, width=1.1, modDepth=0.1, duck=0.2, routing=Send (1), mix=1, keepDry=Off (0), wetDb=0, outputDb=0, earlyDb=0, lateDb=0, timeShape=Off (0), shimmerAmount=0, freeze=Off (0)

Reject/revise if: Any original dry impulse remains at the output or low-frequency tails build.

Intent check: Specify a pure wet-only filtered chamber return for an aux bus. Assess against: Any original dry impulse remains at the output or low-frequency tails build.. Original design-stage calibration status: **not_run**; final numerical results and individual owner-audition status are in RACK_BANK_REPORT.md.
