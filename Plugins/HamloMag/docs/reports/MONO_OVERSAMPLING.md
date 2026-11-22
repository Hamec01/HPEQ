# Mono oversampling correction, 0.2.2

The HamloRack expanded Debug parity matrix exposed CRT allocations only in mono
8x / selected sampler settings. The captured stack is JUCE assertion logging at
DelayLine::process (prepared stereo state versus a mono downsampling block),
not a request to relax the zero-allocation requirement. The initial run failed
60 allocation/free expectations. Ordinary Release output stayed finite and
exact against the old native wrapper, so Release-only probes missed this defect.

Keep the existing stereo oversamplers and pad their unobserved right input/output
with a 64-float stack buffer. No heap, layout/parameter/latency change or kernel
substitution is introduced. The same shared DSP source is compiled by standalone
HamloMag and the rack adapter. The stereo path performs the same operations;
the mono observable channel keeps its previous computation. A new test compares
mono against silent-right stereo at 44.1/48/96 kHz, 2x/4x/8x, four sampler modes.
The rack's strict native parity and Debug CRT probe cover the repaired path.

Validation results are recorded in the final HamloRack integration receipt.
The imported standalone base is the RACK0 0.2.1 source tree; this is an explicitly
authorized local source correction, not a claim of an unchanged upstream tree.
