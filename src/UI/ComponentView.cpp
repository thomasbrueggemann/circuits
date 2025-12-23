#include "ComponentView.h"
#include "../Engine/Components/Component.h"
#include "../Engine/Components/Potentiometer.h"
#include "../Engine/Components/Switch.h"
#include "../Engine/Components/VacuumTube.h"

ComponentView::ComponentView(CircuitComponent *comp, CircuitGraph &graph)
    : component(comp), circuitGraph(graph) {
  if (component) {
    canvasPosition = component->getPosition();
  }
}

ComponentView::~ComponentView() = default;

void ComponentView::draw(juce::Graphics &g,
                         const juce::AffineTransform &transform, float zoom) {
  if (!component)
    return;

  auto bounds = juce::Rectangle<float>(-WIDTH / 2, -HEIGHT / 2, WIDTH, HEIGHT);

  // Apply transform
  g.saveState();
  g.addTransform(transform);

  // Selection highlight
  if (selected) {
    g.setColour(juce::Colour(0xFF00aaff).withAlpha(0.3f));
    g.fillRoundedRectangle(bounds.expanded(5), 8.0f);
    g.setColour(juce::Colour(0xFF00aaff));
    g.drawRoundedRectangle(bounds.expanded(5), 8.0f, 2.0f);
  }

  // Draw based on component type
  switch (component->getType()) {
  case ComponentType::Resistor:
    drawResistor(g, bounds);
    break;
  case ComponentType::Capacitor:
    drawCapacitor(g, bounds);
    break;
  case ComponentType::Potentiometer:
    drawPotentiometer(g, bounds);
    break;
  case ComponentType::Switch:
    drawSwitch(g, bounds);
    break;
  case ComponentType::VacuumTube:
    drawVacuumTube(g, bounds);
    break;
  case ComponentType::AudioInput:
    drawAudioInput(g, bounds);
    break;
  case ComponentType::AudioOutput:
    drawAudioOutput(g, bounds);
    break;
  default:
    g.setColour(juce::Colour(0xFFcccccc));
    g.drawRect(bounds, 2.0f);
    break;
  }

  // Draw label
  g.setColour(juce::Colour(0xFFaabbcc));
  g.setFont(10.0f / zoom);
  g.drawText(component->getName(), bounds.translated(0, HEIGHT / 2 + 5),
             juce::Justification::centredTop);

  // Draw value
  g.setFont(8.0f / zoom);
  g.setColour(juce::Colour(0xFF8899aa));
  g.drawText(component->getValueString(), bounds.translated(0, HEIGHT / 2 + 15),
             juce::Justification::centredTop);

  g.restoreState();
}

void ComponentView::drawResistor(juce::Graphics &g,
                                 juce::Rectangle<float> bounds) {
  g.setColour(juce::Colour(0xFF00ccff));

  float cx = bounds.getCentreX();
  float cy = bounds.getCentreY();
  float w = bounds.getWidth() * 0.8f;

  // Horizontal leads
  g.drawLine(-WIDTH / 2, cy, -w / 2, cy, 2.0f);
  g.drawLine(w / 2, cy, WIDTH / 2, cy, 2.0f);

  // Zigzag body
  juce::Path path;
  path.startNewSubPath(-w / 2, cy);
  float zigW = w / 6;
  float zigH = 8;
  path.lineTo(-w / 2 + zigW * 0.5f, cy - zigH);
  path.lineTo(-w / 2 + zigW * 1.5f, cy + zigH);
  path.lineTo(-w / 2 + zigW * 2.5f, cy - zigH);
  path.lineTo(-w / 2 + zigW * 3.5f, cy + zigH);
  path.lineTo(-w / 2 + zigW * 4.5f, cy - zigH);
  path.lineTo(-w / 2 + zigW * 5.5f, cy);
  path.lineTo(w / 2, cy);
  g.strokePath(path, juce::PathStrokeType(2.0f));
}

void ComponentView::drawCapacitor(juce::Graphics &g,
                                  juce::Rectangle<float> bounds) {
  g.setColour(juce::Colour(0xFF00ff88));

  float cx = bounds.getCentreX();
  float cy = bounds.getCentreY();
  float plateGap = 8.0f;

  // Leads
  g.drawLine(-WIDTH / 2, cy, cx - plateGap / 2, cy, 2.0f);
  g.drawLine(cx + plateGap / 2, cy, WIDTH / 2, cy, 2.0f);

  // Plates
  g.drawLine(cx - plateGap / 2, cy - 12, cx - plateGap / 2, cy + 12, 2.5f);
  g.drawLine(cx + plateGap / 2, cy - 12, cx + plateGap / 2, cy + 12, 2.5f);
}

