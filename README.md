# Parity

A reference-track comparison plugin for your DAW's master channel, built with C++ and [JUCE](https://juce.com).

Load a professionally mixed/mastered reference track and compare it against your own mix using objective audio measurements and fast A/B playback.

Parity helps answer questions like:

- Is my low end too loud?
- Is my mix brighter or darker than the reference?
- Is my mix more or less dynamic?
- How does my loudness compare?
- Where are the largest spectral differences?

## Features

- **Reference playback synced to the DAW transport** — the reference follows your project's playhead, so A/B comparisons line up in time
- **One-click A/B switching** — a large MIX | REF switch swaps between your mix and the reference with a click-free crossfade
- **EBU R128 / ITU-R BS.1770 loudness metering** — momentary, short-term, and gated integrated LUFS plus peak, for both mix and reference, with a color-coded delta column
- **Offline reference analysis** — full-file integrated loudness is computed the moment a reference loads
- **FFT spectrum comparison** — overlay the mix and reference spectra, or view the per-frequency difference curve; realtime and long-term-average smoothing modes
- **Clean retro-instrument UI** — light, minimal, resizable

## Formats & Platforms

- VST3 and Audio Unit (AU)
- macOS (Apple Silicon), tested primarily in Ableton Live
- Windows support may come later

## Building

Requirements: CMake 3.22+, Xcode command line tools.

```bash
git clone https://github.com/shoumik77/Parity.git
cd Parity
git submodule update --init --recursive

cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

The build produces `Parity.vst3` and `Parity.component` and copies them to the standard macOS plugin directories (`~/Library/Audio/Plug-Ins/VST3` and `~/Library/Audio/Plug-Ins/Components`).

### Tests

A loudness spec-compliance check (EBU Tech 3341 sine test case) builds as a console app:

```bash
cmake --build build --target LoudnessSanityTest
./build/LoudnessSanityTest_artefacts/Release/LoudnessSanityTest
```

## Usage

1. Place Parity on your project's **master channel**
2. Click **LOAD REFERENCE** and pick a reference track (wav/aiff/flac/mp3)
3. Press play — the reference follows the DAW playhead
4. Flip the **MIX | REF** switch to A/B against the reference
5. Read the loudness table and spectrum view to see where your mix differs

## Project Structure

```text
source/
├── PluginProcessor.*      # audio processing, playhead sync, state
├── PluginEditor.*         # editor layout
├── Audio/
│   ├── ReferencePlayer.*  # reference file loading + DAW-synced playback
│   ├── LoudnessAnalyzer.* # EBU R128 / BS.1770 loudness measurement
│   └── SpectrumAnalyzer.* # FFT spectrum tap
└── UI/
    ├── ParityLookAndFeel.* # retro instrument theme
    ├── ABSwitch.*          # MIX | REF segmented switch
    └── SpectrumView.*      # spectrum overlay / difference plot
```

## License

See [LICENSE](LICENSE).
