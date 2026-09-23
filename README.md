# Transient Shaper

A real-time audio transient shaper built with C++ and JUCE. It is available as a VST3 effect and as a Windows standalone application.

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

The VST3 is built from the same source. Open the project in Visual Studio/CMake and build the `MySynth_VST3` target; CMake installs it in the current user's VST3 folder.

## Controls

| Control | Range | What it does |
| --- | ---: | --- |
| Attack | -12 to +12 dB | Attenuates or emphasizes fast transients |
| Sustain | -12 to +12 dB | Attenuates or emphasizes the signal body after the onset |
| Sensitivity | 0.25x to 4x | Sets how strongly fast envelope rises are detected as transients |
| Output | -12 to +12 dB | Sets the final output level |

## Build from source

The repository includes JUCE under `modules/JUCE`.

Requirements:

- Windows 10 or later
- CMake 3.23.1 or later
- Visual Studio 2022 Build Tools with the C++ desktop workload

From the repository root, run:

```powershell
cmake -S . -B build -A x64
cmake --build build --config Release --target MySynth_Standalone MySynth_VST3
```

Build outputs are created under:

```text
build/MySynth_artefacts/Release/Standalone/Transient Shaper.exe
build/MySynth_artefacts/Release/VST3/Transient Shaper.vst3
```

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

## Submission documents

- [User Guide (PDF)](docs/submission/Transient%20Shaper%20-%20User%20Guide.pdf)
- [Technical Summary (PDF)](docs/submission/Transient%20Shaper%20-%20Technical%20Summary.pdf)
- [Testing Report (PDF)](docs/submission/Transient%20Shaper%20-%20Testing%20Report.pdf)
- [Windows VST3 bundle (ZIP)](release/Transient%20Shaper%20VST3%20Windows.zip)
