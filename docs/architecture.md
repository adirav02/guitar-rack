# GuitarRack architecture

Current tone-upgrade behavior is documented in [tone-upgrade.md](tone-upgrade.md);
exact new type/version/index/range/default mappings are in [tone-controls.md](tone-controls.md).
The legacy implementation descriptions below remain compatibility references.
They do not supersede the new profile/UI/catalog contracts.

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
layouts are validated; M2 does not build a plugin, publish host parameters, or
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
immediately. Rack bypass retains trim, output gain and protection; effect bypass
retains all other effects; Mute ramps the final output to silence.

Processing uses fixed 512-sample stereo scratch chunks. Zero blocks do nothing;
variable and oversized blocks are chunked without resizing. Each chunk reads
both inputs before writing outputs, so in-place stereo and mono-to-stereo
operation work. Effects follow input trim and precede output gain/protection. Invalid input samples become zero; extreme finite
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
ramp. A late backend restart cannot reinstate authorization. The rack worker
re-prepares at the actual rate before callbacks resume; see M2 ownership below.

Peak snapshots and diagnostic bits use lock-free atomics polled at 20 Hz. Peak
publication has fixed work; a racing poll may defer a peak to the next poll.
The UI holds clipping indicators until cleared. Configuration distinguishes
requested/actual format, device-reported I/O estimate, selected DSP latency (Legacy 128 / Creative 4096 samples),
and **unmeasured** round trip. Unsupported backend xrun reporting is displayed
as unavailable. No CPU/round-trip/dropout benchmark is inferred from tests.

## Fixed rack/state contract

M2 implements the registry, UUID/slot identities, separate order, and internal
fixed bank. Host parameter publication and preset serialization are deferred:

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
  Current parameters have one authority in the fixed bank. Structural snapshots
  contain no control values and cannot overwrite newer automation.
- Effect replacement and full preset load can change the meaning of existing
  slot automation; M2/M6 must document that behavior before publishing plugins.

No host parameter IDs have been published. Preset/state serialization, remembered
device preferences, additional catalog expansion and VST3 remain in their planned milestones. M3 mappings are in [effects.md](effects.md).


## M2 live serial rack: ownership and bounded transfer

`RackDefinition` holds instance UUIDs, immutable slot/type/version identities and
an independent ordered UUID list. Parameter/bypass values belong exclusively to
`ParameterBank` (16 slots × 16 normalized atomic controls, per-slot bypass and
rack bypass). The UI/control producer changes the requested definition under a
control-only mutex. The registry is immutable after engine construction. No
callback accesses the mutable definition, mutex, GUI or worker condition variable.

One worker copies the latest requested definition, reserves its slots under the
control mutex, then constructs/prepares processors outside that mutex. There is
one coalesced requested definition, one atomic prepared-pointer mailbox and a
four-entry retirement ring. A newer request replaces the queued definition;
when the mailbox is occupied, the worker waits/polls outside audio rather than
building additional graphs. Total graph ownership, including a build in progress,
is capped at four. At most two graphs ever process concurrently. There is no
shared_ptr/refcount destruction in processing.

At the start of a nonempty device/host block, audio takes at most one pointer,
checks its generation against the latest request, and either activates it or
retires it as stale. Activation inherits matching small DSP/bypass smoother state and
reads current controls from the bank. It performs bounded comparisons (at most
16 × 16 identities) and no construction. An accepted fade completes even if
newer requests arrive; another transition cannot start until the next block
following completion. The generation check is the acceptance linearization
point. The old/new outputs crossfade linearly for 20 ms at the actual sample
rate, using preallocated stereo buffers; unchanged Gain ramps retain their
progress. The model becomes **committed only after the crossfade acknowledgment**.
The UI shows the requested rack as pending, polls committed acknowledgment, and
restores the retained definition on preparation failure.

The callback is the sole retirement producer while live; the worker is the sole
consumer. Release/acquire indices publish raw pointers after their final audio
use. Every retired pointer is distinct and still worker-owned. Since only four
graphs can exist, at most four entries can await reclamation; the ring cannot
saturate independently of graph capacity. Audio never retries, waits, allocates,
locks, deletes, or notifies a condition variable. The worker removes graph
ownership and destroys DSP only after retirement publication. At shutdown the
adapter first drains callbacks; the worker then destroys all remaining graphs.

An atomic activation-reader counter brackets the bounded block-boundary swap.
Failure rollback waits **on the worker only** for an already-entered activation
to finish, then retains its accepted destination if a fade is in flight. Entries
after the failed request was published cannot accept older pending results.
New control requests cancel stale failures as well as stale successes. Failed
validation never changes requested/committed structure. Factory exceptions,
null factories, false prepare results, over-budget latency and invalid/unbounded tails discard the
replacement and keep the accepted sound. Parameter updates are never restored
from graph snapshots. UI sliders use validated physical units; future host writes
use the same normalized bank without calling structural/UI setters.

