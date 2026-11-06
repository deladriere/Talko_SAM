#include <Arduino.h>
#include <cmath>
#include <hardware/gpio.h>
#include <hardware/pwm.h>
#include <hardware/sync.h>

#include "banks.h"
#include "fx.h"
#include "speech.h"
#include "ws2812.pio.h"

namespace {

constexpr uint ErrorPin = 0;
constexpr uint GatePin = 2;
constexpr uint EncoderA = 3;
constexpr uint EncoderB = 4;
constexpr uint SegmentA = 5;
constexpr uint DecimalPoint = 12;
constexpr uint SelectPin = 13;
constexpr uint ModeB = 14;
constexpr uint ModeA = 15;
constexpr uint NeoPixelPin = 16;
constexpr uint NeoPixelStateMachine = 0;
constexpr uint NeoPixelFrequency = 800000;
constexpr uint SoundPot = 26;
constexpr uint FxPot = 27;
constexpr uint SpeedPot = 28;
constexpr uint PitchPot = 29;

constexpr uint DisplayBrightness = 100;
constexpr uint IdleBrightness = 40;
constexpr uint IdleMs = 5 * 60 * 1000;
constexpr uint SelectDebounceMs = 10;
constexpr uint ErrorDebounceMs = 4;
constexpr uint GateDebounceUs = 4000;
constexpr uint VoiceHoldMs = 250;
constexpr uint VoiceBlinkHalfMs = 85;
constexpr uint EncoderSettleMs = 25;
constexpr uint AdcCenter = 2048;
constexpr uint AdcMax = 4095;
constexpr uint SoundMax = 4000;
constexpr uint8_t VersionMajor = 0;
constexpr uint8_t VersionMinor = 7;

// Common cathode display masks, from segment A in bit 0 to G in bit 6.
constexpr uint8_t Digits[] = {
    0x3f, 0x06, 0x5b, 0x4f, 0x66, 0x6d, 0x7d, 0x07, 0x7f, 0x6f,
};
constexpr uint8_t VoiceGlyphs[] = {
    0x6d, // S, S.A.M.
    0x79, // E, Elf
    0x50, // r, Robot
    0x78, // t, Stuffy
    0x38, // L, Little Old Lady
    0x77, // A, Alien
};
static_assert(sizeof(VoiceGlyphs) == voiceCount);
constexpr uint8_t FxGlyphs[] = {
    0x40, // -, Off
    0x5c, // o, Ring Mod
    0x50, // r, Reverb
    0x5e, // d, Delay
    0x79, // E, Glitch
    0x6d, // S, Saturation
    0x7d, // G, Growl
};
static_assert(sizeof(FxGlyphs) == unsigned(Fx::Count));
constexpr uint8_t SnakeSegments[] = {0, 1, 6, 4, 3, 2, 6, 5};

struct Button {
  bool raw = false;
  bool held = false;
  uint32_t changedAt = 0;

  bool update(bool pressed, uint32_t now, uint32_t debounceMs) {
    if (pressed != raw) {
      raw = pressed;
      changedAt = now;
    }
    if (held == raw || now - changedAt < debounceMs) {
      return false;
    }
    held = raw;
    return true;
  }
};

struct Pot {
  int filtered = 0;
  int lastActivity = 0;

  void begin(uint pin) {
    lastActivity = analogRead(pin);
    filtered = lastActivity * 8;
  }

  bool update(uint pin) {
    filtered += analogRead(pin) - value();
    if (abs(value() - lastActivity) < 32) {
      return false;
    }
    lastActivity = value();
    return true;
  }

