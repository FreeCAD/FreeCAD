// SPDX-License-Identifier: LGPL-2.1-or-later

#include <algorithm>
#include <cmath>
#include <QApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QMouseEvent>
#include <QPushButton>
#include <QSignalBlocker>
#include <QTableWidget>
#include <QVBoxLayout>
#include <Inventor/SbLine.h>
#include <Inventor/SoPickedPoint.h>
#include <memory>
#include <Inventor/SbPlane.h>
#include <Inventor/nodes/SoBaseColor.h>
#include <Inventor/nodes/SoCamera.h>
#include <Inventor/nodes/SoCoordinate3.h>
#include <Inventor/nodes/SoDrawStyle.h>
#include <Inventor/nodes/SoFont.h>
#include <Inventor/nodes/SoLineSet.h>
#include <Inventor/nodes/SoPickStyle.h>
#include <Inventor/nodes/SoPointSet.h>
#include <Inventor/nodes/SoMarkerSet.h>
#include <App/Application.h>
#include <Gui/Inventor/MarkerBitmaps.h>
#include <Inventor/nodes/SoSeparator.h>
#include <Inventor/nodes/SoText2.h>
#include <Inventor/nodes/SoTranslation.h>
#include <BRep_Tool.hxx>
#include <GeomAPI_ProjectPointOnCurve.hxx>
#include <TopExp_Explorer.hxx>
#include <TopExp.hxx>
#include <TopTools_IndexedMapOfShape.hxx>
#include <TopoDS.hxx>
#include <App/Document.h>
#include <Base/Exception.h>
#include <Gui/Document.h>
#include <Gui/InputHint.h>
#include <Gui/QuantitySpinBox.h>
#include <Inventor/actions/SoRayPickAction.h>
#include <Inventor/SoRenderManager.h>
#include <Inventor/details/SoTextDetail.h>
#include <Gui/MainWindow.h>
#include <Gui/Selection/Selection.h>
#include <Gui/View3DInventorViewer.h>
#include <Mod/Surface/App/FeatureFreehandBSpline.h>
#include "FreehandBSplineEditor.h"
#include "ViewProviderFreehandBSpline.h"

using namespace SurfaceGui;

namespace
{
// Share Sketcher's preference keys without requiring the Sketcher workbench to be loaded.
struct EditStyle
{
    ParameterGrp::handle view = App::GetApplication().GetParameterGroupByPath(
        "User parameter:BaseApp/Preferences/View"
    );
    ParameterGrp::handle sketch = App::GetApplication().GetParameterGroupByPath(
        "User parameter:BaseApp/Preferences/Mod/Sketcher/View"
    );
    SbColor color(const char* key, unsigned long fallback) const
    {
        SbColor result;
        float transparency = 0;
        result.setPackedValue(static_cast<uint32_t>(view->GetUnsigned(key, fallback)), transparency);
        return result;
    }
    SbColor geometry = color("EditedEdgeColor", 0xffffffff);
    SbColor construction = color("ConstructionColor", 0x0000dcff);
    SbColor constraint = color("ConstrainedIcoColor", 0xff2600ff);
    SbColor selection = color("SelectionColor", 0x00abffff);
    SbColor preselection = color("HighlightColor", 0x0ac8ffff);
    SbColor highlight(const SbColor& normal, bool selected, bool hovered) const
    {
        return hovered ? preselection : selected ? selection : normal;
    }
    double scale(double pixelRatio) const
    {
        return pixelRatio * std::clamp(view->GetFloat("ViewScalingFactor", 1.0), 0.5, 5.0);
    }
    SoMarkerSet* marker(double pixelRatio) const
    {
        const auto sizes = Gui::Inventor::MarkerBitmaps::getSupportedSizes("CIRCLE_FILLED");
        auto size = static_cast<int>(std::lround(view->GetInt("MarkerSize", 7) * pixelRatio));
        if (!sizes.empty()) {
            const auto match = std::lower_bound(sizes.begin(), sizes.end(), size);
            size = match != sizes.end() ? *match : sizes.back();
        }
        auto result = new SoMarkerSet;
        result->markerIndex = Gui::Inventor::MarkerBitmaps::getMarkerIndex("CIRCLE_FILLED", size);
        return result;
    }
};
SbVec3f coin(const Base::Vector3d& p)
{
    return SbVec3f(static_cast<float>(p.x), static_cast<float>(p.y), static_cast<float>(p.z));
}
Base::Vector3d vector(const gp_Pnt& p)
{
    return Base::Vector3d(p.X(), p.Y(), p.Z());
}
double squareDistance(const QPointF& a, const QPointF& b)
{
    const auto d = a - b;
    return d.x() * d.x() + d.y() * d.y();
}
double segmentDistance(const QPointF& p, const QPointF& a, const QPointF& b, double& t)
{
    const auto ab = b - a, ap = p - a;
    const double length = squareDistance(a, b);
    t = length > 1e-12 ? std::clamp((ap.x() * ab.x() + ap.y() * ab.y()) / length, 0.0, 1.0) : 0;
    return squareDistance(p, a + ab * t);
}
}  // namespace

