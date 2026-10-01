# New effect controls — tone upgrade, 2026-10-01

Generated from the executable registry. Every entry below is **version 1** under
a new type ID. Existing type/version/index/range/default/choice mappings are
frozen in `tests/fixtures/legacy/registry.json`. Values use linear normalization
`(physical-minimum)/(maximum-minimum)`; enums use the listed integer ordinal.
The 16 slots and 16 controls per slot remain fixed. Unused sparse indices are
reserved no-ops; replacing an effect changes the interpretation of that slot.
Controls normally ramp for 20 ms; algorithm-specific transitions are described
in [effects.md](effects.md). No host parameters have been published as a VST3.

## `amp_solid_clean` v1

| Index | Key | Units | Range | Default / enums |
|---|---|---|---|---|
| 00 | `gain_db` | dB | 0…24 | 3 |
| 01 | `bass_db` | dB | -12…12 | 0 |
| 02 | `mid_db` | dB | -12…12 | 0 |
| 03 | `treble_db` | dB | -12…12 | 0 |
| 04 | `master_percent` | % | 0…100 | 35 |
| 05 | `presence_percent` | % | 0…100 | 40 |
| 06 | `output_db` | dB | -36…6 | -12 |
| 11 | `bright_percent` | % | 0…100 | 0 |

## `amp_us_combo` v1

| Index | Key | Units | Range | Default / enums |
|---|---|---|---|---|
| 00 | `gain_db` | dB | 0…24 | 3 |
| 01 | `bass_db` | dB | -12…12 | 0 |
| 02 | `mid_db` | dB | -12…12 | 0 |
| 03 | `treble_db` | dB | -12…12 | 0 |
| 04 | `master_percent` | % | 0…100 | 35 |
| 05 | `presence_percent` | % | 0…100 | 40 |
| 06 | `output_db` | dB | -36…6 | -12 |
| 07 | `sag_percent` | % | 0…100 | 20 |
| 11 | `bright_percent` | % | 0…100 | 0 |

## `amp_brit_chime` v1

| Index | Key | Units | Range | Default / enums |
|---|---|---|---|---|
| 00 | `gain_db` | dB | 0…48 | 18 |
| 01 | `bass_db` | dB | -12…12 | 0 |
| 03 | `treble_db` | dB | -12…12 | 0 |
| 04 | `master_percent` | % | 0…100 | 35 |
| 06 | `output_db` | dB | -36…6 | -12 |
| 07 | `sag_percent` | % | 0…100 | 20 |
| 12 | `cut_percent` | % | 0…100 | 35 |

## `amp_brit_crunch` v1

| Index | Key | Units | Range | Default / enums |
|---|---|---|---|---|
| 00 | `gain_db` | dB | 0…48 | 18 |
| 01 | `bass_db` | dB | -12…12 | 0 |
| 02 | `mid_db` | dB | -12…12 | 0 |
| 03 | `treble_db` | dB | -12…12 | 0 |
| 04 | `master_percent` | % | 0…100 | 35 |
| 05 | `presence_percent` | % | 0…100 | 40 |
| 06 | `output_db` | dB | -36…6 | -12 |
| 07 | `sag_percent` | % | 0…100 | 20 |

## `amp_brit_hotrod` v1

| Index | Key | Units | Range | Default / enums |
|---|---|---|---|---|
| 00 | `gain_db` | dB | 0…48 | 18 |
| 01 | `bass_db` | dB | -12…12 | 0 |
| 02 | `mid_db` | dB | -12…12 | 0 |
| 03 | `treble_db` | dB | -12…12 | 0 |
| 04 | `master_percent` | % | 0…100 | 35 |
| 05 | `presence_percent` | % | 0…100 | 40 |
| 06 | `output_db` | dB | -36…6 | -12 |
| 07 | `sag_percent` | % | 0…100 | 20 |
| 08 | `low_cut_hz` | Hz | 50…250 | 120 |

## `amp_mark` v1

| Index | Key | Units | Range | Default / enums |
|---|---|---|---|---|
| 00 | `gain_db` | dB | 0…48 | 18 |
| 01 | `bass_db` | dB | -12…12 | -4 |
| 02 | `mid_db` | dB | -12…12 | 0 |
| 03 | `treble_db` | dB | -12…12 | 0 |
| 04 | `master_percent` | % | 0…100 | 35 |
| 05 | `presence_percent` | % | 0…100 | 40 |
| 06 | `output_db` | dB | -36…6 | -12 |
| 07 | `sag_percent` | % | 0…100 | 20 |
| 10 | `lead_gain_db` | dB | 0…36 | 18 |

