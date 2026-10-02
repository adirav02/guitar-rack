# Foo Fighters, Shinedown and Alter Bridge templates

These **33 complete racks and 49 reusable pedal voices** cover nine original
album songs. They extend the existing catalog to **168 full sounds / 305 voices**
without changing earlier recipes, UUIDs, portable presets, cabinet assets or
effect versions. [Recording and tuning research](modern-rock-tone-research.md)
separates artist/engineer accounts, arrangement evidence and unresolved details
from these proposed app settings. Every new sound remains **Listening pending**.

## Load and play

Rebuild and restart GuitarRack using the [README](../README.md). In
**Choose Preset...**, set Source/Artist/Category to **All**, search a song or
**Modern rock research**, select the intended guitar role and press **Load**.
Use **Save As** for pickup-specific adjustments. These are independent complete
chains; they do not switch sections automatically or generate a second player.

All 33 fit **Legacy 128** with four to six slots. At 48 kHz its transport delay
is 2.667 ms, excluding device I/O and intentional ambience. Physically tune the
guitar; no pitch processor is inserted. This is not a hardware CPU guarantee:
the Everlong/My Hero and Tremonti rhythm blends use Dual Amp, so check live CPU
and model/graph fades on the target machine.

| Song | Physical tuning for this arrangement | Rack roles |
|---|---|---|
| Everlong | Drop D: D A D G B E | Ringing intro, full rhythm, upper detail, reduced-drive breakdown adaptation |
| Walk | Standard E: E A D G B E | Picked clean, driving rhythm, upper chime, melodic accent |
| My Hero | Standard E: E A D G B E | Body/edge rhythm, octave detail, reduced-drive breakdown |
| The Pretender | Standard E: E A D G B E | Dark clean intro, staccato rhythm, open chorus, countermelody |
| Sound of Madness | Drop D: D A D G B E | Tight riff, broad chords, filtered lead adaptation |
| Monsters | D standard: D G C F A D | Verse clarity adaptation, wiry riff, fuller chorus, melodic accents |
| Cut the Cord | Drop D: D A D G B E | Rough fuzz/combo, open chorus, filtered hook accent |
| Devil | Drop D: D A D G B E | Dry stops, open chorus, filtered texture adaptation |
| Blackbird | Eb standard: Eb Ab Db Gb Bb Eb | Fingerpicked clean, two supporting rhythm roles, separate Kennedy/Tremonti leads |

The research matrix records each tuning's evidence strength. **Monsters** is
directly confirmed by Myers; Sound of Madness/Cut the Cord/Devil rely on
instruction/transcription evidence. Blackbird uses an Eb-standard full-song
arrangement. A Kennedy-specific Drop C# report remains unverified in studio
terms; his catalog details expose it. Use the appropriate fingerings if physically
retuning to C# G# C# F# A# D#. Walk's auxiliary baritone tuning is unresolved.

Start with instrument-input trim 0 dB and dry peaks roughly -18 to -9 dBFS.
For Foo Fighters, use moderate bridge-humbucker output and reduce gain before
losing chord detail. For Monsters, a clear single-coil-compatible voice is
informed by later live evidence, not an authenticated studio pickup assignment.
For Blackbird, use clear neck/intermediate fingerpicking and test the two lead
voices separately. Input trim, picking, string tension and voicing matter more
than trying to copy historical knob numbers into unrelated controls.

Cut the Cord's rough sustain-fuzz/US-combo design follows album-level small-amp
evidence. It is not a Superfuzz circuit or an eight-inch speaker capture. The
stationary filtered Shinedown alternatives cannot reproduce Whammy pitch movement;
their adaptation labels are intentional. Foo and Blackbird parallel amp-family
blends share EQ and one downstream synthetic cabinet, unlike two separately
miked rigs. All cabinets and amp networks are original approximations.

## Ordered recipes

Controls live in the reusable voicings in
[library.json](../assets/sounds/library.json); pickup, tuning and evidence details
live in [catalog.json](../assets/sounds/catalog.json). The following order and
output trims come directly from the authoritative factory JSON. Cabinet labels
refer to bundled synthetic filters.

