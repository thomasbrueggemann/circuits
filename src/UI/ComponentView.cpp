#include "ComponentView.h"
#include "../Engine/Components/Component.h"
#include "../Engine/Components/Diode.h"
#include "../Engine/Components/DiodePair.h"
#include "../Engine/Components/Ground.h"
#include "../Engine/Components/Inductor.h"
#include "../Engine/Components/Potentiometer.h"
#include "../Engine/Components/SoftClipper.h"
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

  // Apply transform (position and scale)
  g.saveState();
  g.addTransform(transform);

  // Apply rotation
  float rotationRad =
      juce::degreesToRadians(static_cast<float>(component->getRotation()));
  g.addTransform(juce::AffineTransform::rotation(rotationRad));

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
  case ComponentType::Inductor:
    drawInductor(g, bounds);
    break;
  case ComponentType::Potentiometer:
    drawPotentiometer(g, bounds);
    break;
  case ComponentType::Switch:
    drawSwitch(g, bounds);
    break;
  case ComponentType::Diode:
    drawDiode(g, bounds);
    break;
  case ComponentType::DiodePair:
    drawDiodePair(g, bounds);
    break;
  case ComponentType::SoftClipper:
    drawSoftClipper(g, bounds);
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
  case ComponentType::Ground:
    drawGround(g, bounds);
    break;
  default:
    g.setColour(juce::Colour(0xFFcccccc));
    g.drawRect(bounds, 2.0f);
    break;
  }

  g.restoreState();

  // Draw labels outside the rotation transform
  g.saveState();
  g.addTransform(transform);

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
  float cy = bounds.getCentreY();
  float w = bounds.getWidth() * 0.7f;
  float bodyH = 10.0f;

  // Leads
  g.setColour(juce::Colour(0xFF888888));
  g.drawLine(-WIDTH / 2, cy, -w / 2, cy, 1.5f);
  g.drawLine(w / 2, cy, WIDTH / 2, cy, 1.5f);

  // Resistor body (beige/tan)
  g.setColour(juce::Colour(0xFFd2b48c));
  g.fillRoundedRectangle(-w / 2, cy - bodyH / 2, w, bodyH, 2.0f);
  g.setColour(juce::Colours::black.withAlpha(0.3f));
  g.drawRoundedRectangle(-w / 2, cy - bodyH / 2, w, bodyH, 2.0f, 1.0f);

  // Color bands (simulated for now, let's add 3 bands)
  float bandW = 3.0f;
  float bandGap = (w - bandW * 4) / 5;

  // Band 1
  g.setColour(juce::Colour(0xFF8b4513)); // Brown
  g.fillRect(-w / 2 + bandGap, cy - bodyH / 2, bandW, bodyH);

  // Band 2
  g.setColour(juce::Colours::black);
  g.fillRect(-w / 2 + bandGap * 2 + bandW, cy - bodyH / 2, bandW, bodyH);

  // Multiplier
  g.setColour(juce::Colour(0xFFff4500)); // Orange
  g.fillRect(-w / 2 + bandGap * 3 + bandW * 2, cy - bodyH / 2, bandW, bodyH);

  // Tolerance (Gold)
  g.setColour(juce::Colour(0xFFffd700));
  g.fillRect(w / 2 - bandGap - bandW, cy - bodyH / 2, bandW, bodyH);
}

