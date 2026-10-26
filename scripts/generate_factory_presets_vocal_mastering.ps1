[CmdletBinding()]
param(
    [string] $OutputDirectory = (Join-Path $PSScriptRoot '..\Assets\Presets\Factory\Vocals-Mastering-and-More')
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

function Eq([string]$Shape,[double]$Hz,[double]$Gain=0,[double]$Q=.707,[int]$Slope=12) {
    [ordered]@{ shape=$Shape; frequencyHz=$Hz; gainDb=$Gain; q=$Q; slopeDbPerOct=$Slope; dynamic=$false }
}
function DynEq([string]$Shape,[double]$Hz,[double]$Gain,[double]$Q,[double]$Threshold,[double]$Ratio,
               [double]$Attack,[double]$Release,[double]$Range) {
    [ordered]@{ shape=$Shape; frequencyHz=$Hz; gainDb=$Gain; q=$Q; slopeDbPerOct=12; dynamic=$true;
        thresholdDb=$Threshold; ratio=$Ratio; attackMs=$Attack; releaseMs=$Release; rangeDb=$Range }
}
function Fx([string]$Type,[double]$Mix=1,[hashtable]$Params=@{}) { [ordered]@{type=$Type;mix=$Mix;params=$Params} }
function Comp([double]$Threshold=-24,[double]$Ratio=3,[double]$Attack=12,[double]$Release=90,
              [double]$Mix=1,[int]$Character=0,[int]$Detector=1,[int]$Auto=1,[double]$Knee=8) {
    Fx Compressor $Mix @{thresholdDb=$Threshold;ratio=$Ratio;kneeDb=$Knee;attackMs=$Attack;releaseMs=$Release;
        makeupDb=0;detector=$Detector;character=$Character;autoMakeup=$Auto}
}
function Sat([double]$Drive=4,[double]$Mix=.5,[int]$Model=1,[double]$Tone=0,[double]$Harm=.5,
             [double]$EvenOdd=0,[int]$Thump=0,[double]$Trim=0) {
    Fx Saturator $Mix @{driveDb=$Drive;curve=0;bias=0;trimDb=$Trim;oversampling=2;autoGain=1;model=$Model;
        punish=0;tone=$Tone;satLowCutHz=20;satHighCutHz=20000;thump=$Thump;steep=0;harmonics=$Harm;
        evenOdd=$EvenOdd;tapeBias=.5;speed=1;wow=.02;hiss=0;hum=0;crackle=0;noiseFollow=1}
}
function Verb([double]$Mix=.12,[int]$Space=2,[double]$Pre=28,[double]$Decay=1.4,[double]$LowCut=180,
              [double]$HighCut=11000,[double]$Width=1.1,[double]$Duck=.45) {
    Fx Reverb $Mix @{space=$Space;color=2;predelayMs=$Pre;predelaySync=0;decaySeconds=$Decay;size=.9;
        hfDecayRatio=.48;lowCutHz=$LowCut;highCutHz=$HighCut;width=$Width;attack=.2;earlyDiffusion=.68;
        lateDiffusion=.74;modRate=.55;modDepth=.25;bassMult=1;bassXoverHz=300;highXoverHz=4000;
        distance=.3;duck=$Duck;freeze=0}
}
function Delay([double]$Mix=.1,[int]$Sync=8,[double]$Feedback=.22,[int]$Character=2,[double]$Width=1,
               [double]$LowCut=180,[double]$Damping=7500,[double]$Duck=.55) {
    Fx Delay $Mix @{timeMs=250;sync=$Sync;feedback=$Feedback;pingPong=0;dampingHz=$Damping;lowCutHz=$LowCut;
        width=$Width;character=$Character;timeRMs=375;syncR=6;link=1;cross=0;driveDb=2;modRate=.45;
        modDepth=.12;diffusion=.12;pitch=0;lowRes=0;highRes=0;bandHz=1800;bandGainDb=0;bandQ=1;
        duck=$Duck;freeze=0;lofi=0;timeMode=0}
}
function Stereo([double]$Width=1,[double]$Stereoize=0,[double]$Space=0,[double]$MonoBass=0,
                [double]$Mix=1,[int]$Style=0) {
    Fx StereoProcessor $Mix @{width=$Width;balance=0;style=$Style;stereoize=$Stereoize;space=$Space;
        monoBassHz=$MonoBass;rotation=0;safe=1;monoListen=0;outputDb=0}
}
function Limit([double]$Threshold=-1,[double]$Ceiling=-1,[double]$Release=100,[double]$Mix=1,
               [int]$Style=0,[double]$Link=1) {
    Fx Limiter $Mix @{thresholdDb=$Threshold;ceilingDb=$Ceiling;releaseMs=$Release;lookaheadMs=3;
        truePeak=1;autoRelease=1;style=$Style;link=$Link;monitor=0;outputDb=0}
}
function Transient([double]$Attack=2,[double]$Sustain=0,[double]$Mix=.7) {
    Fx TransientShaper $Mix @{attackDb=$Attack;sustainDb=$Sustain;speed=.55;style=0;lookahead=1;
        rise=1;decay=1;sensitivity=.85;preserve=1;listen=0;outputDb=0}
}
function Preset([string]$Category,[string]$Name,[string]$Intent,[string[]]$Tags,[object[]]$Eq,[object[]]$Fx,
                [double]$Output=0,[bool]$Lenin=$false,[bool]$Stalin=$false) {
    [ordered]@{category=$Category;name=$Name;intent=$Intent;tags=$Tags;eq=$Eq;fx=$Fx;
        outputGainDb=$Output;lenin=$Lenin;stalin=$Stalin}
}

$presets=[Collections.Generic.List[object]]::new()

# Lead vocal profiles: HPF, low-mid cleanup, presence/air and a source-appropriate dynamic sibilance band.
$lead=@(
 @('Pop Lead Polish','Controlled, bright and centered modern pop lead','pop,polished',85,280,-2,3200,2,12000,2,6800,-3,-26,3.5,12,85,1,4,.35,1,.10,.08),
 @('Rap Lead Forward','Dense mid-forward vocal that remains intelligible over drums','rap,forward',75,240,-2.5,2200,2.5,10500,1.5,6200,-2.5,-28,4,8,65,1,6,.42,4,.06,.10),
 @('Trap Air Lead','Bright airy lead with controlled sibilance and a short echo','trap,airy',90,320,-2,3800,2,14500,3,7600,-4,-27,4,10,75,1,5,.38,4,.08,.14),
 @('RNB Velvet','Smooth intimate vocal with soft upper mids','rnb,smooth',70,350,-2,2500,1,11500,1.5,6500,-2.5,-29,3,18,125,.9,4,.32,1,.13,.08),
 @('Rock Vocal Grip','Assertive vocal density without losing consonants','rock,aggressive',85,420,-2.5,2800,3,10000,1,5900,-2,-24,5,18,90,1,6,.45,3,.08,.05),
 @('Indie Intimate','Close, natural vocal with restrained brightness','indie,natural',75,260,-1.5,2100,1,11000,.5,7000,-2,-30,2.5,20,150,.85,3,.25,1,.12,.04),
 @('EDM Lead Lift','Bright firm lead designed for dense synth arrangements','edm,bright',95,300,-2.5,4200,2.5,13500,2.5,7200,-3.5,-26,4,10,70,1,5,.4,4,.10,.12),
 @('Ballad Open Vocal','Gentle leveling with an open natural top','ballad,dynamic',70,280,-1,2700,1,12500,1,7200,-2,-31,2,25,180,.8,3,.22,1,.16,.03),
 @('Low Male Clarity','Reduces chest buildup while preserving authority','male,low',65,190,-2.5,1800,2,9500,1,5600,-2,-27,3.5,16,120,.95,4,.32,1,.08,.06),
 @('High Vocal Soft Air','Smooths upper-mid intensity before adding air','female,high',105,420,-2,3400,1.5,14000,2,7800,-4,-29,3,14,105,.9,3,.28,1,.12,.05),
 @('Parallel Vocal Weight','Heavy compression blended beneath the original','parallel,dense',80,300,-2,2500,2,11000,1.5,6500,-3,-38,10,2,70,.34,7,.5,3,.07,.08),
 @('Vintage Tape Vocal','Warm rounded vocal colour with softened air','vintage,tape',75,250,-1.5,2200,1.5,10500,-.5,6200,-2,-27,3,12,140,.9,7,.58,2,.10,.05),
 @('Podcast Studio Voice','Dry, even speech with plosive and sibilance control','speech,podcast',80,220,-2,2500,2,10500,.5,6500,-3,-30,3.5,8,100,1,3,.22,1,0,0),
 @('Vocal Rescue Boxy','Stronger low-mid cleanup for enclosed recordings','repair,boxy',90,480,-4,3000,1.5,11000,1,6900,-3,-28,3,12,110,.9,3,.24,0,.06,.03),
 @('Vocal Rescue Harsh','Dynamic upper-mid control with conservative air','repair,harsh',85,320,-1.5,2800,1,12000,.5,4100,-4,-30,2.5,18,130,.85,2,.2,0,.08,.02)
)
foreach($p in $lead){
 $eq=@((Eq LowCut $p[3] 0 .707 18),(Eq Bell $p[4] $p[5] 1.1),(Eq Bell $p[6] $p[7] .9),(Eq HighShelf $p[8] $p[9] .7),(DynEq Bell $p[10] 0 2.2 -30 4 .6 45 $p[11]))
 $fx=@((Comp $p[12] $p[13] $p[14] $p[15] $p[16] 1 1 1 8),(Sat $p[17] $p[18] $p[19] .08 .48 .12 0 0))
 if($p[20]-gt 0){$fx+=Verb $p[20] 2 28 1.25 180 11500 1.05 .5}
 if($p[21]-gt 0){$fx+=Delay $p[21] 8 .2 2 1 180 7200 .6}
 $presets.Add((Preset LeadVocal $p[0] $p[1] ($p[2]-split ',') $eq $fx -1))
}

$back=@(
 @('Pop Stack Glue','Polished wide backing-vocal stack behind the lead','pop,stack',150,350,-2.5,2800,1,10500,1,7000,-4,-30,4,8,90,1.22,.18,.14),
 @('Dark Behind Lead','Reduced presence and air to maintain depth','dark,support',180,420,-2,3200,-2,9500,-1,6500,-3,-29,3,10,120,1.12,.10,.18),
 @('Airy Harmony Bed','Wide floating harmonies with controlled esses','airy,harmony',160,300,-2,3600,.5,13500,2.5,7600,-5,-31,3.5,12,115,1.35,.20,.18),
 @('Rap Double Tight','Compact center-supporting rap double','rap,double',120,280,-2.5,1800,2,9000,0,6000,-3,-27,5,5,70,1.04,.06,.08),
 @('Trap Adlib Wide','Wide bright ad-lib with tempo echo','trap,adlib',160,380,-2,4200,2,13000,2,7500,-4,-28,4,6,80,1.38,.10,.12),
 @('RNB Choir Silk','Smooth lush stereo backing choir','rnb,choir',140,420,-2,2600,.5,12000,1.5,6800,-4,-31,3,15,140,1.32,.18,.20),
 @('Rock Gang Shout','Dense controlled group shout with mild ambience','rock,gang',110,350,-2,2100,2.5,9500,.5,5600,-2.5,-25,6,12,85,1.12,.14,.08),
 @('Indie Ghost Layer','Soft distant double that adds texture, not focus','indie,texture',190,500,-2.5,3000,-1,10500,0,7200,-3,-33,3,8,150,1.28,.22,.16),
 @('Backing Vocal DeEss','Strong shared sibilance control for stacked parts','utility,deess',170,320,-2,3000,0,11000,.5,7200,-6,-32,4,6,65,1.16,.06,.08),
 @('Mono Support Double','Narrow backing layer for a stable vocal center','utility,mono',130,300,-2,2200,1,10000,.5,6500,-3,-29,4,8,95,.82,.05,.06)
)
foreach($p in $back){
 $eq=@((Eq LowCut $p[3] 0 .707 24),(Eq Bell $p[4] $p[5] 1.1),(Eq Bell $p[6] $p[7] .9),(Eq HighShelf $p[8] $p[9] .7),(DynEq Bell $p[10] 0 2.4 -32 5 .4 35 $p[11]))
 $fx=@((Comp $p[12] $p[13] $p[14] $p[15] 1 1 1 1 8),(Sat 3 .28 1 .05 .42 .1 0 0),(Stereo $p[16] .22 .18 0 .9 0))
 if($p[17]-gt 0){$fx+=Verb $p[17] 3 34 1.6 240 10500 1.2 .5}
 if($p[18]-gt 0){$fx+=Delay $p[18] 7 .24 2 1.2 220 6800 .55}
 $presets.Add((Preset BackingVocal $p[0] $p[1] ($p[2]-split ',') $eq $fx -1.5))
}

# Mastering: five intensity/colour choices for six broad release contexts. Thresholds are intentionally
# conservative; the source must be trimmed into the preset rather than chasing a label with blind gain.
$genres=@(
 [ordered]@{folder='Pop';tag='pop';low=75;lowGain=.5;mud=300;mudGain=-.6;air=12000;airGain=.7;ct=-14;cr=1.7;ca=35;crel=150;drive=2.2;model=1;width=1.04;mono=110},
 [ordered]@{folder='Rap';tag='rap';low=65;lowGain=.7;mud=250;mudGain=-.7;air=9500;airGain=.4;ct=-15;cr=1.8;ca=28;crel=125;drive=2.8;model=3;width=1.02;mono=125},
 [ordered]@{folder='Trap';tag='trap';low=52;lowGain=.5;mud=210;mudGain=-.8;air=12500;airGain=.6;ct=-15;cr=1.9;ca=24;crel=110;drive=3.2;model=4;width=1.05;mono=140},
 [ordered]@{folder='Techno';tag='techno';low=58;lowGain=.5;mud=230;mudGain=-.6;air=10500;airGain=.3;ct=-14;cr=1.8;ca=20;crel=95;drive=3.0;model=3;width=1.02;mono=150},
 [ordered]@{folder='House-EDM';tag='house,edm';low=62;lowGain=.4;mud=280;mudGain=-.6;air=11500;airGain=.7;ct=-14;cr=1.7;ca=28;crel=115;drive=2.6;model=1;width=1.06;mono=135},
 [ordered]@{folder='Rock-Indie';tag='rock,indie';low=85;lowGain=.3;mud=350;mudGain=-.5;air=10000;airGain=.4;ct=-13;cr=1.6;ca=40;crel=170;drive=2.5;model=2;width=1.02;mono=95}
)
$masterVariants=@(
 [ordered]@{name='Transparent Finish';intent='Minimal tonal correction and transparent peak control';tag='transparent';eq=.6;comp=.72;drive=.65;width=0;lim=-1.2;rel=130;style=0},
 [ordered]@{name='Streaming Balanced';intent='Balanced translation with true-peak headroom';tag='streaming';eq=1;comp=.82;drive=.8;width=.01;lim=-2.2;rel=105;style=0},
 [ordered]@{name='Loud Controlled';intent='Denser level with protected stereo image and low-end center';tag='loud';eq=.9;comp=.95;drive=1.15;width=0;lim=-4.2;rel=75;style=1},
 [ordered]@{name='Warm Cohesion';intent='Rounder transients and gentle analog-style density';tag='warm';eq=.8;comp=.85;drive=1.25;width=-.01;lim=-2.6;rel=125;style=2},
 [ordered]@{name='Open Dynamic';intent='Preserves movement while adding a small amount of polish';tag='dynamic';eq=1.1;comp=.58;drive=.5;width=.02;lim=-1.5;rel=150;style=0}
)
foreach($g in $genres){foreach($v in $masterVariants){
 $category="Mastering/$($g.folder)";$name="$($g.folder) — $($v.name)"
 $eq=@((Eq LowCut 24 0 .707 12),(Eq LowShelf $g.low ($g.lowGain*$v.eq) .7),(DynEq Bell $g.mud 0 1.0 -24 1.8 25 180 ($g.mudGain*$v.eq)),(Eq HighShelf $g.air ($g.airGain*$v.eq) .7))
 $fx=@((Comp $g.ct $g.cr $g.ca $g.crel $v.comp 0 1 0 12),(Sat ($g.drive*$v.drive) .55 $g.model .02 .4 .05 0 0),(Stereo ($g.width+$v.width) 0 0 $g.mono 1 0),(Limit $v.lim -1 $v.rel 1 $v.style 1))
 $tags=@($g.tag-split ',')+$v.tag+@('mastering')
 $presets.Add((Preset $category $name "$($v.intent); tuned for $($g.folder)." $tags $eq $fx 0 ($v.tag-eq 'warm') $false))
}}

$guitar=@(
 @('Acoustic Guitar Natural','Natural cleanup and controlled pick peaks','acoustic,natural',70,220,-2,2500,1,10500,1,-27,3,18,130,3,.28,.08),
 @('Acoustic Guitar Bright','Airy strumming with dynamic harshness control','acoustic,bright',80,300,-2,3800,2,12500,2,-28,3,16,115,3,.24,.10),
 @('Electric Clean Chime','Clear clean guitar with gentle stereo space','electric,clean',75,260,-2,2200,1.5,9000,1,-26,2.5,22,150,3,.25,.12),
 @('Rock Rhythm Tight','Dense rhythm guitar with low-mid cleanup','rock,rhythm',85,320,-3,1800,2,8500,0,-24,4,12,90,5,.42,.04),
 @('Metal Rhythm Wall','Tight high-gain guitar with fizz control','metal,heavy',90,250,-3.5,1500,2,7200,-1.5,-22,5,8,70,7,.48,.02),
 @('Lead Guitar Singing','Sustaining forward lead with short ambience','lead,sustain',90,350,-2,2400,3,9500,1,-26,4,14,140,6,.4,.13),
 @('Funk Guitar Snap','Fast clean articulation and reduced low mids','funk,clean',100,280,-3,3200,2,10500,1,-25,3,24,75,4,.28,.05),
 @('Lo-Fi Guitar Print','Dark tape-coloured guitar texture','lofi,tape',90,300,-2,1700,1,7000,-2,-28,3,8,170,8,.62,.08)
)
foreach($p in $guitar){$eq=@((Eq LowCut $p[3] 0 .707 18),(Eq Bell $p[4] $p[5] 1.1),(Eq Bell $p[6] $p[7] 1),(Eq HighShelf $p[8] $p[9] .7),(DynEq Bell 4200 0 1.6 -28 2.5 4 80 -2));$fx=@((Comp $p[10] $p[11] $p[12] $p[13] .9 1 1 1 7),(Sat $p[14] $p[15] $(if($p[0]-eq 'Lo-Fi Guitar Print'){2}else{3}) .02 .5 .08 0 0));if($p[16]-gt 0){$fx+=Verb $p[16] 2 20 1.2 160 9500 1.1 .35};$presets.Add((Preset Guitar $p[0] $p[1] ($p[2]-split ',') $eq $fx -1))}

$synth=@(
 @('Analog Pad Width','Wide warm pad with mono-safe low end','pad,analog',35,250,-1.5,1800,1,9000,.5,4,.42,1.3,.16),
 @('Digital Pad Smooth','Controls glassy peaks and adds soft depth','pad,digital',45,400,-2,3200,-1,12000,1,3,.3,1.35,.18),
 @('Pluck Forward','Fast defined pluck with compact tail','pluck,transient',70,280,-2,2500,2,10500,1,5,.38,1.15,.08),
 @('Arp Controlled','Even rhythmic arpeggio with restrained width','arp,rhythmic',60,350,-2,2200,1,10000,.5,4,.34,1.18,.06),
 @('Lead Synth Focus','Centered synth lead with presence and peak control','lead,mono-safe',80,300,-2,1800,2.5,9000,1,6,.42,1.05,.08),
 @('Supersaw Tamer','Reduces upper-mid glare while retaining size','supersaw,edm',90,450,-2,2800,-2,11500,1,5,.38,1.22,.10),
 @('Acid Resonance Control','Dynamic control for moving resonant lines','acid,resonant',60,280,-1.5,1200,1,8500,0,6,.45,1.05,.05),
 @('Lo-Fi Synth Print','Narrow vintage synth texture with tape colour','lofi,tape',55,320,-1,1600,1,7000,-2,9,.68,1.12,.08)
)
foreach($p in $synth){$eq=@((Eq LowCut $p[3] 0 .707 18),(Eq Bell $p[4] $p[5] 1),(DynEq Bell $p[6] $p[7] 1.3 -25 2.5 8 100 -2.5),(Eq HighShelf $p[8] $p[9] .7));$fx=@((Comp -24 3 14 105 .82 1 1 1 7),(Sat $p[10] $p[11] $(if($p[0]-eq 'Lo-Fi Synth Print'){2}else{4}) .05 .55 .1 0 0),(Stereo $p[12] .2 .2 120 .9 0));if($p[13]-gt 0){$fx+=Verb $p[13] 3 25 1.7 180 11000 1.2 .4};$presets.Add((Preset Synth $p[0] $p[1] ($p[2]-split ',') $eq $fx -1))}

$keys=@(
 @('Grand Piano Natural','Transparent piano cleanup and peak control','piano,natural',30,220,-1.5,2500,1,11000,.5,2,.2,1.05),
 @('Pop Piano Bright','Forward piano for dense pop arrangements','piano,pop',45,300,-2,3200,2,12000,1.5,4,.3,1.12),
 @('Felt Piano Intimate','Soft close piano with warm ambience','piano,felt',35,260,-1,1800,.5,9000,-1,3,.35,1.08),
 @('Rhodes Velvet','Warm electric piano with smooth upper mids','rhodes,warm',45,350,-2,1400,2,8500,.5,5,.48,1.16),
 @('Wurlitzer Bite','Controlled bark and vintage harmonic density','wurli,vintage',55,280,-2,2200,2.5,9000,0,6,.5,1.08),
 @('Keys Bus Glue','Cohesive keyboard subgroup with safe width','keys,bus',35,300,-1.5,1800,1,10500,.5,3,.28,1.1)
)
foreach($p in $keys){$eq=@((Eq LowCut $p[3] 0 .707 12),(Eq Bell $p[4] $p[5] 1),(Eq Bell $p[6] $p[7] .9),(Eq HighShelf $p[8] $p[9] .7));$fx=@((Comp -25 2.5 22 150 .82 0 1 1 9),(Sat $p[10] $p[11] 1 .02 .45 .08 0 0),(Stereo $p[12] .08 .12 90 .9 0));if($p[0]-in @('Felt Piano Intimate','Rhodes Velvet')){$fx+=Verb .11 3 24 1.5 120 9500 1.15 .35};$presets.Add((Preset Keys $p[0] $p[1] ($p[2]-split ',') $eq $fx -1))}

$mixbus=@(
 @('Mix Bus Transparent','Very light cohesion and true-peak safety','transparent,clean',-.4,.4,-14,1.5,35,180,1.8,.28,1.01,-1.2),
 @('Mix Bus Modern Pop','Polished top and controlled contemporary density','pop,polished',-.6,.7,-15,1.8,30,140,2.5,.38,1.03,-2.2),
 @('Mix Bus Hip Hop','Centered low end with punch-preserving glue','hip-hop,punch',-.7,.4,-15,1.9,28,120,2.8,.42,1.01,-2.5),
 @('Mix Bus Electronic','Firm electronic mix cohesion and peak control','electronic,club',-.6,.5,-14,1.8,22,105,3,.44,1.03,-2.8),
 @('Mix Bus Analog Warm','Rounded tape-style colour and soft glue','warm,analog',-.3,.2,-14,1.6,38,190,3.2,.5,1.01,-1.8),
 @('Mix Bus Open Rock','Preserves live transients and stereo stability','rock,dynamic',-.5,.3,-13,1.5,45,180,2.4,.32,1.01,-1.5),
 @('Mix Bus Wide Safe','Small mono-compatible width increase','wide,mono-safe',-.5,.5,-14,1.6,35,160,2,.3,1.07,-1.5),
 @('Mix Bus PreMaster','Conservative headroom-oriented finishing chain','premaster,utility',-.5,.2,-12,1.4,45,220,1.5,.22,1,-.8)
)
foreach($p in $mixbus){$eq=@((Eq LowCut 24 0 .707 12),(DynEq Bell 280 0 1 -24 1.6 30 180 $p[3]),(Eq HighShelf 11000 $p[4] .7));$fx=@((Comp $p[5] $p[6] $p[7] $p[8] .78 0 1 0 12),(Sat $p[9] $p[10] $(if($p[0]-eq 'Mix Bus Analog Warm'){2}else{1}) .01 .38 .04 0 0),(Stereo $p[11] 0 0 100 1 0),(Limit $p[12] -1 130 .75 0 1));$presets.Add((Preset MixBus $p[0] $p[1] ($p[2]-split ',') $eq $fx 0 ($p[0]-eq 'Mix Bus Analog Warm') $false))}

if($presets.Count-ne 85){throw "Expected 85 presets, got $($presets.Count)"}

function EscapeXml([string]$s){[Security.SecurityElement]::Escape($s)}
function Num($v){[Convert]::ToString($v,[Globalization.CultureInfo]::InvariantCulture)}
function Slug([string]$s){(($s.ToLowerInvariant()-replace '[^a-z0-9]+','-').Trim('-'))}
function NewXml($preset){
 $sb=[Text.StringBuilder]::new();[void]$sb.Append('<?xml version="1.0" encoding="UTF-8"?>')
 [void]$sb.Append("<HPEQ schemaVersion=`"1`"><Parameters><PARAM id=`"bypass`" value=`"0.0`"/><PARAM id=`"outputGain`" value=`"$(Num $preset.outputGainDb)`"/></Parameters>")
 $flags='';if($preset.lenin){$flags+=' lenin="1"'};if($preset.stalin){$flags+=' stalin="1"'}
 [void]$sb.Append("<HpeqProject schemaVersion=`"1`" routing=`"Parallel`"$flags selectedLayer=`"0`" nextNodeId=`"$($preset.eq.Count+1)`">")
 [void]$sb.Append("<Layer index=`"0`" name=`"$(EscapeXml $preset.name)`" enabled=`"1`" mute=`"0`" solo=`"0`" gainDb=`"0`" mix=`"1`" pan=`"0`" width=`"1`" channel=`"Stereo`">")
 $id=1;foreach($b in $preset.eq){[void]$sb.Append("<EqNode id=`"$id`" enabled=`"1`" shape=`"$($b.shape)`" frequencyHz=`"$(Num $b.frequencyHz)`" gainDb=`"$(Num $b.gainDb)`" q=`"$(Num $b.q)`" slopeDbPerOct=`"$($b.slopeDbPerOct)`" channel=`"Stereo`"");if($b.dynamic){[void]$sb.Append(" dynamic=`"1`" dynThresholdDb=`"$(Num $b.thresholdDb)`" dynRatio=`"$(Num $b.ratio)`" dynAttackMs=`"$(Num $b.attackMs)`" dynReleaseMs=`"$(Num $b.releaseMs)`" dynRangeDb=`"$(Num $b.rangeDb)`"")};[void]$sb.Append('/>');$id++}
 [void]$sb.Append('<MixChain>');foreach($s in $preset.fx){[void]$sb.Append("<FxSlot type=`"$($s.type)`" enabled=`"1`" mix=`"$(Num $s.mix)`"><Params");foreach($k in ($s.params.Keys|Sort-Object)){[void]$sb.Append(" $k=`"$(Num $s.params[$k])`"")};[void]$sb.Append('/></FxSlot>')};[void]$sb.Append('</MixChain></Layer>')
 for($i=1;$i-lt 10;$i++){[void]$sb.Append("<Layer index=`"$i`" name=`"Layer $($i+1)`" enabled=`"1`" mute=`"0`" solo=`"0`" gainDb=`"0`" mix=`"1`" pan=`"0`" width=`"1`" channel=`"Stereo`"/>")};[void]$sb.Append('</HpeqProject></HPEQ>');$sb.ToString()
}

$out=[IO.Path]::GetFullPath($OutputDirectory);$project=[IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'));$allowed=[IO.Path]::GetFullPath((Join-Path $project 'Assets\Presets'));$prefix=$allowed.TrimEnd([IO.Path]::DirectorySeparatorChar)+[IO.Path]::DirectorySeparatorChar
if(-not $out.StartsWith($prefix,[StringComparison]::OrdinalIgnoreCase)){throw "Refusing to replace a directory outside $allowed"}
if(Test-Path -LiteralPath $out){Remove-Item -LiteralPath $out -Recurse -Force};[void](New-Item -ItemType Directory -Path $out)
$utf8=[Text.UTF8Encoding]::new($false);$items=@();$i=1
foreach($p in $presets){$dir=Join-Path $out $p.category;[void](New-Item -ItemType Directory -Force -Path $dir);$file=('{0:D3}-{1}.hpeq'-f $i,(Slug $p.name));$rel=("$($p.category)/$file");$xml=NewXml $p;[xml]$parsed=$xml;if(@($parsed.HPEQ.HpeqProject.Layer).Count-ne 10){throw "Invalid layers: $rel"};if($p.fx.Count-gt 6){throw "Chain over limit: $rel"};$xb=$utf8.GetBytes($xml);$bytes=[byte[]]::new(9+$xb.Length);[BitConverter]::GetBytes([uint32]0x21324356).CopyTo($bytes,0);[BitConverter]::GetBytes([uint32]$xb.Length).CopyTo($bytes,4);$xb.CopyTo($bytes,8);$bytes[-1]=0;[IO.File]::WriteAllBytes((Join-Path $dir $file),$bytes);$items+=[ordered]@{id=('factory-{0:D3}'-f (100+$i));name=$p.name;category=$p.category;file=$rel;intent=$p.intent;tags=$p.tags;outputGainDb=$p.outputGainDb};$i++}
$manifest=[ordered]@{format='HPEQ-JUCE-State';formatVersion=1;pluginStateSchema=1;presetCount=$items.Count;categories=[ordered]@{LeadVocal=15;BackingVocal=10;Mastering=30;Guitar=8;Synth=8;Keys=6;MixBus=8};masteringGenres=[ordered]@{Pop=5;Rap=5;Trap=5;Techno=5;'House-EDM'=5;'Rock-Indie'=5};loading='Each .hpeq file is a JUCE copyXmlToBinary state block and can be passed directly to FxPluginProcessor::setStateInformation.';designNotes=@('Vocal dynamic EQ bands are starting points for sibilance and resonance control; tune frequency and threshold to the singer.','Mastering presets deliberately avoid fixed LUFS claims because achieved loudness depends on the input mix.','Mastering chains keep the limiter last, low frequencies centered, and broadband compression subtle.','Backing-vocal presets use stronger filtering and de-essing than lead presets so stacks remain behind the lead.');references=@('https://www.soundonsound.com/techniques/vocal-production','https://www.soundonsound.com/techniques/techniques-vocal-de-essing','https://www.soundonsound.com/techniques/mixing-pop-vocals','https://www.izotope.com/community/blog/how-to-use-compression-in-mastering','https://www.izotope.com/community/blog/we-are-listening-your-mastering-questions-answered','https://www.izotope.com/en/learn/6-tips-for-widening-the-stereo-image-of-a-mix');presets=$items}
[IO.File]::WriteAllText((Join-Path $out 'manifest.json'),($manifest|ConvertTo-Json -Depth 8),$utf8)
$readme=@'
# HPEQ Factory Presets — Vocals, Mastering & More

85 full-state HPEQ schema-v1 presets: 15 lead vocals, 10 backing vocals, 30 mastering presets
(five each for Pop, Rap, Trap, Techno, House/EDM and Rock/Indie), plus Guitar, Synth, Keys and Mix Bus.

Each `.hpeq` is a JUCE `copyXmlToBinary` state block ready for `FxPluginProcessor::setStateInformation`.
`manifest.json` contains stable IDs, categories, tags, descriptions and relative paths.

These are musically targeted starting points, not automatic analysis. Vocal de-essing bands must be tuned to the
singer, and dynamics thresholds must be matched to the input level. Mastering presets intentionally make no fixed
LUFS promise; compare level-matched, watch gain reduction and preserve the mix's transient intent.
'@
[IO.File]::WriteAllText((Join-Path $out 'README.md'),$readme,$utf8)
Write-Host "Generated $($presets.Count) presets in $out"
