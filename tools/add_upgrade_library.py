#!/usr/bin/env python3
"""Idempotent explicit-ID upgrade recipes. Never rewrites legacy definitions.
IDs use immutable recipe keys, not names/positions. After first creation, edited
records remain authoritative. The ordinary generator does not assign identities.
"""
import json,uuid
from pathlib import Path
root=Path(__file__).resolve().parents[1]
path=root/'assets/sounds/library.json';library=json.loads(path.read_text())
metadata=json.loads((root/'assets/sounds/catalog.json').read_text())
namespace=uuid.UUID('13c8dd53-b62b-4528-ab66-85f0c8c095cc')
cabs={c['name']:c['id'] for c in json.loads((root/'assets/ir/catalog.json').read_text())['assets']}
def identity(key):return str(uuid.uuid5(namespace,key))
voices={p['name']:p['id'] for p in library['pedals']}
def pedal(key,name,type,**parameters):
 id=identity('pedal:'+key);voices[name]=id
 if not any(p['id']==id for p in library['pedals']):library['pedals'].append(dict(id=id,key=key,name=name,effect_type=type,effect_version=1,parameters=parameters))
 return name
families=[('solid','amp_solid_clean','Solid Platform'),('us','amp_us_combo','US Combo'),('chime','amp_brit_chime','British Chime'),('crunch','amp_brit_crunch','British Crunch'),('hotrod','amp_brit_hotrod','British Hot-Rod'),('mark','amp_mark','Mark'),('modern','amp_modern_tight','Modern Tight'),('recto','amp_recto','Recto Heavy')]
for key,type,name in families:
 for variant,gain,master,output in [('clean',2,25,-6),('edge',10,45,-10),('rhythm',22,35,-14)]:
  if key in ('solid','us') and variant=='rhythm':gain=18
  p=dict(gain_db=gain,master_percent=master,output_db=output,bass_db=-4 if key=='mark' else -1,treble_db=1)
  if key!='chime':p['mid_db']=1
  if key=='mark':p['lead_gain_db']=14 if variant=='clean' else 22
  if key=='modern':p.update(sag_percent=5,low_cut_hz=140,resonance_percent=20)
  if key=='recto':p.update(sag_percent=40,resonance_percent=35)
  pedal(f'amp.{key}.{variant}',f'{name} {variant.title()}',type,**p)
 pedal(f'amp.{key}.lead',f'{name} Lead',type,gain_db=20 if key in ('solid','us') else 30,master_percent=55,output_db=-16,bass_db=-4 if key=='mark' else 0,treble_db=0)
