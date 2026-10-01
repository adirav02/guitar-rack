#!/usr/bin/env python3
"""Verify additive seeding, edit preservation and every checkpoint identity.

Run with a git baseline to compare the entire previous catalog, including the
Radiohead/Muse additions, independently of the older frozen legacy fixtures.
"""
import argparse
import copy
import json
import subprocess
import tempfile
from pathlib import Path
from add_alternative_templates import seed, COLLECTION

ROOT = Path(__file__).resolve().parents[1]


def check(baseline):
    library = json.loads((ROOT/'assets/sounds/library.json').read_text())
    catalog = json.loads((ROOT/'assets/sounds/catalog.json').read_text())
    if baseline:
        for path, current, kinds in (
            ('assets/sounds/library.json',library,('pedals','racks')),
            ('assets/sounds/catalog.json',catalog,('sounds',))):
            old = json.loads(subprocess.check_output(['git','show',baseline+':'+path],cwd=ROOT))
            for kind in kinds:
                by_id = {x['id']:x for x in current[kind]}
                assert len(by_id)==len(current[kind]), 'Duplicate UUID'
                assert all(by_id[x['id']]==x for x in old[kind]), 'Checkpoint recipe/metadata changed'
        # Existing portable files and all compatibility assets must be byte-identical.
        paths = subprocess.check_output(['git','ls-tree','-r','--name-only',baseline,
                                         'assets/presets','assets/ir','tests/fixtures/legacy'],cwd=ROOT,text=True).splitlines()
        for path in paths:
            local = ROOT/path
            if not local.exists():
                # A connector-only checkout may lack a large binary. Require its
                # index object to remain exactly the baseline object; do not recapture.
                expected = subprocess.check_output(['git','rev-parse',baseline+':'+path],cwd=ROOT,text=True).strip()
                actual = subprocess.check_output(['git','ls-files','-s','--',path],cwd=ROOT,text=True).split()[1]
                assert actual==expected, 'Missing binary identity changed'
                print('Not locally readable; unchanged Git object:',path)
            else:
                expected = subprocess.check_output(['git','rev-parse',baseline+':'+path],cwd=ROOT,text=True).strip()
                actual = subprocess.check_output(['git','hash-object',path],cwd=ROOT,text=True).strip()
                assert actual==expected, 'Existing preset/IR/compatibility bytes changed: '+path
    selected = [m for m in catalog['sounds'] if COLLECTION in m['tags']]
    assert len(selected)==42
    assert len({m['tags'][5] for m in selected})==14
    by_id = {r['id']:r for r in library['racks']}
    voices = {p['id']:p for p in library['pedals']}
    for m in selected:
        r = by_id[m['id']]
        assert len(r['pedal_ids'])==len(r['instance_ids'])<=16
        assert all(voices[p]['effect_type'] not in ('expression_pitch','pitch') for p in r['pedal_ids'])
        assert m['listening']=='Listening pending'
        assert 'physical tuning' in m['tuning']
    # Repetition must preserve byte representation, then preserve deliberate edits
    # to newly seeded entries as well as earlier entries.
    with tempfile.TemporaryDirectory() as directory:
        root = Path(directory)
        for path in ('assets/sounds/library.json','assets/sounds/catalog.json','assets/ir/catalog.json'):
            destination=root/path; destination.parent.mkdir(parents=True,exist_ok=True)
            destination.write_bytes((ROOT/path).read_bytes())
        if baseline:
            for path in ('assets/sounds/library.json', 'assets/sounds/catalog.json'):
                (root/path).write_bytes(subprocess.check_output(
                    ['git', 'show', baseline+':'+path], cwd=ROOT))
            assert seed(root)==(42,60)
            assert json.loads((root/'assets/sounds/library.json').read_text())==library
            assert json.loads((root/'assets/sounds/catalog.json').read_text())==catalog
        before = [(root/p).read_bytes() for p in ('assets/sounds/library.json','assets/sounds/catalog.json')]
        assert seed(root)==(0,0)
        assert before==[(root/p).read_bytes() for p in ('assets/sounds/library.json','assets/sounds/catalog.json')]
        edited_library, edited_catalog = copy.deepcopy(library), copy.deepcopy(catalog)
        edited_library['racks'][-1]['output_db']=-17.25
        edited_library['pedals'][-1]['parameters']['drive_db']=11.25
        edited_catalog['sounds'][-1]['notes']='Owner-adjusted version retained'
        for path,value in (('assets/sounds/library.json',edited_library),('assets/sounds/catalog.json',edited_catalog)):
            (root/path).write_text(json.dumps(value))
        assert seed(root)==(0,0)
        assert json.loads((root/'assets/sounds/library.json').read_text())==edited_library
        assert json.loads((root/'assets/sounds/catalog.json').read_text())==edited_catalog
    print('PASS: 42 racks / 14 songs, Legacy-compatible recipes, full checkpoint preservation, idempotence and edited-entry retention')


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--baseline')
    check(parser.parse_args().baseline)
