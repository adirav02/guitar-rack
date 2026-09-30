# Portable state and Linux persistence

`ToneState` is a configuration-only model independent of JUCE, physical devices
and DSP history. It separates the rack definition (UUIDs, immutable slots,
effect type/version and order) from the fixed control snapshot and preset
identity/name. `guitarrack_state` uses the already pinned JUCE core JSON library;
no new dependency is fetched. JSON uses physical values, and registry version 1
permanently resolves keys to the existing normalized bank indices. Processing
views are transient and are never serialized.

## Schema and validation

`assets/presets/schema-v1.json` describes version 1 using JSON Schema Draft
2020-12. Clean, Crunch and Lead Delay are tracked examples; CTest validates them
against the runtime validator and compares their configurations with embedded
factory definitions, so the binary runs without a source-directory dependency.
No distributable third-party assets are used.

Runtime additionally checks relationships that JSON Schema cannot express:
unique UUIDs/slots, exact order bijection, supported required effects/versions,
immutable known identities and registry parameter keys/ranges. Objects are
closed. Every effect parameter is required; missing controls are not filled with
new defaults. `assets` is empty in schema v1; M5 schema v2 adds strict cabinet references. Unknown effects are not dropped.
The migration entry point accepts v1 unchanged and v2 with IR manifests.
Version 0 and future versions are errors; no fabricated legacy migration occurs.

Preflight rejects duplicate keys (including escaped equivalents), malformed
syntax/trailing content, invalid UTF-8 and Unicode surrogates, nulls, excessive
nesting/nodes, non-finite values, integer overflow and wrong field types. Only regular files are read (FIFO/device/directory imports are rejected without
blocking the parser). Files are bounded to 128 KiB, nesting to 32 and value nodes to 4096. Names contain
1–256 UTF-8 bytes. IDs are nonzero canonical lowercase UUIDs. Parameter/globals
are finite physical numbers within the actual registry/monitor ranges.

## Storage and user actions

The XDG data directory holds user UUID files and `last-tone.json`; the XDG config
directory holds versioned device/window preferences. Relative XDG paths are
ignored in favor of the home-directory defaults. Preset names never become
paths. Native Import/Export dialogs accept selected filesystem paths; exported
presets contain no paths, hardware settings, transport/Mute or temporal buffers.
Factory examples are embedded and their tracked files are never overwritten.
Save on a factory routes through Save As; custom imports can Save their existing
UUID. Save As generates one new preset UUID and preserves all effect identities.

Save/Export/settings use a same-directory mode-0600 temporary file, complete
writes, file fsync, close, rename and directory fsync. Pre-rename failure cleans
the temporary file and preserves the old destination. Directory-sync failure
occurs after rename: the new contents may be present, but durability is uncertain
and an error is still reported. Dirty state remains set on failed saves. This is
the Linux filesystem implementation; Windows atomic replacement is M7 work.

The message-thread preset session compares canonical serialized current tone
with its saved/loaded baseline. Parameter, global, bypass, structure and order
edits affect dirty state; device/Mute/transport edits do not. Save/Discard/Cancel
are explicit native dialogs when switching/importing/closing. A failed Save or
cancelled naming/file dialog leaves the action unfinished. Discard permits the
requested action without saving; failed imports still retain the edited sound.
On a normal close, recall stores the selected saved/loaded configuration, so
Discard never persists the discarded edits. Abrupt process termination does not
update last selection/settings; already saved user files remain in the library.

## Deleting a saved user preset

Delete eligibility requires a settled transaction, a selected named custom UUID,
and a validated managed-library entry. Factories, unnamed working racks and
unsaved external imports are protected. The shared dirty dialog stages
Save/Discard/Cancel for deletion; then a named confirmation defaults to Cancel
(initial focus, Enter and Escape). Neither cancellation resolves dirty state or
writes files. The captured UUID is rechecked at commit, so a newer selection or
pending load invalidates confirmation.

`PresetStore::erase` receives the UUID, never the display name/import path. It
rejects factory UUIDs, invalid IDs, symlink directories/entries, nonregular files,
invalid JSON and UUID mismatches. An opened store directory anchors `openat`,
entry checks and `unlinkat`; validation reads the same opened file descriptor.
The final entry device/inode must still match. Only that managed UUID file is
unlinked; no asset cleanup, export cleanup or name search occurs. External
processes racing path replacement are not a cross-process transaction lock.

The control/message thread performs filesystem work; parser and audio threads
never save/delete files. Save is serialized before deletion, inside the same
failure-handling transaction. Matching `last-tone.json` is atomically replaced
with Clean before unlink, then the store directory is fsynced. Other recall and
device/window preferences are unchanged. On failure, the original file/recall
are restored where possible; restoration failures are included in the error.
No in-memory selection, catalog, rack or dirty baseline commits on failure.
An unrecoverable filesystem fault can prevent rollback/durable persistence; the
error identifies that limit instead of claiming success.

