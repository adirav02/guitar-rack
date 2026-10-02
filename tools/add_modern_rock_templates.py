#!/usr/bin/env python3
"""Add researched song roles without replacing any existing factory record.

JSON remains authoritative. UUID keys are immutable, and repeated seeding retains
owner edits. Historical claims and limitations live in modern-rock-tone-research.md.
"""
import argparse
import copy
import json
import uuid
from pathlib import Path

NAMESPACE = uuid.UUID('8eb9076d-160c-4947-bc40-bcaed06aa4ab')
COLLECTION = 'Modern rock research'


def identity(key):
    return str(uuid.uuid5(NAMESPACE, key))


def seed(root):
    lp, cp = root/'assets/sounds/library.json', root/'assets/sounds/catalog.json'
    library, catalog = json.loads(lp.read_text()), json.loads(cp.read_text())
    before_library, before_catalog = copy.deepcopy(library), copy.deepcopy(catalog)
    cabinets = {a['name']:a['id'] for a in json.loads((root/'assets/ir/catalog.json').read_text())['assets']}
    voices = {p['name']:p['id'] for p in library['pedals']}

    def pedal(key, name, effect, version=1, **parameters):
        key = 'modernrock.'+key
        pid = identity('pedal:'+key)
        if not any(p['id']==pid for p in library['pedals']):
            library['pedals'].append(dict(id=pid,key=key,name=name,effect_type=effect,
                                         effect_version=version,parameters=parameters))
        if name in voices and voices[name]!=pid:
            raise ValueError('Voicing name collision: '+name)
        voices[name]=pid
        return name

    def amp(key, name, effect, gain, bass, mid, treble, master=30, output=-9, **extra):
        return pedal(key,name,effect,gain_db=gain,bass_db=bass,mid_db=mid,treble_db=treble,
                     master_percent=master,output_db=output,**extra)

    def eq(key, name, hp, lp_hz, low, mid, high):
        return pedal(key,name,'parametric_eq',high_pass_hz=hp,low_pass_hz=lp_hz,
                     band1_hz=low[0],band1_db=low[1],band1_q=.8,
                     band2_hz=mid[0],band2_db=mid[1],band2_q=.8,
                     band3_hz=high[0],band3_db=high[1],band3_q=.9,output_db=0)

    def echo(key, name, time, mix, feedback=22):
        return pedal(key,name,'echo',version=2,time_ms=time,feedback_percent=feedback,
                     mix_percent=mix,mode=1,color_hz=3800,wet_low_cut_hz=240,
                     wow_percent=0,stereo=0,heads=0,output_db=0)

    # Reuse protected existing voicings by UUID rather than duplicating generic tools.
    def existing(name):
        if name not in voices:
            raise ValueError('Missing prerequisite factory voicing: '+name)
        return name

    comp=existing('Research / Natural Pick Compression')
    gate=existing('Research / Rhythm Noise Control')
    room=existing('Research / Dry Tracking Room')
    air=existing('Research / Acoustic Air')
    plate=existing('Research / Restrained Lead Plate')
    cabinet=existing('Cabinet Unity')
    short=echo('echo.short','Modern Rock / Short Melodic Repeat',210,7)
    kennedy_echo=echo('echo.kennedy','Alter Bridge / Expressive Lead Repeat',390,12,24)
    tremonti_echo=echo('echo.tremonti','Alter Bridge / Articulate Lead Repeat',260,8)
    mix_comp=pedal('comp.mix','Foo Fighters / Light Mix Punch','compressor',
                   threshold_db=-19,ratio=1.6,attack_ms=28,release_ms=125,makeup_db=0,mix_percent=40)

    ff_ring=amp('amp.ff.ring','Foo Fighters / Ringing American Crunch','amp_us_combo',
                12,-1,2,0,master=30,output=-7,presence_percent=30,sag_percent=22)
    ff_pair=pedal('amp.ff.pair','Foo Fighters / Complementary Body And Edge','dual_amp',
                  model_a=1,gain_a_db=9,master_a_percent=28,level_a_db=0,
                  model_b=7,gain_b_db=16,master_b_percent=30,level_b_db=-3,
                  blend_percent=44,bass_db=-1,mid_db=2,treble_db=0,presence_percent=32,output_db=-10)
    ff_clean=amp('amp.ff.clean','Foo Fighters / Clear American Picking','amp_us_combo',
                 1,-1,1,0,master=19,output=-3,bright_percent=12)
    ff_break=amp('amp.ff.break','Foo Fighters / Reduced Drive Detail','amp_us_combo',
                 6,-2,1,-1,master=24,output=-6,presence_percent=25)
    ff_walk=amp('amp.ff.walk','Foo Fighters / Garage American Drive','amp_us_combo',
                17,-1,2,1,master=37,output=-9,presence_percent=36,sag_percent=22)
    ff_chime=pedal('amp.ff.chime','Foo Fighters / Upper Chime Support','amp_brit_chime',
                   gain_db=16,bass_db=-2,treble_db=1,master_percent=28,cut_percent=34,sag_percent=18,output_db=-9)
    ff_accent=amp('amp.ff.accent','Foo Fighters / Cutting Melodic Drive','amp_brit_crunch',
                  19,-3,3,0,master=30,output=-9,presence_percent=34,sag_percent=15)
    ff_pretender=amp('amp.ff.pretender','Foo Fighters / Dry British Attack','amp_brit_hotrod',
                     18,-2,2,1,master=32,output=-9,low_cut_hz=85,presence_percent=36,sag_percent=15)
    ff_open=amp('amp.ff.open','Foo Fighters / Open British Chords','amp_brit_crunch',
                21,-1,2,0,master=36,output=-9,presence_percent=32,sag_percent=24)

    ff_ring_eq=eq('eq.ff.ring','Foo Fighters / Low-D Chord Separation',65,6800,(210,-1.5),(1400,1),(3900,-1.5))
    ff_body_eq=eq('eq.ff.body','Foo Fighters / Firm Chord Body',75,6300,(200,-.5),(1100,1),(3600,-1.5))
    ff_upper_eq=eq('eq.ff.upper','Foo Fighters / Upper Part Definition',120,6800,(260,-2),(1800,2),(4300,-1.5))
    ff_clean_eq=eq('eq.ff.clean','Foo Fighters / Picked Clean Balance',80,8100,(240,-1.5),(1500,.5),(4200,-1))
    ff_dark_eq=eq('eq.ff.dark','Foo Fighters / Dark Intro Balance',70,6500,(220,-.5),(1300,.5),(3900,-2))
    ff_walk_eq=eq('eq.ff.walk','Foo Fighters / Garage Mid Body',85,6400,(220,-1),(950,1.5),(3600,-1.5))
    ff_stop_eq=eq('eq.ff.stop','Foo Fighters / Staccato Low-End Control',90,6500,(180,-2),(1700,1.5),(4100,-1.5))

    sd_madness=amp('amp.sd.madness','Shinedown / Madness Riff Definition','amp_modern_tight',
                   21,-1,2,0,master=33,output=-10,low_cut_hz=70,presence_percent=32,sag_percent=10,resonance_percent=25)
    sd_mad_open=amp('amp.sd.madopen','Shinedown / Madness Open Chords','amp_recto',
                    20,0,2,-1,master=36,output=-10,presence_percent=27,sag_percent=30,resonance_percent=30)
    sd_lead=amp('amp.sd.lead','Shinedown / Forward Melodic Sustain','amp_brit_hotrod',
                23,-2,3,0,master=34,output=-10,low_cut_hz=95,presence_percent=30,sag_percent=20)
    sd_clean=amp('amp.sd.clean','Shinedown / Sparse Verse Clarity','amp_solid_clean',
                 2,-1,1,0,master=20,output=-4,presence_percent=28)
    sd_monster=amp('amp.sd.monster','Shinedown / Wiry D-Standard Riff','amp_brit_hotrod',
                   20,-2,2,1,master=32,output=-10,low_cut_hz=65,presence_percent=37,sag_percent=14)
    sd_monster_open=amp('amp.sd.monsteropen','Shinedown / Monsters Full Chords','amp_modern_tight',
                        22,-1,2,0,master=34,output=-10,low_cut_hz=60,presence_percent=32,sag_percent=12,resonance_percent=28)
    sd_combo=amp('amp.sd.combo','Shinedown / Rough Small-Combo Support','amp_us_combo',
                 5,-2,2,-1,master=28,output=-8,presence_percent=24,sag_percent=25)
    sd_cord_open=amp('amp.sd.cordopen','Shinedown / Open Combo Drive','amp_us_combo',
                     18,-1,2,0,master=38,output=-9,presence_percent=30,sag_percent=30)
    sd_fuzz=pedal('drive.sd.cord','Shinedown / Rough Combo Fuzz Adaptation','sustain_fuzz',
                  drive_db=20,compression_percent=32,tone_hz=3500,output_db=-12)
    sd_dev=amp('amp.sd.devil','Shinedown / Devil Dry Stop Attack','amp_modern_tight',
               22,-2,2,0,master=32,output=-10,low_cut_hz=75,presence_percent=36,sag_percent=8,resonance_percent=24)
    sd_dev_open=amp('amp.sd.devilopen','Shinedown / Devil Broad Chords','amp_recto',
                    21,-1,2,-1,master=35,output=-10,presence_percent=28,sag_percent=28,resonance_percent=30)
    sd_texture=pedal('drive.sd.texture','Shinedown / Filtered Texture Drive','oxford_distortion',
                     drive_db=19,bass_db=-2,contour_percent=18,treble_db=-1,output_db=-10)
    filter_wah=pedal('wah.sd.color','Shinedown / Stationary Lead Color Adaptation','wah',
                     position_percent=48,heel_hz=350,toe_hz=1900,q=1.2,mix_percent=58,output_db=-1)
    sd_mad_eq=eq('eq.sd.madness','Shinedown / Madness Low-D Control',60,6400,(180,-1.5),(1200,1.5),(3700,-2))
    sd_monster_eq=eq('eq.sd.monster','Shinedown / Monsters Low-String Balance',55,6700,(170,-1.5),(1600,1.5),(4200,-1.5))
    sd_cord_eq=eq('eq.sd.cord','Shinedown / Combo Fuzz Definition',75,5800,(200,-2),(1150,1.5),(3400,-1.5))
    sd_dev_eq=eq('eq.sd.devil','Shinedown / Devil Riff Space',65,6200,(160,-2),(1200,2),(3800,-2))
    sd_texture_eq=eq('eq.sd.texture','Shinedown / Narrow Texture Adaptation',220,4200,(280,-2),(1100,3),(3400,-2))
    lead_eq=eq('eq.lead','Modern Rock / Lead Mid Separation',95,6500,(240,-1.5),(1500,2),(3900,-1.5))

    ab_clean=amp('amp.ab.clean','Alter Bridge / Fingerpicked Clean Body','amp_us_combo',
                 0,-1,1,-1,master=19,output=-3,bright_percent=5)
    ab_rhythm=pedal('amp.ab.rhythm','Alter Bridge / Complementary Heavy Body','dual_amp',
                    model_a=7,gain_a_db=18,master_a_percent=33,level_a_db=0,
                    model_b=6,gain_b_db=16,master_b_percent=31,level_b_db=-2,
                    blend_percent=38,bass_db=0,mid_db=2,treble_db=-1,presence_percent=28,output_db=-10)
    ab_support=amp('amp.ab.support','Alter Bridge / Supporting Mark-Family Rhythm','amp_mark',
                   13,-2,2,0,master=30,output=-10,lead_gain_db=4,presence_percent=30,sag_percent=16)
    ab_kennedy=amp('amp.ab.kennedy','Alter Bridge / Expressive Mark-Family Lead','amp_mark',
                   18,-3,3,-1,master=32,output=-10,lead_gain_db=8,presence_percent=27,sag_percent=20)
    ab_tremonti=amp('amp.ab.tremonti','Alter Bridge / Articulate Hot-Rod Lead','amp_brit_hotrod',
                    22,-2,3,0,master=33,output=-10,low_cut_hz=100,presence_percent=33,sag_percent=18)
    ab_clean_eq=eq('eq.ab.clean','Alter Bridge / Fingerstyle String Balance',70,7800,(220,-1),(1400,.5),(4300,-1.5))
    ab_body_eq=eq('eq.ab.body','Alter Bridge / Heavy Chord Separation',60,5900,(150,-1),(950,1),(3300,-2))
    ab_support_eq=eq('eq.ab.support','Alter Bridge / Complementary Upper Weight',85,6500,(220,-2),(1700,1.5),(4000,-1.5))
    ab_kennedy_eq=eq('eq.ab.kennedy','Alter Bridge / Warm Expressive Lead Mids',95,5800,(250,-1),(1100,2),(3600,-2))
    ab_tremonti_eq=eq('eq.ab.tremonti','Alter Bridge / Fast Lead Note Definition',110,6500,(240,-2),(1900,2),(4300,-1.5))

    tunings={
        'everlong':('Drop D: D A D G B E','T1 original lesson/publisher score.'),
        'walk':('Standard E: E A D G B E','T2 teaching arrangement; exact auxiliary baritone tuning unresolved.'),
        'hero':('Standard E: E A D G B E','T3 original lesson explicitly specifies tuning.'),
        'pretender':('Standard E: E A D G B E','T4b original teacher/score; generic Drop-D lists rejected.'),
        'madness':('Drop D: D A D G B E','T5 original lesson description; studio tuning confirmation pending.'),
        'monsters':('D standard: D G C F A D','S6 artist explicitly states D standard, T6 published score agrees.'),
        'cord':('Drop D: D A D G B E','T7 published guitar arrangement; studio confirmation pending.'),
        'devil':('Drop D: D A D G B E','T8 original cover/transcription; studio tuning not independently verified.'),
        'blackbird':('Eb standard: Eb Ab Db Gb Bb Eb','T9 unified full-song arrangement; alternate Kennedy Drop C# reported, studio tuning not verified.')}
    evidence={'everlong':'F1/F2','walk':'F4/F5','hero':'F1/F2','pretender':'F3/T4',
              'madness':'S1/S4','monsters':'S1/S5/S6','cord':'S2/S3','devil':'S5','blackbird':'A1/A2/A3'}
    clear='Clear neck/middle pickup or split humbucker; gentle pick/finger attack and full decay.'
    ff_pick='Clear bridge humbucker; controlled low strings, firm rhythm and audible inner chord notes.'
    sd_pick='Bridge humbucker; accurate palm muting and clean stops. Studio pickup assignment unresolved.'
    monster_pick='Clear bridge pickup or single coil; adjust input trim for output. Silver Sky evidence is later live, not a studio claim.'
    ab_pick='Bridge humbucker; moderate gain, precise chord voicing and independent second performance.'
    bc,oc,dc,tc='british-4x12','clean-open-2x12','dark-full-4x12','tight-modern-4x12'

    def sound(song,role,artist,title,gain,character,recipe,cab,pickups,notes,output=-9):
        key='modernrock.'+song+'.'+role;sid=identity('sound:'+key)
        if len(recipe)>16 or recipe.count(cabinet)!=1:
            raise ValueError('Invalid recipe: '+key)
        if not any(r['id']==sid for r in library['racks']):
            library['racks'].append(dict(id=sid,key=key,name=artist+' / '+title,
                output_db=OUTPUT_LEVELS.get(key,output),pedals=recipe,pedal_ids=[voices[p] for p in recipe],
                instance_ids=[identity(f'instance:{key}:{i}') for i in range(len(recipe))],
                cabinet_assets={str(recipe.index(cabinet)):cabinets[cab]}))
        if not any(m['id']==sid for m in catalog['sounds']):
            amp_names=[p for p in recipe if next(v for v in library['pedals'] if v['id']==voices[p])['effect_type'].startswith('amp_') or next(v for v in library['pedals'] if v['id']==voices[p])['effect_type']=='dual_amp']
            tuning,support=tunings[song]
            catalog['sounds'].append(dict(id=sid,artist=artist,style='Alternative Rock' if artist=='Foo Fighters' else 'Hard Rock',
                gain=gain,character=character,tags=[artist,title,gain,'Song templates',COLLECTION,song,role,key],
                tuning=tuning+'; physical tuning, no pitch processor. '+support,pickups=pickups,
                input='Instrument input; start trim 0 dB, dry peaks -18 to -9 dBFS. Match pickup level before adding gain.',
                amp=', '.join(amp_names)+'; original approximations, not authenticated recording equipment',
                cabinet=cab+'; original synthetic filter, not a measured recording cabinet',
                notes='Proposed app settings, not historical knob positions. '+notes,
                evidence='Sources '+evidence[song]+' and tuning evidence in docs/modern-rock-tone-research.md; exact overdub settings and album match unverified.',
                listening='Listening pending'))

    blend_note='Shared-EQ parallel model families and one synthetic cab; not two miked historical rigs. Check live CPU/graph fades.'
    sound('everlong','intro','Foo Fighters','Everlong - Ringing Intro Crunch','Crunch','Low-D add-nine clarity with responsive electric crunch.',[ff_ring,cabinet,ff_ring_eq,room],oc,ff_pick,'Album electric intro target, not a solo acoustic performance.')
    sound('everlong','rhythm','Foo Fighters','Everlong - Full Chord Rhythm','Distortion','Complementary American body/edge with moderate saturation.',[ff_pair,cabinet,ff_ring_eq,room],bc,ff_pick,blend_note)
    sound('everlong','upper','Foo Fighters','Everlong - Upper Part Detail','Crunch','Less low end for complementary upper-string parts.',[ff_accent,cabinet,ff_upper_eq,room],bc,ff_pick,'Independent overdub/upper role; more gain cannot reproduce a second take.')
    sound('everlong','breakdown','Foo Fighters','Everlong - Clean Breakdown Adaptation','Clean','Reduced-drive alternative for the quieter passage.',[ff_break,cabinet,ff_dark_eq,air],oc,clear,'Playable reduced-drive adaptation, not a verified separate clean studio rig.',output=0)
    sound('walk','clean','Foo Fighters','Walk - Picked Intro Clean','Clean','Clear ringing picked notes with restrained air.',[comp,ff_clean,cabinet,ff_clean_eq,air],oc,clear,'F4 studio family context; exact Walk clean guitar/amp pairing unverified.',output=0)
    sound('walk','rhythm','Foo Fighters','Walk - Driving Rhythm','Distortion','Responsive American garage drive with chord articulation.',[ff_walk,cabinet,ff_walk_eq,room],bc,ff_pick,'No fake baritone or automatic shifting; the layered album needs separate instruments/performances.')
    sound('walk','upper','Foo Fighters','Walk - Upper Chime Support','Crunch','Chime-family counterpart with trimmed bass.',[ff_chime,cabinet,ff_upper_eq,room],oc,ff_pick,'Shiflett album-level AC30 context, not an isolated Walk track assignment.')
    sound('walk','lead','Foo Fighters','Walk - Melodic Accent Lead','Distortion','Forward melodic notes with quiet short repeats.',[ff_accent,cabinet,lead_eq,short,plate],bc,ff_pick,'Independent accent/lead voice; exact studio repeats unverified.')
    sound('hero','rhythm','Foo Fighters','My Hero - Body And Edge Rhythm','Distortion','Firm chord weight with complementary amp-family edges.',[ff_pair,cabinet,ff_body_eq,room],bc,ff_pick,blend_note)
    sound('hero','upper','Foo Fighters','My Hero - Octave Melody Detail','Crunch','Clear upper-register melodic accents.',[ff_accent,cabinet,ff_upper_eq,room],bc,ff_pick,'Perform the octave/melody part; no harmony generator, E-Bow synthesis or artificial doubling.')
    sound('hero','breakdown','Foo Fighters','My Hero - Reduced Drive Breakdown','Crunch','Quieter chord/detail alternative retaining electric body.',[ff_break,cabinet,ff_clean_eq,room],oc,clear,'Reduced-drive arrangement alternative; exact separate studio rig not established.',output=0)
    sound('pretender','clean','Foo Fighters','The Pretender - Dark Clean Intro','Clean','Dark A-minor arpeggio balance with quiet room air.',[comp,ff_clean,cabinet,ff_dark_eq,air],oc,clear,'Standard E, not a generic Drop-D Grohl preset. Specific studio clean amp unresolved.',output=0)
    sound('pretender','staccato','Foo Fighters','The Pretender - Staccato Rhythm','Distortion','Dry picked stops, controlled low end and light mix punch.',[ff_pretender,cabinet,ff_stop_eq,mix_comp,room],bc,ff_pick,'F3 supports mix aggression; this compressor is not an 1176 emulation.')
    sound('pretender','chorus','Foo Fighters','The Pretender - Open Chorus Rhythm','Distortion','Open chord body with clear moving bass notes.',[ff_open,cabinet,ff_body_eq,room],bc,ff_pick,'Arrangement and strumming add weight; studio amp model remains unverified.')
    sound('pretender','counter','Foo Fighters','The Pretender - Cutting Countermelody','Distortion','Separate upper-string melody and bridge-riff detail.',[ff_accent,cabinet,ff_upper_eq,room],bc,ff_pick,'Independent countermelody performance, not an automated extra guitar.')
    sound('madness','rhythm','Shinedown','Sound of Madness - Tight Riff Rhythm','High Gain','Defined low-D riff attacks and dry stops.',[gate,sd_madness,cabinet,sd_mad_eq],tc,sd_pick,'Specific 2008 amp/guitar unresolved; not the later touring Fractal rig.')
    sound('madness','chorus','Shinedown','Sound of Madness - Broad Chorus Chords','High Gain','Broader chord decay with controlled low-D weight.',[sd_mad_open,cabinet,sd_mad_eq,room],dc,sd_pick,'Separate chord voice; exact studio layer assignment unverified.')
    sound('madness','lead','Shinedown','Sound of Madness - Filtered Lead Adaptation','Distortion','Mid-forward lead with restrained stationary filter color.',[filter_wah,sd_lead,cabinet,lead_eq,short,plate],bc,sd_pick,'Fixed wah/EQ is not Whammy transposition. Nick Perri album lead credit prevents automatic Myers attribution.')
    sound('monsters','verse','Shinedown','Monsters - Verse Clarity Adaptation','Clean','Sparse picked alternative with dry low-string clarity.',[comp,sd_clean,cabinet,sd_monster_eq,room],oc,clear,'Application verse alternative, not an authenticated isolated clean studio track.',output=0)
    sound('monsters','riff','Shinedown','Monsters - Wiry Main Riff','High Gain','Cutting D-standard riff with controlled body.',[gate,sd_monster,cabinet,sd_monster_eq],bc,monster_pick,'D standard is artist-supported; later Silver Sky/Axe-Fx evidence does not establish the 2018 studio rig.')
    sound('monsters','chorus','Shinedown','Monsters - Full Chorus Rhythm','High Gain','Fuller low-string chord sustain without losing attack.',[sd_monster_open,cabinet,sd_monster_eq,room],tc,monster_pick,'Separate fuller chorus design, no fake stereo double.')
    sound('monsters','lead','Shinedown','Monsters - Melodic Accent Lead','Distortion','Forward melodic accents with quiet repeats.',[sd_lead,cabinet,lead_eq,short,plate],bc,monster_pick,'General playable accent/lead alternative; exact isolated overdub rig unverified.')
    sound('cord','riff','Shinedown','Cut the Cord - Rough Fuzz Combo','Distortion','Rough combo/fuzz articulation for the picked riff.',[gate,sd_fuzz,sd_combo,cabinet,sd_cord_eq],oc,sd_pick,'S2 small-combo/fuzz album context. Not a Superfuzz/octave-fuzz circuit or the historical eight-inch speaker.')
    sound('cord','chorus','Shinedown','Cut the Cord - Open Combo Chorus','Distortion','More open American-combo chord attack.',[sd_cord_open,cabinet,sd_cord_eq,room],oc,sd_pick,'Specific song amp not established by album inventory; independent chorus alternative.')
    sound('cord','accent','Shinedown','Cut the Cord - Filtered Hook Accent','Distortion','Focused hook/double-stop accent color.',[filter_wah,sd_combo,cabinet,lead_eq,room],oc,sd_pick,'Stationary filter adaptation, not an assertion of a recorded wah solo.')
    sound('devil','rhythm','Shinedown','Devil - Dry Stop Rhythm','High Gain','Controlled low-D attack and precise stops.',[gate,sd_dev,cabinet,sd_dev_eq],tc,sd_pick,'Attention Attention studio setup unresolved; live Fractal inventory is not a studio preset.')
    sound('devil','chorus','Shinedown','Devil - Open Chorus Rhythm','High Gain','Broader chord sustain with quiet room decay.',[sd_dev_open,cabinet,sd_dev_eq,room],dc,sd_pick,'Arrangement alternative; no song timeline, keyboards or backing vocals generated.')
    sound('devil','texture','Shinedown','Devil - Filtered Texture Adaptation','Distortion','Narrow electric texture for a produced-intro alternative.',[sd_texture,sd_combo,cabinet,sd_texture_eq,short],oc,sd_pick,'Explicit electric adaptation; cannot reconstruct synth/vocal/production layers.')
    sound('blackbird','clean','Alter Bridge','Blackbird - Fingerpicked Clean','Clean','Warm clear fingerstyle notes with restrained air.',[comp,ab_clean,cabinet,ab_clean_eq,air],oc,clear,'Use thumb/two fingers and let voices ring. Eb unified arrangement, not an acoustic-guitar assumption.',output=0)
    sound('blackbird','tremonti.rhythm','Alter Bridge','Blackbird - Tremonti Role Rhythm','High Gain','Moderate-gain complementary heavy chord body.',[ab_rhythm,cabinet,ab_body_eq,room],dc,ab_pick,'A1 supports Rectifier/Uberschall pairing. '+blend_note)
    sound('blackbird','kennedy.rhythm','Alter Bridge','Blackbird - Kennedy Role Support','Distortion','Complementary upper mids and reduced low-string weight.',[ab_support,cabinet,ab_support_eq,room],tc,ab_pick,'A2 album Mark/Herbert context; unified Eb arrangement. Player-specific Drop C# reports are not studio-verified.')
    sound('blackbird','kennedy.lead','Alter Bridge','Blackbird - Kennedy Expressive Lead','Distortion','Warm melodic first-solo sustain with quiet longer repeats.',[ab_kennedy,cabinet,ab_kennedy_eq,kennedy_echo,plate],tc,ab_pick,'First solo per A1, Modern Eagle solo instrument per A3; proposed settings. Eb arrangement; alternate Drop C# report unverified.')
    sound('blackbird','tremonti.lead','Alter Bridge','Blackbird - Tremonti Articulate Lead','Distortion','Brighter second-solo attack and note separation.',[ab_tremonti,cabinet,ab_tremonti_eq,tremonti_echo,plate],bc,ab_pick,'A1 studio Ecstasy differs from live Shiva; original hot-rod family is an approximation, not either circuit.')

    for kind in ('pedals','racks'):
        current={x['id']:x for x in library[kind]}
        assert len(current)==len(library[kind]), 'Duplicate library UUID'
        assert all(current[x['id']]==x for x in before_library[kind]), 'Existing recipe changed'
    current={x['id']:x for x in catalog['sounds']}
    assert len(current)==len(catalog['sounds']), 'Duplicate catalog UUID'
    assert all(current[x['id']]==x for x in before_catalog['sounds']), 'Existing metadata changed'
    for path,data,before in ((lp,library,before_library),(cp,catalog,before_catalog)):
        if data!=before:
            path.write_text(json.dumps(data,indent=2,ensure_ascii=False)+'\n')
    return len(library['racks'])-len(before_library['racks']),len(library['pedals'])-len(before_library['pedals'])


