#!/usr/bin/env python3
"""Add original Radiohead/Muse starting points without rewriting existing tones.

Seed identities derive from immutable keys. Once added, library/catalog JSON is
canonical; re-running preserves edits. See docs/song-tone-research.md for sources
and the unverified reference-listening limitations.
"""
import json
import uuid
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
NAMESPACE = uuid.UUID('8eb9076d-160c-4947-bc40-bcaed06aa4ab')
# Nominal synthetic phrase RMS target ~0.01 at 48 kHz; owner level matching remains pending.
OUTPUT_LEVELS = {
    "radiohead.letdown.clean": 1.5,
    "radiohead.letdown.space": 4.5,
    "radiohead.nosurprises.clean": 1.0,
    "radiohead.alien.foundation": -2.5,
    "radiohead.alien.lead": 1.0,
    "radiohead.weirdfishes.clean": 2.5,
    "muse.pluginbaby.fuzz": -11.0,
    "muse.stockholm.rhythm": -13.0,
    "muse.starlight.chime": -1.5,
    "muse.knights.western": -8.0,
    "muse.space.lead": -8.0,
    "muse.time.fuzz": -14.5,
    "muse.citizen.distortion": -17.5,
    "muse.citizen.clean": 4.0,
    "muse.newborn.rhythm": -10.5,
    "muse.newborn.lead": -2.0,
    "muse.hysteria.guitar": -9.5,
    "muse.psycho.rhythm": -12.5,
    "muse.reapers.tapping": -9.5,
    "muse.reapers.lead": -8.0,
    "muse.defector.rhythm": -13.0,
    "muse.globalist.intro": 3.0,
    "muse.globalist.heavy": -15.0,
    "muse.globalist.outro": 0.5
}
library_path = ROOT / 'assets/sounds/library.json'
catalog_path = ROOT / 'assets/sounds/catalog.json'
library = json.loads(library_path.read_text())
catalog = json.loads(catalog_path.read_text())
voices = {p['name']: p['id'] for p in library['pedals']}
cabinets = {c['name']: c['id'] for c in json.loads((ROOT / 'assets/ir/catalog.json').read_text())['assets']}


def identity(key):
    return str(uuid.uuid5(NAMESPACE, key))


def pedal(key, name, effect, version=1, **parameters):
    voice_id = identity('pedal:' + key)
    existing = next((p for p in library['pedals'] if p['id'] == voice_id), None)
    if existing is None:
        library['pedals'].append(dict(id=voice_id, key=key, name=name,
                                     effect_type=effect, effect_version=version,
                                     parameters=parameters))
    voices[name] = voice_id
    return name


def sound(key, name, artist, gain, character, recipe, *, cab, amp, pickups,
          notes, tuning='Standard E', output=-9, collection=None):
    sound_id = identity('sound:' + key)
    if not any(r['id'] == sound_id for r in library['racks']):
        library['racks'].append(dict(
            id=sound_id, key=key, name=artist + ' / ' + name, output_db=OUTPUT_LEVELS.get(key, output),
            pedals=recipe, pedal_ids=[voices[p] for p in recipe],
            instance_ids=[identity(f'instance:{key}:{slot}') for slot in range(len(recipe))],
            cabinet_assets={str(slot): cabinets[cab] for slot, p in enumerate(recipe)
                            if p == 'Cabinet Unity'}))
    if not any(m['id'] == sound_id for m in catalog['sounds']):
        catalog['sounds'].append(dict(
            id=sound_id, artist=artist, style='Art Rock' if artist == 'Radiohead' else 'Alternative Rock',
            gain=gain, character=character, tags=[artist, name, gain, 'Song templates', key] + ([collection] if collection else []),
            tuning=tuning, pickups=pickups,
            input='Begin at trim 0 dB; aim for dry peaks around -18 to -9 dBFS. Match your pickup level.',
            amp=amp + ' (original inspired model)',
            cabinet=cab + ' (original synthetic response; not a measured speaker)',
            notes='Inferred guitar starting point; recording settings and reference audio unverified. ' + notes,
            evidence='Primary DSP sources in docs/song-tone-research.md; synthetic signal/state checks only, album match unverified.',
            listening='Listening pending'))


