# Release notes

## 1.0.1 - 2026-11-07

- In the middle switch position, gate-off stops the voice and clears pending
  speech while delay and reverb tails continue to ring out.
- Turning FX fully counterclockwise or changing effects still clears the tail.
- Document the USB serial label output and the 115200-baud monitor setting.
- Add playback simulations for gate cutoff, tails, pending-trigger
  cancellation, FX bypass, and serial label output with host backpressure.
- Announce and display the patch version at startup, including its RGB LED code.

## 1.0 - 2026-11-07

- Set the startup display and spoken version to 1.0. The rear LED flashes
  red once, then blue once, with no minor-version count flashes for zero.
- Keep the ten banks and all 161 entries from prototype 0.7.
- Correct Delay so the fully counterclockwise FX position is dry, matching
  the other effects. Above its bottom dead zone, the knob sets 40–700 ms.
- Pin the PlatformIO platform used for the release build.
- Align the README with the panel's SPEAK label and the complete vocabulary.
- Identify the upstream engine, its authors, and the Talko modifications;
  preserve the original upstream README and include third-party notices.

Build and host checks are described in the README. Physical panel behavior,
real-time audio timing, USB uploads, and live Visual Console operation still
need validation on a Talko module. The upstream S.A.M. redistribution
license remains unresolved.
