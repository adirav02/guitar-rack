# Song template evidence — 2026-10-01

See [verification and environment](../../testing.md#song-template-verification-2026-10-01).
The 9/9 core and 5/5 headless state/compatibility results are recorded from the
completed CTest tool outputs in that document. The retained LastTest logs cover
final affected-library/state checks and the selected 2-suite sanitizer run;
they do not contain an additional full native run. `headroom.csv` is the actual
final 36-row unclamped synthetic audit. `headless-verification.cmake` is the
path-specific harness used in this environment; copy it as CMakeLists.txt into
an ignored helper directory to reproduce the state build here. It is not the
native standalone build configuration.

No device, display, guitar, YouTube audio, real-host or Windows acceptance is
claimed. No legacy fixture was recaptured.
