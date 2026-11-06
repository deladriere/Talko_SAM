#include "fx.h"

#include "speech.h"
#include <SamData.h>
#include <TalkoSamControls.h>

#include <Arduino.h>
#include <atomic>
#include <cmath>

namespace {

constexpr unsigned AdcMaximum = 4095;
constexpr unsigned GlitchCenter = 2048;
constexpr unsigned GlitchHalfWidth = 50;
constexpr unsigned GlitchDecisionMs = 80;
constexpr unsigned DelaySize = 16384;
constexpr unsigned DelayMask = DelaySize - 1;
constexpr int SampleCenter = 128;
constexpr int InternalScale = 128;
constexpr int InternalLimit = SampleCenter * InternalScale;
constexpr int Unity = 256;
constexpr float Tau = 6.28318530718f;

// The upper bits select the effect. The low 16 bits carry its live parameter.
std::atomic<uint32_t> currentFx{0};
int16_t sine[256];
uint16_t ringPhase = 0;

int16_t delayLine[DelaySize];
unsigned delayHead = 0;
unsigned delayValid = 0;
Fx delayEffect = Fx::Off;
uint32_t delayEpoch = 0;

constexpr float Resonance = 0.965f;
constexpr int CoefficientScale = 1 << 14;
constexpr int Damping = int(Resonance * Resonance * CoefficientScale + 0.5f);
int16_t formantCoefficients[256];

struct Resonator {
  int last = 0;
  int beforeLast = 0;
};

Resonator formants[3];
uint32_t noiseState = 0x63e49b27;
int lastNoise = 0;
int previousNoise = 0;

constexpr unsigned samplesForMs(unsigned ms) {
  return (SpeechSampleRate * ms + 500) / 1000;
}

int limitSample(int sample) {
  return constrain(sample, -InternalLimit, InternalLimit - 1);
}

int tap(unsigned offset) {
  return offset <= delayValid ? delayLine[(delayHead - offset) & DelayMask] : 0;
}

void pushDelay(int sample) {
  delayLine[delayHead] = limitSample(sample);
  delayHead = (delayHead + 1) & DelayMask;
  if (delayValid < DelaySize) {
    ++delayValid;
  }
}

int mixSamples(int dry, int wet, int mix) {
  return (dry * (Unity - mix) + wet * mix) >> 8;
}

int noiseExcitation() {
  noiseState ^= noiseState << 13;
  noiseState ^= noiseState >> 17;
  noiseState ^= noiseState << 5;
  int noise = int(noiseState >> 24) - SampleCenter;
  // The two-sample difference gives each resonator a band-pass response.
  int excitation = noise - previousNoise;
  previousNoise = lastNoise;
  lastNoise = noise;
  return excitation;
}

int resonate(Resonator &state, int coefficient, int excitation) {
  int feedback = coefficient * state.last - Damping * state.beforeLast;
  int sample = excitation + ((feedback + CoefficientScale / 2) >> 14);
  state.beforeLast = state.last;
  state.last = constrain(sample, -4096, 4095);
  return state.last;
}

} // namespace

void setupEffects() {
  for (unsigned i = 0; i < 256; ++i) {
    sine[i] = int16_t(sinf(Tau * i / 256.0f) * 32767.0f);
    // SAM advances its formant phase once per voiced tick, about 3.24 PCM samples.
    float angle = Tau * i / (256.0f * 3.24f);
    formantCoefficients[i] =
        int16_t(2.0f * Resonance * cosf(angle) * CoefficientScale);
  }
}