pedal('drive.oxford.punch','Oxford Punch','oxford_distortion',drive_db=18,bass_db=1,contour_percent=25,treble_db=2,output_db=-14)
pedal('drive.oxford.grind','Contour Grind','oxford_distortion',drive_db=28,bass_db=0,contour_percent=60,treble_db=0,output_db=-16)
pedal('drive.oxford.open','Open Contour Crunch','oxford_distortion',drive_db=8,bass_db=-1,contour_percent=15,treble_db=1,output_db=-8)
pedal('drive.asym.tight','Tight Mid Boost','asymmetric_drive',drive_db=3,low_cut_hz=170,tone_hz=6500,output_db=3)
pedal('drive.asym.warm','Warm Asymmetric Drive','asymmetric_drive',drive_db=14,low_cut_hz=90,tone_hz=4000,output_db=-10)
pedal('drive.hard.alternative','Hard Alternative','hard_distortion',drive_db=21,low_cut_hz=80,filter_hz=3900,output_db=-14)
pedal('drive.sustain.sing','Singing Sustain Fuzz','sustain_fuzz',drive_db=24,compression_percent=45,tone_hz=3100,output_db=-16)
pedal('drive.sustain.doom','Doom Sustain Fuzz','sustain_fuzz',drive_db=30,compression_percent=65,tone_hz=1800,output_db=-17)
pedal('drive.bias.stable','Singing Stable Bias Fuzz','bias_fuzz',drive_db=25,gate_db=-72,compression_percent=30,bias_percent=8,stability_percent=100,output_db=-16)
pedal('drive.bias.velcro','Velcro Bias Fuzz','bias_fuzz',drive_db=30,gate_db=-42,compression_percent=70,bias_percent=-45,stability_percent=60,output_db=-16)
pedal('drive.bias.control','Controlled Bias Fuzz','bias_fuzz',drive_db=24,gate_db=-55,compression_percent=45,bias_percent=30,stability_percent=75,output_db=-15)
pedal('boost.tight','Metal Tight Boost','tight_boost',drive_db=1,low_cut_hz=140,attack_percent=30,output_db=6)
pedal('gate.tight','Tight Gate','noise_gate',threshold_db=-54,range_db=65,attack_ms=1,hold_ms=35,release_ms=130,hysteresis_db=7)
pedal('gate.lead','Lead Decay Gate','noise_gate',threshold_db=-64,range_db=45,attack_ms=2,hold_ms=90,release_ms=350,hysteresis_db=8)
pedal('eq.thrash','Thrash Graphic','graphic_eq',**{'80_hz_db':3,'240_hz_db':-1,'750_hz_db':-6,'2200_hz_db':2,'6600_hz_db':1,'output_db':-1})
pedal('eq.stadium','Stadium Graphic','graphic_eq',**{'80_hz_db':2,'240_hz_db':-1,'750_hz_db':-4,'2200_hz_db':1,'6600_hz_db':0,'output_db':-1})
pedal('eq.post.tight','Post-Cab Tight','parametric_eq',high_pass_hz=80,low_pass_hz=8000,band1_hz=180,band1_db=-1,band2_hz=850,band2_db=1,band3_hz=3800,band3_db=-2)
pedal('eq.post.full','Post-Cab Full','parametric_eq',high_pass_hz=70,low_pass_hz=7200,band1_hz=150,band1_db=0,band2_hz=900,band2_db=1,band3_hz=3500,band3_db=-1)
pedal('eq.nasal','Nasal Mid Fuzz','parametric_eq',high_pass_hz=100,low_pass_hz=6200,band2_hz=1100,band2_db=5,band2_q=1.2)
pedal('mod.phaser.subtle','Subtle Four-Stage','phaser',rate_hz=.7,depth_percent=55,feedback_percent=10,mix_percent=18,stages=0)
pedal('mod.flanger.slow','Slow Flange','flanger',rate_hz=.2,depth_percent=35,manual_ms=3.5,feedback_percent=25,mix_percent=20)
pedal('wah.singing','Singing Manual Wah','wah',position_percent=65,heel_hz=300,toe_hz=2400,q=2.4,output_db=0,mix_percent=70)
pedal('wah.resonant','Art Envelope Wah','envelope_wah',sensitivity_db=18,attack_ms=10,release_ms=220,q=2.5,mix_percent=65)
pedal('echo.lead','Modest Tape Lead','echo',time_ms=320,feedback_percent=25,mode=2,color_hz=4200,mix_percent=15)
pedal('echo.rhythmic','Rhythmic Tape Echo','echo',bpm=110,clock=1,subdivision=2,feedback_percent=35,mode=2,color_hz=3800,wow_percent=5,mix_percent=24)
pedal('echo.western','Western Echo','echo',time_ms=180,feedback_percent=28,mode=1,color_hz=4500,mix_percent=20)
pedal('echo.long','Long Stereo Echo','echo',time_ms=700,feedback_percent=55,mode=1,color_hz=4800,stereo=1,mix_percent=38)
pedal('space.room','Gentle Room','space_reverb',mode=0,decay_seconds=.8,pre_delay_ms=8,damping_hz=6500,mix_percent=10)
pedal('space.plate','Lead Plate','space_reverb',mode=1,decay_seconds=1.8,pre_delay_ms=18,damping_hz=5500,mix_percent=14)
pedal('space.hall','Ambient Hall','space_reverb',mode=2,decay_seconds=7,pre_delay_ms=35,damping_hz=4200,mix_percent=40)
pedal('swell.bloom','Ambient Swell','swell',attack_ms=550,release_ms=150,threshold_db=-65,output_db=0)
pedal('hold.layer','Layer Hold','audio_hold',hold=0,mix_percent=65,output_db=-6)
pedal('pitch.octave','Expressive Octave','expression_pitch',heel_semitones=0,toe_semitones=12,expression_percent=0,detune_cents=0,mix_percent=100,output_db=-3)
pedal('pitch.detune','Spectral Detune Layer','expression_pitch',heel_semitones=0,toe_semitones=12,expression_percent=0,detune_cents=9,mix_percent=30,output_db=0)
pedal('dual.muse','Tight / Chime 70-30','dual_amp',model_a=6,gain_a_db=20,model_b=2,gain_b_db=8,blend_percent=30,level_a_db=-2,level_b_db=0,output_db=-14)

pedal('amp.recto.lowstrings','Recto Low String Dynamics','amp_recto',gain_db=18,bass_db=-4,mid_db=2,treble_db=0,master_percent=50,sag_percent=55,resonance_percent=25,output_db=-14)
pedal('chorus.clear','Clear Slow Chorus','chorus',rate_hz=.35,depth_percent=22,width_percent=60,mix_percent=18)

