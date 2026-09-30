"""Narrow, fail-closed patch for JUCE 8.0.15 Linux callback ownership.

Runs at dependency preparation, never at application runtime. Official source
hashes are checked after newline normalization. See docs/architecture.md.
"""
import hashlib
from pathlib import Path
import sys


def replace_once(text, before, after):
    if text.count(before) != 1:
        raise RuntimeError(f"Expected exactly one patch anchor: {before[:80]!r}")
    return text.replace(before, after, 1)


def alsa(text):
    text = replace_once(text, 'namespace juce\n{', '#include "guitarrack_CallbackGate.h"\n\nnamespace juce\n{')
    text = replace_once(text, '        const ScopedLock sl (callbackLock);\n        callback = newCallback;', '        callback.replace (newCallback);')
    text = replace_once(text, '                const ScopedLock sl (callbackLock);', '                guitarrack::CallbackGate<AudioIODeviceCallback>::Lease lease (callback);\n                auto* activeCallback = lease.get();')
    text = replace_once(text, '                if (callback != nullptr)\n                {\n                    callback->audioDeviceIOCallbackWithContext', '                if (activeCallback != nullptr)\n                {\n                    activeCallback->audioDeviceIOCallbackWithContext')
    text = replace_once(text, '    AudioIODeviceCallback* callback = nullptr;', '    guitarrack::CallbackGate<AudioIODeviceCallback> callback;')
    text = replace_once(text, '    CriticalSection callbackLock;\n', '')
    text = replace_once(text, '        auto oldCallback = internal.callback;', '        auto* oldCallback = internal.callback.get();')
    text = replace_once(text, '        numChannelsRunning = numChannels;', '''        numChannelsRunning = numChannels;
        scratch.ensureSize (static_cast<size_t> (sizeof (float))
                            * static_cast<size_t> (bufferSize)
                            * static_cast<size_t> (numChannels), false);''')
    scratch_resize = '            scratch.ensureSize ((size_t) ((int) sizeof (float) * numSamples * numChannelsRunning), false);'
    if text.count(scratch_resize) != 2:
        raise RuntimeError('Unexpected ALSA conversion scratch layout')
    text = text.replace(scratch_resize, '''            if (static_cast<size_t> (numSamples) * static_cast<size_t> (numChannelsRunning)
                * sizeof (float) > scratch.getSize())
                return false;''')
    text = replace_once(text, '    int underrunCount = 0, overrunCount = 0;', '    std::atomic<int> underrunCount { 0 }, overrunCount { 0 };')
    text = replace_once(text, '        error.clear();\n        sampleRate = newSampleRate;', '        error.clear();\n        workerFailed.store (false);\n        sampleRate = newSampleRate;')
    text = replace_once(text, '        audioIoInProgress = false;\n    }\n\n    int getBitDepth()', '        audioIoInProgress = false;\n        workerFailed.store (! threadShouldExit());\n    }\n\n    int getBitDepth()')
    text = replace_once(text, '    guitarrack::CallbackGate<AudioIODeviceCallback> callback;', '    guitarrack::CallbackGate<AudioIODeviceCallback> callback;\n    std::atomic<bool> workerFailed { false };')
    text = replace_once(text, 'return isStarted && internal.error.isEmpty();', 'return isStarted && ! internal.workerFailed.load();')
    return text


