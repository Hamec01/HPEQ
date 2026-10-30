# HamloEcho working handoff

2026-10-30: owner approved the Echo plan and implementation. Scope: E:/HamloEcho only. Verb is complete; do not modify it. No subagents, subtree, integration or push.

Current stage: M0 baseline and Echo-only Legacy extraction. Design/targets: docs/HAMLOECHO_V2.md. Baseline complete: 109/109 original expectations; 160 capture cases and 160/160 self-checks, zero failures. Archive: out/m0 (ignored, preserve it). Reports: docs/reports/v2/M0_BASELINE.md and M0_SHA256.txt.

Next: run Tools/isolate_echo_m0.py to derive the Echo-only source/build from pinned M0 ede0db8. Build Tests/Lab/VST3/Snapshot, verify the frozen 160 cases, run tests and validators. The new LegacyEchoCore files are not yet verified; do not call extraction complete until raw sample comparison passes. Preserve Tests/EchoLabMain.cpp unchanged during extraction. No active jobs are needed for continuation.

After verified extraction, prototype eight note voices and reverse capture before implementing new DSP. Manual SOURCE NOTE is the agreed initial reference; no automatic/polyphonic note correction is promised. New feature parameters must preserve all original host indices, including bypass at index 27. Old state over an edited new instance must reset additions to Legacy defaults.

Model recommendation: Astra xhigh for DSP and unexpected compatibility/measurement failures; medium for build/test execution. No model/effort switch has been performed by the agent.