FreehandBSplineEditor::FreehandBSplineEditor(ViewProviderFreehandBSpline* provider)
    : Gui::SelectionObserver(false)
    , vp(provider)
    , feature(static_cast<Surface::FreehandBSpline*>(provider->getObject()))
    , overlay(new SoSeparator)
    , sceneRoot(provider->getRoot())
{
    setWindowTitle(tr("Freehand B-spline"));
    setObjectName(QStringLiteral("FreehandBSplineEditor"));
    overlay->setName("FreehandBSplineEditOverlay");
    overlay->ref();
    sceneRoot->ref();
    sceneRoot->addChild(overlay);
    auto layout = new QVBoxLayout(this);
    drawing = feature->Points.getSize() < 2;
    preview = new SoSeparator;
    preview->ref();
    preview->setName("FreehandBSplineDrawingPreview");
    sceneRoot->addChild(preview);
    endDrawingButton = new QPushButton(tr("End B-spline"));
    endDrawingButton->setObjectName(QStringLiteral("endBSpline"));
    endDrawingButton->setVisible(drawing);
    connect(endDrawingButton, &QPushButton::clicked, this, [this] { endDrawing(); });
    layout->addWidget(endDrawingButton);
    snapping = new QCheckBox(tr("Snap to vertices and edges"));
    snapping->setObjectName(QStringLiteral("snapping"));
    snapping->setChecked(true);
    layout->addWidget(snapping);
    coordinates = new QCheckBox(tr("Show point coordinates"));
    coordinates->setChecked(true);
    layout->addWidget(coordinates);
    auto closed = new QCheckBox(tr("Closed curve"));
    closed->setObjectName(QStringLiteral("closedCurve"));
    closed->setChecked(feature->Periodic.getValue());
    layout->addWidget(closed);
    auto parameterization = new QComboBox;
    parameterization->setObjectName(QStringLiteral("parameterization"));
    parameterization->addItem(tr("Uniform"), 0.0);
    parameterization->addItem(tr("Centripetal"), 0.5);
    parameterization->addItem(tr("Chord length"), 1.0);
    parameterization->setCurrentIndex(
        feature->Parameterization.getValue() < 0.25       ? 0
            : feature->Parameterization.getValue() > 0.75 ? 2
                                                          : 1
    );
    parameterization->setToolTip(
        tr("Controls how the curve bends between interpolation points.\n"
           "Uniform: gives each span equal weight, regardless of point spacing.\n"
           "Centripetal: uses the square root of point spacing; a balanced default for unevenly "
           "spaced points.\n"
           "Chord length: uses the distance between points to weight each span.\n"
           "All three methods pass through the same points.")
    );
    auto interpolationRow = new QHBoxLayout;
    interpolationRow->addWidget(new QLabel(tr("Interpolation spacing")));
    interpolationRow->addWidget(parameterization, 1);
    layout->addLayout(interpolationRow);
    table = new QTableWidget(0, 5);
    table->setObjectName(QStringLiteral("interpolationPoints"));
    table->setHorizontalHeaderLabels(
        {tr("Locked to"), tr("Tangent to"), tr("X (mm)"), tr("Y (mm)"), tr("Z (mm)")}
    );
    table->setSelectionBehavior(QAbstractItemView::SelectRows);
    table->setSelectionMode(QAbstractItemView::ExtendedSelection);
    table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Interactive);
    table->setColumnWidth(0, 112);
    table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Interactive);
    table->setColumnWidth(1, 100);
    for (int column = 2; column < 5; ++column) {
        table->horizontalHeader()->setSectionResizeMode(column, QHeaderView::Interactive);
        table->setColumnWidth(column, 60);
    }
    table->setMinimumHeight(140);
    layout->addWidget(table);
    auto buttons = new QGridLayout;
    auto button = [&](const QString& text, const char* name, int row, int column, auto callback) {
        auto widget = new QPushButton(text);
        widget->setObjectName(QString::fromLatin1(name));
        buttons->addWidget(widget, row, column);
        connect(widget, &QPushButton::clicked, this, callback);
    };
    button(tr("Align selection"), "alignSelection", 0, 0, [this] { alignPoints(); });
    button(tr("Lock to"), "lockPoint", 0, 1, [this] {
        if (selectedPoints.size() != 1) {
            status->setText(tr("Select one spline point, then a vertex, edge or face to lock it to."));
            return;
        }
        try {
            if (selectedPointIsLocked()) {
                feature->setPointSupport(*selectedPoints.begin(), nullptr, {});
                applyPoints(feature->Points.getValues());
                return;
            }
            App::DocumentObject* reference = nullptr;
            std::string subname;
            for (auto selected : Gui::Selection().getSelectionEx()) {
                if (selected.getObject() == feature) {
                    continue;
                }
                const auto subs = selected.getSubNames();
                if (reference || subs.size() > 1) {
                    throw Base::ValueError("Select exactly one reference vertex, edge or face.");
                }
                reference = selected.getObject();
                subname = subs.empty() ? std::string() : subs.front();
            }
            if (!reference) {
                throw Base::ValueError("Select a reference vertex, edge or face in the view, then "
                                       "press Lock to.");
            }
            feature->setPointSupport(*selectedPoints.begin(), reference, subname);
            applyPoints(feature->Points.getValues());
        }
        catch (const Base::Exception& error) {
            status->setText(QString::fromUtf8(error.what()));
        }
        catch (const Standard_Failure& error) {
            status->setText(QString::fromUtf8(error.GetMessageString()));
        }
    });
    button(tr("Set linear interpolation"), "toggleInterpolation", 1, 0, [this] {
        setLinear(!selectedSpansAreLinear());
    });
    button(tr("Select all"), "selectAll", 1, 1, [this] { selectAllPoints(); });
    editControls = new QWidget;
    editControls->setLayout(buttons);
    editControls->setEnabled(!drawing);
    table->setEnabled(!drawing);
    layout->addWidget(editControls);
    status = new QLabel;
    status->setWordWrap(true);
    layout->addWidget(status);
    connect(coordinates, &QCheckBox::toggled, this, [this] { refresh(); });
    connect(closed, &QCheckBox::toggled, this, [this](bool value) {
        feature->Periodic.setValue(value);
        selectedSegments.clear();
        applyPoints(feature->Points.getValues());
    });
    connect(
        parameterization,
        qOverload<int>(&QComboBox::currentIndexChanged),
        this,
        [this, parameterization] {
            feature->Parameterization.setValue(parameterization->currentData().toDouble());
            applyPoints(feature->Points.getValues());
        }
    );
    connect(table, &QTableWidget::itemSelectionChanged, this, [this] {
        if (refreshing) {
            return;
        }
        selectedPoints.clear();
        selectedSegments.clear();
        for (auto item : table->selectedItems()) {
            selectedPoints.insert(item->row());
        }
        refresh();
    });
    connect(table, &QTableWidget::cellChanged, this, [this](int row, int column) {
        if (refreshing) {
            return;
        }
        if (column == 0) {
            try {
                const auto text = table->item(row, column)->text().trimmed();
                App::DocumentObject* object = nullptr;
                std::string subname;
                if (!text.isEmpty()) {
                    const auto split = text.indexOf(QLatin1Char('.'));
                    const auto objectName = split < 0 ? text : text.left(split);
                    object = feature->getDocument()->getObject(objectName.toUtf8().constData());
                    subname = split < 0 ? std::string() : text.mid(split + 1).toStdString();
                    if (!object) {
                        throw Base::ValueError("The reference object does not exist.");
                    }
                }
                feature->setPointSupport(row, object, subname);
                applyPoints(feature->Points.getValues());
            }
            catch (const Base::Exception& error) {
                refresh();
                status->setText(QString::fromUtf8(error.what()));
            }
            catch (const Standard_Failure& error) {
                refresh();
                status->setText(QString::fromUtf8(error.GetMessageString()));
            }
            return;
        }
        if (column == 1) {
            return;
        }
        bool valid = false;
        double value = table->item(row, column)->text().toDouble(&valid);
        if (!valid || !std::isfinite(value)) {
            refresh();
            return;
        }
        auto points = feature->Points.getValues();
        points[row][column - 2] = value;
        applyPoints(points);
    });
    Gui::Selection().clearSelection();
    refresh();
}

FreehandBSplineEditor::~FreehandBSplineEditor()
{
    qApp->removeEventFilter(this);
    setViewer(nullptr);
    sceneRoot->removeChild(preview);
    preview->unref();
    sceneRoot->removeChild(overlay);
    sceneRoot->unref();
    overlay->unref();
}

void FreehandBSplineEditor::setViewer(Gui::View3DInventorViewer* view)
{
    finishCoordinateEdit(false);
    detachSelection();
    qApp->removeEventFilter(this);
    const bool hadViewer = viewer;
    if (viewer) {
        if (drawing) {
            Gui::ToolHandler::deactivate();
        }
    }
    viewer = view;
    if (viewer) {
        attachSelection();
        qApp->installEventFilter(this);
        viewer->getGLWidget()->setMouseTracking(true);
        cacheSnapShapes();
        if (drawing) {
            Gui::ToolHandler::activate();
        }
    }
    dragging = false;
    ownedButtons = Qt::NoButton;
    if (viewer) {
        updateHints();
    }
    else if (hadViewer && Gui::getMainWindow()) {
        Gui::getMainWindow()->hideHints();
    }
}

