#include "TopBar.h"
#include "../Engine/Components/AudioInput.h"

TopBar::TopBar(CircuitGraph &graph, CircuitEngine &engine)
    : circuitGraph(graph), circuitEngine(engine) {

  // START Button
  startButton = std::make_unique<juce::TextButton>("START");
  startButton->setColour(juce::TextButton::buttonColourId,
                         juce::Colour(0xFF2ecc71));
  startButton->onClick = [this]() {
    circuitEngine.setSimulationActive(true);
    updateButtonStates();
  };
  addAndMakeVisible(*startButton);

  // STOP Button
  stopButton = std::make_unique<juce::TextButton>("STOP");
  stopButton->setColour(juce::TextButton::buttonColourId,
                        juce::Colour(0xFFe74c3c));
  stopButton->onClick = [this]() {
    circuitEngine.setSimulationActive(false);
    updateButtonStates();
  };
  addAndMakeVisible(*stopButton);

  // Status Label
  statusLabel = std::make_unique<juce::Label>("", "STOPPED");
  statusLabel->setJustificationType(juce::Justification::centred);
  statusLabel->setColour(juce::Label::textColourId, juce::Colour(0xFF7f8c8d));
  addAndMakeVisible(*statusLabel);

  // Generator Header
  genHeaderLabel = std::make_unique<juce::Label>("", "SIGNAL GENERATOR");
  genHeaderLabel->setJustificationType(juce::Justification::centredLeft);
  genHeaderLabel->setFont(juce::FontOptions(14.0f, juce::Font::bold));
  addAndMakeVisible(*genHeaderLabel);

  // Source Selector
  signalSourceSelector = std::make_unique<juce::ComboBox>();
  signalSourceSelector->addItem("DAW Input", 1);
  signalSourceSelector->addItem("Sine Wave", 2);
  signalSourceSelector->addItem("Square Wave", 3);
  signalSourceSelector->addItem("White Noise", 4);
  signalSourceSelector->onChange = [this]() {
    if (auto *input = getFirstInput()) {
      input->setSource(
          static_cast<SignalSource>(signalSourceSelector->getSelectedId() - 1));
    }
  };
  addAndMakeVisible(*signalSourceSelector);

  // Freq
  frequencySlider = std::make_unique<juce::Slider>(
      juce::Slider::LinearHorizontal, juce::Slider::TextBoxLeft);
  frequencySlider->setRange(20.0, 20000.0, 1.0);
  frequencySlider->setSkewFactorFromMidPoint(440.0);
  frequencySlider->onValueChange = [this]() {
    if (auto *input = getFirstInput()) {
      input->setFrequency(frequencySlider->getValue());
    }
  };
  addAndMakeVisible(*frequencySlider);

  frequencyLabel = std::make_unique<juce::Label>("", "Hz");
  addAndMakeVisible(*frequencyLabel);

  // Amp
  amplitudeSlider = std::make_unique<juce::Slider>(
      juce::Slider::LinearHorizontal, juce::Slider::TextBoxLeft);
  amplitudeSlider->setRange(0.0, 10.0, 0.01);
  amplitudeSlider->onValueChange = [this]() {
    if (auto *input = getFirstInput()) {
      input->setAmplitude(amplitudeSlider->getValue());
    }
  };
  addAndMakeVisible(*amplitudeSlider);

  amplitudeLabel = std::make_unique<juce::Label>("", "V");
  addAndMakeVisible(*amplitudeLabel);

  startTimerHz(10);
  updateButtonStates();
}

TopBar::~TopBar() { stopTimer(); }

void TopBar::paint(juce::Graphics &g) {
  auto bounds = getLocalBounds().toFloat();

  // Background
  g.setColour(juce::Colours::magenta);
  g.fillRect(bounds);

  // Border
  g.setColour(juce::Colour(0xFF2d3a4f));
  g.drawHorizontalLine(getHeight() - 1, 0.0f, static_cast<float>(getWidth()));

  // Section dividers
  g.drawVerticalLine(180, 10.0f, static_cast<float>(getHeight() - 10));
}

void TopBar::resized() {
  auto bounds = getLocalBounds();

  // Left: simulation controls
  auto simBounds = bounds.removeFromLeft(180).reduced(10, 5);
  auto buttonArea = simBounds.removeFromTop(30);
  startButton->setBounds(
      buttonArea.removeFromLeft(simBounds.getWidth() / 2 - 5));
  buttonArea.removeFromLeft(10);
  stopButton->setBounds(buttonArea);
  statusLabel->setBounds(simBounds);

  // Right: generator controls
  bounds.removeFromLeft(10);
  genHeaderLabel->setBounds(bounds.removeFromLeft(150));

  auto genArea = bounds.reduced(10, 5);
  signalSourceSelector->setBounds(
      genArea.removeFromLeft(120).withSizeKeepingCentre(120, 24));
  genArea.removeFromLeft(15);

  auto freqArea = genArea.removeFromLeft(200);
  frequencySlider->setBounds(freqArea.removeFromTop(30));

  genArea.removeFromLeft(10);
  auto ampArea = genArea.removeFromLeft(200);
  amplitudeSlider->setBounds(ampArea.removeFromTop(30));
}

void TopBar::timerCallback() {
  updateButtonStates();

  // Synchronize controls with internal input state if it exists
  if (auto *input = getFirstInput()) {
    signalSourceSelector->setSelectedId(
        static_cast<int>(input->getSource()) + 1, juce::dontSendNotification);

    if (!frequencySlider->isMouseButtonDown()) {
      frequencySlider->setValue(input->getFrequency(),
                                juce::dontSendNotification);
    }
    if (!amplitudeSlider->isMouseButtonDown()) {
      amplitudeSlider->setValue(input->getAmplitude(),
                                juce::dontSendNotification);
    }

    genHeaderLabel->setText("SIGNAL GENERATOR", juce::dontSendNotification);
    genHeaderLabel->setColour(juce::Label::textColourId, juce::Colours::white);
  } else {
    genHeaderLabel->setText("NO INPUT NODE", juce::dontSendNotification);
    genHeaderLabel->setColour(juce::Label::textColourId,
                              juce::Colours::red.withAlpha(0.6f));
  }
}

void TopBar::updateButtonStates() {
  bool active = circuitEngine.isSimulationActive();
  startButton->setEnabled(!active);
  stopButton->setEnabled(active);

  if (active) {
    statusLabel->setText("RUNNING", juce::dontSendNotification);
    statusLabel->setColour(juce::Label::textColourId, juce::Colour(0xFF2ecc71));
  } else {
    statusLabel->setText("STOPPED", juce::dontSendNotification);
    statusLabel->setColour(juce::Label::textColourId, juce::Colour(0xFFe74c3c));
  }
}

AudioInput *TopBar::getFirstInput() {
  for (const auto &comp : circuitGraph.getComponents()) {
    if (comp->getType() == ComponentType::AudioInput) {
      return static_cast<AudioInput *>(comp.get());
    }
  }
  return nullptr;
}