def rack(key,name,artist,gain,character,recipe,cab='british-4x12',tuning='Standard E',notes='',amp='See ordered amp family'):
 id=identity('sound:'+key);pedal_ids=[voices[n] for n in recipe]
 record=dict(id=id,key=key,name=name,output_db=-12,pedals=recipe,pedal_ids=pedal_ids,instance_ids=[identity('instance:'+key+':'+str(j)) for j in range(len(recipe))])
 record['cabinet_assets']={str(j):cabs[cab] for j,n in enumerate(recipe) if n=='Cabinet Unity'}
 if not any(p['id']==id for p in library['racks']):library['racks'].append(record)
 if not any(p['id']==id for p in metadata['sounds']):metadata['sounds'].append(dict(id=id,artist=artist,style='Art Rock' if artist=='Radiohead' else 'Rock / Metal' if artist in ['Muse','Metallica','Rock / Metal'] else 'Ambient',gain=gain,character=character,tags=[artist,gain,key],tuning=tuning,pickups='Bridge humbucker for gain; neck/single coil for glass. Adjust by ear.',input='Original starting point: trim 0 dB; match calibrated DI level and avoid clipping.',amp=amp,cabinet=cab+' (original synthetic; measured IR listening pending)',notes='Original inspired recipe, not historical recording settings. '+notes,evidence='Synthetic tests and level/headroom render checks; circuit accuracy / album matching unverified',listening='Listening pending'))
