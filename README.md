# Talko S.A.M. 1.0

Firmware for Talko 2 (RP2040), with ten banks, 161 speech and percussion
entries, six voice presets, live controls, and effects. Speech is synthesized
on the module from text or phonemes.

Read the [user manual](https://www.polaxis.be/talko-sam-user-manual/)
and the [complete vocabulary](docs/VOCABULARY.md).

To install or replace any firmware, put Talko into BOOTSEL mode and copy
[Talko_SAM-1.0.uf2](Talko_SAM-1.0.uf2) onto the `RPI-RP2` drive.
Hold BOOTSEL, press and release RESET, then release BOOTSEL. Leave USB
connected until the copy finishes and the module restarts. Keep the previous
UF2 if you want to switch back.

The firmware includes Arduino-Pico USB serial and its reset-to-BOOTSEL
handler for normal PlatformIO uploads. On macOS, look for the module's
`/dev/cu.usbmodem…` device. The first installation from early prototypes
must use BOOTSEL.

Open this folder in PlatformIO and run `pio run` to rebuild the UF2. The output
is `.pio/build/pico/firmware.uf2`. Use the Upload task or `pio run -t upload`
with the Talko serial port. Open PlatformIO Monitor at 115200 to see one
label per triggered phrase as it starts. Alphabet entries show their
letters, and the vowel and consonant banks show phoneme labels.

The PlatformIO platform is pinned to commit
`4d1ad09d78445f16688627de0819646fd655910e`, which selects Arduino-Pico
4.7.0 (`b8864711cd7801b7062c10f6b84d67f90284723b`). The release was built
with PlatformIO Core 6.2.0 and the RP2040 GCC 14.2.0 toolchain.

For lip sync in Kraftor Visual Console, select **S.A.M.** under **Phoneme
model** in the 2D Mouth or 3D Head menu, then connect to Talko over Serial.
Console requests phoneme events on the same port. Try bank **7** for vowels
and diphthongs, bank **8** for consonants, or bank **3** for whole words.
The phoneme stream follows playback across all ten banks.

At startup, the RGB LED uses the Talko2 sequence. Red flashes count the major
version, one blue flash marks the separator, and green flashes count the
minor version. For 1.0, the LED flashes red once and blue once, with no
green count flashes for zero. The S.A.M. voice then says “version one point
zero” as the display shows `1`, the decimal point, and `0`.

| Control | Action |
|---|---|
| Encoder turn | Select a bank. The display shows its number with the dot off. |
| SOUND | Choose an entry within the bank. |
| SPEAK button or GATE IN | Speak the selected entry. In the middle switch position, a low gate or releasing SPEAK cuts speech and the next trigger starts a new phrase. The outer positions play once per trigger. |
| SPEED and PITCH | Turn clockwise for faster speech or higher pitch. Both work during speech. The middle positions use the selected voice's original settings. Speed spans roughly ¼× to 4× on voiced sounds, and pitch spans roughly two octaves either side of the preset. |
| Encoder press and turn | Hold to show the current voice glyph with a fast blinking dot. Turn while holding to choose a voice. Press SPEAK or trigger GATE IN while still holding to hear the highlighted voice. Release to confirm your choice and return to the bank. |
| Encoder tap | Switch between banks and FX. The steady dot marks the FX page. Turn the encoder to select an effect, then use the FX knob to adjust it. The selected effect stays active when you return to banks. |
| ERROR held during speech | Loop recent speech. Turn SPEED to shorten or lengthen the loop and PITCH to change its playback rate. Both respond to movement from their positions when ERROR was pressed. Release to resume speech from the same point. The digit flickers during the stutter. |

When triggered during speech, the latest pending entry plays after the
current phrase. A gate cutoff in the middle switch position clears the
phrase, any pending trigger, and the delay or reverb tail. In the outer
positions, echoes and reverb ring out after speech. The encoder, knobs,
ERROR, and GATE wake the display from its five-minute Snake screensaver.

| FX glyph | Effect | FX knob |
|---|---|---|
| `-` | Off | Dry speech |
| `o` | Ring Mod / Dalek | Carrier frequency from 30 to 1200 Hz, with bypass at the bottom |
| `r` | Reverb | Wet level and decay |
| `d` | Delay | Echo time from 40 to 700 ms, with bypass at the bottom |
| `E` | Error / Glitch | Off below noon. In a narrow band at noon, the effect has an even chance to stutter or play normally every 80 ms. Past noon, turn clockwise to make stutter bursts more frequent. |
| `S` | Saturation / Distortion | Drive into clipping |
| `G` | Growl | Blend the voiced buzz into noise shaped by the vowel formants |

| Bank | Contents | Entries |
|---|---|---:|
| 0 | Zero through nine | 10 |
| 1 | Alphabet A–Z | 26 |
| 2 | NATO alphabet | 26 |
| 3 | Single words | 16 |
| 4 | Commands | 12 |
| 5 | Computer status | 12 |
| 6 | Radio calls | 12 |
| 7 | Vowels and diphthongs | 17 |
| 8 | Consonants | 18 |
| 9 | Vocal percussion | 12 |

| Glyph | Preset | Speed | Pitch | Mouth | Throat |
|---|---|---:|---:|---:|---:|
| `S` | S.A.M. | 72 | 64 | 128 | 128 |
| `E` | Elf | 72 | 64 | 160 | 110 |
| `r` | Robot | 92 | 60 | 190 | 190 |
| `t` | Stuffy | 82 | 72 | 105 | 110 |
| `L` | Little Old Lady | 82 | 32 | 145 | 145 |
| `A` | Alien | 100 | 64 | 200 | 150 |

The seven-segment `S` looks like `5`. The blinking dot identifies voice selection.
Mouth and Throat are set by the voice preset. A power cycle starts at bank 0
with the original S.A.M. voice.

## S.A.M. source and license

The engine is adapted from [ESP8266SAM 1.1.0](https://github.com/earlephilhower/ESP8266SAM/tree/bb3e2559387dc89b43cc1a6c37abaa02eb8410d1)
by **Earle F. Philhower, III**, based on [Sebastian Macke's C adaptation
of S.A.M.](https://github.com/s-macke/SAM), originally published by
**Don't Ask Software**.

The Talko adaptation by **Jean-Luc Deladrière / Polaxis** adds RP2040 PWM
audio, the panel interface and banks, live pitch and speed, gate cancellation,
stutter, effects, and playback-aligned phoneme events. It also includes
parser bounds fixes. The reciter and original lookup tables are retained.
See [third-party notices](THIRD_PARTY_NOTICES.md) for the modified-file list,
the Pico WS2812 helper attribution, and framework notices.

**Licensing remains unresolved for the reverse-engineered S.A.M. engine.**
Upstream does not grant a clear redistribution license. This repository
does not assign a blanket open-source license to that engine. Its original
[upstream README and license statement](lib/ESP8266SAM/UPSTREAM.md) are
preserved unchanged; attribution alone does not resolve redistribution rights.

## Verification

Version 1.0 builds for RP2040 with the pinned PlatformIO platform and
Arduino-Pico. Host checks rendered all 161 entries with six voices at three
speed/pitch scales, with FX off and with Growl, for 5,796 renders under
AddressSanitizer. An impulse check covers dry output at the bottom of each
effect, including the corrected Delay bypass. See [release notes](CHANGELOG.md).

To repeat those host checks with Clang installed, run
`python3 tests/run_host_checks.py`.

The firmware's framed phoneme protocol matches the S.A.M.-compatible
Visual Console source. A host build does not verify the physical controls,
audio timing, USB enumeration/upload, or live lip sync on a Talko module.
Those hardware checks remain to be confirmed for this release.