void ComponentView::drawCapacitor(juce::Graphics &g,
                                  juce::Rectangle<float> bounds) {
  float cx = bounds.getCentreX();
  float cy = bounds.getCentreY();
  float bodyW = 14.0f;
  float bodyH = 20.0f;

  // Leads
  g.setColour(juce::Colour(0xFF888888));
  g.drawLine(-WIDTH / 2, cy, -bodyW / 2, cy, 1.5f);
  g.drawLine(bodyW / 2, cy, WIDTH / 2, cy, 1.5f);

  // Capacitor body (Blue electrolytic cylinder look)
  g.setColour(juce::Colour(0xFF2255aa));
  g.fillRoundedRectangle(-bodyW / 2, cy - bodyH / 2, bodyW, bodyH, 1.0f);

  // Negative stripe
  g.setColour(juce::Colour(0xFFbbbbbb));
  g.fillRect(-bodyW / 2 + 2, cy - bodyH / 2, 4.0f, bodyH);

  // Shine/Highlight
  g.setColour(juce::Colours::white.withAlpha(0.2f));
  g.fillRect(bodyW / 2 - 4, cy - bodyH / 2, 2.0f, bodyH);

  g.setColour(juce::Colours::black.withAlpha(0.4f));
  g.drawRoundedRectangle(-bodyW / 2, cy - bodyH / 2, bodyW, bodyH, 1.0f, 1.0f);
}

void ComponentView::drawInductor(juce::Graphics &g,
                                 juce::Rectangle<float> bounds) {
  float cy = bounds.getCentreY();
  float w = bounds.getWidth() * 0.7f;
  float coilH = 10.0f;

  // Leads
  g.setColour(juce::Colour(0xFF888888));
  g.drawLine(-WIDTH / 2, cy, -w / 2, cy, 1.5f);
  g.drawLine(w / 2, cy, WIDTH / 2, cy, 1.5f);

  // Coil body (copper wire look)
  g.setColour(juce::Colour(0xFFb87333)); // Copper color
  
  // Draw 4 coil humps
  juce::Path coil;
  float coilW = w / 4;
  coil.startNewSubPath(-w / 2, cy);
  for (int i = 0; i < 4; i++) {
    float startX = -w / 2 + i * coilW;
    coil.addCentredArc(startX + coilW / 2, cy, coilW / 2, coilH / 2, 0,
                       juce::MathConstants<float>::pi, 0, false);
  }
  coil.lineTo(w / 2, cy);
  g.strokePath(coil, juce::PathStrokeType(2.5f));

  // Shine highlight
  g.setColour(juce::Colour(0xFFd4956a));
  for (int i = 0; i < 4; i++) {
    float cx = -w / 2 + coilW / 2 + i * coilW;
    g.drawLine(cx - 2, cy - coilH / 2 + 1, cx + 2, cy - coilH / 2 + 1, 1.0f);
  }
}

void ComponentView::drawDiode(juce::Graphics &g,
                              juce::Rectangle<float> bounds) {
  float cy = bounds.getCentreY();
  float w = bounds.getWidth() * 0.6f;
  float triH = 12.0f;

  // Leads
  g.setColour(juce::Colour(0xFF888888));
  g.drawLine(-WIDTH / 2, cy, -w / 3, cy, 1.5f);
  g.drawLine(w / 3, cy, WIDTH / 2, cy, 1.5f);

  // Diode body (black epoxy)
  g.setColour(juce::Colour(0xFF222222));
  g.fillRoundedRectangle(-w / 3, cy - triH / 2 - 2, w * 2 / 3, triH + 4, 2.0f);

  // Cathode band (silver stripe)
  g.setColour(juce::Colour(0xFFcccccc));
  g.fillRect(w / 3 - 6, cy - triH / 2 - 2, 4.0f, triH + 4);

  // Triangle symbol overlay
  g.setColour(juce::Colour(0xFF00ccff).withAlpha(0.5f));
  juce::Path tri;
  tri.startNewSubPath(-w / 6, cy - triH / 2 + 2);
  tri.lineTo(w / 6 - 4, cy);
  tri.lineTo(-w / 6, cy + triH / 2 - 2);
  tri.closeSubPath();
  g.fillPath(tri);
}

