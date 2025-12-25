#include "ComponentPalette.h"

//==============================================================================
// PaletteItem Implementation
//==============================================================================

ComponentPalette::PaletteItem::PaletteItem(const juce::String &name,
                                           const juce::String &dragId,
                                           ComponentType type)
    : itemName(name), dragIdentifier(dragId), componentType(type) {}

void ComponentPalette::PaletteItem::paint(juce::Graphics &g) {
  auto bounds = getLocalBounds().toFloat().reduced(4);

  // Background
  juce::Colour bgColour =
      isBeingDragged ? juce::Colour(0xFF3a4a5e) : juce::Colour(0xFF2a3a4e);
  if (isMouseOver())
    bgColour = bgColour.brighter(0.1f);

  g.setColour(bgColour);
  g.fillRoundedRectangle(bounds, 6.0f);

  // Border
  g.setColour(juce::Colour(0xFF4a5a6e));
  g.drawRoundedRectangle(bounds, 6.0f, 1.0f);

  // Symbol
  auto symbolBounds =
      bounds.reduced(8).removeFromTop(bounds.getHeight() * 0.6f);
  drawSymbol(g, symbolBounds);

  // Name
  g.setColour(juce::Colour(0xFFb0c0d0));
  g.setFont(10.0f);
  g.drawText(itemName, bounds.removeFromBottom(16),
             juce::Justification::centred);
}

