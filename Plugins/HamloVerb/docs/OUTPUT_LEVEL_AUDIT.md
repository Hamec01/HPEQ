# Output-level audit / 2026-11-04

Owner clarified standalone HamloVerb,not Revolution. Parentfde1a57. First verify
the existing Insert/Send contract before choosing a corrective workflow.

Code hypothesis:Legacy adds wet over unity dry; new Insert characters use
(1-MIX)*dry+MIX*wet,then Output trim. Before the predelay has elapsed,the wet
signal is absent,so the source alone is reduced. Duck and time shaping affect
the wet path and may make this more apparent. Send intentionally has no dry.

Fixed measurement before execution:48kHz,1000Hz/-12dBFS sine,Predelay500ms,
neutral wet processing,one second initial silence. Measure the first4096 source
samples,strictly before wet arrival. All6 new characters atMIX.25/.5/1:
dry coefficient.75/.5/0 within1e-5; Legacy MIX.5 keeps coefficient1; new Send
is exactly zero in that window; Output-6dB scales the mixed signal by-6dB.
Also enumerate the installed208-program source bank's mix/output ranges.

This proves gain law and routing,not the exact cause in the owner's unsupplied
project. Preset/character,Insert/Send,MIX,Output,Duck and a sample would be needed
to distinguish a second issue. Do not copy the unrelated Revolution guard fix
into HamloVerb based on its measurements. Keep production audio and presets intact.

Confirmed:all18 modern early-window probes follow1-MIX; Legacy keeps unity.
The200 modern presets haveMIX.153442..944734,Output0,so their source coefficient
can be as low as.055266(-25.15dB) while the delayed wet has not arrived.
Owner reiterated that the sample itself becomes quieter. Implement a selectable
KEEP DRY workflow:unity dry+MIX*wet in modern Insert; Send unchanged; Output still
scales the whole result. Default off and old state missing ID resets off to keep
existing sound. Preserve this workflow choice across preset loads like routing.
Use a20ms sample-clocked transition; append the parameter; no new audio buffer.

Acceptance fixed before production change:all208 cold program renders match the
new frozen baseline; KEEP DRY early windows exactly dry atOutput0 for all6 modes
andMIX0/.25/.5/1; dry+MIX*Send identity within2e-6; Send exact with either setting;
state migration/roundtrip; blocks1/64/127/512/4096 exact in mono/stereo including
parameter events; old390 M0 reference cases; full native suite and validators.
No guarantee of constant total loudness or peaks:wet interference,Output trim,
Duck and Send behavior are separate. Existing frozen library calibration applies
to the original balance law; KEEP DRY is an explicit alternate workflow.
