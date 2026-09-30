# Cabinet assets and portable bundles (M5)

Cabinet convolution is linear FIR processing. It provides frequency/impulse
response shaping, not nonlinear amp simulation. The supplied filter is synthetic
and must never be described as a measured guitar cabinet.

## Accepted WAVs and resource bounds

- Little-endian RIFF/WAVE, one `fmt ` and one `data` chunk; complete declared
  RIFF/chunk lengths and padding. Unknown bounded ancillary chunks are skipped.
- Mono PCM 16/24/32-bit signed, or IEEE float32; standard fmt size 16, or 18 with
  zero extension. Compressed, extensible, RF64, multichannel and other encodings
  fail with a re-export instruction.
- Source rate 8000–192000 Hz; at least one sample, at most 500 ms / 96000 samples;
  maximum file 1 MiB. A final size check rejects changing/truncated source files.
- Every sample finite and within ±1; energy must exceed 1e-16 (near-silent IRs
  rejected); sum of absolute taps ≤128 bounds worst-case FIR gain. No automatic
  peak/energy normalization, onset trimming or substituted sound.
- Actual processing rate 8000–768000 Hz; destination ≤384000 samples per IR and
  aggregate ≤768000 per graph. At 48 kHz sixteen 500 ms IRs fit this storage
  budget; higher rates may require fewer/shorter assets. This is a memory/work
  ceiling, not a physical CPU headroom guarantee.
- One parser task and one graph build can run concurrently; imports coalesce,
  four graphs are owned and at most two process. No per-IR preparation threads.

Decode, windowed-sinc resampling, FFT preparation/reset/destruction and all file
operations run off audio. A failed/cancelled/stale load retains the accepted
cabinet, rack, selected preset and dirty baseline. Output Level uses bank index
0, -24…+6 dB/default 0; it smooths over 20 ms. No convolution history is saved.
Zero buffering latency is tested; nonzero IR onset is intentional response time.
The whole rack separately reserves 128 samples for oversampling alignment.

## Managed local storage

`$XDG_DATA_HOME/GuitarRack/ir/<sha256>.wav` (fallback
`~/.local/share/GuitarRack/ir`) contains byte-for-byte original user WAVs. Asset
IDs are lowercase 64-character SHA-256 checksums. Selecting an external WAV
validates and installs a content-addressed copy; its personal path is never
saved in tone state. Presets/recall remain JSON in their original directories.
Every non-synthetic asset read verifies its checksum before decode. Managed
directory components and asset entries reject symlinks; reads are bounded,
nonblocking and regular-file-only. Writes use a directory descriptor, exclusive
0600 temporary file, complete write, fsync, atomic rename and directory fsync.
Failed directory sync reports uncertain durability. External processes racing
directory replacement are not coordinated by a cross-process transaction lock.

The synthetic starter's identity is resolved from embedded bytes. It has a
tracked source-independent fixture and CC0 provenance; no managed file is
required for that special exact identity. This does not substitute it for a
missing user asset. Asset IDs always describe the exact original bytes.

IR changes are full asynchronous graph transactions. Required assets all
resolve before acceptance; replacing one IR preserves UUID, immutable slot,
effect version, order and controls. An IR edit retains the selected preset's
baseline and becomes dirty. Cancel is effective before acceptance; an accepted
fade finishes. Assets installed by stale/cancelled/failed requests may remain
unreferenced. **Preset deletion never deletes shared assets.** Asset cleanup is
outside this scope.

## Schema v2

Valid M4 schema v1 remains supported unchanged, with empty assets. Distortion
and Reverb can use v1. Cabinet requires v2, which adds `ir_asset` to every
instance (empty for other effects) and manifest records:

```json
{
  "asset_id": "<64 lowercase SHA-256 hex characters>",
  "sha256": "<same checksum>",
  "relative_path": "ir/<same checksum>.wav",
  "format": "wav"
}
```

Manifest fields are closed; IDs/checksums/path must agree. Exactly the unique
referenced IRs are declared; missing, duplicate or unused records fail. No paths
from a manifest are used as extraction destinations. Schema fixtures are
`schema-v1.json` and `schema-v2.json`; runtime also validates relationships,
finite numbers, immutable identities, 128 KiB JSON, 32 nesting levels and 4096
value nodes. Future versions fail. There is no invented pre-M4 migration.

## Portable `.grbundle` format

This deliberately small uncompressed container avoids a ZIP/tar parser and
compressed-bomb/link semantics. All integers are unsigned little-endian:

1. Eight magic bytes `GRBNDL2\n`.
2. u32 JSON byte count (≤128 KiB), then that many strict UTF-8 preset JSON bytes.
3. u32 asset count (≤16; exactly manifest's unique required asset count).
4. For each asset: u16 name length (exactly 71), the ASCII name
   `ir/<sha256>.wav`, u32 WAV byte count (≤1 MiB), then exact WAV bytes.
5. End of file; any trailing records fail.

Total file ≤17 MiB. Traversal/absolute/backslash/duplicate/unreferenced paths,
unsupported encodings, wrong checksums, truncation, extra/link records and size
violations all fail before managed writes. The format has no compression or link
record; neither can be extracted. All bytes/decoded IRs are validated first,
then installed under checksum filenames; no arbitrary directory tree is created.
Disk failure can leave already installed validated unreferenced assets, but the
running preset remains unchanged. Exports use atomic complete-file replacement.

IR-bearing racks always export bundle bytes; `.grbundle` is the native default
and recommended extension. Asset-free `.json` export remains supported. Import
recognizes magic irrespective of extension, otherwise requires strict JSON and
locally resolvable assets. A bundle is portable after the original external WAV
and source machine's managed storage disappear. Tests import into a separate
empty store and verify identical IR bytes/configuration.

## Rights

Only the original synthetic starter is distributed by this task. Its
[asset notice](../assets/ir/README.md) dedicates the WAV to CC0; the project's code
license and JUCE dependency terms are unchanged. User-provided IRs may have their
own restrictions; including their bytes in a private export does not grant new
redistribution rights. No downloaded, branded or measured cabinet is bundled.