rack('oxford.punch','Radiohead / Oxford Contour Drive','Radiohead','Distortion','Upper-mid punch with a high-headroom pedal platform.', ['Oxford Punch','Solid Platform Clean','Cabinet Unity','Post-Cab Tight','Gentle Room'],amp='Solid-State Clean')
rack('oxford.lead','Radiohead / Oxford Filter Echo','Radiohead','Distortion','Contour grind with a deliberate resonant sweep and modest tape echo.', ['Contour Grind','Art Envelope Wah','Solid Platform Clean','Cabinet Unity','Modest Tape Lead','Lead Plate'],amp='Solid-State Clean')
rack('art.open','Radiohead / Open Art-Rock Crunch','Radiohead','Crunch','Open, less compressed crunch with clear pick attack.', ['Open Contour Crunch','US Combo Edge','Cabinet Unity','Gentle Room'],cab='clean-open-2x12',amp='US Clean Combo')
rack('art.glass','Radiohead / Glass and Echo','Radiohead','Clean','Articulate clean/chime with restrained stereo motion.', ['Solid Platform Clean','Cabinet Unity','Arpeggio Air','Western Echo','Gentle Room'],cab='clean-open-2x12',amp='Solid-State Clean')
rack('art.filter','Radiohead / Resonant Filter Drive','Radiohead','Crunch','Envelope band-pass emphasis into asymmetric edge breakup.', ['Art Envelope Wah','Warm Asymmetric Drive','US Combo Edge','Cabinet Unity','Gentle Room'],cab='clean-open-2x12',amp='US Clean Combo')
rack('art.bloom','Radiohead / Ambient Bloom','Radiohead','Ambient','Swelling detuned layers with long echo and hall.', ['British Chime Clean','Cabinet Unity','Ambient Swell','Spectral Detune Layer','Long Stereo Echo','Ambient Hall','Layer Hold'],cab='chime-2x12',amp='British Chime',notes='Requires stopped-state Creative 4096 profile. Hold captures audio; it does not sustain strings.')
rack('art.rhythm','Radiohead / Rhythmic Tape Echo','Radiohead','Clean','Dotted-eighth echo with clear attacks; no song-part switching.', ['British Chime Edge','Cabinet Unity','Rhythmic Tape Echo','Gentle Room'],cab='chime-2x12',amp='British Chime')
rack('muse.stable','Muse / Singing Stable Fuzz','Muse','Fuzz','Stable bias fuzz, subtle four-stage motion and chime.', ['Singing Stable Bias Fuzz','Subtle Four-Stage','British Chime Edge','Cabinet Unity','Lead Plate'],cab='chime-2x12',amp='British Chime',notes='Plug In Baby-inspired guitar starting point; no Fuzz Factory circuit claim.')
rack('muse.chaos','Muse / Gated Chaos Fuzz','Muse','Experimental','Bias cutoff and controllable velcro decay; no free oscillator.', ['Velcro Bias Fuzz','British Crunch Edge','Cabinet Unity','Gentle Room'],amp='British Crunch')
rack('muse.blend','Muse / Saturated Chime Blend','Muse','High Gain','Parallel tight saturation and chime clarity, initially 70/30.', ['Tight Gate','Tight / Chime 70-30','Cabinet Unity','Post-Cab Tight','Gentle Room'],amp='Modern Tight + British Chime',notes='Two correlated amp branches feed one shared cabinet. This is not double tracking.')
rack('muse.tight','Muse / Tight Modern Drive','Muse','High Gain','Focused attack with a low-drive bass-tightening boost.', ['Tight Gate','Metal Tight Boost','Modern Tight Rhythm','Cabinet Unity','Post-Cab Tight','Gentle Room'],cab='tight-modern-4x12',amp='Modern Tight')
rack('muse.nasal','Muse / Nasal Fuzz','Muse','Fuzz','Mid-forward sustained guitar fuzz; not a bass-recording reproduction.', ['Singing Sustain Fuzz','British Crunch Edge','Cabinet Unity','Nasal Mid Fuzz','Lead Plate'],amp='British Crunch')
rack('muse.western','Muse / Western Echo','Muse','Crunch','British crunch with a short, restrained colored echo.', ['British Crunch Rhythm','Cabinet Unity','Western Echo','Gentle Room'],amp='British Crunch')
rack('muse.pitch','Muse / Expressive Pitch Fuzz','Muse','Experimental','Software expression over a stable guitar fuzz.', ['Singing Stable Bias Fuzz','Expressive Octave','British Chime Edge','Cabinet Unity','Modest Tape Lead'],cab='chime-2x12',amp='British Chime',notes='Requires Creative 4096. Monophonic spectral starting point; chords/attacks have artifacts. No DigiTech equivalence.')
rack('metal.thrash86',"Metallica / Thrash '86",'Metallica','High Gain','Low early bass, sharp Mark attack and a late graphic scoop.', ['Tight Gate','Mark Rhythm','Thrash Graphic','Cabinet Unity','Post-Cab Tight','Gentle Room'],cab='tight-modern-4x12',amp='Mark High Gain')
rack('metal.stadium','Metallica / Black Stadium','Metallica','High Gain','Fuller, darker heavy drive with retained midrange.', ['Tight Gate','Mark Rhythm','Stadium Graphic','Cabinet Unity','Post-Cab Full','Gentle Room'],cab='dark-full-4x12',amp='Mark High Gain')
rack('metal.d','Metallica / Slow Heavy D-standard','Metallica','High Gain','Broad heavy low-end dynamics, bounded post-cab bass.', ['Tight Gate','Recto Low String Dynamics','Cabinet Unity','Post-Cab Full','Gentle Room'],cab='dark-full-4x12',tuning='D standard; retune guitar physically',amp='Recto Heavy')
rack('metal.wah','Metallica / Singing Wah Drive','Metallica','High Gain','Manual software wah and a modest singing Mark lead.', ['Lead Decay Gate','Singing Manual Wah','Mark Lead','Cabinet Unity','Post-Cab Tight','Modest Tape Lead','Lead Plate'],cab='tight-modern-4x12',amp='Mark High Gain')
rack('metal.clean','Metallica / Glass Chorus Clean','Metallica','Clean','Soft clean combo shaping with slow gentle chorus and short echo/room.', ['US Combo Clean','Cabinet Unity','Clear Slow Chorus','Western Echo','Gentle Room'],cab='clean-open-2x12',amp='US Clean Combo')
rack('rock.crunch','Rock / Classic British Crunch','Rock / Metal','Crunch','Mid-focused crunch and volume cleanup starting point.', ['British Crunch Rhythm','Cabinet Unity','Gentle Room'],amp='British Crunch')
rack('rock.hotrod','Rock / Hot-Rod Lead','Rock / Metal','High Gain','Extra filtered gain stage with modest echo/plate sustain.', ['Lead Decay Gate','British Hot-Rod Lead','Cabinet Unity','Post-Cab Tight','Modest Tape Lead','Lead Plate'],amp='British Hot-Rod')
rack('metal.modern','Metal / Modern Tight Rhythm','Rock / Metal','High Gain','Tight boost, interstage bass control and palm-mute definition.', ['Tight Gate','Metal Tight Boost','Modern Tight Rhythm','Cabinet Unity','Post-Cab Tight'],cab='tight-modern-4x12',amp='Modern Tight')
rack('rock.recto','Rock / Recto Heavy Rock','Rock / Metal','High Gain','Broad low-end dynamics distinct from the tight rhythm family.', ['Tight Gate','Recto Heavy Rhythm','Cabinet Unity','Post-Cab Full','Gentle Room'],cab='dark-full-4x12',amp='Recto Heavy')
rack('metal.doom','Metal / Doom Fuzz Wall','Rock / Metal','Fuzz','Thick sustained fuzz into a low-gain amplifier, with controlled sub-bass.', ['Doom Sustain Fuzz','US Combo Edge','Cabinet Unity','Post-Cab Full','Lead Plate'],cab='dark-full-4x12',amp='US Clean Combo')
library['description']='Original playable inspired sounds and reusable voicings. Legacy identities/recipes are frozen; new models and synthetic cabinet families are separate. Listening approval belongs only to the protected Subterranean sound.'
path.write_text(json.dumps(library,indent=2)+'\n');(root/'assets/sounds/catalog.json').write_text(json.dumps(metadata,indent=2)+'\n')
print('Upgrade recipes present; existing records preserved.')
