#include "ControlPanel.h"
#include "../Engine/Components/Potentiometer.h"
#include "../Engine/Components/Switch.h"

//==============================================================================
// VintageLookAndFeel Implementation
//==============================================================================

void ControlPanel::VintageLookAndFeel::drawRotarySlider(
    juce::Graphics &g, int x, int y, int width, int height,
    float sliderPosProportional, float rotaryStartAngle, float rotaryEndAngle,
    juce::Slider &slider) {
  juce::ignoreUnused(slider);

  auto bounds = juce::Rectangle<int>(x, y, width, height).toFloat().reduced(4);
  auto radius = juce::jmin(bounds.getWidth(), bounds.getHeight()) / 2.0f;
  auto centreX = bounds.getCentreX();
  auto centreY = bounds.getCentreY();
  auto rx = centreX - radius;
  auto ry = centreY - radius;
  auto rw = radius * 2.0f;
  auto angle = rotaryStartAngle +
               sliderPosProportional * (rotaryEndAngle - rotaryStartAngle);

  // Outer ring (dark)
  g.setColour(juce::Colour(0xFF1a1a2e));
  g.fillEllipse(rx - 3, ry - 3, rw + 6, rw + 6);

  // Main knob body gradient
  juce::ColourGradient gradient(juce::Colour(0xFF3a4a5e), centreX, ry,
                                juce::Colour(0xFF2a3a4e), centreX, ry + rw,
                                false);
  g.setGradientFill(gradient);
  g.fillEllipse(rx, ry, rw, rw);

  // Inner shadow
  g.setColour(juce::Colour(0xFF1a2a3e).withAlpha(0.5f));
  g.drawEllipse(rx + 2, ry + 2, rw - 4, rw - 4, 2.0f);

  // Highlight arc (shows position)
  juce::Path arcPath;
  arcPath.addCentredArc(centreX, centreY, radius * 0.85f, radius * 0.85f, 0.0f,
                        rotaryStartAngle, angle, true);
  g.setColour(juce::Colour(0xFF00aaff));
  g.strokePath(arcPath, juce::PathStrokeType(3.0f));

  // Pointer line
  juce::Path pointer;
  auto pointerLength = radius * 0.6f;
  auto pointerThickness = radius * 0.15f;
  pointer.addRoundedRectangle(-pointerThickness / 2, -radius + 6,
                              pointerThickness, pointerLength, 2.0f);
  pointer.applyTransform(
      juce::AffineTransform::rotation(angle).translated(centreX, centreY));

  g.setColour(juce::Colour(0xFFdddddd));
  g.fillPath(pointer);

  // Center cap
  float capSize = radius * 0.3f;
  g.setColour(juce::Colour(0xFF5a6a7e));
  g.fillEllipse(centreX - capSize, centreY - capSize, capSize * 2, capSize * 2);
}

void ControlPanel::VintageLookAndFeel::drawToggleButton(
    juce::Graphics &g, juce::ToggleButton &button,
    bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) {
  juce::ignoreUnused(shouldDrawButtonAsDown);

  auto bounds = button.getLocalBounds().toFloat().reduced(4);
  bool isOn = button.getToggleState();

  // Switch track
  float trackHeight = 24.0f;
  float trackWidth = 44.0f;
  auto trackBounds = juce::Rectangle<float>(
      bounds.getCentreX() - trackWidth / 2,
      bounds.getCentreY() - trackHeight / 2, trackWidth, trackHeight);

  g.setColour(isOn ? juce::Colour(0xFF00aa55) : juce::Colour(0xFF3a4a5e));
  g.fillRoundedRectangle(trackBounds, trackHeight / 2);

  // Inner shadow
  g.setColour(juce::Colour(0xFF1a2a3e).withAlpha(0.3f));
  g.drawRoundedRectangle(trackBounds.reduced(1), trackHeight / 2, 1.5f);

  // Switch knob
  float knobSize = trackHeight - 4;
  float knobX =
      isOn ? trackBounds.getRight() - knobSize - 2 : trackBounds.getX() + 2;

  juce::ColourGradient knobGradient(
      juce::Colour(0xFFeeeeee), knobX + knobSize / 2, trackBounds.getY(),
      juce::Colour(0xFFcccccc), knobX + knobSize / 2, trackBounds.getBottom(),
      false);
  g.setGradientFill(knobGradient);
  g.fillEllipse(knobX, trackBounds.getCentreY() - knobSize / 2, knobSize,
                knobSize);

  // Highlight on hover
  if (shouldDrawButtonAsHighlighted) {
    g.setColour(juce::Colours::white.withAlpha(0.1f));
    g.fillRoundedRectangle(trackBounds, trackHeight / 2);
  }

  // Label
  g.setColour(juce::Colour(0xFFaabbcc));
  g.setFont(11.0f);
  auto textBounds = bounds;
  textBounds.removeFromTop(trackBounds.getBottom() - bounds.getY() + 5);
  g.drawText(button.getButtonText(), textBounds,
             juce::Justification::centredTop);
}

