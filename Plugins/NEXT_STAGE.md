# Hamlo source refresh handoff - 2026-11-04

Owner requested all current sources pushed to GitHub Hamec01/HPEQ. Updated
dedicated plugins/hamloverb to HamloVerb0.2.1 source115db13 and
plugins/hamlorevolution to verified HamloRevolution0.3.0 source8470eeb, then
merged into feature/hamlo-plugins. Other8 dedicated branches already match.
Exact10/10 subtree trees and standalone ancestry checked on aggregate AND
dedicated branches. Dedicated diffs touch only own subtree/manifest; aggregate
diff touches only Plugins/. Source identity manifest:sync/2026-11-04.json.

Replay of original product acceptance receipts passed:
E:/HamloVerb:python -I -X utf8 Tools/verify_keepdry.py
E:/HamloRevolutionTransitions:python -I -X utf8 Tools/verify_r12.py
This is source synchronization; no DSP changes or fresh build/host-test claim.
Build/dist/out and private user audio stay excluded. No binary GitHub release.

Revolution original master retains parked R11 draft, unchanged/uncommitted.
Do not merge it into R12 or call it released. E:/HamloRevolutionTransitions is
the authoritative clean0.3.0 source. HPEQ main worktree stays on its original
feature/hpeq-m0-m1. Fader remains unfinished0.1.0 F3, UI/library pending.

Final metadata commit follows this handoff. Publish aggregate+10 topic refs in
one non-force atomic push; verify all11 remote hashes and clean/up-to-date
aggregate afterward. Local evidence:out/publish-20261104, including branch
trees, blob-size preflight and push/readback result. No subagents/model switch.
Future pushes still require a new owner request. Medium is sufficient for git;
Astra xhigh for DSP/compatibility diagnosis; no setting changed here.

# Historical publication handoff - 2026-11-03

Owner request: commit/push all standalone plug-ins into HPEQ, with individual
branches. All10sources were clean and committed already; no empty source commits.
WorktreeE:/HPEQ-hamlo-plugins stays onfeature/hamlo-plugins; mainE:/HPEQ remains
onfeature/hpeq-m0-m1 and untouched. No subagents/model switch/installed VST edit.

Tenplugins/*branches were created from28d237e. Everybranch imports only its own
changed subtree (if needed) plusPlugins/sync/<name>.json. Existing7trees already
matched; Zastava/Fader/Revolutionupdated. Fullsourcehistoryretained. All branches
merged intoaggregatefeature/hamlo-plugins. Registry/provenance updated separately
soeachPlugins/<name>tree remains byte-exact to its standaloneHEAD. Versions and
source/topicheads inPlugins/sync/2026-11-03.json. Publicationmetadata commit
follows this handoff; identify tip withgit log-1feature/hamlo-plugins.

Faderinternal0.1.0isF3integration only; UI/library stillpending. Others retain
individual reports/limits. EQstaysinHPEQ;HamloProd containsawebsite,notaudioplugin.
Build/dist/out caches are ignored; no forced artifact inclusion or GitHub release.

Checks for this source-only synchronization:10exactsubtreeGit treeIDs/source
ancestor checks; topicdiffscope restricted to ownprefix/manifest; aggregate
changes onlyPlugins/; cleanstandalonecheckouts/sourceheadsunchanged; GitHub
blob-size preflight; non-forceatomicpush of aggregate+10branches; remoteheads
readback vs localIDs. NoDSPcodechange/rebuildtestclaim. Preserveexistingplugin
acceptance. Details/localreceipts:out/publish-20261103/*.json/logs inthisworktree.

On successfulpush gitstatusmustbeclean andaggregateupstreamnotahead/behind.
Ifsourcechangeslater, subtreepullfromtheactualsourcebranch,verifyexacttreeand
recordnewprovenance; don'ttreatthissnapshotaspermissionforfuturepushes. Continue
onlytheowner'srequesteddevelopment. Mediumsufficesforroutinegit/buildwork;
AstraxhighforDSP/compatibilityinvestigation;noswitchperformed.
