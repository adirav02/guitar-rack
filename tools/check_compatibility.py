#!/usr/bin/env python3
"""Frozen baseline identities, state bytes, registry normalization and assets."""
import json,hashlib,subprocess,sys
from pathlib import Path
root=Path(__file__).resolve().parents[1];frozen=root/'tests/fixtures/legacy'
old=json.loads((frozen/'library.json').read_text());new=json.loads((root/'assets/sounds/library.json').read_text())
for kind in ['pedals','racks']:
 by_id={x['id']:x for x in new[kind]}
 assert len(by_id)==len(new[kind]),'Duplicate factory UUID'
 for record in old[kind]:
  current=by_id[record['id']]
  for key,value in record.items():assert current[key]==value,f'Legacy {record["id"]} changed {key}'
for file,checksum in json.loads((frozen/'checksums.json').read_text()).items():
 assert hashlib.sha256((root/file).read_bytes()).hexdigest()==checksum,f'Legacy bytes changed: {file}'
if len(sys.argv)>1:
 registry=json.loads(subprocess.check_output([sys.argv[1]],text=True))
 for descriptor in json.loads((frozen/'registry.json').read_text()):
  current=next(x for x in registry if x['type']==descriptor['type'] and x['version']==descriptor['version'])
  assert current==descriptor,f'Legacy control/version contract changed: {descriptor["type"]}'
# Freeze the approved recipe explicitly, independently of its friendly name.
approved=json.loads((root/'assets/presets/experimental/50000000-0000-4000-8000-000000000008.json').read_text())
assert approved['globals']['output_db']==-12
assert [i['effect_type'] for i in approved['instances']]==['amp','cabinet','tremolo','delay','reverb']
assert approved['instances'][1]['ir_asset']=='4a9ac650b2983ca0c49c138d11beccd2d0c8607ac9fccf47c537fbbf7bb7adae'
print('PASS: all frozen legacy identities, recipes, bytes and control mappings; protected Subterranean')

metadata=json.loads((root/'assets/sounds/catalog.json').read_text())['sounds']
expected={r['id'] for r in new['racks']}|{f'00000000-0000-4000-8000-{n:012d}' for n in range(1,6)}
assert {m['id'] for m in metadata}==expected and len(metadata)==len(expected),'Catalog coverage/identity mismatch'
assert [m['id'] for m in metadata if 'approved' in m['listening'].lower()]==['50000000-0000-4000-8000-000000000008'],'Listening approval must remain specific to Subterranean'