void ComponentView::drawDiodePair(juce::Graphics &g,
                                  juce::Rectangle<float> bounds) {
  float cy = bounds.getCentreY();
  float w = bounds.getWidth() * 0.5f;
  float h = bounds.getHeight() * 0.5f;

  // Leads
  g.setColour(juce::Colour(0xFF888888));
  g.drawLine(-WIDTH / 2, cy, -w / 2, cy, 1.5f);
  g.drawLine(w / 2, cy, WIDTH / 2, cy, 1.5f);

  // Body background
  g.setColour(juce::Colour(0xFF333333));
  g.fillRoundedRectangle(-w / 2, cy - h / 2, w, h, 3.0f);
  g.setColour(juce::Colour(0xFF555555));
  g.drawRoundedRectangle(-w / 2, cy - h / 2, w, h, 3.0f, 1.0f);

  // Diode symbols inside
  g.setColour(juce::Colour(0xFF00ccff));
  float triSize = 6.0f;
  float offset = 5.0f;
  
  // Top diode (pointing right)
  juce::Path tri1;
  tri1.startNewSubPath(-triSize, cy - offset - triSize / 2);
  tri1.lineTo(triSize, cy - offset);
  tri1.lineTo(-triSize, cy - offset + triSize / 2);
  tri1.closeSubPath();
  g.strokePath(tri1, juce::PathStrokeType(1.5f));
  g.drawLine(triSize, cy - offset - triSize / 2, triSize, cy - offset + triSize / 2, 1.5f);

  // Bottom diode (pointing left)
  juce::Path tri2;
  tri2.startNewSubPath(triSize, cy + offset - triSize / 2);
  tri2.lineTo(-triSize, cy + offset);
  tri2.lineTo(triSize, cy + offset + triSize / 2);
  tri2.closeSubPath();
  g.strokePath(tri2, juce::PathStrokeType(1.5f));
  g.drawLine(-triSize, cy + offset - triSize / 2, -triSize, cy + offset + triSize / 2, 1.5f);
}

void ComponentView::drawSoftClipper(juce::Graphics &g,
                                    juce::Rectangle<float> bounds) {
  float cx = bounds.getCentreX();
  float cy = bounds.getCentreY();
  float w = bounds.getWidth() * 0.5f;
  float h = bounds.getHeight() * 0.5f;

  // Leads
  g.setColour(juce::Colour(0xFF888888));
  g.drawLine(-WIDTH / 2, cy, -w / 2, cy, 1.5f);
  g.drawLine(w / 2, cy, WIDTH / 2, cy, 1.5f);

  // IC/chip body
  g.setColour(juce::Colour(0xFF1a1a2e));
  g.fillRoundedRectangle(-w / 2, cy - h / 2, w, h, 2.0f);
  g.setColour(juce::Colour(0xFF444466));
  g.drawRoundedRectangle(-w / 2, cy - h / 2, w, h, 2.0f, 1.0f);

  // Tanh curve symbol
  g.setColour(juce::Colour(0xFFff8800));
  juce::Path curve;
  curve.startNewSubPath(-w / 2 + 4, cy + h / 4);
  curve.cubicTo(-w / 6, cy + h / 5,
                -w / 8, cy,
                0, cy);
  curve.cubicTo(w / 8, cy,
                w / 6, cy - h / 5,
                w / 2 - 4, cy - h / 4);
  g.strokePath(curve, juce::PathStrokeType(2.0f));

  // Pin 1 marker (dot)
  g.setColour(juce::Colour(0xFF888888));
  g.fillEllipse(-w / 2 + 3, cy - h / 2 + 3, 4, 4);
}

void ComponentView::drawPotentiometer(juce::Graphics &g,
                                      juce::Rectangle<float> bounds) {
  float cx = bounds.getCentreX();
  float cy = bounds.getCentreY();
  float radius = 15.0f;

  // Knob body (Dark plastic)
  g.setColour(juce::Colour(0xFF222222));
  g.fillEllipse(cx - radius, cy - radius, radius * 2, radius * 2);

  // Outer ring/shadow
  g.setColour(juce::Colours::black);
  g.drawEllipse(cx - radius, cy - radius, radius * 2, radius * 2, 1.5f);

  // Pointer line
  g.setColour(juce::Colour(0xFFff8800));
  juce::Path p;
  p.addRectangle(-1.0f, -radius, 2.0f, radius * 0.4f);
  p.applyTransform(juce::AffineTransform::rotation(-2.0f).translated(cx, cy));
  g.fillPath(p);

  // Terminals (3 pins)
  g.setColour(juce::Colour(0xFF888888));
  g.drawLine(-WIDTH / 2, cy, cx - radius, cy, 1.5f);  // Left
  g.drawLine(cx + radius, cy, WIDTH / 2, cy, 1.5f);   // Right
  g.drawLine(cx, cy - radius, cx, -HEIGHT / 2, 1.5f); // Top (wiper)
}

