#pragma once
#include <cstdint>
#include <string>
#include <vector>
#include <span>

namespace guitarrack {
inline constexpr std::size_t maxIRBytes = 1024 * 1024;
inline constexpr std::size_t maxIRSourceSamples = 96000;
inline constexpr std::size_t maxGraphIRSamples = 768000;
struct ImpulseResponse {
    double sampleRate = 48000;
    std::vector<float> samples;
};
// Off-thread APIs. WAV: little-endian RIFF, mono PCM16/24/32 or IEEE float32.
ImpulseResponse decodeIR(std::span<const std::uint8_t>);
ImpulseResponse resampleIR(const ImpulseResponse&, double targetRate);
std::string sha256(std::span<const std::uint8_t>);
bool validAssetId(const std::string&) noexcept;
std::vector<std::uint8_t> syntheticIRBytes();
const std::string& syntheticIRId();
}
