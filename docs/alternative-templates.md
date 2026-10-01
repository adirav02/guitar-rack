# Alternative recording-inspired rack templates

42 independent complete sounds and 60 reusable pedal voicings for all 14 requested
songs. Every earlier 93 sound / 196 voice retains its identity and recipe.
The total inventory is 135 sounds / 256 voices; quantity is not listening approval.

[Recording research, tuning matrix and evidence register](alternative-tone-research.md)
separates historical findings from proposed settings. All album matching remains
pending. Amp families are original approximations and cabinet filters synthetic.

## Load a sound

Rebuild and restart the app. Open **Choose Preset...**, set filters to **All**,
search **Alternative research** or a song/artist, select and press **Load**.
Each clean/rhythm/lead sound stands alone; no scene switching is required.
Use **Save As** to keep adjustments. Individual voicings appear in the existing
pedal preset menu, and portable JSON files are tracked under `assets/presets/experimental`.

All 42 templates fit **Legacy 128** and contain no pitch processor. Physically
retune using the catalog tuning notes. **Acoustic Source** templates need an
acoustic pickup/microphone source. **Electric Adaptation** is an explicit compromise.
The Outlaws bowed-layer sound is a picked/swelled adaptation; it does not create
bow excitation. Soundgarden role names are complementary design proposals, not
verified isolated Cornell/Thayil tracks. Toxicity's Dual Amp has shared EQ/one
synthetic cab and requires a hardware CPU check, especially during graph fades.

## Complete recipes

Controls and stable UUIDs are authoritative in `assets/sounds/library.json`.
The table lists the actual ordered pedal voices. The selected cabinet is stored
as a checksum identity on Cabinet Unity; no cabinet means an acoustic source path.