void ComponentPalette::PaletteItem::drawSymbol(juce::Graphics &g,
                                               juce::Rectangle<float> bounds) {
  g.setColour(juce::Colour(0xFF00ccff));

  float cx = bounds.getCentreX();
  float cy = bounds.getCentreY();
  float w = bounds.getWidth() * 0.8f;
  float h = bounds.getHeight() * 0.6f;

  switch (componentType) {
  case ComponentType::Resistor: {
    // Zigzag resistor symbol
    juce::Path path;
    path.startNewSubPath(cx - w / 2, cy);
    float zigW = w / 6;
    float zigH = h / 2;
    path.lineTo(cx - w / 2 + zigW * 0.5f, cy - zigH);
    path.lineTo(cx - w / 2 + zigW * 1.5f, cy + zigH);
    path.lineTo(cx - w / 2 + zigW * 2.5f, cy - zigH);
    path.lineTo(cx - w / 2 + zigW * 3.5f, cy + zigH);
    path.lineTo(cx - w / 2 + zigW * 4.5f, cy - zigH);
    path.lineTo(cx - w / 2 + zigW * 5.5f, cy);
    path.lineTo(cx + w / 2, cy);
    g.strokePath(path, juce::PathStrokeType(2.0f));
    break;
  }
  case ComponentType::Capacitor: {
    // Parallel plate capacitor symbol
    float plateGap = 6.0f;
    g.drawLine(cx - w / 2, cy, cx - plateGap / 2, cy, 2.0f);
    g.drawLine(cx + plateGap / 2, cy, cx + w / 2, cy, 2.0f);
    g.drawLine(cx - plateGap / 2, cy - h / 2, cx - plateGap / 2, cy + h / 2,
               2.0f);
    g.drawLine(cx + plateGap / 2, cy - h / 2, cx + plateGap / 2, cy + h / 2,
               2.0f);
    break;
  }
  case ComponentType::Inductor: {
    // Coil inductor symbol
    juce::Path path;
    path.startNewSubPath(cx - w / 2, cy);
    float coilW = w / 5;
    for (int i = 0; i < 4; i++) {
      float startX = cx - w / 2 + coilW * 0.5f + i * coilW;
      path.addCentredArc(startX, cy, coilW / 2, h / 3, 0, 
                         juce::MathConstants<float>::pi, 0, false);
    }
    path.lineTo(cx + w / 2, cy);
    g.strokePath(path, juce::PathStrokeType(2.0f));
    break;
  }
  case ComponentType::Potentiometer: {
    // Resistor with arrow
    juce::Path path;
    path.startNewSubPath(cx - w / 2, cy);
    float zigW = w / 5;
    float zigH = h / 3;
    path.lineTo(cx - w / 2 + zigW * 0.5f, cy - zigH);
    path.lineTo(cx - w / 2 + zigW * 1.5f, cy + zigH);
    path.lineTo(cx - w / 2 + zigW * 2.5f, cy - zigH);
    path.lineTo(cx - w / 2 + zigW * 3.5f, cy + zigH);
    path.lineTo(cx - w / 2 + zigW * 4.5f, cy);
    path.lineTo(cx + w / 2, cy);
    g.strokePath(path, juce::PathStrokeType(2.0f));

    // Arrow
    g.drawArrow(juce::Line<float>(cx, cy + h * 0.7f, cx, cy + zigH * 0.5f),
                2.0f, 8.0f, 6.0f);
    break;
  }
  case ComponentType::Switch: {
    // SPST switch symbol
    g.drawLine(cx - w / 2, cy, cx - w / 6, cy, 2.0f);
    g.drawLine(cx + w / 6, cy, cx + w / 2, cy, 2.0f);
    g.fillEllipse(cx - w / 6 - 3, cy - 3, 6, 6);
    g.fillEllipse(cx + w / 6 - 3, cy - 3, 6, 6);
    g.drawLine(cx - w / 6, cy, cx + w / 8, cy - h / 2, 2.0f);
    break;
  }
  case ComponentType::Diode: {
    // Diode symbol (triangle with bar)
    g.drawLine(cx - w / 2, cy, cx - w / 6, cy, 2.0f);
    g.drawLine(cx + w / 6, cy, cx + w / 2, cy, 2.0f);
    // Triangle pointing right
    juce::Path triangle;
    triangle.startNewSubPath(cx - w / 6, cy - h / 2);
    triangle.lineTo(cx + w / 6, cy);
    triangle.lineTo(cx - w / 6, cy + h / 2);
    triangle.closeSubPath();
    g.strokePath(triangle, juce::PathStrokeType(2.0f));
    // Bar at cathode
    g.drawLine(cx + w / 6, cy - h / 2, cx + w / 6, cy + h / 2, 2.0f);
    break;
  }
  case ComponentType::DiodePair: {
    // Anti-parallel diode pair symbol
    float triH = h / 3;
    // Top diode (pointing right)
    juce::Path tri1;
    tri1.startNewSubPath(cx - w / 6, cy - triH - triH / 2);
    tri1.lineTo(cx + w / 6, cy - triH);
    tri1.lineTo(cx - w / 6, cy - triH + triH / 2);
    tri1.closeSubPath();
    g.strokePath(tri1, juce::PathStrokeType(1.5f));
    g.drawLine(cx + w / 6, cy - triH - triH / 2, cx + w / 6, cy - triH + triH / 2, 1.5f);
    // Bottom diode (pointing left)
    juce::Path tri2;
    tri2.startNewSubPath(cx + w / 6, cy + triH - triH / 2);
    tri2.lineTo(cx - w / 6, cy + triH);
    tri2.lineTo(cx + w / 6, cy + triH + triH / 2);
    tri2.closeSubPath();
    g.strokePath(tri2, juce::PathStrokeType(1.5f));
    g.drawLine(cx - w / 6, cy + triH - triH / 2, cx - w / 6, cy + triH + triH / 2, 1.5f);
    // Connection lines
    g.drawLine(cx - w / 2, cy, cx - w / 6, cy, 2.0f);
    g.drawLine(cx + w / 6, cy, cx + w / 2, cy, 2.0f);
    g.drawLine(cx - w / 6, cy - triH, cx - w / 6, cy + triH, 1.0f);
    g.drawLine(cx + w / 6, cy - triH, cx + w / 6, cy + triH, 1.0f);
    break;
  }
  case ComponentType::SoftClipper: {
    // Soft clipper symbol (tanh curve in a box)
    float boxW = w * 0.6f;
    float boxH = h * 0.8f;
    g.drawRect(cx - boxW / 2, cy - boxH / 2, boxW, boxH, 1.5f);
    // Tanh-like curve inside
    juce::Path curve;
    curve.startNewSubPath(cx - boxW / 2 + 3, cy + boxH / 3);
    curve.cubicTo(cx - boxW / 6, cy + boxH / 4,
                  cx - boxW / 8, cy,
                  cx, cy);
    curve.cubicTo(cx + boxW / 8, cy,
                  cx + boxW / 6, cy - boxH / 4,
                  cx + boxW / 2 - 3, cy - boxH / 3);
    g.strokePath(curve, juce::PathStrokeType(2.0f));
    // Connection lines
    g.drawLine(cx - w / 2, cy, cx - boxW / 2, cy, 2.0f);
    g.drawLine(cx + boxW / 2, cy, cx + w / 2, cy, 2.0f);
    break;
  }
  case ComponentType::VacuumTube: {
    // Triode symbol (circle with plate, grid, cathode)
    float radius = std::min(w, h) / 2 - 4;
    g.drawEllipse(cx - radius, cy - radius, radius * 2, radius * 2, 2.0f);

    // Plate (top)
    g.drawLine(cx - radius * 0.5f, cy - radius * 0.4f, cx + radius * 0.5f,
               cy - radius * 0.4f, 2.0f);
    g.drawLine(cx, cy - radius, cx, cy - radius * 0.4f, 1.5f);

    // Grid (middle, dashed)
    g.drawLine(cx - radius * 0.4f, cy, cx + radius * 0.4f, cy, 1.5f);
    g.drawLine(cx - radius, cy, cx - radius * 0.6f, cy, 1.5f);

    // Cathode (bottom)
    g.drawLine(cx - radius * 0.3f, cy + radius * 0.4f, cx + radius * 0.3f,
               cy + radius * 0.4f, 2.0f);
    g.drawLine(cx, cy + radius, cx, cy + radius * 0.4f, 1.5f);
    break;
  }
  case ComponentType::AudioInput: {
    // Speaker/input symbol
    g.drawRect(cx - w / 4, cy - h / 3, w / 4, h * 2 / 3, 2.0f);
    juce::Path cone;
    cone.startNewSubPath(cx, cy - h / 3);
    cone.lineTo(cx + w / 3, cy - h / 2);
    cone.lineTo(cx + w / 3, cy + h / 2);
    cone.lineTo(cx, cy + h / 3);
    cone.closeSubPath();
    g.strokePath(cone, juce::PathStrokeType(2.0f));

    // Arrow pointing in
    g.drawArrow(juce::Line<float>(cx - w / 2, cy, cx - w / 4, cy), 2.0f, 8.0f,
                6.0f);
    break;
  }
  case ComponentType::AudioOutput: {
    // Speaker/output symbol
    g.drawRect(cx - w / 4, cy - h / 3, w / 4, h * 2 / 3, 2.0f);
    juce::Path cone;
    cone.startNewSubPath(cx, cy - h / 3);
    cone.lineTo(cx + w / 3, cy - h / 2);
    cone.lineTo(cx + w / 3, cy + h / 2);
    cone.lineTo(cx, cy + h / 3);
    cone.closeSubPath();
    g.strokePath(cone, juce::PathStrokeType(2.0f));

    // Sound waves
    for (int i = 1; i <= 2; i++) {
      float arcRadius = w / 4 + i * 6;
      juce::Path arc;
      arc.addCentredArc(cx + w / 3, cy, arcRadius, arcRadius, 0, -0.5f, 0.5f,
                        true);
      g.strokePath(arc, juce::PathStrokeType(1.5f));
    }
    break;
  }
  case ComponentType::Ground: {
    // Ground symbol
    g.drawLine(cx, cy - h / 3, cx, cy + h / 6, 2.0f);
    g.drawLine(cx - w / 4, cy + h / 6, cx + w / 4, cy + h / 6, 2.0f);
    g.drawLine(cx - w / 6, cy + h / 6 + 4, cx + w / 6, cy + h / 6 + 4, 2.0f);
    g.drawLine(cx - w / 10, cy + h / 6 + 8, cx + w / 10, cy + h / 6 + 8, 2.0f);
    break;
  }
  default:
    g.drawRect(bounds, 2.0f);
    break;
  }
}