pedal('song.comp.transient', 'Song / Gentle Pick Compression', 'compressor',
      threshold_db=-24, ratio=2.2, attack_ms=25, release_ms=160, makeup_db=2, mix_percent=70)
pedal('song.comp.bell', 'Song / Bell Compression', 'compressor',
      threshold_db=-27, ratio=2.5, attack_ms=18, release_ms=180, makeup_db=2, mix_percent=75)
pedal('song.amp.chime', 'Song / Clear Chime', 'amp_brit_chime',
      gain_db=1, bass_db=-2, treble_db=2, cut_percent=30, master_percent=20, output_db=-3)
pedal('song.amp.round', 'Song / Rounded Clean Combo', 'amp_us_combo',
      gain_db=0, bass_db=-2, mid_db=0, treble_db=0, master_percent=20, output_db=-3)
pedal('song.amp.alien', 'Song / Warm Alien Clean', 'amp_solid_clean',
      gain_db=2, bass_db=-2, mid_db=1, treble_db=0, master_percent=20, output_db=-3)
pedal('song.amp.starlight', 'Song / Starlight Edge', 'amp_brit_chime',
      gain_db=6, bass_db=-2, treble_db=2, cut_percent=35, master_percent=28, output_db=-6)
pedal('song.eq.chime', 'Song / Chime Focus', 'parametric_eq',
      high_pass_hz=85, low_pass_hz=8500, band1_hz=220, band1_db=-1.5,
      band2_hz=1800, band2_db=1, band3_hz=4200, band3_db=-1)
pedal('song.eq.bell', 'Song / Rounded Bell Focus', 'parametric_eq',
      high_pass_hz=90, low_pass_hz=6500, band1_hz=240, band1_db=-1,
      band2_hz=1500, band2_db=.5, band3_hz=3800, band3_db=-1.5)
pedal('song.eq.alien', 'Song / Alien Lead Focus', 'parametric_eq',
      high_pass_hz=100, low_pass_hz=7200, band1_hz=240, band1_db=-1.5,
      band2_hz=1100, band2_db=1, band3_hz=3600, band3_db=-1)
pedal('song.mod.arpeggio', 'Song / Arpeggio Halo', 'chorus',
      rate_hz=.28, depth_percent=16, width_percent=65, mix_percent=12)
pedal('song.mod.alien', 'Song / Alien Slow Motion', 'chorus',
      rate_hz=.32, depth_percent=32, width_percent=85, mix_percent=23)
pedal('song.mod.space', 'Song / Slow Space Chorus', 'chorus',
      rate_hz=.18, depth_percent=38, width_percent=90, mix_percent=20)


def echo(key, name, time, feedback, mix, *, mode=2, color=4500, wow=0,
         speed=.35, width=0, low_cut=150, bpm=120, clock=0, subdivision=0):
    return pedal(key, name, 'echo', version=2, time_ms=time, bpm=bpm,
                 clock=clock, subdivision=subdivision, mode=mode,
                 feedback_percent=feedback, color_hz=color, wow_percent=wow,
                 stereo=0, heads=0, mix_percent=mix, output_db=0,
                 wet_low_cut_hz=low_cut, wow_rate_hz=speed, wow_width_percent=width)


echo('song.echo.letdown', 'Song / Let Down Picking Echo', 205, 22, 16,
     color=5200, wow=4, speed=.4, width=35)
echo('song.echo.letdown.space', 'Song / Let Down Space Echo', 410, 34, 25,
     color=4200, wow=28, speed=.24, width=90, low_cut=180)
echo('song.echo.alien.rhythm', 'Song / Alien Foundation Echo', 400, 24, 15,
     color=4200, wow=18, speed=.3, width=60)
echo('song.echo.alien.lead', 'Song / Alien Floating Echo', 400, 42, 30,
     color=3600, wow=45, speed=.22, width=100, low_cut=200)
echo('song.echo.weird', 'Song / Interlocking Arpeggio Echo', 200, 22, 16,
     color=5200, wow=8, speed=.4, width=65)