| Factory name | Ordered voicings | Synthetic cabinet | Output dB |
|---|---|---|---|
| Foo Fighters / Everlong - Ringing Intro Crunch | Foo Fighters / Ringing American Crunch → Cabinet Unity → Foo Fighters / Low-D Chord Separation → Research / Dry Tracking Room | clean-open-2x12 | -6.5 |
| Foo Fighters / Everlong - Full Chord Rhythm | Foo Fighters / Complementary Body And Edge → Cabinet Unity → Foo Fighters / Low-D Chord Separation → Research / Dry Tracking Room | british-4x12 | -4.0 |
| Foo Fighters / Everlong - Upper Part Detail | Foo Fighters / Cutting Melodic Drive → Cabinet Unity → Foo Fighters / Upper Part Definition → Research / Dry Tracking Room | british-4x12 | -8.0 |
| Foo Fighters / Everlong - Clean Breakdown Adaptation | Foo Fighters / Reduced Drive Detail → Cabinet Unity → Foo Fighters / Dark Intro Balance → Research / Acoustic Air | clean-open-2x12 | -0.5 |
| Foo Fighters / Walk - Picked Intro Clean | Research / Natural Pick Compression → Foo Fighters / Clear American Picking → Cabinet Unity → Foo Fighters / Picked Clean Balance → Research / Acoustic Air | clean-open-2x12 | 2.5 |
| Foo Fighters / Walk - Driving Rhythm | Foo Fighters / Garage American Drive → Cabinet Unity → Foo Fighters / Garage Mid Body → Research / Dry Tracking Room | british-4x12 | -7.5 |
| Foo Fighters / Walk - Upper Chime Support | Foo Fighters / Upper Chime Support → Cabinet Unity → Foo Fighters / Upper Part Definition → Research / Dry Tracking Room | clean-open-2x12 | -8.5 |
| Foo Fighters / Walk - Melodic Accent Lead | Foo Fighters / Cutting Melodic Drive → Cabinet Unity → Modern Rock / Lead Mid Separation → Modern Rock / Short Melodic Repeat → Research / Restrained Lead Plate | british-4x12 | -8.0 |
| Foo Fighters / My Hero - Body And Edge Rhythm | Foo Fighters / Complementary Body And Edge → Cabinet Unity → Foo Fighters / Firm Chord Body → Research / Dry Tracking Room | british-4x12 | -4.5 |
| Foo Fighters / My Hero - Octave Melody Detail | Foo Fighters / Cutting Melodic Drive → Cabinet Unity → Foo Fighters / Upper Part Definition → Research / Dry Tracking Room | british-4x12 | -8.0 |
| Foo Fighters / My Hero - Reduced Drive Breakdown | Foo Fighters / Reduced Drive Detail → Cabinet Unity → Foo Fighters / Picked Clean Balance → Research / Dry Tracking Room | clean-open-2x12 | 0.0 |
| Foo Fighters / The Pretender - Dark Clean Intro | Research / Natural Pick Compression → Foo Fighters / Clear American Picking → Cabinet Unity → Foo Fighters / Dark Intro Balance → Research / Acoustic Air | clean-open-2x12 | 1.5 |
| Foo Fighters / The Pretender - Staccato Rhythm | Foo Fighters / Dry British Attack → Cabinet Unity → Foo Fighters / Staccato Low-End Control → Foo Fighters / Light Mix Punch → Research / Dry Tracking Room | british-4x12 | -10.5 |
| Foo Fighters / The Pretender - Open Chorus Rhythm | Foo Fighters / Open British Chords → Cabinet Unity → Foo Fighters / Firm Chord Body → Research / Dry Tracking Room | british-4x12 | -11.0 |
| Foo Fighters / The Pretender - Cutting Countermelody | Foo Fighters / Cutting Melodic Drive → Cabinet Unity → Foo Fighters / Upper Part Definition → Research / Dry Tracking Room | british-4x12 | -8.0 |
| Shinedown / Sound of Madness - Tight Riff Rhythm | Research / Rhythm Noise Control → Shinedown / Madness Riff Definition → Cabinet Unity → Shinedown / Madness Low-D Control | tight-modern-4x12 | -12.0 |
| Shinedown / Sound of Madness - Broad Chorus Chords | Shinedown / Madness Open Chords → Cabinet Unity → Shinedown / Madness Low-D Control → Research / Dry Tracking Room | dark-full-4x12 | -15.0 |
| Shinedown / Sound of Madness - Filtered Lead Adaptation | Shinedown / Stationary Lead Color Adaptation → Shinedown / Forward Melodic Sustain → Cabinet Unity → Modern Rock / Lead Mid Separation → Modern Rock / Short Melodic Repeat → Research / Restrained Lead Plate | british-4x12 | -8.5 |
| Shinedown / Monsters - Verse Clarity Adaptation | Research / Natural Pick Compression → Shinedown / Sparse Verse Clarity → Cabinet Unity → Shinedown / Monsters Low-String Balance → Research / Dry Tracking Room | clean-open-2x12 | -1.5 |
| Shinedown / Monsters - Wiry Main Riff | Research / Rhythm Noise Control → Shinedown / Wiry D-Standard Riff → Cabinet Unity → Shinedown / Monsters Low-String Balance | british-4x12 | -11.5 |
| Shinedown / Monsters - Full Chorus Rhythm | Shinedown / Monsters Full Chords → Cabinet Unity → Shinedown / Monsters Low-String Balance → Research / Dry Tracking Room | tight-modern-4x12 | -11.5 |
| Shinedown / Monsters - Melodic Accent Lead | Shinedown / Forward Melodic Sustain → Cabinet Unity → Modern Rock / Lead Mid Separation → Modern Rock / Short Melodic Repeat → Research / Restrained Lead Plate | british-4x12 | -11.0 |
| Shinedown / Cut the Cord - Rough Fuzz Combo | Research / Rhythm Noise Control → Shinedown / Rough Combo Fuzz Adaptation → Shinedown / Rough Small-Combo Support → Cabinet Unity → Shinedown / Combo Fuzz Definition | clean-open-2x12 | -10.0 |
| Shinedown / Cut the Cord - Open Combo Chorus | Shinedown / Open Combo Drive → Cabinet Unity → Shinedown / Combo Fuzz Definition → Research / Dry Tracking Room | clean-open-2x12 | -9.5 |
| Shinedown / Cut the Cord - Filtered Hook Accent | Shinedown / Stationary Lead Color Adaptation → Shinedown / Rough Small-Combo Support → Cabinet Unity → Modern Rock / Lead Mid Separation → Research / Dry Tracking Room | clean-open-2x12 | 6 |
| Shinedown / Devil - Dry Stop Rhythm | Research / Rhythm Noise Control → Shinedown / Devil Dry Stop Attack → Cabinet Unity → Shinedown / Devil Riff Space | tight-modern-4x12 | -12.0 |
| Shinedown / Devil - Open Chorus Rhythm | Shinedown / Devil Broad Chords → Cabinet Unity → Shinedown / Devil Riff Space → Research / Dry Tracking Room | dark-full-4x12 | -15.0 |
| Shinedown / Devil - Filtered Texture Adaptation | Shinedown / Filtered Texture Drive → Shinedown / Rough Small-Combo Support → Cabinet Unity → Shinedown / Narrow Texture Adaptation → Modern Rock / Short Melodic Repeat | clean-open-2x12 | -5.0 |
| Alter Bridge / Blackbird - Fingerpicked Clean | Research / Natural Pick Compression → Alter Bridge / Fingerpicked Clean Body → Cabinet Unity → Alter Bridge / Fingerstyle String Balance → Research / Acoustic Air | clean-open-2x12 | 3.0 |
| Alter Bridge / Blackbird - Tremonti Role Rhythm | Alter Bridge / Complementary Heavy Body → Cabinet Unity → Alter Bridge / Heavy Chord Separation → Research / Dry Tracking Room | dark-full-4x12 | -12.0 |
| Alter Bridge / Blackbird - Kennedy Role Support | Alter Bridge / Supporting Mark-Family Rhythm → Cabinet Unity → Alter Bridge / Complementary Upper Weight → Research / Dry Tracking Room | tight-modern-4x12 | 6 |
| Alter Bridge / Blackbird - Kennedy Expressive Lead | Alter Bridge / Expressive Mark-Family Lead → Cabinet Unity → Alter Bridge / Warm Expressive Lead Mids → Alter Bridge / Expressive Lead Repeat → Research / Restrained Lead Plate | tight-modern-4x12 | -1.5 |
| Alter Bridge / Blackbird - Tremonti Articulate Lead | Alter Bridge / Articulate Hot-Rod Lead → Cabinet Unity → Alter Bridge / Fast Lead Note Definition → Alter Bridge / Articulate Lead Repeat → Research / Restrained Lead Plate | british-4x12 | -10.0 |

