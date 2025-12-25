#pragma once

#include "../Engine/CircuitGraph.h"
#include <JuceHeader.h>

class ComponentPalette;
class ComponentView;
class WireView;

/**
 * CircuitDesigner - Main canvas for designing circuits.
 *
 * Features:
 * - Drag and drop components from palette
 * - Connect components with wires
 * - Pan and zoom navigation
 * - Component selection and editing
 * - Wire probing for oscilloscope
 */
class CircuitDesigner : public juce::Component, public juce::DragAndDropTarget {
public:
  CircuitDesigner(CircuitGraph &graph, ComponentPalette &palette);
  ~CircuitDesigner() override;

  void paint(juce::Graphics &g) override;
  void resized() override;

  // Mouse handling
  void mouseDown(const juce::MouseEvent &e) override;
  void mouseDrag(const juce::MouseEvent &e) override;
  void mouseUp(const juce::MouseEvent &e) override;
  void mouseDoubleClick(const juce::MouseEvent &e) override;
  void mouseWheelMove(const juce::MouseEvent &e,
                      const juce::MouseWheelDetails &wheel) override;
  void mouseMove(const juce::MouseEvent &e) override;

  bool keyPressed(const juce::KeyPress &key) override;

  // Drag and drop
  bool
  isInterestedInDragSource(const SourceDetails &dragSourceDetails) override;
  void itemDragEnter(const SourceDetails &dragSourceDetails) override;
  void itemDragMove(const SourceDetails &dragSourceDetails) override;
  void itemDragExit(const SourceDetails &dragSourceDetails) override;
  void itemDropped(const SourceDetails &dragSourceDetails) override;

  // Callbacks
  std::function<void(int nodeId)> onWireSelected;
  std::function<void()> onCircuitChanged;

  // Rebuild views from graph (e.g., after loading)
  void rebuildFromGraph();

  // Canvas transformation
  juce::Point<float> screenToCanvas(juce::Point<float> screenPos) const;
  juce::Point<float> canvasToScreen(juce::Point<float> canvasPos) const;

  void centerView();
  void zoomToFit();

private:
  CircuitGraph &circuitGraph;

  // Component views
  std::vector<std::unique_ptr<ComponentView>> componentViews;
  std::vector<std::unique_ptr<WireView>> wireViews;

  // Canvas transformation
  float zoomLevel = 1.0f;
  juce::Point<float> viewOffset{0.0f, 0.0f};

  // Interaction state
  enum class Mode { Select, DrawWire, Pan };
  Mode currentMode = Mode::Select;

  ComponentView *selectedComponent = nullptr;
  WireView *selectedWire = nullptr;

  // Wire drawing
  int wireStartNode = -1;
  juce::Point<float> wireStartPosition;
  juce::Point<float> wireEndPoint;
  bool isDrawingWire = false;

  // Panning
  juce::Point<float> panStartOffset;
  juce::Point<int> panStartMouse;
  bool isPanning = false;

  // Drag preview
  bool showDragPreview = false;
  juce::Point<float> dragPreviewPosition;
  ComponentType dragPreviewType = ComponentType::Resistor;

  // Grid
  static constexpr float GRID_SIZE = 20.0f;
  static constexpr float SNAP_DISTANCE = 10.0f;

  juce::Point<float> snapToGrid(juce::Point<float> pos) const;

  // Drawing helpers
  void drawGrid(juce::Graphics &g);
  void drawComponents(juce::Graphics &g);
  void drawWires(juce::Graphics &g);
  void drawWirePreview(juce::Graphics &g);
  void drawDragPreview(juce::Graphics &g);
  void drawConnectionPoints(juce::Graphics &g);

  // Component management
  void addComponent(ComponentType type, juce::Point<float> position);
  void removeSelectedComponent();
  void rotateSelectedComponent();
  void removeSelectedWire();
  ComponentView *findComponentAt(juce::Point<float> canvasPos);
  int findNodeAt(juce::Point<float> canvasPos);

  // Wire management
  void startWire(int startNode, juce::Point<float> startPos);
  void finishWire(int endNode);
  void finishWireOnWire(WireView *targetWire, juce::Point<float> junctionPos);
  void cancelWire();
  WireView *findWireAt(juce::Point<float> canvasPos);
  void updateWirePositionsForComponent(ComponentView *view);

  // Junction management
  std::pair<WireView *, juce::Point<float>>
  findWireJunctionAt(juce::Point<float> canvasPos);
  void drawJunctions(juce::Graphics &g);

  // Rebuild views from graph
  void rebuildViews();

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CircuitDesigner)
};
