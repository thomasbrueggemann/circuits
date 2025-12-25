#include "CircuitDesigner.h"
#include "../Engine/Components/AudioInput.h"
#include "../Engine/Components/AudioOutput.h"
#include "../Engine/Components/Capacitor.h"
#include "../Engine/Components/Diode.h"
#include "../Engine/Components/DiodePair.h"
#include "../Engine/Components/Ground.h"
#include "../Engine/Components/Inductor.h"
#include "../Engine/Components/Potentiometer.h"
#include "../Engine/Components/Resistor.h"
#include "../Engine/Components/SoftClipper.h"
#include "../Engine/Components/Switch.h"
#include "../Engine/Components/VacuumTube.h"
#include "ComponentPalette.h"
#include "ComponentView.h"
#include "WireView.h"

CircuitDesigner::CircuitDesigner(CircuitGraph &graph, ComponentPalette &pal)
    : circuitGraph(graph) {
  juce::ignoreUnused(pal);
  setWantsKeyboardFocus(true);
  rebuildViews();
}

CircuitDesigner::~CircuitDesigner() = default;

void CircuitDesigner::paint(juce::Graphics &g) {
  // Background
  g.fillAll(juce::Colour(0xFF1e2530));

  drawGrid(g);
  drawWires(g);
  drawJunctions(g);
  drawComponents(g);
  drawConnectionPoints(g);

  if (isDrawingWire)
    drawWirePreview(g);

  if (showDragPreview)
    drawDragPreview(g);
}

void CircuitDesigner::resized() {}

void CircuitDesigner::drawGrid(juce::Graphics &g) {
  g.setColour(juce::Colour(0xFF2a3242));

  float gridSpacing = GRID_SIZE * zoomLevel;

  // Calculate visible area in canvas coordinates
  auto bounds = getLocalBounds().toFloat();

  float startX = std::fmod(-viewOffset.x * zoomLevel, gridSpacing);
  float startY = std::fmod(-viewOffset.y * zoomLevel, gridSpacing);

  // Vertical lines
  for (float x = startX; x < bounds.getWidth(); x += gridSpacing) {
    g.drawVerticalLine(static_cast<int>(x), 0.0f, bounds.getHeight());
  }

  // Horizontal lines
  for (float y = startY; y < bounds.getHeight(); y += gridSpacing) {
    g.drawHorizontalLine(static_cast<int>(y), 0.0f, bounds.getWidth());
  }
}

void CircuitDesigner::drawComponents(juce::Graphics &g) {
  for (auto &view : componentViews) {
    auto screenPos = canvasToScreen(view->getCanvasPosition());

    juce::AffineTransform transform =
        juce::AffineTransform::translation(screenPos.x, screenPos.y)
            .scaled(zoomLevel);

    view->draw(g, transform, zoomLevel);
  }
}

void CircuitDesigner::drawWires(juce::Graphics &g) {
  for (auto &wire : wireViews) {
    auto startPos = canvasToScreen(wire->getStartPosition());
    auto endPos = canvasToScreen(wire->getEndPosition());

    bool isSelected = (wire.get() == selectedWire);
    wire->draw(g, startPos, endPos, isSelected);
  }
}

void CircuitDesigner::drawWirePreview(juce::Graphics &g) {
  if (wireStartNode < 0)
    return;

  // Use stored start position (canvas coordinates -> screen)
  auto startPos = canvasToScreen(wireStartPosition);
  auto endPos = canvasToScreen(wireEndPoint);

  g.setColour(juce::Colour(0xFF00ff88).withAlpha(0.7f));

  juce::Path wirePath;
  wirePath.startNewSubPath(startPos);

  // Simple orthogonal routing
  float midX = (startPos.x + endPos.x) / 2.0f;
  wirePath.lineTo(midX, startPos.y);
  wirePath.lineTo(midX, endPos.y);
  wirePath.lineTo(endPos);

  g.strokePath(wirePath, juce::PathStrokeType(2.0f * zoomLevel));
}

// ... (skip lines)