void ComponentPalette::PaletteItem::mouseDown(const juce::MouseEvent &e) {
  juce::ignoreUnused(e);
  isBeingDragged = true;
  repaint();
}

void ComponentPalette::PaletteItem::mouseDrag(const juce::MouseEvent &e) {
  if (e.getDistanceFromDragStart() > 5) {
    // Start drag and drop
    auto *container =
        juce::DragAndDropContainer::findParentDragContainerFor(this);
    if (container) {
      container->startDragging(dragIdentifier, this);
    }
  }
}

void ComponentPalette::PaletteItem::mouseUp(const juce::MouseEvent &e) {
  juce::ignoreUnused(e);
  isBeingDragged = false;
  repaint();
}

//==============================================================================
// ComponentPalette Implementation
//==============================================================================

ComponentPalette::ComponentPalette() { createItems(); }

ComponentPalette::~ComponentPalette() = default;

void ComponentPalette::createItems() {
  items.clear();

  struct ItemInfo {
    juce::String name;
    juce::String dragId;
    ComponentType type;
  };

  std::vector<ItemInfo> itemInfos = {
      {"Resistor", "component:resistor", ComponentType::Resistor},
      {"Capacitor", "component:capacitor", ComponentType::Capacitor},
      {"Inductor", "component:inductor", ComponentType::Inductor},
      {"Pot", "component:potentiometer", ComponentType::Potentiometer},
      {"Switch", "component:switch", ComponentType::Switch},
      {"Diode", "component:diode", ComponentType::Diode},
      {"Diode Pair", "component:diodepair", ComponentType::DiodePair},
      {"Clipper", "component:softclipper", ComponentType::SoftClipper},
      {"Tube", "component:tube", ComponentType::VacuumTube},
      {"Input", "component:input", ComponentType::AudioInput},
      {"Output", "component:output", ComponentType::AudioOutput},
      {"Ground", "component:ground", ComponentType::Ground}};

  for (const auto &info : itemInfos) {
    auto item =
        std::make_unique<PaletteItem>(info.name, info.dragId, info.type);
    addAndMakeVisible(*item);
    items.push_back(std::move(item));
  }
}

void ComponentPalette::paint(juce::Graphics &g) {
  // Dark sidebar background
  g.fillAll(juce::Colour(0xFF1a2332));

  // Title
  g.setColour(juce::Colour(0xFF8899aa));
  g.setFont(12.0f);
  g.drawText("Components", getLocalBounds().removeFromTop(30),
             juce::Justification::centred);
}

void ComponentPalette::resized() {
  auto bounds = getLocalBounds();
  bounds.removeFromTop(35); // Title space

  int itemHeight = 70;
  int padding = 5;

  for (auto &item : items) {
    item->setBounds(bounds.removeFromTop(itemHeight).reduced(padding));
  }
}
