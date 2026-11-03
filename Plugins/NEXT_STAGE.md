# Hamlo source publication handoff - 2026-11-03

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