echo('song.echo.starlight', 'Song / Chiming Eighth Echo', 250, 22, 14,
     color=5000, wow=8, speed=.4, width=50, bpm=120, clock=1, subdivision=1)
echo('song.echo.western', 'Song / Western Gallop Echo', 175, 23, 16,
     color=4200, wow=10, speed=.5, width=55)
echo('song.echo.lead', 'Song / Stadium Lead Echo', 360, 30, 19,
     color=3800, wow=20, speed=.3, width=80, low_cut=200)


def space(key, name, mode, decay, pre, damping, mix):
    return pedal(key, name, 'space_reverb', mode=mode, decay_seconds=decay,
                 pre_delay_ms=pre, damping_hz=damping, tone_hz=6500,
                 mix_percent=mix, output_db=0)


space('song.space.pick', 'Song / Picking Plate', 1, 1.8, 22, 5800, 14)
space('song.space.bell', 'Song / Bell Room', 0, 1.2, 12, 5200, 11)
space('song.space.letdown', 'Song / Let Down Hall', 2, 3.6, 28, 4500, 24)
space('song.space.alien', 'Song / Alien Floating Hall', 2, 4.2, 30, 3800, 27)
space('song.space.muse', 'Song / Muse Lead Plate', 1, 1.6, 18, 4800, 12)
pedal('song.fuzz.plug', 'Song / Plug In Singing Fuzz', 'bias_fuzz',
      drive_db=25, gate_db=-70, compression_percent=35, bias_percent=10,
      stability_percent=100, output_db=-15)
pedal('song.fuzz.time', 'Song / Focused Sustain Fuzz', 'sustain_fuzz',
      drive_db=22, compression_percent=40, tone_hz=3300, output_db=-14)
pedal('song.amp.stockholm', 'Song / Stockholm Tight Rhythm', 'amp_modern_tight',
      gain_db=23, bass_db=-3, mid_db=2, treble_db=0, master_percent=35,
      presence_percent=35, sag_percent=5, low_cut_hz=150,
      resonance_percent=18, output_db=-14)

sound('radiohead.letdown.clean', 'Let Down - Clean Arpeggio', 'Radiohead', 'Clean',
      'Clear interlocking picking with a quiet filtered echo and short plate.',
      ['Song / Gentle Pick Compression', 'Song / Clear Chime', 'Cabinet Unity',
       'Song / Chime Focus', 'Song / Let Down Picking Echo', 'Song / Picking Plate'],
      cab='chime-2x12', amp='British Chime', pickups='Neck/middle single coil or clear humbucker; pick lightly and evenly.',
      notes='Play the arpeggio cleanly; the layered second guitar is a separate performance, not generated by delay.')
sound('radiohead.letdown.space', 'Let Down - Space Layer', 'Radiohead', 'Ambient',
      'A separate slow stereo halo for sustained notes around the clean arpeggio.',
      ['Song / Gentle Pick Compression', 'Song / Clear Chime', 'Cabinet Unity',
       'Song / Chime Focus', 'Song / Slow Space Chorus', 'Song / Let Down Space Echo', 'Song / Let Down Hall'],
      cab='chime-2x12', amp='British Chime', pickups='Clear neck/middle sound; sustain upper-register notes or use volume swells.',
      notes='Optional accompaniment texture. Use the Clean Arpeggio template for the defined picking part.')
sound('radiohead.nosurprises.clean', 'No Surprises - Bell Clean', 'Radiohead', 'Clean',
      'Rounded clean guitar with even bell-like attacks and a small room; no echo wash.',
      ['Song / Bell Compression', 'Song / Rounded Clean Combo', 'Cabinet Unity',
       'Song / Rounded Bell Focus', 'Song / Bell Room'],
      cab='clean-open-2x12', amp='US Clean Combo', pickups='Neck/middle pickup; clear high-register picking with gentle dynamics.',
      notes='The recording also has a separate glockenspiel. This is a clean guitar template, not a recreation of that instrument.')
