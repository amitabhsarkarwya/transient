# Transient Shaper

A real-time audio transient shaper built with C++ and JUCE. It is available as a VST3 effect for Windows and macOS, and as a Windows standalone application.

## Features

- Independent Attack and Sustain shaping, from -12 dB to +12 dB
- Sensitivity control for the fast-versus-slow envelope detector
- Output trim from -12 dB to +12 dB
- Mono and stereo support
- Live envelope display and input/output peak readouts
- No dynamic memory allocation in the audio processing callback
- Windows XP inspired chrome with cybersigil accents

## Download for Windows

The ready-to-run standalone application is included at [`release/Transient Shaper.exe`](release/Transient%20Shaper.exe). It does not need a DAW. Run it, select an audio input and output device, and adjust the controls while audio is playing.

The VST3 is built from the same source. Open the project in Visual Studio/CMake and build the `TransientShaper_VST3` target; CMake installs it in the current user's VST3 folder.

## Download for macOS

Download the [macOS VST3 Installer](release/Transient-Shaper-VST3-macOS.pkg). It installs a universal VST3 build for Apple silicon and Intel Macs. The macOS build and pluginval validation run in [GitHub Actions](https://github.com/amitabhsarkarwya/transient/actions/workflows/macos-vst3.yml); each successful run also provides a downloadable installer artifact.

The package is currently unsigned and not notarized. Gatekeeper may require Control-clicking the downloaded package and choosing **Open** the first time.

## Controls

| Control | Range | What it does |
| --- | ---: | --- |
| Attack | -12 to +12 dB | Attenuates or emphasizes fast transients |
| Sustain | -12 to +12 dB | Attenuates or emphasizes the signal body after the onset |
| Sensitivity | 0.25x to 4x | Sets how strongly fast envelope rises are detected as transients |
| Output | -12 to +12 dB | Sets the final output level |

## Build from source

The repository includes JUCE under `modules/JUCE`.

### Windows

Requirements:

- Windows 10 or later
- CMake 3.23.1 or later
- Visual Studio 2022 Build Tools with the C++ desktop workload

From the repository root, run:

```powershell
cmake -S . -B build -A x64
cmake --build build --config Release --target TransientShaper_Standalone TransientShaper_VST3
```

Build outputs are created under:

```text
build/TransientShaper_artefacts/Release/Standalone/Transient Shaper.exe
build/TransientShaper_artefacts/Release/VST3/Transient Shaper.vst3
```

### macOS universal VST3

Requirements: macOS 11 or later, Xcode command line tools, CMake 3.23.1 or later, and Ninja.

```sh
cmake -S . -B build-macos -G Ninja -DCMAKE_BUILD_TYPE=Release \
  "-DCMAKE_OSX_ARCHITECTURES=arm64;x86_64" \
  -DCMAKE_OSX_DEPLOYMENT_TARGET=11.0
cmake --build build-macos --target TransientShaper_VST3 --parallel 3
bash scripts/package_macos_vst3.sh build-macos dist 0.0.1
```

The plugin bundle is created at `build-macos/TransientShaper_artefacts/Release/VST3/Transient Shaper.vst3`; the `.pkg` installer is written to `dist/`.

## Source layout

- `Source/PluginProcessor.*` — parameters, state handling, detector and audio processing
- `Source/PluginEditor.*` — plugin controls, envelope display and meters
- `Source/CustomLookAndFeel.h` — custom JUCE controls
- `modules/JUCE` — JUCE framework source
- `release` — ready-to-run Windows standalone build

## Licensing

JUCE is distributed under its own licensing terms; see `modules/JUCE/LICENSE.md`. No separate license has been selected for the Transient Shaper source yet.

## Install the VST3 plug-in

Download and run [`release/Transient Shaper VST3 Setup.exe`](release/Transient%20Shaper%20VST3%20Setup.exe). The setup wizard installs the VST3 plug-in only (not the standalone app) to the standard Windows VST3 folder, requests administrator permission, and adds an uninstall entry in Windows Apps. Restart your DAW or rescan plug-ins after installation.

The standalone Windows app is available separately as [`release/Transient Shaper.exe`](release/Transient%20Shaper.exe).

## Install the VST3 plug-in on macOS

Download and open [`release/Transient-Shaper-VST3-macOS.pkg`](release/Transient-Shaper-VST3-macOS.pkg). Follow the Installer prompts; it places `Transient Shaper.vst3` in `/Library/Audio/Plug-Ins/VST3`. Restart your DAW or rescan plug-ins after installation.

## Submission documents

- [User Guide (PDF)](docs/submission/Transient%20Shaper%20-%20User%20Guide.pdf)
- [Technical Summary (PDF)](docs/submission/Transient%20Shaper%20-%20Technical%20Summary.pdf)
- [Testing Report (PDF)](docs/submission/Transient%20Shaper%20-%20Testing%20Report.pdf)
- [Windows VST3 bundle (ZIP)](release/Transient%20Shaper%20VST3%20Windows.zip)
- [macOS VST3 Installer](release/Transient-Shaper-VST3-macOS.pkg)
