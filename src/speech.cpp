#include "speech.h"
#include "fx.h"

#include <Arduino.h>
#include <SamData.h>
#include <TalkoSamControls.h>
#include <atomic>
#include <cstring>
#include <hardware/clocks.h>
#include <hardware/pwm.h>
#include <pico/util/queue.h>
#include <reciter.h>
#include <sam.h>

namespace {

constexpr uint AudioPin = 1;
constexpr uint CarrierCycles = 4;
constexpr uint QueueSize = 256;
constexpr uint QueueMask = QueueSize - 1;
constexpr uint HistorySize = 4096;
constexpr uint HistoryMask = HistorySize - 1;
constexpr uint MinimumHistory = 128;
constexpr uint DefaultLoopLength = 1024;
constexpr uint AutomaticLoopLength = 512;
constexpr uint MinimumLoopLength = 32;
constexpr uint FractionBits = 8;
constexpr uint UnitScale = 1u << FractionBits;
constexpr uint DefaultControls = (UnitScale << 16) | UnitScale;
constexpr uint8_t Silence = 128;

// The PWM interrupt consumes PCM from startup or the rendering core.
uint8_t pcm[QueueSize];
uint8_t pcmPhonemes[QueueSize];
std::atomic<uint8_t> playedPhoneme{0};
std::atomic<uint32_t> writeIndex{0};
std::atomic<uint32_t> readIndex{0};

// The UI advances the epoch at gate cutoff. Both cores discard old speech.
std::atomic<uint32_t> gateEpoch{0};
std::atomic<uint32_t> renderEpoch{0};
uint32_t historyEpoch = 0;

uint8_t history[HistorySize];
uint historyWrite = 0;
uint historyCount = 0;
uint replayPhase = 0;
uint speechSinceGlitch = 0;
uint glitchRemaining = 0;
uint32_t glitchRandom = 0x389ac231;
uint8_t carrierPhase = 0;
uint audioSlice;
uint audioChannel;
uint32_t pwmPeriod;

std::atomic<bool> audioReady{false};
std::atomic<bool> rendering{false};
std::atomic<bool> errorHeld{false};
std::atomic<bool> stuttering{false};

// Pack two Q8 scale factors into a single word for a coherent read on core 1.
std::atomic<uint32_t> controls{DefaultControls};
uint32_t controlsAtErrorPress = DefaultControls;  // Written on the panel core.
std::atomic<uint32_t> replayControls{DefaultControls};

struct Phrase {
  const char *text;
  const char *label;
  bool phonetic;
  uint8_t voice;
  uint32_t epoch;
};

queue_t phrases;
queue_t consoleLabels;
SamData engineData;

bool automaticStutter(uint16_t interval, bool manual, bool hasSpeech) {
  if (interval == 0 || manual || !hasSpeech) {
    speechSinceGlitch = 0;
    glitchRemaining = 0;
    return false;
  }
  if (interval == GlitchHold) {
    speechSinceGlitch = 0;
    glitchRemaining = 0;
    return historyCount >= MinimumHistory;
  }
  if (glitchRemaining > 0) {
    --glitchRemaining;
    return true;
  }
  if (historyCount < MinimumHistory || ++speechSinceGlitch < interval) {
    return false;
  }

  speechSinceGlitch = 0;
  glitchRandom ^= glitchRandom << 13;
  glitchRandom ^= glitchRandom >> 17;
  glitchRandom ^= glitchRandom << 5;
  // Repeat a short fragment for about 45–90 ms, then resume the phrase.
  glitchRemaining = 1000 + (glitchRandom & 1023);
  return true;
}

uint8_t nextStutterSample(bool manual) {
  uint32_t replay = manual ? replayControls.load(std::memory_order_relaxed)
                           : DefaultControls;
  uint lengthScale = replay & 0xffff;
  uint step = replay >> 16;
  uint baseLength = min(historyCount, manual ? DefaultLoopLength : AutomaticLoopLength);
  uint length = constrain((baseLength * lengthScale) >> FractionBits,
                          MinimumLoopLength, historyCount);
  uint limit = length << FractionBits;
  if (replayPhase >= limit) {
    replayPhase = 0;
  }

  uint first = (historyWrite + HistorySize - length) & HistoryMask;
  uint offset = replayPhase >> FractionBits;
  uint fraction = replayPhase & (UnitScale - 1);
  uint a = history[(first + offset) & HistoryMask];
  uint b = history[offset + 1 == length ? first : (first + offset + 1) & HistoryMask];
  uint sample = (a * (UnitScale - fraction) + b * fraction + UnitScale / 2) >> FractionBits;

  replayPhase += step;
  if (replayPhase >= limit) {
    replayPhase -= limit;
  }
  return sample;
}

uint8_t nextSample(uint16_t glitchInterval) {
  uint32_t write = writeIndex.load(std::memory_order_acquire);
  uint32_t epoch = gateEpoch.load(std::memory_order_relaxed);
  if (historyEpoch != epoch) {
    historyEpoch = epoch;
    historyCount = 0;
    speechSinceGlitch = 0;
    glitchRemaining = 0;
    replayPhase = 0;
    stuttering.store(false, std::memory_order_relaxed);
  }
  if (renderEpoch.load(std::memory_order_relaxed) != epoch) {
    playedPhoneme.store(0, std::memory_order_relaxed);
    readIndex.store(write, std::memory_order_release);
    return Silence;
  }

  uint32_t read = readIndex.load(std::memory_order_relaxed);
  bool manual = errorHeld.load(std::memory_order_relaxed);
  bool automatic = automaticStutter(glitchInterval, manual, read != write);
  if (historyCount >= MinimumHistory && (manual || automatic)) {
    if (replayPhase == 0) {
      stuttering.store(true, std::memory_order_relaxed);
    }
    return nextStutterSample(manual);
  }

  if (stuttering.load(std::memory_order_relaxed)) {
    stuttering.store(false, std::memory_order_relaxed);
    replayPhase = 0;
  }

  if (read == write) {
    playedPhoneme.store(0, std::memory_order_relaxed);
    historyCount = 0;
    return Silence;
  }
  uint8_t sample = pcm[read & QueueMask];
  playedPhoneme.store(pcmPhonemes[read & QueueMask], std::memory_order_relaxed);
  readIndex.store(read + 1, std::memory_order_release);
  history[historyWrite] = sample;
  historyWrite = (historyWrite + 1) & HistoryMask;
  if (historyCount < HistorySize) {
    ++historyCount;
  }
  return sample;
}

void __isr __not_in_flash_func(audioInterrupt)() {
  pwm_clear_irq(audioSlice);
  if (++carrierPhase == CarrierCycles) {
    carrierPhase = 0;
    FxSettings fx = effectSettings();
    uint8_t sample = nextSample(fx.effect == Fx::Glitch ? fx.parameter : 0);
    sample = processOutputFx(sample, fx);
    pwm_set_chan_level(audioSlice, audioChannel, (sample * pwmPeriod) >> 8);
  }
}

void outputSample(void *context, unsigned char sample) {
  uint32_t write = writeIndex.load(std::memory_order_relaxed);
  while (write - readIndex.load(std::memory_order_acquire) >= QueueSize) {
    if (TalkoSamCancelled()) {
      return;
    }
    tight_loop_contents();
  }
  if (TalkoSamCancelled()) {
    return;
  }
  pcm[write & QueueMask] = sample;
  pcmPhonemes[write & QueueMask] = engineData.render.outputPhoneme;
  writeIndex.store(write + 1, std::memory_order_release);

  if (context) {
    auto &label = *static_cast<const char **>(context);
    if (label) {
      // Report the first sample of a started phrase, never a pending trigger.
      queue_try_add(&consoleLabels, &label);
      label = nullptr;
    }
  }
}

void speak(Phrase phrase) {
  char input[256] = {};
  size_t length = strlen(phrase.text);
  if (length > sizeof(input) - 3) {
    return;
  }

  engineData = {};
  resetGrowl();
  memcpy(input, phrase.text, length);
  input[length] = phrase.phonetic ? '\x9b' : '[';
  if (!phrase.phonetic && !TextToPhonemes(input)) {
    return;
  }

  const Voice &voice = voices[phrase.voice];
  SetSpeed(voice.speed);
  SetPitch(voice.pitch);
  SetMouth(voice.mouth);
  SetThroat(voice.throat);
  SetInput(input);
  SAMMain(outputSample, &phrase.label);
}

void drainSamples() {
  while (readIndex.load(std::memory_order_acquire) !=
         writeIndex.load(std::memory_order_relaxed)) {
    tight_loop_contents();
  }
}

} // namespace