sound('radiohead.alien.foundation', 'Subterranean Homesick Alien - Clean Foundation', 'Radiohead', 'Clean',
      'Warm articulate clean accompaniment, a quiet modulated echo and gentle space.',
      ['Song / Gentle Pick Compression', 'Song / Warm Alien Clean', 'Cabinet Unity',
       'Song / Rounded Bell Focus', 'Song / Arpeggio Halo', 'Song / Alien Foundation Echo', 'Song / Picking Plate'],
      cab='clean-open-2x12', amp='Solid-State Clean', pickups='Neck or middle pickup; let notes ring without excessive bass.',
      notes='Use Floating Lead for the wider space part. Echo is a free 400-ms starting point; adjust to the reference by ear.')
sound('radiohead.alien.lead', 'Subterranean Homesick Alien - Floating Lead', 'Radiohead', 'Ambient',
      'Audible dry attack surrounded by slow chorus, dark moving repeats and a diffuse hall.',
      ['Song / Gentle Pick Compression', 'Song / Warm Alien Clean', 'Cabinet Unity',
       'Song / Alien Lead Focus', 'Song / Alien Slow Motion', 'Song / Alien Floating Echo', 'Song / Alien Floating Hall'],
      cab='clean-open-2x12', amp='Solid-State Clean', pickups='Bright neck/middle or bridge pickup with treble softened; let sustained notes breathe.',
      notes='400-ms echo, 42% feedback, 30% wet; hall 4.2 s / 30-ms predelay. Supplied YouTube audio was inaccessible. Compare against it by ear; existing Subterranean Pulse remains separate.')
sound('radiohead.weirdfishes.clean', 'Weird Fishes - Interlocking Clean', 'Radiohead', 'Clean',
      'A dry-forward chiming arpeggio with subtle chorus and restrained repetitions.',
      ['Song / Gentle Pick Compression', 'Song / Clear Chime', 'Cabinet Unity',
       'Song / Chime Focus', 'Song / Arpeggio Halo', 'Song / Interlocking Arpeggio Echo', 'Song / Picking Plate'],
      cab='chime-2x12', amp='British Chime', pickups='Neck/middle single coil; even fingerpicking or a light pick.',
      notes='Multiple interlocking guitar performances create the arrangement; the delay is supporting ambience.')
sound('muse.pluginbaby.fuzz', 'Plug In Baby - Singing Fuzz', 'Muse', 'Fuzz',
      'Stable aggressive bias fuzz, chime support and a quiet lead plate.',
      ['Song / Plug In Singing Fuzz', 'British Chime Edge', 'Cabinet Unity', 'Post-Cab Tight', 'Song / Muse Lead Plate'],
      cab='chime-2x12', amp='British Chime', pickups='Bridge humbucker; precise single-note picking and moderate input trim.',
      notes='Bias fuzz is an original algorithm; no exact Fuzz Factory circuit or studio-settings claim.', output=-12)
sound('muse.stockholm.rhythm', 'Stockholm Syndrome - Tight Rhythm', 'Muse', 'High Gain',
      'Tight modern saturation, controlled bass and dry palm-muted attack.',
      ['Tight Gate', 'Metal Tight Boost', 'Song / Stockholm Tight Rhythm', 'Cabinet Unity', 'Post-Cab Tight'],
      cab='tight-modern-4x12', amp='Modern Tight', pickups='Bridge humbucker; palm mute cleanly and keep gate below the quietest intended note.',
      notes='No echo/reverb wash. Retune the guitar physically; the rack does not retune strings.', tuning='Drop D; retune guitar physically', output=-12)
sound('muse.starlight.chime', 'Starlight - Chiming Guitar', 'Muse', 'Crunch',
      'Compressed chime with light breakup, a quiet tempo eighth echo and short plate.',
      ['Song / Gentle Pick Compression', 'Song / Starlight Edge', 'Cabinet Unity',
       'Song / Chime Focus', 'Song / Chiming Eighth Echo', 'Song / Picking Plate'],
      cab='chime-2x12', amp='British Chime', pickups='Bridge/middle or split humbucker; play octave accompaniment with clear attacks.',
      notes='120 BPM is adjustable, not a verified recording tempo. The signature keyboard melody is a separate instrument.')