Slots remain reserved by all building, pending, active, fading and retired graphs.
A removed slot cannot be reinitialized for a new UUID until all those references
are reclaimed. Fast remove/add can therefore return a temporary “slot retiring”
error. Once reusable, the slot's generic controls describe the new effect: old
host automation retains its **slot**, not a removed UUID. A future full preset
load may similarly change automation meanings and must preserve its exact slot
mapping. M4 will serialize identities and bank values; M6 will publish the bank
once and implement host gestures. M2 does not claim cross-launch persistence.

Preparation, Stop and destruction require the adapter to detach/drain callbacks.
While stopped, the worker owns DSP access, activates edits without an audible
fade, resets temporal/smoothing state, and acknowledges commits. `prepare` waits
outside the processing callback for a fresh graph at the actual sample rate,
then transfers DSP ownership back to audio. Device configuration changes still
stop monitoring and require explicit Start; they never race a live re-prepare.

Gain v1 uses Control 00, normalized as `(dB + 60) / 84`, physical range -60 to
+24 dB, default 0 dB, a 20 ms linear amplitude ramp, and zero latency/tail. Other
controls are unused no-ops. Individual and rack bypass crossfade to dry over
20 ms while processors keep running. M3 extends the earlier M2 zero-tail
restriction to finite nonnegative tails <=120 seconds per stage; nonzero processing
latency remains rejected until dry/transition compensation is implemented. No
seamless temporal tails across structural replacement are promised. All public chunk processing callers provide 1–512 samples; the shared
engine bounds chunks from arbitrary device blocks and sanitizes nonfinite DSP
output before protection, exposing an atomic diagnostic flag.


## M3 temporal ownership and transitions

The M2 producer/consumer, stale-generation, saturation, rollback, reservation and
reclamation contract above remains intact. Each new effect is prepared by the
worker; the callback only reads the existing atomic bank and updates DSP-owned
small controls/state. Overdrive/EQ copy fixed-size history and smoother state
only for matching identities at the same sample rate. Delay copies only small
control/read-head state at activation, leaving its freshly prepared ring empty.
It never clears, copies, reallocates or destroys ring storage in the callback.
A fixed 20 ms ingress ramp softens the leading edge written to each fresh ring.
Structural crossfades can truncate previous echoes and produce a dip in wet-only
chains, including when another card is edited; continuous temporal history is
explicitly outside this M3 implementation. Failed/stale builds leave active
history intact. Off-audio Stop/reprepare clears all temporal storage after
callbacks drain; fresh preparation uses the actual sample rate.

Delay time changes use two read heads and a 100 ms fade. A running fade completes
before consuming the newest time target, coalescing rapid changes with fixed
work. Feedback and Mix remain independent 20 ms ramps. Individual/rack bypass
and Mute hide wet output while processors continue advancing. The Card Reset
button changes controls only. Detailed algorithms, tail bounds, units/mappings,
resource bounds and listening limitations are in [effects.md](effects.md).

Prepared graph tails sum serial stage bounds. Block-boundary activation publishes
the maximum active/fading bound to a lock-free double atomic; fade completion
publishes the accepted graph's bound. The shared adapter reads that atomic for
its tail query, without looking at editable state or acquiring a mutex. This is
a conservative declaration, not host validation or measured decay/latency.

## M4 extension: full tone transactions

The M2/M3 sections above describe their original stage. M4 now implements portable
presets and local preferences; host/VST3 integration remains deferred. The current
transaction, snapshot, processing-view, cancellation and persistence contracts
are detailed in [presets.md](presets.md).

There is still one editable parameter authority. Prepared graphs now use fixed
private processing views synchronized from that authority; during a full preset
fade, the old graph's view is frozen to preserve its original slot meanings.
The callback installs only prevalidated fixed-size controls and preconverted
globals at acceptance. No JSON, I/O, construction, locks, waits or reclamation
enters the callback. Message-thread editing/saving is withheld until commit
acknowledgment. Parser serials, graph generations and control revisions prevent
stale full loads from overwriting newer requests/controls. Global gains use the
existing 20 ms monitor ramps. Start/Stop, Mute and physical routing are excluded.


## M5 latency and cabinet assets

The existing control/parser/graph-worker/audio ownership remains. Distortion and
Reverb were implemented in separate detached worktrees and their three-file
changes integrated by the main agent. Shared interfaces, registry, fixed bank,
asset/preset/GUI/latency integration and regression verification stayed in the
main worktree. No graph-routing framework or extra product was introduced.

