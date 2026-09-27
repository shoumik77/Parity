# Parity

**A reference-track comparison plugin for producers and mixing engineers.**

Parity is a native audio plugin built with **C++ and JUCE** that lets you compare your mix against a professionally mixed and mastered reference track directly inside your DAW.

Instead of constantly switching between your DAW and an external player, Parity keeps the reference synced with your project and gives you objective measurements for **loudness, dynamics, and frequency balance** alongside instant A/B playback.

> Built to answer a simple question: **How does my mix actually compare to the reference?**

---

## Demo


https://github.com/user-attachments/assets/63f36222-b18c-4c48-803f-9ecd9f808db0

**Demo:** Load a reference track, play it in sync with the DAW, switch instantly between MIX and REF, and compare loudness and frequency balance in real time.

---

## What Parity Does

When mixing against a reference track, small differences can be difficult to judge by ear alone. Playback level, frequency balance, and dynamics can all make two tracks feel different.

Parity puts those comparisons directly on the DAW's master channel.

Use it to answer questions like:

- Is my low end too loud compared to the reference?
- Is my mix brighter or darker?
- Am I compressing the track too heavily?
- How far apart are the tracks in LUFS?
- Which frequency ranges differ the most?
- Does my mix still hold up when I A/B at the same point in the song?

---

## Features

### DAW-Synced Reference Playback

Parity reads the host transport position and keeps the reference track aligned with the DAW playhead.

Press play, pause, or seek in your project and the reference follows automatically.

This makes it possible to compare corresponding sections of two tracks without manually repositioning an external audio player.

### Instant A/B Comparison

A large **MIX | REF** switch lets you move between your mix and reference track without leaving the plugin.

A short crossfade is applied during transitions to prevent clicks and discontinuities.

### Loudness Analysis

Parity implements loudness measurement based on **EBU R128 / ITU-R BS.1770**.

Both the mix and reference expose:

- Momentary LUFS
- Short-term LUFS
- Integrated LUFS
- Peak level
- Mix/reference delta

Integrated reference loudness is also calculated offline as soon as a track is loaded, so the full-track measurement is immediately available.

### Spectrum Comparison

Real-time FFT analysis visualizes the frequency balance of both tracks.

Two viewing modes are available:

**Overlay**

Displays the mix and reference spectra together.

**Difference**

Displays the per-frequency difference between the two signals, making it easier to identify areas where the mix contains more or less energy than the reference.

Both real-time and long-term-average smoothing modes are supported.

### Plugin UI

Parity uses a custom JUCE interface inspired by studio hardware and measurement equipment.

The interface is:

- Resizable
- Minimal
- Focused on quick A/B decisions
- Designed to keep the most important measurements visible at a glance

---

## How It Works

```text
                    ┌─────────────────────┐
DAW Master ────────►│                     │
                    │      PARITY         │
Reference File ────►│                     │
                    └──────────┬──────────┘
                               │
              ┌────────────────┼────────────────┐
              ▼                ▼                ▼
       Loudness Analysis   FFT Analysis    A/B Routing
              │                │                │
              ▼                ▼                ▼
        LUFS / Peak       Spectrum View    MIX / REF
```

Parity processes two audio sources:

1. **Mix input** — audio arriving from the DAW's master channel
2. **Reference input** — audio decoded from the loaded reference track

The analysis pipeline independently measures both signals before sending the selected source through the output path.

The reference player uses the DAW's current transport position to determine where the reference should be playing, keeping playback synchronized when the user plays, pauses, or seeks.

---

## Engineering Highlights

Parity is also an exploration of real-time audio software and plugin development in modern C++.

Some of the main engineering challenges include:

- Building a real-time-safe audio processing path
- Synchronizing external audio with a DAW's host transport
- Implementing click-free signal switching
- Performing FFT analysis without blocking the audio thread
- Implementing standards-based loudness measurement
- Separating real-time processing from UI visualization
- Managing plugin state and reference-track state
- Building a custom, resizable JUCE interface
- Supporting both VST3 and Audio Unit plugin formats from one codebase

