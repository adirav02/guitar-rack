#!/usr/bin/env python3
"""Extend strict schema cases using JSON emitted by export_sound_library."""
import json
import sys
from pathlib import Path

root=Path(__file__).resolve().parents[1]
registry=json.loads(Path(sys.argv[1]).read_text())
for version in (1,2):
    path=root/f"assets/presets/schema-v{version}.json"
    schema=json.loads(path.read_text())
    instance=schema["properties"]["instances"]["items"]
    descriptors=[d for d in registry if version==2 or d["type"]!="cabinet"]
    instance["properties"]["effect_type"]["enum"]=list(dict.fromkeys(d["type"] for d in descriptors))
    instance["properties"]["effect_version"]={"type":"integer","minimum":1,"maximum":2}
    cases=[]
    for d in descriptors:
        properties={}
        for p in d["parameters"]:
            value={"type":"integer" if p["choices"] else "number","minimum":p["minimum"],"maximum":p["maximum"],
                   "description":f'Physical {p["units"] or "mode"}; stable bank index {p["index"]}, linear normalization.'}
            if p["choices"]:
                value["description"]+=" Choices: "+", ".join(f"{i}={x}" for i,x in enumerate(p["choices"]))+"."
            properties[p["key"]]=value
        case={"properties":{"effect_type":{"const":d["type"]},"effect_version":{"const":d["version"]},
              "parameters":{"type":"object","additionalProperties":False,"required":list(properties),"properties":properties}}}
        if version==2:
            case["properties"]["ir_asset"]={"type":"string","pattern":"^[0-9a-f]{64}$"} if d["type"]=="cabinet" else {"const":""}
        cases.append(case)
    # One exact effect/version pair; retain the existing structural/asset contract.
    instance.pop("allOf",None)
    instance["oneOf"]=cases
    path.write_text(json.dumps(schema,indent=2)+"\n")
print("Updated strict effect/version/control cases in schema v1 and v2.")

pedal={"type":"object","additionalProperties":False,"required":["schema_version","preset_id","name","effect_type","effect_version","parameters"],
       "properties":{"schema_version":{"const":1},"preset_id":schema["properties"]["preset_id"],"name":schema["properties"]["name"],
                     "effect_type":instance["properties"]["effect_type"],"effect_version":instance["properties"]["effect_version"],"parameters":{"type":"object"}},
       "oneOf":[{"properties":{k:v for k,v in case["properties"].items() if k!="ir_asset"}} for case in cases]}
(root/"assets/presets/pedal-schema-v1.json").write_text(json.dumps(pedal,indent=2)+"\n")
