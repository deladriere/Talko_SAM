#include <cassert>
#include <cstdio>

// Exercise the real PCM consumption, cutoff and console paths. Only hardware
// calls and SDK queues are simulated; this does not measure RP2040 timing.
#include "../../src/speech.cpp"

unsigned tick() {
  for (unsigned i = 0; i < CarrierCycles; ++i) audioInterrupt();
  return (hostPwmLevel * 256u + pwmPeriod / 2) / pwmPeriod;
}

void checkLabels() {
  triggerSpeech("HELLO", "HELLO", false, 0);
  serviceSpeechConsole();
  assert(Serial.tx.empty()); // A waiting trigger has not been uttered yet.

  Phrase next;
  assert(queue_try_remove(&phrases, &next));
  renderEpoch.store(next.epoch);
  Serial.capacity = 0;
  outputSample(&next.label, 200);
  serviceSpeechConsole();
  assert(Serial.tx.empty()); // A busy host must not block the panel.
  Serial.capacity = 64;
  serviceSpeechConsole();
  assert(Serial.tx == "HELLO\r\n");
  serviceSpeechConsole();
  assert(Serial.tx == "HELLO\r\n");
  tick();
  puts("USB serial: one label at phrase start; pending trigger and backpressure checked");
}

void checkGateTail(Fx effect) {
  setEffect(effect, 0);
  tick();
  renderEpoch.store(gateEpoch.load());
  setEffect(effect, effect == Fx::Delay ? 71 : 4095);
  for (unsigned i = 0; i < 5000; ++i) {
    outputSample(nullptr, 230);
    tick();
  }

  // Queue extra dry speech and another phrase, then close the middle-mode gate.
  for (unsigned i = 0; i < 64; ++i) outputSample(nullptr, 230);
  triggerSpeech("GOODBYE", "GOODBYE", false, 0);
  setErrorHeld(true);
  stopSpeech();
  unsigned audibleTail = tick() != Silence;
  assert(readIndex.load() == writeIndex.load());
  assert(historyCount == 0 && playedPhoneme.load() == 0 && !isStuttering());

  const auto stoppedAt = writeIndex.load();
  loop1(); // The stale pending phrase is discarded before rendering.
  assert(writeIndex.load() == stoppedAt);
  for (unsigned i = 0; i < 1000; ++i) audibleTail += tick() != Silence;
  assert(audibleTail > 0);

  // Bypass and switching at zero must still clear old echoes.
  setEffect(effect, 0);
  assert(tick() == Silence);
  setEffect(Fx::Off, 0);
  assert(tick() == Silence);
  setEffect(effect, effect == Fx::Delay ? 71 : 4095);
  for (unsigned i = 0; i < 5000; ++i) assert(tick() == Silence);
  setErrorHeld(false);
  printf("%s: dry speech/pending trigger stop, tail rings out, bypass clears tail\n",
         effect == Fx::Delay ? "Delay" : "Reverb");
}

int main() {
  setupSpeech();
  checkLabels();
  checkGateTail(Fx::Delay);
  checkGateTail(Fx::Reverb);
}