Success removes the entry, clears its selection, and gives the current rack a
fresh working UUID and `Untitled rack` label. It remains dirty until Save As. No
DSP graph is built/reset and no parameter, routing, gain, bypass, Mute or
transport value changes; delay history continues. Close with Discard recalls
Clean, not the deleted tone or unsaved working edits. Save As saves the working
rack with a new preset UUID and unchanged effect identities/slots/controls.

Delete is rejected while parser, graph preparation or accepted fades are pending.
After success, pre-delete serials/results are invalidated, the last acknowledged
tone generation is recorded, and cached library selection of a deleted UUID is
rejected. All mutation APIs have the existing single message/control producer;
there is no background save that can recreate the file. Explicitly importing an
exported copy after deletion is a new request and remains supported.

## Async transaction, stale loads and host boundary

One parser thread owns a coalesced request and a coalesced result, each with a
serial. Reading, syntax checks, migration and validation run there. The message
thread polls results and submits a complete validated tone to the existing rack
worker. Old parser serials are ignored. Control-revision changes made during
parsing invalidate the older request. No preset I/O or parsing enters audio.

The graph worker prepares all DSP/resources and converts global dB to amplitude.
A tone request carries its own stable transaction identity across sample-rate
rebuild graph generations. The existing single pending-pointer mailbox,
four-owned-graph bound, two-processing-graph bound and retirement ring remain.
A full load may restore occupied slots because each prepared graph now owns a
fixed private processing view of the sole editable parameter bank. Ordinary
edits sync that view from the authority; a preset fade freezes the old view so
old DSP never interprets the replacement's slot controls. It adds no callback
container growth or graph destruction.

At a nonempty block boundary, generation and control revisions are checked
before acceptance. Newer pre-acceptance controls reject the tone. Acceptance
copies the fixed 16×16 controls/bypasses and globals, then activates the graph.
The existing gain/rack/effect/structural smoothers produce bounded transitions.
The current output protection and routing remain active. UI tone editing/saving
is disabled until acknowledgment, so no coherent capture races this bounded
install. Selected identity and clean baseline become visible after the 20 ms
fade acknowledgment. Failed/stale preparation never writes running controls.
An already accepted transition is allowed to complete if a newer request fails.

Cancellation before acceptance supersedes the generation; stale results are
reclaimed off audio. Cancellation after acceptance reports that the fade must
finish, rather than pretending an accepted sound never happened. While stopped,
the worker performs the same commit without opening a device or authorizing
Start. Stop/reprepare drain callbacks as before; Mute remains independent.

The control API has one designated producer; snapshot capture is off audio and
rejects a pending/in-flight tone. Later host integration must route requests and
snapshot acquisition through this boundary and define gestures/automation timing.
M4 does not implement the currently empty AudioProcessor host-state overrides or
publish host parameters. A full preset restores exact slots and may change what
slot automation means. Registry mappings and IDs remain unchanged.


## M5 IR assets and portable bundles

Schema v1 remains readable/writable for valid M4 presets; it now also describes
asset-free Distortion/Reverb. Schema v2 adds required `ir_asset` on each instance
(empty except Cabinet) and exact IR manifest records. The manifest uses SHA-256
as identity/checksum and only `ir/<sha256>.wav` as a relative reference. UUID,
slot, effect version, processing order and all physical parameter mappings remain
unchanged. Convolution buffers/history and original personal paths are absent.
See [assets.md](assets.md) for byte format, limits, validation and provenance.

Managed presets and `last-tone.json` stay JSON in the same locations. Managed IR
bytes live separately in the sibling `ir` directory, with checksum filenames.
The binary embeds the synthetic starter, so factories and their bundles require
no source-tree path. Non-synthetic assets must exist and match checksums: missing
or invalid IRs fail load, including while stopped. They are never bypassed or
replaced automatically. The parser resolves all required assets before the graph
worker prepares them; no selected sound/baseline commits before acknowledgment.

Native Cabinet **Load IR...** validates/copies the WAV on the parser, then requests
an existing full-tone transaction with the same preset/instance/slot identities.
Successful IR editing preserves the selected preset and its clean baseline and
sets dirty according to the new configuration. Failed, cancelled and stale
parsing/preparation keep the prior cabinet, controls, selection and dirty state.
Structure generations join existing bank/global revisions to reject a load
whose snapshot was overtaken by reorder/removal. Save/Save As also validate assets.
Stop/sample-rate restart resamples and prepares the same asset off audio.

Export defaults to `.grbundle`; IR-bearing racks always export bundle bytes,
even if a different filename extension is chosen. Asset-free `.json` exports
remain supported. Import recognizes bundle magic or strict preset JSON, so M4
exports continue working. Bundles carry preset JSON and the exact required WAV
bytes; they can import after the original external and source managed IRs are
unavailable. A JSON-only IR preset requires matching assets already in the local
store and fails with an actionable missing-asset error otherwise. Portable bundle
validation completes before any managed asset writes; no arbitrary path is
extracted. Failed disk writes can leave unreferenced validated assets, but never
partially switch the tone. Preset deletion keeps all shared IR files, as before.
