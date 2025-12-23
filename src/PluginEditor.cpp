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

CircuitsAudioProcessorEditor::~CircuitsAudioProcessorEditor() { stopTimer(); }

void CircuitsAudioProcessorEditor::paint(juce::Graphics &g) {
  // Dark background gradient
  juce::ColourGradient gradient(
      juce::Colour(0xFF1a1a2e), 0.0f, 0.0f, juce::Colour(0xFF16213e),
      static_cast<float>(getWidth()), static_cast<float>(getHeight()), false);
  g.setGradientFill(gradient);
  g.fillAll();

  // Subtle border lines
  g.setColour(juce::Colour(0xFF2d3a4f));
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
  // Update simulation state display
  oscilloscopeView->setSimulationRunning(
      audioProcessor.getCircuitEngine().isSimulationActive());

  // Update oscilloscope with latest voltage history
  int nodeCount = audioProcessor.getCircuitGraph().getNodeCount();
  oscilloscopeView->setNodeInfo(audioProcessor.getProbeNodeId(), nodeCount);

  if (nodeCount > 0) {
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

  // If no output, try to find any node with a connection
  if (graph.getNodeCount() > 0) {
    audioProcessor.setProbeNode(0); // Probe first node
    oscilloscopeView->setProbeActive(true);
  }
}