void CircuitDesigner::startWire(int startNode, juce::Point<float> startPos) {
  isDrawingWire = true;
  wireStartNode = startNode;

  // Snap start position to the node center for accurate drawing
  // We can find the node again or just use the passed pos if we trust it.
  // Ideally, find the node center:
  juce::Point<float> nodeCenter = startPos;

  for (auto &view : componentViews) {
    for (auto &[nodeId, pos] : view->getTerminalPositions()) {
      if (nodeId == startNode) {
        // If the passed startPos is close to this node, snap to it
        // This handles the shared node case by snapping to the one we clicked
        // CLOSEST to But since 'startPos' comes from 'mouseDown', it's just the
        // mouse click. We want the exact terminal position.
        if (pos.getDistanceFrom(startPos) < 20.0f) {
          nodeCenter = pos;
          goto found;
        }
      }
    }
  }
found:
  wireStartPosition = nodeCenter;
  wireEndPoint = nodeCenter;
  repaint();
}

void CircuitDesigner::drawDragPreview(juce::Graphics &g) {
  auto screenPos = canvasToScreen(dragPreviewPosition);

  g.setColour(juce::Colour(0xFF00aaff).withAlpha(0.5f));
  g.fillRoundedRectangle(screenPos.x - 30 * zoomLevel,
                         screenPos.y - 20 * zoomLevel, 60 * zoomLevel,
                         40 * zoomLevel, 5.0f);
}

void CircuitDesigner::drawConnectionPoints(juce::Graphics &g) {
  auto mousePos = getMouseXYRelative().toFloat();
  float snapDist = 15.0f; // Screen pixels

  // Draw connection points (nodes) for each component
  for (auto &view : componentViews) {
    auto terminals = view->getTerminalPositions();

    for (auto &[nodeId, pos] : terminals) {
      auto screenPos = canvasToScreen(pos);

      // Check if hovered (use consistent pixel distance)
      float distance = mousePos.getDistanceFrom(screenPos);

      if (distance < snapDist) {
        g.setColour(juce::Colour(0xFF00ff88));
        g.fillEllipse(screenPos.x - 8 * zoomLevel, screenPos.y - 8 * zoomLevel,
                      16 * zoomLevel, 16 * zoomLevel);
      } else {
        g.setColour(juce::Colour(0xFF4a5568));
        g.fillEllipse(screenPos.x - 5 * zoomLevel, screenPos.y - 5 * zoomLevel,
                      10 * zoomLevel, 10 * zoomLevel);
      }
    }
  }

  // When drawing a wire, show potential junction point on existing wires
  if (isDrawingWire) {
    auto canvasPos = screenToCanvas(mousePos);
    auto [targetWire, junctionPos] = findWireJunctionAt(canvasPos);
    if (targetWire) {
      auto screenJunctionPos = canvasToScreen(junctionPos);
      // Draw junction preview
      g.setColour(juce::Colour(0xFF00ff88).withAlpha(0.8f));
      g.fillEllipse(screenJunctionPos.x - 8 * zoomLevel,
                    screenJunctionPos.y - 8 * zoomLevel, 16 * zoomLevel,
                    16 * zoomLevel);
      g.setColour(juce::Colours::white);
      g.drawEllipse(screenJunctionPos.x - 8 * zoomLevel,
                    screenJunctionPos.y - 8 * zoomLevel, 16 * zoomLevel,
                    16 * zoomLevel, 2.0f);
    }
  }
}

