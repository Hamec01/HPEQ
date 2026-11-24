# Macros, external sidechain and final output

## Host automation

Open **Macros**, choose **Assign**, then choose a module and its parameter.
The sixteen host controls always retain the names **Macro 01** through
**Macro 16** and IDs `macro01` through `macro16`, regardless of module order.
Record or draw automation for these controls in the DAW. The editor can stay closed.

A macro can control several module parameters. A parameter has one macro.
**Range** sets the physical values at macro 0 and macro 1; reversed ranges work.
Choose a destination from Range to edit or remove just that assignment.
**Clear** removes all destinations of that macro. Unassign and Clear retain
the current mapped parameter targets. These actions have separate Undo steps.
The rack Undo button covers project edits and assignments; macro and gain
parameter values use the DAW's parameter undo, following the existing Output policy.

Assignments belong to modules. Moving a module retains them; copying a module
explicitly copies them; deleting it removes its destinations. They do not attach
to an unrelated replacement at the same rack position. Save the DAW project or
a user preset to retain the assignments, endpoint ranges and macro values.

The embedded module's original knob also controls its assigned macro and forwards
its host gesture. Other destinations of that macro follow it. The macro panel
shows the first destination and a count; its tooltip lists the other destinations.

This version maps the embedded modules' exposed native parameters. Rack EQ points,
region boundaries and layer controls are outside this mapping system. Macro
targets have a 20 ms ramp and a fixed 16-sample control clock; modules retain
their own parameter smoothing. Discrete choices still switch at their native
steps. JUCE delivers VST3 automation at audio callback boundaries.

## External compressor key

Route the source track to HamloRack's **Sidechain** input in the DAW, then enable
**External SC** on the desired HamloComp page. Each compressor chooses its
detector separately. The auxiliary bus starts disabled and supports mono or
stereo with a matching mono or stereo main input/output.

A disabled or unavailable auxiliary bus uses internal detection. An enabled
silent key produces no gain reduction. The compressor's SOURCE selection still
shapes its detector. All five models work with the key; key audio is never mixed
into the program output. The rack aligns it to the program's accumulated latency
at each compressor. Mid/Side processing uses the full external key; Dual Mono
uses the respective left/right key, with a mono key shared by both channels.
If the DAW keeps the key bus enabled after removing a send, its silent input
still counts as an external key. Disable the bus or External SC for internal detection.

Actual FL Studio sidechain routing and a saved-session audition remain owner
checks; independent hosts and validators do not certify a particular DAW setup.

## Final output

**Final Trim** (-24 to +12 dB, default 0) follows every effect, tube colour,
Master limiter and the existing Output control. It does not drive the limiter
harder. Positive trim can raise the final signal above a limiter's ceiling.
The final output meters include this gain. Settled global Bypass returns the
aligned dry signal with neutral gain.

The **CLIP** indicator latches final true-peak overload. Click it to reset peaks
without clearing integrated loudness. The existing loudness **Reset** remains
a complete loudness reset. Final Trim and monitoring do not add a clipper or
another mastering limiter.

New assignments, External SC and non-neutral Final Trim use state schema 3.
Old schema 1/2 projects load with their original controls and neutral new
features. See [the stage contract](AUTOMATION_SIDECHAIN_OUTPUT.md) and
[current verification](CURRENT_VERIFICATION.json) for measured qualification.