QWidget* FreehandBSplineEditor::getCursorWidget()
{
    return viewer ? viewer->getGLWidget() : nullptr;
}

QString FreehandBSplineEditor::getCrosshairCursorSVGName() const
{
    return QStringLiteral("Surface_Pointer_FreehandBSpline");
}

void FreehandBSplineEditor::endDrawing()
{
    if (!validate()) {
        return;
    }
    if (viewer) {
        Gui::ToolHandler::deactivate();
    }
    drawing = false;
    preview->removeAllChildren();
    endDrawingButton->hide();
    editControls->setEnabled(true);
    table->setEnabled(true);
    if (viewer) {
        viewer->getGLWidget()->setFocus();
    }
    refresh();
}

void FreehandBSplineEditor::updateDrawingPreview(const QPointF& position)
{
    auto points = feature->Points.getValues();
    auto candidate = onViewPlane(position, toWorld(points.empty() ? Base::Vector3d() : points.back()));
    if (snapping->isChecked()) {
        snap(candidate, position);
    }
    points.push_back(toLocal(candidate));
    updateCurvePreview(points, true);
}

void FreehandBSplineEditor::updateCurvePreview(const std::vector<Base::Vector3d>& points, bool showCursor)
{
    preview->removeAllChildren();
    auto pick = new SoPickStyle;
    pick->style = SoPickStyle::UNPICKABLE;
    preview->addChild(pick);
    auto color = new SoBaseColor;
    const EditStyle preferences;
    const auto ratio = viewer ? viewer->getGLWidget()->devicePixelRatioF() : devicePixelRatioF();
    color->rgb = preferences.geometry;
    preview->addChild(color);
    auto style = new SoDrawStyle;
    style->lineWidth = preferences.sketch->GetInt("EdgeWidth", 2) * preferences.scale(ratio);
    style->linePattern = preferences.sketch->GetInt("EdgePattern", 0xffff);
    preview->addChild(style);
    if (showCursor && !points.empty()) {
        auto cursor = new SoCoordinate3;
        cursor->point.set1Value(0, coin(points.back()));
        preview->addChild(cursor);
        preview->addChild(preferences.marker(ratio));
    }
    try {
        // Use the same interpolation as the feature, without modifying document properties.
        auto curve = feature->interpolate(points);
        auto vertices = new SoCoordinate3;
        constexpr int samples = 128;
        for (int i = 0; i <= samples; ++i) {
            double parameter = curve->FirstParameter()
                + (curve->LastParameter() - curve->FirstParameter()) * i / samples;
            vertices->point.set1Value(i, coin(vector(curve->Value(parameter))));
        }
        preview->addChild(vertices);
        auto line = new SoLineSet;
        line->numVertices.set1Value(0, samples + 1);
        preview->addChild(line);
    }
    catch (const Base::Exception&) {
        // An insufficient or coincident provisional point still has a cursor marker.
    }
    catch (const Standard_Failure&) {
    }
}

void FreehandBSplineEditor::updateHints()
{
    if (!viewer) {
        return;
    }
    using enum Gui::InputHint::UserInput;
    std::list<Gui::InputHint> hints;
    if (drawing) {
        hints = {
            {.message = tr("%1 place interpolation point"), .sequences = {MouseLeft}},
            {.message = tr("%1 / %2 end B-spline and edit points"),
             .sequences = {MouseRight, KeyEscape}},
        };
    }
    else if (dragging) {
        hints = {
            {.message = axis < 0
                 ? tr("%1 move selection")
                 : tr("%1 move along %2").arg(QStringLiteral("%1"), QString(QChar('X' + axis))),
             .sequences = {MouseMoveLeft}},
            {.message = tr("%1 fine movement"), .sequences = {ModifierShift}},
            {.message = tr("%1 / %2 / %3 toggle axis constraint"), .sequences = {KeyX, KeyY, KeyZ}},
            {.message = tr("%1 cancel drag"), .sequences = {KeyEscape}},
        };
    }
    else {
        hints = {
            {.message = tr("%1 extend selection"), .sequences = {{ModifierCtrl, MouseLeft}}},
            {.message = tr("%1 drag selection"), .sequences = {MouseMoveLeft}},
            {.message = tr("%1 insert point"), .sequences = {MouseDoubleLeft}},
            {.message = tr("%1 select all"), .sequences = {{ModifierCtrl, KeyA}}},
        };
        if (!movingPoints().empty()) {
            hints.push_back({.message = tr("%1 delete points"), .sequences = {KeyDelete}});
        }
    }
    Gui::getMainWindow()->showHints(hints);
}

Base::Vector3d FreehandBSplineEditor::toWorld(const Base::Vector3d& p) const
{
    Base::Vector3d result;
    App::GeoFeature::getGlobalPlacement(feature).multVec(p, result);
    return result;
}
Base::Vector3d FreehandBSplineEditor::toLocal(const Base::Vector3d& p) const
{
    Base::Vector3d result;
    App::GeoFeature::getGlobalPlacement(feature).inverse().multVec(p, result);
    return result;
}
QPointF FreehandBSplineEditor::screen(const Base::Vector3d& p) const
{
    const auto size = viewer->getGLWidget()->size();
    SbVec3f projected;
    viewer->getCamera()
        ->getViewVolume(static_cast<float>(size.width()) / size.height())
        .projectToScreen(coin(p), projected);
    return {projected[0] * size.width(), (1 - projected[1]) * size.height()};
}
Base::Vector3d FreehandBSplineEditor::onViewPlane(
    const QPointF& position,
    const Base::Vector3d& origin
) const
{
    const auto size = viewer->getGLWidget()->size();
    auto volume = viewer->getCamera()->getViewVolume(static_cast<float>(size.width()) / size.height());
    SbLine ray;
    volume.projectPointToLine(
        SbVec2f(
            static_cast<float>(position.x() / size.width()),
            static_cast<float>(1 - position.y() / size.height())
        ),
        ray
    );
    SbPlane plane(volume.getProjectionDirection(), coin(origin));
    SbVec3f result;
    if (!plane.intersect(ray, result)) {
        return origin;
    }
    return Base::Vector3d(result[0], result[1], result[2]);
}

bool FreehandBSplineEditor::selectedPointIsLocked() const
{
    if (selectedPoints.size() != 1 || !selectedSegments.empty()) {
        return false;
    }
    const auto& indices = feature->SupportPointIndices.getValues();
    return std::find(indices.begin(), indices.end(), *selectedPoints.begin()) != indices.end();
}

bool FreehandBSplineEditor::selectedSpansAreLinear() const
{
    const int count = feature->Points.getSize();
    const int spans = feature->Periodic.getValue() ? count : count - 1;
    const auto& flags = feature->LinearSegments.getValues();
    bool found = false;
    for (int i = 0; i < spans; ++i) {
        if (selectedSegments.count(i)) {
            found = true;
            if (i >= static_cast<int>(flags.size()) || !flags[i]) {
                return false;
            }
        }
    }
    return found;
}

