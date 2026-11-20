# HamloRack preset design overview

250 authored candidates. Design validation passed; native runtime, audio calibration and listening are pending. Full parameter settings and rejection checks: [catalog](PRESET_CATALOG.md). Integration and acceptance: [RACK0](RACK0_DESIGN.md).

## Boom Bap

| ID | Category | Name | Intended result | Ordered processing |
|---|---|---|---|---|
| HR001 | Kick | BB Classic Front Knock | Keep the first knock forward and remove cardboard behind it. | Udarnik > Mag |
| HR002 | Kick | BB Dusty Short Foot | Shorten a long dusty kick while retaining its low fundamental. | Zastava > Mag |
| HR003 | Kick | BB Jazzy Felt Weight | Round the top of a soft acoustic kick without blunting its low pulse. | Udarnik > Comp |
| HR004 | Kick | BB Gold Mid Knock | Add mid harmonics to a sub-heavy kick for small-speaker audibility. | Mag > Comp |
| HR005 | Kick | BB Underground Parallel Heel | Blend a crushed mid-band under the original attack. | Comp; 95-1800 Hz: Mag |
| HR006 | Kick | BB Lofi Cassette Pillow | Soften a brittle sampled kick with a slower dark tape response. | Mag > Udarnik |
| HR007 | Snare | BB Classic Wooden Crack | Recover stick attack above a short wooden body. | Udarnik > Mag |
| HR008 | Snare | BB Dusty Sampler Rim | Give a clean rim a restrained sampled edge and dark room. | Mag > Verb |
| HR009 | Snare | BB Jazzy Ghost Keeper | Control accents gently while leaving ghost notes readable. | Comp > Verb |
| HR010 | Snare | BB Gold Plate Halo | Put a short plate behind a dry forward backbeat. | Comp > Verb |
| HR011 | Snare | BB Basement Parallel Bark | Add a dirty parallel body with the clean crack intact. | Mag > Comp |
| HR012 | Snare | BB Lofi Brush Room | Make a long brush sample softer and more intimate. | Udarnik > Verb |
| HR013 | Hats | BB Silk Closed Hats | Remove abrasive upper peaks while keeping the timekeeping edge. | Mag |
| HR014 | Hats | BB Dusty Twelve Bit Hats | Add sampled grain above the kick without broadband noise. | Mag |
| HR015 | Hats | BB Jazzy Open Hat Air | Let open hats breathe in a small stereo room. | Verb > Shirota |
| HR016 | Hats | BB Swing Echo Tick | Add one quiet swung eighth response after the hat. | Echo |
| HR017 | Hats | BB Soft Offbeat Tuck | Reduce front-edge aggression and push hats behind the snare. | Udarnik > Fader |
| HR018 | Percussion | BB Conga Palm Body | Emphasize hand tone and trim hard slap peaks. | Udarnik > Mag |
| HR019 | Percussion | BB Shaker Back Wall | Place a sharp shaker behind the beat in a dark room. | Verb > Shirota |
| HR020 | Percussion | BB Rim Dub Answer | Answer a dry rim with a narrow filtered quarter delay. | Echo |
| HR021 | Percussion | BB Last Step Shuffle | Change only the final two sixteenths of a percussion bar. | Revolution > Mag |
| HR022 | Drum Bus | BB Classic Bus Cement | Join the drum body with slow-attack VCA reduction. | Comp > Mag |
| HR023 | Drum Bus | BB Dusty Parallel Basement | Layer dense sampler crunch under a clean drum loop. | Mag > Comp |
| HR024 | Drum Bus | BB Jazzy Room Glue | Give a sparse jazz break a low-density shared room. | Comp > Verb |
| HR025 | Drum Bus | BB Gold Snare Shoulder | Add body only around the snare register and leave the sub alone. | Comp; 160-2500 Hz: Udarnik |
| HR026 | Drum Bus | BB Underground Punch Return | Bring short gritty sustain under intact transients. | Comp > Mag |
| HR027 | Drum Bus | BB Lofi Worn Break | Darken a bright break and add restrained tape instability. | Mag > Udarnik |
| HR028 | Drum Bus | BB Bar End Turn | Keep the groove and make a restrained end-of-bar reply on the tops. | Comp; 1200-16000 Hz: Revolution |
| HR029 | Bass | BB Upright Round Fingers | Level an upright line while retaining finger articulation. | Comp > Mag |
| HR030 | Bass | BB Electric Pocket Bass | Tighten an electric bass into the kick pocket with modest harmonics. | Comp > Mag |
| HR031 | Bass | BB Sub On Small Speakers | Add upper harmonics while keeping the sub centered. | Mag > Shirota |
| HR032 | Bass | BB Tape Note Sustain | Support short bass-note tails with gentle tube compression. | Comp > Mag |
| HR033 | Bass | BB Muted Sample Bass | Remove upper spill from a sampled bass phrase and contain its tail. | Zastava > Comp |
| HR034 | Vocal | BB Dry Front Rap | Keep consonants close with FET control and little ambience. | Comp > Verb |
| HR035 | Vocal | BB Jazzy Intimate Verse | Ride quiet syllables gently before soft opto leveling. | Fader > Comp |
| HR036 | Vocal | BB Tape Slap Story | Add a quiet free-time slap under a warm spoken lead. | Mag > Echo |
| HR037 | Vocal | BB Backroom Hook | Place a doubled hook behind the lead with a dark chamber. | Comp > Shirota > Verb |
| HR038 | Vocal | BB Phrase Tail Answer | Let a ducked quarter echo emerge after the rap phrase. | Comp > Echo |
| HR039 | Keys | BB Felt Chord Bed | Round sharp piano hammers and give chords a small shared chamber. | Udarnik > Mag > Verb |
| HR040 | Keys | BB Rhodes Tube Cushion | Add soft even harmonics under a dry electric piano. | Mag > Comp |
| HR041 | Keys | BB Sampler Piano Slice | Give chopped piano a restrained sampled texture without extending gaps. | Mag > Udarnik |
| HR042 | Keys | BB Wide Chord Reply | Send a filtered eighth answer wide while the original chords remain centered. | Echo |
| HR043 | Samples | BB Dusty Soul Frame | Darken a bright soul loop and retain the chopped envelope. | Mag > Comp |
| HR044 | Samples | BB Jazzy Horn Seat | Tame a nasal horn loop and seat it in a short room. | Comp > Verb |
| HR045 | Samples | BB Last Word Reverse | Reverse only the final eighth of the sample phrase. | Revolution > Mag |
| HR046 | Samples | BB Filtered Pan Conversation | Move the mids across the phrase while leaving its low anchor steady. | Mag; 450-9000 Hz: Revolution |
| HR047 | Master | BB Classic Open Master | Finish a warm boom-bap premaster with minimal reduction and intact snare. | Mag > Comp > RedWall |
| HR048 | Master | BB Dusty Album Cohesion | Join a dusty mix gently without raising noise or narrowing the sample bed. | Comp > Mag > RedWall |
| HR049 | Master | BB Jazzy Headroom Master | Preserve spacious live dynamics with a conservative final ceiling. | Comp > Shirota > RedWall |
| HR050 | Master | BB Gold Low End Frame | Center only the low end and trim a crowded low-mid band before limiting. | Shirota > Comp > RedWall |

