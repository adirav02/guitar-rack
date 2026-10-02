# Codex instructions — GuitarRack

## Mission and source of truth

Build the real-time guitar effects rack described in `PLAN.md`. Linux/Fedora standalone comes first; share DSP with Linux VST3 and later Windows products. The owner uses an M-Audio M-Track Solo and wants to add many built-in effects, reorder them, and save sounds.

Read `PLAN.md` before implementation. The repository contains a working Linux standalone through M4, the initial M5, experimental pedalboard and tone upgrade software. Read docs/tone-upgrade.md, docs/song-templates.md, docs/modern-rock-templates.md and the latest docs/testing.md record for current contracts: 42 types/44 descriptors, 168 sounds, 305 voicings, explicit Legacy 128 / Creative 4096 latency, strict local preferences v2 and frozen legacy compatibility. JUCE 8.0.15 is pinned to 91ad83ae34a81e0833b1a2b0866f54846370ae53. Read current PLAN.md status and verification records; do not restart M0. Software results and owner listening evidence are separate. The final tone-upgrade release/debug and optimized ASan/UBSan suites pass 15/15, with 4/4 selected TSan checks. The 60-row synthetic benchmark exposes dense-chain and combined Dual model/graph-fade overloads; 16 slots are a capacity limit, not a hardware deadline guarantee. Native display/HiDPI, hardware/round-trip/soak and new listening acceptance remain pending.

Use the canonical filename `AGENTS.md` at the repository root. Linux paths are case-sensitive. Do not create a second `AGENTS.MD` or duplicate lowercase plan that can drift out of sync.

Explicit owner instructions take precedence over this document. Inspect applicable parent/nested AGENTS instructions and the current repository before editing. Treat `PLAN.md` as the product/architecture contract; record necessary changes and their reasons there.

## Working style

- Communicate with the owner in concise English unless asked otherwise. Use English in code, application labels, commit messages, and technical documentation.
- Work one milestone at a time. Carry authorized work through implementation and relevant checks rather than stopping at a proposed plan.
- Make routine reversible implementation decisions autonomously. Do not ask permission for every dependency integration, refactor, build, or bug fix.
- Clarify materially missing facts or consequential product changes. Do not infer additional scope such as cloud services, third-party plugin hosting, or a mobile app.
- Do not create/push a remote repository, publish binaries, change global audio policies, or install/remove system packages without the owner's authorization for that action. The active tone and pedalboard upgrade authorizes local implementation, builds and checks; remote writes, package installation and publication remain excluded.
- When system packages are missing, report the exact Fedora install command; if installation is already authorized, continue within that authorization.
- Do not introduce a mandatory multi-agent workflow. Use additional agents only if the owner or another applicable instruction requests them.
- Provide progress updates during longer work. Report outcomes, checks, and remaining limitations clearly.

## Inspect before changing

1. Read applicable instructions, `PLAN.md`, README/build files, and relevant architecture notes.
2. Inspect git status and preserve uncommitted owner changes. Use `rg` for text/file discovery.
3. Verify Fedora release, compiler, CMake, Ninja, JUCE pin, and existing audio backend when relevant.
4. If hardware access is unavailable, implement and test independent work; explicitly leave physical tests pending.
5. Do not overwrite user files, create commits, or push changes unless that is included in the active request. Do not discard changes to make tests pass.

## Architecture constraints

- C++20 and CMake. Pin JUCE to a stable exact version/commit; verify current official requirements.
- One DSP/rack implementation for standalone and plugin. Platform/device logic belongs in adapters.
- Native JUCE GUI; disable the embedded browser (`JUCE_WEB_BROWSER=0`) and avoid unnecessary OpenGL dependencies.
- Keep persistent rack state, editable control state, and prepared processing graph responsibilities separate.
- Begin with a serial rack, a registry, stable effect type IDs, persistent instance IDs, and immutable slot IDs. Reorder modifies order only.
- Use the initial 16-slot, 16-control-per-slot fixed host bank described in PLAN.md. Freeze published parameter IDs and normalization. Do not dynamically add/remove host parameters.
- GUI labels may describe effects; generic host parameter names remain stable. Document automation semantics on effect replacement/preset load.
- Use one coherent parameter authority and validated off-thread structural updates. Prevent stale graphs from overwriting current automation.
- Avoid speculative frameworks, empty future directories, broad refactors, or a general-purpose routing graph before the serial rack works.
- Do not hardcode card index 3, USB bus addresses, sample rate assumptions, or the interface's display name.

## Real-time rules — mandatory

Inside the audio callback and all functions it calls:

