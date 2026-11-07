#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <banks.h>
#include <speech.h>
#include <fx.h>
#include <SamData.h>
#include <TalkoSamControls.h>
#include <sam.h>
#include <reciter.h>

SamData data;
SamData *samdata = &data;
unsigned scale = 256;
size_t samples = 0;
int TalkoSamCancelled() { return 0; }
uint16_t TalkoSamSpeed(unsigned char base) { return std::max(1u, (base * scale + 128) >> 8); }
uint16_t TalkoSamPitchPeriod(unsigned char base) { return std::max(1u, ((base ? unsigned(base) : 256u) * scale + 128) >> 8); }
void output(void *, unsigned char) { if (++samples > 5000000) std::abort(); }

int main() {
  setupEffects();
  unsigned entries = 0;
  for (const auto &b : banks) entries += b.count;
  printf("Bank count=%zu, entries=%u, voices=%d\n", bankCount, entries, voiceCount);
  unsigned renders = 0;
  for (Fx fx : {Fx::Off, Fx::Growl}) {
    setEffect(fx, 4095);
    for (unsigned s : {64u, 256u, 1024u}) {
      scale = s;
      for (const auto &voice : voices) {
        for (size_t b = 0; b < bankCount; ++b) {
          for (size_t e = 0; e < banks[b].count; ++e) {
            data = {};
            resetGrowl();
            char input[256] = {};
            std::strcpy(input, banks[b].entries[e]);
            input[std::strlen(input)] = banks[b].phonetic ? '\x9b' : '[';
            if (!banks[b].phonetic && !TextToPhonemes(input)) { printf("Reciter failed at %zu:%zu\n", b,e); return 1; }
            SetSpeed(voice.speed); SetPitch(voice.pitch); SetMouth(voice.mouth); SetThroat(voice.throat);
            SetInput(input); samples=0;
            if (!SAMMain(output, nullptr) || !samples) { printf("Render failed at %zu:%zu\n", b,e); return 1; }
            ++renders;
          }
        }
      }
    }
  }
  printf("Completed %u renders with AddressSanitizer\n", renders);
  for (unsigned f = 0; f < unsigned(Fx::Count); ++f) {
    Fx fx = Fx(f);
    setEffect(fx, 0);
    auto settings = effectSettings();
    unsigned changed = 0;
    unsigned first = processOutputFx(255, settings, f + 1);
    changed += first != 255;
    for (unsigned i=1; i<2000; ++i) changed += processOutputFx(128, settings, f + 1) != 128;
    if (changed != 0) { printf("Dry bypass failed for FX %u\n",f); return 1; }
    printf("FX=%u at ADC=0: parameter=%u, impulse=%u, altered samples=%u\n", f, settings.parameter, first, changed);
  }
}
