# HamloComp — agent notes

- The owner's compressor as its own VST3, extracted from HPEQ (E:\HPEQ). Chat with the owner in Russian;
  everything in the product (plug-in text, installer, README, notes) in English.
- Rules as in HPEQ: measure before tuning, the same test before and after, numbers not impressions, disclose
  any threshold or method change made after a run.
- A DSP change must keep `HamloComp_Tests` green and is checked against the reference
  (`HamloComp_Reference`); a deliberate change of the sound is reported with numbers, and noted in
  docs/SYNC_WITH_HPEQ.md so it can be carried back into HPEQ.
- Parameter IDs, ranges, the state tag and the plug-in codes (BBLB / HmCp) are permanent: saved projects and
  automation depend on them.
- Commit once per stage.