void CircuitDesigner::mouseDown(const juce::MouseEvent &e) {
  auto canvasPos = screenToCanvas(e.position);

  if (e.mods.isMiddleButtonDown() ||
      (e.mods.isLeftButtonDown() && e.mods.isAltDown())) {
    // Start panning
    isPanning = true;
    panStartOffset = viewOffset;
    panStartMouse = e.getPosition();
    return;
  }

  if (e.mods.isRightButtonDown()) {
    // Context menu or cancel wire
    if (isDrawingWire) {
      cancelWire();
    } else {
      // Show context menu
      juce::PopupMenu m;
      auto *clickedComp = findComponentAt(canvasPos);

      if (clickedComp) {
        // Deselect previous and select this one
        if (selectedComponent && selectedComponent != clickedComp)
          selectedComponent->setSelected(false);

        selectedComponent = clickedComp;
        selectedComponent->setSelected(true);
        selectedWire = nullptr;
        repaint();

        m.addItem(1, "Rotate (R)");
        m.addItem(2, "Edit Value...");
        m.addSeparator();
        m.addItem(3, "Delete");

        m.showMenuAsync(juce::PopupMenu::Options(), [this](int result) {
          if (result == 1)
            rotateSelectedComponent();
          else if (result == 2 && selectedComponent)
            selectedComponent->showValueEditor();
          else if (result == 3)
            removeSelectedComponent();
        });
      } else {
        auto *clickedWire = findWireAt(canvasPos);
        if (clickedWire) {
          selectedWire = clickedWire;
          selectedComponent = nullptr;
          repaint();

          m.addItem(1, "Delete Wire");
          m.showMenuAsync(juce::PopupMenu::Options(), [this](int result) {
            if (result == 1)
              removeSelectedWire();
          });
        }
      }
    }
    return;
  }

  // Prioritize finishing an active wire if we click a node
  int clickedNode = findNodeAt(canvasPos);
  if (isDrawingWire && clickedNode >= 0) {
    if (clickedNode != wireStartNode) {
      finishWire(clickedNode);
    }
    return;
  }

  // Check if we're finishing a wire on another wire (creating a junction)
  if (isDrawingWire) {
    auto [targetWire, junctionPos] = findWireJunctionAt(canvasPos);
    if (targetWire) {
      finishWireOnWire(targetWire, junctionPos);
      return;
    }
  }

  // Always start a wire if connected to a node, even if inside a component
  if (clickedNode >= 0) {
    startWire(clickedNode, canvasPos);
    grabKeyboardFocus();
    return;
  }

  // Check if we can start a wire from an existing junction
  for (const auto &junction : circuitGraph.getJunctions()) {
    auto screenJunctionPos = canvasToScreen(junction.position);
    auto screenMousePos = canvasToScreen(canvasPos);
    if (screenJunctionPos.getDistanceFrom(screenMousePos) < 15.0f) {
      startWire(junction.nodeId, junction.position);
      grabKeyboardFocus();
      return;
    }
  }

  // Check if clicking on a component body first (for selection/dragging)
  ComponentView *clickedComponent = findComponentAt(canvasPos);

  // Clicked on empty space or other element - cancel wire if drawing
  if (isDrawingWire) {
    // If clicking on empty space, we just update the end point, don't cancel
    // This allows click-move-click worklow
  }

  // Check if clicking on a wire
  WireView *clickedWire = findWireAt(canvasPos);
  if (clickedWire) {
    selectedWire = clickedWire;
    selectedComponent = nullptr;

    if (onWireSelected)
      onWireSelected(clickedWire->getNodeA());

    repaint();
    return;
  }

  // Check if clicking on a component
  clickedComponent = findComponentAt(canvasPos);
  if (clickedComponent) {
    // Deselect previous
    if (selectedComponent)
      selectedComponent->setSelected(false);

    selectedComponent = clickedComponent;
    selectedComponent->setSelected(true);
    selectedWire = nullptr;

    grabKeyboardFocus();
    repaint();
    return;
  }

  // Clicked on empty space - deselect
  if (selectedComponent) {
    selectedComponent->setSelected(false);
    selectedComponent = nullptr;
  }
  selectedWire = nullptr;

  // If we clicked empty space while drawing wire, cancel it
  if (isDrawingWire && clickedNode < 0) {
    cancelWire();
  }

  if (onWireSelected)
    onWireSelected(-1);

  repaint();
}

void CircuitDesigner::mouseDrag(const juce::MouseEvent &e) {
  if (isPanning) {
    auto delta = e.getPosition() - panStartMouse;
    viewOffset = panStartOffset -
                 juce::Point<float>(delta.x / zoomLevel, delta.y / zoomLevel);
    repaint();
    return;
  }

  if (isDrawingWire) {
    auto canvasPos = screenToCanvas(e.position);
    int snappedNode = findNodeAt(canvasPos);

    if (snappedNode >= 0 && snappedNode != wireStartNode) {
      // Find the node position to snap the preview to
      for (const auto &view : componentViews) {
        for (const auto &term : view->getTerminalPositions()) {
          if (term.first == snappedNode) {
            wireEndPoint = term.second;
            repaint();
            return;
          }
        }
      }
    }

    // Check for snapping to existing junctions
    for (const auto &junction : circuitGraph.getJunctions()) {
      auto screenJunctionPos = canvasToScreen(junction.position);
      if (e.position.getDistanceFrom(screenJunctionPos) < 15.0f) {
        wireEndPoint = junction.position;
        repaint();
        return;
      }
    }

    // Check for snapping to wire (potential junction point)
    auto [targetWire, junctionPos] = findWireJunctionAt(canvasPos);
    if (targetWire) {
      wireEndPoint = junctionPos;
      repaint();
      return;
    }

    wireEndPoint = canvasPos;
    repaint();
    return;
  }

  if (selectedComponent && e.mods.isLeftButtonDown()) {
    // Drag component
    auto canvasPos = screenToCanvas(e.position);
    auto snappedPos = snapToGrid(canvasPos);
    selectedComponent->setCanvasPosition(snappedPos);

    // Update the actual component in the graph
    if (auto *comp = selectedComponent->getComponent()) {
      comp->setPosition(snappedPos);
    }

    updateWirePositionsForComponent(selectedComponent);

    repaint();
  }
}