## Reproduce and verify

The additive seed retains existing entries and owner edits. UUID keys are
immutable. Review JSON changes before regenerating embedded data and portable files:

```bash
python3 tools/add_modern_rock_templates.py
python3 tools/generate_sound_library.py
python3 tools/generate_sound_catalog.py
cmake --build --preset linux-release
./build/linux-release/guitarrack_export_sounds . > build/linux-release/registry.json
python3 tools/update_sound_schemas.py build/linux-release/registry.json
python3 tools/generate_effect_docs.py build/linux-release/registry.json
python3 tools/check_modern_rock_templates.py --baseline b8e7dc6a42ff086470c7187a3a405c1af2d01a6e
ctest --preset linux-release
./build/linux-release/guitarrack_export_sounds --song-pack build/song-presets
./build/linux-release/guitarrack_render --catalog-audit captures/modern-rock-headroom.csv "Modern rock research"
```

The song pack exports **99 cabinet-inclusive bundles**, including all earlier
66 song chains, and verifies each through the application's importer. Import
the `.grbundle` files in the rebuilt application; edited existing bundles are
retained rather than overwritten. No external audio or third-party IR is added.

Output calibration uses the existing original eight-second synthetic phrase
plus eight-second tail at 48 kHz, with input scales 0.3, 1 and 3. Only new rack
output is adjusted toward 0.015 nominal phrase RMS, rounded to 0.5 dB, capped at
+6 dB and constrained to a worst-input peak below 0.6. It checks headroom, not
perceived loudness, real-pickup matching or album tone. See the
[verification record](testing.md#modern-rock-template-expansion-2026-10-01)
for software results and pending reference/native/hardware checks.
