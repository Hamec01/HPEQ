# F5 library and release acceptance / 2026-11-04, before measurements

F4 parent6455e9f. Add200 recipes after the original seven and Default, exactly
208host programs. Preserve original0..7program names/settings/state/PCM, all
77host parameters/schema3/VST3identity and neutral modern construction.

Four genres x50:Boom Bap90,Trap140,Techno132,DnB174BPM. Per genre groups:
Kick6,Snare/Clap6,Hats/Cymbals5,Percussion4,Drum Bus7,808/Bass5,Vocals5,
Keys/Piano4,Samples/Melody4,Master4. Recipes distribute Match/Ride/Motion:
11/19/20per genre. Old utility Trim recipes remain. StableID,genre/group,
purpose/description,source material,tempo and Gentle/Medium/Heavy required.
Changing onlyMIX/Output or metadata is not an independent recipe. Relevant
parameter signatures must be unique within group across the four genres.

Match is a workflow recipe:matchedDb0 on load; Learn->Stop->Apply explicitly.
Calibration exercises this actual workflow before rendering, not auto-Apply on
preset load. Ride has Natural/Fast, timing/range/activity; Motion has genre
rhythmic curves/cycle/phase/right offset. All recipes use unityMIX/output so
work is not manufactured by a final gain change.

Material:owner WAVs in existing mat/wav folder, strongest6s window, resample48k,
RMS target-18dBFS subject to sample peak-9dBFS. The conservative common9dB peak
headroom permits Ride's up-to6dB boost; it is not a limiter/preset output pad.
Store original/preparedSHA,start/length/rate/gain and actual prepared levels.
Active20ms windows after first0.5s, inputRMS>-60dBFS. Work is the mean absolute
window energy-level difference between actual output/input, in dB. Bands:
Gentle1..3,Medium3..6,Heavy6..12. Goals2/4.5/9dB plus small genre offsets stay
inside those fixed bands. Calibrate Ride target, Motion depth, Match target.
Continuous root brackets may be found by scan; an unreachable goal fails and
is disclosed, never repaired by changing the band. Final check rerenders the
compiled recipes. Peak<=-1dBTP on these prepared demonstrations, not a universal
peak ceiling for arbitrary audio. Verify independently at8x from exported WAVs.

Acceptance:200/200 finite/in-band/unique/valid metadata/ranges/state/0latency;
50per genre and exact group counts; actual Match proposals/Apply valid; old
states reset new recipes correctly. Resume per-ID calibration only when DSP,
method,input and uncalibrated-recipe identities match. Do not reuse another DSP.
Preserve failed/intermediate reports. Kernel unchanged; rerun original host
checks and legacy comparison. Library baseline reportsn/a before it exists.

F5LegacyLab is a separate version of F3LegacyLab, with host-program contract
enumeration explicitly restricted to original0..7. Same226audio cases and three
restore procedures; own method ID includes this source, allocation audit and
this document. Capture BEFORE library changes and verify AFTER. Do not relax
or edit the accepted F3method to accept208programs.

Finalrelease0.2.0:review genre-filtered UI and all pages;native/host/legacy
checks,pluginval5..10,Steinberg537,owner-material A/Bexamples,report/README/
AGENTS/handoff,portableVST3/ZIP/SHA256,localcommit. Binary in package equals
validated DLL. No installer,installed overwrite,GitHub/HPEQ push or manualFL
certification claim. Full acoustic DSP proof remains historical F2/F3 plus
unchanged kernel identity; do not claim new listening success from statistics.