sound('muse.knights.western', 'Knights of Cydonia - Western Guitar', 'Muse', 'Crunch',
      'British crunch and a short dark echo for galloping guitar articulation.',
      ['British Crunch Rhythm', 'Cabinet Unity', 'Post-Cab Tight', 'Song / Western Gallop Echo', 'Gentle Room'],
      cab='british-4x12', amp='British Crunch', pickups='Bridge pickup; precise galloping rhythm and cleanly separated notes.',
      notes='Guitar texture only; layered vocals, keyboards and other instruments are not synthesized.', output=-12)
sound('muse.space.lead', 'Space Lead - Sustained Echo', 'Muse', 'Distortion',
      'Sustained mid-forward drive with restrained stereo echo and plate ambience.',
      ['Lead Decay Gate', 'British Hot-Rod Lead', 'Cabinet Unity', 'Post-Cab Tight',
       'Song / Stadium Lead Echo', 'Song / Muse Lead Plate'],
      cab='british-4x12', amp='British Hot-Rod', pickups='Bridge humbucker; sustain single notes and bends, reduce bass if the echoes crowd the attack.',
      notes='Reusable Muse-inspired lead texture; no particular studio song or hardware chain is asserted.', output=-12)
sound('muse.time.fuzz', 'Time Is Running Out - Guitar Fuzz', 'Muse', 'Fuzz',
      'Focused midrange sustaining guitar fuzz with restrained ambience.',
      ['Song / Focused Sustain Fuzz', 'British Crunch Edge', 'Cabinet Unity', 'Nasal Mid Fuzz', 'Song / Muse Lead Plate'],
      cab='british-4x12', amp='British Crunch', pickups='Bridge humbucker; match pickup level and use short articulated phrases.',
      notes='A guitar adaptation of the song texture; the signature bass part is a separate instrument.', output=-12)

# Additional song recipes: original approximations using existing processors.
pedal('muse.citizen.fuzz', 'Muse / Citizen Dense Sustain', 'sustain_fuzz',
      drive_db=21, compression_percent=30, tone_hz=3600, output_db=-14)
pedal('muse.citizen.amp', 'Muse / Citizen Low-String Amp', 'amp_brit_hotrod',
      gain_db=16, bass_db=-2, mid_db=2, treble_db=0, master_percent=38,
      presence_percent=30, low_cut_hz=70, output_db=-11)
pedal('muse.citizen.clean', 'Muse / Citizen Ending Clean Amp', 'amp_us_combo',
      gain_db=0, bass_db=-1, mid_db=0, treble_db=1, master_percent=18, output_db=-3)
pedal('muse.citizen.eq', 'Muse / Citizen Low-String Focus', 'parametric_eq',
      high_pass_hz=50, low_pass_hz=6800, band1_hz=180, band1_db=-2,
      band2_hz=950, band2_db=2, band3_hz=3800, band3_db=-1)
pedal('muse.newborn.fuzz', 'Muse / New Born Tight Bias Fuzz', 'bias_fuzz',
      drive_db=27, gate_db=-68, compression_percent=40, bias_percent=15,
      stability_percent=95, output_db=-15)
pedal('muse.newborn.amp', 'Muse / New Born Forward Chime', 'amp_brit_chime',
      gain_db=7, bass_db=-3, treble_db=1, cut_percent=38, master_percent=30, output_db=-9)
pedal('muse.hysteria.drive', 'Muse / Hysteria Guitar Saturation', 'hard_distortion',
      drive_db=19, low_cut_hz=100, filter_hz=4400, output_db=-13)
pedal('muse.hysteria.eq', 'Muse / Hysteria Guitar Mid Focus', 'parametric_eq',
      high_pass_hz=90, low_pass_hz=6500, band1_hz=200, band1_db=-1,
      band2_hz=1200, band2_db=2.5, band2_q=.9, band3_hz=3800, band3_db=-1)
