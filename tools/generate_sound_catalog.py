#!/usr/bin/env python3
"""Strict versioned, non-automated sound metadata, embedded off audio."""
import json, uuid
from pathlib import Path
root=Path(__file__).resolve().parents[1]
data=json.loads((root/'assets/sounds/catalog.json').read_text())
if set(data)!={'schema_version','sounds'} or data['schema_version']!=1:
    raise ValueError('Unsupported catalog schema')
fields='id artist style gain character tuning pickups input amp cabinet notes evidence listening'.split()
ids=set()
code='#include "SoundCatalog.h"\nnamespace guitarrack {\nconst std::vector<SoundMetadata>& soundCatalog() {\nstatic const std::vector<SoundMetadata> entries{\n'
for s in data['sounds']:
    if set(s)!=set(fields+['tags']) or s['id'] in ids or s['gain'] not in ['Clean','Crunch','Distortion','Fuzz','High Gain','Ambient','Experimental']:
        raise ValueError('Invalid catalog record')
    if str(uuid.UUID(s['id']))!=s['id']: raise ValueError('Noncanonical catalog UUID')
    ids.add(s['id'])
    for k in fields:
        if not isinstance(s[k],str) or not s[k] or len(s[k].encode())>1024:
            raise ValueError('Invalid catalog string')
    if not isinstance(s['tags'],list) or len(s['tags'])>32 or any(not isinstance(x,str) or not x or len(x.encode())>256 for x in s['tags']):
        raise ValueError('Invalid tags')
    code+='{' + ','.join(json.dumps(s[k],ensure_ascii=True) for k in fields)+',{'+','.join(json.dumps(x,ensure_ascii=True) for x in s['tags'])+'}},\n'
code+='}; return entries; }\nconst SoundMetadata* soundMetadata(const std::string& id) {for(const auto& s:soundCatalog()) if(s.id==id) return &s; return nullptr;}\n}\n'
(root/'src/core/SoundCatalog.cpp').write_text(code)
