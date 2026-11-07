# Third-party sources and licensing

## S.A.M. speech engine

The vendored engine comes from **ESP8266SAM 1.1.0**, by **Earle F. Philhower,
III**, at commit `bb3e2559387dc89b43cc1a6c37abaa02eb8410d1`:

- [ESP8266SAM source](https://github.com/earlephilhower/ESP8266SAM/tree/bb3e2559387dc89b43cc1a6c37abaa02eb8410d1)
- [Sebastian Macke's original C adaptation](https://github.com/s-macke/SAM)
- Original S.A.M. software published by **Don't Ask Software**.

[UPSTREAM.md](lib/ESP8266SAM/UPSTREAM.md) is an unchanged copy of the
upstream README, including its license statement. Upstream does not provide
a clear redistribution license for the reverse-engineered S.A.M. engine.
The historic descriptions of abandonware and possible fair use in that
README are upstream's statements, not a license grant from this project.
No blanket open-source license is asserted for this repository.

The Talko changes by Jean-Luc Deladrière / Polaxis are:

| File | Changes from the upstream commit |
|---|---|
| `lib/ESP8266SAM/src/render.c` | Reset output state, live speed/pitch hooks, gate cancellation, Growl hook, and phoneme tracking. |
| `lib/ESP8266SAM/src/sam.c` | Output reset, phoneme-code export, parser bounds and phoneme-length fixes. |
| `lib/ESP8266SAM/src/SamData.h` | Add the output phoneme field. The original chris.l author notice is retained. |
| `lib/ESP8266SAM/src/render.h` | Declare output reset. |
| `lib/ESP8266SAM/src/sam.h` | Declare phoneme-code export. |
| `lib/ESP8266SAM/src/TalkoSamControls.h` | New hooks connecting the engine to Talko controls and effects. |

`ReciterTabs.h`, `RenderTabs.h`, `SamTabs.h`, `reciter.c`, `reciter.h`, and
both `esp8266sam_debug` files match that upstream commit byte for byte.
The upstream `ESP8266SAM.cpp` and `ESP8266SAM.h` wrapper files are not
included; Talko calls the vendored C engine directly.

## Raspberry Pi WS2812 PIO helper

[src/ws2812.pio.h](src/ws2812.pio.h) contains a generated WS2812 program
and initialization helper derived from the Raspberry Pi Pico examples:

- [WS2812 example source](https://github.com/raspberrypi/pico-examples/blob/master/pio/ws2812/ws2812.pio)
- Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
- BSD 3-Clause license, reproduced in
  [Pico-examples-BSD-3-Clause.txt](third_party/licenses/Pico-examples-BSD-3-Clause.txt).

## Build framework and SDK

The firmware links [Arduino-Pico 4.7.0](https://github.com/earlephilhower/arduino-pico/tree/b8864711cd7801b7062c10f6b84d67f90284723b),
including its RP2040 SDK support. Arduino-Pico's LGPL 2.1 text is included in
[Arduino-Pico-LGPL-2.1.txt](third_party/licenses/Arduino-Pico-LGPL-2.1.txt).
See that framework's [source and dependency notices](https://github.com/earlephilhower/arduino-pico/blob/b8864711cd7801b7062c10f6b84d67f90284723b/README.md)
for its components and their separate licenses. The pinned PlatformIO build
configuration and project sources allow rebuilding and relinking the firmware.