## Trap

| ID | Category | Name | Intended result | Ordered processing |
|---|---|---|---|---|
| HR051 | Kick | Trap ATL Click Over Eight | Create a compact upper click that separates the kick from an 808. | Udarnik > Comp |
| HR052 | Kick | Trap Dark Low Hammer | Keep a dark low kick weighty and shorten its overlap with the bass. | Zastava > Mag |
| HR053 | Kick | Trap Cloud Soft Knock | Make a hard kick gentler for a floating melodic beat. | Udarnik > Mag |
| HR054 | Kick | Trap Rage Mid Bite | Add controlled midrange edge while holding the kick sub clean. | Mag > Udarnik |
| HR055 | Kick | Trap Memphis Short Crunch | Make a short kick feel sampled and compressed without a long release. | Mag > Comp |
| HR056 | Kick | Trap Luxury Clean Heel | Tidy the kick low mids with a clean high-crest transient. | Udarnik > Comp |
| HR057 | Snare | Trap ATL Clap Stack | Bring clap onset forward while reducing long upper tails. | Udarnik > Mag |
| HR058 | Snare | Trap Dark Rim Needle | Focus a narrow rim without exaggerating its nasal ring. | Comp > Verb |
| HR059 | Snare | Trap Cloud Air Clap | Spread the airy clap tail around a dry center. | Verb > Shirota |
| HR060 | Snare | Trap Rage Distorted Backbeat | Blend a rough upper snare layer while keeping body clean. | Udarnik; 900-11000 Hz: Mag > Comp |
| HR061 | Snare | Trap Memphis Gated Plate | End a synthetic plate sharply after the clap. | Mag > Verb |
| HR062 | Snare | Trap Luxury Snap Body | Add a little body below a bright clean snap. | Udarnik > Comp |
| HR063 | Hats | Trap ATL Roll Definition | Keep rapid ticks separate by shortening bodies rather than boosting treble. | Udarnik |
| HR064 | Hats | Trap Dark Metal Tame | Suppress sharp metal resonances while retaining quiet roll notes. | Comp |
| HR065 | Hats | Trap Cloud Wide Tick | Place only high hat energy wider with a stable dry center. | Shirota > Verb |
| HR066 | Hats | Trap Rage End Roll | Add one short BLOCK roll at the last sixteenth. | Revolution |
| HR067 | Hats | Trap Triplet Echo Ghost | Put a quiet triplet response around straight hats. | Echo |
| HR068 | Percussion | Trap Bell Ping Seat | Seat a hard bell with a damped offset reply. | Mag > Echo |
| HR069 | Percussion | Trap Memphis Cowbell Grit | Add a sampled rough edge with its fundamental held steady. | Mag > Udarnik |
| HR070 | Percussion | Trap Texture Stereo Dots | Separate dry percussion from small left-right echoes. | Echo |
| HR071 | Percussion | Trap Short Reverse Accent | Reverse the final small percussion gesture only. | Revolution > Verb |
| HR072 | Drum Bus | Trap ATL Dry Glue | Bind kick and snare with a fast recovery while preserving the front edge. | Comp > Mag |
| HR073 | Drum Bus | Trap Dark Parallel Pressure | Add compressed mid-body under clean drum peaks. | Comp; 120-6500 Hz: Mag |
| HR074 | Drum Bus | Trap Cloud Open Tops | Keep low drums centered and give upper percussion modest air. | Comp > Shirota |
| HR075 | Drum Bus | Trap Rage Hit Contrast | Increase hit/body contrast without adding full-range clipping. | Udarnik > Mag |
| HR076 | Drum Bus | Trap Memphis Sample Bus | Make a clean drum bus cohere around a restrained sampled midrange. | Mag > Comp |
| HR077 | Drum Bus | Trap Last Beat Chop | Chop only the upper drum band at the last two subdivisions. | Comp; 800-16000 Hz: Revolution |
| HR078 | Drum Bus | Trap Luxury Peak Discipline | Contain occasional snare spikes with short limiter recovery. | Comp > RedWall |
| HR079 | Bass | Trap ATL Eight Translation | Generate harmonics above a clean 808 fundamental. | Mag > Shirota |
| HR080 | Bass | Trap Dark Long Eight | Level long 808 notes without compressing away the initial drop. | Comp > Mag |
| HR081 | Bass | Trap Cloud Soft Sub | Soften upper buzz on a melodic 808 while maintaining pitch. | Mag > Comp |
| HR082 | Bass | Trap Rage Upper Growl | Distort the bass upper band while leaving the sub undriven. | Shirota; 150-5000 Hz: Mag > Comp |
| HR083 | Bass | Trap Memphis Short Eight | Contain an 808 tail and add compact sampled midrange. | Zastava > Mag |
| HR084 | Vocal | Trap ATL Front Lead | Hold a dry rap vocal forward with fast FET control and restrained air. | Fader > Comp > Mag |
| HR085 | Vocal | Trap Cloud Floating Hook | Let a ducked bright hall bloom after the hook words. | Comp > Verb |
| HR086 | Vocal | Trap Dark Telephone Adlib | Band-limit an adlib and place a gritty echo behind it. | Mag > Echo |
| HR087 | Vocal | Trap Rage Parallel Shout | Add rough density under a controlled shout without boosting ess sounds. | Comp > Mag > Verb |
| HR088 | Vocal | Trap Luxury Pitch Replies | Place a quiet fifth-up reply after the dry phrase. | Comp > Echo |
| HR089 | Keys | Trap Cloud Bell Bed | Keep bell attack clear above a dark softly spreading tail. | Udarnik > Verb > Shirota |
| HR090 | Keys | Trap Dark Piano Space | Remove muddy piano sustain and add a dark chamber. | Comp > Verb |
| HR091 | Keys | Trap Rage Pluck Edge | Add controlled iron harmonics to a hard synth-key pluck. | Mag > Udarnik |
| HR092 | Keys | Trap Luxury Eighth Sparkle | Place light dotted-eighth answers around electric keys. | Mag > Echo > Shirota |
| HR093 | Samples | Trap Memphis Tape Scene | Make a sample darker and more compact without adding continuous noise. | Mag > Comp |
| HR094 | Samples | Trap Cloud Octave Mist | Add sparse octave-up echo with a quiet soft space tail. | Echo |
| HR095 | Samples | Trap Last Word Tape Stop | Stop the final sample subdivision and resume cleanly on the downbeat. | Revolution |
| HR096 | Samples | Trap Gated Melody Pocket | Carve a syncopated melody envelope around the kick and snare. | Zastava > Verb |
| HR097 | Master | Trap ATL Sub Anchor | Keep the sub mono and let kick and vocal peaks survive gentle limiting. | Shirota > Comp > RedWall |
| HR098 | Master | Trap Cloud Air Master | Open a soft hook mix while containing upper harshness dynamically. | Comp > Shirota > RedWall |
| HR099 | Master | Trap Dark Density Master | Support midrange density without saturating the deepest 808. | Mag > Comp > RedWall |
| HR100 | Master | Trap Luxury Clean Ceiling | Finish a balanced clean mix with little colour and bounded stereo width. | Shirota > Comp > RedWall |