//==============================================================================
// ControlPanel Implementation
//==============================================================================

ControlPanel::ControlPanel(CircuitGraph &graph, CircuitEngine &engine)
    : circuitGraph(graph), circuitEngine(engine) {
  setLookAndFeel(&vintageLook);
  rebuildControls();
}

ControlPanel::~ControlPanel() { setLookAndFeel(nullptr); }

void ControlPanel::paint(juce::Graphics &g) {
  // Dark panel background
  g.fillAll(juce::Colour(0xFF1a2332));

  // Title
  g.setColour(juce::Colour(0xFF8899aa));
  g.setFont(14.0f);
  g.drawText("Controls", getLocalBounds().removeFromTop(35),
             juce::Justification::centred);

  // Separator line
  g.setColour(juce::Colour(0xFF2a3a4e));
  g.drawHorizontalLine(38, 10.0f, static_cast<float>(getWidth() - 10));

  // Message if no controls
  if (knobs.empty() && switches.empty()) {
    g.setColour(juce::Colour(0xFF5a6a7a));
    g.setFont(11.0f);
    g.drawText("Add potentiometers\nor switches to see\ncontrols here",
               getLocalBounds().reduced(10).translated(0, 50),
               juce::Justification::centredTop);
  }
}

void ControlPanel::resized() {
  auto bounds = getLocalBounds();
  bounds.removeFromTop(45); // Title space

  int knobSize = 80;
  int switchHeight = 60;
  int padding = 10;

  // Layout knobs
  for (auto &knob : knobs) {
    auto knobBounds = bounds.removeFromTop(knobSize + 20);
    knob.slider->setBounds(knobBounds.reduced(padding).removeFromTop(knobSize));
    knob.label->setBounds(knobBounds.removeFromBottom(16));
  }

  // Layout switches
  for (auto &sw : switches) {
    sw.toggle->setBounds(bounds.removeFromTop(switchHeight).reduced(padding));
  }
}

void ControlPanel::rebuildControls() {
  knobs.clear();
  switches.clear();

  // Find all potentiometers and switches
  for (const auto &comp : circuitGraph.getComponents()) {
    if (comp->getType() == ComponentType::Potentiometer) {
      auto *pot = dynamic_cast<Potentiometer *>(comp.get());
      if (pot) {
        createKnob(pot->getId(), pot->getName(), 0.0, 1.0,
                   pot->getWiperPosition());
      }
    } else if (comp->getType() == ComponentType::Switch) {
      auto *sw = dynamic_cast<Switch *>(comp.get());
      if (sw) {
        createSwitch(sw->getId(), sw->getName(), sw->isClosed());
      }
    }
  }

  resized();
  repaint();
}

void ControlPanel::createKnob(int componentId, const juce::String &name,
                              double minVal, double maxVal, double defaultVal) {
  KnobControl knob;
  knob.componentId = componentId;

  // Create slider
  knob.slider = std::make_unique<juce::Slider>(
      juce::Slider::RotaryHorizontalVerticalDrag, juce::Slider::NoTextBox);
  knob.slider->setRange(minVal, maxVal, 0.001);
  knob.slider->setValue(defaultVal);
  knob.slider->setLookAndFeel(&vintageLook);

  // Callback to update circuit
  knob.slider->onValueChange = [this, componentId]() {
    auto *comp = circuitGraph.getComponent(componentId);
    if (auto *pot = dynamic_cast<Potentiometer *>(comp)) {
      // Find the slider
      for (auto &k : knobs) {
        if (k.componentId == componentId) {
          pot->setWiperPosition(k.slider->getValue());
          circuitEngine.setComponentValue(componentId,
                                          pot->getTotalResistance());
          break;
        }
      }
    }
  };

  // Create label
  knob.label = std::make_unique<juce::Label>("", name);
  knob.label->setJustificationType(juce::Justification::centred);
  knob.label->setColour(juce::Label::textColourId, juce::Colour(0xFFaabbcc));

  addAndMakeVisible(*knob.slider);
  addAndMakeVisible(*knob.label);

  knobs.push_back(std::move(knob));
}

void ControlPanel::createSwitch(int componentId, const juce::String &name,
                                bool defaultState) {
  SwitchControl sw;
  sw.componentId = componentId;

  sw.toggle = std::make_unique<juce::ToggleButton>(name);
  sw.toggle->setToggleState(defaultState, juce::dontSendNotification);
  sw.toggle->setLookAndFeel(&vintageLook);

  // Callback to update circuit
  sw.toggle->onClick = [this, componentId]() {
    auto *comp = circuitGraph.getComponent(componentId);
    if (auto *switchComp = dynamic_cast<Switch *>(comp)) {
      for (auto &s : switches) {
        if (s.componentId == componentId) {
          switchComp->setClosed(s.toggle->getToggleState());
          // Trigger circuit rebuild
          circuitEngine.setComponentValue(componentId,
                                          switchComp->getResistance());
          break;
        }
      }
    }
  };

  addAndMakeVisible(*sw.toggle);

  switches.push_back(std::move(sw));
}
