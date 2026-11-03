# HamloFader: audit complete, F1 next

2026-11-03; parenta7d26e8, audit commit follows (git log -1). Owner approved full
Trim/Match/Ride/Motion as separate modes; Ride Natural/Fast and adjustable range.
No new feature question pending. No subagents, other products/HPEQ, installed
copy change, installer, push/subtree or claimed model switch.

Audit:32production units including whole HPEQ engine;8parameters,8programs,
0sample latency. Unchanged rebuild49/49native,1180x700panel inspected. Source,
Tests and CMake unchanged. Original/rebuilt VST/Tests/Snapshot byte-identical.
Archiveout/audit/original; reportdocs/reports/audit/REPORT.md andacceptance.json.
DLL43cf82d8e3287d83f0c5a94a9fa7e35d81ba4f359a32e8124d6e9d2f7feb2fc5.

F1_METHODS.md fixes the next isolation acceptance before new renders. Build a
single source/method-pinned original/current laboratory, capture PCM/states/
meters and actual fresh/edited restored observations, then isolate Legacy from
LayerEngine/FxNodeProcessor/unrelated modules. Preserve original8IDs/programs,
state/class identity, settled-float and changing-double gain paths, delta/mix
arithmetic, ramps and meter updates. No full Legacy compatibility claim yet.

Existing tests only use48kdespite a44.1/48klabel; they test one program's block
invariance and fresh parameter restoration. MATCH is UI target minus3sRMS, not
LUFS or a rider; Solo has no useful standalone action. Two Match factory presets
only set target; selected program is not serialized. SeeFADER_V2_AUDIT.md.

Build:cmake --build build --config Release --target HamloFader_Tests HamloFader_Snapshot HamloFader_VST3 -- /m:2 /v:minimal
Tests:build/Release/HamloFader_Tests.exe. JUCE8.0.12local atE:/HPEQ/build/_deps/juce-src.
Python:E:/HamloShirota/build/s3_venv/Scripts/python.exe -I -X utf8.
No active jobs. RecommendedAstra xhigh for isolation/measurement diagnosis;
medium for known build/test execution. No switch performed.
