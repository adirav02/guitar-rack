# Additional Muse template evidence — 2026-10-01

See [verification](../../testing.md#muse-template-expansion-verification-2026-10-01)
and [album-tone research limitations](../../muse-tone-research.md).

- `core-library-tests.txt`: final affected library test, 1/1 pass.
- `state-and-identity-tests.txt`: final persistence/assets/voices/legacy identity
  checks, 4/4 pass.
- `schema-and-preservation.txt`: strict schemas and exact pre-expansion record
  preservation (81 sounds / 174 voices).
- `idempotence.txt`: 101 generated artifacts preserved byte-for-byte.
- `headroom.csv`: actual final 36-row synthetic render audit, before Monitor
  protection. Its output suggestions are tool diagnostics, not further applied
  changes; the calibrated template output levels are in the factory data.
- `application-pack.txt`: 24 cabinet-inclusive bundles verified through the
  application importer, original registry/legacy checks and ZIP integrity.

Commands for the affected tests in this environment:

```bash
/tmp/guitar-rack-tools/cmake/data/bin/ctest --test-dir build/core-debug \
  -R experimental_signals --output-on-failure
/tmp/guitar-rack-tools/cmake/data/bin/ctest --test-dir build/song-tones/headless \
  -R 'StateTests|AssetTests|PedalPresetTests|legacy_identity' \
  --output-on-failure -j 2
build/song-tones/headless/render_sounds --catalog-audit \
  build/song-tones/muse-expansion-final.csv 'Muse expansion'
```

The earlier [headless harness](../song-templates/headless-verification.cmake)
contains the pinned-JUCE build configuration used here. This evidence covers
template software behavior and initial headroom. No native standalone build,
device, display, real-host, Windows or album-tone listening acceptance is claimed.
