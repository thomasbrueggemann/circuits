#include "PluginEditor.h"
#include "Engine/Components/Component.h"
#include "PluginProcessor.h"

CircuitsAudioProcessorEditor::CircuitsAudioProcessorEditor(
    CircuitsAudioProcessor &p)
    : AudioProcessorEditor(&p), audioProcessor(p) {
  // Create UI components
  componentPalette = std::make_unique<ComponentPalette>();
  circuitDesigner = std::make_unique<CircuitDesigner>(
      audioProcessor.getCircuitGraph(), *componentPalette);
  controlPanel = std::make_unique<ControlPanel>(
      audioProcessor.getCircuitGraph(), audioProcessor.getCircuitEngine());
  oscilloscopeView = std::make_unique<OscilloscopeView>();
  topBar = std::make_unique<TopBar>(audioProcessor.getCircuitGraph(),
                                    audioProcessor.getCircuitEngine());

  // Apply LookAndFeel
  setLookAndFeel(&analogLookAndFeel);

  // Add components
  addAndMakeVisible(*componentPalette);
  addAndMakeVisible(*circuitDesigner);
  addAndMakeVisible(*controlPanel);
  addAndMakeVisible(*oscilloscopeView);
  addAndMakeVisible(*topBar);

  // Wire selection callback for oscilloscope
  circuitDesigner->onWireSelected = [this](int nodeId) {
    audioProcessor.setProbeNode(nodeId);
    oscilloscopeView->setProbeActive(nodeId >= 0);
  };

  // Circuit change callback to update control panel
  circuitDesigner->onCircuitChanged = [this]() {
    updateControlPanel();

    auto &engine = audioProcessor.getCircuitEngine();
    auto &graph = audioProcessor.getCircuitGraph();
    engine.setCircuit(graph);

    // Auto-start simulation when circuit changes
    engine.setSimulationActive(true);
    
    autoProbe();
  };

  // Set editor size
  setSize(1200, 800);
  setResizable(true, true);
  setResizeLimits(800, 600, 2400, 1600);

  // Start timer for UI updates
  startTimerHz(30);

  // Initial auto-probe
  autoProbe();
}

CircuitsAudioProcessorEditor::~CircuitsAudioProcessorEditor() {
  setLookAndFeel(nullptr);
  stopTimer();
}

void CircuitsAudioProcessorEditor::paint(juce::Graphics &g) {
  // Warm, subtle vintage texture background
  juce::ColourGradient gradient(
      juce::Colour(0xFF2d2d2d), 0.0f, 0.0f, juce::Colour(0xFF232323),
      static_cast<float>(getWidth()), static_cast<float>(getHeight()), false);
  g.setGradientFill(gradient);
  g.fillAll();

  // Subtle noise/grain effect (simulated with a pattern if possible, but let's
  // keep it simple for now)
  g.setColour(juce::Colours::black.withAlpha(0.1f));
  for (int i = 0; i < getWidth(); i += 2) {
    g.drawVerticalLine(i, 0.0f, (float)getHeight());
  }

  // Subtle border lines
  g.setColour(juce::Colour(0xFF3d3d3d));
  g.drawVerticalLine(PALETTE_WIDTH, 0.0f, static_cast<float>(getHeight()));
  g.drawVerticalLine(getWidth() - CONTROL_PANEL_WIDTH, 0.0f,
                     static_cast<float>(getHeight() - OSCILLOSCOPE_HEIGHT));
  g.drawHorizontalLine(getHeight() - OSCILLOSCOPE_HEIGHT,
                       static_cast<float>(PALETTE_WIDTH),
                       static_cast<float>(getWidth() - CONTROL_PANEL_WIDTH));
}

void CircuitsAudioProcessorEditor::resized() {
  auto bounds = getLocalBounds();

  // Top Bar
  topBar->setBounds(bounds.removeFromTop(TOP_BAR_HEIGHT));

  // Left palette
  componentPalette->setBounds(bounds.removeFromLeft(PALETTE_WIDTH));

  // Right control panel
  controlPanel->setBounds(bounds.removeFromRight(CONTROL_PANEL_WIDTH));

  // Bottom oscilloscope (in the middle section)
  oscilloscopeView->setBounds(bounds.removeFromBottom(OSCILLOSCOPE_HEIGHT));

  // Remaining space for circuit designer
  circuitDesigner->setBounds(bounds);
}

void CircuitsAudioProcessorEditor::timerCallback() {
  // Update sample rate for accurate FFT frequency display
  oscilloscopeView->setSampleRate(audioProcessor.getCurrentSampleRate());
  
  // Update simulation state display
  oscilloscopeView->setSimulationRunning(
      audioProcessor.getCircuitEngine().isSimulationActive());

  // Update oscilloscope with latest voltage history
  bool isValid = audioProcessor.getCircuitEngine().isSimulationValid();
  oscilloscopeView->setSimulationValid(isValid);

  int nodeCount = audioProcessor.getCircuitGraph().getNodeCount();
  int probeNodeId = audioProcessor.getProbeNodeId();
  oscilloscopeView->setNodeInfo(probeNodeId, nodeCount);

  // Only update waveform if we have a valid probe node
  if (probeNodeId >= 0 && nodeCount > 0) {
    std::vector<float> samples;
    audioProcessor.getLatestSamples(samples);
    oscilloscopeView->updateWaveform(samples);
  }

  oscilloscopeView->repaint();
}

void CircuitsAudioProcessorEditor::updateControlPanel() {
  controlPanel->rebuildControls();
}

void CircuitsAudioProcessorEditor::autoProbe() {
  auto &graph = audioProcessor.getCircuitGraph();

  for (const auto &comp : graph.getComponents()) {
    if (comp->getType() == ComponentType::AudioOutput) {
      audioProcessor.setProbeNode(comp->getNode1());
      oscilloscopeView->setProbeActive(true);
      return;
    }
  }

  // If no output, try to find any node with a connection that is NOT ground
  for (int i = 1; i < graph.getNodeCount(); ++i) {
    audioProcessor.setProbeNode(i);
    oscilloscopeView->setProbeActive(true);
    return;
  }

  oscilloscopeView->setProbeActive(false);
}
