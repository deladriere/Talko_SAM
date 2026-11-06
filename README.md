# Talko S.A.M. 0.7 prototype

Firmware for the Talko RP2040 hardware. To update a module running 0.1–0.3,
put it into BOOTSEL mode and copy `Talko_SAM-0.7.uf2` onto the `RPI-RP2`
drive. Once 0.7 boots, it exposes a USB serial port for normal PlatformIO
uploads. On macOS, look for the module's `/dev/cu.usbmodem…` device.

Open this folder in PlatformIO and run `pio run` to rebuild the UF2. The output
is `.pio/build/pico/firmware.uf2`. Use the Upload task or `pio run -t upload`
with the Talko serial port. Open PlatformIO Monitor at 115200 to see one
label per triggered phrase as it starts. Alphabet entries show their
letters, and the vowel and consonant banks show phoneme labels.

For lip sync in Kraftor Visual Console, select **S.A.M.** under **Phoneme
model** in the 2D Mouth or 3D Head menu, then connect to Talko over Serial.
Console requests phoneme events on the same port. Try bank **7** for vowels
and diphthongs, bank **8** for consonants, or bank **3** for whole words.
The phoneme stream follows playback across all ten banks.

At startup, the RGB LED uses the Talko2 sequence. Red flashes count the major
version, one blue flash marks the separator, and green flashes count the
minor version. For 0.7, the LED flashes blue once and green seven times.
The S.A.M. voice then says “version zero point seven” as the display shows
`0`, the decimal point, and `7`.

| Control | Action |
|---|---|
| Encoder turn | Select a bank. The display shows its number with the dot off. |
| SOUND | Choose an entry within the bank. |
| GATE button or input | Speak the selected entry. In the middle switch position, a low gate cuts speech and the next trigger starts a new phrase. The outer positions play once per trigger. |
| SPEED and PITCH | Turn clockwise for faster speech or higher pitch. Both work during speech. The middle positions use the selected voice's original settings. Speed spans roughly ¼× to 4× on voiced sounds, and pitch spans roughly two octaves either side of the preset. |
| Encoder press and turn | Hold to show the current voice glyph with a fast blinking dot. Turn while holding to choose a voice. Trigger GATE while still holding to hear the highlighted voice. Release to confirm your choice and return to the bank. |
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
| `d` | Delay | Echo time from 40 to 700 ms |
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

The engine is adapted from [ESP8266SAM 1.1.0](https://github.com/earlephilhower/ESP8266SAM/tree/bb3e2559387dc89b43cc1a6c37abaa02eb8410d1).
The PlatformIO registry lists its license as **Unknown**. Redistribution
rights for the underlying reverse-engineered S.A.M. code are unknown. See
the [upstream notes](lib/ESP8266SAM/UPSTREAM.md).

## Verification

Built for RP2040 with PlatformIO and Arduino-Pico. Host checks rendered all
161 entries using all six voices at three knob settings and checked mid-phrase
modulation, the first encoder detent, the three encoder display states, gate cutoff,
and stutter resume. Host checks also covered FX output, gate cutoff of tails,
Growl at all voice presets, plain console labels, playback-aligned
phoneme events, and the Error FX threshold around noon. The 0.7 build includes
Arduino-Pico USB serial and its BOOTSEL reset handler.
USB enumeration and upload await a test on a Talko module.
