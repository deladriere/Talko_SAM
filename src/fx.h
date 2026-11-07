#pragma once

#include <stdint.h>

enum class Fx : uint8_t { Off, RingMod, Reverb, Delay, Glitch, Saturation, Growl, Count };

constexpr uint16_t GlitchHold = 1;  // Reserved for stuttering until the next noon decision.

struct FxSettings {
  Fx effect;
  uint16_t parameter;
};

void setupEffects();
void setEffect(Fx effect, uint16_t adc);
FxSettings effectSettings();
uint8_t processOutputFx(uint8_t sample, FxSettings settings);
void resetGrowl();