bool FreehandBSplineEditor::hasValidReference() const
{
    try {
        App::DocumentObject* reference = nullptr;
        std::string subname;
        for (auto selected : Gui::Selection().getSelectionEx()) {
            if (selected.getObject() == feature) {
                continue;
            }
            const auto subs = selected.getSubNames();
            if (reference || subs.size() > 1) {
                return false;
            }
            reference = selected.getObject();
            subname = subs.empty() ? std::string() : subs.front();
        }
        if (!reference || reference->getDocument() != feature->getDocument()) {
            return false;
        }
        const auto dependents = feature->getInListRecursive();
        if (std::find(dependents.begin(), dependents.end(), reference) != dependents.end()) {
            return false;
        }
        auto shape = Part::Feature::getShape(
            reference,
            Part::ShapeOption::ResolveLink | Part::ShapeOption::Transform
                | Part::ShapeOption::NeedSubElement,
            subname.c_str()
        );
        if (shape.IsNull()) {
            return false;
        }
        if (shape.ShapeType() == TopAbs_VERTEX || shape.ShapeType() == TopAbs_EDGE
            || shape.ShapeType() == TopAbs_FACE) {
            return true;
        }
        if ((shape.ShapeType() == TopAbs_WIRE || shape.ShapeType() == TopAbs_COMPOUND)
            && !TopExp_Explorer(shape, TopAbs_FACE).More()) {
            TopExp_Explorer edges(shape, TopAbs_EDGE);
            if (edges.More()) {
                edges.Next();
                return !edges.More();
            }
        }
    }
    catch (const Base::Exception&) {
    }
    catch (const Standard_Failure&) {
    }
    return false;
}

void FreehandBSplineEditor::onSelectionChanged(const Gui::SelectionChanges&)
{
    if (viewer) {
        updateToolButtons();
    }
}

void FreehandBSplineEditor::updateToolButtons()
{
    const auto moving = movingPoints();
    const auto& points = feature->Points.getValues();
    auto lock = findChild<QPushButton*>(QStringLiteral("lockPoint"));
    lock->setText(selectedPointIsLocked() ? tr("Unlock") : tr("Lock to"));
    lock->setEnabled(
        !drawing && selectedPoints.size() == 1 && selectedSegments.empty()
        && (selectedPointIsLocked() || hasValidReference())
    );
    auto interpolation = findChild<QPushButton*>(QStringLiteral("toggleInterpolation"));
    interpolation->setText(
        selectedSpansAreLinear() ? tr("Set smooth interpolation") : tr("Set linear interpolation")
    );
    interpolation->setEnabled(!drawing && !selectedSegments.empty());
    findChild<QPushButton*>(QStringLiteral("alignSelection"))
        ->setEnabled(
            !drawing && moving.size() >= 3
            && (points[*moving.rbegin()] - points[*moving.begin()]).Sqr() >= 1e-14
        );
    findChild<QPushButton*>(QStringLiteral("selectAll"))
        ->setEnabled(!drawing && !points.empty() && selectedPoints.size() != points.size());
}

void FreehandBSplineEditor::refresh()
{
    feature->sanitizeReferences();
    const EditStyle preferences;
    const auto ratio = viewer ? viewer->getGLWidget()->devicePixelRatioF() : devicePixelRatioF();
    refreshing = true;
    QSignalBlocker blocker(table);
    const auto& points = feature->Points.getValues();
    table->setRowCount(static_cast<int>(points.size()));
    table->clearSelection();
    coordinateLabels.assign(points.size(), nullptr);
    overlay->removeAllChildren();
    auto pick = new SoPickStyle;
    pick->style = SoPickStyle::UNPICKABLE;
    overlay->addChild(pick);
    const int count = static_cast<int>(points.size());
    const int spans = feature->Periodic.getValue() && count > 2 ? count : count - 1;
    for (int i = 0; i < spans; ++i) {
        auto separator = new SoSeparator;
        auto color = new SoBaseColor;
        color->rgb = preferences.highlight(
            preferences.construction,
            selectedSegments.count(i),
            hoveredSegment == i
        );
        auto style = new SoDrawStyle;
        style->lineWidth = preferences.sketch->GetInt("ConstructionWidth", 2)
            * preferences.scale(ratio);
        style->linePattern = preferences.sketch->GetInt("ConstructionPattern", 0xfcfc);
        auto vertices = new SoCoordinate3;
        vertices->point.set1Value(0, coin(points[i]));
        vertices->point.set1Value(1, coin(points[(i + 1) % count]));
        auto line = new SoLineSet;
        line->numVertices.set1Value(0, 2);
        separator->addChild(color);
        separator->addChild(style);
        separator->addChild(vertices);
        separator->addChild(line);
        overlay->addChild(separator);
    }
    for (int i = 0; i < count; ++i) {
        for (int j = 0; j < 3; ++j) {
            auto item = table->item(i, j + 2);
            if (!item) {
                item = new QTableWidgetItem;
                table->setItem(i, j + 2, item);
            }
            item->setText(QString::number(points[i][j], 'g', 10));
            item->setSelected(selectedPoints.count(i));
        }
        auto reference = table->item(i, 0);
        if (!reference) {
            reference = new QTableWidgetItem;
            table->setItem(i, 0, reference);
        }
        QString name;
        const auto indices = feature->SupportPointIndices.getValues();
        for (size_t j = 0; j < indices.size(); ++j) {
            if (indices[j] == i && feature->Support.getValues()[j]) {
                name = QString::fromUtf8(feature->Support.getValues()[j]->getNameInDocument());
                if (!feature->Support.getSubValues()[j].empty()) {
                    name += QLatin1Char('.')
                        + QString::fromStdString(feature->Support.getSubValues()[j]);
                }
            }
        }
        reference->setText(name);
        reference->setToolTip(
            tr("Persistent reference: ObjectName.Vertex1, ObjectName.Edge1 or "
               "ObjectName.Face1.\nEdit to replace the reference, or clear to unlock.")
        );
        reference->setSelected(selectedPoints.count(i));
        auto tangentItem = table->item(i, 1);
        if (!tangentItem) {
            tangentItem = new QTableWidgetItem;
            tangentItem->setFlags(tangentItem->flags() & ~Qt::ItemIsEditable);
            table->setItem(i, 1, tangentItem);
        }
        tangentItem->setSelected(selectedPoints.count(i));
        if (name.isEmpty()) {
            table->removeCellWidget(i, 1);
        }
        else {
            auto combo = qobject_cast<QComboBox*>(table->cellWidget(i, 1));
            if (!combo) {
                combo = new QComboBox;
                combo->setToolTip(tr("Follow an edge tangent or the tangent plane of a face. "
                                     "None leaves the spline direction unconstrained."));
                table->setCellWidget(i, 1, combo);
                connect(combo, qOverload<int>(&QComboBox::activated), this, [this, combo, i] {
                    const auto objects = feature->TangentSupport.getValues();
                    const auto names = feature->TangentSupport.getSubValues();
                    const auto indices = feature->TangentPointIndices.getValues();
                    try {
                        feature->setPointTangent(i, combo->currentData().toString().toStdString());
                        feature->interpolate();
                        applyPoints(feature->Points.getValues());
                    }
                    catch (const Base::Exception& error) {
                        feature->TangentSupport.setValues(objects, names);
                        feature->TangentPointIndices.setValues(indices);
                        refresh();
                        status->setText(QString::fromUtf8(error.what()));
                    }
                    catch (const Standard_Failure& error) {
                        feature->TangentSupport.setValues(objects, names);
                        feature->TangentPointIndices.setValues(indices);
                        refresh();
                        status->setText(QString::fromUtf8(error.GetMessageString()));
                    }
                });
            }
            const QSignalBlocker comboBlocker(combo);
            combo->clear();
            combo->addItem(tr("None"), QString());
            try {
                for (const auto& choice : feature->tangentChoices(i)) {
                    const auto label = QString::fromStdString(choice);
                    combo->addItem(label, label);
                }
            }
            catch (const Base::Exception&) {
            }
            catch (const Standard_Failure&) {
            }
            const auto& tangentIndices = feature->TangentPointIndices.getValues();
            for (size_t j = 0; j < tangentIndices.size(); ++j) {
                if (tangentIndices[j] == i) {
                    const auto value = QString::fromStdString(feature->TangentSupport.getSubValues(
                    )[j]);
                    int selected = combo->findData(value);
                    if (selected < 0) {
                        combo->addItem(value, value);
                        selected = combo->count() - 1;
                    }
                    combo->setCurrentIndex(selected);
                }
            }
        }
        auto separator = new SoSeparator;
        auto color = new SoBaseColor;
        const bool endpoint = !feature->Periodic.getValue() && (i == 0 || i == count - 1);
        const auto normal = !name.isEmpty() ? preferences.constraint
            : endpoint                      ? preferences.geometry
                                            : preferences.construction;
        color->rgb = preferences.highlight(normal, selectedPoints.count(i), hoveredPoint == i);
        auto vertex = new SoCoordinate3;
        vertex->point.set1Value(0, coin(points[i]));
        separator->addChild(color);
        separator->addChild(vertex);
        separator->addChild(preferences.marker(ratio));
        if (coordinates->isChecked()) {
            auto translation = new SoTranslation;
            translation->translation = coin(points[i]);
            auto font = new SoFont;
            font->size = 10;
            auto text = new SoText2;
            coordinateLabels[i] = text;
            const QStringList labels {
                QStringLiteral("  %1").arg(i + 1),
                tr("  X %1").arg(points[i].x, 0, 'f', 2),
                tr("  Y %1").arg(points[i].y, 0, 'f', 2),
                tr("  Z %1").arg(points[i].z, 0, 'f', 2),
            };
            for (int row = 0; row < labels.size(); ++row) {
                text->string.set1Value(row, labels[row].toUtf8().constData());
            }
            separator->addChild(translation);
            separator->addChild(font);
            separator->addChild(text);
        }
        overlay->addChild(separator);
    }
    updateCurvePreview(points);
    updateToolButtons();
    refreshing = false;
    updateHints();
}