SamData *samdata = &engineData;
bool core1_separate_stack = true;

void setupSpeech() {
  setupEffects();
  queue_init(&phrases, sizeof(Phrase), 1);
  queue_init(&consoleLabels, sizeof(const char *), 8);

  audioSlice = pwm_gpio_to_slice_num(AudioPin);
  audioChannel = pwm_gpio_to_channel(AudioPin);
  constexpr uint carrierRate = SpeechSampleRate * CarrierCycles;
  pwmPeriod = (clock_get_hz(clk_sys) + carrierRate / 2) / carrierRate;

  pwm_config config = pwm_get_default_config();
  pwm_config_set_wrap(&config, pwmPeriod - 1);
  pwm_init(audioSlice, &config, false);
  pwm_set_chan_level(audioSlice, audioChannel, pwmPeriod / 2);
  gpio_set_function(AudioPin, GPIO_FUNC_PWM);

  irq_set_exclusive_handler(PWM_IRQ_WRAP, audioInterrupt);
  pwm_clear_irq(audioSlice);
  pwm_set_irq_enabled(audioSlice, true);
  irq_set_enabled(PWM_IRQ_WRAP, true);
  pwm_set_enabled(audioSlice, true);
}

void speakStartup(const char *phrase) {
  speak({phrase, nullptr, false, 0, 0});
  drainSamples();
}

void startSpeechWorker() {
  audioReady.store(true, std::memory_order_release);
}