void ComponentView::drawPotentiometer(juce::Graphics &g,
                                      juce::Rectangle<float> bounds) {
  g.setColour(juce::Colour(0xFFffaa00));

  float cx = bounds.getCentreX();
  float cy = bounds.getCentreY();
  float w = bounds.getWidth() * 0.7f;

  // Leads
  g.drawLine(-WIDTH / 2, cy, -w / 2, cy, 2.0f);
  g.drawLine(w / 2, cy, WIDTH / 2, cy, 2.0f);

  // Resistor body
  juce::Path path;
  path.startNewSubPath(-w / 2, cy);
  float zigW = w / 5;
  float zigH = 6;
  path.lineTo(-w / 2 + zigW * 0.5f, cy - zigH);
  path.lineTo(-w / 2 + zigW * 1.5f, cy + zigH);
  path.lineTo(-w / 2 + zigW * 2.5f, cy - zigH);
  path.lineTo(-w / 2 + zigW * 3.5f, cy + zigH);
  path.lineTo(-w / 2 + zigW * 4.5f, cy);
  path.lineTo(w / 2, cy);
  g.strokePath(path, juce::PathStrokeType(2.0f));

  // Wiper arrow
  g.drawArrow(juce::Line<float>(cx, cy + 15, cx, cy + 4), 2.0f, 8.0f, 6.0f);

  // Wiper terminal
  g.drawLine(cx, cy - HEIGHT / 2, cx, cy - 10, 2.0f);
}

void ComponentView::drawSwitch(juce::Graphics &g,
                               juce::Rectangle<float> bounds) {
  auto *sw = dynamic_cast<Switch *>(component);
  bool closed = sw ? sw->isClosed() : false;

  g.setColour(closed ? juce::Colour(0xFF00ff00) : juce::Colour(0xFFff4444));

  float cx = bounds.getCentreX();
  float cy = bounds.getCentreY();

  // Leads
  g.drawLine(-WIDTH / 2, cy, -10, cy, 2.0f);
  g.drawLine(10, cy, WIDTH / 2, cy, 2.0f);

  // Contact points
  g.fillEllipse(-13, cy - 3, 6, 6);
  g.fillEllipse(7, cy - 3, 6, 6);

  // Switch arm
  if (closed) {
    g.drawLine(-10, cy, 10, cy, 2.5f);
  } else {
    g.drawLine(-10, cy, 5, cy - 12, 2.5f);
  }
}

void ComponentView::drawVacuumTube(juce::Graphics &g,
                                   juce::Rectangle<float> bounds) {
  g.setColour(juce::Colour(0xFFff8800));

  float cx = bounds.getCentreX();
  float cy = bounds.getCentreY();
  float radius = 16;

  // Envelope (circle)
  g.drawEllipse(cx - radius, cy - radius, radius * 2, radius * 2, 2.0f);

  // Plate (top)
  g.drawLine(cx - 8, cy - 6, cx + 8, cy - 6, 2.0f);
  g.drawLine(cx, cy - radius - 5, cx, cy - 6, 2.0f);

  // Grid (middle)
  for (int i = -6; i <= 6; i += 4) {
    g.drawLine(static_cast<float>(cx + i - 1), cy,
               static_cast<float>(cx + i + 1), cy, 1.5f);
  }
  g.drawLine(cx - radius - 5, cy, cx - 8, cy, 2.0f);

  // Cathode (bottom)
  g.drawLine(cx - 5, cy + 6, cx + 5, cy + 6, 2.0f);
  g.drawLine(cx, cy + radius + 5, cx, cy + 6, 2.0f);
}

void ComponentView::drawAudioInput(juce::Graphics &g,
                                   juce::Rectangle<float> bounds) {
  g.setColour(juce::Colour(0xFF00ffaa));

  float cx = bounds.getCentreX();
  float cy = bounds.getCentreY();

  // Arrow pointing in
  g.drawArrow(juce::Line<float>(cx - 20, cy, cx + 5, cy), 3.0f, 12.0f, 10.0f);

  // Input symbol
  g.drawRoundedRectangle(cx, cy - 8, 16, 16, 4.0f, 2.0f);

  // "IN" label inside
  g.setFont(10.0f);
  g.drawText("IN", juce::Rectangle<float>(cx, cy - 8, 16, 16),
             juce::Justification::centred);

  // Output lead
  g.drawLine(cx + 16, cy, WIDTH / 2, cy, 2.0f);
}