void FreehandBSplineEditor::applyPoints(const std::vector<Base::Vector3d>& points)
{
    hoveredPoint = -1;
    hoveredSegment = -1;
    feature->Points.setValues(points);
    try {
        feature->updateSupportedPoints(true);
    }
    catch (const Base::Exception& error) {
        refresh();
        status->setText(QString::fromUtf8(error.what()));
        return;
    }
    catch (const Standard_Failure& error) {
        refresh();
        status->setText(QString::fromUtf8(error.GetMessageString()));
        return;
    }
    if (points.size() >= (feature->Periodic.getValue() ? 3U : 2U)) {
        if (validate()) {
            feature->recomputeFeature();
        }
    }
    else {
        feature->Shape.setValue(TopoDS_Shape());
    }
    refresh();
}

bool FreehandBSplineEditor::validate()
{
    try {
        feature->interpolate();
        status->clear();
        return true;
    }
    catch (const Standard_Failure& error) {
        status->setText(QString::fromUtf8(error.GetMessageString()));
    }
    catch (const Base::Exception& error) {
        status->setText(QString::fromUtf8(error.what()));
    }
    return false;
}

std::set<int> FreehandBSplineEditor::movingPoints() const
{
    auto result = selectedPoints;
    const int count = feature->Points.getSize();
    for (int i : selectedSegments) {
        result.insert(i);
        result.insert((i + 1) % count);
    }
    return result;
}

int FreehandBSplineEditor::hitPoint(const QPointF& position) const
{
    const auto& points = feature->Points.getValues();
    double distance = 100.0;
    int hit = -1;
    for (size_t i = 0; i < points.size(); ++i) {
        const double d = squareDistance(position, screen(toWorld(points[i])));
        if (d < distance) {
            distance = d;
            hit = static_cast<int>(i);
        }
    }
    return hit;
}
int FreehandBSplineEditor::hitSegment(const QPointF& position) const
{
    const auto& points = feature->Points.getValues();
    const int count = static_cast<int>(points.size());
    int hit = -1;
    double distance = 64.0;
    for (int i = 0; i < (feature->Periodic.getValue() ? count : count - 1); ++i) {
        double t;
        const double d = segmentDistance(
            position,
            screen(toWorld(points[i])),
            screen(toWorld(points[(i + 1) % count])),
            t
        );
        if (d < distance) {
            hit = i;
            distance = d;
        }
    }
    return hit;
}

void FreehandBSplineEditor::selectAllPoints()
{
    selectedPoints.clear();
    selectedSegments.clear();
    for (int i = 0; i < feature->Points.getSize(); ++i) {
        selectedPoints.insert(i);
    }
    refresh();
}

void FreehandBSplineEditor::deletePoints()
{
    const auto removed = movingPoints();
    if (removed.empty()) {
        return;
    }
    auto points = feature->Points.getValues();
    std::vector<Base::Vector3d> remaining;
    boost::dynamic_bitset<> linear;
    const auto flags = feature->LinearSegments.getValues();
    std::vector<int> retained;
    for (int i = 0; i < static_cast<int>(points.size()); ++i) {
        if (!removed.count(i)) {
            remaining.push_back(points[i]);
            retained.push_back(i);
        }
    }
    if (remaining.size() < (feature->Periodic.getValue() ? 3U : 2U)) {
        status->setText(tr("Keep at least two points, or three for a closed curve."));
        return;
    }
    const size_t spans = feature->Periodic.getValue() ? retained.size() : retained.size() - 1;
    for (size_t i = 0; i < spans; ++i) {
        int previous = retained[i], next = retained[(i + 1) % retained.size()];
        linear.push_back(
            next == (previous + 1) % static_cast<int>(points.size())
            && previous < static_cast<int>(flags.size()) && flags[previous]
        );
    }
    feature->remapSupports(retained);
    feature->LinearSegments.setValues(linear);
    selectedPoints.clear();
    selectedSegments.clear();
    applyPoints(remaining);
}

void FreehandBSplineEditor::alignPoints()
{
    const auto selected = movingPoints();
    if (selected.size() < 3) {
        status->setText(tr("Select at least three points to align."));
        return;
    }
    auto points = feature->Points.getValues();
    const auto origin = points[*selected.begin()];
    auto direction = points[*selected.rbegin()] - origin;
    double length = direction.Sqr();
    if (length < 1e-14) {
        status->setText(tr("The first and last selected points must differ."));
        return;
    }
    for (int i : selected) {
        points[i] = origin + direction * ((points[i] - origin).Dot(direction) / length);
    }
    applyPoints(points);
}

