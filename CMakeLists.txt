cmake_minimum_required(VERSION 3.22)

project(Circuits VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add JUCE
add_subdirectory(JUCE)

# Plugin target
juce_add_plugin(Circuits
    COMPANY_NAME "CircuitLab"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS TRUE
    COPY_PLUGIN_AFTER_BUILD TRUE
    PLUGIN_MANUFACTURER_CODE Clab
    PLUGIN_CODE Circ
    FORMATS AU VST3 Standalone
    PRODUCT_NAME "Circuits"
)

# Generate JUCE header
juce_generate_juce_header(Circuits)

# Source files
target_sources(Circuits
PRIVATE
    # Main Plugin
    src/PluginProcessor.cpp
    src/PluginEditor.cpp
    src/CircuitSerializer.cpp
    
    # Engine
    src/Engine/CircuitEngine.cpp
    src/Engine/CircuitGraph.cpp
    src/Engine/MNASolver.cpp
    
    # Components
    src/Engine/Components/Resistor.cpp
    src/Engine/Components/Capacitor.cpp
    src/Engine/Components/Potentiometer.cpp
    src/Engine/Components/Switch.cpp
    src/Engine/Components/VacuumTube.cpp
    src/Engine/Components/AudioInput.cpp
    src/Engine/Components/AudioOutput.cpp
    
    # UI
    src/UI/CircuitDesigner.cpp
    src/UI/ComponentPalette.cpp
    src/UI/ComponentView.cpp
    src/UI/WireView.cpp
    src/UI/ControlPanel.cpp
    src/UI/OscilloscopeView.cpp
)

# Include directories
target_include_directories(Circuits
PRIVATE
    src
    src/Engine
    src/Engine/Components
    src/UI
)

# Compile definitions
target_compile_definitions(Circuits
PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0
)

# Link JUCE modules
target_link_libraries(Circuits
PRIVATE
    juce::juce_audio_utils
    juce::juce_audio_processors
    juce::juce_gui_basics
    juce::juce_gui_extra
    juce::juce_dsp
PUBLIC
    juce::juce_recommended_config_flags
    juce::juce_recommended_lto_flags
    juce::juce_recommended_warning_flags
)
