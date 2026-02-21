# Beatwerk

A drum machine audio plugin that bridges **Ableton Live Drum Racks** with hardware MIDI controllers. Load `.adg` presets, trigger samples from your pads, rearrange kits with drag & drop, and navigate presets via MIDI — all from a dark-themed UI designed for live performance.

Built with [JUCE](https://juce.com/) and C++20. Available as **VST3**, **Audio Unit**, and **Standalone** application.

## Features

### Ableton Drum Rack Integration

- Automatically detects Ableton Live installation and scans Core Library & User Library for Drum Rack presets (`.adg`)
- Parses gzipped Ableton XML, extracts sample paths, and intelligently maps samples to pads using keyword matching (kick, snare, tom, hihat, crash, ride, etc.)
- Supports relative path resolution for Core Library, User Library, and external sample references

### Pad Grid

- Configurable pad layout with multiple electronic drum kit mappings supported
- Each pad displays: name, trigger type (Head / Rim / X-Stick / Open / Closed / etc.), MIDI note, and loaded sample name
- Velocity-sensitive triggering with visual flash animation
- Click a pad to preview the sample

### Drag & Drop

- **External files** — drop `.wav`, `.aif`, `.aiff`, `.flac`, or `.mp3` onto any pad
- **Pad-to-pad swapping** — drag one pad onto another to swap their samples
- Custom mappings are saved per preset and persist across sessions

### Preset Browser

- Scrollable list with alphabet bar (A-Z, #) for quick navigation
- Active preset highlighted with accent color
- Click to load any preset instantly

### MIDI Preset Navigation

- Navigate presets with MIDI CC messages from your controller
- Configurable MIDI channel (Any, or Ch 1-16)
- Configurable CC numbers for Previous / Next preset (default: CC#1 / CC#2)

### MIDI Learn

- Click **Learn** next to Prev or Next CC in Settings
- Send any CC from your controller — it's captured and assigned automatically

### Preset Saving

- Save customized kits as JSON presets
- Stored in `~/Library/Application Support/Beatwerk/Presets/`
- Saved presets appear alongside Ableton presets in the browser

### Sample Engine

- Up to 8 polyphonic voices per pad (192 total)
- Voice stealing (oldest voice) when all voices are active
- Automatic resampling to match host sample rate
- Mono and stereo sample support

## Installation

### macOS Installer

Download `Beatwerk-1.0.0-macOS.pkg` from the [Releases](https://github.com/abnuk/beatwerk/releases) page and run it. You can choose which components to install:

| Component | Install Location |
|---|---|
| Standalone App | `/Applications/Beatwerk.app` |
| VST3 Plugin | `/Library/Audio/Plug-Ins/VST3/Beatwerk.vst3` |
| Audio Unit | `/Library/Audio/Plug-Ins/Components/Beatwerk.component` |

After installing, rescan plugins in your DAW if needed.

### Uninstalling

```bash
./installer/uninstall.sh
```

Or manually remove the files from the locations listed above.

## Building from Source

### Requirements

- macOS 11.0+ (Big Sur or later)
- CMake 3.22+
- C++20 compatible compiler (Xcode 13+)

### Build

```bash
git clone --recursive https://github.com/abnuk/beatwerk.git
cd beatwerk
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

Build artefacts will be in `build/Beatwerk_artefacts/Release/`.

### Universal Binary (Apple Silicon + Intel)

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_OSX_ARCHITECTURES="arm64;x86_64"
cmake --build build --config Release
```

### Create macOS Installer

```bash
./installer/create_installer.sh            # uses existing build
./installer/create_installer.sh --build    # compiles first, then creates installer
```

To sign the installer for distribution:

```bash
./installer/create_installer.sh --sign "Developer ID Installer: Your Name (TEAM_ID)"
```

## Project Structure

```
beatwerk/
├── CMakeLists.txt           # Build configuration
├── Source/
│   ├── PluginProcessor.*    # Audio processing & state management
│   ├── PluginEditor.*       # Main UI, settings overlay
│   ├── SampleEngine.*       # Polyphonic sample playback
│   ├── MidiMapper.*         # Pad layout, MIDI routing, MIDI Learn
│   ├── AdgParser.*          # Ableton .adg file parser
│   ├── PresetManager.*      # Preset scanning, loading, saving
│   ├── PadComponent.*       # Pad UI with drag & drop
│   ├── PadMappingManager.*  # Per-preset custom pad mappings
│   ├── PresetListComponent.*# Preset browser with alphabet nav
│   └── LookAndFeel.*        # Dark theme styling
├── installer/
│   ├── create_installer.sh  # macOS .pkg builder
│   ├── uninstall.sh         # Uninstall helper
│   ├── distribution.xml     # Installer component definitions
│   └── resources/           # Installer UI (welcome, readme)
└── JUCE/                    # JUCE framework
```

## License

All rights reserved.