void CircuitDesigner::mouseUp(const juce::MouseEvent &e) {
  isPanning = false;

  if (isDrawingWire) {
    auto canvasPos = screenToCanvas(e.position);
    int endNode = findNodeAt(canvasPos);

    if (endNode >= 0 && endNode != wireStartNode) {
      finishWire(endNode);
    } else {
      // Check if we can connect to an existing junction
      for (const auto &junction : circuitGraph.getJunctions()) {
        auto screenJunctionPos = canvasToScreen(junction.position);
        if (e.position.getDistanceFrom(screenJunctionPos) < 15.0f) {
          if (junction.nodeId != wireStartNode) {
            finishWire(junction.nodeId);
          }
          return;
        }
      }

      // Check if releasing on a wire (create junction)
      auto [targetWire, junctionPos] = findWireJunctionAt(canvasPos);
      if (targetWire) {
        finishWireOnWire(targetWire, junctionPos);
        return;
      }

      // Do nothing if we release on the start node or empty space
      // This allows the "click-move-click" workflow
      // We only cancel if explicitly clicking empty space in mouseDown or Right
      // Click
    }
  }
}

void CircuitDesigner::mouseMove(const juce::MouseEvent &e) {
  if (isDrawingWire) {
    auto canvasPos = screenToCanvas(e.position);
    int snappedNode = findNodeAt(canvasPos);

    if (snappedNode >= 0 && snappedNode != wireStartNode) {
      // Find the node position to snap the preview to
      for (const auto &view : componentViews) {
        for (const auto &term : view->getTerminalPositions()) {
          if (term.first == snappedNode) {
            wireEndPoint = term.second;
            repaint();
            return;
          }
        }
      }
    }

    // Check for snapping to existing junctions
    for (const auto &junction : circuitGraph.getJunctions()) {
      auto screenJunctionPos = canvasToScreen(junction.position);
      if (e.position.getDistanceFrom(screenJunctionPos) < 15.0f) {
        wireEndPoint = junction.position;
        repaint();
        return;
      }
    }

    // Check for snapping to wire (potential junction point)
    auto [targetWire, junctionPos] = findWireJunctionAt(canvasPos);
    if (targetWire) {
      wireEndPoint = junctionPos;
      repaint();
      return;
    }

    wireEndPoint = canvasPos;
    repaint();
  }

  // Always repaint to update connection point highlights
  repaint();
}

void CircuitDesigner::mouseDoubleClick(const juce::MouseEvent &e) {
  auto canvasPos = screenToCanvas(e.position);
  ComponentView *clickedComponent = findComponentAt(canvasPos);

  if (clickedComponent) {
    // Open value editor
    clickedComponent->showValueEditor();
  }
}

void CircuitDesigner::mouseWheelMove(const juce::MouseEvent &e,
                                     const juce::MouseWheelDetails &wheel) {
  // Zoom centered on mouse position
  auto mouseCanvasPos = screenToCanvas(e.position);

  float zoomDelta = wheel.deltaY * 0.1f;
  float newZoom = juce::jlimit(0.25f, 4.0f, zoomLevel + zoomDelta);

  if (std::abs(newZoom - zoomLevel) > 1e-4f) {
    // Adjust offset to zoom towards mouse
    float zoomRatio = newZoom / zoomLevel;
    viewOffset = mouseCanvasPos - (mouseCanvasPos - viewOffset) / zoomRatio;

    zoomLevel = newZoom;
    repaint();
  }
}

bool CircuitDesigner::isInterestedInDragSource(
    const SourceDetails &dragSourceDetails) {
  return dragSourceDetails.description.toString().startsWith("component:");
}

void CircuitDesigner::itemDragEnter(const SourceDetails &dragSourceDetails) {
  juce::ignoreUnused(dragSourceDetails);
  cancelWire();
  showDragPreview = true;
  repaint();
}

