#pragma once
#include "core/Effects.h"
namespace guitarrack {
std::vector<EffectDescriptor> creativeDescriptors();
EffectDescriptor echoDescriptor(unsigned version=1);
EffectDescriptor spaceReverbDescriptor();
EffectDescriptor expressionPitchDescriptor();
}
