#include "CircuitDesigner.h"
#include "../Engine/Components/AudioInput.h"
#include "../Engine/Components/AudioOutput.h"
#include "../Engine/Components/Capacitor.h"
#include "../Engine/Components/Potentiometer.h"
#include "../Engine/Components/Resistor.h"
#include "../Engine/Components/Switch.h"
#include "../Engine/Components/VacuumTube.h"
#include "ComponentPalette.h"
#include "ComponentView.h"
#include "WireView.h"

CircuitDesigner::CircuitDesigner(CircuitGraph &graph, ComponentPalette &pal)
    : circuitGraph(graph), palette(pal) {
  setWantsKeyboardFocus(true);
  rebuildViews();
}

CircuitDesigner::~CircuitDesigner() = default;

void CircuitDesigner::paint(juce::Graphics &g) {
  // Background
  g.fillAll(juce::Colour(0xFF1e2530));

  drawGrid(g);
  drawWires(g);
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

  // Find start position from node
  juce::Point<float> startPos;
  for (auto &view : componentViews) {
    auto terminals = view->getTerminalPositions();
    for (auto &[nodeId, pos] : terminals) {
      if (nodeId == wireStartNode) {
        startPos = canvasToScreen(pos);
        break;
      }
    }
  }

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

void CircuitDesigner::drawDragPreview(juce::Graphics &g) {
  auto screenPos = canvasToScreen(dragPreviewPosition);

  g.setColour(juce::Colour(0xFF00aaff).withAlpha(0.5f));
  g.fillRoundedRectangle(screenPos.x - 30 * zoomLevel,
                         screenPos.y - 20 * zoomLevel, 60 * zoomLevel,
                         40 * zoomLevel, 5.0f);
}

void CircuitDesigner::drawConnectionPoints(juce::Graphics &g) {
  // Draw connection points (nodes) for each component
  for (auto &view : componentViews) {
    auto terminals = view->getTerminalPositions();

    for (auto &[nodeId, pos] : terminals) {
      auto screenPos = canvasToScreen(pos);

      // Check if hovered
      auto mousePos = getMouseXYRelative().toFloat();
      float distance = mousePos.getDistanceFrom(screenPos);

      if (distance < 15.0f * zoomLevel) {
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
    }
    return;
  }

  // Check if clicking on a connection point
  int clickedNode = findNodeAt(canvasPos);
  if (clickedNode >= 0) {
    if (isDrawingWire) {
      finishWire(clickedNode);
    } else {
      startWire(clickedNode, canvasPos);
    }
    return;
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
  ComponentView *clickedComponent = findComponentAt(canvasPos);
  if (clickedComponent) {
    // Deselect previous
    if (selectedComponent)
      selectedComponent->setSelected(false);

    selectedComponent = clickedComponent;
    selectedComponent->setSelected(true);
    selectedWire = nullptr;

    repaint();
    return;
  }

  // Clicked on empty space - deselect
  if (selectedComponent) {
    selectedComponent->setSelected(false);
    selectedComponent = nullptr;
  }
  selectedWire = nullptr;

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
    wireEndPoint = screenToCanvas(e.position);
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

    repaint();
  }
}

void CircuitDesigner::mouseUp(const juce::MouseEvent &e) {
  juce::ignoreUnused(e);
  isPanning = false;
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

  if (newZoom != zoomLevel) {
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
  else if (desc == "component:potentiometer")
    dragPreviewType = ComponentType::Potentiometer;
  else if (desc == "component:switch")
    dragPreviewType = ComponentType::Switch;
  else if (desc == "component:tube")
    dragPreviewType = ComponentType::VacuumTube;
  else if (desc == "component:input")
    dragPreviewType = ComponentType::AudioInput;
  else if (desc == "component:output")
    dragPreviewType = ComponentType::AudioOutput;

  repaint();
}

void CircuitDesigner::itemDragExit(const SourceDetails &dragSourceDetails) {
  juce::ignoreUnused(dragSourceDetails);
  showDragPreview = false;
  repaint();
}

void CircuitDesigner::itemDropped(const SourceDetails &dragSourceDetails) {
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
  else if (desc == "component:potentiometer")
    type = ComponentType::Potentiometer;
  else if (desc == "component:switch")
    type = ComponentType::Switch;
  else if (desc == "component:tube")
    type = ComponentType::VacuumTube;
  else if (desc == "component:input")
    type = ComponentType::AudioInput;
  else if (desc == "component:output")
    type = ComponentType::AudioOutput;

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
  // Create nodes for the component
  int node1 = circuitGraph.createNode();
  int node2 = circuitGraph.createNode();

  std::unique_ptr<CircuitComponent> comp;
  juce::String name;

  int id = circuitGraph.getComponentCount();

  switch (type) {
  case ComponentType::Resistor:
    name = "R" + juce::String(id);
    comp = std::make_unique<Resistor>(id, name, node1, node2, 10000.0);
    break;
  case ComponentType::Capacitor:
    name = "C" + juce::String(id);
    comp = std::make_unique<Capacitor>(id, name, node1, node2, 100e-9);
    break;
  case ComponentType::Potentiometer: {
    int node3 = circuitGraph.createNode();
    name = "P" + juce::String(id);
    comp =
        std::make_unique<Potentiometer>(id, name, node1, node2, node3, 10000.0);
    break;
  }
  case ComponentType::Switch:
    name = "SW" + juce::String(id);
    comp = std::make_unique<Switch>(id, name, node1, node2);
    break;
  case ComponentType::VacuumTube: {
    int node3 = circuitGraph.createNode(); // Plate node
    name = "V" + juce::String(id);
    comp = std::make_unique<VacuumTube>(id, name, node1, node2, node3);
    break;
  }
  case ComponentType::AudioInput:
    name = "IN";
    comp = std::make_unique<AudioInput>(id, name, node1,
                                        circuitGraph.getGroundNodeId());
    break;
  case ComponentType::AudioOutput:
    name = "OUT";
    comp = std::make_unique<AudioOutput>(id, name, node1,
                                         circuitGraph.getGroundNodeId());
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

ComponentView *CircuitDesigner::findComponentAt(juce::Point<float> canvasPos) {
  for (auto &view : componentViews) {
    auto bounds = view->getBoundsInCanvas();
    if (bounds.contains(canvasPos))
      return view.get();
  }
  return nullptr;
}

int CircuitDesigner::findNodeAt(juce::Point<float> canvasPos) {
  for (auto &view : componentViews) {
    auto terminals = view->getTerminalPositions();
    for (auto &[nodeId, pos] : terminals) {
      if (canvasPos.getDistanceFrom(pos) < SNAP_DISTANCE)
        return nodeId;
    }
  }
  return -1;
}

void CircuitDesigner::startWire(int startNode, juce::Point<float> startPos) {
  isDrawingWire = true;
  wireStartNode = startNode;
  wireEndPoint = startPos;
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
    if (wire->hitTest(canvasPos))
      return wire.get();
  }
  return nullptr;
}

void CircuitDesigner::rebuildViews() {
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

    if (foundStart && foundEnd) {
      auto wireView =
          std::make_unique<WireView>(wire.id, wire.nodeA, wire.nodeB);
      wireView->setPositions(startPos, endPos);
      wireViews.push_back(std::move(wireView));
    }
  }

  repaint();
}