  int value() const { return filtered / 8; }
};

Button selectButton;
Button error;
Pot sound;
Pot fxPot;
Pot speedPot;
Pot pitchPot;

uint8_t selectedBank = 0;
uint8_t selectedVoice = 0;
uint8_t pendingVoice = 0;
uint8_t lastEncoderState = 0;
int8_t encoderSteps = 0;
volatile int encoderDetents = 0;
bool voiceGesture = false;
bool fxPage = false;
Fx selectedFx = Fx::Off;
bool gateWasClosed = false;
uint32_t selectDownAt = 0;
uint32_t voiceTurnAt = 0;
uint32_t lastActivityAt = 0;
uint32_t lastPotReadAt = 0;

enum GateEvent : uint8_t { GateOpened = 1, GateClosed = 2 };
volatile uint8_t gateEvents = 0;
uint32_t lastGateEdge = 0;

void gateInterrupt() {
  // The input transistor inverts the external gate.
  if (gpio_get(GatePin)) {
    gateEvents |= GateClosed;
    return;
  }
  uint32_t now = micros();
  if (now - lastGateEdge >= GateDebounceUs) {
    lastGateEdge = now;
    gateEvents |= GateOpened;
  }
}

uint8_t takeGateEvents() {
  uint32_t state = save_and_disable_interrupts();
  uint8_t events = gateEvents;
  gateEvents = 0;
  restore_interrupts(state);
  return events;
}

uint8_t encoderState() {
  uint32_t pins = gpio_get_all();
  return (((pins >> EncoderA) & 1u) << 1) | ((pins >> EncoderB) & 1u);
}

void encoderInterrupt() {
  // Same full quadrature table as Talko2. Listen to both channels.
  constexpr int8_t transitions[16] = {
       0, -1,  1,  0,
       1,  0,  0, -1,
      -1,  0,  0,  1,
       0,  1, -1,  0,
  };
  uint8_t state = encoderState();
  encoderSteps += transitions[(lastEncoderState << 2) | state];
  lastEncoderState = state;
  if (encoderSteps == 4 || encoderSteps == -4) {
    encoderDetents += encoderSteps > 0 ? 1 : -1;
    encoderSteps = 0;
  }
}

int takeEncoderMovement() {
  uint32_t state = save_and_disable_interrupts();
  int movement = encoderDetents;
  encoderDetents = 0;
  restore_interrupts(state);
  return movement;
}

uint16_t periodScale(int adc) {
  // Two octaves either side of the preset, with a deadband at noon.
  constexpr int deadband = 55;
  int offset = int(AdcCenter) - adc;
  if (abs(offset) <= deadband) {
    return 256;
  }
  offset += offset > 0 ? -deadband : deadband;
  float exponent = (2.0f * offset) / (AdcCenter - deadband);
  return uint16_t(256.0f * exp2f(exponent) + 0.5f);
}

void updatePots(uint32_t now) {
  if (now - lastPotReadAt < 2) {
    return;
  }
  lastPotReadAt = now;
  bool soundMoved = sound.update(SoundPot);
  bool fxMoved = fxPot.update(FxPot);
  bool speedMoved = speedPot.update(SpeedPot);
  bool pitchMoved = pitchPot.update(PitchPot);
  if (soundMoved || fxMoved || speedMoved || pitchMoved) {
    lastActivityAt = now;
  }
  setSpeechControls(periodScale(speedPot.value()), periodScale(pitchPot.value()));
  setEffect(selectedFx, fxPot.value());
  serviceSpeechConsole();
}

void drawDisplay(uint8_t mask, bool dot, uint brightness) {
  static uint16_t lastDisplay = 0xffff;
  uint16_t display = (brightness << 8) | (dot ? 0x80 : 0) | mask;
  if (display == lastDisplay) {
    return;
  }
  lastDisplay = display;

  for (uint segment = 0; segment < 7; ++segment) {
    pwm_set_gpio_level(SegmentA + segment, (mask & (1u << segment)) ? brightness : 0);
  }
  pwm_set_gpio_level(DecimalPoint, dot ? DisplayBrightness : 0);
}

void flashVersionLed(uint32_t grb, uint8_t count) {
  for (uint8_t i = 0; i < count; ++i) {
    pio_sm_put_blocking(pio0, NeoPixelStateMachine, grb << 8);
    delay(150);
    pio_sm_put_blocking(pio0, NeoPixelStateMachine, 0);
    delay(300);
  }
}

void showVersionLed() {
  uint offset = pio_add_program(pio0, &ws2812_program);
  ws2812_program_init(pio0, NeoPixelStateMachine, offset, NeoPixelPin,
                      NeoPixelFrequency, false);
  pio_sm_put_blocking(pio0, NeoPixelStateMachine, 0);
  delay(5);

  // Talko2 uses red for major, blue for the separator, and green for minor.
  flashVersionLed(100u << 8, VersionMajor);
  delay(200);
  flashVersionLed(255u, 1);
  delay(200);
  flashVersionLed(100u << 16, VersionMinor);
  delay(200);
}

void announceVersion() {
  speakStartup("VERSION");
  drawDisplay(Digits[VersionMajor], false, DisplayBrightness);
  speakStartup(numbers[VersionMajor]);
  delay(500);
  drawDisplay(0, true, DisplayBrightness);
  speakStartup("POINT");
  delay(500);
  drawDisplay(Digits[VersionMinor], false, DisplayBrightness);
  speakStartup(numbers[VersionMinor]);
  delay(500);
}

void updateDisplay(uint32_t now, uint8_t voice) {
  if (now - lastActivityAt >= IdleMs) {
    uint index = ((now - lastActivityAt - IdleMs) / 300) % sizeof(SnakeSegments);
    drawDisplay(1u << SnakeSegments[index], false, IdleBrightness);
    return;
  }

  uint8_t mask = Digits[selectedBank];
  bool dot = false;
  bool voiceHold = selectButton.held && now - selectDownAt >= VoiceHoldMs;
  if (voiceGesture || voiceHold) {
    mask = VoiceGlyphs[voice];
    uint32_t startedAt = voiceGesture ? voiceTurnAt : selectDownAt + VoiceHoldMs;
    dot = ((now - startedAt) / VoiceBlinkHalfMs) % 2 == 0;
  } else if (fxPage) {
    mask = FxGlyphs[unsigned(selectedFx)];
    dot = true;
  }
  // The digit signals ERROR so the dot always identifies the encoder mode.
  uint brightness = isStuttering() && (now / 55) % 2
                        ? DisplayBrightness / 4
                        : DisplayBrightness;
  drawDisplay(mask, dot, brightness);
}

} // namespace

