#!/usr/bin/env python3
"""Generate new control documentation from the executable registry export."""
import json,sys
from pathlib import Path
root=Path(__file__).resolve().parents[1]
registry=json.loads(Path(sys.argv[1]).read_text())
legacy={d['type'] for d in json.loads((root/'tests/fixtures/legacy/registry.json').read_text())}
text='''# New effect controls — tone upgrade, 2026-10-01

Generated from the executable registry. Exact versions appear in each heading.
Existing type/version/index/range/default/choice mappings are
frozen in `tests/fixtures/legacy/registry.json`. Values use linear normalization
`(physical-minimum)/(maximum-minimum)`; enums use the listed integer ordinal.
The 16 slots and 16 controls per slot remain fixed. Unused sparse indices are
reserved no-ops; replacing an effect changes the interpretation of that slot.
Controls normally ramp for 20 ms; algorithm-specific transitions are described
in [effects.md](effects.md). No host parameters have been published as a VST3.

'''
for d in registry:
    if d['type'] in legacy:continue
    text+='## `'+d['type']+'` v'+str(d['version'])+'\n\n| Index | Key | Units | Range | Default / enums |\n|---|---|---|---|---|\n'
    for p in d['parameters']:
        enum='; '+', '.join(f'{i}={v}' for i,v in enumerate(p['choices'])) if p['choices'] else ''
        text+=f"| {p['index']:02d} | `{p['key']}` | {p['units'] or 'enum'} | {p['minimum']:g}…{p['maximum']:g} | {p['default']:g}{enum} |\n"
    text+='\n'
(root/'docs/tone-controls.md').write_text(text)