# Nominal original-phrase calibration only; not pickup/perceived-level matching.
OUTPUT_LEVELS = {'modernrock.everlong.intro': -6.5,
 'modernrock.everlong.rhythm': -4.0,
 'modernrock.everlong.upper': -8.0,
 'modernrock.everlong.breakdown': -0.5,
 'modernrock.walk.clean': 2.5,
 'modernrock.walk.rhythm': -7.5,
 'modernrock.walk.upper': -8.5,
 'modernrock.walk.lead': -8.0,
 'modernrock.hero.rhythm': -4.5,
 'modernrock.hero.upper': -8.0,
 'modernrock.hero.breakdown': 0.0,
 'modernrock.pretender.clean': 1.5,
 'modernrock.pretender.staccato': -10.5,
 'modernrock.pretender.chorus': -11.0,
 'modernrock.pretender.counter': -8.0,
 'modernrock.madness.rhythm': -12.0,
 'modernrock.madness.chorus': -15.0,
 'modernrock.madness.lead': -8.5,
 'modernrock.monsters.verse': -1.5,
 'modernrock.monsters.riff': -11.5,
 'modernrock.monsters.chorus': -11.5,
 'modernrock.monsters.lead': -11.0,
 'modernrock.cord.riff': -10.0,
 'modernrock.cord.chorus': -9.5,
 'modernrock.cord.accent': 6,
 'modernrock.devil.rhythm': -12.0,
 'modernrock.devil.chorus': -15.0,
 'modernrock.devil.texture': -5.0,
 'modernrock.blackbird.clean': 3.0,
 'modernrock.blackbird.tremonti.rhythm': -12.0,
 'modernrock.blackbird.kennedy.rhythm': 6,
 'modernrock.blackbird.kennedy.lead': -1.5,
 'modernrock.blackbird.tremonti.lead': -10.0}

if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--root',type=Path,default=Path(__file__).resolve().parents[1])
    racks,pedals=seed(parser.parse_args().root)
    print(f'Added {racks} racks and {pedals} voicings; existing records retained.')