`RackInstance::irAsset` holds a cabinet's checksum configuration, not immutable
identity. UUID, slot, type and version remain immutable; replacing an IR keeps
those four fields. Rack validation rejects missing/invalid cabinet IDs and IR
references on other types. The main control producer installs an asset resolver;
the graph worker copies it under the control mutex before preparation. It captures
an AssetStore by value, without GUI/session lifetime dependencies. The resolver
reads/checks/decodes only off audio. Source-rate decoded IRs remain processor-owned
so repeated preparation retains the selected asset. Construction, resampling,
FFT tables/kernel transforms, allocations, reset/clearing and all reclamation
stay on the worker. A parser may validate files concurrently with one graph build;
there is exactly one parser preparation and one graph preparation, no per-IR
workers or unbounded background convolution queue.

Before any convolution kernel construction, the worker resolves the graph's IRs
sequentially and bounds the sum of destination samples to 768000. Sources are
≤96000 mono samples each; destination ≤384000. FFT kernels plus two histories
use about 48 bytes/destination tap, ≤36.9 MB per graph, plus source storage up to
6.15 MB, fixed processing arrays and a single resampling temporary ≤1.54 MB.
Four graph ownership limits still apply; cabinet resources are approximately
under 180 MB across four maximally budgeted graphs, excluding other effects,
parser/bundle storage, GUI and allocator overhead. At high rates the budget can
reject combinations otherwise valid at 48 kHz. Errors retain the previous graph;
there is no silent shortening, normalizing or asset substitution.

A cabinet uses direct FIR head + radix-2 FFT tail (256-frame internal quantum).
Twiddles and bit-reversal tables are calculated during prepare. Processing only
uses fixed arrays/pre-sized vectors, arithmetic and `std::complex`; no FFT/DSP
library, first-use singleton, loader, background task, refcount release, file
read or mutex is reachable from the callback. The head is causal at sample 0;
tail partitions are computed before their next block is needed, so buffering
latency is zero independent of device chunks. Reference convolution tests span
partition boundaries and arbitrary callback partitions. CPU work is bounded by
validated partition counts; boundaries do more work and are included in the
unpaced release benchmark. Cost/headroom on the physical interface is pending.

Legacy Distortion adds 8 samples; the other legacy effects add zero declared
buffering delay. New amp/drive/Dual Amp networks add 16 samples and Expression
Pitch declares 2048 samples; the selected profile bounds the serial sum.
In the legacy profile, every prepared graph is padded to **128 total samples** (originally 16×8), with fixed
per-node dry delay for individual bypass and a separate 128-sample rack dry ring.
This is an intentional small latency increase for prior empty/M4 racks that
avoids changing latency on structural/preset edits. It is reported as samples
and milliseconds in standalone; `MonitorProcessor` declares it outside audio.
No VST3 or host acceptance is inferred from that adapter declaration. Future
larger-latency processors now use the explicitly selected stopped-state Creative
4096 profile; incompatible loads retain the accepted sound.

At activation, the replacement starts processing fresh padding and temporal
storage while the old graph remains audible for 128 warm-up samples. Then the
existing 20 ms crossfade runs, using aligned graph outputs. This avoids copying
old transformed padding history into a differently transformed graph. Matching
Distortion state and per-node dry rings are fixed small copies; Cabinet/Reverb
copy only small control smoothers and ramp ingress into fresh history. Delay,
Reverb and Cabinet tails can shorten, including IR replacement and full recall.
Failed/cancelled/stale loads do not touch active histories. Stop/drained rate
restart clears all compensation and DSP histories off audio.

IR selection uses the preset parser's coalesced work/result slots, bank/global
revision checks plus a new structure-generation check. The parser validates and
installs content-addressed assets, then the main producer submits a full tone
transaction to the existing worker. A bounded set of in-flight IR-edit transaction
IDs distinguishes editing an asset from selecting a clean preset: acknowledgment
keeps the prior selected preset/baseline and marks the changed cabinet dirty.
Pre-acceptance cancellation and newer structure/control requests reject the
older result; accepted transitions finish as before. New tone values install
only at the callback boundary. The GUI never convolves or owns DSP histories.
Managed assets left by unsuccessful/stale imports are harmless unreferenced files;
cleanup is explicitly outside scope. Asset/bundle format/security and licenses
are documented in assets.md and assets/ir/README.md.


## Coherent pedal voicings (experimental extension)

The designated message/control producer validates a complete physical control
vector against its exact effect/version before changing a slot. A per-slot atomic
sequence surrounds all single and grouped control writes. The payload itself is
atomic; sequentially consistent payload/sequence operations avoid data races and
ensure that equal even sequences identify one coherent row. Each prepared DSP
view attempts to read the row twice at a chunk boundary. Contention retains the
previous private view until a later chunk; the callback never waits or retries
without a fixed bound. A voicing therefore changes only current controls through
existing smoothers, without replacing processors or resetting temporal rings.

Full tone transactions still install at the existing activation boundary while
message-thread editing is disabled. Graph lifetime, stale requests, bounded
mailboxes/retirement and single control-producer rules remain. New DSP allocates
its rings and descriptor copies during preparation, copies only small fixed state
on activation and frees resources through the existing off-audio reclaimer.
Pedal persistence and menu refresh use the message thread, never the callback.