## `amp_modern_tight` v1

| Index | Key | Units | Range | Default / enums |
|---|---|---|---|---|
| 00 | `gain_db` | dB | 0…48 | 18 |
| 01 | `bass_db` | dB | -12…12 | 0 |
| 02 | `mid_db` | dB | -12…12 | 0 |
| 03 | `treble_db` | dB | -12…12 | 0 |
| 04 | `master_percent` | % | 0…100 | 35 |
| 05 | `presence_percent` | % | 0…100 | 40 |
| 06 | `output_db` | dB | -36…6 | -12 |
| 07 | `sag_percent` | % | 0…100 | 5 |
| 08 | `low_cut_hz` | Hz | 50…250 | 120 |
| 09 | `resonance_percent` | % | 0…100 | 20 |

## `amp_recto` v1

| Index | Key | Units | Range | Default / enums |
|---|---|---|---|---|
| 00 | `gain_db` | dB | 0…48 | 18 |
| 01 | `bass_db` | dB | -12…12 | 0 |
| 02 | `mid_db` | dB | -12…12 | 0 |
| 03 | `treble_db` | dB | -12…12 | 0 |
| 04 | `master_percent` | % | 0…100 | 35 |
| 05 | `presence_percent` | % | 0…100 | 40 |
| 06 | `output_db` | dB | -36…6 | -12 |
| 07 | `sag_percent` | % | 0…100 | 45 |
| 09 | `resonance_percent` | % | 0…100 | 40 |

## `asymmetric_drive` v1

| Index | Key | Units | Range | Default / enums |
|---|---|---|---|---|
| 00 | `drive_db` | dB | 0…36 | 8 |
| 01 | `low_cut_hz` | Hz | 40…300 | 100 |
| 02 | `tone_hz` | Hz | 800…10000 | 4800 |
| 03 | `output_db` | dB | -30…18 | -8 |

## `oxford_distortion` v1

| Index | Key | Units | Range | Default / enums |
|---|---|---|---|---|
| 00 | `drive_db` | dB | 0…42 | 18 |
| 01 | `bass_db` | dB | -12…12 | 0 |
| 02 | `contour_percent` | % | 0…100 | 35 |
| 03 | `treble_db` | dB | -12…12 | 1 |
| 04 | `output_db` | dB | -36…6 | -12 |

## `hard_distortion` v1

| Index | Key | Units | Range | Default / enums |
|---|---|---|---|---|
| 00 | `drive_db` | dB | 0…48 | 16 |
| 01 | `low_cut_hz` | Hz | 30…300 | 70 |
| 02 | `filter_hz` | Hz | 500…10000 | 3300 |
| 03 | `output_db` | dB | -36…6 | -12 |

## `sustain_fuzz` v1

| Index | Key | Units | Range | Default / enums |
|---|---|---|---|---|
| 00 | `drive_db` | dB | 0…48 | 20 |
| 01 | `compression_percent` | % | 0…100 | 35 |
| 02 | `tone_hz` | Hz | 500…9000 | 2700 |
| 03 | `output_db` | dB | -36…6 | -15 |

## `bias_fuzz` v1

| Index | Key | Units | Range | Default / enums |
|---|---|---|---|---|
| 00 | `drive_db` | dB | 0…48 | 20 |
| 01 | `gate_db` | dBFS | -80…-18 | -65 |
| 02 | `compression_percent` | % | 0…100 | 35 |
| 03 | `bias_percent` | % | -80…80 | 10 |
| 04 | `stability_percent` | % | 0…100 | 100 |
| 05 | `output_db` | dB | -36…6 | -15 |

## `tight_boost` v1

| Index | Key | Units | Range | Default / enums |
|---|---|---|---|---|
| 00 | `drive_db` | dB | 0…12 | 1 |
| 01 | `low_cut_hz` | Hz | 40…350 | 140 |
| 02 | `attack_percent` | % | 0…100 | 35 |
| 03 | `output_db` | dB | -18…24 | 6 |

## `graphic_eq` v1