void FreehandBSplineEditor::setLinear(bool value)
{
    const int count = feature->Points.getSize();
    const int spans = feature->Periodic.getValue() ? count : count - 1;
    if (spans < 1) {
        return;
    }
    auto flags = feature->LinearSegments.getValues();
    flags.resize(spans, false);
    bool changed = false;
    for (int i = 0; i < spans; ++i) {
        if (selectedSegments.count(i)) {
            flags[i] = value;
            changed = true;
        }
    }
    if (!changed) {
        status->setText(tr("Select one or more segments."));
        return;
    }
    feature->LinearSegments.setValues(flags);
    applyPoints(feature->Points.getValues());
}

void FreehandBSplineEditor::insertPoint(const QPointF& position)
{
    if (feature->Points.getSize() < 2) {
        return;
    }
    try {
        auto curve = feature->interpolate();
        const double first = curve->FirstParameter(), last = curve->LastParameter();
        double parameter = first, distance = 100.0;
        const int samples = std::max(200, feature->Points.getSize() * 30);
        for (int i = 0; i < samples; ++i) {
            const double a = first + (last - first) * i / samples,
                         b = first + (last - first) * (i + 1) / samples;
            double t;
            double d = segmentDistance(
                position,
                screen(toWorld(vector(curve->Value(a)))),
                screen(toWorld(vector(curve->Value(b)))),
                t
            );
            if (d < distance) {
                distance = d;
                parameter = a + (b - a) * t;
            }
        }
        if (distance >= 100.0) {
            return;
        }
        auto points = feature->Points.getValues();
        Base::Vector3d added = vector(curve->Value(parameter));
        for (const auto& p : points) {
            if ((p - added).Length() < 1e-7) {
                return;
            }
        }
        size_t index = points.size();
        for (size_t i = 1; i < points.size(); ++i) {
            GeomAPI_ProjectPointOnCurve project(gp_Pnt(points[i].x, points[i].y, points[i].z), curve);
            if (project.LowerDistanceParameter() > parameter) {
                index = i;
                break;
            }
        }
        auto flags = feature->LinearSegments.getValues();
        flags.resize(feature->Periodic.getValue() ? points.size() : points.size() - 1, false);
        const bool splitLinear = index > 0 && index - 1 < flags.size() && flags[index - 1];
        const size_t insertion = std::min(index, flags.size());
        flags.resize(flags.size() + 1);
        for (size_t i = flags.size() - 1; i > insertion; --i) {
            flags[i] = flags[i - 1];
        }
        flags[insertion] = splitLinear;
        feature->LinearSegments.setValues(flags);
        std::vector<int> retained;
        for (int i = 0; i < static_cast<int>(points.size()); ++i) {
            retained.push_back(i);
        }
        retained.insert(retained.begin() + index, -1);
        feature->remapSupports(retained);
        points.insert(points.begin() + index, added);
        selectedPoints = {static_cast<int>(index)};
        selectedSegments.clear();
        applyPoints(points);
    }
    catch (const Standard_Failure&) {
    }
    catch (const Base::Exception&) {
    }
}

void FreehandBSplineEditor::cacheSnapShapes()
{
    snapVertices.clear();
    snapCurves.clear();
    const auto dependents = feature->getInListRecursive();
    for (auto object : feature->getDocument()->getObjects()) {
        if (object == feature
            || std::find(dependents.begin(), dependents.end(), object) != dependents.end()) {
            continue;
        }
        auto provider = vp->getDocument()->getViewProvider(object);
        if (!provider || !provider->isVisible()) {
            continue;
        }
        try {
            auto shape = Part::Feature::getShape(
                object,
                Part::ShapeOption::ResolveLink | Part::ShapeOption::Transform
            );
            if (shape.IsNull()) {
                continue;
            }
            // Unique indexed topology gives the same VertexN/EdgeN names as the property editor.
            TopTools_IndexedMapOfShape vertices, edges;
            TopExp::MapShapes(shape, TopAbs_VERTEX, vertices);
            TopExp::MapShapes(shape, TopAbs_EDGE, edges);
            const std::string objectName = object->getNameInDocument();
            for (int i = 1; i <= vertices.Extent(); ++i) {
                snapVertices.push_back(
                    {vector(BRep_Tool::Pnt(TopoDS::Vertex(vertices(i)))),
                     {objectName, "Vertex" + std::to_string(i)}}
                );
            }
            for (int i = 1; i <= edges.Extent(); ++i) {
                SnapCurve edge;
                edge.reference = {objectName, "Edge" + std::to_string(i)};
                edge.curve = BRep_Tool::Curve(TopoDS::Edge(edges(i)), edge.first, edge.last);
                if (!edge.curve.IsNull()) {
                    snapCurves.push_back(edge);
                }
            }
        }
        catch (const Standard_Failure&) {
        }
        catch (const Base::Exception&) {
        }
    }
}

bool FreehandBSplineEditor::snap(
    Base::Vector3d& point,
    const QPointF& position,
    SnapReference* reference
) const
{
    double best = 100.0;
    bool found = false;
    Base::Vector3d target = point;
    // Prefer a vertex to an edge within the same pixel tolerance.
    for (const auto& vertex : snapVertices) {
        double d = squareDistance(screen(vertex.point), position);
        if (d < best) {
            best = d;
            target = vertex.point;
            if (reference) {
                *reference = vertex.reference;
            }
            found = true;
        }
    }
    if (found) {
        point = target;
        return true;
    }
    for (const auto& edge : snapCurves) {
        // Screen-space sampling also finds edges in planes at a different depth.
        for (int i = 0; i < 40; ++i) {
            double a = edge.first + (edge.last - edge.first) * i / 40;
            double b = edge.first + (edge.last - edge.first) * (i + 1) / 40;
            double t;
            double d = segmentDistance(
                position,
                screen(vector(edge.curve->Value(a))),
                screen(vector(edge.curve->Value(b))),
                t
            );
            if (d < best) {
                best = d;
                target = vector(edge.curve->Value(a + (b - a) * t));
                if (reference) {
                    *reference = edge.reference;
                }
                found = true;
            }
        }
    }
    if (found) {
        point = target;
    }
    return found;
}