pedal('muse.psycho.amp', 'Muse / Psycho Dry Riff Amp', 'amp_modern_tight',
      gain_db=21, bass_db=-2, mid_db=2.5, treble_db=-1, master_percent=36,
      presence_percent=30, sag_percent=8, low_cut_hz=120, resonance_percent=25, output_db=-13)
pedal('muse.reapers.tap', 'Muse / Reapers Tapping Amp', 'amp_brit_hotrod',
      gain_db=18, bass_db=-3, mid_db=2, treble_db=1, master_percent=35,
      presence_percent=38, low_cut_hz=140, output_db=-12)
pedal('muse.reapers.wah', 'Muse / Reapers Lead Wah', 'wah',
      position_percent=62, heel_hz=300, toe_hz=2600, q=2.0, output_db=0, mix_percent=55)
pedal('muse.defector.amp', 'Muse / Defector Broad Rhythm Amp', 'amp_brit_hotrod',
      gain_db=22, bass_db=0, mid_db=1, treble_db=0, master_percent=40,
      presence_percent=32, low_cut_hz=100, output_db=-14)
pedal('muse.globalist.intro', 'Muse / Globalist Intro Clean Amp', 'amp_solid_clean',
      gain_db=0, bass_db=-2, mid_db=0, treble_db=1, master_percent=18, output_db=-3)
pedal('muse.globalist.heavy', 'Muse / Globalist Heavy Amp', 'amp_recto',
      gain_db=22, bass_db=-3, mid_db=1.5, treble_db=-1, master_percent=42,
      presence_percent=28, sag_percent=35, resonance_percent=25, output_db=-14)
pedal('muse.globalist.outro', 'Muse / Globalist Outro Clean Amp', 'amp_us_combo',
      gain_db=0, bass_db=-2, mid_db=1, treble_db=-1, master_percent=20, output_db=-3)
pedal('muse.clean.chorus', 'Muse / Gentle Ending Chorus', 'chorus',
      rate_hz=.25, depth_percent=18, width_percent=70, mix_percent=13)
echo('muse.citizen.echo', 'Muse / Citizen Ending Echo', 380, 29, 20,
     color=4500, wow=15, speed=.25, width=75, low_cut=160)
echo('muse.reapers.echo', 'Muse / Reapers Lead Echo', 300, 28, 18,
     color=4200, wow=8, speed=.45, width=70, low_cut=180)
echo('muse.globalist.echo', 'Muse / Globalist Intro Echo', 460, 36, 26,
     color=3900, wow=25, speed=.2, width=90, low_cut=180)
space('muse.citizen.space', 'Muse / Citizen Ending Plate', 1, 2.4, 24, 4800, 19)
space('muse.globalist.space', 'Muse / Globalist Intro Hall', 2, 4.8, 32, 4000, 28)
space('muse.globalist.outro.space', 'Muse / Globalist Outro Room', 0, 1.5, 16, 4500, 13)

sound('muse.citizen.distortion', 'Citizen Erased - Dense Distortion', 'Muse', 'Distortion',
      'Dense sustained distortion with controlled low strings and a forward guitar midrange.',
      ['Muse / Citizen Dense Sustain', 'Muse / Citizen Low-String Amp', 'Cabinet Unity',
       'Muse / Citizen Low-String Focus', 'Gentle Room'],
      cab='dark-full-4x12', amp='British Hot-Rod', pickups='Bridge humbucker; articulate the low-register riff and damp unused strings.',
      tuning='Low-register / extended-range part; use appropriate strings and physical tuning',
      notes='Heavy section starting point. The rack does not generate a low string or retune the guitar; match the arrangement physically.',
      collection='Muse expansion', output=-12)
sound('muse.citizen.clean', 'Citizen Erased - Clean Ending', 'Muse', 'Clean',
      'Soft clean ending guitar with light compression, slow chorus and a restrained echo/plate tail.',
      ['Song / Gentle Pick Compression', 'Muse / Citizen Ending Clean Amp', 'Cabinet Unity',
       'Song / Rounded Bell Focus', 'Muse / Gentle Ending Chorus', 'Muse / Citizen Ending Echo', 'Muse / Citizen Ending Plate'],
      cab='clean-open-2x12', amp='US Clean Combo', pickups='Neck or split humbucker; reduce guitar volume and pick softly, letting chords ring.',
      notes='Separate clean ending template. The piano and other layered instruments in the ending are not synthesized by this guitar chain.',
      collection='Muse expansion', output=-9)
