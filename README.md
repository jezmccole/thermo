# Thermo

A modern, ambient, always-on desktop edge thermometer for MacBook running Omarchy (Linux / Hyprland).

Sits quietly on the desktop behind all windows along the right edge of your screen, displaying a minimal graphical representation of your MacBook hardware temperatures.

## Features

- **Ambient Desktop Placement**: Runs on Wayland `LayerBottom`, living behind all active tiled and floating application windows on your desktop.
- **Ultra-Minimal Footprint**: Only a few pixels wide when resting, completely unobtrusive and non-distracting.
- **Organic Heat Gradient**: Cold blue (`#38BDF8`) through a muted neutral midpoint and orange to hot crimson (`#DC2626`), with matching blue outlines.
- **Interactive Holographic Readout**: Hovering reveals a stationary frosted glass readout badge beside the rail. The rail never moves or expands on hover; a continuous hover area and brief exit delay keep the badge readable. Transparent areas allow desktop clicks through.
- **Live Fan Speeds**: The smoked glass badge reports each available hardware fan in RPM, refreshed every 1.5 seconds. Missing readings are shown as unavailable.
- **Draggable Position**: Click and drag the thermometer strip vertically along the edge of your screen to set your preferred resting position.
- **MacBook Sensor Detection**: Autodetects all Apple SMC hardware sensors (`applesmc`) and CPU package sensors (`coretemp`). Defaults to **Central Processor Proximity (`TC0P`)**, with options for Average CPU temperature, System average, or individual sensors.
- **Live Omarchy Theming**: Watches desktop accent and Hyprland window rounding live.
- **Configurable Autostart**: Pre-enabled to launch on login, toggleable inside the settings window.

The default rail height is 127 pixels, adjustable in Settings.

## Usage

```bash
# Launch Thermo (starts background strip and opens settings if run interactively)
thermo

# Open Settings window directly
thermo settings

# Run in background without opening settings (used for login autostart)
thermo --background

# Output current sensor readings and config in JSON
thermo --json

# Terminate running instance
thermo quit
```

## Shortcuts (Settings Window)

| Key | Action |
| :--- | :--- |
| `Esc` / `Q` | Close Settings (keeps desktop thermometer active) |
| `Space` | Refresh sensors |

## License

MIT © 2026 jezmccole