## Techno

| ID | Category | Name | Intended result | Ordered processing |
|---|---|---|---|---|
| HR101 | Kick | Techno Peak Front Pillar | Keep a four-on-floor kick firm with low-mid space and an intact transient. | Udarnik > Comp |
| HR102 | Kick | Techno Hypnotic Round Pulse | Round a sharp kick and let its low body carry the pulse. | Mag > Udarnik |
| HR103 | Kick | Techno Minimal Dry Foot | Remove excess tail and leave room for a sparse bass pattern. | Zastava > Udarnik |
| HR104 | Kick | Techno Detroit Iron Center | Give a clean electronic kick restrained iron midrange. | Mag > Comp |
| HR105 | Kick | Techno Dub Dark Rumble Seed | Derive a filtered room bloom from the kick without a long bright tail. | Mag; 70-420 Hz: Verb |
| HR106 | Kick | Techno Hard Parallel Hammer | Blend aggressive upper kick colour below a clean low anchor. | Udarnik; 130-4500 Hz: Mag > Comp |
| HR107 | Snare | Techno Peak Clap Hit | Emphasize clap attack and pull its tail off the next hat. | Udarnik > Comp |
| HR108 | Snare | Techno Hypnotic Rim Chamber | Place a dry rim in a small dark chamber with a clear center. | Mag > Verb |
| HR109 | Snare | Techno Minimal Tight Snap | Contain a synthetic snap with a fast drum envelope. | Zastava > Udarnik |
| HR110 | Snare | Techno Detroit Short Plate | Give a machine snare a metallic but bounded plate tail. | Mag > Verb |
| HR111 | Snare | Techno Dub Snare Throw | Send a damped offbeat repeat behind a dry snare. | Echo |
| HR112 | Snare | Techno Hard Gate Burst | Add a short dense room burst behind a hard snare. | Comp > Verb |
| HR113 | Hats | Techno Peak Open Hat Seat | Shorten overlong offbeat hats while keeping their high lift. | Udarnik > Mag |
| HR114 | Hats | Techno Hypnotic Metallic Dots | Keep a repetitive closed hat focused with subtle dark echoes. | Echo |
| HR115 | Hats | Techno Minimal Tick Transient | Remove body from a small machine tick without expanding stereo width. | Udarnik > Fader |
| HR116 | Hats | Techno Detroit Tape Air | Round brittle hats and keep a modest high-band stereo spread. | Mag > Shirota |
| HR117 | Hats | Techno Hard Last Step Rush | Add one rising BLOCK burst at the phrase edge. | Revolution |
| HR118 | Percussion | Techno Hypnotic Tom Focus | Keep low tom notes distinct and soften excessive stick bite. | Udarnik > Comp |
| HR119 | Percussion | Techno Minimal Wood Reply | Add a quiet short delay around a dry wooden hit. | Echo |
| HR120 | Percussion | Techno Dub Chord Perc Space | Create a filtered ping-pong response with a stable dry hit. | Echo |
| HR121 | Percussion | Techno Industrial Mid Scrape | Add parallel hard colour to metallic percussion without brittle treble. | Mag > Comp |
| HR122 | Drum Bus | Techno Peak Quarter Glue | Let the compressor recover inside a quarter while keeping the kick front. | Comp > Mag |
| HR123 | Drum Bus | Techno Hypnotic Tape Bed | Bind a rolling machine beat with modest tape density. | Mag > Comp |
| HR124 | Drum Bus | Techno Minimal Clean Space | Control the envelope lightly while preserving sparse gaps. | Comp > Verb |
| HR125 | Drum Bus | Techno Detroit Parallel Circuit | Add an energetic mid-band parallel layer beneath clean machine transients. | Comp; 170-6000 Hz: Mag |
| HR126 | Drum Bus | Techno Dub Tops Feedback | Create a dark upper-band delay bed while the low rhythm stays dry. | Comp; 700-12000 Hz: Echo |
| HR127 | Drum Bus | Techno Hard Controlled Density | Increase parallel drum density with protected transient contrast. | Udarnik > Mag > Comp |
| HR128 | Drum Bus | Techno Phrase Tops Shuffle | Shuffle the final upper-drum fragments while preserving the four-on-floor. | Comp; 900-16000 Hz: Revolution |
| HR129 | Bass | Techno Peak Rolling Bass | Level a rolling line without blurring its sixteenth-note spaces. | Comp > Mag |
| HR130 | Bass | Techno Hypnotic Tempo Dip | Cut a gentle tempo-shaped pocket at the start of each quarter. | Fader > Mag |
| HR131 | Bass | Techno Acid Upper Bite | Add controlled resonance-band grit above an undriven sub. | Shirota; 180-7000 Hz: Mag > Comp |
| HR132 | Bass | Techno Dub Round Sustain | Smooth a soft dub-bass line without generating extra low tails. | Comp > Mag |
| HR133 | Bass | Techno Hard Mono Grind | Add a compact mid grind with mono low end and short releases. | Mag > Comp > Shirota |
| HR134 | Vocal | Techno Spoken Close Command | Keep a spoken phrase dry, focused and stable in front of a loud beat. | Fader > Comp |
| HR135 | Vocal | Techno Dub Word Echo | Let a damped dotted echo emerge between spoken words. | Comp > Echo |
| HR136 | Vocal | Techno Gated Chant | Shape a held chant into a repeating syncopated figure. | Zastava > Verb |
| HR137 | Vocal | Techno Warehouse Call | Put a clear dry call ahead of a ducked dark warehouse tail. | Comp > Verb |
| HR138 | Vocal | Techno Last Word Throw Lane | Send only the final two steps to a Revolution echo throw. | Revolution |
| HR139 | Keys | Techno Detroit Chord Punch | Keep a chord stab short, warm and distinct from the kick. | Udarnik > Mag > Comp |
| HR140 | Keys | Techno Dub Chord Tunnel | Create a damped syncopated chord echo with a small chamber return. | Echo > Verb |
| HR141 | Keys | Techno Minimal Pluck Seat | Tuck bright plucks behind the rhythm with restrained early reflections. | Udarnik > Verb |
| HR142 | Keys | Techno Peak Chord Breathing | Give a sustained chord a gentle beat-linked dip and recovery. | Fader > Shirota > Verb |
| HR143 | Samples | Techno Hypnotic Filter Step | Move only the sample upper mids through a repeating low-pass gesture. | Mag; 600-14000 Hz: Revolution |
| HR144 | Samples | Techno Industrial Broken Loop | Add two explicit BLOCK chops near the end of an industrial loop. | Mag > Revolution |
| HR145 | Samples | Techno Dub Stereo Field | Give an ambient texture width and a slow beat-linked pan journey. | Shirota |
| HR146 | Samples | Techno One Beat Reverse Swell | Add a quiet reverse-shaped reverb approaching the next sample hit. | Verb |
| HR147 | Master | Techno Peak Clean Pressure | Finish a club premaster with a stable low center and controlled limiter work. | Comp > Shirota > RedWall |
| HR148 | Master | Techno Hypnotic Longform | Preserve small evolving accents while adding gentle tape cohesion. | Mag > Comp > RedWall |
| HR149 | Master | Techno Dub Depth Master | Keep dub space deep and protect low-mid headroom. | Shirota > Comp > RedWall |
| HR150 | Master | Techno Hard Transient Reserve | Keep aggressive midrange while reserving clean space for kick peaks. | Comp > Mag > RedWall |

