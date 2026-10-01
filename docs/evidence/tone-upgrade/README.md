# Tone upgrade evidence — 2026-10-01

These are local software results on the pinned baseline and Fedora toolchain in
[the current verification record](../../testing.md#tone-upgrade-current-verification-2026-10-01).
They do not approve guitar tone, album matching, display behavior or hardware deadlines.

- `release-tests.txt`, `debug-tests.txt`: final full CTest console results.
  `debug-details.txt` retains measured signal output. A stale console label in
  that full run says “77 pedal round trips”; the loop actually tests all 146.
  The label now uses the actual count; targeted reruns passed in both builds.
- `address-undefined-tests.txt`, `address-undefined-details.txt`: full final
  optimized Debug (`-O1 -g`, assertions enabled) ASan/UBSan run, 15/15 pass.
  Leak detection is disabled and unverified. The older unoptimized render
  timeout is retained in the ignored build logs and explained in testing.md.
- `thread-sanitize-tests.txt`: four selected final TSan suites, no reported race.
- `generation-check.txt`: exact regeneration of 81 files and frozen legacy
  UUID/recipe/byte/control verification. The legacy render fixture is separate
  under tests/fixtures/legacy; it was never recaptured for a failure.
- `amp-response.csv`: final test output at 48 kHz, default controls except Gain
  0 dB; small-signal input amplitude 0.005 at 82/750/4500 Hz. Third-harmonic ratios
  use a 211-Hz input, amplitude 0.1, Gain 0/max; impulse 0.25, late peak after
  0.5 s. Output remains default −12 dB. These are actual model responses, not
  measured commercial amplifiers or perceived level comparisons.
- `pitch-held-notes.csv`: 48-kHz held notes, ±12/24 st, input amplitude 0.1,
  two-second render and last-half frequency projections at one-cent resolution.
  Transient/bend/chord limitations are recorded separately in testing.md.
- `alias-4x65.csv`, `alias-8x129.csv`, `alias-current.csv`: 168 cases each,
  candidate/selected/final cached-math versions. Reference nonharmonic energy
  includes settling/filter leakage; it is not a pure alias certificate.
- `catalog-headroom.csv`: final 207 cases (69 racks × three strengths), measured
  input peaks, output peaks/phrase RMS, processor profile and offline suggested
  output. Suggestions did not change legacy gains. A mathematical synthetic DI
  is not a pickup calibration or perceived loudness appraisal.
- `benchmark-before-optimization.csv`, `benchmark-matrix.csv`: 60 rows each,
  release-only mean/p99/max and two-graph fade wall times. Final matrix runs
  before concurrent build/diagnostic/render work. Dense and some Dual transition
  cases exceed device periods; scheduling outliers are retained, not filtered.

All CSVs are bounded, deterministic signal scenarios except wall-clock timing.
The separate source tools, exact commands, portable tolerances, diagnostics,
latency/tail policies and physical blockers are documented in testing.md and
[the upgrade contract](../../tone-upgrade.md). Only the exact legacy
Subterranean Pulse has owner listening approval. New cabs are original CC0
synthetic filters, not measured speakers.