bool FreehandBSplineEditor::editCoordinateAt(const QPointF& position)
{
    if (drawing || !coordinates->isChecked() || hitPoint(position) >= 0) {
        return false;
    }
    // Pick the existing text without changing the displayed scene's unpickable state.
    auto root = std::unique_ptr<SoSeparator, void (*)(SoSeparator*)>(
        new SoSeparator,
        [](SoSeparator* node) { node->unref(); }
    );
    root->ref();
    auto pickStyle = new SoPickStyle;
    pickStyle->style = SoPickStyle::SHAPE;
    pickStyle->setOverride(true);
    root->addChild(pickStyle);
    root->addChild(viewer->getSoRenderManager()->getSceneGraph());
    SoRayPickAction pick(viewer->getSoRenderManager()->getViewportRegion());
    const auto ratio = viewer->getGLWidget()->devicePixelRatioF();
    pick.setPoint(SbVec2s(
        static_cast<short>(position.x() * ratio),
        static_cast<short>((viewer->getGLWidget()->height() - position.y()) * ratio)
    ));
    pick.setRadius(0);
    pick.setPickAll(true);
    pick.apply(root.get());
    const auto& hits = pick.getPickedPointList();
    for (int j = 0; j < hits.getLength(); ++j) {
        const auto hit = hits[j];
        const auto found
            = std::find(coordinateLabels.begin(), coordinateLabels.end(), hit->getPath()->getTail());
        if (found == coordinateLabels.end()) {
            continue;
        }
        const auto detail = dynamic_cast<const SoTextDetail*>(hit->getDetail());
        if (!detail || detail->getStringIndex() < 1 || detail->getStringIndex() > 3) {
            continue;
        }
        const int pointIndex = static_cast<int>(std::distance(coordinateLabels.begin(), found));
        // Keep coordinate clicks clear of the point handle and the label's leading spaces.
        constexpr double coordinateClickOffset = 14.0;
        const auto anchor = screen(toWorld(feature->Points.getValues()[pointIndex]));
        if (position.x() < anchor.x() + coordinateClickOffset) {
            continue;
        }
        finishCoordinateEdit(true);
        if (coordinateSpinBox) {
            return true;
        }
        coordinatePoint = pointIndex;
        coordinateAxis = detail->getStringIndex() - 1;
        auto spin = new Gui::QuantitySpinBox(viewer->getGLWidget());
        coordinateSpinBox = spin;
        spin->setObjectName(QStringLiteral("pointCoordinateEditor"));
        spin->setUnit(Base::Unit::Length);
        spin->setRange(-1e12, 1e12);
        spin->setValue(feature->Points.getValues()[coordinatePoint][coordinateAxis]);
        spin->setToolTip(tr("Point %1 - %2").arg(coordinatePoint + 1).arg(QChar('X' + coordinateAxis))
        );
        spin->resize(spin->sizeHint().expandedTo(QSize(140, 0)));
        const auto bounds = viewer->getGLWidget()->size();
        // Like EditableDatumLabel, center the editor at the label and clamp it to the view.
        spin->move(
            std::clamp(
                static_cast<int>(position.x()) - spin->width() / 2,
                0,
                std::max(0, bounds.width() - spin->width())
            ),
            std::clamp(
                static_cast<int>(position.y()) - spin->height() / 2,
                0,
                std::max(0, bounds.height() - spin->height())
            )
        );
        connect(spin, &QAbstractSpinBox::editingFinished, this, [this] {
            finishCoordinateEdit(true);
        });
        spin->show();
        spin->raise();
        spin->setFocus();
        spin->selectNumber();
        return true;
    }
    return false;
}

void FreehandBSplineEditor::finishCoordinateEdit(bool apply)
{
    if (!coordinateSpinBox) {
        return;
    }
    auto spin = coordinateSpinBox.data();
    if (apply && !spin->hasValidInput()) {
        return;
    }
    const int point = coordinatePoint;
    const int component = coordinateAxis;
    const double value = spin->rawValue();
    coordinateSpinBox = nullptr;
    coordinatePoint = coordinateAxis = -1;
    spin->hide();
    spin->deleteLater();
    if (apply && std::isfinite(value) && point >= 0 && point < feature->Points.getSize()) {
        auto points = feature->Points.getValues();
        points[point][component] = value;
        applyPoints(points);
    }
}

