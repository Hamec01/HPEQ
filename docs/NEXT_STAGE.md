# HamloEcho working handoff

2026-10-30: owner approved the Echo plan and implementation. Scope: E:/HamloEcho only. Verb is complete; do not modify it. No subagents, subtree, integration or push.

Completed stage: M0 baseline and Echo-only Legacy extraction. Baseline commit eb3b9a9; the extraction is the following stage commit (see git log). Design/targets: docs/HAMLOECHO_V2.md. Original tests 109/109; 160 captures/self-checks and 160/160 extracted renders bit-identical. Current tests 590/590; old states restored on fresh/edited instances; pluginval 5..10 pass; Steinberg 537/537; legacy_core.png visually inspected. No active jobs.

Archive: out/m0 (ignored, preserve it). Full evidence: docs/reports/v2/LEGACY_EXTRACTION_REPORT.md, M0_BASELINE.md, M0_SHA256.txt, legacy_after.json and validators/. Source has been narrowed to 26 files; ten cpp files build the shared library. Tools/isolate_echo_m0.py records the completed extraction only; do not run it over future V2 implementation. Preserve Tests/EchoLabMain.cpp as the frozen compatibility lab. New measurements can live in a separate lab executable without altering the 160-case archive contract.

Next coherent stage: prototype eight note voices and reverse capture before implementing the complete new DSP. Measure pitch accuracy/sidebands/transients and CPU first; include mono/stereo and tempo events. Decide buffer sizes, wet accumulation latency and bounds from measurements; keep dry/host latency zero. Manual SOURCE NOTE is the agreed initial reference; no automatic/polyphonic note correction is promised. New feature parameters must preserve all original host indices, including bypass at index 27. Old state over an edited new instance must reset additions to Legacy defaults.

Remaining: new DSP/UI, 200 calibrated presets, release lab/audition and 0.2.0 installer/ZIP. Current build is still 0.1.0. No new effects or presets have been claimed complete. FL Studio audition of the extracted build remains unverified.

Model recommendation: Astra xhigh for DSP and unexpected compatibility/measurement failures; medium for build/test execution. No model/effort switch has been performed by the agent.