void setup1() {
  while (!audioReady.load(std::memory_order_acquire)) {
    tight_loop_contents();
  }
}

void loop1() {
  Phrase phrase;
  queue_remove_blocking(&phrases, &phrase);
  if (phrase.epoch != gateEpoch.load(std::memory_order_acquire)) {
    return;
  }
  renderEpoch.store(phrase.epoch, std::memory_order_relaxed);
  rendering.store(true, std::memory_order_relaxed);
  speak(phrase);
  drainSamples();
  rendering.store(false, std::memory_order_relaxed);
}

void triggerSpeech(const char *phrase, const char *label, bool phonetic, uint8_t voice) {
  Phrase next{phrase, label, phonetic, voice, gateEpoch.load(std::memory_order_relaxed)};
  // Keep only the most recent pending trigger during a phrase.
  Phrase pending;
  queue_try_remove(&phrases, &pending);
  queue_try_add(&phrases, &next);
}

void serviceSpeechConsole() {
  constexpr char Request[] = "\x1eSAM\x1f";
  static uint8_t line[64];
  static size_t sent = 0;
  static size_t length = 0;
  static size_t requestOffset = 0;
  static bool mouthEnabled = false;
  static uint8_t lastPhoneme = 255;
  const char *label;

  if (!Serial) {
    sent = length;
    requestOffset = 0;
    mouthEnabled = false;
    queue_try_remove(&consoleLabels, &label);
    return;
  }

  // Console requests framed mouth data. Ordinary USB serial monitors get labels.
  for (unsigned i = 0; i < 16 && Serial.available(); ++i) {
    int byte = Serial.read();
    if (byte == Request[requestOffset]) {
      if (++requestOffset == sizeof(Request) - 1) {
        mouthEnabled = true;
        lastPhoneme = 255; // Send the current state as soon as the request arrives.
        requestOffset = 0;
      }
    } else {
      requestOffset = (byte == Request[0]) ? 1 : 0;
    }
  }

  int available = Serial.availableForWrite();
  if (available <= 0) {
    return;
  }
  if (sent == length) {
    uint8_t phoneme = playedPhoneme.load(std::memory_order_relaxed);
    if (mouthEnabled && phoneme != lastPhoneme) {
      char code[3];
      GetPhonemeCode(phoneme, code);
      memcpy(line, "\x1eSAM:", 5);
      length = 5 + strlen(code);
      memcpy(line + 5, code, length - 5);
      line[length++] = 0x1f;
      lastPhoneme = phoneme;
    } else if (queue_try_remove(&consoleLabels, &label)) {
      size_t textLength = min(strlen(label), sizeof(line) - 2);
      memcpy(line, label, textLength);
      line[textLength] = '\r';
      line[textLength + 1] = '\n';
      length = textLength + 2;
    } else {
      return;
    }
    sent = 0;
  }
  sent += Serial.write(line + sent, min(length - sent, size_t(available)));
}

void stopSpeech() {
  uint32_t epoch = gateEpoch.load(std::memory_order_relaxed);
  gateEpoch.store(epoch + 1, std::memory_order_release);
}

void setSpeechControls(uint16_t speedScale, uint16_t pitchScale) {
  controls.store((uint32_t(pitchScale) << 16) | speedScale, std::memory_order_relaxed);
  if (errorHeld.load(std::memory_order_relaxed)) {
    // Relative movement keeps the captured sound's original pitch at the press.
    uint initialSpeed = controlsAtErrorPress & 0xffff;
    uint initialPitch = controlsAtErrorPress >> 16;
    uint lengthScale = constrain((speedScale * UnitScale) / initialSpeed,
                                 UnitScale / 4, UnitScale * 4);
    uint step = constrain((initialPitch * UnitScale) / pitchScale,
                          UnitScale / 4, UnitScale * 4);
    replayControls.store((step << 16) | lengthScale, std::memory_order_relaxed);
  }
}

void setErrorHeld(bool held) {
  if (held) {
    controlsAtErrorPress = controls.load(std::memory_order_relaxed);
    replayControls.store(DefaultControls, std::memory_order_relaxed);
  }
  errorHeld.store(held, std::memory_order_relaxed);
}

bool isSpeaking() {
  return rendering.load(std::memory_order_relaxed);
}

bool isStuttering() {
  return stuttering.load(std::memory_order_relaxed);
}

int TalkoSamCancelled() {
  return renderEpoch.load(std::memory_order_relaxed) !=
         gateEpoch.load(std::memory_order_acquire);
}

uint16_t TalkoSamSpeed(unsigned char baseSpeed) {
  uint scale = controls.load(std::memory_order_relaxed) & 0xffff;
  return max(1u, (baseSpeed * scale + 128) >> 8);
}

uint16_t TalkoSamPitchPeriod(unsigned char framePeriod) {
  uint scale = controls.load(std::memory_order_relaxed) >> 16;
  // The original 8-bit pulse counter treats zero as 256 ticks.
  uint period = framePeriod ? framePeriod : 256u;
  return max(1u, (period * scale + 128) >> 8);
}