- No heap allocation/deallocation, unbounded loops, blocking waits, mutex acquisition, file/network I/O, serialization, logging, GUI calls, or effect construction.
- Preallocate scratch buffers, delay lines, and graph processing resources during preparation.
- Handle variable, zero, and oversized blocks without callback allocation; use bounded chunking where required.
- Read controls through appropriate lock-free atomics/bounded transfers and smooth audible transitions.
- Do not mutate the editable rack container while processing it.
- Build structural replacements and load/resample IR files off-thread.
- Activate a prepared graph at a block boundary with bounded transitions. Reclaim retired objects off-thread after acknowledgment.
- Do not assume `shared_ptr` solves reclamation: its final release must not destroy DSP in the callback.
- Bound handoff/retirement queues and simultaneous graphs; reject/coalesce outside audio rather than waiting in audio.
- Keep output samples finite, clear unused channels, and ensure device failure produces silence.

Write a short architecture note describing thread ownership, producer/consumer boundaries, lifetime, queue saturation, sample-rate changes, and stale-request handling before shipping live rack edits. If an early milestone supports editing only while stopped, state the limitation and leave live-edit acceptance incomplete.

## Audio behavior

- Standalone must not automatically enable a live input on startup. Require explicit Start and valid device/channel selection.
- For the owner's interface, physical Input 2 is the mono guitar source; Line/Instrument is Instrument, and Direct/USB is USB. The GUI must support other devices and channels too.
- Map mono guitar to both output channels. The plugin supports validated mono-to-stereo and stereo-to-stereo layouts and never opens physical audio devices.
- Keep mute, rack bypass, and per-effect bypass behavior distinct and smooth.
- Support device disconnect/reconnect and actual sample-rate/buffer changes.
- Report requested/actual configuration and distinguish estimated DSP/device latency from measured round trip.
- Do not automatically replace PipeWire, enable a competing server, alter system scheduling, or reroute the user's desktop audio.
- Windows uses a native build and the official interface ASIO driver. Do not claim Linux testing proves Windows compatibility.

## DSP and state quality

- Give each effect meaningful units, bounded ranges, neutral/conservative defaults, stable parameter mappings, and preparation/reset behavior.
- Use physically meaningful tests: frequency response, impulse timing, envelopes, bounded output, continuity, and neutral/bypass behavior.
- Label early overdrive/amp implementations honestly. Evaluate aliasing, oversampling latency, dry/wet alignment, and cabinet processing before describing tone as polished.
- Validate presets before changing running state. Reject unknown required effects, invalid IDs/references, duplicate slots, non-finite/out-of-range values, oversized inputs, and unsupported schema versions.
- Preset load is transactional; failed or stale requests retain the previous rack. Save atomically.
- Keep hardware/device settings out of portable presets and host tone state.
- Preserve stable slot identity, effect version, ordering, parameter mapping, and IR references on state round trips.
- Load IRs off-thread; bound resource use, prevent path traversal, and give missing-asset errors without silently changing the sound.
- Do not distribute assets without documented rights. Record dependency/IR licenses and source provenance. Do not choose a project license that conflicts with dependency terms.

## Validation and completion

- Run the smallest meaningful checks for the milestone/change; do not add tests that merely mirror implementation details.
- Build affected targets and run relevant CTest checks. Once they pass, repeat only after further changes or unresolved concerns.
- Inspect real-time call paths and allocation/lifetime behavior. Run sanitizers in diagnostic builds separately from performance measurements.
- Test plugin parameters, automation, buses, state restore, editor lifecycle, latency/tails, and offline rendering in a real host when that milestone is reached.
- Never fabricate hardware, driver, listening, host, sanitizer, or benchmark results. A rendered UI or successful build does not verify live audio.
- Record toolchain/host versions, commands and results, plus the actual device/backend/sample rate/buffer for physical tests.
- Mark PLAN.md checkboxes complete only with evidence. Keep required owner guitar tests explicitly pending if unavailable.
- Update README/build instructions and architecture decisions when behavior changes. Keep documentation commands executable for the current repository.

Definition of done for an implementation task: requested behavior is implemented, relevant checks pass or blockers are specifically reported, no user changes are lost, the plan reflects actual status, and the handoff states what the owner can run next.

## Starting task for a new empty project only

This checkout already has the application; do not restart M0. For a new empty project, implement M0 before M1. Deliver a standalone window, explicit device/channel selection, safe Start/Stop/Mute, clean mono guitar monitoring, meters, and gain controls. Defer the rack effects until the audio path works. Follow any narrower scope in the active request.

## Tone upgrade ownership and acceptance

All legacy IDs/recipes/control mappings and original cabinet bytes are frozen in
`tests/fixtures/legacy`; never recapture them merely to pass a regression. New
factory identity is explicit and independent of display name/array position.
Regenerate source/schema/docs using the commands in docs/tone-upgrade.md.
Only the exact Art Rock / Subterranean Pulse legacy sound has owner listening
approval; all upgraded catalog entries remain listening pending. Local Linux
build/test/render evidence is separate from physical/display acceptance. The
upgrade does not complete M5 as a whole, VST3 or Windows.