## Drum and Bass

| ID | Category | Name | Intended result | Ordered processing |
|---|---|---|---|---|
| HR151 | Kick | DnB Modern Fast Punch | Keep a short kick attack distinct inside a fast break. | Udarnik > Comp |
| HR152 | Kick | DnB Roller Low Pocket | Tighten a rolling kick tail without stripping its low-note weight. | Zastava > Mag |
| HR153 | Kick | DnB Liquid Soft Front | Soften an aggressive kick for an open liquid arrangement. | Udarnik > Comp |
| HR154 | Kick | DnB Neuro Mid Click | Add a compact mid click above clean low-end energy. | Udarnik; 500-6000 Hz: Mag > Comp |
| HR155 | Kick | DnB Jump Up Solid Knock | Emphasize the audible low-mid punch without bloated sub sustain. | Udarnik > Mag |
| HR156 | Kick | DnB Break Resampled Foot | Blend sampled texture into an acoustic kick with fast release. | Mag > Comp |
| HR157 | Snare | DnB Modern Snare Whip | Increase attack/body contrast without making the snare louder by default. | Udarnik > Mag |
| HR158 | Snare | DnB Roller Dry Backbeat | Trim a lingering snare tail while preserving ghost notes. | Zastava > Comp |
| HR159 | Snare | DnB Liquid Chamber Lift | Put a small dark chamber behind a clean light snare. | Udarnik > Verb |
| HR160 | Snare | DnB Neuro Parallel Snap | Add a narrow rough snare edge under the original dry hit. | Comp > Mag |
| HR161 | Snare | DnB Jump Up Short Plate | Give a machine backbeat size with a quickly gated plate. | Udarnik > Verb |
| HR162 | Snare | DnB Break Ghost Detail | Level hard accents gently enough to expose break ghost articulation. | Comp > Mag |
| HR163 | Hats | DnB Modern Fast Silk | Reduce upper bite while keeping dense rolls legible. | Udarnik |
| HR164 | Hats | DnB Roller Shuffled Tops | Add a quiet eighth-triplet ghost around a straight top loop. | Echo |
| HR165 | Hats | DnB Liquid Wide Air | Widen soft cymbal air while keeping the low-mid stereo anchor. | Shirota > Verb |
| HR166 | Hats | DnB Neuro Metallic Tuck | Control a hard metallic top resonance with short envelope action. | Comp > Udarnik |
| HR167 | Hats | DnB Break End Ratchet | Put a single BLOCK roll at the end of the top phrase. | Revolution |
| HR168 | Percussion | DnB Modern Tom Separation | Shorten tom tails and expose their attack inside a busy fill. | Udarnik > Zastava |
| HR169 | Percussion | DnB Roller Wood Groove | Keep woody syncopations compact with a modest iron body. | Mag > Comp |
| HR170 | Percussion | DnB Liquid Bell Glints | Place two quiet stereo bell responses around a dry event. | Echo |
| HR171 | Percussion | DnB Neuro Fragment Answer | Repeat the final percussion fragment once with a restrained wet throw. | Revolution |
| HR172 | Drum Bus | DnB Modern Crest Glue | Glue a fast break with release short enough to recover before the next hit. | Comp > Udarnik |
| HR173 | Drum Bus | DnB Roller Parallel Weight | Add mid-body weight under a clean rolling break. | Comp; 130-5500 Hz: Mag |
| HR174 | Drum Bus | DnB Liquid Open Break | Retain room dynamics and add only a small high-band spread. | Comp > Shirota > Verb |
| HR175 | Drum Bus | DnB Neuro Tight Drum Wall | Shorten drum bodies before a restrained parallel hard-colour layer. | Udarnik > Mag > Comp |
| HR176 | Drum Bus | DnB Jump Up Front Back | Favor kick/snare front edges without increasing cymbal sustain. | Udarnik > Mag > Comp |
| HR177 | Drum Bus | DnB Break Sampler Cohesion | Bring a chopped acoustic break together with restrained sampler colour. | Mag > Comp |
| HR178 | Drum Bus | DnB Last Beat Break Edit | Use explicit upper-band reverse and repeat steps at the phrase end. | Comp; 1000-16000 Hz: Revolution |
| HR179 | Bass | DnB Modern Sub Level | Hold a sine-like sub steady with no stereo motion below the crossover. | Comp > Shirota |
| HR180 | Bass | DnB Roller Mid Translation | Add restrained audible harmonics above an undriven low sub. | Mag > Comp |
| HR181 | Bass | DnB Liquid Warm Sustain | Level a soft bass phrase with smooth opto recovery and tape warmth. | Comp > Mag |
| HR182 | Bass | DnB Neuro Clean Sub Dirty Top | Separate aggressive growl treatment from a mono sub anchor. | Shirota; 180-8000 Hz: Mag > Comp |
| HR183 | Bass | DnB Jump Up Tempo Pocket | Dip the bass envelope on quarter-note starts without requiring a key bus. | Fader > Mag > Shirota |
| HR184 | Vocal | DnB Modern Topline Seat | Control a sung lead with fast recovery and bounded sibilance. | Fader > Comp |
| HR185 | Vocal | DnB Liquid Hall Lift | Give a melodic lead a ducked hall that blooms in phrase gaps. | Comp > Verb |
| HR186 | Vocal | DnB Roller MC Close | Keep a fast MC phrase dry and intelligible with limited leveling boost. | Zastava > Comp > Mag |
| HR187 | Vocal | DnB Neuro Short Radio Call | Make a compact gritty band-limited call with a single sync reply. | Mag > Echo |
| HR188 | Vocal | DnB Break Reversed Tail | Reverse a quiet end-of-phrase response without reversing dry words. | Comp > Echo |
| HR189 | Keys | DnB Liquid Piano Bloom | Keep piano hammers clean while a filtered hall supports the chords. | Comp > Verb |
| HR190 | Keys | DnB Roller Stab Cut | Make chord stabs short enough for a fast rolling arrangement. | Udarnik > Mag > Comp |
| HR191 | Keys | DnB Neuro Pluck Focus | Constrain a bright synth pluck before adding mid-band grit. | Mag > Udarnik |
| HR192 | Keys | DnB Liquid Rhodes Drifts | Add quiet alternating eighth echoes behind a warm Rhodes chord. | Mag > Echo |
| HR193 | Samples | DnB Break Ghost Preservation | Glue a vintage break sample while preserving low-level ghost articulation. | Mag > Comp |
| HR194 | Samples | DnB Neuro Last Eighth Flip | Reverse only the final eighth of a melodic sample. | Revolution |
| HR195 | Samples | DnB Liquid Skyline Texture | Make a quiet atmospheric sample broad above a clean low cut. | Shirota > Verb |
| HR196 | Samples | DnB Roller Rhythmic Fragments | Cut selected off-grid sample fragments into an explicit 16-step response. | Revolution > Verb |
| HR197 | Master | DnB Modern Drum Headroom | Finish a fast full mix with gentle bus control and restrained final limiting. | Comp > Shirota > RedWall |
| HR198 | Master | DnB Liquid Open Finish | Retain the open piano and vocal field with a stable low center. | Comp > Shirota > RedWall |
| HR199 | Master | DnB Neuro Controlled Edge | Tame upper-mid density before a conservative peak ceiling. | Mag > Comp > RedWall |
| HR200 | Master | DnB Roller Low End Finish | Keep the sub mono and prevent low-mid buildup from dominating the limiter. | Shirota > Comp > RedWall |