sound('muse.newborn.rhythm', 'New Born - Fuzz Rhythm', 'Muse', 'Fuzz',
      'Tight aggressive bias fuzz with forward chime and dry riff articulation.',
      ['Muse / New Born Tight Bias Fuzz', 'Muse / New Born Forward Chime', 'Cabinet Unity',
       'Post-Cab Tight', 'Gentle Room'],
      cab='chime-2x12', amp='British Chime', pickups='Bridge humbucker; pick the riff precisely and palm mute the low string.',
      tuning='Drop D; retune guitar physically',
      notes='Guitar riff starting point. The piano introduction is a separate instrument; no particular fuzz circuit is claimed.',
      collection='Muse expansion', output=-12)
sound('muse.newborn.lead', 'New Born - Filtered Fuzz Lead', 'Muse', 'Fuzz',
      'Mid-focused fuzz lead with a manual resonant wah and quiet stereo echo.',
      ['Muse / New Born Tight Bias Fuzz', 'Singing Manual Wah', 'Muse / New Born Forward Chime',
       'Cabinet Unity', 'Post-Cab Tight', 'Song / Stadium Lead Echo', 'Song / Muse Lead Plate'],
      cab='chime-2x12', amp='British Chime', pickups='Bridge humbucker; use single notes, bends and deliberate wah Position changes.',
      tuning='Drop D; retune guitar physically',
      notes='Lead-section alternative to Fuzz Rhythm. Wah is controlled manually by the Position knob; no pedal/MIDI mapping or automated performance is included.',
      collection='Muse expansion', output=-12)
sound('muse.hysteria.guitar', 'Hysteria - Guitar Drive', 'Muse', 'Distortion',
      'Focused saturated guitar accompaniment and lead, with strong upper mids and a small plate.',
      ['Muse / Hysteria Guitar Saturation', 'British Crunch Edge', 'Cabinet Unity',
       'Muse / Hysteria Guitar Mid Focus', 'Song / Muse Lead Plate'],
      cab='british-4x12', amp='British Crunch', pickups='Bridge humbucker; separate the guitar lines from the bass riff and keep fast notes clear.',
      notes='Guitar template for the song. Its signature opening riff is bass; this does not reproduce that bass recording or add an octave bass voice.',
      collection='Muse expansion', output=-12)
sound('muse.psycho.rhythm', 'Psycho - Dry Riff', 'Muse', 'High Gain',
      'Dry punchy riff distortion with modest gain, tight bass and firm palm-muted attacks.',
      ['Tight Gate', 'Muse / Psycho Dry Riff Amp', 'Cabinet Unity', 'Post-Cab Tight'],
      cab='tight-modern-4x12', amp='Modern Tight', pickups='Bridge humbucker; articulate the riff and set the gate below quiet intended notes.',
      tuning='Drop D; retune guitar physically',
      notes='No added delay/reverb, preserving the riff attack. Adjust amp Gain before Output to suit your pickup.',
      collection='Muse expansion', output=-12)
sound('muse.reapers.tapping', 'Reapers - Articulate Tapping', 'Muse', 'High Gain',
      'Controlled hot-rod gain and light compression for even tapping attacks without an echo wash.',
      ['Song / Gentle Pick Compression', 'Muse / Reapers Tapping Amp', 'Cabinet Unity',
       'Post-Cab Tight', 'Song / Muse Lead Plate'],
      cab='tight-modern-4x12', amp='British Hot-Rod', pickups='Bridge humbucker; use even tapping/legato dynamics and mute unused strings.',
      tuning='Match the guitar part; retune physically when needed',
      notes='Tapping-intro guitar texture. The compressor does not generate tapped notes; avoid a strong gate that cuts off legato.',
      collection='Muse expansion', output=-12)
