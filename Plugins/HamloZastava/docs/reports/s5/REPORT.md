# S5 release: HamloZastava 0.2.0

2026-11-03; parent b1f4f73. Four modern roles and Soft/Hard profiles now ship
with 200 calibrated genre recipes. The VST3/ZIP are ready for owner listening.
No installer, installed plug-in replacement, HPEQ, subtree or push change.

## Delivered behavior

The original Default plus six recipes keep programs0–6. New programs7–206 add
50 recipes each for Boom Bap88BPM, Trap144, Techno132 and DnB174. Per genre:
16Clean,14Drum,8Duck,12Rhythm. All have permanent ID, purpose, reference material,
tempo and depth. All200active processing signatures and names are unique, beyond
Mix/Output and reference BPM. All78parameter definitions/order/ranges remain.

Factory loading applies the modern recipe, clears key audition and preserves
independent host bypass. QUICK opens its role; Rhythm retains the pattern tab.
Genre/group filters navigate the expanded library. Modified marking compares
canonical parameter values to their recipe grid; it no longer depends on exact
normalized-float equality for new decimal settings. The original15comparison
rules remain. Modern project states save the permanent recipe ID; a recognized
ID wins over stale numeric position. Unknown IDs keep supplied settings with a
valid numeric fallback. Compact Legacy state encoding and old-state reset stay.

No DSP kernel changes in S5. Black Steel, shared PresetScreen and the Legacy
panel source are unchanged. Six actual images were inspected, including the
half-size32-step page: no clipping/overlap found; names/genre/group/count and
precision values are readable at their tested scale. Missing external key and
stopped synced transport explicitly show dry. Reference preset BPM and the
actual host BPM are separate values, as documented in the controls guide.

## Accepted measurements

| Check | Result |
|---|---|
| Compiled library | 200/200;207total host programs;200distinct active signatures |
| Annotation acceptance |64Clean,56Drum,32Duck,48Rhythm; all meet their declared depth band and role-specific limits |
| Clean protected detail | Worst wanted loss0.000166dB; quiet loss1.758086dB; onset loss0.079562dB (limits1/2/3dB) |
| Drum | Exactly8onsets for all56; worst first20ms loss0.993693dB (limit1dB); valid post-envelope windows;8..36dB rejection |
| Duck |1.999997..9.000000dB key-active attenuation; worst recovery loss0.084179dB (limit1.5dB) |
| Rhythm |8..36dB full/floor contrast; full-step gain effectively unity; at least2full and2floor steps per pattern |
| Peaks | Worst annotated -5.999999dBTP; worst natural -6.000000dBTP (4x polyphase; limit-1dBTP) |
| State/audio | All200restore selected ID, serialized state, parameters and PCM exactly on fresh and prepared/edited instances |
| Callback allocations | Zero measured C++ audio allocations in all library renders; native program switching also zero |
| Native suite |2507passed/0failed, including all207programs at blocks1/64/127/512/4096 after prepare256 |
| Host adapter |283passed/0failed/0n-a with the retained-prefix S5clone |
| Original M0 |229primary PCM/state/latency observations +458actual fresh/edited restoration observations exact |
| Kernel |275passed/0failed;66new PCM files equal accepted S4files, including five-minute192kstress |
| Cache |7negative guards reject changed DSP/method/catalog/input/recipe/request/raw output; valid job resumes without rewriting output |
| Validators |pluginval5,6,7,8,9,10exit0; Steinberg537passed/0failed |
| UI / auditions |6inspected actual editor images;16owner-material A/B pairs plus4Duck external-key WAVs |

Worst mean callback fraction in the400library fixture/natural runs was
6.820805% of audio duration at48kHz/256. These are wall-time
laboratory observations on this computer, including concurrent verification
activity; they are not a controlled FL Studio CPU benchmark. Fixed latency is
240samples at48kHz (5ms). All data/temporary audio buffers remain prepared.

