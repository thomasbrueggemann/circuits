# Circuits - Analog Circuit Simulation VST Plugin

A JUCE-based VST3/AU plugin for real-time analog circuit modeling with a visual drag-and-drop circuit designer.

![Plugin Screenshot](docs/screenshot.png)

## Features

- **Drag-and-drop circuit designer** - Place components on an infinite canvas
- **Real-time simulation** - Wave Digital Filters (WDF) for guaranteed stability and efficiency
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

The plugin uses **Wave Digital Filters (WDF)** for circuit simulation. WDF is a physically-motivated approach that models circuits using wave variables instead of voltages and currents.

Key advantages of WDF:
- **Guaranteed stability** for linear circuits (passive components)
- **No matrix inversion** required - efficient real-time processing
- **Natural handling** of reactive elements (capacitors, inductors)
- **Bilinear transform** discretization matches analog frequency response
- **Local nonlinear solving** at root elements

### WDF Theory

Each circuit element is represented as a port with:
- Incident wave `a` (coming into the element)
- Reflected wave `b` (going out of the element)
- Port resistance `R` (characteristic impedance)

Wave relationships:
```
v = a + b           (voltage at port)
i = (a - b) / R     (current through port)
```

Circuit topology is built using adaptors:
- **Series adaptor**: Connects elements in series (same current)
- **Parallel adaptor**: Connects elements in parallel (same voltage)

### Nonlinear Elements

Nonlinear components (vacuum tubes, diodes) are handled at the WDF tree root using **Newton-Raphson iteration**. The triode model uses Koren equations:

```cpp
E1 = (vp / kp) * log(1 + exp(kp * (1/mu + vg / sqrt(kvb + vp*vp))))
Ip = (E1 > 0) ? pow(E1, ex) / kg1 : 0
```

### Performance

- 2x oversampling for improved frequency response
- DC blocking filter on output
- Soft clipping for safe output levels
- Sample-rate adaptive component values

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
│   │   ├── WDF/             # Wave Digital Filter engine
│   │   │   ├── WDFCore.h        # Base classes and basic elements
│   │   │   ├── WDFNonlinear.h   # Nonlinear elements (diodes, tubes)
│   │   │   ├── WDFRTypeAdaptor.h # Multi-port adaptors
│   │   │   └── WDFEngine.*      # WDF tree builder and processor
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
- [x] Inductors (WDF model implemented)
- [x] Diode clipping stages (WDF nonlinear model)
- [ ] SPICE netlist import
- [ ] Circuit library/presets browser
- [ ] Frequency response analyzer
- [ ] R-type adaptors for complex topologies

## References

- Fettweis, A. (1986). "Wave digital filters: Theory and practice"
- Werner, K. J. (2016). "Virtual Analog Modeling of Audio Circuitry Using Wave Digital Filters"
- Koren, N. (1996). "Improved vacuum-tube models for SPICE simulations"