sound('muse.reapers.lead', 'Reapers - Singing Lead', 'Muse', 'High Gain',
      'Forward sustained lead with a moderate manual wah and filtered stereo echoes.',
      ['Lead Decay Gate', 'Muse / Reapers Lead Wah', 'British Hot-Rod Lead', 'Cabinet Unity',
       'Post-Cab Tight', 'Muse / Reapers Lead Echo', 'Song / Muse Lead Plate'],
      cab='british-4x12', amp='British Hot-Rod', pickups='Bridge humbucker; play single-note runs and bends, and adjust wah Position to taste.',
      tuning='Match the guitar part; retune physically when needed',
      notes='Lead starting point. Recorded pitch/pedal gestures are not automated or recreated; this template uses no pitch processor and fits Legacy 128.',
      collection='Muse expansion', output=-12)
sound('muse.defector.rhythm', 'Defector - Broad Distortion', 'Muse', 'High Gain',
      'Broad British saturation, fuller cabinet body and a quiet plate behind the dry guitar.',
      ['Tight Gate', 'Muse / Defector Broad Rhythm Amp', 'Cabinet Unity',
       'Post-Cab Full', 'Song / Muse Lead Plate'],
      cab='dark-full-4x12', amp='British Hot-Rod', pickups='Bridge humbucker; keep chord attacks distinct and reduce bass if the mix gets crowded.',
      tuning='Match the guitar part; retune physically when needed',
      notes='Rhythm/lead guitar texture. Stacked guitar takes and layered vocals in a recording remain separate performances.',
      collection='Muse expansion', output=-12)
sound('muse.globalist.intro', 'The Globalist - Spacious Clean Intro', 'Muse', 'Ambient',
      'Clear sustained introductory guitar surrounded by slow chorus, dark echo and a diffuse hall.',
      ['Song / Gentle Pick Compression', 'Muse / Globalist Intro Clean Amp', 'Cabinet Unity',
       'Song / Alien Lead Focus', 'Song / Slow Space Chorus', 'Muse / Globalist Intro Echo', 'Muse / Globalist Intro Hall'],
      cab='clean-open-2x12', amp='Solid-State Clean', pickups='Neck/middle or split humbucker; sustain sparse notes and use gentle volume swells.',
      notes='Opening clean guitar adaptation. Whistling, orchestral parts and layered slide performances are not generated.',
      collection='Muse expansion', output=-9)
sound('muse.globalist.heavy', 'The Globalist - Heavy Middle', 'Muse', 'High Gain',
      'Heavy middle-section guitar with tight low-end support, broad saturation and no long ambience.',
      ['Tight Gate', 'Metal Tight Boost', 'Muse / Globalist Heavy Amp', 'Cabinet Unity',
       'Post-Cab Tight', 'Gentle Room'],
      cab='dark-full-4x12', amp='Recto Heavy', pickups='Bridge humbucker; palm mute accurately and damp low strings between phrases.',
      tuning='Match the low-register part with physical tuning / appropriate strings',
      notes='Independent heavy-section template; changing to intro/outro is a normal preset load, with the existing transition/tail policy.',
      collection='Muse expansion', output=-12)
sound('muse.globalist.outro', 'The Globalist - Clean Outro Guitar', 'Muse', 'Clean',
      'Rounded dry-forward clean chord accompaniment with light compression and a short room.',
      ['Song / Bell Compression', 'Muse / Globalist Outro Clean Amp', 'Cabinet Unity',
       'Song / Rounded Bell Focus', 'Muse / Globalist Outro Room'],
      cab='clean-open-2x12', amp='US Clean Combo', pickups='Neck pickup; use soft chord voicings and let the clean attacks breathe.',
      notes='Guitar adaptation for the piano-led ending, not a reproduction of the piano tone. Use Spacious Clean Intro for the larger echo/hall texture.',
      collection='Muse expansion', output=-9)

library_path.write_text(json.dumps(library, indent=2) + '\n')
catalog_path.write_text(json.dumps(catalog, indent=2) + '\n')
print(f'Song templates present; {len(library["racks"])} racks / {len(library["pedals"])} voices. Existing records preserved.')