bool FreehandBSplineEditor::eventFilter(QObject* watched, QEvent* event)
{
    auto widget = qobject_cast<QWidget*>(watched);
    if (coordinateSpinBox && widget
        && (widget == coordinateSpinBox || coordinateSpinBox->isAncestorOf(widget))) {
        if (event->type() == QEvent::ShortcutOverride || event->type() == QEvent::KeyPress) {
            const int key = static_cast<QKeyEvent*>(event)->key();
            if (key == Qt::Key_Escape || key == Qt::Key_Return || key == Qt::Key_Enter) {
                event->accept();
                if (event->type() == QEvent::KeyPress) {
                    finishCoordinateEdit(key != Qt::Key_Escape);
                    if (!coordinateSpinBox && viewer) {
                        viewer->getGLWidget()->setFocus();
                    }
                }
                return true;
            }
        }
        return QWidget::eventFilter(watched, event);
    }
    const bool inPanel = widget && (widget == this || isAncestorOf(widget));
    const bool inViewer = widget && viewer && (widget == viewer || viewer->isAncestorOf(widget));
    const bool editingInput = (inPanel || inViewer) && !qobject_cast<QLineEdit*>(watched);
    if ((editingInput || (drawing && inPanel))
        && (event->type() == QEvent::ShortcutOverride || event->type() == QEvent::KeyPress)) {
        auto key = static_cast<QKeyEvent*>(event);
        const bool selectAll = !drawing && key->matches(QKeySequence::SelectAll);
        const bool remove = !drawing && key->key() == Qt::Key_Delete && !movingPoints().empty();
        const bool constraint = dragging
            && (key->key() == Qt::Key_X || key->key() == Qt::Key_Y || key->key() == Qt::Key_Z);
        const bool escape = key->key() == Qt::Key_Escape && (drawing || dragging);
        if (selectAll || remove || constraint || escape) {
            event->accept();
            if (event->type() == QEvent::ShortcutOverride) {
                return true;
            }
            if (selectAll) {
                selectAllPoints();
            }
            if (remove && !dragging) {
                deletePoints();
            }
            if (constraint) {
                const int requested = key->key() - Qt::Key_X;
                axis = axis == requested ? -1 : requested;
            }
            if (escape && drawing) {
                endDrawing();
                return true;
            }
            if (escape) {
                if (dragging) {
                    applyPoints(dragPoints);
                }
                dragging = false;
                axis = -1;
            }
            updateHints();
            return true;
        }
    }
    if (!viewer || watched != viewer->getGLWidget()) {
        return QWidget::eventFilter(watched, event);
    }
    if (event->type() == QEvent::Enter || event->type() == QEvent::FocusIn) {
        updateHints();
    }
    if (event->type() == QEvent::ContextMenu && suppressContextMenu) {
        suppressContextMenu = false;
        return true;
    }
    // Keep complete button sequences together: native navigation must receive the
    // release for every press passed through to it (including reference selection).
    if (event->type() == QEvent::MouseButtonRelease) {
        auto mouse = static_cast<QMouseEvent*>(event);
        if (!ownedButtons.testFlag(mouse->button())) {
            return false;
        }
        ownedButtons &= ~Qt::MouseButtons(mouse->button());
        if (mouse->button() == Qt::LeftButton) {
            if (dragging) {
                feature->getDocument()->recompute();
            }
            dragging = false;
            axis = -1;
            updateHints();
        }
        return true;
    }
    // The drawing handler owns pointer input until the user explicitly ends the spline.
    if (drawing) {
        if (event->type() == QEvent::ContextMenu) {
            return true;
        }
        if (event->type() == QEvent::MouseMove) {
            auto mouse = static_cast<QMouseEvent*>(event);
            if ((mouse->buttons() & ~ownedButtons) != Qt::NoButton) {
                return false;
            }
            updateDrawingPreview(mouse->position());
            return true;
        }
        if (event->type() == QEvent::Leave) {
            updateCurvePreview(feature->Points.getValues());
        }
        if (event->type() == QEvent::MouseButtonPress) {
            auto mouse = static_cast<QMouseEvent*>(event);
            if (mouse->button() == Qt::RightButton) {
                ownedButtons |= Qt::RightButton;
                suppressContextMenu = true;
                endDrawing();
                return true;
            }
            if (mouse->button() == Qt::LeftButton && !mouse->modifiers().testFlag(Qt::AltModifier)) {
                ownedButtons |= Qt::LeftButton;
                auto points = feature->Points.getValues();
                auto added = onViewPlane(
                    mouse->position(),
                    toWorld(points.empty() ? Base::Vector3d() : points.back())
                );
                SnapReference reference;
                if (snapping->isChecked()) {
                    cacheSnapShapes();
                    snap(added, mouse->position(), &reference);
                }
                added = toLocal(added);
                if (points.empty() || (points.back() - added).Length() > 1e-7) {
                    const auto previous = points;
                    points.push_back(added);
                    try {
                        if (!reference.objectName.empty()) {
                            auto object = feature->getDocument()->getObject(
                                reference.objectName.c_str()
                            );
                            if (!object) {
                                throw Base::ValueError("The snapped reference no longer exists.");
                            }
                            feature->Points.setValues(points);
                            feature->setPointSupport(
                                static_cast<int>(points.size()) - 1,
                                object,
                                reference.subname
                            );
                        }
                        applyPoints(points);
                    }
                    catch (const Base::Exception& error) {
                        applyPoints(previous);
                        status->setText(QString::fromUtf8(error.what()));
                    }
                    catch (const Standard_Failure& error) {
                        applyPoints(previous);
                        status->setText(QString::fromUtf8(error.GetMessageString()));
                    }
                }
                updateCurvePreview(feature->Points.getValues());
                return true;
            }
        }
        if (event->type() == QEvent::MouseButtonDblClick) {
            auto mouse = static_cast<QMouseEvent*>(event);
            if (mouse->button() == Qt::LeftButton) {
                ownedButtons |= Qt::LeftButton;
                return true;
            }
            return false;
        }
    }
    if (event->type() == QEvent::Leave || event->type() == QEvent::MouseMove) {
        int point = -1;
        int segment = -1;
        if (event->type() == QEvent::MouseMove) {
            auto mouse = static_cast<QMouseEvent*>(event);
            if (!drawing && !dragging && mouse->buttons() == Qt::NoButton) {
                point = hitPoint(mouse->position());
                segment = point < 0 ? hitSegment(mouse->position()) : -1;
            }
        }
        if (point != hoveredPoint || segment != hoveredSegment) {
            hoveredPoint = point;
            hoveredSegment = segment;
            refresh();
        }
    }
    if (event->type() == QEvent::MouseButtonDblClick) {
        auto mouse = static_cast<QMouseEvent*>(event);
        if (mouse->button() == Qt::LeftButton) {
            ownedButtons |= Qt::LeftButton;
            dragging = false;
            insertPoint(mouse->position());
            updateHints();
            return true;
        }
    }
    if (event->type() == QEvent::MouseButtonPress) {
        auto mouse = static_cast<QMouseEvent*>(event);
        if (mouse->button() != Qt::LeftButton || mouse->modifiers().testFlag(Qt::AltModifier)) {
            return false;
        }
        if (editCoordinateAt(mouse->position())) {
            ownedButtons |= Qt::LeftButton;
            return true;
        }
        viewer->getGLWidget()->setFocus();
        const auto position = mouse->position();
        int point = hitPoint(position);
        int segment = point < 0 ? hitSegment(position) : -1;
        if (point < 0 && segment < 0) {
            const auto ratio = viewer->getGLWidget()->devicePixelRatioF();
            std::unique_ptr<SoPickedPoint> picked(viewer->pickPoint(SbVec2s(
                static_cast<short>(position.x() * ratio),
                static_cast<short>((viewer->getGLWidget()->height() - position.y()) * ratio)
            )));
            if (!picked && !mouse->modifiers().testFlag(Qt::ControlModifier)) {
                selectedPoints.clear();
                selectedSegments.clear();
                refresh();
            }
            return false;  // Native selection handles external references and empty space.
        }
        ownedButtons |= Qt::LeftButton;
        const bool control = mouse->modifiers().testFlag(Qt::ControlModifier);
        const bool already = point >= 0 ? selectedPoints.count(point)
                                        : selectedSegments.count(segment);
        if (!control && !already) {
            selectedPoints.clear();
            selectedSegments.clear();
        }
        if (point >= 0) {
            if (control && already) {
                selectedPoints.erase(point);
            }
            else {
                selectedPoints.insert(point);
            }
        }
        if (segment >= 0) {
            if (control && already) {
                selectedSegments.erase(segment);
            }
            else {
                selectedSegments.insert(segment);
            }
        }
        auto moving = movingPoints();
        dragging = (point >= 0 || segment >= 0) && !moving.empty() && !(control && already);
        if (dragging) {
            dragPoints = feature->Points.getValues();
            dragAnchor = point >= 0 ? point : segment;
            dragOrigin = onViewPlane(position, toWorld(dragPoints[dragAnchor]));
            pressPosition = position;
            axis = -1;
            cacheSnapShapes();
        }
        refresh();
        return true;
    }
    if (event->type() == QEvent::MouseMove && dragging) {
        auto mouse = static_cast<QMouseEvent*>(event);
        if (!mouse->buttons().testFlag(Qt::LeftButton)) {
            dragging = false;
            ownedButtons &= ~Qt::MouseButtons(Qt::LeftButton);
            axis = -1;
            updateHints();
            return false;
        }
        auto delta = onViewPlane(mouse->position(), dragOrigin) - dragOrigin;
        if (axis >= 0) {
            Base::Vector3d unit;
            unit[axis] = 1.0;
            auto origin = screen(dragOrigin), direction = screen(dragOrigin + unit) - origin;
            auto offset = mouse->position() - pressPosition;
            double length = direction.x() * direction.x() + direction.y() * direction.y();
            delta = unit
                * (length > 1e-12 ? (offset.x() * direction.x() + offset.y() * direction.y()) / length
                                  : 0);
        }
        const bool fine = mouse->modifiers().testFlag(Qt::ShiftModifier);
        if (fine) {
            delta *= 0.1;
        }
        if (snapping->isChecked() && !fine && axis < 0) {
            auto candidate = toWorld(dragPoints[dragAnchor]) + delta;
            if (snap(candidate, screen(candidate))) {
                delta = candidate - toWorld(dragPoints[dragAnchor]);
            }
        }
        auto points = dragPoints;
        for (int i : movingPoints()) {
            points[i] = toLocal(toWorld(dragPoints[i]) + delta);
        }
        applyPoints(points);
        return true;
    }
    return QWidget::eventFilter(watched, event);
}
