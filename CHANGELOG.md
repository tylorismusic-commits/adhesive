# Changelog

## 0.2.0

- Added the suite-wide skin system. The free Classic theme remains the default,
  and the header swatch cycles through any installed skins.
- Skin selection is user-level, shared across the suite, and persists in
  `~/.config/hearth/skins.conf`. It never changes audio, presets, or project
  state.
- Added the loader for Catppuccin Mocha, Studio 4000, Tokyo Night, and
  user-created skins.

## 0.1.3

- The IN/BYPASS control is now a normal plugin parameter, not the CLAP host
  bypass. Toggling it no longer changes REAPER's own FX bypass state.

## 0.1.2

- CLAP builds now forward the meter ports to the native UI, so the VU and
  gain-reduction meters update in CLAP exactly as they do in LV2.

## 0.1.1

- Added the Harrison/Ardour inline-display extension with a live
  gain-reduction bar in the mixer.
- Added a CLAP build that uses the same DSP and the same X11 UI, and reports
  gain reduction through `clap.gain-adjustment-metering` so REAPER can display
  it on the track meter.
- Added a CLAP host test covering gain-reduction reporting and UI embedding.

## 0.1.0

- Initial release.
- SSL 4000G-inspired VCA Glue compressor with stepped ratio, attack, release,
  100 Hz side-chain high-pass, soft knee, parallel Mix, and Auto release.
- Reel-to-reel and cassette tape stages with record pre-emphasis, head bump,
  ADAA magnetic saturation, playback bandwidth limiting, wow and flutter, and
  parallel Tape Mix.
- Tape can run before or after Glue.
- Animated reel-to-reel and cassette transport graphics.
- Always-on subtle output transformer, active even when Glue and Tape are
  bypassed.
- Constant 4 ms reported latency with an aligned dry path.
- Hidden +0.2 dBFS soft clipper.
- Working in-UI preset bar with five musical presets and Broken Machine.
- Resizable, HiDPI-aware X11 UI with Jost ExtraBold wordmark, original knob
  artwork, brushed-metal chassis, glass VU, and fader.