## Studio

| ID | Category | Name | Intended result | Ordered processing |
|---|---|---|---|---|
| HR201 | Lead Vocal | Studio Transparent Lead | Stabilize phrase level gently before clean vocal compression. | Fader > Comp |
| HR202 | Lead Vocal | Studio Intimate Soul | Smooth close-miked soul singing with soft harmonics and a tiny chamber. | Comp > Mag > Verb |
| HR203 | Lead Vocal | Studio Rock Front Edge | Blend FET density into a rock lead while keeping strong consonants. | Comp > Mag > Verb |
| HR204 | Lead Vocal | Studio Spoken Warmth | Reduce room noise gently and level spoken phrases without a radio crush. | Zastava > Fader > Comp |
| HR205 | Lead Vocal | Studio Dry Fast Verse | Make a fast dry rap verse readable with quick release and no added space. | Comp > Mag |
| HR206 | Lead Vocal | Studio Ballad Open Hall | Keep the lead ahead of a wide hall that opens in sustained gaps. | Comp > Verb |
| HR207 | Lead Vocal | Studio Whisper Support | Support quiet whisper syllables with a bounded rider and soft air control. | Fader > Comp |
| HR208 | Lead Vocal | Studio Small Room Truth | Put a dry vocal in a believable short room without washing out detail. | Comp > Verb |
| HR209 | Lead Vocal | Studio Telephone Bridge | Make an intentional band-limited bridge voice with controlled drive. | Comp > Mag |
| HR210 | Lead Vocal | Studio Fifth Answer | Answer the end of a phrase with a quiet fifth-up sync reflection. | Comp > Echo |
| HR211 | Lead Vocal | Studio Short Double Width | Use quiet offset unpitched reflections as a restrained vocal double. | Comp > Echo |
| HR212 | Lead Vocal | Studio Parallel Shout Bed | Add a rough low-level density bed while retaining the dry shout crest. | Comp > Mag > Verb |
| HR213 | Backing Vocal | Studio Backing Soft Seat | Place a backing stack behind the lead with controlled upper attack. | Comp > Udarnik > Verb |
| HR214 | Backing Vocal | Studio Chorus Side Halo | Make the high backing field wider while preserving a centered vowel body. | Comp > Shirota > Verb |
| HR215 | Backing Vocal | Studio Dark Gang Stack | Keep a gang shout compact, rough and slightly behind the lead. | Comp > Mag > Verb |
| HR216 | Backing Vocal | Studio Air Choir Bloom | Add a quiet octave shimmer only to the filtered reverb return. | Comp > Verb |
| HR217 | Backing Vocal | Studio Reverse Choir Swell | Create a quiet reverse-shaped reverb rise behind held backing vowels. | Verb > Shirota |
| HR218 | Backing Vocal | Studio Rhythmic Hook Gate | Shape a backing hook into explicit soft sixteenth-note openings. | Zastava > Verb |
| HR219 | Guitar | Studio Acoustic Pick Balance | Reduce pick bite and stabilize acoustic strums without flattening them. | Comp > Verb |
| HR220 | Guitar | Studio Fingerstyle Detail | Retain finger transients while supporting quiet ringing notes. | Fader > Comp |
| HR221 | Guitar | Studio Funk Short Chank | Keep muted electric chords tight with a little iron midrange. | Udarnik > Mag > Comp |
| HR222 | Guitar | Studio Clean Amp Slap | Put a filtered short slap behind a clean electric lead. | Mag > Echo |
| HR223 | Guitar | Studio Grit Lead Sustain | Add a parallel rough edge and smooth sustain under a guitar lead. | Comp > Mag > Verb |
| HR224 | Guitar | Studio Wide Rhythm Pair | Widen the high part of a recorded guitar pair without inventing low stereo. | Comp > Shirota |
| HR225 | Guitar | Studio Ambient Guitar Trail | Let a filtered delay feed a quiet long hall after clean guitar notes. | Echo > Verb |
| HR226 | Guitar | Studio Garage Sample Edge | Give a dry guitar sample a compact sampled edge and a small room. | Mag > Comp > Verb |
| HR227 | Keys | Studio Grand Clear Body | Trim piano low-mid congestion while leaving natural hammer dynamics. | Comp > Verb |
| HR228 | Keys | Studio Felt Close Piano | Soften felt-piano mechanical bite and bring the room closer. | Udarnik > Mag > Verb |
| HR229 | Keys | Studio Rhodes Velvet | Smooth electric-piano chords with restrained even harmonics. | Mag > Comp |
| HR230 | Keys | Studio Organ Iron Seat | Give a steady organ soft iron density and a bounded upper field. | Mag > Shirota > Verb |
| HR231 | Keys | Studio Wide Piano Early Room | Use early room reflections to widen piano above its low body. | Verb > Shirota |
| HR232 | Keys | Studio Tape Keys Drift | Add mild tape instability to a sustained key texture. | Mag > Verb |
| HR233 | Keys | Studio Piano Reverse Reply | Add a quiet reversed echo after the piano chord attack. | Echo > Verb |
| HR234 | Keys | Studio Chord Soft Pulse | Make a sustained chord breathe gently over a two-beat custom curve. | Fader > Shirota |
| HR235 | Synth | Studio Lead Focus Cut | Tame upper-mid lead resonance and add a small filtered sync response. | Comp > Echo |
| HR236 | Synth | Studio Pad Wide Calm | Make a pad wide above a mono low anchor with a slow spatial journey. | Shirota > Verb |
| HR237 | Synth | Studio Pluck Falling Notes | Answer a pluck with sparse descending interval reflections. | Echo |
| HR238 | Synth | Studio Synth Texture Stair | Use explicit SHAPE drive steps on the upper band of a synth phrase. | Mag; 600-11000 Hz: Revolution |
| HR239 | Synth | Studio Short Stereo Spark | Add small diffuse high-band stereo energy to a mono pluck. | Shirota > Verb |
| HR240 | Synth | Studio End Phrase Half Time | Apply half-time only on the final two subdivisions of a synth phrase. | Revolution > Echo |
| HR241 | Mix Bus | Studio Mix Gentle Glue | Bind a balanced premaster with minimal reduction and no forced loudness. | Comp > Mag |
| HR242 | Mix Bus | Studio Mix Tape Cohesion | Add restrained tape density while holding the deepest bass clean. | Mag > Comp |
| HR243 | Mix Bus | Studio Mix Low Center | Center only low-frequency mix energy and gently open the top field. | Shirota > Comp |
| HR244 | Mix Bus | Studio Mix Dynamic Mud | Let the low mids duck only when dense chords build up. | Comp |
| HR245 | Mix Bus | Studio Mix Soft Top Control | Control a brittle upper mix band while leaving the rest open. | Comp; 5000-16000 Hz: Comp |
| HR246 | Mix Bus | Studio Mix Parallel Body | Blend a little clean parallel density under the full dry premaster. | Comp > Mag |
| HR247 | Utility | Studio Capture Match Minus Eighteen | Prepare a bounded RMS match capture for comparing source clips at -18 dBFS. | Fader |
| HR248 | Utility | Studio Dual Mono Safe Fold | Fold a stereo source to mono for a deliberate center-only utility. | Fader |
| HR249 | Utility | Studio One Beat Breathing | Apply a clear quarter-note dip and recovery to a sustained input. | Fader |
| HR250 | Utility | Studio Filtered Space Return | Specify a pure wet-only filtered chamber return for an aux bus. | Verb |
