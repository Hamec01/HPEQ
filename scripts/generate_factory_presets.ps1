[CmdletBinding()]
param(
    [string] $OutputDirectory = (Join-Path $PSScriptRoot '..\Assets\Presets\Factory\Drums-and-Bass')
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

function Eq {
    param([string]$Shape, [double]$FrequencyHz, [double]$GainDb = 0, [double]$Q = 0.707, [int]$Slope = 12)
    [ordered]@{ shape=$Shape; frequencyHz=$FrequencyHz; gainDb=$GainDb; q=$Q; slopeDbPerOct=$Slope }
}

function Fx {
    param([string]$Type, [double]$Mix = 1, [hashtable]$Params = @{})
    [ordered]@{ type=$Type; mix=$Mix; params=$Params }
}

function Preset {
    param([string]$Category, [string]$Name, [string]$Intent, [string[]]$Tags,
          [object[]]$Eq, [object[]]$Fx, [double]$OutputGainDb = 0, [bool]$Lenin = $false, [bool]$Stalin = $false)
    [ordered]@{ category=$Category; name=$Name; intent=$Intent; tags=$Tags; eq=$Eq; fx=$Fx;
                 outputGainDb=$OutputGainDb; lenin=$Lenin; stalin=$Stalin }
}

function Compressor {
    param([double]$Threshold=-24,[double]$Ratio=4,[double]$Attack=12,[double]$Release=90,
          [double]$Mix=1,[int]$Character=1,[int]$Detector=0,[int]$AutoMakeup=1,[double]$Knee=6)
    Fx Compressor $Mix @{ thresholdDb=$Threshold; ratio=$Ratio; kneeDb=$Knee; attackMs=$Attack;
        releaseMs=$Release; makeupDb=0; detector=$Detector; character=$Character; autoMakeup=$AutoMakeup }
}

function Transient {
    param([double]$AttackDb=4,[double]$SustainDb=-2,[double]$Mix=1,[double]$Speed=.62,
          [int]$Style=0,[double]$Rise=1,[double]$Decay=1,[double]$Preserve=1)
    Fx TransientShaper $Mix @{ attackDb=$AttackDb; sustainDb=$SustainDb; speed=$Speed; style=$Style;
        lookahead=1; rise=$Rise; decay=$Decay; sensitivity=.88; preserve=$Preserve; listen=0; outputDb=0 }
}

function Saturator {
    param([double]$Drive=6,[double]$Mix=1,[int]$Model=1,[double]$Tone=0,[double]$Harmonics=.5,
          [double]$EvenOdd=0,[int]$Thump=0,[int]$Punish=0,[double]$Trim=0)
    Fx Saturator $Mix @{ driveDb=$Drive; curve=0; bias=0; trimDb=$Trim; oversampling=2; autoGain=1;
        model=$Model; punish=$Punish; tone=$Tone; satLowCutHz=20; satHighCutHz=20000; thump=$Thump;
        steep=0; harmonics=$Harmonics; evenOdd=$EvenOdd; tapeBias=.5; speed=1; wow=.03; hiss=0;
        hum=0; crackle=0; noiseFollow=1 }
}

function Gate {
    param([double]$Threshold=-42,[double]$Ratio=8,[double]$Range=18,[double]$Attack=.3,
          [double]$Hold=24,[double]$Release=100,[double]$Mix=1,[int]$Style=0,
          [double]$KeyHp=20,[double]$KeyLp=20000)
    Fx GateExpander $Mix @{ thresholdDb=$Threshold; ratio=$Ratio; rangeDb=$Range; attackMs=$Attack;
        holdMs=$Hold; releaseMs=$Release; hysteresisDb=4; style=$Style; lookahead=1; keyHpHz=$KeyHp;
        keyLpHz=$KeyLp; keyListen=0; flip=0; outputDb=0 }
}

function Stereo {
    param([double]$Width=1,[double]$Stereoize=0,[double]$Space=0,[double]$MonoBassHz=0,
          [double]$Mix=1,[int]$Style=0)
    Fx StereoProcessor $Mix @{ width=$Width; balance=0; style=$Style; stereoize=$Stereoize; space=$Space;
        monoBassHz=$MonoBassHz; rotation=0; safe=1; monoListen=0; outputDb=0 }
}

function Reverb {
    param([double]$Mix=.15,[int]$Space=2,[double]$Predelay=18,[double]$Decay=1.2,
          [double]$LowCut=180,[double]$HighCut=11000,[double]$Width=1)
    Fx Reverb $Mix @{ space=$Space; color=2; predelayMs=$Predelay; predelaySync=0; decaySeconds=$Decay;
        size=.8; hfDecayRatio=.45; lowCutHz=$LowCut; highCutHz=$HighCut; width=$Width; attack=.18;
        earlyDiffusion=.65; lateDiffusion=.7; modRate=.5; modDepth=.25; bassMult=1;
        bassXoverHz=300; highXoverHz=4000; distance=.25; duck=.35; freeze=0 }
}

function Limiter {
    param([double]$Threshold=-1,[double]$Ceiling=-1,[double]$Release=70,[double]$Mix=1,[int]$Style=0)
    Fx Limiter $Mix @{ thresholdDb=$Threshold; ceilingDb=$Ceiling; releaseMs=$Release; lookaheadMs=2;
        truePeak=1; autoRelease=1; style=$Style; link=1; monitor=0; outputDb=0 }
}

$presets = [System.Collections.Generic.List[object]]::new()

# Kick: weight, mud control and beater definition are moved independently so the pack works on both
# synthesized and recorded sources.  Compression timing follows the intended envelope, not a genre label.
$kick = @(
    @('Club Foundation','Firm four-on-the-floor weight with a clean beater edge','house,club',55,2.5,230,-3,3200,2.5,5,-2,-22,4,18,85,1,5,.72,1,0),
    @('Deep House Pillow','Rounded low end and softened attack','deep-house,warm',62,3,280,-2.5,4200,-1,-2,3,-25,3,5,150,1,4,.65,1,0),
    @('Techno Hammer','Hard mid punch with short controlled tail','techno,punch',52,2,190,-4,4100,4,7,-5,-20,6,24,65,1,9,.82,4,1),
    @('Warehouse Rumble Seed','Dark kick prepared for a short rumble tail','techno,rumble',48,2.5,210,-3,2400,1,3,1,-24,4,16,120,.88,7,.68,1,0),
    @('909 Clean Drive','Classic electronic knock with restrained tape colour','909,house',58,2,310,-2,2800,3,4,-2,-23,4,20,95,1,6,.72,1,0),
    @('808 Short Punch','Sub-focused electronic kick with a tighter envelope','808,trap',46,3,180,-2.5,1800,2,5,-6,-26,4,15,105,1,7,.7,5,1),
    @('Boom Bap Dust','Warm body, woody knock and softened top','boom-bap,lofi',68,2.5,260,-2,780,2.5,2,2,-24,3,12,140,.82,8,.68,2,0),
    @('Trap Knock','Compact low end and audible click for dense arrangements','trap,modern',51,2,240,-3.5,3600,3.5,6,-4,-22,5,18,75,1,8,.75,4,0),
    @('DnB Needle','Short sub, pronounced attack and fast recovery','dnb,fast',57,1.5,300,-4,5200,4,8,-7,-20,5,22,48,1,7,.78,4,0),
    @('Rock Inside Mic','Beater definition with controlled low-mid boxiness','rock,acoustic',72,2,340,-3,2700,4,5,-1,-21,4,28,110,1,5,.65,3,0),
    @('Rock Outside Mic','Low-end tail and gentler front edge','rock,acoustic,body',64,3,250,-2,3500,-1,-3,4,-27,6,1.5,180,1,4,.62,1,0),
    @('Metal Trigger Seat','Very tight low end and cutting beater definition','metal,tight',66,1.5,420,-4.5,5800,5,9,-8,-18,7,30,55,1,8,.82,4,0),
    @('Indie Soft Felt','Natural rounded transient and open sustain','indie,soft',78,2,300,-1.5,2800,-1,-2,4,-25,2.5,8,180,.78,3,.5,1,0),
    @('Funk Tight Foot','Dry, articulate and fast without excessive sub','funk,tight',82,1,210,-3,2200,3,5,-6,-21,4,22,70,1,5,.62,3,0),
    @('Pop Radio Punch','Balanced low end, click and consistent envelope','pop,polished',60,2,250,-3,4200,2.5,5,-2,-24,4,24,100,1,5,.72,1,0),
    @('Lo-Fi Cardboard','Mid-forward, bandwidth-limited character','lofi,character',88,1,240,-1,1250,3,1,3,-25,3,6,170,.78,10,.74,2,0),
    @('Hardcore Concrete','Aggressive clipped weight for hard dance','hardcore,distorted',50,3,180,-2.5,3000,4,8,-3,-18,8,12,55,1,15,.88,4,1),
    @('Sub Safe Utility','Conservative cleanup and level control','utility,clean',55,1.5,260,-2,2500,1,2,-1,-20,3,20,120,1,3,.5,0,0)
)
foreach ($p in $kick) {
    $eq = @((Eq LowCut 27 0 .707 24),(Eq Bell $p[3] $p[4] .8),(Eq Bell $p[5] $p[6] 1.15),(Eq Bell $p[7] $p[8] 1.0),(Eq HighCut 16500 0 .707 12))
    $fx = @((Transient $p[9] $p[10] 1 .68 0 1 1 1),(Compressor $p[11] $p[12] $p[13] $p[14] $p[15] 1 0 1 5),(Saturator $p[16] $p[17] $p[18] .05 .55 0 $p[19] 0))
    if ($p[0] -eq 'Warehouse Rumble Seed') { $fx += Reverb .18 5 0 .8 90 4200 .7 }
    elseif ($p[0] -in @('Metal Trigger Seat','Funk Tight Foot','DnB Needle')) { $fx = @((Gate -40 10 22 .15 18 70 .9 0 28 10000)) + $fx }
    elseif ($p[0] -eq 'Sub Safe Utility') { $fx += Limiter -1 -1 85 .8 0 }
    $presets.Add((Preset Kick $p[0] $p[1] ($p[2] -split ',') $eq $fx -1))
}

$snare = @(
    @('Pop Center Crack','Centered crack with polished body','pop,polished',205,2.5,520,-3,3200,4,10500,1.5,6,-2,-24,4,18,105,1,5,.6,4,.08),
    @('Trap Rim Snap','Lean body and sharp upper-mid snap','trap,sharp',240,1,680,-3,4800,5,12000,2,8,-5,-22,5,12,75,1,7,.7,4,.05),
    @('Boom Bap Wood','Woody low-mid knock with a dusty top','boom-bap,warm',190,3,430,-2,850,3,7200,-1,3,2,-25,4,8,145,.85,8,.7,2,.06),
    @('Rock Shell','Full acoustic shell with forward crack','rock,acoustic',185,3,720,-3,2500,4,9000,1,5,0,-23,4,24,130,1,5,.52,3,.1),
    @('Arena Backbeat','Large snare with bright room-ready attack','rock,big',175,3,600,-2.5,2800,4,11000,2,6,2,-25,5,30,180,1,6,.6,3,.2),
    @('Metal Rifle','Extremely tight, bright and consistent','metal,tight',210,1.5,500,-4,5200,6,12500,2,9,-7,-19,8,20,60,1,8,.75,4,.04),
    @('Indie Tea Towel','Dry softened snare with shortened sustain','indie,dry',220,2,750,-2,2600,1,9000,-2,-2,-6,-27,3,6,120,.9,3,.45,1,.03),
    @('Disco Hand Snap','Bright controlled snap with light ambience','disco,bright',230,1.5,450,-2.5,4200,4,11500,2,5,-3,-23,4,16,90,1,5,.55,4,.13),
    @('Neo Soul Pocket','Round body and relaxed transient','neo-soul,warm',195,2.5,620,-2,2100,2,9200,.5,2,2,-26,3,10,165,.82,4,.48,1,.08),
    @('DnB Ghost Cutter','Fast recovery and articulate ghost notes','dnb,fast',215,1,500,-3,3800,4,10000,1,7,-4,-24,5,14,55,1,6,.55,4,.04),
    @('Techno Clap Stack','Wide bright clap/snare cohesion','techno,clap',250,0,700,-2,1800,2,9500,3,5,-1,-23,4,8,100,.9,8,.68,4,.15),
    @('Gated Eighties','Large bright hit with deliberately gated tail','80s,gated',180,3,650,-2,2400,3,10500,2,7,4,-22,6,5,160,1,5,.55,3,.28),
    @('Lo-Fi Paper','Narrow, papery and textured','lofi,character',230,1,480,-1,1450,4,6500,-3,2,3,-26,3,5,170,.8,10,.72,2,.05),
    @('Funk Piccolo','Higher tuning and quick lively transient','funk,piccolo',280,2,620,-2.5,3600,4,10500,1.5,7,-5,-22,4,20,75,1,5,.5,3,.07),
    @('Brush Gentle','Soft dynamics and natural airy tail','jazz,brush',180,1,500,-1.5,2800,.5,12000,1,-3,5,-30,2,4,220,.72,2,.35,1,.14),
    @('Side Stick Focus','Midrange definition without false low weight','sidestick,utility',320,1,700,-2,2200,4,9000,0,5,-5,-25,3,10,95,.9,4,.45,3,.04),
    @('Snare Rescue Neutral','Moderate cleanup, punch and peak safety','utility,clean',200,2,560,-3,3000,3,10000,1,4,-2,-24,4,20,120,1,4,.45,1,.05)
)
foreach ($p in $snare) {
    $eq = @((Eq LowCut 75 0 .707 18),(Eq Bell $p[3] $p[4] 1),(Eq Bell $p[5] $p[6] 1.2),(Eq Bell $p[7] $p[8] 1),(Eq HighShelf $p[9] $p[10] .7))
    $fx = @((Transient $p[11] $p[12] 1 .65 0 1 1 1),(Compressor $p[13] $p[14] $p[15] $p[16] $p[17] 1 0 1 6),(Saturator $p[18] $p[19] $p[20] .08 .55 .08 0 0))
    if ($p[21] -gt .1) { $fx += Reverb $p[21] $(if($p[0] -eq 'Gated Eighties'){4}else{2}) 18 $(if($p[0] -eq 'Gated Eighties'){1.5}else{.85}) 220 11000 1.1 }
    if ($p[0] -in @('Metal Rifle','Indie Tea Towel','Gated Eighties')) { $fx = @((Gate -38 10 24 .12 35 100 .92 0 90 12000)) + $fx }
    $presets.Add((Preset Snare $p[0] $p[1] ($p[2] -split ',') $eq $fx -1))
}

$hats = @(
    @('Airy Pop Closed','Clean airy closed hat with gentle peak control','pop,closed',420,7800,-2,12500,2,3,-3,3,.4,1.15,.12),
    @('Trap Tick','Short bright tick that stays out of the vocal','trap,closed',650,6200,-3,10500,3,7,-8,5,.5,1.05,.08),
    @('Dark House Shuffle','Dark softened hats for rolling house grooves','house,dark',380,7200,-2,9800,-2,1,-1,5,.65,1.18,.14),
    @('Techno Steel','Hard metallic presence with controlled harshness','techno,metallic',500,4300,-2.5,11800,2,4,-4,8,.72,1.22,.18),
    @('DnB Razor','Fast bright definition for dense breakbeats','dnb,bright',700,6900,-2,13500,3,8,-7,6,.55,1.1,.1),
    @('Boom Bap Dust Hats','Rolled-off dusty loop treatment','boom-bap,lofi',300,7600,-1,10500,-3,0,2,8,.7,1.08,.08),
    @('Open Hat Tamer','Controls long open-hat sustain without dulling attack','open-hat,control',320,5200,-3,11000,1,5,-6,4,.45,1.18,.12),
    @('Disco Silk','Wide silky top with restrained 8 kHz bite','disco,smooth',260,7800,-2.5,13000,2,2,-1,3,.45,1.32,.16),
    @('Acoustic Hat Natural','Minimal cleanup for recorded cymbals','acoustic,natural',180,6800,-1.5,12000,.5,1,0,2,.3,1.05,.04),
    @('Indie Wash','Soft widened hat wash','indie,wide',220,5000,-2,10500,1,-1,2,3,.45,1.3,.18),
    @('Funk Chick','Dry mid-forward foot/closed-hat articulation','funk,dry',420,6500,-2,9800,1,6,-6,3,.38,1.08,.05),
    @('Hyperpop Spark','Very bright controlled synthetic sparkle','hyperpop,bright',800,7400,-3,15000,4,7,-4,9,.75,1.35,.2),
    @('Lo-Fi Cassette Hat','Bandwidth-limited and gently unstable colour','lofi,tape',280,6100,-1,9200,-4,0,2,11,.8,1.12,.06),
    @('Shaker Pocket','Smooths shaker peaks while preserving motion','shaker,perc',260,4500,-2,11500,1,2,-2,3,.4,1.25,.12),
    @('Cymbal Safe Utility','Conservative harshness control and mono safety','utility,clean',200,7200,-2,13500,1,1,-1,2,.25,1,.02)
)
foreach ($p in $hats) {
    $eq = @((Eq LowCut $p[3] 0 .707 24),(Eq Bell $p[4] $p[5] 1.8),(Eq HighShelf $p[6] $p[7] .7),(Eq HighCut 19000 0 .707 12))
    $fx = @((Transient $p[8] $p[9] .9 .72 1 1 1 1),(Saturator $p[10] $p[11] $(if($p[0] -eq 'Lo-Fi Cassette Hat'){2}else{4}) .18 .6 .15 0 0),(Stereo $p[12] .18 .15 0 .9 0))
    if ($p[13] -gt .1) { $fx += Reverb $p[13] 1 8 .65 500 14500 1.2 }
    $presets.Add((Preset HiHat $p[0] $p[1] ($p[2] -split ',') $eq $fx -1.5))
}

$perc = @(
    @('Congas Natural','Keeps hand percussion lively while evening hits','conga,natural',85,240,-2,900,2,4500,1,3,0,-28,3,8,120,.8,3,.42,.08),
    @('Toms Modern','Solid body, reduced boxiness and present stick','toms,modern',45,420,-3,900,2.5,4200,2,5,-2,-24,4,18,130,1,5,.55,.08),
    @('Toms Vintage','Round darker toms with tape-like cohesion','toms,vintage',40,500,-2,760,2,3200,0,2,2,-27,3,7,190,.85,7,.64,.12),
    @('Bongo Snap','Fast hand attack with trimmed low-mid ring','bongo,fast',110,380,-2.5,1200,2,5500,2,7,-4,-25,4,12,85,.9,4,.5,.06),
    @('Cajon Knock','Balances bass thump and front-plate snap','cajon,acoustic',55,280,-2.5,950,2,3200,3,5,-1,-25,3.5,18,125,.9,4,.42,.07),
    @('Claves Cut','Narrow woody attack that cuts without harshness','claves,wood',250,900,-2,2100,3,5700,-2,5,-5,-28,3,8,80,.75,3,.35,.05),
    @('Cowbell Control','Tames ring while retaining the musical pitch','cowbell,control',180,560,-2,1200,1,3400,-3,2,-1,-26,3,10,140,.85,4,.4,.07),
    @('Tambourine Air','Bright tambourine energy with controlled peaks','tambourine,bright',380,4200,-2,8500,2,13000,2,4,-2,-27,3,6,95,.8,3,.4,.14),
    @('Shaker Glue','Smooth, wide shaker bed for pop and dance','shaker,wide',300,3500,-1.5,7200,-2,12000,2,2,1,-30,2.5,5,150,.72,2,.3,.12),
    @('Latin Bus','Cohesive mixed Latin percussion subgroup','latin,bus',55,320,-2,1800,1.5,7600,1,3,0,-24,3,15,135,.9,4,.48,.1),
    @('Afro Pocket','Preserves hand dynamics with warm midrange focus','afro,organic',60,280,-1.5,1050,2,6200,1,3,1,-27,2.5,12,160,.82,4,.42,.12),
    @('Tech Perc Click','Short synthetic click and tightly gated decay','techno,synthetic',140,500,-3,2400,3,7200,2,7,-7,-22,5,8,65,1,7,.65,.04),
    @('Industrial Metal','Aggressive metallic percussion texture','industrial,distorted',120,700,-2,2600,2,6800,3,6,-2,-20,6,10,90,1,11,.78,.12),
    @('Cinematic Hits','Large controlled impacts with width and tail','cinematic,big',28,180,-2,850,2,4200,1,5,3,-24,4,25,220,.9,6,.52,.24),
    @('Percussion Rescue','Conservative cleanup and transient consistency','utility,clean',45,350,-2.5,1200,1.5,5200,1,3,-1,-25,3,15,130,.9,3,.35,.05)
)
foreach ($p in $perc) {
    $eq = @((Eq LowCut $p[3] 0 .707 18),(Eq Bell $p[4] $p[5] 1.2),(Eq Bell $p[6] $p[7] 1),(Eq Bell $p[8] $p[9] 1.1))
    $fx = @((Transient $p[10] $p[11] 1 .62 0 1 1 1),(Compressor $p[12] $p[13] $p[14] $p[15] $p[16] 1 1 1 6),(Saturator $p[17] $p[18] $(if($p[0] -eq 'Toms Vintage'){2}else{3}) .05 .5 .1 0 0))
    if ($p[0] -eq 'Tech Perc Click') { $fx = @((Gate -42 10 28 .1 12 60 1 0 120 14000)) + $fx }
    if ($p[19] -gt .1) { $fx += Reverb $p[19] $(if($p[0] -eq 'Cinematic Hits'){5}else{2}) 16 $(if($p[0] -eq 'Cinematic Hits'){2.2}else{.9}) 180 11500 1.15 }
    $presets.Add((Preset Percussion $p[0] $p[1] ($p[2] -split ',') $eq $fx -1))
}

$bus = @(
    @('VCA Gentle Glue','Subtle cohesion that keeps kick and snare transients','glue,clean',28,0,250,-1,9000,.5,-22,2,30,120,.85,3,.35,1.02,90,0),
    @('VCA Punch Bus','Forward transients and compact sustain','punch,vca',28,0,300,-1.5,8500,1,-20,3,35,85,.9,4,.42,1.04,80,0),
    @('Parallel New York','Aggressive compression blended under the dry kit','parallel,aggressive',35,0,280,-2,7000,1,-38,10,.5,70,.38,8,.55,1.02,65,0),
    @('Tape Cohesion','Warm peak rounding with mild high-frequency softening','tape,warm',25,.5,260,-1,10000,-.5,-24,2.5,18,150,.82,6,.65,1.02,90,0),
    @('Console Snap','Mid-forward transient focus and subtle saturation','console,punch',30,0,350,-1.5,8000,1,-22,3,25,100,.88,5,.5,1.04,75,0),
    @('House Pump','Tempo-friendly pumping for four-on-the-floor drums','house,pump',28,1,240,-1.5,9500,1,-30,5,3,190,.82,7,.48,1.03,75,0),
    @('Techno Clamp','Dense controlled drum bus for loud club arrangements','techno,dense',30,0,220,-2,7800,.5,-25,6,8,85,.95,8,.62,1.02,60,1),
    @('DnB Fast Glue','Fast recovery and bright transient retention','dnb,fast',36,0,300,-2,10500,1,-23,4,18,48,.88,5,.48,1.02,55,0),
    @('Boom Bap Weight','Warm low-mid body and parallel density','boom-bap,warm',28,1,320,-1,7200,-1,-30,6,8,150,.55,8,.62,1.01,90,0),
    @('Rock Kit Cohesion','Natural bus compression for acoustic drums','rock,acoustic',24,0,400,-1.5,10000,.5,-20,2.5,28,140,.88,4,.36,1.02,90,0),
    @('Metal Drum Armor','Tight consistent peaks with controlled cymbal edge','metal,tight',35,0,300,-2.5,6500,-1.5,-22,5,22,70,.94,7,.52,1.01,55,0),
    @('Indie Room Lift','Adds sustain and width without hard peak control','indie,room',24,.5,450,-1,9000,1,-27,2,12,190,.72,3,.32,1.1,100,0),
    @('Lo-Fi Drum Print','Dark compressed texture resembling a printed loop','lofi,print',40,0,300,-1,6500,-2,-29,5,5,170,.75,11,.75,1.01,80,0),
    @('Funk Pocket','Quick rhythmic recovery and restrained low end','funk,groove',32,0,280,-1.5,9000,.5,-23,3,24,65,.87,4,.38,1.02,70,0),
    @('Disco Polish','Smooth controlled top and gently widened image','disco,polished',28,.5,350,-1,11000,1,-22,2.5,26,115,.84,3,.3,1.08,90,0),
    @('Pop Drum Finish','Balanced modern polish with true-peak safety','pop,polished',28,0,300,-1.5,10500,.8,-23,3,25,105,.9,4,.4,1.03,65,0),
    @('Crushed Room Parallel','Fast heavy room compression blended for excitement','room,parallel',45,0,350,-2,7000,1,-42,12,.2,110,.3,10,.65,1.12,75,0),
    @('Transient Recovery','Restores attack after over-compressed samples','repair,transient',30,0,280,-1,9500,.5,-18,2,35,100,.65,2,.25,1.02,90,0),
    @('Mono Club Safe','Mono-compatible low end and conservative stereo width','club,mono-safe',28,0,260,-1.5,9000,.5,-23,3,24,100,.88,4,.35,.96,75,0),
    @('Drum Bus Safety','Minimal transparent finish with headroom','utility,clean',25,0,320,-1,10000,.3,-18,2,30,140,.7,2,.22,1,90,0)
)
foreach ($p in $bus) {
    $eq = @((Eq LowCut $p[3] 0 .707 18),(Eq LowShelf 75 $p[4] .7),(Eq Bell $p[5] $p[6] .9),(Eq HighShelf $p[7] $p[8] .7))
    $fx = @((Compressor $p[9] $p[10] $p[11] $p[12] $p[13] 1 1 0 6),(Saturator $p[14] $p[15] $(if($p[0] -in @('Tape Cohesion','Lo-Fi Drum Print')){2}else{3}) .03 .48 .08 0 0),(Stereo $p[16] .08 .08 $p[17] .85 0))
    if ($p[0] -eq 'Transient Recovery') { $fx = @((Transient 5 -1 .75 .62 0 1 1 1)) + $fx }
    if ($p[0] -eq 'Indie Room Lift') { $fx += Reverb .1 4 22 1.3 120 9500 1.15 }
    if ($p[0] -in @('Techno Clamp','Metal Drum Armor','Pop Drum Finish','Drum Bus Safety')) { $fx += Limiter -1.5 -1 70 .75 0 }
    $presets.Add((Preset DrumBus $p[0] $p[1] ($p[2] -split ',') $eq $fx -1))
}

$bass = @(
    @('Sub Mono Foundation','Clean mono sub with restrained harmonics','sub,mono',24,48,2,210,-2,900,0,-26,3,18,150,1,4,.42,5,120,0),
    @('808 Translation','Adds audible harmonics while protecting the deepest octave','808,trap',22,52,2,180,-2.5,950,2,-28,4,12,130,1,9,.62,5,140,0),
    @('Reese Controlled','Controls low-mid density and keeps the sub centered','reese,dnb',28,70,1,260,-3,1700,2,-25,4,8,90,.95,7,.55,4,150,0),
    @('DnB Mid Bite','Fast consistent bass with assertive midrange harmonics','dnb,aggressive',30,75,1,320,-2.5,2200,3,-23,5,6,65,1,8,.65,4,130,0),
    @('House Round Bass','Smooth low-end body and gentle note leveling','house,warm',28,82,2,280,-2,1200,1,-27,3,22,170,.9,5,.45,1,140,0),
    @('Techno Rumble Bass','Dense dark bass with controlled upper resonance','techno,dark',25,58,2,240,-2.5,1500,-1,-24,4,10,120,1,8,.55,2,150,0),
    @('Funk Finger Bass','Preserves pluck and brings forward note definition','funk,electric',35,95,1,280,-2,1200,2,-24,3.5,28,110,.9,4,.42,3,110,4),
    @('Pick Bass Attack','Focused string attack for rock arrangements','rock,pick',38,100,1,320,-2.5,2200,3,-23,4,24,95,.95,5,.46,3,110,5),
    @('Synthwave Bass','Firm retro synth body with warm saturation','synthwave,retro',28,70,2,350,-2,1400,2,-25,4,15,145,.95,7,.58,2,130,0),
    @('Acid Line Focus','Controls resonant movement without flattening it','acid,303',38,90,0,420,-2,1800,3,-22,3,8,85,.9,6,.52,4,100,2),
    @('Indie Bass Warmth','Natural leveling and mild tape colour','indie,warm',32,85,1.5,300,-1.5,900,1,-27,3,30,190,.85,5,.45,1,120,3),
    @('Metal Bass Grind','Centered low end with aggressive upper harmonics','metal,grind',40,90,0,250,-3,1600,4,-22,5,12,80,1,10,.72,4,130,5),
    @('Lo-Fi Bass Print','Rounded bandwidth and audible vintage texture','lofi,tape',35,95,1,280,-1,750,1,-28,3,8,180,.85,9,.7,2,120,1),
    @('Pop Bass Consistency','Even notes, clear translation and mono-safe lows','pop,polished',30,75,1.5,260,-2.5,1300,2,-26,4,18,130,1,5,.48,3,130,2),
    @('Bass Rescue Neutral','Conservative rumble removal, mud control and leveling','utility,clean',28,80,1,280,-2,1000,1,-24,3,22,150,.9,3,.3,0,120,1)
)
foreach ($p in $bass) {
    $eq = @((Eq LowCut $p[3] 0 .707 24),(Eq LowShelf $p[4] $p[5] .75),(Eq Bell $p[6] $p[7] 1.05),(Eq Bell $p[8] $p[9] .9),(Eq HighCut 12500 0 .707 12))
    $fx = @((Compressor $p[10] $p[11] $p[12] $p[13] $p[14] 1 1 1 6),(Saturator $p[15] $p[16] $p[17] .02 .62 .12 0 0),(Stereo 1 0 0 $p[18] 1 0))
    if ($p[19] -gt 0) { $fx = @((Transient $p[19] 0 .65 .5 0 1 1 1)) + $fx }
    if ($p[0] -eq 'Bass Rescue Neutral') { $fx += Limiter -1.5 -1 100 .7 0 }
    $presets.Add((Preset Bass $p[0] $p[1] ($p[2] -split ',') $eq $fx -1))
}

if ($presets.Count -ne 100) { throw "Expected 100 presets, got $($presets.Count)" }

function Escape-Xml([string]$Text) { [System.Security.SecurityElement]::Escape($Text) }
function Number($Value) { [Convert]::ToString($Value, [Globalization.CultureInfo]::InvariantCulture) }

function New-PresetXml($Preset) {
    $name = Escape-Xml $Preset.name
    $rootFlags = if ($Preset.lenin -or $Preset.stalin) { " lenin=`"$(if($Preset.lenin){1}else{0})`" stalin=`"$(if($Preset.stalin){1}else{0})`"" } else { '' }
    $sb = [Text.StringBuilder]::new()
    [void]$sb.Append('<?xml version="1.0" encoding="UTF-8"?>')
    [void]$sb.Append("<HPEQ schemaVersion=`"1`"><Parameters><PARAM id=`"bypass`" value=`"0.0`"/><PARAM id=`"outputGain`" value=`"$(Number $Preset.outputGainDb)`"/></Parameters>")
    [void]$sb.Append("<HpeqProject schemaVersion=`"1`" routing=`"Parallel`"$rootFlags selectedLayer=`"0`" nextNodeId=`"$($Preset.eq.Count + 1)`">")
    [void]$sb.Append("<Layer index=`"0`" name=`"$name`" enabled=`"1`" mute=`"0`" solo=`"0`" gainDb=`"0`" mix=`"1`" pan=`"0`" width=`"1`" channel=`"Stereo`">")
    $id = 1
    foreach ($band in $Preset.eq) {
        [void]$sb.Append("<EqNode id=`"$id`" enabled=`"1`" shape=`"$($band.shape)`" frequencyHz=`"$(Number $band.frequencyHz)`" gainDb=`"$(Number $band.gainDb)`" q=`"$(Number $band.q)`" slopeDbPerOct=`"$($band.slopeDbPerOct)`" channel=`"Stereo`"/>")
        $id++
    }
    [void]$sb.Append('<MixChain>')
    foreach ($slot in $Preset.fx) {
        [void]$sb.Append("<FxSlot type=`"$($slot.type)`" enabled=`"1`" mix=`"$(Number $slot.mix)`"><Params")
        foreach ($key in ($slot.params.Keys | Sort-Object)) {
            [void]$sb.Append(" $key=`"$(Number $slot.params[$key])`"")
        }
        [void]$sb.Append('/></FxSlot>')
    }
    [void]$sb.Append('</MixChain></Layer>')
    for ($i = 1; $i -lt 10; $i++) {
        [void]$sb.Append("<Layer index=`"$i`" name=`"Layer $($i + 1)`" enabled=`"1`" mute=`"0`" solo=`"0`" gainDb=`"0`" mix=`"1`" pan=`"0`" width=`"1`" channel=`"Stereo`"/>")
    }
    [void]$sb.Append('</HpeqProject></HPEQ>')
    $sb.ToString()
}

function Slug([string]$Text) {
    (($Text.ToLowerInvariant() -replace '[^a-z0-9]+','-').Trim('-'))
}

$output = [IO.Path]::GetFullPath($OutputDirectory)
$projectRoot = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
$allowedRoot = [IO.Path]::GetFullPath((Join-Path $projectRoot 'Assets\Presets'))
$allowedPrefix = $allowedRoot.TrimEnd([IO.Path]::DirectorySeparatorChar) + [IO.Path]::DirectorySeparatorChar
if (-not $output.StartsWith($allowedPrefix, [StringComparison]::OrdinalIgnoreCase)) {
    throw "Refusing to replace a directory outside $allowedRoot"
}
if (Test-Path -LiteralPath $output) { Remove-Item -LiteralPath $output -Recurse -Force }
[void](New-Item -ItemType Directory -Path $output)
$utf8 = [Text.UTF8Encoding]::new($false)
$manifestItems = @()
$index = 1
foreach ($preset in $presets) {
    $categoryDir = Join-Path $output $preset.category
    if (-not (Test-Path -LiteralPath $categoryDir)) { [void](New-Item -ItemType Directory -Path $categoryDir) }
    $fileName = '{0:D3}-{1}.hpeq' -f $index,(Slug $preset.name)
    $relative = "$($preset.category)/$fileName"
    $path = Join-Path $categoryDir $fileName
    $xml = New-PresetXml $preset
    [xml]$parsed = $xml
    if ($parsed.HPEQ.HpeqProject.Layer.Count -ne 10) { throw "Invalid layer count: $relative" }
    if ($preset.fx.Count -gt 6) { throw "Mix chain over limit: $relative" }
    $xmlBytes = $utf8.GetBytes($xml)
    $bytes = [byte[]]::new(8 + $xmlBytes.Length + 1)
    [BitConverter]::GetBytes([uint32]0x21324356).CopyTo($bytes, 0)
    [BitConverter]::GetBytes([uint32]$xmlBytes.Length).CopyTo($bytes, 4)
    $xmlBytes.CopyTo($bytes, 8)
    $bytes[$bytes.Length - 1] = 0
    [IO.File]::WriteAllBytes($path, $bytes)
    $manifestItems += [ordered]@{ id=('factory-{0:D3}' -f $index); name=$preset.name; category=$preset.category;
        file=$relative; intent=$preset.intent; tags=$preset.tags; outputGainDb=$preset.outputGainDb }
    $index++
}

$manifest = [ordered]@{
    format='HPEQ-JUCE-State'; formatVersion=1; pluginStateSchema=1; presetCount=$manifestItems.Count
    categories=[ordered]@{ Kick=18; Snare=17; HiHat=15; Percussion=15; DrumBus=20; Bass=15 }
    loading='Each .hpeq file is a JUCE copyXmlToBinary state block and can be passed directly to FxPluginProcessor::setStateInformation.'
    designNotes=@(
        'Thresholds are input-level dependent; trim the source before changing the musical timing.',
        'Broad boosts are modest, resonant cuts are narrow, and every preset leaves output headroom.',
        'Drum-bus presets use restrained serial compression except presets explicitly tagged parallel or aggressive.',
        'Bass presets center the low end with monoBassHz while allowing upper harmonics to remain stereo.'
    )
    references=@(
        'https://www.soundonsound.com/techniques/drum-compression',
        'https://www.soundonsound.com/techniques/mix-tips-kick-bass',
        'https://www.soundonsound.com/techniques/secrets-mix-engineers-dylan-3d-dresdow',
        'https://www.izotope.com/en/learn/drum-compression',
        'https://www.izotope.com/community/blog/drum-bus'
    )
    presets=$manifestItems
}
[IO.File]::WriteAllText((Join-Path $output 'manifest.json'), ($manifest | ConvertTo-Json -Depth 8), $utf8)

$readme = @'
# HPEQ Factory Presets — Drums & Bass

100 full-state factory presets for HPEQ schema v1. Each `.hpeq` file is the exact binary block produced by
JUCE `AudioProcessor::copyXmlToBinary`; its bytes can be passed directly to
`FxPluginProcessor::setStateInformation`.

The pack is organized as Kick (18), Snare (17), HiHat (15), Percussion (15), DrumBus (20), and Bass (15).
`manifest.json` is the browser catalog and contains stable IDs, display names, tags, intent, relative paths,
and output trim. Presets are starting points: compressor and gate thresholds necessarily depend on source level.

Signal order inside every preset is PRE-EQ followed by the layer MIX chain. The designs favor complementary EQ,
envelope-aware attack/release, moderate saturation, mono-safe low end, and explicit headroom. Presets with strong
colour are named/tagged accordingly.

Design references: Sound On Sound's drum-compression and kick/bass technique articles, its Dylan “3-D”
Dresdow interview, and iZotope's drum-compression and drum-bus guides. Their advice was translated to HPEQ's
actual DSP ranges rather than copied as brand-specific settings.
'@
[IO.File]::WriteAllText((Join-Path $output 'README.md'), $readme, $utf8)

Write-Host "Generated $($presets.Count) presets in $output"
