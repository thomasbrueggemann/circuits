# Circuits - Analog Circuit Simulation VST Plugin

A JUCE-based VST3/AU plugin for real-time analog circuit modeling with a visual drag-and-drop circuit designer.

![Plugin Screenshot](docs/screenshot.png)

## Features

- **Drag-and-drop circuit designer** - Place components on an infinite canvas
- **Real-time simulation** - Modified Nodal Analysis (MNA) with Newton-Raphson for nonlinear components
- **Component library**:
  - Resistors with adjustable values
  - Capacitors with adjustable values
  - Potentiometers (linear and logarithmic tapers)
  - Switches (SPST)
  - Vacuum tubes (12AX7, 12AT7, 12AU7, EL34 presets)
  - Audio input/output nodes
- **Auto-generated controls** - Potentiometers become knobs, switches become toggles
- **Oscilloscope** - Click any wire to probe voltage in time domain
- **Circuit saving** - Presets saved with DAW project

## Building

### Prerequisites

- CMake 3.22+
- C++17 compiler
- JUCE framework

### Setup JUCE

Clone JUCE into the project directory:

```bash
cd circuits
git clone https://github.com/juce-framework/JUCE.git
```

Or add as submodule:

```bash
git submodule add https://github.com/juce-framework/JUCE.git
```

### Build

```bash
mkdir build && cd build
cmake ..
cmake --build . --config Release
```

The plugin will be built and copied to your system's plugin folder.

### Build Targets

- **VST3**: `circuits_VST3` - Industry-standard plugin format
- **AU**: `circuits_AU` - macOS Audio Unit
- **Standalone**: `circuits_Standalone` - Standalone application for testing

## Usage

### Basic Workflow

1. **Add components** - Drag from the left palette onto the canvas
2. **Connect nodes** - Click a terminal and drag to another terminal
3. **Adjust values** - Double-click a component to edit values
4. **Add controls** - Potentiometers and switches auto-appear in the right panel
5. **Probe signals** - Click any wire to see its voltage on the oscilloscope

### Keyboard Shortcuts

| Key | Action |
|-----|--------|
| Delete/Backspace | Remove selected component |
| Escape | Cancel wire drawing |
| Alt+Drag | Pan canvas |
| Mouse wheel | Zoom |

### Value Format

When entering component values, use standard prefixes:

| Suffix | Multiplier | Example |
|--------|------------|---------|
| M | 10^6 | 1M = 1 MΩ |
| k | 10^3 | 10k = 10 kΩ |
| u/µ | 10^-6 | 100u = 100 µF |
| n | 10^-9 | 47n = 47 nF |
| p | 10^-12 | 22p = 22 pF |

## Example Circuits

Example circuits are provided in the `examples/` folder:

- **fuzz_pedal.json** - Classic fuzz effect based on Fuzz Face topology
- **tube_preamp.json** - 12AX7 single-stage preamp with volume control
- **distortion.json** - Distortion pedal with gain, tone, and volume

## Technical Details

### Circuit Simulation

The plugin uses **Modified Nodal Analysis (MNA)** to solve circuit equations:

```
Gx = z

Where:
G = conductance matrix (stamped by each component)
x = solution vector (node voltages + branch currents)
z = right-hand side vector (sources)
```

For nonlinear components (vacuum tubes), we use **Newton-Raphson iteration** to find the operating point.

### Vacuum Tube Model

Triodes are modeled using Koren equations:

```cpp
E1 = (vp / kp) * log(1 + exp(kp * (1/mu + vg / sqrt(kvb + vp*vp))))
Ip = (E1 > 0) ? pow(E1, ex) / kg1 : 0
```

### Performance

- 2x oversampling for numerical stability
- DC blocking filter on output
- Soft clipping for safe output levels

## Project Structure

```
circuits/
├── CMakeLists.txt           # Build configuration
├── JUCE/                    # JUCE framework (clone separately)
├── .github/workflows/       # GitHub Actions CI/CD
├── src/
│   ├── PluginProcessor.*    # Audio processing
│   ├── PluginEditor.*       # Main UI
│   ├── CircuitSerializer.*  # JSON save/load
│   ├── Engine/
│   │   ├── CircuitEngine.*  # High-level simulation
│   │   ├── CircuitGraph.*   # Circuit topology
│   │   ├── MNASolver.*      # MNA implementation
│   │   └── Components/      # Component models
│   └── UI/
│       ├── CircuitDesigner.*    # Canvas
│       ├── ComponentPalette.*   # Palette sidebar
│       ├── ComponentView.*      # Component rendering
│       ├── WireView.*           # Wire rendering
│       ├── ControlPanel.*       # Knobs/switches
│       └── OscilloscopeView.*   # Scope display
└── examples/                # Example circuits
```

## License

MIT License - See LICENSE file for details.

## Contributing

Contributions welcome! Please open an issue or pull request.

### Ideas for Future Development

- [ ] Additional tube types (pentodes, power tubes)
- [ ] Transformers
- [ ] Inductors
- [ ] Diode clipping stages
- [ ] SPICE netlist import
- [ ] Circuit library/presets browser
- [ ] Frequency response analyzer
