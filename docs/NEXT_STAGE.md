# HamloEcho working handoff

2026-10-30: owner approved the Echo plan and implementation. Scope: E:/HamloEcho only. Verb is complete; do not modify it. No subagents, subtree, integration or push.

Completed stages: M0 baseline eb3b9a9, Echo-only Legacy extraction 8c9cdb0, then the standalone note/Reverse prototype (following commit; see git log). Design/targets: docs/HAMLOECHO_V2.md and docs/PROTOTYPE_METHODS.md. Legacy tests remain 590/590; original 160 renders were bit-identical and validators passed. New prototype does not link into the VST3. No active jobs.

Archive: out/m0 (ignored, preserve it). Full evidence: docs/reports/v2/LEGACY_EXTRACTION_REPORT.md, M0_BASELINE.md, M0_SHA256.txt, legacy_after.json and validators/. Source has been narrowed to 26 files; ten cpp files build the shared library. Tools/isolate_echo_m0.py records the completed extraction only; do not run it over future V2 implementation. Preserve Tests/EchoLabMain.cpp as the frozen compatibility lab. New measurements can live in a separate lab executable without altering the 160-case archive contract.

Prototype complete as an experiment: 204/204 pitch cases, worst 0.2383 cents; 12/12 exact timing; 12/12 exact Reverse references; 20 block records and four long ring-wrap records agree; all 258 renders finite and C++ allocation-free. Final eight-voice CPU mean 15.3109%, two renderers 31.0961%; worst blocks 25.5507%/51.3057%. Isolated component only, not E10 for the full plugin. Shared coefficients 6,435,072 bytes; per-instance prepared history/windows 5,023,872 bytes at 48 kHz. Full evidence and limits: docs/reports/prototype/PROTOTYPE_REPORT.md. Both original and refined runs are retained; final executable matches all 254 measured refined render hashes.

Preserve ignored out/prototypes/ (raw runs, source snapshots and archived executables). Listening files: out/prototypes/AudioExamples/ (27 WAVs, source metadata and hashes in the report directory). Known-note demo: Notes_C5_to_C4_G4_C5_C3_Insert.wav. Material examples are interval textures with unspecified roots. No human audition is claimed.

Next coherent stage: resolve short-attack preservation and pitched rhythmic onset before making this the final drums pitch path. The impulse diagnostic loses up to 13.40 dB of raw energy and up to 11.19 dB at +24; this is an exposed limitation, not a passed quality requirement. First write a rate-aware transient reference and an onset-phase matrix before modifying sound. Pitch uses 80 ms dual windows and up to 20 ms matching; wet source-age bound reaches 260.71 ms at +24, with variable actual attack onset. Choose and document an explicit onset/capture policy. Reverse+pitch is not implemented yet. Do not silently ignore this combination in a production UI. Complete DSP integration follows; avoid tripling the costly pitch engine for three bands without measurements.

Manual SOURCE NOTE remains the initial reference; no automatic/polyphonic note correction is promised. Keep dry/host latency zero. New feature parameters must preserve all original host indices, including bypass at index 27. Loading an old state over an edited new instance must reset additions to Legacy defaults.

Remaining: new DSP/UI, 200 calibrated presets, release lab/audition and 0.2.0 installer/ZIP. Current build is still 0.1.0. No new effects or presets have been claimed complete. FL Studio audition of the extracted build remains unverified.

Model recommendation: Astra xhigh for DSP and unexpected compatibility/measurement failures; medium for build/test execution. No model/effort switch has been performed by the agent.