| Factory sound | Ordered voices | Synthetic cabinet | Rack Output (dB) |
|---|---|---|---|
| Green Day / Outlaws - Clear Clean | Research / Natural Pick Compression → Green Day / Clear Picked Combo → Cabinet Unity → Green Day / Picked Clarity → Research / Acoustic Air | clean-open-2x12 | 4 |
| Green Day / Outlaws - Broad Crunch | Green Day / Broad British Crunch → Cabinet Unity → Green Day / Broad Outlaws Body → Research / Dry Tracking Room | british-4x12 | -10.5 |
| Green Day / Outlaws - Singing Lead | Green Day / Singing British Lead → Cabinet Unity → Research / Singing Mid Focus → Research / Quiet Lead Echo → Research / Restrained Lead Plate | british-4x12 | -11 |
| Green Day / Outlaws - Bowed Layer Adaptation | Research / Natural Pick Compression → Green Day / Clear Picked Combo → Cabinet Unity → Green Day / Picked Clarity → Research / Layered Halo Echo → Research / Bowed Layer Hall | clean-open-2x12 | 6 |
| Green Day / Revolution Radio - Punch Rhythm | Green Day / Punchy British Rhythm → Cabinet Unity → Green Day / Revolution Chord Bite → Research / Dry Tracking Room | british-4x12 | -10 |
| Green Day / Revolution Radio - Forward Lead | Green Day / Singing British Lead → Cabinet Unity → Green Day / Revolution Chord Bite → Research / Short Melodic Echo → Research / Restrained Lead Plate | british-4x12 | -11 |
| Green Day / Wake Me Up When September Ends - Acoustic Source | Research / Natural Pick Compression → Research / Acoustic Source Balance → Research / Acoustic Air | None | -2.5 |
| Green Day / Wake Me Up When September Ends - Electric Clean Adaptation | Research / Natural Pick Compression → Green Day / Clear Picked Combo → Cabinet Unity → Research / Electric Acoustic Adaptation → Research / Acoustic Air | clean-open-2x12 | 4.5 |
| Green Day / Wake Me Up When September Ends - Heavy Rhythm | Green Day / American Heavy Support → Cabinet Unity → Green Day / September Electric Weight → Research / Dry Tracking Room | british-4x12 | -11.5 |
| Green Day / Wake Me Up When September Ends - Melodic Lead | Green Day / Singing British Lead → Cabinet Unity → Research / Singing Mid Focus → Research / Quiet Lead Echo → Research / Restrained Lead Plate | british-4x12 | -11 |
| Soundgarden / Outshined - Cornell Role Rhythm | Soundgarden / Cornell Role Warm Rhythm → Cabinet Unity → Soundgarden / Warm Low-D Weight → Research / Dry Tracking Room | dark-full-4x12 | -14.5 |
| Soundgarden / Outshined - Thayil Role Rhythm | Soundgarden / Thayil Role Cutting Rhythm → Cabinet Unity → Soundgarden / Complementary Guitar Bite → Research / Dry Tracking Room | british-4x12 | -12 |
| Soundgarden / Outshined - Thayil Lead | Soundgarden / Thayil Role Raw Lead → Cabinet Unity → Research / Singing Mid Focus → Research / Restrained Lead Plate | british-4x12 | -12.5 |
| Soundgarden / Slaves and Bulldozers - Cornell Role Weight | Soundgarden / Cornell Role Bulldozer Body → Cabinet Unity → Soundgarden / Bulldozer Low-Mid Body → Research / Dry Tracking Room | dark-full-4x12 | -18.5 |
| Soundgarden / Slaves and Bulldozers - Thayil Role Bite | Soundgarden / Thayil Role Cutting Rhythm → Cabinet Unity → Soundgarden / Complementary Guitar Bite → Research / Dry Tracking Room | british-4x12 | -12 |
| Soundgarden / Slaves and Bulldozers - Raw Lead | Soundgarden / Thayil Role Raw Lead → Cabinet Unity → Soundgarden / Complementary Guitar Bite → Research / Restrained Lead Plate | british-4x12 | -13 |
| Nirvana / Heart-Shaped Box - Dark Clean | Nirvana / Dark In Utero Clean → Cabinet Unity → Nirvana / Dark Drop-Csharp Picking → Research / Dry Tracking Room | clean-open-2x12 | -3 |
| Nirvana / Heart-Shaped Box - Abrasive Rhythm | Nirvana / Abrasive In Utero Drive → Nirvana / In Utero Drive Support → Cabinet Unity → Nirvana / In Utero Abrasive Mids → Research / Dry Tracking Room | clean-open-2x12 | -15 |
| Nirvana / Heart-Shaped Box - Focused Lead | Nirvana / Rough Melodic Distortion → Nirvana / In Utero Drive Support → Cabinet Unity → Nirvana / In Utero Abrasive Mids → Research / Restrained Lead Plate | clean-open-2x12 | -16 |
| Nirvana / Smells Like Teen Spirit - Watery Clean | Nirvana / Watery Clean Motion → Nirvana / Open Clean Combo → Cabinet Unity → Nirvana / Dry Clean Body → Research / Dry Tracking Room | clean-open-2x12 | 6 |
| Nirvana / Smells Like Teen Spirit - Explosive Rhythm | Nirvana / Hard Chord Distortion → Nirvana / Controlled Nevermind Support → Cabinet Unity → Nirvana / Nevermind Hard-Chord Focus → Research / Dry Tracking Room | british-4x12 | -4.5 |
| Nirvana / Smells Like Teen Spirit - Melodic Lead | Nirvana / Rough Melodic Distortion → Nirvana / Distorted Mono Water → Nirvana / Controlled Nevermind Support → Cabinet Unity → Research / Singing Mid Focus → Research / Restrained Lead Plate | british-4x12 | -2.5 |
| Nirvana / Come as You Are - Deep Mono Chorus | Nirvana / Deep Mono Water → Nirvana / Open Clean Combo → Cabinet Unity → Nirvana / Low-String Water Balance → Research / Dry Tracking Room | clean-open-2x12 | 5 |
| Nirvana / Come as You Are - Chorus Drive | Nirvana / Low-String Chorus Drive → Nirvana / Deep Mono Water → Nirvana / Pedal Driven Combo → Cabinet Unity → Nirvana / Low-String Water Balance → Research / Dry Tracking Room | clean-open-2x12 | -9.5 |
| Nirvana / Come as You Are - Watery Lead | Nirvana / Rough Melodic Distortion → Nirvana / Distorted Mono Water → Nirvana / Pedal Driven Combo → Cabinet Unity → Research / Singing Mid Focus → Research / Restrained Lead Plate | clean-open-2x12 | -9 |
| Nirvana / Lithium - Dry Clean | Nirvana / Open Clean Combo → Cabinet Unity → Nirvana / Dry Clean Body → Research / Dry Tracking Room | clean-open-2x12 | 3.5 |
| Nirvana / Lithium - Dark Sustain Fuzz | Nirvana / Dark Lithium Sustain Fuzz → Nirvana / Pedal Driven Combo → Cabinet Unity → Nirvana / Dark Lithium Weight → Research / Dry Tracking Room | dark-full-4x12 | -13 |
| Nirvana / The Man Who Sold the World MTV Unplugged - Acoustic Source | Research / Acoustic Source Balance → Research / Acoustic Air | None | -1.5 |
| Nirvana / The Man Who Sold the World MTV Unplugged - Colored Acoustic Lead | Nirvana / Restrained Unplugged Color → Nirvana / Electro-Acoustic Mono Color → Nirvana / Electro-Acoustic Clean Support → Cabinet Unity → Nirvana / Dry Clean Body → Research / Acoustic Air | clean-open-2x12 | 1 |
| Nirvana / The Man Who Sold the World MTV Unplugged - Electric Adaptation | Research / Natural Pick Compression → Nirvana / Electro-Acoustic Mono Color → Nirvana / Electro-Acoustic Clean Support → Cabinet Unity → Research / Electric Acoustic Adaptation → Research / Acoustic Air | clean-open-2x12 | 6 |
| Nirvana / In Bloom - Dry Clean | Nirvana / Open Clean Combo → Cabinet Unity → Nirvana / Dry Clean Body → Research / Dry Tracking Room | clean-open-2x12 | 3.5 |
| Nirvana / In Bloom - Full Hard Rhythm | Nirvana / Hard Chord Distortion → Nirvana / Pedal Driven Combo → Cabinet Unity → Nirvana / Nevermind Hard-Chord Focus → Research / Dry Tracking Room | dark-full-4x12 | -11 |
| Nirvana / In Bloom - Rough Lead | Nirvana / Rough Melodic Distortion → Nirvana / Pedal Driven Combo → Cabinet Unity → Research / Singing Mid Focus → Research / Restrained Lead Plate | clean-open-2x12 | -11.5 |
| System Of A Down / Chop Suey! - Picked Clean Adaptation | Research / Natural Pick Compression → System Of A Down / Clear Picked Amp → Cabinet Unity → System Of A Down / Defined Picking → Research / Acoustic Air | clean-open-2x12 | 0.5 |
| System Of A Down / Chop Suey! - Tight Rhythm | Research / Rhythm Noise Control → System Of A Down / Chop Tight Attack → Cabinet Unity → System Of A Down / Chop Low-C Stops | tight-modern-4x12 | -12 |
| System Of A Down / Chop Suey! - Open Heavy Chords | System Of A Down / Open Toxicity Chords → Cabinet Unity → System Of A Down / Toxicity Weight And Bite → Research / Dry Tracking Room | dark-full-4x12 | -16 |
| System Of A Down / Toxicity - Defined Clean | System Of A Down / Clear Picked Amp → Cabinet Unity → System Of A Down / Defined Picking → Research / Dry Tracking Room | clean-open-2x12 | 1.5 |
| System Of A Down / Toxicity - Body And Bite Rhythm | Research / Rhythm Noise Control → System Of A Down / Toxicity Body And Teeth → Cabinet Unity → System Of A Down / Toxicity Weight And Bite | dark-full-4x12 | -12.5 |
| System Of A Down / Toxicity - Articulate Heavy Lead | System Of A Down / Open Toxicity Chords → Cabinet Unity → Research / Singing Mid Focus → Research / Restrained Lead Plate | tight-modern-4x12 | -10.5 |
| System Of A Down / Soldier Side - Clear Clean | Research / Natural Pick Compression → System Of A Down / Clear Picked Amp → Cabinet Unity → System Of A Down / Defined Picking → Research / Acoustic Air | clean-open-2x12 | 0.5 |
| System Of A Down / Soldier Side - British Heavy Rhythm | System Of A Down / Hypnotize British Drive → Cabinet Unity → System Of A Down / Hypnotize String Attack → Research / Dry Tracking Room | british-4x12 | -11.5 |
| System Of A Down / Soldier Side - Singing Lead | System Of A Down / Melodic British Lead → Cabinet Unity → Research / Singing Mid Focus → Research / Quiet Lead Echo → Research / Restrained Lead Plate | british-4x12 | -11.5 |