void setEffect(Fx effect, uint16_t adc) {
  static bool wasAtGlitchNoon = false;
  static bool stutterAtNoon = false;
  static uint32_t lastGlitchDecision = 0;

  bool atGlitchNoon = effect == Fx::Glitch && adc >= GlitchCenter - GlitchHalfWidth &&
                      adc <= GlitchCenter + GlitchHalfWidth;
  if (atGlitchNoon) {
    uint32_t now = millis();
    if (!wasAtGlitchNoon || now - lastGlitchDecision >= GlitchDecisionMs) {
      stutterAtNoon = random(2) != 0;
      lastGlitchDecision = now;
    }
  }
  wasAtGlitchNoon = atGlitchNoon;

  unsigned amount = (adc * Unity + AdcMaximum / 2) / AdcMaximum;
  unsigned parameter = 0;
  switch (effect) {
  case Fx::RingMod:
    if (adc > 70) {
      unsigned frequency = 30 + (adc - 70u) * 1170 / (AdcMaximum - 70);
      parameter = (frequency * 65536u + SpeechSampleRate / 2) / SpeechSampleRate;
    }
    break;
  case Fx::Reverb:
    if (adc > 800) {
      parameter = (adc - 800u) * Unity / (AdcMaximum - 800);
    }
    break;
  case Fx::Delay:
    parameter = samplesForMs(40 + 660 * adc / AdcMaximum);
    break;
  case Fx::Glitch:
    if (atGlitchNoon && stutterAtNoon) {
      parameter = GlitchHold;
    } else if (adc > GlitchCenter + GlitchHalfWidth) {
      constexpr unsigned GlitchUpperRange = AdcMaximum - GlitchCenter - GlitchHalfWidth - 1;
      parameter = samplesForMs(120 + 540 * (AdcMaximum - adc) / GlitchUpperRange);
    }
    break;
  case Fx::Saturation:
    parameter = 7 * amount;
    break;
  case Fx::Growl:
    parameter = amount;
    break;
  case Fx::Off:
  case Fx::Count:
    break;
  }
  currentFx.store((uint32_t(effect) << 16) | parameter, std::memory_order_relaxed);
}

FxSettings effectSettings() {
  uint32_t packed = currentFx.load(std::memory_order_relaxed);
  return {Fx(packed >> 16), uint16_t(packed)};
}

uint8_t processOutputFx(uint8_t sample, FxSettings settings, uint32_t gateEpoch) {
  if (settings.effect != delayEffect || gateEpoch != delayEpoch) {
    // Invalidate the old tail in constant time, without clearing RAM in the IRQ.
    delayValid = 0;
    delayEffect = settings.effect;
    delayEpoch = gateEpoch;
  }
  unsigned parameter = settings.parameter;
  if (parameter == 0) {
    delayValid = 0;
    return sample;
  }

  int dry = (int(sample) - SampleCenter) * InternalScale;
  int output = dry;
  switch (settings.effect) {
  case Fx::RingMod:
    output = (dry * sine[ringPhase >> 8]) >> 15;
    ringPhase += parameter;
    break;
  case Fx::Reverb: {
    // Talko2's three feedback taps, scaled to the S.A.M. sample rate.
    int taps = (154 * tap(samplesForMs(80)) + 102 * tap(samplesForMs(160)) +
                77 * tap(samplesForMs(320))) >> 8;
    int decay = 38 + ((166 * parameter) >> 8);
    int wet = (taps * decay) >> 8;
    pushDelay(dry + ((wet * 90) >> 8));
    output = mixSamples(dry, wet, (90 * parameter) >> 8);
    break;
  }
  case Fx::Delay: {
    int wet = tap(parameter);
    pushDelay(dry + ((wet * 110) >> 8));
    output = mixSamples(dry, wet, 96);
    break;
  }
  case Fx::Saturation: {
    int driven = (dry * (Unity + int(parameter))) >> 8;
    constexpr int Knee = InternalLimit * 3 / 4;
    int level = abs(driven);
    if (level > Knee) {
      level = Knee + (level - Knee) / 3;
    }
    output = driven < 0 ? -level : level;
    break;
  }
  case Fx::Off:
  case Fx::Glitch:
  case Fx::Growl:
  case Fx::Count:
    return sample;
  }
  return (limitSample(output) + InternalLimit) >> 7;
}

void resetGrowl() {
  for (auto &formant : formants) {
    formant = {};
  }
  lastNoise = 0;
  previousNoise = 0;
}

void TalkoSamGrowl(uint8_t *samples, uint8_t count, uint8_t frame) {
  FxSettings settings = effectSettings();
  if (settings.effect != Fx::Growl || settings.parameter == 0) {
    return;
  }

  const auto &render = samdata->render;
  const int coefficients[] = {
      formantCoefficients[render.frequency1[frame]],
      formantCoefficients[render.frequency2[frame]],
      formantCoefficients[render.frequency3[frame]],
  };
  const int amplitudes[] = {
      render.amplitude1[frame] & 0x0f,
      render.amplitude2[frame] & 0x0f,
      render.amplitude3[frame] & 0x0f,
  };
  for (unsigned i = 0; i < count; ++i) {
    int excitation = noiseExcitation();
    int noise = 0;
    for (unsigned j = 0; j < 3; ++j) {
      noise += resonate(formants[j], coefficients[j], excitation) * amplitudes[j];
    }
    int dry = int(samples[i]) - SampleCenter;
    int wet = constrain(noise / 128, -128, 127);
    samples[i] = mixSamples(dry, wet, settings.parameter) + SampleCenter;
  }
}