def jack(text):
    text = replace_once(text, 'namespace juce\n{', '#include "guitarrack_CallbackGate.h"\n\nnamespace juce\n{')
    text = replace_once(text, 'newCallback != callback)', 'newCallback != callback.get())')
    text = replace_once(text, '            if (newCallback != nullptr)\n                newCallback->audioDeviceAboutToStart (this);\n\n            AudioIODeviceCallback* const oldCallback = callback;\n\n            {\n                const ScopedLock sl (callbackLock);\n                callback = newCallback;\n            }', '            auto* oldCallback = callback.get();\n            callback.replace (nullptr);\n\n            if (newCallback != nullptr)\n                newCallback->audioDeviceAboutToStart (this);\n\n            callback.replace (newCallback);')
    text = replace_once(text, 'return callback != nullptr;', 'return callback.get() != nullptr;')
    text = replace_once(text, '    void process (const int numSamples)\n    {', '''    void process (const int numSamples)
    {
        guitarrack::CallbackGate<AudioIODeviceCallback>::Lease lease (callback);
        auto* activeCallback = lease.get();
        if (activeCallback == nullptr)
        {
            // Do not read masks while the control thread changes them detached.
            for (auto* port : outputPorts)
                if (auto* data = juce::jack_port_get_buffer (port, static_cast<jack_nframes_t> (numSamples)))
                    zeromem (data, static_cast<size_t> (numSamples) * sizeof (float));
            return;
        }''')
    text = replace_once(text, '        const ScopedLock sl (callbackLock);\n\n        if (callback != nullptr)', '        if (activeCallback != nullptr)')
    text = replace_once(text, '                callback->audioDeviceIOCallbackWithContext', '                activeCallback->audioDeviceIOCallbackWithContext')
    text = replace_once(text, '            AudioIODeviceCallback* const oldCallback = callback;', '            AudioIODeviceCallback* const oldCallback = callback.get();')
    text = replace_once(text, '    AudioIODeviceCallback* callback = nullptr;\n    CriticalSection callbackLock;', '    guitarrack::CallbackGate<AudioIODeviceCallback> callback;')
    text = replace_once(text, '        if (deviceIsOpen && newCallback != callback.get())', '        if (deviceIsOpen && ! serverShutdown.load() && newCallback != callback.get())')
    text = replace_once(text, '    void stop() override\n    {\n        start (nullptr);\n    }', '''    void stop() override
    {
        auto* oldCallback = callback.get();
        callback.replace (nullptr);
        if (oldCallback != nullptr)
            oldCallback->audioDeviceStopped();
    }''')
    text = replace_once(text, 'return deviceIsOpen;', 'return deviceIsOpen && ! serverShutdown.load();')
    text = replace_once(text, 'return callback.get() != nullptr;', 'return callback.get() != nullptr && ! serverShutdown.load();')
    text = replace_once(text, '    void close() override\n    {\n        stop();', '''    void close() override
    {
        stop();
        if (serverShutdown.load())
            client = nullptr;''')
    text = replace_once(text, '            device->client = nullptr;\n            device->close();', '            device->serverShutdown.store (true);')
    text = replace_once(text, '    bool deviceIsOpen = false;', '    bool deviceIsOpen = false;\n    std::atomic<bool> serverShutdown { false };')
    return text


def main():
    root, project = map(Path, sys.argv[1:])
    native = root / 'modules/juce_audio_devices/native'
    records = [
        ('juce_ALSA_linux.cpp', '02669a73d33c23185bfcbdb23d2c27951321e05b9702b4a44513f3342323fa4d', '4f4027e8d62fa4bdbb66589c62ff474bca90cd01a13bec82773cea46bfd11849', alsa),
        ('juce_JackAudio.cpp', '2d82dfa971d98ac10a021092c1026aca11ba7475a01f48577ddf091fd074c167', '9cbcd215b83b02b305772330381fe11331d7a3b27da7e1523fc4fccbe2b2480b', jack),
    ]
    # Validate both files before mutating either one.
    writes = []
    for name, expected, patched_hash, transform in records:
        path = native / name
        text = path.read_text().rstrip() + '\n'
        marker = '// GuitarRack callback gate patch for JUCE 8.0.15\n'
        if text.startswith(marker):
            if hashlib.sha256(text.encode()).hexdigest() != patched_hash:
                raise RuntimeError(f'Unexpected modified JUCE source: {name}')
            continue
        if hashlib.sha256(text.encode()).hexdigest() != expected:
            raise RuntimeError(f'Unexpected JUCE source: {name}; exact 8.0.15 required')
        patched = marker + transform(text)
        if 'callbackLock' in patched:
            raise RuntimeError(f'Unpatched callback lock: {name}')
        writes.append((path, patched))
    for path, text in writes:
        path.write_text(text)
    header = native / 'guitarrack_CallbackGate.h'
    contents = (project / 'src/core/CallbackGate.h').read_text()
    if not header.exists() or header.read_text() != contents:
        header.write_text(contents)


if __name__ == '__main__':
    main()
