#pragma once
#include "core/Effects.h"
namespace guitarrack {
std::vector<EffectDescriptor> amplifierDescriptors();
EffectDescriptor dualAmpDescriptor();
std::vector<EffectDescriptor> driveDescriptors();
EffectDescriptor graphicEQDescriptor();
EffectDescriptor parametricEQDescriptor();
EffectDescriptor noiseGateDescriptor();
}
