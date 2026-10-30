# Immutable prototype reference

`DSP/ReflectionPrototype.*` is the exact own prototype source from commit 05a1445, before attack preservation.
It is compiled only by HamloEcho_AttackBaselineLab, using the same current lab harness as the candidate.
Never link it into the VST3 or edit it to match later DSP. It permits baseline reruns when a measurement
method is corrected or strengthened without changing the old algorithm. Original executables/raw runs
are also preserved under ignored out/prototypes and out/attacks.