void CircuitDesigner::itemDragMove(const SourceDetails &dragSourceDetails) {
  dragPreviewPosition =
      snapToGrid(screenToCanvas(dragSourceDetails.localPosition.toFloat()));

  // Parse component type
  auto desc = dragSourceDetails.description.toString();
  if (desc == "component:resistor")
    dragPreviewType = ComponentType::Resistor;
  else if (desc == "component:capacitor")
    dragPreviewType = ComponentType::Capacitor;
  else if (desc == "component:inductor")
    dragPreviewType = ComponentType::Inductor;
  else if (desc == "component:potentiometer")
    dragPreviewType = ComponentType::Potentiometer;
  else if (desc == "component:switch")
    dragPreviewType = ComponentType::Switch;
  else if (desc == "component:diode")
    dragPreviewType = ComponentType::Diode;
  else if (desc == "component:diodepair")
    dragPreviewType = ComponentType::DiodePair;
  else if (desc == "component:softclipper")
    dragPreviewType = ComponentType::SoftClipper;
  else if (desc == "component:tube")
    dragPreviewType = ComponentType::VacuumTube;
  else if (desc == "component:input")
    dragPreviewType = ComponentType::AudioInput;
  else if (desc == "component:output")
    dragPreviewType = ComponentType::AudioOutput;
  else if (desc == "component:ground")
    dragPreviewType = ComponentType::Ground;

  repaint();
}

void CircuitDesigner::itemDragExit(const SourceDetails &dragSourceDetails) {
  juce::ignoreUnused(dragSourceDetails);
  showDragPreview = false;
  repaint();
}

void CircuitDesigner::itemDropped(const SourceDetails &dragSourceDetails) {
  cancelWire();
  showDragPreview = false;

  auto canvasPos =
      snapToGrid(screenToCanvas(dragSourceDetails.localPosition.toFloat()));

  // Parse component type from description
  auto desc = dragSourceDetails.description.toString();
  ComponentType type = ComponentType::Resistor;

  if (desc == "component:resistor")
    type = ComponentType::Resistor;
  else if (desc == "component:capacitor")
    type = ComponentType::Capacitor;
  else if (desc == "component:inductor")
    type = ComponentType::Inductor;
  else if (desc == "component:potentiometer")
    type = ComponentType::Potentiometer;
  else if (desc == "component:switch")
    type = ComponentType::Switch;
  else if (desc == "component:diode")
    type = ComponentType::Diode;
  else if (desc == "component:diodepair")
    type = ComponentType::DiodePair;
  else if (desc == "component:softclipper")
    type = ComponentType::SoftClipper;
  else if (desc == "component:tube")
    type = ComponentType::VacuumTube;
  else if (desc == "component:input")
    type = ComponentType::AudioInput;
  else if (desc == "component:output")
    type = ComponentType::AudioOutput;
  else if (desc == "component:ground")
    type = ComponentType::Ground;

  addComponent(type, canvasPos);
  repaint();
}

juce::Point<float>
CircuitDesigner::screenToCanvas(juce::Point<float> screenPos) const {
  return juce::Point<float>(screenPos.x / zoomLevel + viewOffset.x,
                            screenPos.y / zoomLevel + viewOffset.y);
}

juce::Point<float>
CircuitDesigner::canvasToScreen(juce::Point<float> canvasPos) const {
  return juce::Point<float>((canvasPos.x - viewOffset.x) * zoomLevel,
                            (canvasPos.y - viewOffset.y) * zoomLevel);
}

juce::Point<float> CircuitDesigner::snapToGrid(juce::Point<float> pos) const {
  return juce::Point<float>(std::round(pos.x / GRID_SIZE) * GRID_SIZE,
                            std::round(pos.y / GRID_SIZE) * GRID_SIZE);
}

void CircuitDesigner::centerView() {
  viewOffset = juce::Point<float>(-getWidth() / (2.0f * zoomLevel),
                                  -getHeight() / (2.0f * zoomLevel));
  repaint();
}