## Provenance and disclosed iterations

S5_METHODS.md recorded the goals before the new catalog. S4sources, CMake, target
document and exact validated bundle were archived at out/s5/baseline. The initial
M0baseline clone matched all existing S1v3observations. New library functions are
n/a in M0. Frozen S2/S3/S4numerical drivers and criteria were not changed.
S5clones restrict Legacy fixtures/automation to the original seven positions;
the host clone checks that prefix, while library tests independently require207.

The initial authored catalog passed195/200. Four Hat Blips variants and DB Kick
Staccato triggered exactly8times but their short Body2.75..5.5ms began the tail
inside the protected attack window. Measured first20ms losses were1.095..3.148dB.
Body was manually corrected to20ms for Hat Blips and18ms for DB Kick Staccato.
No metric, fixture, threshold or DSP goal was loosened. This manual envelope
revision is separate from automatic threshold/onset/retrigger proposals.
Initial recipes/trials remain in initial_recipes.json and calibration_initial.json.
The revised pre-calibration inputs remain in calibration_seeds.json.

The disclosure changed the method stamp: tools were rebuilt, calibration was
rerun under that stamp (200/200), proposals were exported into source, then all
actual compiled recipes were rerendered without overrides for final acceptance.
Both M0/current Legacy sides were recaptured/verified with the final matching
S5method, again exact to the original S1primary and actual restored observations.
Routine test compilation/type corrections happened before runtime acceptance;
there was no numerical target change. Native/UI checks were run again after
final catalog export. Raw temporary trials and partial cache files are retained.

Cache identity covers DSP/mapping, measurement method, full catalog, recipe,
request and prepared input/key hashes. Reuse also verifies output SHA256. Final
source identity is embedded in the VST, snapshot and processor lab. Independent
audit recomputes metrics from raw files, validates applied recipe deltas, checks
retained source contracts and binds validator results to the exact shipped DLL.

## Evidence, artifacts and limits

Authoritative: acceptance.json, library_check.json, catalog.json, cache_guards.json,
calibration.json, kernel.json, host_final.json, legacy_final.json,
before_legacy_final.json, native_final.txt, screenshots.json, images/,
auditions.json and validators_final/. Package hashes/exact ZIP-DLL comparison
are recorded separately in package.json and dist/SHA256SUMS-0.2.0.txt.

Source catalog: Source/Presets/ZastavaRecipes.json and its generated C++ header.
Prepared audio/cache: out/s5/fixtures and out/s5/cache. Auditions:
E:/HamloZastava/out/s5/auditions (48kHz float WAV; dry has the same5msPDC).
Selected audition recipes cover every genre/role, not all200listening judgments.

Final compiled production source ID:
`55618b72cadcad7fc4c881ec53e747c503120aca1e32b2766f66937e8b3b969a`.
Final VST DLL SHA256:
`645ac589fa7903033057e8973dd7ae55e115f4e865711d9640cae800f9a25a5c`.
Kernel source ID remains
`9877b2194c06629e6b0f93535f8beb528dc748a7d201f949176bda3829c206ac`.

All S5numerical acceptance goals passed. **Owner FL Studio listening and host
workflow approval are pending.** Fixtures are constructed owner-derived test
arrangements; they do not establish semantic cleanup on every natural recording.
Natural excerpts check finite output/peak, not manually annotated wanted/noise
separation. Threshold and detector filtering must fit the recording. Mono main
uses the first channel of a stereo external key, preserving the documented S4
policy. The plug-in does not claim64-bit(double) processing support.

The release contains a separate VST3directory and ZIP with the exact validated
binary; no setup.exe was produced and no installed copy was replaced. Owner FL
checks: use INIT for a neutral modern start; try all four role filters; route a
kick to Duck's External Key; run/stop synced Rhythm; reopen a saved project and
verify chosen recipe/settings. Old projects can be checked against the retained
Legacy engine. No unperformed listening or FL acceptance is claimed.