## Reproduce and verify

The additive seed preserves edits and never updates an existing UUID. Regenerate
embedded sources and portable files after reviewing authoritative JSON changes:

```bash
python3 tools/add_alternative_templates.py
python3 tools/generate_sound_library.py
python3 tools/generate_sound_catalog.py
cmake --build --preset linux-release
./build/linux-release/guitarrack_export_sounds . > build/linux-release/registry.json
python3 tools/update_sound_schemas.py build/linux-release/registry.json
python3 tools/generate_effect_docs.py build/linux-release/registry.json
python3 tools/check_alternative_templates.py --baseline 15870b779998c5d056fec53212b2669e332dacd8
ctest --preset linux-release
./build/linux-release/guitarrack_export_sounds --song-pack build/expanded-song-presets
./build/linux-release/guitarrack_render --catalog-audit captures/alternative-headroom.csv "Alternative research"
```

The song pack now exports 66 song bundles, including all 24 previous Radiohead/Muse
chains. The exporter verifies each bundle through the application's importer and
refuses to overwrite changed bundles. No copyrighted audio or external IR is added.
Synthetic calibration controls output level/headroom, not perceived loudness or
recording authenticity. [Verification and limits](testing.md#alternative-template-expansion-2026-10-01)
record the exact checks and pending native/hardware/reference acceptance.

Outputs use the existing original synthetic phrase at 48 kHz, with input scales
0.3, 1 and 3 and an eight-second tail. New rack output alone is calibrated toward
0.015 phrase RMS at nominal input, rounded to 0.5 dB and capped at +6 dB; the
loud-input peak must remain below 0.6. This is not perceived loudness matching,
pickup calibration or evidence of album accuracy. Raw before/after results and
all test limitations are in the [verification record](testing.md#alternative-template-expansion-2026-10-01).