void CircuitDesigner::zoomToFit() {
  if (componentViews.empty()) {
    centerView();
    return;
  }

  // Calculate bounding box of all components
  float minX = std::numeric_limits<float>::max();
  float minY = std::numeric_limits<float>::max();
  float maxX = std::numeric_limits<float>::lowest();
  float maxY = std::numeric_limits<float>::lowest();

  for (auto &view : componentViews) {
    auto pos = view->getCanvasPosition();
    minX = std::min(minX, pos.x - 50);
    minY = std::min(minY, pos.y - 50);
    maxX = std::max(maxX, pos.x + 50);
    maxY = std::max(maxY, pos.y + 50);
  }

  float contentWidth = maxX - minX;
  float contentHeight = maxY - minY;

  float zoomX = getWidth() / contentWidth;
  float zoomY = getHeight() / contentHeight;

  zoomLevel = juce::jlimit(0.25f, 2.0f, std::min(zoomX, zoomY) * 0.8f);
  viewOffset = juce::Point<float>(minX, minY);

  repaint();
}

void CircuitDesigner::addComponent(ComponentType type,
                                   juce::Point<float> position) {
  std::unique_ptr<CircuitComponent> comp;
  juce::String name;

  int id = circuitGraph.getComponentCount();

  switch (type) {
  case ComponentType::Resistor: {
    int node1 = circuitGraph.createNode();
    int node2 = circuitGraph.createNode();
    name = "R" + juce::String(id);
    comp = std::make_unique<Resistor>(id, name, node1, node2, 10000.0);
    break;
  }
  case ComponentType::Capacitor: {
    int node1 = circuitGraph.createNode();
    int node2 = circuitGraph.createNode();
    name = "C" + juce::String(id);
    comp = std::make_unique<Capacitor>(id, name, node1, node2, 100e-9);
    break;
  }
  case ComponentType::Inductor: {
    int node1 = circuitGraph.createNode();
    int node2 = circuitGraph.createNode();
    name = "L" + juce::String(id);
    comp = std::make_unique<Inductor>(id, name, node1, node2, 100e-3);
    break;
  }
  case ComponentType::Potentiometer: {
    int node1 = circuitGraph.createNode();
    int node2 = circuitGraph.createNode();
    int node3 = circuitGraph.createNode();
    name = "POT" + juce::String(id);
    comp =
        std::make_unique<Potentiometer>(id, name, node1, node2, node3, 10000.0);
    break;
  }
  case ComponentType::Switch: {
    int node1 = circuitGraph.createNode();
    int node2 = circuitGraph.createNode();
    name = "SW" + juce::String(id);
    comp = std::make_unique<Switch>(id, name, node1, node2);
    break;
  }
  case ComponentType::Diode: {
    int node1 = circuitGraph.createNode();
    int node2 = circuitGraph.createNode();
    name = "D" + juce::String(id);
    comp = std::make_unique<Diode>(id, name, node1, node2);
    break;
  }
  case ComponentType::DiodePair: {
    int node1 = circuitGraph.createNode();
    int node2 = circuitGraph.createNode();
    name = "DP" + juce::String(id);
    comp = std::make_unique<DiodePair>(id, name, node1, node2);
    break;
  }
  case ComponentType::SoftClipper: {
    int node1 = circuitGraph.createNode();
    int node2 = circuitGraph.createNode();
    name = "SC" + juce::String(id);
    comp = std::make_unique<SoftClipper>(id, name, node1, node2);
    break;
  }
  case ComponentType::VacuumTube: {
    int node1 = circuitGraph.createNode();
    int node2 = circuitGraph.createNode();
    int node3 = circuitGraph.createNode(); // Plate node
    name = "V" + juce::String(id);
    comp = std::make_unique<VacuumTube>(id, name, node1, node2, node3);
    break;
  }
  case ComponentType::AudioInput: {
    int node1 = circuitGraph.createNode();
    name = "IN";
    comp = std::make_unique<AudioInput>(id, name, node1,
                                        circuitGraph.getGroundNodeId());
    break;
  }
  case ComponentType::AudioOutput: {
    int node1 = circuitGraph.createNode();
    name = "OUT";
    comp = std::make_unique<AudioOutput>(id, name, node1,
                                         circuitGraph.getGroundNodeId());
    break;
  }
  case ComponentType::Ground:
    name = "GND";
    comp = std::make_unique<Ground>(id, name, circuitGraph.getGroundNodeId());
    break;
  default:
    return;
  }

  if (comp) {
    comp->setPosition(position);
    circuitGraph.addComponent(std::move(comp));
    rebuildViews();

    if (onCircuitChanged)
      onCircuitChanged();
  }
}

void CircuitDesigner::removeSelectedComponent() {
  if (selectedComponent) {
    if (auto *comp = selectedComponent->getComponent()) {
      circuitGraph.removeComponent(comp->getId());
    }
    selectedComponent = nullptr;
    rebuildViews();

    if (onCircuitChanged)
      onCircuitChanged();
  }
}

