#!/usr/bin/env python3
"""Seed researched, independent tones additively; JSON remains authoritative.

Stable UUIDs derive from immutable keys, never names or array offsets. Re-running
preserves all existing records, including owner edits to previously seeded tones.
Historical evidence is documented separately from these proposed app settings.
"""
import argparse
import copy
import json
import uuid
from pathlib import Path

NAMESPACE = uuid.UUID('8eb9076d-160c-4947-bc40-bcaed06aa4ab')
COLLECTION = 'Alternative research'


def identity(key):
    return str(uuid.uuid5(NAMESPACE, key))


def seed(root):
    lp, cp = root / 'assets/sounds/library.json', root / 'assets/sounds/catalog.json'
    library, catalog = json.loads(lp.read_text()), json.loads(cp.read_text())
    before_library, before_catalog = copy.deepcopy(library), copy.deepcopy(catalog)
    cabinets = {x['name']: x['id'] for x in json.loads((root / 'assets/ir/catalog.json').read_text())['assets']}
    voices = {p['name']: p['id'] for p in library['pedals']}

    def pedal(key, name, effect, version=1, **parameters):
        key = 'alternative.' + key
        pid = identity('pedal:' + key)
        if not any(p['id'] == pid for p in library['pedals']):
            library['pedals'].append(dict(id=pid, key=key, name=name,
                                         effect_type=effect, effect_version=version,
                                         parameters=parameters))
        if name in voices and voices[name] != pid:
            raise ValueError('Voicing name collision: ' + name)
        voices[name] = pid
        return name

    def amp(key, name, family, gain, bass, mid, treble, *, master=28, output=-9, **extra):
        parameters = dict(gain_db=gain, bass_db=bass, mid_db=mid, treble_db=treble,
                          master_percent=master, output_db=output, **extra)
        return pedal(key, name, family, **parameters)

    def eq(key, name, hp, lp_hz, low, mid, high):
        return pedal(key, name, 'parametric_eq', high_pass_hz=hp, low_pass_hz=lp_hz,
                     band1_hz=low[0], band1_db=low[1], band1_q=.8,
                     band2_hz=mid[0], band2_db=mid[1], band2_q=.8,
                     band3_hz=high[0], band3_db=high[1], band3_q=.9, output_db=0)

    def room(key, name, mode, decay, mix, pre=12):
        return pedal(key, name, 'space_reverb', mode=mode, decay_seconds=decay,
                     pre_delay_ms=pre, damping_hz=4800, tone_hz=6500,
                     mix_percent=mix, output_db=0)

    def echo(key, name, time, mix):
        return pedal(key, name, 'echo', version=2, time_ms=time, feedback_percent=20,
                     mix_percent=mix, mode=1, color_hz=3500, wet_low_cut_hz=220,
                     wow_percent=0, stereo=0, heads=0, output_db=0)

    comp = pedal('comp.pick', 'Research / Natural Pick Compression', 'compressor',
                 threshold_db=-25, ratio=2, attack_ms=24, release_ms=170, makeup_db=2, mix_percent=60)
    gate = pedal('gate.rhythm', 'Research / Rhythm Noise Control', 'noise_gate',
                 threshold_db=-58, range_db=45, attack_ms=1, hold_ms=45, release_ms=140, hysteresis_db=5)
    # A decay-friendly gate is deliberately absent from clean and raw lead paths.
    small = room('room.small', 'Research / Dry Tracking Room', 0, .55, 5)
    acoustic_room = room('room.acoustic', 'Research / Acoustic Air', 0, .9, 8)
    plate = room('room.lead', 'Research / Restrained Lead Plate', 1, 1.35, 8, 18)
    halo = room('room.halo', 'Research / Bowed Layer Hall', 2, 3.2, 22, 28)
    lead_echo = echo('echo.lead', 'Research / Quiet Lead Echo', 280, 8)
    halo_echo = echo('echo.halo', 'Research / Layered Halo Echo', 410, 17)
    short_echo = echo('echo.short', 'Research / Short Melodic Echo', 190, 7)
    chorus_deep = pedal('chorus.caya', 'Nirvana / Deep Mono Water', 'chorus',
                        rate_hz=.75, depth_percent=80, width_percent=0, mix_percent=50)
    chorus_teen = pedal('chorus.teen', 'Nirvana / Watery Clean Motion', 'chorus',
                        rate_hz=.85, depth_percent=65, width_percent=0, mix_percent=45)
    chorus_lead = pedal('chorus.lead', 'Nirvana / Distorted Mono Water', 'chorus',
                        rate_hz=.8, depth_percent=70, width_percent=0, mix_percent=42)
    chorus_mtv = pedal('chorus.mtv', 'Nirvana / Electro-Acoustic Mono Color', 'chorus',
                       rate_hz=.65, depth_percent=42, width_percent=0, mix_percent=28)

    acoustic_eq = eq('eq.acoustic', 'Research / Acoustic Source Balance', 65, 11500, (230,-2), (1300,0), (4200,-1))
    electric_acoustic_eq = eq('eq.adaptation', 'Research / Electric Acoustic Adaptation', 90, 9000, (220,-2), (1700,1), (4000,-2))
    gd_clean_eq = eq('eq.gd.clean', 'Green Day / Picked Clarity', 85, 7800, (250,-1.5), (1800,1), (4200,-1))
    gd_outlaws_eq = eq('eq.gd.outlaws', 'Green Day / Broad Outlaws Body', 75, 6200, (190,.5), (950,1), (3400,-1.5))
    gd_punch_eq = eq('eq.gd.punch', 'Green Day / Revolution Chord Bite', 85, 6700, (240,-1.5), (1500,1.5), (3900,-1))
    gd_sept_eq = eq('eq.gd.september', 'Green Day / September Electric Weight', 80, 6000, (200,-.5), (1000,1), (3500,-2))
    lead_eq = eq('eq.lead', 'Research / Singing Mid Focus', 95, 6500, (240,-1.5), (1300,2), (3900,-1.5))
    sg_weight_eq = eq('eq.sg.weight', 'Soundgarden / Warm Low-D Weight', 55, 5400, (140,1), (700,1.5), (3100,-2))
    sg_bite_eq = eq('eq.sg.bite', 'Soundgarden / Complementary Guitar Bite', 65, 6000, (220,-1), (1600,2), (3600,-2))
    sg_slaves_eq = eq('eq.sg.slaves', 'Soundgarden / Bulldozer Low-Mid Body', 50, 5000, (140,1.5), (650,2), (2800,-1.5))
    nv_clean_eq = eq('eq.nv.clean', 'Nirvana / Dry Clean Body', 65, 7600, (210,-1), (1100,.5), (3800,-1.5))
    nv_water_eq = eq('eq.nv.water', 'Nirvana / Low-String Water Balance', 55, 6300, (180,.5), (1200,.5), (3400,-2))
    nv_heavy_eq = eq('eq.nv.heavy', 'Nirvana / Nevermind Hard-Chord Focus', 70, 6400, (230,-1.5), (2400,1.5), (4300,-2))
    nv_lithium_eq = eq('eq.nv.lithium', 'Nirvana / Dark Lithium Weight', 50, 5000, (150,1), (850,-.5), (2900,-1))
    nv_hsb_eq = eq('eq.nv.hsb', 'Nirvana / In Utero Abrasive Mids', 60, 6900, (230,-1), (1700,2), (4100,-1.5))
    nv_hsb_clean_eq = eq('eq.nv.hsb.clean', 'Nirvana / Dark Drop-Csharp Picking', 55, 6000, (180,.5), (1000,.5), (3500,-2))
    soad_clean_eq = eq('eq.soad.clean', 'System Of A Down / Defined Picking', 65, 8000, (230,-2), (1600,1), (4300,-1.5))
    soad_chop_eq = eq('eq.soad.chop', 'System Of A Down / Chop Low-C Stops', 60, 6400, (170,-2), (1250,1.5), (3600,-2))
    soad_tox_eq = eq('eq.soad.tox', 'System Of A Down / Toxicity Weight And Bite', 55, 6000, (150,-1), (1100,1), (3200,-1.5))
    soad_soldier_eq = eq('eq.soad.soldier', 'System Of A Down / Hypnotize String Attack', 65, 6800, (230,-1.5), (1600,2), (4000,-2))

    gd_clean = amp('amp.gd.clean', 'Green Day / Clear Picked Combo', 'amp_us_combo', 0,-1,0,1, master=18, output=-3, bright_percent=10)
    gd_outlaws = amp('amp.gd.outlaws', 'Green Day / Broad British Crunch', 'amp_brit_crunch', 18,0,2,-1, master=36, presence_percent=30, sag_percent=24)
    gd_revolution = amp('amp.gd.revolution', 'Green Day / Punchy British Rhythm', 'amp_brit_crunch', 21,-2,2,1, master=38, presence_percent=42, sag_percent=18)
    gd_heavy = amp('amp.gd.heavy', 'Green Day / American Heavy Support', 'amp_brit_hotrod', 17,-1,2,0, master=38, presence_percent=30, sag_percent=20, low_cut_hz=85)
    gd_lead = amp('amp.gd.lead', 'Green Day / Singing British Lead', 'amp_brit_hotrod', 20,-2,3,0, master=36, presence_percent=33, sag_percent=22, low_cut_hz=100)
    sg_cornell = amp('amp.sg.cornell', 'Soundgarden / Cornell Role Warm Rhythm', 'amp_brit_crunch', 21,1,2,-3, master=40, presence_percent=20, sag_percent=30)
    sg_thayil = amp('amp.sg.thayil', 'Soundgarden / Thayil Role Cutting Rhythm', 'amp_brit_hotrod', 19,-1,3,-1, master=36, presence_percent=28, sag_percent=20, low_cut_hz=65)
    sg_slaves = amp('amp.sg.slaves', 'Soundgarden / Cornell Role Bulldozer Body', 'amp_recto', 19,1,2,-3, master=38, presence_percent=20, sag_percent=42, resonance_percent=36)
    sg_lead = amp('amp.sg.lead', 'Soundgarden / Thayil Role Raw Lead', 'amp_brit_hotrod', 23,-1,3,-1, master=40, presence_percent=27, sag_percent=26, low_cut_hz=70)
    nv_clean = amp('amp.nv.clean', 'Nirvana / Open Clean Combo', 'amp_us_combo', 0,0,1,0, master=19, output=-3)
    nv_support = amp('amp.nv.support', 'Nirvana / Pedal Driven Combo', 'amp_us_combo', 4,0,1,-1, master=28, output=-6)
    nv_mark = amp('amp.nv.mark', 'Nirvana / Controlled Nevermind Support', 'amp_mark', 5,-3,2,1, master=27, lead_gain_db=5, presence_percent=30, sag_percent=16, output=-8)
    nv_hsb_clean = amp('amp.nv.hsb.clean', 'Nirvana / Dark In Utero Clean', 'amp_solid_clean', 2,0,1,-1, master=20, output=-3, presence_percent=25)
    nv_hsb_drive = amp('amp.nv.hsb.drive', 'Nirvana / In Utero Drive Support', 'amp_solid_clean', 5,-1,2,0, master=25, output=-6, presence_percent=30)
    mtv_combo = amp('amp.nv.mtv', 'Nirvana / Electro-Acoustic Clean Support', 'amp_us_combo', 0,-2,0,-1, master=16, output=-3)
    soad_clean = amp('amp.soad.clean', 'System Of A Down / Clear Picked Amp', 'amp_solid_clean', 0,-1,0,1, master=20, output=-3, presence_percent=30)
    soad_tox = pedal('amp.soad.tox', 'System Of A Down / Toxicity Body And Teeth', 'dual_amp',
                     model_a=7, gain_a_db=22, master_a_percent=34, level_a_db=-2,
                     model_b=3, gain_b_db=19, master_b_percent=35, level_b_db=0,
                     blend_percent=40, bass_db=-1, mid_db=2, treble_db=0, presence_percent=33, output_db=-10)
    soad_chop = amp('amp.soad.chop', 'System Of A Down / Chop Tight Attack', 'amp_modern_tight', 23,-2,2,0, master=32, presence_percent=35, sag_percent=7, low_cut_hz=75, resonance_percent=24, output=-10)
    soad_open = amp('amp.soad.open', 'System Of A Down / Open Toxicity Chords', 'amp_recto', 22,0,2,-1, master=36, presence_percent=28, sag_percent=38, resonance_percent=32, output=-10)
    soad_soldier = amp('amp.soad.soldier', 'System Of A Down / Hypnotize British Drive', 'amp_brit_hotrod', 22,-2,2,1, master=36, presence_percent=38, sag_percent=18, low_cut_hz=75, output=-10)
    soad_lead = amp('amp.soad.lead', 'System Of A Down / Melodic British Lead', 'amp_brit_hotrod', 24,-2,3,0, master=34, presence_percent=32, sag_percent=20, low_cut_hz=90, output=-10)
    nv_hard = pedal('drive.nv.hard', 'Nirvana / Hard Chord Distortion', 'hard_distortion', drive_db=27, low_cut_hz=65, filter_hz=4100, output_db=-10)
    nv_lead_drive = pedal('drive.nv.lead', 'Nirvana / Rough Melodic Distortion', 'hard_distortion', drive_db=30, low_cut_hz=80, filter_hz=3700, output_db=-10)
    nv_caya_drive = pedal('drive.nv.caya', 'Nirvana / Low-String Chorus Drive', 'hard_distortion', drive_db=23, low_cut_hz=50, filter_hz=3500, output_db=-9)
    nv_lithium = pedal('drive.nv.lithium', 'Nirvana / Dark Lithium Sustain Fuzz', 'sustain_fuzz', drive_db=24, compression_percent=48, tone_hz=2800, output_db=-13)
    nv_hsb_dist = pedal('drive.nv.hsb', 'Nirvana / Abrasive In Utero Drive', 'oxford_distortion', drive_db=23, bass_db=-1, contour_percent=22, treble_db=1, output_db=-10)
    mtv_drive = pedal('drive.nv.mtv', 'Nirvana / Restrained Unplugged Color', 'hard_distortion', drive_db=12, low_cut_hz=100, filter_hz=3400, output_db=-5)

    tunings = {
        'outlaws': ('Standard E: E A D G B E', 'T1 transcription-level; studio tuning not independently confirmed.'),
        'revolution': ('Standard E: E A D G B E', 'T2 transcription-level; studio tuning not independently confirmed.'),
        'september': ('Standard E: E A D G B E', 'T3 instructional source.'),
        'outshined': ('Drop D: D A D G B E', 'T4 explicitly matches recording tuning.'),
        'slaves': ('Drop D: D A D G B E', 'T5 transcription-level; studio tuning not independently confirmed.'),
        'hsb': ('Drop C#: C# G# C# F# A# D#', 'T6 instructional source.'),
        'teen': ('Standard E: E A D G B E', 'T7 published score; later live versions can differ.'),
        'caya': ('D standard: D G C F A D', 'T9 explicitly distinguishes album D standard from live Eb.'),
        'lithium': ('D standard: D G C F A D', 'T10 explicitly matches album tuning.'),
        'mtv': ('Eb standard: Eb Ab Db Gb Bb Eb', 'T8 explicitly matches MTV Unplugged.'),
        'bloom': ('Standard E: E A D G B E', 'T11 instructional score.'),
        'chop': ('Drop C: C G C F A D', 'T12 song lesson, D2 previous-era tuning.'),
        'tox': ('Drop C: C G C F A D', 'T13 explicitly specifies song tuning.'),
        'soldier': ('Drop C#: C# G# C# F# A# D#', 'D2 artist session tuning, T14 song score.')}
    evidence = {'outlaws':'G1', 'revolution':'G1', 'september':'G2', 'outshined':'S1', 'slaves':'S1/S2',
                'hsb':'N5', 'teen':'N1/N2', 'caya':'N1/N3/N4', 'lithium':'N1', 'mtv':'N1/N6',
                'bloom':'N1', 'chop':'D1', 'tox':'D1', 'soldier':'D2'}

    def sound(song, role, artist, title, gain, character, recipe, *, cab=None, pickups, notes, output=-9):
        key = f'alternative.{song}.{role}'
        sid = identity('sound:' + key)
        if len(recipe) > 16 or (cab is not None) != ('Cabinet Unity' in recipe):
            raise ValueError('Invalid recipe/cabinet: ' + key)
        if not any(r['id'] == sid for r in library['racks']):
            library['racks'].append(dict(id=sid, key=key, name=artist+' / '+title,
                                        output_db=OUTPUT_LEVELS.get(key, output), pedals=recipe,
                                        pedal_ids=[voices[p] for p in recipe],
                                        instance_ids=[identity(f'instance:{key}:{slot}') for slot in range(len(recipe))],
                                        cabinet_assets={str(i):cabinets[cab] for i,p in enumerate(recipe) if p=='Cabinet Unity'}))
        if not any(m['id'] == sid for m in catalog['sounds']):
            amp_names = [p for p in recipe if next(v for v in library['pedals'] if v['id']==voices[p])['effect_type'].startswith('amp_') or next(v for v in library['pedals'] if v['id']==voices[p])['effect_type']=='dual_amp']
            tuning, support = tunings[song]
            catalog['sounds'].append(dict(
                id=sid, artist=artist, style='Alternative Rock' if artist=='Green Day' else 'Alternative Metal' if artist=='System Of A Down' else 'Grunge',
                gain=gain, character=character, tags=[artist,title,gain,'Song templates',COLLECTION,song,role,key],
                tuning=tuning+'; physical tuning, no pitch processor. '+support,
                pickups=pickups,
                input='Instrument input for electric; appropriate input for acoustic pickup/microphone. Start trim 0 dB and dry peaks -18 to -9 dBFS; match pickup level.',
                amp=(', '.join(amp_names)+'; original approximation, not recording equipment') if amp_names else 'No amp model; source-preserving acoustic EQ path',
                cabinet=(cab+'; original synthetic response, not a measured recording cabinet') if cab else 'No electric cabinet: preserve the acoustic source body',
                notes='Proposed app settings, not historical knob positions. '+notes,
                evidence=f'Recording/session sources {evidence[song]} and tuning evidence in docs/alternative-tone-research.md. Individual overdub settings and album match unverified.',
                listening='Listening pending'))

    hb = 'Bridge P-90 or clear humbucker; controlled bass and firm chord attack.'
    clear = 'Clear neck/middle pickup or split humbucker; gentle picked notes and full decay.'
    nv_hb = 'Bridge humbucker; keep chord attack and string noise rather than maximizing saturation.'
    sg_hb = 'Bridge humbucker; physical Drop D, relaxed heavy riffing. Player-role settings are proposed.'
    soad_hb = 'Bridge humbucker; accurate low-string palm muting, firm pick attack and clean stops.'
    c = 'Cabinet Unity'
    bc, oc, dc, tc = 'british-4x12', 'clean-open-2x12', 'dark-full-4x12', 'tight-modern-4x12'

    sound('outlaws','clean','Green Day','Outlaws - Clear Clean','Clean','Dry-forward clear guitar for softer playing.',[comp,gd_clean,c,gd_clean_eq,acoustic_room],cab=oc,pickups=clear,notes='An independent clean adaptation; not a claim that guitar fills every quiet passage.',output=0)
    sound('outlaws','rhythm','Green Day','Outlaws - Broad Crunch','Crunch','Broad midrange rock body with preserved chord attack.',[gd_outlaws,c,gd_outlaws_eq,small],cab=bc,pickups=hb,notes='Album amp families guide the color; multiple recorded guitars remain separate performances.')
    sound('outlaws','lead','Green Day','Outlaws - Singing Lead','Distortion','Sustained lead with quiet repeats and controlled highs.',[gd_lead,c,lead_eq,lead_echo,plate],cab=bc,pickups=hb,notes='Lead sustain is shaped independently of rhythm output; echo timing is proposed.')
    sound('outlaws','halo','Green Day','Outlaws - Bowed Layer Adaptation','Ambient','Sustained clean halo for volume-swelling a separate part.',[comp,gd_clean,c,gd_clean_eq,halo_echo,halo],cab=oc,pickups=clear+' Use the guitar volume to soften attacks.',notes='Dugan identifies actual bowed guitar. This picked/swelled adaptation does not simulate bow excitation.',output=0)
    sound('revolution','rhythm','Green Day','Revolution Radio - Punch Rhythm','Crunch','Forward, articulate British rock crunch.',[gd_revolution,c,gd_punch_eq,small],cab=bc,pickups=hb,notes='Moderate saturation retains chord strings; album layering is not synthesized.')
    sound('revolution','lead','Green Day','Revolution Radio - Forward Lead','Distortion','Singing upper mids and short supporting space.',[gd_lead,c,gd_punch_eq,short_echo,plate],cab=bc,pickups=hb,notes='Independent general lead voice, not a claimed single isolated studio track.')
    sound('september','acoustic','Green Day','Wake Me Up When September Ends - Acoustic Source','Clean','Natural acoustic picking with restrained room air.',[comp,acoustic_eq,acoustic_room],pickups='Steel-string acoustic microphone or balanced acoustic pickup; preserve natural body.',notes='Acoustic source required. No electric amp/cab and no acoustic-body synthesis.',output=0)
    sound('september','clean','Green Day','Wake Me Up When September Ends - Electric Clean Adaptation','Clean','Clear electric-guitar adaptation of delicate acoustic picking.',[comp,gd_clean,c,electric_acoustic_eq,acoustic_room],cab=oc,pickups=clear,notes='Explicit electric adaptation; cannot recreate the acoustic instrument body.',output=0)
    sound('september','rhythm','Green Day','Wake Me Up When September Ends - Heavy Rhythm','Distortion','Broad heavy electric chords without metal-style scooping.',[gd_heavy,c,gd_sept_eq,small],cab=bc,pickups=hb,notes='American Idiot-era electric direction, separate from the later Revolution Radio sessions.')
    sound('september','lead','Green Day','Wake Me Up When September Ends - Melodic Lead','Distortion','Focused melodic sustain with quiet delay.',[gd_lead,c,lead_eq,lead_echo,plate],cab=bc,pickups=hb,notes='Use phrasing and bends for melody; delay is a proposed quiet support.')

    sound('outshined','cornell','Soundgarden','Outshined - Cornell Role Rhythm','Distortion','Warm midrange Drop-D foundation.',[sg_cornell,c,sg_weight_eq,small],cab=dc,pickups=sg_hb,notes='Cornell-role approximation; exact recorded amp/pickup/overdub assignment unverified.')
    sound('outshined','thayil','Soundgarden','Outshined - Thayil Role Rhythm','Distortion','Contrasting cutting guitar with controlled treble.',[sg_thayil,c,sg_bite_eq,small],cab=bc,pickups=sg_hb,notes='Play another part for a layered result; a single pickup/chain does not reproduce both guitarists.')
    sound('outshined','lead','Soundgarden','Outshined - Thayil Lead','Distortion','Raw midrange lead that retains bends and irregular attack.',[sg_lead,c,lead_eq,plate],cab=bc,pickups=sg_hb,notes='Targets the album guitar solo; video/radio edits may omit it. No mandatory wah.')
    sound('slaves','cornell','Soundgarden','Slaves and Bulldozers - Cornell Role Weight','Distortion','Low-D weight with slow low-end bloom.',[sg_slaves,c,sg_slaves_eq,small],cab=dc,pickups=sg_hb,notes='Warm rhythm-role design, not an authenticated Rectifier studio claim. Let notes breathe.')
    sound('slaves','thayil','Soundgarden','Slaves and Bulldozers - Thayil Role Bite','Distortion','Contrasting upper mids for the second riffing guitar.',[sg_thayil,c,sg_bite_eq,small],cab=bc,pickups=sg_hb,notes='Independent complementary tone; no stereo-doubling trick replaces another performance.')
    sound('slaves','lead','Soundgarden','Slaves and Bulldozers - Raw Lead','Distortion','Loose sustained lead with restrained room decay.',[sg_lead,c,sg_bite_eq,plate],cab=bc,pickups=sg_hb,notes='Blowing-on-strings/feedback gestures are performance techniques; this rack does not generate them.')

    sound('hsb','clean','Nirvana','Heart-Shaped Box - Dark Clean','Clean','Dark but defined low-tuned arpeggios.',[nv_hsb_clean,c,nv_hsb_clean_eq,small],cab=oc,pickups=clear,notes='In Utero target; no automatic Small Clone borrowed from Nevermind.',output=0)
    sound('hsb','rhythm','Nirvana','Heart-Shaped Box - Abrasive Rhythm','Distortion','Pedal-driven abrasive mids with clean amp support.',[nv_hsb_dist,nv_hsb_drive,c,nv_hsb_eq,small],cab=oc,pickups=nv_hb,notes='Oxford topology is a proposed drive surrogate, not a SansAmp circuit reproduction.')
    sound('hsb','lead','Nirvana','Heart-Shaped Box - Focused Lead','Distortion','Dry-forward bent-note sustain without automatic experimental modulation.',[nv_lead_drive,nv_hsb_drive,c,nv_hsb_eq,plate],cab=oc,pickups=nv_hb,notes='Commercial album lead starting point; Pedal X original-session sound is not assumed to survive this mix.')
    sound('teen','clean','Nirvana','Smells Like Teen Spirit - Watery Clean','Clean','Mono watery clean with strong dynamic contrast.',[chorus_teen,nv_clean,c,nv_clean_eq,small],cab=oc,pickups=clear,notes='Small Clone behavior inspires the mono blend; this generic chorus is not that circuit.',output=0)
    sound('teen','rhythm','Nirvana','Smells Like Teen Spirit - Explosive Rhythm','Distortion','Hard-clipped chord attack with controlled amp saturation.',[nv_hard,nv_mark,c,nv_heavy_eq,small],cab=bc,pickups=nv_hb,notes='Vig recollections differ in pedal specificity. Op-Amp Hard Distortion is neither a verified DS-1 nor RAT model.')
    sound('teen','lead','Nirvana','Smells Like Teen Spirit - Melodic Lead','Distortion','Rough melodic lead with mild mono movement.',[nv_lead_drive,chorus_lead,nv_mark,c,lead_eq,plate],cab=bc,pickups=nv_hb,notes='Chorus use on this proposed lead remains a listening hypothesis; no historical knob settings claimed.')
    sound('caya','clean','Nirvana','Come as You Are - Deep Mono Chorus','Clean','Deep watery low-string motion, near-even wet/dry blend.',[chorus_deep,nv_clean,c,nv_water_eq,small],cab=oc,pickups='Clear neck or articulate bridge pickup; physically D standard, not Drop D.',notes='Start 0.75 Hz, 80% depth, 50% mix, width 0. Original mono/BBD behavior is only approximated.',output=0)
    sound('caya','rhythm','Nirvana','Come as You Are - Chorus Drive','Distortion','Hard drive into deep mono modulation with dark lower strings.',[nv_caya_drive,chorus_deep,nv_support,c,nv_water_eq,small],cab=oc,pickups=nv_hb,notes='Distortion precedes chorus and amp; mix/rate are proposed. No live tuning substitution.')
    sound('caya','lead','Nirvana','Come as You Are - Watery Lead','Distortion','Sustaining mono-chorused lead with clear bends.',[nv_lead_drive,chorus_lead,nv_support,c,lead_eq,plate],cab=oc,pickups=nv_hb,notes='Sustain/bends and open-string interactions come from playing; extra ambience stays restrained.')
    sound('lithium','clean','Nirvana','Lithium - Dry Clean','Clean','Dry and open whole-step-down chord picking.',[nv_clean,c,nv_clean_eq,small],cab=oc,pickups=clear,notes='No chorus inserted without evidence. D standard is required for album voicings.',output=0)
    sound('lithium','fuzz','Nirvana','Lithium - Dark Sustain Fuzz','Fuzz','Dark thumping fuzz body through a pedal-friendly combo.',[nv_lithium,nv_support,c,nv_lithium_eq,small],cab=dc,pickups=nv_hb,notes='Vig identifies Big Muff/Bassman direction; these original algorithms are approximations.')
    sound('mtv','acoustic','Nirvana','The Man Who Sold the World MTV Unplugged - Acoustic Source','Clean','Acoustic body and pick decay with light room support.',[acoustic_eq,acoustic_room],pickups='Electro-acoustic or microphone source; an acoustic pickup closest to the performance is preferred.',notes='Clean source option for the acoustic texture, not the complete amplified Cobain sound or Pat Smear part.',output=0)
    sound('mtv','lead','Nirvana','The Man Who Sold the World MTV Unplugged - Colored Acoustic Lead','Crunch','Lightly driven, mono-modulated electro-acoustic lead.',[mtv_drive,chorus_mtv,mtv_combo,c,nv_clean_eq,acoustic_room],cab=oc,pickups='Electro-acoustic pickup source; use an appropriate instrument/preamp input and preserve acoustic dynamics.',notes='Twin/effects documented; precise DS-2/chorus switch states and FOH blend unverified. This is an optional colored path, not an exact recording chain.')
    sound('mtv','electric','Nirvana','The Man Who Sold the World MTV Unplugged - Electric Adaptation','Clean','Clearly labeled electric interpretation with mono acoustic-like color.',[comp,chorus_mtv,mtv_combo,c,electric_acoustic_eq,acoustic_room],cab=oc,pickups=clear,notes='Electric-guitar adaptation only. It does not synthesize the Martin acoustic body or the second guitar.',output=0)
    sound('bloom','clean','Nirvana','In Bloom - Dry Clean','Clean','Dry clean body without obligatory chorus.',[nv_clean,c,nv_clean_eq,small],cab=oc,pickups=clear,notes='Clean/heavy contrast is preserved; exact quiet-track recording rig is unverified.',output=0)
    sound('bloom','rhythm','Nirvana','In Bloom - Full Hard Rhythm','Distortion','Broad hard distortion with a slightly darker combo foundation.',[nv_hard,nv_support,c,nv_heavy_eq,small],cab=dc,pickups=nv_hb,notes='Separate from Teen Spirit amp support; recorded width needs independently played overdubs.')
    sound('bloom','lead','Nirvana','In Bloom - Rough Lead','Distortion','Rough lead articulation with a small amount of room.',[nv_lead_drive,nv_support,c,lead_eq,plate],cab=oc,pickups=nv_hb,notes='Lead chorus is not assumed. Adjust sustained bends and pick attack before adding modulation.')

    sound('chop','clean','System Of A Down','Chop Suey! - Picked Clean Adaptation','Clean','Defined clean electric picking for the layered opening texture.',[comp,soad_clean,c,soad_clean_eq,acoustic_room],cab=oc,pickups=clear,notes='Electric adaptation of acoustic/layered picking; it cannot synthesize the acoustic body or simultaneous parts.',output=0)
    sound('chop','rhythm','System Of A Down','Chop Suey! - Tight Rhythm','High Gain','Controlled low-C stops and sharp chord attack.',[gate,soad_chop,c,soad_chop_eq],cab=tc,pickups=soad_hb,notes='Single original tight amp approximation of the era. Gate can be lowered/removed for longer decays.')
    sound('chop','open','System Of A Down','Chop Suey! - Open Heavy Chords','High Gain','Less-gated open heavy body for sustained chords.',[soad_open,c,soad_tox_eq,small],cab=dc,pickups=soad_hb,notes='Independent open alternative; no pedal distortion or obligatory chorus.')
    sound('tox','clean','System Of A Down','Toxicity - Defined Clean','Clean','Clear picked low-C notes with short room air.',[soad_clean,c,soad_clean_eq,small],cab=oc,pickups=clear,notes='Exact clean studio guitar/amp not established by the heavy tracking account.',output=0)
    sound('tox','rhythm','System Of A Down','Toxicity - Body And Bite Rhythm','High Gain','Original Recto/British parallel blend with controlled low-C weight.',[gate,soad_tox,c,soad_tox_eq],cab=dc,pickups=soad_hb,notes='D1 supports Mesa/Marshall combination; this blend has shared EQ and one synthetic cabinet, not two historical miked rigs. Check CPU on hardware.')
    sound('tox','lead','System Of A Down','Toxicity - Articulate Heavy Lead','High Gain','Focused sustaining lead alternative with restrained space.',[soad_open,c,lead_eq,plate],cab=tc,pickups=soad_hb,notes='General playable lead voice from the requested tone; exact isolated lead-track setup unverified.')
    sound('soldier','clean','System Of A Down','Soldier Side - Clear Clean','Clean','Clear clean picking for the full Hypnotize song.',[comp,soad_clean,c,soad_clean_eq,acoustic_room],cab=oc,pickups=clear,notes='Full Hypnotize track, not Mezmerize Intro; specific clean amp assignment is unverified.',output=0)
    sound('soldier','rhythm','System Of A Down','Soldier Side - British Heavy Rhythm','High Gain','Brighter British saturation with low-Csharp string definition.',[soad_soldier,c,soad_soldier_eq,small],cab=bc,pickups=soad_hb,notes='D2 describes SG/JMP and higher tuning; this original Hot-Rod model does not reproduce the unusual tracking room.')
    sound('soldier','lead','System Of A Down','Soldier Side - Singing Lead','Distortion','Melodic British sustain with quiet repeats and plate.',[soad_lead,c,lead_eq,lead_echo,plate],cab=bc,pickups=soad_hb,notes='Separate melodic voice; harmony overdubs require another performance, not added gain.')

    # Guard preservation before writing either authoritative JSON file.
    for kind in ('pedals', 'racks'):
        current = {x['id']:x for x in library[kind]}
        assert len(current) == len(library[kind]), 'Duplicate library UUID'
        assert all(current[x['id']] == x for x in before_library[kind]), 'Existing recipe changed'
    current = {x['id']:x for x in catalog['sounds']}
    assert len(current) == len(catalog['sounds']), 'Duplicate catalog UUID'
    assert all(current[x['id']] == x for x in before_catalog['sounds']), 'Existing metadata changed'
    for path, data, before in ((lp,library,before_library),(cp,catalog,before_catalog)):
        if data != before:
            path.write_text(json.dumps(data, indent=2, ensure_ascii=False)+'\n')
    return len(library['racks'])-len(before_library['racks']), len(library['pedals'])-len(before_library['pedals'])