void setup() {
  pinMode(NeoPixelPin, OUTPUT);
  digitalWrite(NeoPixelPin, LOW);
  pinMode(GatePin, INPUT_PULLUP);
  pinMode(ErrorPin, INPUT_PULLUP);
  pinMode(SelectPin, INPUT_PULLUP);
  pinMode(EncoderA, INPUT_PULLUP);
  pinMode(EncoderB, INPUT_PULLUP);
  pinMode(ModeA, INPUT_PULLUP);
  pinMode(ModeB, INPUT_PULLUP);

  for (uint pin = SegmentA; pin <= DecimalPoint; ++pin) {
    uint slice = pwm_gpio_to_slice_num(pin);
    pwm_set_wrap(slice, 255);
    pwm_set_gpio_level(pin, 0);
    gpio_set_function(pin, GPIO_FUNC_PWM);
    pwm_set_enabled(slice, true);
  }

  analogReadResolution(12);
  sound.begin(SoundPot);
  fxPot.begin(FxPot);
  speedPot.begin(SpeedPot);
  pitchPot.begin(PitchPot);
  showVersionLed();
  setupSpeech();
  announceVersion();
  startSpeechWorker();
  setSpeechControls(periodScale(speedPot.value()), periodScale(pitchPot.value()));
  lastGateEdge = micros() - GateDebounceUs;
  attachInterrupt(digitalPinToInterrupt(GatePin), gateInterrupt, CHANGE);
  delay(EncoderSettleMs);
  lastEncoderState = encoderState();
  attachInterrupt(digitalPinToInterrupt(EncoderA), encoderInterrupt, CHANGE);
  attachInterrupt(digitalPinToInterrupt(EncoderB), encoderInterrupt, CHANGE);
  lastActivityAt = millis();
}

void loop() {
  uint32_t now = millis();
  updatePots(now);
  bool selectChanged = selectButton.update(digitalRead(SelectPin) == LOW, now, SelectDebounceMs);
  if (selectChanged && selectButton.held) {
    selectDownAt = now;
  }

  int movement = takeEncoderMovement();
  if (movement != 0) {
    lastActivityAt = now;
    if (selectButton.held || selectButton.raw || voiceGesture) {
      if (!voiceGesture) {
        pendingVoice = selectedVoice;
        voiceTurnAt = now;
      }
      voiceGesture = true;
      int voice = (int(pendingVoice) + movement) % voiceCount;
      pendingVoice = voice < 0 ? voice + voiceCount : voice;
    } else if (fxPage) {
      selectedFx = Fx(constrain(int(selectedFx) + movement, 0, int(Fx::Count) - 1));
      setEffect(selectedFx, fxPot.value());
    } else {
      selectedBank = constrain(int(selectedBank) + movement, 0, int(bankCount) - 1);
    }
  }

  if (!selectButton.held && !selectButton.raw && voiceGesture) {
    selectedVoice = pendingVoice;
    voiceGesture = false;
    fxPage = false;
  } else if (selectChanged && !selectButton.held && now - selectDownAt < VoiceHoldMs) {
    fxPage = !fxPage;
  }

  bool errorChanged = error.update(digitalRead(ErrorPin) == LOW, now, ErrorDebounceMs);
  if (errorChanged) {
    setErrorHeld(error.held);
  }

  uint8_t events = takeGateEvents();
  uint32_t pins = gpio_get_all();
  constexpr uint32_t modePins = (1u << ModeA) | (1u << ModeB);
  bool middlePosition = (pins & modePins) == modePins;
  bool gateActive = (pins & (1u << GatePin)) == 0;
  bool gateClosed = middlePosition && !gateActive;
  if (middlePosition && ((events & GateClosed) || (gateClosed && !gateWasClosed))) {
    stopSpeech();
  }
  gateWasClosed = gateClosed;

  uint8_t voice = voiceGesture ? pendingVoice : selectedVoice;
  bool trigger = (events & GateOpened) && !gateClosed;
  if (trigger) {
    const Bank &current = banks[selectedBank];
    size_t index = (constrain(sound.value(), 0, int(SoundMax)) * current.count) /
                   (SoundMax + 1);
    const char *text = current.entries[index];
    const char *label = current.labels ? current.labels[index] : text;
    triggerSpeech(text, label, current.phonetic, voice);
  }

  if (events || selectChanged || selectButton.held || errorChanged || error.held ||
      isSpeaking() || gateActive) {
    lastActivityAt = now;
  }
  updateDisplay(now, voice);
}