void ComponentView::drawSwitch(juce::Graphics &g,
                               juce::Rectangle<float> bounds) {
  auto *sw = dynamic_cast<Switch *>(component);
  bool closed = sw ? sw->isClosed() : false;

  float cy = bounds.getCentreY();

  // Metal body/shadow
  g.setColour(juce::Colour(0xFF333333));
  g.fillRect(-15.0f, cy - 8.0f, 30.0f, 16.0f);
  g.setColour(juce::Colour(0xFF888888));
  g.drawRect(-15.0f, cy - 8.0f, 30.0f, 16.0f, 1.0f);

  // Toggle lever
  g.setColour(juce::Colour(0xFFaaaaaa));
  if (closed) {
    g.drawLine(0, cy, 10, cy, 4.0f); // Toggled right
    g.setColour(juce::Colour(0xFF00ff00).withAlpha(0.6f));
    g.fillEllipse(5, cy - 3, 6, 6); // Green led
  } else {
    g.drawLine(0, cy, -10, cy - 5, 4.0f); // Toggled left/up
    g.setColour(juce::Colour(0xFFff4444).withAlpha(0.6f));
    g.fillEllipse(-11, cy - 3, 6, 6); // Red led
  }

  // Leads
  g.setColour(juce::Colour(0xFF888888));
  g.drawLine(-WIDTH / 2, cy, -15, cy, 1.5f);
  g.drawLine(15, cy, WIDTH / 2, cy, 1.5f);
}

