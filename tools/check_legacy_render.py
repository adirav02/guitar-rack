#!/usr/bin/env python3
import gzip,subprocess,sys,tempfile
from pathlib import Path
fixture=Path(__file__).resolve().parents[1]/'tests/fixtures/legacy/render.txt.gz'
with tempfile.TemporaryDirectory(prefix='guitarrack-legacy-') as directory:
 path=Path(directory)/'render.txt';path.write_bytes(gzip.decompress(fixture.read_bytes()))
 raise SystemExit(subprocess.call([sys.argv[1],'check',str(path)]))