void ComponentView::drawAudioOutput(juce::Graphics &g,
                                    juce::Rectangle<float> bounds) {
  g.setColour(juce::Colour(0xFFff00aa));

  float cx = bounds.getCentreX();
  float cy = bounds.getCentreY();

  // Input lead
  g.drawLine(-WIDTH / 2, cy, cx - 16, cy, 2.0f);

  // Output symbol
  g.drawRoundedRectangle(cx - 16, cy - 8, 16, 16, 4.0f, 2.0f);

  // Arrow pointing out
  g.drawArrow(juce::Line<float>(cx, cy, cx + 20, cy), 3.0f, 12.0f, 10.0f);

  // "OUT" label above
  g.setFont(8.0f);
  g.drawText("OUT", juce::Rectangle<float>(cx - 16, cy - 8, 16, 16),
             juce::Justification::centred);
}

juce::Rectangle<float> ComponentView::getBoundsInCanvas() const {
  return juce::Rectangle<float>(canvasPosition.x - WIDTH / 2,
                                canvasPosition.y - HEIGHT / 2, WIDTH, HEIGHT);
}

std::vector<std::pair<int, juce::Point<float>>>
ComponentView::getTerminalPositions() const {
  std::vector<std::pair<int, juce::Point<float>>> terminals;

  if (!component)
    return terminals;

  // Standard two-terminal components
  terminals.push_back({component->getNode1(),
                       canvasPosition + juce::Point<float>(-WIDTH / 2, 0)});
  terminals.push_back({component->getNode2(),
                       canvasPosition + juce::Point<float>(WIDTH / 2, 0)});

  // Three-terminal components
  if (component->getType() == ComponentType::Potentiometer) {
    auto *pot = dynamic_cast<Potentiometer *>(component);
    if (pot) {
      terminals.push_back(
          {pot->getNode3(),
           canvasPosition + juce::Point<float>(0, -HEIGHT / 2)});
    }
  } else if (component->getType() == ComponentType::VacuumTube) {
    auto *tube = dynamic_cast<VacuumTube *>(component);
    if (tube) {
      // Grid at left, cathode at bottom, plate at top
      terminals.clear();
      terminals.push_back(
          {tube->getNode1(),
           canvasPosition + juce::Point<float>(-WIDTH / 2 - 5, 0)}); // Grid
      terminals.push_back(
          {tube->getNode2(),
           canvasPosition + juce::Point<float>(0, HEIGHT / 2 + 5)}); // Cathode
      terminals.push_back(
          {tube->getPlateNode(),
           canvasPosition + juce::Point<float>(0, -HEIGHT / 2 - 5)}); // Plate
    }
  }

  return terminals;
}

void ComponentView::showValueEditor() {
  if (!component)
    return;

  // Create alert window for value editing
  auto *window = new juce::AlertWindow(
      "Edit " + component->getName(),
      "Enter new value:", juce::MessageBoxIconType::NoIcon);

  window->addTextEditor("value", component->getValueString(), "Value:");
  window->addButton("OK", 1, juce::KeyPress(juce::KeyPress::returnKey));
  window->addButton("Cancel", 0, juce::KeyPress(juce::KeyPress::escapeKey));

  window->enterModalState(
      true, juce::ModalCallbackFunction::create([this, window](int result) {
        if (result == 1) {
          auto text = window->getTextEditorContents("value");

          // Parse value (handle k, M, u, n, p suffixes)
          double newValue = 0.0;
          double multiplier = 1.0;

          if (text.endsWithIgnoreCase("M"))
            multiplier = 1e6;
          else if (text.endsWithIgnoreCase("k"))
            multiplier = 1e3;
          else if (text.endsWithIgnoreCase("u") || text.endsWithIgnoreCase("µ"))
            multiplier = 1e-6;
          else if (text.endsWithIgnoreCase("n"))
            multiplier = 1e-9;
          else if (text.endsWithIgnoreCase("p"))
            multiplier = 1e-12;

          // Remove suffix for parsing
          auto numText = text.trimCharactersAtEnd("MkKuUnNpPΩFµ ");
          newValue = numText.getDoubleValue() * multiplier;

          if (newValue > 0) {
            component->setValue(newValue);
          }
        }
        delete window;
      }),
      true);
}