void CircuitDesigner::rotateSelectedComponent() {
  if (selectedComponent) {
    if (auto *comp = selectedComponent->getComponent()) {
      int newRot = (comp->getRotation() + 90) % 360;
      comp->setRotation(newRot);

      // Re-read position for the view (though it shouldn't have changed,
      // it might help trigger updates if we used a more complex system)
      selectedComponent->setCanvasPosition(comp->getPosition());

      // Update connected wires
      updateWirePositionsForComponent(selectedComponent);

      if (onCircuitChanged)
        onCircuitChanged();

      repaint();
    }
  }
}

void CircuitDesigner::removeSelectedWire() {
  if (selectedWire) {
    circuitGraph.removeWire(selectedWire->getId());
    selectedWire = nullptr;
    rebuildViews();

    if (onCircuitChanged)
      onCircuitChanged();
  }
}

bool CircuitDesigner::keyPressed(const juce::KeyPress &key) {
  if (key == juce::KeyPress::deleteKey || key == juce::KeyPress::backspaceKey) {
    if (selectedComponent) {
      removeSelectedComponent();
      return true;
    } else if (selectedWire) {
      removeSelectedWire();
      return true;
    }
  } else if (key.getTextCharacter() == 'r' || key.getTextCharacter() == 'R') {
    if (selectedComponent) {
      rotateSelectedComponent();
      return true;
    }
  }
  return false;
}

ComponentView *CircuitDesigner::findComponentAt(juce::Point<float> canvasPos) {
  for (auto &view : componentViews) {
    auto bounds = view->getBoundsInCanvas();
    if (bounds.contains(canvasPos))
      return view.get();
  }
  return nullptr;
}

int CircuitDesigner::findNodeAt(juce::Point<float> canvasPos) {
  auto screenMousePos = canvasToScreen(canvasPos);
  float snapDist = 15.0f; // Screen pixels (larger for easier tapping)

  int closestNode = -1;
  float minDistance = snapDist;

  for (auto &view : componentViews) {
    auto terminals = view->getTerminalPositions();
    for (auto &[nodeId, pos] : terminals) {
      auto screenNodePos = canvasToScreen(pos);
      float distance = screenMousePos.getDistanceFrom(screenNodePos);

      if (distance < minDistance) {
        minDistance = distance;
        closestNode = nodeId;
      }
    }
  }
  return closestNode;
}

void CircuitDesigner::finishWire(int endNode) {
  if (wireStartNode >= 0 && endNode >= 0 && wireStartNode != endNode) {
    circuitGraph.connectNodes(wireStartNode, endNode);
    rebuildViews();

    if (onCircuitChanged)
      onCircuitChanged();
  }

  cancelWire();
}

void CircuitDesigner::cancelWire() {
  isDrawingWire = false;
  wireStartNode = -1;
  repaint();
}

WireView *CircuitDesigner::findWireAt(juce::Point<float> canvasPos) {
  for (auto &wire : wireViews) {
    if (wire->hitTest(canvasPos, zoomLevel))
      return wire.get();
  }
  return nullptr;
}

std::pair<WireView *, juce::Point<float>>
CircuitDesigner::findWireJunctionAt(juce::Point<float> canvasPos) {
  float tolerance = 15.0f / zoomLevel; // Screen pixels converted to canvas

  for (auto &wire : wireViews) {
    if (wire->hitTest(canvasPos, zoomLevel)) {
      auto closestPoint = wire->getClosestPointOnWire(canvasPos);

      // Don't create junction too close to existing endpoints
      float distToStart = closestPoint.getDistanceFrom(wire->getStartPosition());
      float distToEnd = closestPoint.getDistanceFrom(wire->getEndPosition());

      if (distToStart > tolerance && distToEnd > tolerance) {
        return {wire.get(), closestPoint};
      }
    }
  }

  return {nullptr, {}};
}

void CircuitDesigner::finishWireOnWire(WireView *targetWire,
                                       juce::Point<float> junctionPos) {
  if (!targetWire || wireStartNode < 0)
    return;

  // Create junction on the target wire
  int junctionNode =
      circuitGraph.createJunctionOnWire(targetWire->getId(), junctionPos);

  if (junctionNode >= 0) {
    // Connect the new wire from start node to junction
    circuitGraph.connectNodes(wireStartNode, junctionNode);
    rebuildViews();

    if (onCircuitChanged)
      onCircuitChanged();
  }

  cancelWire();
}

