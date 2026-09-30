# M0/M1 architecture

## Current processing and ownership

The native JUCE standalone shell owns device discovery, the selected physical
channels, and device open/start/stop/close. It deliberately avoids the stock
standalone wrapper, `AudioDeviceManager` callback mixer, and
`AudioProcessorPlayer`: those paths can acquire callback locks or resize buffers.
Creating the window enumerates devices but opens no monitoring stream.
Creating a closed preview device can probe hardware capabilities.

`MonitorProcessor` is the shared JUCE `AudioProcessor` adapter. Both its
`processBlock` and the standalone callback use exactly one `MonitorEngine`.
Physical-device routing lives in `AudioSession`; the engine accepts stereo
pointers, including the same mono source pointer for both sides. The eventual
plugin never needs a physical device. Mono-to-stereo and stereo-to-stereo bus
layouts are validated; M1 does not build a plugin, publish host parameters, or
implement host/preset state.

The message thread owns GUI widgets, discovery objects, device lifetime, error
strings and open/close operations. Structural device/channel/rate/buffer changes
require Stop. No callback stores GUI pointers or touches editable containers.
The backend serializes lifecycle preparation against processing. Prepared
smoothers belong exclusively to the callback while active. The message thread
resets them only after the callback is detached and drained.

`MonitorControls` is the single authority for trim/level/mute. Setters validate
finite bounded values and calculate dB-to-linear conversion outside audio.
Callback reads use lock-free atomics once per block. DSP applies sample-by-sample
linear ramps over 20 ms at the actual sample rate. Start begins at zero, Mute
retains input metering, and Stop fades before closing the device. Errors silence
immediately. This milestone has no effects/bypass controls: mute is not a rack
bypass or effect bypass.

Processing streams samples without scratch storage or temporal effects. Zero
blocks do nothing; variable and oversized blocks need no resizing or chunking.
Each sample reads both inputs before writing outputs, so in-place stereo and
mono-to-stereo operation work. Invalid input samples become zero; extreme finite
samples are bounded before multiplication. The zero-latency output protection is
a **hard ceiling at ±0.98**, linear below the ceiling. Hitting it changes tone;
the UI latches a warning. It is not a transparent limiter or headphone safety
guarantee. Trim/output defaults are 0/-12 dB.

The device opens exactly one input and two distinct outputs. The pinned Linux
backends pack enabled channels in ascending physical order; the adapter maps
left/right from the actual masks rather than indexing packed arrays with physical
channel numbers. Backend substitution, missing pointers, or changed channel
counts fail closed. Every available output is cleared on failure.

## Callback handoff and the pinned JUCE patch

JUCE 8.0.15 at commit `91ad83ae34a81e0833b1a2b0866f54846370ae53`
uses `callbackLock` in ALSA/JACK. `cmake/patch_juce.py` checks normalized source
SHA-256 values and applies a small project-owned patch when fetching or using
the local pinned checkout. Already-patched sources must match their expected
hash too. It retains upstream license notices. Unexpected sources fail the
configuration rather than silently building an unreviewed backend.

The patch uses `CallbackGate<T>` instead of the callback mutex. Each processing
invocation increments a lock-free reader count before loading the callback,
then decrements after its invocation. The single control producer publishes
null, waits for readers to drain **outside audio**, then prepares/publishes or
destroys resources. Sequentially consistent atomics order the detach against
reader entry. The callback has no waits, retries, allocation, locks or reference
counted destruction. There is one callback pointer, one active reader per
backend, no queue, and no accumulated pending replacements; the GUI prohibits
concurrent device changes. Callback lifetime is the containing `AudioSession`
lifetime, and device shutdown drains access before its destruction.

JACK's active channel masks are read only while a callback lease is active.
The detached process path clears all registered output ports without reading
mutable masks. The existing control-thread port update detaches/drains before
changing those masks. Server shutdown publishes an atomic flag; close/reclamation
is deferred to the message thread. ALSA conversion scratch is reserved during
device setup, with bounded size checks during I/O. Its runtime failure/xrun
status uses atomics so GUI polling does not read mutable worker error strings.

The no-lock/no-allocation promise covers GuitarRack's callback and its processing
calls, plus the patched callback dispatch. ALSA's device worker still performs
driver read/write/wait/recovery outside callback dispatch; JACK/PipeWire and
driver internals have their own implementation constraints. These are not
certified by the synthetic allocation test or a successful build. Physical
deadline/dropout behavior remains to be measured.

## Failure, restarts and diagnostics

An explicit Start grants authorization. Lifecycle callbacks can revoke it but
cannot grant it. A device error or unexpected stop revokes authorization and
sets a bounded atomic fault flag. The next processing call clears output on
fault; a GUI timer closes the stream and shows Refresh / Reconnect instructions.
The timer also checks device liveness, actual masks/rate/buffer, and a callback
heartbeat (at least two seconds or four device blocks before timing out).
Configuration changes safely stop the stream and require Refresh/reselection
and a new Start. Reconnection never switches to a microphone or another output
automatically. The GUI Stop ramp has a 250 ms timeout if callbacks disappear.

Every restart prepares from the actual device sample rate and resets the Start
ramp. There are no async graph-building requests in M1, so no stale structural
results can arrive. A late backend restart cannot reinstate authorization.
Off-thread graph building, bounded queue saturation handling, generation IDs,
crossfades and retired graph reclamation remain M2 work; live rack edits are not
implemented or accepted as complete.

Peak snapshots and diagnostic bits use lock-free atomics polled at 20 Hz. Peak
publication has fixed work; a racing poll may defer a peak to the next poll.
The UI holds clipping indicators until cleared. Configuration distinguishes
requested/actual format, device-reported I/O estimate, zero added DSP latency,
and **unmeasured** round trip. Unsupported backend xrun reporting is displayed
as unavailable. No CPU/round-trip/dropout benchmark is inferred from tests.

## Reserved rack/state contract for later milestones

M0 defines this contract in documentation; no unused registry or graph framework
is created yet:

- Effect type IDs will be stable English identifiers such as `gain`,
  `overdrive`, `eq`, `delay`, versioned in the registry.
- Instance IDs are persistent UUIDs. Slot IDs are immutable integers 0–15;
  ordering is a separate list and reorder changes order only.
- Reserve 16 slots × 16 normalized controls plus per-slot bypass and globals.
  IDs such as `slot_03_param_00` and generic host names stay fixed. Effect
  descriptors define permanent versioned normalization and bank indices.
- Persistent tone state contains identity/order/effect versions/control mapping
  and assets; standalone device settings stay outside portable tone/host state.
- The editable definition is control-owned; a prepared graph is audio-owned.
  Current parameters have one authority, eventually integrated with the fixed
  host bank. Structural snapshots cannot overwrite newer automation.
- Effect replacement and full preset load can change the meaning of existing
  slot automation; M2/M6 must document that behavior before publishing plugins.

No parameter IDs have been published in M1. Preset/state serialization,
remembered device preferences, effects, live rack editing, and VST3 targets are
intentionally deferred to their planned milestones.