# Populated from the existing original-phrase audit; not perceived-level or album matching.
OUTPUT_LEVELS = {'alternative.outlaws.clean': 4.0,
 'alternative.outlaws.rhythm': -10.5,
 'alternative.outlaws.lead': -11.0,
 'alternative.outlaws.halo': 6,
 'alternative.revolution.rhythm': -10.0,
 'alternative.revolution.lead': -11.0,
 'alternative.september.acoustic': -2.5,
 'alternative.september.clean': 4.5,
 'alternative.september.rhythm': -11.5,
 'alternative.september.lead': -11.0,
 'alternative.outshined.cornell': -14.5,
 'alternative.outshined.thayil': -12.0,
 'alternative.outshined.lead': -12.5,
 'alternative.slaves.cornell': -18.5,
 'alternative.slaves.thayil': -12.0,
 'alternative.slaves.lead': -13.0,
 'alternative.hsb.clean': -3.0,
 'alternative.hsb.rhythm': -15.0,
 'alternative.hsb.lead': -16.0,
 'alternative.teen.clean': 6,
 'alternative.teen.rhythm': -4.5,
 'alternative.teen.lead': -2.5,
 'alternative.caya.clean': 5.0,
 'alternative.caya.rhythm': -9.5,
 'alternative.caya.lead': -9.0,
 'alternative.lithium.clean': 3.5,
 'alternative.lithium.fuzz': -13.0,
 'alternative.mtv.acoustic': -1.5,
 'alternative.mtv.lead': 1.0,
 'alternative.mtv.electric': 6,
 'alternative.bloom.clean': 3.5,
 'alternative.bloom.rhythm': -11.0,
 'alternative.bloom.lead': -11.5,
 'alternative.chop.clean': 0.5,
 'alternative.chop.rhythm': -12.0,
 'alternative.chop.open': -16.0,
 'alternative.tox.clean': 1.5,
 'alternative.tox.rhythm': -12.5,
 'alternative.tox.lead': -10.5,
 'alternative.soldier.clean': 0.5,
 'alternative.soldier.rhythm': -11.5,
 'alternative.soldier.lead': -11.5}

if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--root', type=Path, default=Path(__file__).resolve().parents[1])
    args = parser.parse_args()
    racks, pedals = seed(args.root)
    print(f'Added {racks} racks and {pedals} pedal voicings; existing JSON retained.')
