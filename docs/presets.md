# M4 portable state and Linux persistence

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
new defaults. `assets` must be empty in M4. Unknown effects are not dropped.
The migration entry point dispatches version 1 only. Both older and future
schema versions are errors until a real migration is implemented.

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