void ComponentView::drawVacuumTube(juce::Graphics &g,
                                   juce::Rectangle<float> bounds) {
  float cx = bounds.getCentreX();
  float cy = bounds.getCentreY();
  float radius = 18;

  // Heater glow (Orange radial gradient)
  juce::ColourGradient heater(juce::Colour(0x66ff8800), cx, cy + 8,
                              juce::Colour(0x00ff8800), cx, cy + radius, true);
  g.setGradientFill(heater);
  g.fillEllipse(cx - 10, cy, 20, 15);

  // Envelope (Glass)
  g.setColour(juce::Colours::white.withAlpha(0.3f));
  g.drawEllipse(cx - radius, cy - radius, radius * 2, radius * 2, 1.5f);

  // Shine on glass
  g.setColour(juce::Colours::white.withAlpha(0.1f));
  g.fillEllipse(cx - radius * 0.7f, cy - radius * 0.7f, radius * 0.4f,
                radius * 0.2f);

  // Internal components
  g.setColour(juce::Colour(0xFF888888));

  // Plate (top)
  g.drawLine(cx - 10, cy - 8, cx + 10, cy - 8, 2.0f);
  g.drawLine(cx, cy - radius - 5, cx, cy - 8, 1.5f);

  // Grid (middle)
  for (int i = -8; i <= 8; i += 4) {
    g.drawLine(static_cast<float>(cx + i - 1), cy,
               static_cast<float>(cx + i + 1), cy, 1.0f);
  }
  g.drawLine(cx - radius - 5, cy, cx - 10, cy, 1.5f);

  // Cathode (bottom)
  g.drawLine(cx - 6, cy + 8, cx + 6, cy + 8, 2.0f);
  g.drawLine(cx, cy + radius + 5, cx, cy + 8, 1.5f);
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

void ComponentView::drawGround(juce::Graphics &g,
                               juce::Rectangle<float> bounds) {
  g.setColour(juce::Colour(0xFFcccccc));

  float cx = bounds.getCentreX();
  float cy = bounds.getCentreY();
  float w = bounds.getWidth() * 0.8f;
  float h = bounds.getHeight() * 0.6f;

  // Vertical line from top
  g.drawLine(cx, -HEIGHT / 2, cx, cy + h / 6, 2.0f);

  // Ground symbol lines
  g.drawLine(cx - w / 4, cy + h / 6, cx + w / 4, cy + h / 6, 2.0f);
  g.drawLine(cx - w / 6, cy + h / 6 + 4, cx + w / 6, cy + h / 6 + 4, 2.0f);
  g.drawLine(cx - w / 10, cy + h / 6 + 8, cx + w / 10, cy + h / 6 + 8, 2.0f);
}

juce::Rectangle<float> ComponentView::getBoundsInCanvas() const {
  if (!component)
    return juce::Rectangle<float>();

  int rot = component->getRotation();
  if (rot == 90 || rot == 270) {
    return juce::Rectangle<float>(canvasPosition.x - HEIGHT / 2,
                                  canvasPosition.y - WIDTH / 2, HEIGHT, WIDTH);
  }

  return juce::Rectangle<float>(canvasPosition.x - WIDTH / 2,
                                canvasPosition.y - HEIGHT / 2, WIDTH, HEIGHT);
}

std::vector<std::pair<int, juce::Point<float>>>
ComponentView::getTerminalPositions() const {
  std::vector<std::pair<int, juce::Point<float>>> terminals;

  if (!component)
    return terminals;

  auto getRotatedPoint = [this](juce::Point<float> localPos) {
    float rotRad =
        juce::degreesToRadians(static_cast<float>(component->getRotation()));
    auto transform = juce::AffineTransform::rotation(rotRad);
    return canvasPosition + localPos.transformedBy(transform);
  };

  if (component->getType() == ComponentType::Ground) {
    // Single terminal at top
    terminals.push_back(
        {component->getNode1(), getRotatedPoint({0, -HEIGHT / 2})});
    return terminals;
  }

  // Standard two-terminal components
  if (component->getType() != ComponentType::VacuumTube &&
      component->getType() != ComponentType::Potentiometer &&
      component->getType() != ComponentType::AudioInput &&
      component->getType() != ComponentType::AudioOutput) {
    terminals.push_back(
        {component->getNode1(), getRotatedPoint({-WIDTH / 2, 0})});
    terminals.push_back(
        {component->getNode2(), getRotatedPoint({WIDTH / 2, 0})});
  }

  // Audio I/O
  if (component->getType() == ComponentType::AudioInput) {
    // In: Node 1 (Signal) at Right. Node 2 (Ground) hidden/unused for wiring
    // here
    terminals.push_back(
        {component->getNode1(), getRotatedPoint({WIDTH / 2, 0})});
  } else if (component->getType() == ComponentType::AudioOutput) {
    // Out: Node 1 (Signal) at Left. Node 2 (Ground) hidden/unused
    terminals.push_back(
        {component->getNode1(), getRotatedPoint({-WIDTH / 2, 0})});
  }

  // Three-terminal components
  if (component->getType() == ComponentType::Potentiometer) {
    auto *pot = dynamic_cast<Potentiometer *>(component);
    if (pot) {
      terminals.push_back({pot->getNode1(), getRotatedPoint({-WIDTH / 2, 0})});
      terminals.push_back({pot->getNode2(), getRotatedPoint({WIDTH / 2, 0})});
      terminals.push_back({pot->getNode3(), getRotatedPoint({0, -HEIGHT / 2})});
    }
  } else if (component->getType() == ComponentType::VacuumTube) {
    auto *tube = dynamic_cast<VacuumTube *>(component);
    if (tube) {
      // Grid at left, cathode at bottom, plate at top
      terminals.push_back(
          {tube->getNode1(), getRotatedPoint({-WIDTH / 2 - 5, 0})}); // Grid
      terminals.push_back(
          {tube->getNode2(), getRotatedPoint({0, HEIGHT / 2 + 5})}); // Cathode
      terminals.push_back({tube->getPlateNode(),
                           getRotatedPoint({0, -HEIGHT / 2 - 5})}); // Plate
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
