# Interaction validation — 12 November 2026

The interaction/accessibility update was checked on an Intel MacBook running
Omarchy/Hyprland on Wayland, with the built-in 2560 × 1600 display at 2× scale.

## Automated

`bin/build`, `bin/test`, and `bin/check-runtime` pass. The 34 Qt Test cases/data
rows include real mouse/key/wheel events against the production QML:

- Six-pixel click jitter, deliberate drags, pointer-grab cancellation, held
  presses, and rail/badge hover transitions.
- Shared 2, 6, and 12 pixel widths on both edges, marker clearance, small-display
  clamping, stable warning/recovery bounds, and the union of badge/corridor masks.
- Frozen sensor/monitor choices while open, committing the highlighted stable
  ID before refreshing, wheel scrolling over closed selectors, popup-first
  Escape, forward/backward focus visibility, and Settings activation requests.
- Fahrenheit scale labels, inspection toggle/10-second timeout, hover surviving
  timeout, placement reset, pulse disabling, and reduced-motion persistence.

Settings renders were inspected at narrow/wide widths, larger fonts, and with a
light palette. The Inspect/Close/Quit actions remain visible in the footer.
The isolated runtime check also exercises repeated Settings and inspection IPC.

## Wayland

A separate instance used temporary preferences and an isolated IPC directory.
A disposable Qt test window was explicitly tiled, floated, and made fullscreen;
compositor state confirmed each mode. In all three, the rails remained on the
bottom layer (48 logical pixels) and the badge appeared on the overlay layer
(320 logical pixels), with the test application retaining keyboard focus.

The fullscreen test window received a real compositor click in transparent
space beside the badge and outside its narrow connecting corridor. The click
did not open Settings. Actual pointer transitions across rail, corridor, and
badge kept the readout open. A vertical badge drag saved the expected position;
a subsequent six-pixel jitter click opened Settings. Ctrl+Q then quit the
isolated instance. The badge was visually checked above the test window.

Physical multi-monitor unplug/reconnect, other display scales, and an assistive
technology audit remain outstanding. Headless CI cannot replace these checks.