| Index | Key | Units | Range | Default / enums |
|---|---|---|---|---|
| 00 | `80_hz_db` | dB | -12…12 | 0 |
| 01 | `240_hz_db` | dB | -12…12 | 0 |
| 02 | `750_hz_db` | dB | -12…12 | 0 |
| 03 | `2200_hz_db` | dB | -12…12 | 0 |
| 04 | `6600_hz_db` | dB | -12…12 | 0 |
| 05 | `output_db` | dB | -24…12 | 0 |

## `parametric_eq` v1

| Index | Key | Units | Range | Default / enums |
|---|---|---|---|---|
| 00 | `band1_hz` | Hz | 30…16000 | 160 |
| 01 | `band1_db` | dB | -12…12 | 0 |
| 02 | `band1_q` | Q | 0.3…5 | 0.7 |
| 03 | `band2_hz` | Hz | 30…16000 | 750 |
| 04 | `band2_db` | dB | -12…12 | 0 |
| 05 | `band2_q` | Q | 0.3…5 | 0.7 |
| 06 | `band3_hz` | Hz | 30…16000 | 3000 |
| 07 | `band3_db` | dB | -12…12 | 0 |
| 08 | `band3_q` | Q | 0.3…5 | 0.7 |
| 09 | `high_pass_hz` | Hz | 20…500 | 20 |
| 10 | `low_pass_hz` | Hz | 1000…20000 | 18000 |
| 11 | `output_db` | dB | -24…12 | 0 |

## `noise_gate` v1

| Index | Key | Units | Range | Default / enums |
|---|---|---|---|---|
| 00 | `threshold_db` | dBFS | -90…-6 | -54 |
| 01 | `range_db` | dB | 0…90 | 60 |
| 02 | `attack_ms` | ms | 0.1…50 | 2 |
| 03 | `hold_ms` | ms | 0…500 | 40 |
| 04 | `release_ms` | ms | 5…2000 | 160 |
| 05 | `hysteresis_db` | dB | 0…18 | 6 |

## `wah` v1

| Index | Key | Units | Range | Default / enums |
|---|---|---|---|---|
| 00 | `position_percent` | % | 0…100 | 50 |
| 01 | `heel_hz` | Hz | 150…1000 | 350 |
| 02 | `toe_hz` | Hz | 1100…4000 | 2400 |
| 03 | `q` | Q | 0.5…8 | 2 |
| 04 | `output_db` | dB | -24…6 | 0 |
| 05 | `mix_percent` | % | 0…100 | 100 |

## `envelope_wah` v1

| Index | Key | Units | Range | Default / enums |
|---|---|---|---|---|
| 00 | `position_percent` | % | 0…100 | 0 |
| 01 | `heel_hz` | Hz | 150…1000 | 350 |
| 02 | `toe_hz` | Hz | 1100…4000 | 2400 |
| 03 | `q` | Q | 0.5…8 | 2 |
| 04 | `attack_ms` | ms | 1…100 | 8 |
| 05 | `release_ms` | ms | 10…1000 | 160 |
| 06 | `sensitivity_db` | dB | 0…36 | 18 |
| 07 | `direction` | enum | 0…1 | 0; 0=Up, 1=Down |
| 08 | `output_db` | dB | -24…6 | 0 |
| 09 | `mix_percent` | % | 0…100 | 100 |

## `phaser` v1

| Index | Key | Units | Range | Default / enums |
|---|---|---|---|---|
| 00 | `rate_hz` | Hz | 0.05…10 | 0.6 |
| 01 | `depth_percent` | % | 0…100 | 60 |
| 02 | `feedback_percent` | % | -80…80 | 15 |
| 03 | `mix_percent` | % | 0…100 | 50 |
| 04 | `stereo_degrees` | deg | 0…180 | 0 |
| 05 | `stages` | enum | 0…1 | 0; 0=4, 1=6 |
| 06 | `output_db` | dB | -24…6 | 0 |

## `flanger` v1

| Index | Key | Units | Range | Default / enums |
|---|---|---|---|---|
| 00 | `rate_hz` | Hz | 0.05…10 | 0.25 |
| 01 | `depth_percent` | % | 0…100 | 40 |
| 02 | `manual_ms` | ms | 0.5…10 | 3 |
| 03 | `feedback_percent` | % | -85…85 | 30 |
| 04 | `mix_percent` | % | 0…100 | 35 |
| 05 | `stereo_degrees` | deg | 0…180 | 90 |
| 06 | `output_db` | dB | -24…6 | 0 |

## `swell` v1