void CircuitDesigner::drawJunctions(juce::Graphics &g) {
  // Draw junction dots where wires connect
  for (const auto &junction : circuitGraph.getJunctions()) {
    auto screenPos = canvasToScreen(junction.position);

    // Junction dot with glow effect
    g.setColour(juce::Colour(0xFF00ff88).withAlpha(0.3f));
    g.fillEllipse(screenPos.x - 8 * zoomLevel, screenPos.y - 8 * zoomLevel,
                  16 * zoomLevel, 16 * zoomLevel);

    g.setColour(juce::Colour(0xFF00ddff));
    g.fillEllipse(screenPos.x - 5 * zoomLevel, screenPos.y - 5 * zoomLevel,
                  10 * zoomLevel, 10 * zoomLevel);

    g.setColour(juce::Colours::white);
    g.fillEllipse(screenPos.x - 2 * zoomLevel, screenPos.y - 2 * zoomLevel,
                  4 * zoomLevel, 4 * zoomLevel);
  }
}

void CircuitDesigner::updateWirePositionsForComponent(ComponentView *view) {
  if (!view)
    return;

  auto terminals = view->getTerminalPositions();

  for (auto &wire : wireViews) {
    bool updated = false;

    for (auto &[nodeId, pos] : terminals) {
      if (wire->getNodeA() == nodeId) {
        wire->setStartPosition(pos);
        updated = true;
      }
      if (wire->getNodeB() == nodeId) {
        wire->setEndPosition(pos);
        updated = true;
      }
    }

    if (updated) {
      // Wire position updated, UI will repaint with new positions
    }
  }
}

void CircuitDesigner::rebuildViews() {
  // Store current selection IDs
  int selectedComponentId = -1;
  int selectedWireId = -1;

  if (selectedComponent) {
    if (auto *comp = selectedComponent->getComponent()) {
      selectedComponentId = comp->getId();
    }
  }

  if (selectedWire) {
    selectedWireId = selectedWire->getId();
  }

  // Clear pointers to avoid dangling references
  selectedComponent = nullptr;
  selectedWire = nullptr;

  componentViews.clear();
  wireViews.clear();

  // Create views for all components
  for (const auto &comp : circuitGraph.getComponents()) {
    auto view = std::make_unique<ComponentView>(comp.get(), circuitGraph);
    view->setCanvasPosition(comp->getPosition());
    componentViews.push_back(std::move(view));
  }

  // Create views for all wires
  for (const auto &wire : circuitGraph.getWires()) {
    // Find positions for wire endpoints
    juce::Point<float> startPos, endPos;
    bool foundStart = false, foundEnd = false;

    // First check component terminals
    for (auto &view : componentViews) {
      auto terminals = view->getTerminalPositions();
      for (auto &[nodeId, pos] : terminals) {
        if (nodeId == wire.nodeA) {
          startPos = pos;
          foundStart = true;
        }
        if (nodeId == wire.nodeB) {
          endPos = pos;
          foundEnd = true;
        }
      }
    }

    // Then check junction positions
    for (const auto &junction : circuitGraph.getJunctions()) {
      if (junction.nodeId == wire.nodeA) {
        startPos = junction.position;
        foundStart = true;
      }
      if (junction.nodeId == wire.nodeB) {
        endPos = junction.position;
        foundEnd = true;
      }
    }

    if (foundStart && foundEnd) {
      auto wireView =
          std::make_unique<WireView>(wire.id, wire.nodeA, wire.nodeB);
      wireView->setPositions(startPos, endPos);
      wireViews.push_back(std::move(wireView));
    }
  }

  // Restore selection if possible
  if (selectedComponentId >= 0) {
    for (auto &view : componentViews) {
      if (auto *comp = view->getComponent()) {
        if (comp->getId() == selectedComponentId) {
          selectedComponent = view.get();
          selectedComponent->setSelected(true);
          break;
        }
      }
    }
  }

  if (selectedWireId >= 0) {
    for (auto &wire : wireViews) {
      if (wire->getId() == selectedWireId) {
        selectedWire = wire.get();
        break;
      }
    }
  }

  repaint();
}

void CircuitDesigner::rebuildFromGraph() {
  rebuildViews();
  zoomToFit();
}
