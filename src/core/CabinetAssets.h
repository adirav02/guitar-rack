#pragma once
#include "Impulse.h"
namespace guitarrack {
struct BuiltinCabinet {std::string id,name;};
const std::vector<BuiltinCabinet>& builtinCabinets();
std::vector<std::uint8_t> builtinIRBytes(const std::string& id); // Off audio; empty if unknown.
}