| Index | Key | Units | Range | Default / enums |
|---|---|---|---|---|
| 00 | `attack_ms` | ms | 5…2000 | 350 |
| 01 | `release_ms` | ms | 10…1000 | 120 |
| 02 | `threshold_db` | dBFS | -90…-12 | -60 |
| 03 | `output_db` | dB | -60…6 | 0 |

## `audio_hold` v1

| Index | Key | Units | Range | Default / enums |
|---|---|---|---|---|
| 00 | `hold` | enum | 0…1 | 0; 0=Off, 1=Hold |
| 01 | `mix_percent` | % | 0…100 | 60 |
| 02 | `output_db` | dB | -24…6 | -6 |

## `echo` v1

| Index | Key | Units | Range | Default / enums |
|---|---|---|---|---|
| 00 | `time_ms` | ms | 1…4000 | 420 |
| 01 | `bpm` | BPM | 40…240 | 120 |
| 02 | `subdivision` | enum | 0…4 | 0; 0=Quarter, 1=Eighth, 2=Dotted eighth, 3=Sixteenth, 4=Half |
| 03 | `clock` | enum | 0…1 | 0; 0=Milliseconds, 1=BPM |
| 04 | `mode` | enum | 0…2 | 0; 0=Digital, 1=Analog, 2=Tape |
| 05 | `feedback_percent` | % | 0…70 | 30 |
| 06 | `color_hz` | Hz | 500…16000 | 5000 |
| 07 | `wow_percent` | % | 0…100 | 0 |
| 08 | `stereo` | enum | 0…1 | 0; 0=Stereo, 1=Ping-pong |
| 09 | `heads` | enum | 0…1 | 0; 0=Single, 1=Three: 1 / 0.75 / 0.5 |
| 10 | `mix_percent` | % | 0…100 | 25 |
| 11 | `output_db` | dB | -24…6 | 0 |

## `space_reverb` v1

| Index | Key | Units | Range | Default / enums |
|---|---|---|---|---|
| 00 | `mode` | enum | 0…2 | 0; 0=Room, 1=Plate, 2=Hall |
| 01 | `decay_seconds` | s | 0.2…12 | 1.5 |
| 02 | `pre_delay_ms` | ms | 0…250 | 12 |
| 03 | `damping_hz` | Hz | 500…18000 | 6000 |
| 04 | `tone_hz` | Hz | 500…18000 | 9000 |
| 05 | `mix_percent` | % | 0…100 | 15 |
| 06 | `output_db` | dB | -24…6 | 0 |

## `expression_pitch` v1

| Index | Key | Units | Range | Default / enums |
|---|---|---|---|---|
| 00 | `heel_semitones` | st | -24…24 | 0 |
| 01 | `toe_semitones` | st | -24…24 | 12 |
| 02 | `expression_percent` | % | 0…100 | 0 |
| 03 | `detune_cents` | cent | -100…100 | 0 |
| 04 | `mix_percent` | % | 0…100 | 100 |
| 05 | `output_db` | dB | -24…6 | 0 |

## `dual_amp` v1

| Index | Key | Units | Range | Default / enums |
|---|---|---|---|---|
| 00 | `model_a` | enum | 0…7 | 6; 0=Solid-State Clean, 1=US Clean Combo, 2=British Chime, 3=British Crunch, 4=British Hot-Rod, 5=Mark High Gain, 6=Modern Tight, 7=Recto Heavy |
| 01 | `gain_a_db` | dB | 0…48 | 18 |
| 02 | `master_a_percent` | % | 0…100 | 35 |
| 03 | `level_a_db` | dB | -24…6 | 0 |
| 04 | `model_b` | enum | 0…7 | 2; 0=Solid-State Clean, 1=US Clean Combo, 2=British Chime, 3=British Crunch, 4=British Hot-Rod, 5=Mark High Gain, 6=Modern Tight, 7=Recto Heavy |
| 05 | `gain_b_db` | dB | 0…48 | 8 |
| 06 | `master_b_percent` | % | 0…100 | 35 |
| 07 | `level_b_db` | dB | -24…6 | 0 |
| 08 | `blend_percent` | % | 0…100 | 30 |
| 09 | `bass_db` | dB | -12…12 | 0 |
| 10 | `mid_db` | dB | -12…12 | 0 |
| 11 | `treble_db` | dB | -12…12 | 0 |
| 12 | `presence_percent` | % | 0…100 | 40 |
| 13 | `output_db` | dB | -36…6 | -12 |

