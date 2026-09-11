// SPDX-License-Identifier: LGPL-2.1-or-later
#pragma once

#include <set>
#include <vector>
#include <QWidget>
#include <QPointer>
#include <Base/Vector3D.h>
#include <Geom_Curve.hxx>
#include <Gui/ToolHandler.h>

class QCheckBox;
class QPushButton;
class QTableWidget;
class QLabel;
class SoSeparator;
class SoText2;
namespace Gui
{
class View3DInventorViewer;
class QuantitySpinBox;
}  // namespace Gui
namespace Surface
{
class FreehandBSpline;
}
namespace SurfaceGui
{
class ViewProviderFreehandBSpline;
class FreehandBSplineEditor: public QWidget, private Gui::ToolHandler, private Gui::SelectionObserver
{
public:
    explicit FreehandBSplineEditor(ViewProviderFreehandBSpline*);
    ~FreehandBSplineEditor() override;
    void setViewer(Gui::View3DInventorViewer*);
    void selectAllPoints();
    bool validate();
    void deletePoints();

protected:
    bool eventFilter(QObject*, QEvent*) override;
    QWidget* getCursorWidget() override;
    QString getCrosshairCursorSVGName() const override;

private:
    ViewProviderFreehandBSpline* vp;
    Surface::FreehandBSpline* feature;
    QPointer<Gui::View3DInventorViewer> viewer;
    SoSeparator* overlay;
    SoSeparator* sceneRoot;
    QTableWidget* table;
    QLabel* status;
    QPushButton* endDrawingButton;
    QWidget* editControls;
    SoSeparator* preview;
    bool drawing {false};
    bool suppressContextMenu {false};
    Qt::MouseButtons ownedButtons {Qt::NoButton};
    void endDrawing();
    void updateDrawingPreview(const QPointF&);
    void updateCurvePreview(const std::vector<Base::Vector3d>&, bool showCursor = false);
    QCheckBox* snapping;
    QCheckBox* coordinates;
    std::set<int> selectedPoints;
    std::set<int> selectedSegments;
    std::vector<Base::Vector3d> dragPoints;
    Base::Vector3d dragOrigin;
    QPointF pressPosition;
    int hoveredPoint {-1};
    int hoveredSegment {-1};
    int dragAnchor {-1};
    int axis {-1};
    bool dragging {false};
    bool refreshing {false};
    QPointer<Gui::QuantitySpinBox> coordinateSpinBox;
    int coordinatePoint {-1};
    int coordinateAxis {-1};
    std::vector<SoText2*> coordinateLabels;
    bool editCoordinateAt(const QPointF&);
    void finishCoordinateEdit(bool apply);
    struct SnapReference
    {
        std::string objectName;
        std::string subname;
    };
    struct SnapVertex
    {
        Base::Vector3d point;
        SnapReference reference;
    };
    struct SnapCurve
    {
        Handle(Geom_Curve) curve;
        double first;
        double last;
        SnapReference reference;
    };
    std::vector<SnapVertex> snapVertices;
    std::vector<SnapCurve> snapCurves;

    Base::Vector3d toWorld(const Base::Vector3d&) const;
    Base::Vector3d toLocal(const Base::Vector3d&) const;
    Base::Vector3d onViewPlane(const QPointF&, const Base::Vector3d&) const;
    QPointF screen(const Base::Vector3d&) const;
    void refresh();
    bool selectedPointIsLocked() const;
    void updateToolButtons();
    bool hasValidReference() const;
    void onSelectionChanged(const Gui::SelectionChanges&) override;
    bool selectedSpansAreLinear() const;
    void updateHints();
    void applyPoints(const std::vector<Base::Vector3d>&);
    std::set<int> movingPoints() const;
    int hitPoint(const QPointF&) const;
    int hitSegment(const QPointF&) const;
    void insertPoint(const QPointF&);
    void alignPoints();
    void setLinear(bool);
    void cacheSnapShapes();
    bool snap(Base::Vector3d&, const QPointF&, SnapReference* reference = nullptr) const;
};
}  // namespace SurfaceGui