The project is structured so audio analysis, reference playback, plugin processing, and visualization remain separate components rather than being tightly coupled inside the plugin processor.

---

## Architecture

```text
DAW / Host
    │
    ▼
PluginProcessor
    │
    ├──► ReferencePlayer
    │       └── File decoding + transport synchronization
    │
    ├──► LoudnessAnalyzer
    │       └── EBU R128 / BS.1770 measurements
    │
    ├──► SpectrumAnalyzer
    │       └── FFT + spectral data
    │
    └──► Output Routing
            └── Click-free MIX / REF crossfade

PluginEditor
    │
    ├──► ABSwitch
    ├──► SpectrumView
    └──► ParityLookAndFeel
```

---

## Project Structure

```text
source/
├── PluginProcessor.*        # Audio processing, transport sync, state
├── PluginEditor.*           # Main plugin UI and layout
│
├── Audio/
│   ├── ReferencePlayer.*    # Reference loading + DAW-synced playback
│   ├── LoudnessAnalyzer.*   # EBU R128 / BS.1770 analysis
│   └── SpectrumAnalyzer.*   # FFT spectrum analysis
│
└── UI/
    ├── ParityLookAndFeel.*  # Custom visual theme
    ├── ABSwitch.*           # MIX | REF control
    └── SpectrumView.*       # Spectrum overlay / difference visualization
```

---

## Usage

1. Add **Parity** to your project's master channel.
2. Click **LOAD REFERENCE**.
3. Select a `.wav`, `.aiff`, `.flac`, or `.mp3` reference track.
4. Press play in your DAW.
5. Parity keeps the reference aligned with the host playhead.
6. Use the **MIX | REF** switch to A/B the signals.
7. Compare loudness measurements and the spectrum view to identify differences between the tracks.

---

## Formats & Platforms

Currently supported:

- **VST3**
- **Audio Unit (AU)**
- **macOS / Apple Silicon**

Development and testing are primarily done in **Ableton Live**.

Windows support is planned for a future release.

---

## Building From Source

### Requirements

- CMake 3.22+
- Xcode Command Line Tools
- macOS
- Git

Clone the repository:

```bash
git clone https://github.com/shoumik77/Parity.git
cd Parity

git submodule update --init --recursive
```

Configure and build:

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

The build produces:

```text
Parity.vst3
Parity.component
```

The plugins are copied to the standard macOS plugin directories:

```text
~/Library/Audio/Plug-Ins/VST3
~/Library/Audio/Plug-Ins/Components
```

Restart or rescan plugins in your DAW after building.

---

## Testing

Parity includes a loudness compliance test based on an **EBU Tech 3341 sine test case**.

Build the test target:

```bash
cmake --build build --target LoudnessSanityTest
```

Run it:

```bash
./build/LoudnessSanityTest_artefacts/Release/LoudnessSanityTest
```

The test is used to validate the loudness analysis implementation against known reference values.

---

## Tech Stack

**Core**

- C++17
- JUCE
- CMake

**Audio / DSP**

- FFT-based spectral analysis
- EBU R128 loudness measurement
- ITU-R BS.1770 loudness algorithms
- Real-time audio processing
- Host transport synchronization

**Plugin Formats**

- VST3
- Audio Unit

---

## Why I Built Parity

Reference tracks are one of the most useful tools in mixing, but the workflow is often awkward: import a track, manually align it, manage routing, compensate for playback differences, and repeatedly switch between signals.

I wanted the comparison itself to feel like a native part of the mixing workflow.

Parity started as a way to make A/B referencing faster, then grew into a deeper C++ audio project involving real-time DSP, host/plugin communication, loudness standards, FFT analysis, audio file playback, and custom JUCE UI development.

---

## Roadmap

Parity is actively being developed. Potential future work includes:

- Loudness-matched A/B playback
- Multiple reference tracks
- Automatic reference alignment
- Improved long-term spectral statistics
- Additional dynamics measurements
- Windows support
- Additional DAW compatibility testing

---

## License

See [LICENSE](https://github.com/shoumik77/Parity/blob/main/LICENSE).
