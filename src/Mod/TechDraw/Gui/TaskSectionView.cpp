/***************************************************************************
 *   Copyright (c) 2016 WandererFan <wandererfan@gmail.com>                *
 *                                                                         *
 *   This file is part of the FreeCAD CAx development system.              *
 *                                                                         *
 *   This library is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU Library General Public           *
 *   License as published by the Free Software Foundation; either          *
 *   version 2 of the License, or (at your option) any later version.      *
 *                                                                         *
 *   This library  is distributed in the hope that it will be useful,      *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU Library General Public License for more details.                  *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this library; see the file COPYING.LIB. If not,    *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,         *
 *   Suite 330, Boston, MA  02111-1307, USA                                *
 *                                                                         *
 ***************************************************************************/

#include <QMessageBox>
#include <QMenu>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QPainter>
#include <QPainterPathStroker>
#include <QPushButton>
#include <QGraphicsItem>
#include <QGraphicsEllipseItem>
#include <QGraphicsPathItem>
#include <QGraphicsScene>
#include <QGraphicsSceneMouseEvent>
#include <QIcon>
#include <QSignalBlocker>
#include <QTimer>
#include <TopExp_Explorer.hxx>
#include <TopoDS.hxx>
#include <algorithm>
#include <array>
#include <cmath>
#include <functional>
#include <iterator>
#include <limits>
#include <numbers>
#include <sstream>

#include "Widgets/CompassWidget.h"
#include "Widgets/VectorEditWidget.h"
#include <App/Application.h>
#include <App/Document.h>
#include <App/Link.h>
#include <App/DocumentObject.h>
#include <App/PropertyStandard.h>
#include <Base/Console.h>
#include <Base/Tools.h>
#include <Gui/BitmapFactory.h>
#include <Gui/Application.h>
#include <Gui/Command.h>
#include <Gui/Control.h>
#include <Gui/InputHint.h>
#include <Gui/MainWindow.h>
#include <Gui/Selection/Selection.h>
#include <Gui/Selection/Selection.h>
#include <Gui/ViewProvider.h>
#include <Gui/WaitCursor.h>
#include <Mod/TechDraw/App/DrawPage.h>
#include <Mod/TechDraw/App/DrawComplexSection.h>
#include <Mod/TechDraw/App/DrawViewPart.h>
#include <Mod/TechDraw/App/DrawViewSection.h>
#include <Mod/TechDraw/App/GeometryObject.h>
#include <Mod/TechDraw/App/LineGenerator.h>
#include <Mod/TechDraw/App/Preferences.h>

// clang-format off
#include "ui_TaskSectionView.h"
#include "DrawGuiUtil.h"
#include "QGIEdge.h"
#include "QGIFace.h"
#include "QGISectionLine.h"
#include "QGIViewPart.h"
#include "QGIVertex.h"
#include "MDIViewPage.h"
#include "PathBuilder.h"
#include "QGSPage.h"
#include "QGVPage.h"
#include "Rez.h"
#include "TaskSectionView.h"
#include "TechDrawHandler.h"
#include "ViewProviderViewPart.h"
#include "ViewProviderPage.h"
// clang-format on


using namespace Gui;
using namespace TechDraw;
using namespace TechDrawGui;

SceneItemDeleter::SceneItemDeleter(QGraphicsScene* scene) :
    m_scene(scene)
{}

void SceneItemDeleter::operator()(QGraphicsItem* item) const
{
    if (m_scene) {
        delete item;
    }
}

namespace
{

template<typename StoredItem, typename ConcreteItem = StoredItem, typename... Args>
SceneItemPtr<StoredItem> makeSceneItem(QGraphicsScene& scene, Args&&... args)
{
    SceneItemPtr<StoredItem> item(
        new ConcreteItem(std::forward<Args>(args)...),
        SceneItemDeleter(&scene));
    scene.addItem(item.get());
    return item;
}

struct ArcSample
{
    double along{0.0};
    double offset{0.0};
};

std::vector<ArcSample> tessellateQuarterArc(double startAlong,
                                            double endAlong,
                                            double startOffset,
                                            double endOffset)
{
    constexpr std::size_t segmentCount = 10;
    std::vector<ArcSample> result;
    result.reserve(segmentCount + 1);
    result.push_back({startAlong, startOffset});
    const double radius = std::abs(endAlong - startAlong);
    const double alongSign = endAlong >= startAlong ? 1.0 : -1.0;
    const double offsetSign = endOffset >= startOffset ? 1.0 : -1.0;
    for (std::size_t segment = 1; segment <= segmentCount; ++segment) {
        const double ratio = static_cast<double>(segment) / segmentCount;
        const double angle = ratio * std::numbers::pi / 2.0;
        result.push_back({
            startAlong + alongSign * radius * std::sin(angle),
            startOffset + offsetSign * radius * (1.0 - std::cos(angle))});
    }
    return result;
}

class SectionPointMarker
{
public:
    void showAt(QGVPage& page,
                const QGIViewPart& baseItem,
                const QPointF& localPoint)
    {
        if (!m_item) {
            constexpr double markerRadius = 4.0;
            m_item = makeSceneItem<QGraphicsEllipseItem>(
                *page.getScene(),
                -markerRadius, -markerRadius,
                markerRadius * 2.0, markerRadius * 2.0);
            m_item->setPen(QPen(Qt::black));
            m_item->setBrush(Qt::black);
            m_item->setZValue(1001.0);
            m_item->setFlag(QGraphicsItem::ItemIgnoresTransformations);
        }
        m_item->setPos(baseItem.mapToScene(localPoint));
    }

private:
    SceneItemPtr<QGraphicsEllipseItem> m_item;
};

class SectionOffsetHandler : public TechDrawHandler
{
protected:
    explicit SectionOffsetHandler(TaskSectionView* task) : m_task(task) {}

    bool snappedLocalPoint(QMouseEvent* event, QPointF& local) const
    {
        if (m_completed) {
            event->accept();
            return false;
        }
        QGIViewPart* baseItem = m_task ? m_task->baseItem() : nullptr;
        if (!viewPage || !baseItem) {
            return false;
        }
        local = baseItem->mapFromScene(viewPage->mapToScene(event->pos()));
        local = m_task->snapViewPoint(
            local, event->modifiers().testFlag(Qt::ControlModifier));
        return true;
    }

    bool acceptsLeftPress(const QMouseEvent* event) const
    {
        return event->button() == Qt::LeftButton && m_task
            && m_task->baseItem() && viewPage && !m_completed;
    }

    void completeAndApply()
    {
        m_completed = true;
        QPointer<QGVPage> page = viewPage;
        QPointer<TaskSectionView> task = m_task;
        QTimer::singleShot(0, viewPage, [page, task]() {
            if (!page) {
                return;
            }
            page->deactivateHandler();
            if (task) {
                QTimer::singleShot(0, task, [task]() {
                    if (task) {
                        task->apply();
                    }
                });
            }
        });
    }

    void updateMarker(const QPointF& localPoint)
    {
        if (viewPage && m_task && m_task->baseItem()) {
            m_pointMarker.showAt(
                *viewPage, *m_task->baseItem(), localPoint);
        }
    }

    void mouseReleaseEvent(QMouseEvent* event) override
    {
        if (event->button() == Qt::LeftButton && viewPage) {
            viewPage->getScene()->clearSelection();
            Gui::Selection().clearSelection();
            event->accept();
            return;
        }
        TechDrawHandler::mouseReleaseEvent(event);
    }

    void keyPressEvent(QKeyEvent* event) override
    {
        Q_UNUSED(event);
    }

    void deactivate() override
    {
        if (!m_completed && m_task) {
            cancelPending();
        }
        if (m_task) {
            m_task->setSectionControlsEnabled(true);
            m_task->showSectionHandles();
        }
        TechDrawHandler::deactivate();
    }

    QString getCrosshairCursorSVGName() const override
    {
        return QStringLiteral("TechDraw_SectionView_Pointer");
    }

    virtual void cancelPending() = 0;

    TaskSectionView* m_task{nullptr};
    bool m_completed{false};

private:
    SectionPointMarker m_pointMarker;
};

class SectionDragHandle final : public QGraphicsEllipseItem
{
public:
    using MoveCallback =
        std::function<void(const QPointF&, Qt::KeyboardModifiers)>;

    SectionDragHandle(const QColor& color, MoveCallback callback) :
        QGraphicsEllipseItem(-4.0, -4.0, 8.0, 8.0),
        m_callback(std::move(callback))
    {
        setPen(QPen(color, 1.5));
        setBrush(color);
        setZValue(1002.0);
        setFlag(QGraphicsItem::ItemIgnoresTransformations);
        setAcceptedMouseButtons(Qt::LeftButton);
        setCursor(Qt::OpenHandCursor);
    }

protected:
    void mousePressEvent(QGraphicsSceneMouseEvent* event) override
    {
        setCursor(Qt::ClosedHandCursor);
        event->accept();
    }

    void mouseMoveEvent(QGraphicsSceneMouseEvent* event) override
    {
        if (m_callback) {
            m_callback(event->scenePos(), event->modifiers());
        }
        event->accept();
    }

    void mouseReleaseEvent(QGraphicsSceneMouseEvent* event) override
    {
        if (m_callback) {
            m_callback(event->scenePos(), event->modifiers());
        }
        setCursor(Qt::OpenHandCursor);
        event->accept();
    }

private:
    MoveCallback m_callback;
};

class SelectableOffsetEdge final : public QGraphicsPathItem
{
public:
    explicit SelectableOffsetEdge(
        std::function<void()> deleteCallback,
        const QString& toolTip =
            QObject::tr("Select and press Delete to remove this offset")) :
        m_deleteCallback(std::move(deleteCallback))
    {
        QPen pen(QColor(35, 95, 210), 2.0, Qt::SolidLine,
                 Qt::RoundCap, Qt::RoundJoin);
        pen.setCosmetic(true);
        setPen(pen);
        setZValue(1001.0);
        setFlag(QGraphicsItem::ItemIsSelectable);
        setFlag(QGraphicsItem::ItemIsFocusable);
        setAcceptedMouseButtons(Qt::LeftButton);
        setCursor(Qt::PointingHandCursor);
        setToolTip(toolTip);
    }

    QPainterPath shape() const override
    {
        QPainterPathStroker stroker;
        stroker.setWidth(Rez::guiX(2.0));
        stroker.setCapStyle(Qt::RoundCap);
        stroker.setJoinStyle(Qt::RoundJoin);
        return stroker.createStroke(path());
    }

protected:
    void paint(QPainter* painter,
               const QStyleOptionGraphicsItem* option,
               QWidget* widget) override
    {
        if (isSelected()) {
            QGraphicsPathItem::paint(painter, option, widget);
        }
    }

    void mousePressEvent(QGraphicsSceneMouseEvent* event) override
    {
        QGraphicsPathItem::mousePressEvent(event);
        setFocus();
        event->accept();
    }

    void keyPressEvent(QKeyEvent* event) override
    {
        if (event->key() == Qt::Key_Delete
            || event->key() == Qt::Key_Backspace) {
            if (m_deleteCallback) {
                m_deleteCallback();
            }
            event->accept();
            return;
        }
        QGraphicsPathItem::keyPressEvent(event);
    }

private:
    std::function<void()> m_deleteCallback;
};

class SectionAddOffsetButton final : public QGraphicsEllipseItem
{
public:
    SectionAddOffsetButton(std::function<void()> simpleCallback,
                           std::function<void()> notchCallback,
                           std::function<void()> arcCallback,
                           std::function<void()> arcNotchCallback) :
        QGraphicsEllipseItem(-12.0, -12.0, 24.0, 24.0),
        m_callbacks{std::move(simpleCallback), std::move(notchCallback),
                    std::move(arcCallback), std::move(arcNotchCallback)}
    {
        setPen(QPen(QColor(40, 40, 40, 120), 1.0));
        setBrush(QColor(255, 255, 255, 215));
        setZValue(1003.0);
        setFlag(QGraphicsItem::ItemIgnoresTransformations);
        setAcceptedMouseButtons(Qt::LeftButton);
        setCursor(Qt::PointingHandCursor);
        setToolTip(QObject::tr("Add an offset to this section-line segment"));
    }

protected:
    void paint(QPainter* painter,
               const QStyleOptionGraphicsItem* option,
               QWidget* widget) override
    {
        QGraphicsEllipseItem::paint(painter, option, widget);
        painter->save();
        painter->setRenderHint(QPainter::Antialiasing);
        painter->setPen(QPen(QColor(29, 132, 72), 3.0,
                             Qt::SolidLine, Qt::RoundCap));
        painter->drawLine(QPointF(0.0, -5.0), QPointF(0.0, 5.0));
        painter->drawLine(QPointF(-5.0, 0.0), QPointF(5.0, 0.0));
        painter->restore();
    }

    void mousePressEvent(QGraphicsSceneMouseEvent* event) override
    {
        event->accept();
    }

    void mouseReleaseEvent(QGraphicsSceneMouseEvent* event) override
    {
        if (!contains(event->pos())) {
            event->accept();
            return;
        }
        const QPoint screenPosition = event->screenPos();
        const auto callbacks = m_callbacks;
        ungrabMouse();
        QTimer::singleShot(
            0, Gui::getMainWindow(),
            [screenPosition, callbacks]() {
                auto* menu = new QMenu(Gui::getMainWindow());
                const std::array<const char*, 4> icons{
                    "actions/section-offset-simple",
                    "actions/section-offset-notch",
                    "actions/section-offset-arc",
                    "actions/section-offset-arc-notch"};
                const std::array<QString, 4> labels{
                    QObject::tr("Simple offset"),
                    QObject::tr("Notched offset"),
                    QObject::tr("Arc offset"),
                    QObject::tr("Arc notch")};
                for (size_t i = 0; i < callbacks.size(); ++i) {
                    QAction* action = menu->addAction(
                        Gui::BitmapFactory().iconFromTheme(icons[i]),
                        labels[i]);
                    const auto callback = callbacks[i];
                    QObject::connect(
                        action, &QAction::triggered, menu,
                        [callback]() {
                            if (callback) {
                                callback();
                            }
                        });
                }
                QObject::connect(
                    menu, &QMenu::aboutToHide,
                    menu, &QObject::deleteLater);
                menu->popup(screenPosition);
            }
        );
        event->accept();
    }

private:
    std::array<std::function<void()>, 4> m_callbacks;
};

class SectionRotationHandle final : public QGraphicsPathItem
{
public:
    using MoveCallback =
        std::function<void(const QPointF&, Qt::KeyboardModifiers, bool)>;
    using VisibilityCallback = std::function<void(bool)>;

    SectionRotationHandle(MoveCallback callback,
                          VisibilityCallback visibilityCallback) :
        m_callback(std::move(callback)),
        m_visibilityCallback(std::move(visibilityCallback))
    {
        QPen pen(QColor(35, 95, 210), 2.0, Qt::SolidLine,
                 Qt::RoundCap, Qt::RoundJoin);
        pen.setCosmetic(true);
        setPen(pen);
        setZValue(1003.0);
        setFlag(QGraphicsItem::ItemIgnoresTransformations);
        setAcceptedMouseButtons(Qt::LeftButton);
        setCursor(Qt::OpenHandCursor);
        setToolTip(QObject::tr(
            "Drag to rotate this half of the section line; hold Ctrl to snap the angle"));
    }

    ~SectionRotationHandle() override
    {
        setFrameVisible(false);
    }

    void setRotationArc(const QPointF& center,
                        const QPointF& endpoint,
                        bool startHalf)
    {
        const QPointF centerToEndpoint = endpoint - center;
        const double endpointDistance =
            std::hypot(centerToEndpoint.x(), centerToEndpoint.y());
        if (endpointDistance < 1.0e-6) {
            setPath({});
            return;
        }
        const QPointF radial =
            centerToEndpoint / endpointDistance;
        const double outsideDistance = endpointDistance * 1.05;
        // This item ignores view transformations, so its local geometry is in
        // screen-space units. Keep the rotation glyph a constant visual size
        // as an endpoint is shortened or extended.
        constexpr double glyphRadius = 200.0;
        const QPointF arcCenter = -radial * glyphRadius;
        const QPointF middleRadius = radial * glyphRadius;
        const double halfSweep = Base::toRadians(8.0);
        auto rotate = [](const QPointF& point, double angle) {
            return QPointF(point.x() * std::cos(angle)
                               - point.y() * std::sin(angle),
                           point.x() * std::sin(angle)
                               + point.y() * std::cos(angle));
        };

        QPainterPath arrow;
        constexpr int arcSegments = 6;
        auto addBranch = [&](double sweep) {
            QPointF previous;
            QPointF penultimate;
            arrow.moveTo(QPointF());
            for (int i = 1; i <= arcSegments; ++i) {
                const double angle =
                    sweep * static_cast<double>(i) / arcSegments;
                penultimate = previous;
                previous = arcCenter + rotate(middleRadius, angle);
                arrow.lineTo(previous);
            }
            QPointF tangent = previous - penultimate;
            tangent /= std::hypot(tangent.x(), tangent.y());
            const QPointF perpendicular(-tangent.y(), tangent.x());
            constexpr double headLength = 8.0 / 3.0;
            constexpr double headWidth = 4.0 / 3.0;
            arrow.moveTo(previous - tangent * headLength
                         + perpendicular * headWidth);
            arrow.lineTo(previous);
            arrow.lineTo(previous - tangent * headLength
                         - perpendicular * headWidth);
        };
        const double orientation = startHalf ? -1.0 : 1.0;
        addBranch(orientation * halfSweep);
        addBranch(-orientation * halfSweep);
        setPos(center + radial * outsideDistance);
        setPath(arrow);
    }

protected:
    void mousePressEvent(QGraphicsSceneMouseEvent* event) override
    {
        setFrameVisible(true);
        setCursor(Qt::ClosedHandCursor);
        event->accept();
    }

    void mouseMoveEvent(QGraphicsSceneMouseEvent* event) override
    {
        if (m_callback) {
            m_callback(event->scenePos(), event->modifiers(), false);
        }
        event->accept();
    }

    void mouseReleaseEvent(QGraphicsSceneMouseEvent* event) override
    {
        if (m_callback) {
            m_callback(event->scenePos(), event->modifiers(), true);
        }
        setFrameVisible(false);
        setCursor(Qt::OpenHandCursor);
        event->accept();
    }

private:
    void setFrameVisible(bool visible)
    {
        if (m_frameVisible == visible) {
            return;
        }
        m_frameVisible = visible;
        if (m_visibilityCallback) {
            m_visibilityCallback(visible);
        }
    }

    MoveCallback m_callback;
    VisibilityCallback m_visibilityCallback;
    bool m_frameVisible{false};
};

std::pair<double, double> sectionLineBounds(QGIViewPart* baseItem,
                                            const QPointF& center,
                                            const Base::Vector3d& direction)
{
    if (!baseItem) {
        const double fallback = Rez::guiX(25.0);
        return {-fallback, fallback};
    }

    QPointF tangent(-direction.y, -direction.x);
    const double tangentLength = std::hypot(tangent.x(), tangent.y());
    if (tangentLength <= std::numeric_limits<double>::epsilon()) {
        tangent = QPointF(0.0, -1.0);
    }
    else {
        tangent /= tangentLength;
    }

    double startAlong = std::numeric_limits<double>::max();
    double endAlong = std::numeric_limits<double>::lowest();
    auto addIntersection = [&](const QPointF& point) {
        const double along = QPointF::dotProduct(
            point - center, tangent);
        startAlong = std::min(startAlong, along);
        endAlong = std::max(endAlong, along);
    };

    const QPointF normal(-tangent.y(), tangent.x());
    constexpr double intersectionTolerance = 1.0e-6;
    auto* base = dynamic_cast<DrawViewPart*>(baseItem->getViewObject());
    if (base) {
        PathBuilder pathBuilder(baseItem);
        for (const BaseGeomPtr& geometry : base->getEdgeGeometry()) {
            if (!geometry
                || geometry->source() != SourceType::GEOMETRY) {
                continue;
            }
            const QPainterPath edgePath =
                pathBuilder.geomToPainterPath(geometry, 0.0);
            for (const QPolygonF& polyline :
                 edgePath.toSubpathPolygons()) {
                for (qsizetype i = 1; i < polyline.size(); ++i) {
                    const QPointF first = polyline[i - 1];
                    const QPointF second = polyline[i];
                    const double firstDistance = QPointF::dotProduct(
                        first - center, normal);
                    const double secondDistance = QPointF::dotProduct(
                        second - center, normal);
                    if (std::abs(firstDistance)
                            <= intersectionTolerance
                        && std::abs(secondDistance)
                            <= intersectionTolerance) {
                        addIntersection(first);
                        addIntersection(second);
                        continue;
                    }
                    if ((firstDistance < -intersectionTolerance
                         && secondDistance < -intersectionTolerance)
                        || (firstDistance > intersectionTolerance
                            && secondDistance > intersectionTolerance)) {
                        continue;
                    }
                    const double denominator =
                        firstDistance - secondDistance;
                    if (std::abs(denominator)
                        <= intersectionTolerance) {
                        continue;
                    }
                    const double ratio = std::clamp(
                        firstDistance / denominator, 0.0, 1.0);
                    addIntersection(
                        first + (second - first) * ratio);
                }
            }
        }
    }

    // If the cutting line misses all projected geometry, retain the former
    // rectangular fallback so the interactive preview remains usable.
    if (startAlong > endAlong) {
        QRectF bounds = baseItem->contentBoundingRect();
        if (bounds.isEmpty()) {
            bounds = baseItem->boundingRect();
        }
        const std::array<QPointF, 4> corners{
            bounds.topLeft(), bounds.topRight(),
            bounds.bottomLeft(), bounds.bottomRight()};
        for (const QPointF& corner : corners) {
            addIntersection(corner);
        }
    }

    const double padding = Rez::guiX(10.0);
    startAlong -= padding;
    endAlong += padding;

    // A center selected close to the frame must remain on the displayed line.
    startAlong = std::min(startAlong, -Rez::guiX(1.0));
    endAlong = std::max(endAlong, Rez::guiX(1.0));
    return {startAlong, endAlong};
}

}  // namespace

TaskSectionView::~TaskSectionView()
{
    clearSectionControls();
}

// ctor for interactive create without a preselected base view
TaskSectionView::TaskSectionView(TechDraw::DrawPage* page, QGVPage* graphicsView) :
    ui(new Ui_TaskSectionView),
    m_base(nullptr),
    m_section(nullptr),
    m_dirName("Aligned"),
    m_doc(page ? page->getDocument() : nullptr),
    m_page(page),
    m_graphicsView(graphicsView),
    m_createMode(true),
    m_scaleEdited(false),
    m_directionChanged(false)
{
    m_sectionName = std::string();
    if (m_page) {
        m_savePageName = m_page->getNameInDocument();
    }
    m_transactionId = Gui::Command::openActiveDocumentCommand(
        QT_TRANSLATE_NOOP("Command", "Create Section View"));

    ui->setupUi(this);
    setUiPrimary();
    setPositioningActive(true);
}

//ctor for create
TaskSectionView::TaskSectionView(TechDraw::DrawViewPart* base) :
    ui(new Ui_TaskSectionView),
    m_base(base),
    m_section(nullptr),
    m_dirName("Aligned"),
    m_doc(nullptr),
    m_page(base ? base->findParentPage() : nullptr),
    m_graphicsView(nullptr),
    m_createMode(true),
    m_scaleEdited(false),
    m_directionChanged(false)
{
    //existence of base is guaranteed by CmdTechDrawSectionView (Command.cpp)

    m_sectionName = std::string();
    m_doc = m_base->getDocument();

    m_saveBaseName = m_base->getNameInDocument();
    m_savePageName = m_base->findParentPage()->getNameInDocument();
    m_transactionId = Gui::Command::openActiveDocumentCommand(
        QT_TRANSLATE_NOOP("Command", "Create Section View"));

    ui->setupUi(this);
    setUiPrimary();
}

//ctor for edit
TaskSectionView::TaskSectionView(TechDraw::DrawViewSection* section) :
    ui(new Ui_TaskSectionView),
    m_base(nullptr),
    m_section(section),
    m_doc(nullptr),
    m_page(section ? section->findParentPage() : nullptr),
    m_graphicsView(nullptr),
    m_createMode(false),
    m_scaleEdited(false),
    m_directionChanged(false)
{
    //existence of section is guaranteed by ViewProviderViewSection.setEdit

    m_doc = m_section->getDocument();
    m_sectionName = m_section->getNameInDocument();
    App::DocumentObject* newObj = m_section->BaseView.getValue();
    m_base = dynamic_cast<TechDraw::DrawViewPart*>(newObj);
    if (!newObj || !m_base) {
        throw Base::RuntimeError("TaskSectionView - BaseView not found");
    }

    m_saveBaseName = m_base->getNameInDocument();
    m_savePageName = m_base->findParentPage()->getNameInDocument();
    m_transactionId = Gui::Command::openActiveDocumentCommand(
        QT_TRANSLATE_NOOP("Command", "Edit Section View"));

    ui->setupUi(this);

    m_dirName = m_section->SectionDirection.getValueAsString();
    m_shapes = m_section->Source.getValues();
    m_xShapes = m_section->XSource.getValues();
    if (auto* complexSection =
            dynamic_cast<TechDraw::DrawComplexSection*>(m_section)) {
        m_profileObject = complexSection->CuttingToolWireObject.getValue();
        if (m_profileObject) {
            m_profileName = m_profileObject->getNameInDocument();
        }
        m_customComplexSection = !isGeneratedProfile(m_profileObject);
    }
    setUiEdit();
    if (isGeneratedProfile(m_profileObject)) {
        restoreModernProfileState();
    }
    updateParallelToVisibility();
}

void TaskSectionView::setUiPrimary()
{
    setWindowTitle(QObject::tr("Create Section View"));

    QPointF center;
    if (m_base) {
        ui->sbScale->setValue(m_base->getScale());
        ui->cmbScaleType->setCurrentIndex(m_base->getScaleType());
    }
    else if (m_page) {
        ui->sbScale->setValue(m_page->Scale.getValue());
        ui->cmbScaleType->setCurrentIndex(0);
    }
    setUiCommon(center);

    if (m_page && ui->leSymbol->text().isEmpty()) {
        ui->leSymbol->setText(
            QString::fromStdString(m_page->getNextViewIdentifier(false)));
    }

    m_compass->setDialAngle(0.0);
    m_viewDirectionWidget->setValueNoNotify(Base::Vector3d(1.0, 0.0, 0.0));
    setInteractiveBase(m_base);
    updateParallelToVisibility();
}

void TaskSectionView::setUiEdit()
{
    setWindowTitle(QObject::tr("Edit Section View"));
    std::string temp = m_section->SectionSymbol.getValue();
    QString qTemp = QString::fromStdString(temp);
    ui->leSymbol->setText(qTemp);

    ui->sbScale->setValue(m_section->getScale());
    ui->cmbScaleType->setCurrentIndex(m_section->getScaleType());
    ui->cbSectionCutOnly->setChecked(
        m_section->SectionCutOnly.getValue());
    ui->cbShowOutsidePartialBoundaries->setChecked(
        m_section->ShowOutsidePartialBoundaries.getValue());
    ui->cbConnectionLine->setChecked(
        m_section->ConnectionLine.getValue());
    //Allow or prevent scale changing initially
    if (m_section->ScaleType.isValue("Custom")) {
        ui->sbScale->setEnabled(true);
    }
    else {
        ui->sbScale->setEnabled(false);
    }

    setUiCommon(sectionOriginToView(
        m_section->SectionOrigin.getValue()));

    // convert section normal to view angle
    Base::Vector3d sectionNormalVec = m_section->SectionNormal.getValue();
    sectionNormalVec.Normalize();
    Base::Vector3d projectedViewDirection = m_base->projectPoint(sectionNormalVec, false);
    projectedViewDirection.Normalize();
    double viewAngle = atan2(-projectedViewDirection.y, -projectedViewDirection.x);
    m_compass->setDialAngle(Base::toDegrees(viewAngle));
    // The direction widget and baseToDisplayedDirection() both operate in
    // BaseView coordinates.  SectionNormal is stored in global coordinates,
    // so feeding it directly to the widget corrupts non-standard views every
    // time the task is reopened and accepted.
    m_viewDirectionWidget->setValueNoNotify(projectedViewDirection * -1.0);

    if (auto* complexSection =
            dynamic_cast<TechDraw::DrawComplexSection*>(m_section)) {
        ui->cmbStrategy->setCurrentIndex(
            static_cast<int>(complexSection->ProjectionStrategy.getValue()));
        {
            const QSignalBlocker blocker(ui->cmbParallelTo);
            ui->cmbParallelTo->setCurrentIndex(static_cast<int>(
                complexSection->SectionViewAlignment.getValue()));
        }
        ui->leSectionObjects->setText(sourcesToString());
        setProfileObject(m_profileObject);
        setCustomComplexSection(m_customComplexSection);
    }

    // setUiCommon() starts with the controls disabled because creation mode
    // may still be waiting for a base view.  An edit task already has a valid
    // base, so restore the normal enabled state immediately.
    setInteractiveBase(m_base);
}

void TaskSectionView::setUiCommon(const QPointF& center)
{
    ui->gbCustomComplexSection->hide();
    ui->parallelToWidget->hide();
    ui->tbSectionSketchBased->setIcon(QIcon(QString::fromStdString(
        App::Application::getResourceDir()
        + "Mod/Sketcher/Resources/icons/SketcherWorkbench.svg")));
    ui->leSectionObjects->setText(sourcesToString());

    ui->sbCenterX->setUnit(Base::Unit::Length);
    ui->sbCenterY->setUnit(Base::Unit::Length);
    setCenterUi(center);

    const bool showManualControls =
        Preferences::getPreferenceGroup("General")->GetBool(
            "ShowSectionViewManualControls", false);
    ui->cbShowManualControls->setChecked(showManualControls);
    updateManualControlsVisibility();

    enableAll(false);

    connect(ui->leSymbol, &QLineEdit::editingFinished, this, &TaskSectionView::onIdentifierChanged);

    //TODO: use event filter instead of keyboard tracking to capture enter/return keys
    // the UI file uses keyboardTracking = false so that a recomputation
    // will only be triggered when the arrow keys of the spinboxes are used
    //if this is not done, recomputes are triggered on each key press giving
    //unaccceptable UX
    connect(ui->sbScale, qOverload<double>(&QDoubleSpinBox::valueChanged), this, &TaskSectionView::onScaleChanged);
    connect(ui->sbCenterX, qOverload<double>(&QuantitySpinBox::valueChanged),
            this, &TaskSectionView::onCenterXChanged);
    connect(ui->sbCenterY, qOverload<double>(&QuantitySpinBox::valueChanged),
            this, &TaskSectionView::onCenterYChanged);

    connect(ui->cmbScaleType, qOverload<int>(&QComboBox::currentIndexChanged), this, &TaskSectionView::scaleTypeChanged);

    connect(ui->cbLiveUpdate, &QToolButton::clicked, this, &TaskSectionView::liveUpdateClicked);
    connect(ui->cbSectionCutOnly, &QCheckBox::clicked,
            this, [this]() { apply(); });
    connect(ui->cbShowOutsidePartialBoundaries, &QCheckBox::clicked,
            this, [this]() { apply(); });
    connect(ui->cbConnectionLine, &QCheckBox::clicked,
            this, [this]() { apply(); });
    connect(ui->cbShowManualControls, &QCheckBox::toggled, this,
            &TaskSectionView::onShowManualControlsToggled);
    connect(ui->pbSectionObjects, &QPushButton::clicked,
            this, &TaskSectionView::onSectionObjectsUseSelectionClicked);
    connect(ui->pbProfileObject, &QPushButton::clicked,
            this, &TaskSectionView::onProfileObjectUseSelectionClicked);
    connect(ui->cmbStrategy, qOverload<int>(&QComboBox::currentIndexChanged),
            this, &TaskSectionView::onProjectionStrategyChanged);
    connect(ui->cmbParallelTo,
            qOverload<int>(&QComboBox::currentIndexChanged), this,
            &TaskSectionView::onParallelToChanged);
    const auto connectPositionButton = [this](QToolButton* button) {
        button->setIconSize(QSize(28, 28));
        connect(button, &QToolButton::toggled, this,
                [this](bool checked) {
                    if (checked) {
                        onSectionLinePositionChanged(0);
                    }
                });
    };
    connectPositionButton(ui->tbSectionHorizontal);
    connectPositionButton(ui->tbSectionVertical);
    connectPositionButton(ui->tbSectionCenterDirection);
    connectPositionButton(ui->tbSectionCenterPoint);
    connectPositionButton(ui->tbSectionTwoPointsPartial);
    connectPositionButton(ui->tbSectionCenterTwoPoints);
    connectPositionButton(ui->tbSectionTwoPoints);
    connectPositionButton(ui->tbSectionSketchBased);
    connect(ui->pbResetSectionLine, &QPushButton::clicked,
            this, &TaskSectionView::onResetSectionLineClicked);
    m_compass = new CompassWidget(this);
    auto layout = ui->compassLayout;
    layout->addWidget(m_compass);
    connect(m_compass, &CompassWidget::angleChanged, this, &TaskSectionView::slotChangeAngle);
    connect(m_compass, &CompassWidget::directionReversed,
            this, &TaskSectionView::reverseSectionDirection);

    m_viewDirectionWidget = new VectorEditWidget(this);
    m_viewDirectionWidget->setLabel(QObject::tr("As vector"));
    m_viewDirectionWidget->setToolTip(QObject::tr("The view direction in BaseView coordinates"));
    auto editLayout = ui->viewDirectionLayout;
    editLayout->addWidget(m_viewDirectionWidget);
    connect(m_viewDirectionWidget, &VectorEditWidget::valueChanged, this,
            &TaskSectionView::slotViewDirectionChanged);
}

void TaskSectionView::setInteractiveBase(TechDraw::DrawViewPart* base)
{
    const bool changed = m_base != base;
    m_base = base;

    if (!m_base) {
        m_saveBaseName.clear();
        enableAll(false);
        Q_EMIT baseViewChanged(false);
        return;
    }

    m_doc = m_base->getDocument();
    m_page = m_base->findParentPage();
    m_saveBaseName = m_base->getNameInDocument();
    if (m_page) {
        m_savePageName = m_page->getNameInDocument();
    }
    if (changed && !m_section) {
        m_shapes = m_base->Source.getValues();
        m_xShapes = m_base->XSource.getValues();
        ui->leSectionObjects->setText(sourcesToString());
        ui->sbScale->setValue(m_base->getScale());
        ui->cmbScaleType->setCurrentIndex(m_base->getScaleType());
        setCenterUi({});
    }

    enableAll(true);
    Q_EMIT baseViewChanged(true);
}

void TaskSectionView::setInteractiveParameters(const Base::Vector3d& origin,
                                               const Base::Vector3d& direction)
{
    if (!m_base || direction.Sqr() <= std::numeric_limits<double>::epsilon()) {
        return;
    }

    setCenterUi(sectionOriginToView(origin));

    Base::Vector3d localDirection(direction);
    localDirection.Normalize();
    m_viewDirectionWidget->setValueNoNotify(localDirection);
    m_compass->setDialAngle(
        Base::toDegrees(std::atan2(localDirection.y, localDirection.x)));
    m_dirName = "Aligned";
    directionChanged(true);
    if (m_temporarySectionLine) {
        apply();
    }
}

void TaskSectionView::setTemporaryHalfRotations(double startRotation,
                                                double endRotation)
{
    m_startRotation = startRotation;
    m_endRotation = endRotation;
    drawTemporarySingleOffset();
    showUnappliedPreview(false);
}

void TaskSectionView::setDirectionControlsVisible(bool visible)
{
    Q_UNUSED(visible);
    updateManualControlsVisibility();
}

void TaskSectionView::setSectionControlsEnabled(bool enabled)
{
    if (m_customComplexSection) {
        enabled = false;
    }
    m_sectionControlsEnabled = enabled;
    if (enabled) {
        updateSectionControls();
    }
    else {
        hideSectionHandles();
    }
}

void TaskSectionView::setCustomComplexSection(bool enabled)
{
    const bool wasEnabled = m_customComplexSection;
    if (enabled && !wasEnabled) {
        m_profileBeforeSketchMode = m_profileObject;
        m_profileNameBeforeSketchMode = m_profileName;
        m_hasProfileBeforeSketchMode = true;
    }
    m_customComplexSection = enabled;
    if (enabled) {
        m_hasPendingOffset = false;
        m_hasPendingArc = false;
        m_hasPendingArcNotch = false;
        ui->gbOrientation->setEnabled(m_base != nullptr);
        ui->gbPlane->setEnabled(m_base != nullptr);
    }
    updateManualControlsVisibility();
    ui->gbCustomComplexSection->setVisible(enabled);
    if (enabled) {
        m_sectionControlsEnabled = false;
        clearSectionControls();
        hideSectionHandles();
        updateCustomProfilePreview();
    }
    else {
        if (wasEnabled && m_hasProfileBeforeSketchMode) {
            setProfileObject(m_profileBeforeSketchMode);
            m_profileName = m_profileNameBeforeSketchMode;
            if (auto* complexSection =
                    dynamic_cast<TechDraw::DrawComplexSection*>(m_section)) {
                complexSection->CuttingToolWireObject.setValue(
                    m_profileBeforeSketchMode);
                complexSection->ProjectionStrategy.setValue(
                    hasAlignedPath() ? 1L : 0L);
            }
            m_profileBeforeSketchMode = nullptr;
            m_profileNameBeforeSketchMode.clear();
            m_hasProfileBeforeSketchMode = false;
        }
        updateTemporarySectionLineFromUi();
        if (m_temporarySectionLine) {
            m_temporarySectionLine->show();
            setSectionControlsEnabled(true);
            showSectionHandles();
        }
    }
    if (m_graphicsView) {
        setPositioningActive(false);
    }
}

bool TaskSectionView::isGeneratedProfile(
    const App::DocumentObject* profile) const
{
    return profile
        && std::string(profile->getNameInDocument()).rfind(
            "SectionProfile", 0) == 0;
}

QString TaskSectionView::sourcesToString() const
{
    QString result;
    QString separator;
    auto append = [&result, &separator](App::DocumentObject* object) {
        if (!object) {
            return;
        }
        result += separator
            + QString::fromStdString(object->getNameInDocument())
            + QStringLiteral(" / ")
            + QString::fromStdString(object->Label.getValue());
        separator = QStringLiteral(", ");
    };
    for (App::DocumentObject* object : m_shapes) {
        append(object);
    }
    for (App::DocumentObject* object : m_xShapes) {
        append(object);
    }
    return result;
}

void TaskSectionView::setProfileObject(App::DocumentObject* profile)
{
    m_profileObject = profile;
    if (profile) {
        m_profileName = profile->getNameInDocument();
        ui->leProfileObject->setText(
            QString::fromStdString(profile->getNameInDocument())
            + QStringLiteral(" / ")
            + QString::fromStdString(profile->Label.getValue()));
    }
    else {
        m_profileName.clear();
        ui->leProfileObject->clear();
    }
    updateCustomProfilePreview();
}

void TaskSectionView::onSectionObjectsUseSelectionClicked()
{
    std::vector<App::DocumentObject*> shapes;
    std::vector<App::DocumentObject*> xShapes;
    std::vector<Gui::SelectionObject> selections =
        Gui::Selection().getSelectionEx();
    for (Gui::SelectionObject& selection : selections) {
        App::DocumentObject* object = selection.getObject();
        if (!object || object == m_profileObject
            || object->isDerivedFrom<TechDraw::DrawPage>()
            || object->isDerivedFrom<TechDraw::DrawViewPart>()) {
            continue;
        }
        if (object->isDerivedFrom<App::LinkElement>()
            || object->isDerivedFrom<App::LinkGroup>()
            || object->isDerivedFrom<App::Link>()) {
            xShapes.push_back(object);
        }
        else {
            shapes.push_back(object);
        }
    }
    if (!shapes.empty() || !xShapes.empty()) {
        m_shapes = std::move(shapes);
        m_xShapes = std::move(xShapes);
        ui->leSectionObjects->setText(sourcesToString());
        apply();
    }
}

void TaskSectionView::onProfileObjectUseSelectionClicked()
{
    std::vector<Gui::SelectionObject> selections =
        Gui::Selection().getSelectionEx();
    for (Gui::SelectionObject& selection : selections) {
        App::DocumentObject* object = selection.getObject();
        if (TechDraw::DrawComplexSection::isProfileObject(object)) {
            setProfileObject(object);
            showUnappliedPreview();
            apply();
            return;
        }
    }
    QMessageBox::warning(
        Gui::getMainWindow(), QObject::tr("Invalid profile"),
        QObject::tr("Select one wire or sketch to use as the section profile."));
}

void TaskSectionView::onProjectionStrategyChanged(int index)
{
    Q_UNUSED(index);
    apply();
}

void TaskSectionView::onParallelToChanged(int index)
{
    Q_UNUSED(index);
    apply();
}

void TaskSectionView::adoptTemporarySectionLine(QGISectionLine* line,
                                                 QGIViewPart* baseItem)
{
    if (!line || !line->scene()) {
        delete line;
        return;
    }
    m_temporarySectionLine = SceneItemPtr<QGISectionLine>(
        line, SceneItemDeleter(line->scene()));
    setBaseGraphicsItem(baseItem);
    configureTemporarySectionLine();
}

void TaskSectionView::adoptSketchBasedBase(QGISectionLine* line,
                                            QGIViewPart* baseItem)
{
    adoptTemporarySectionLine(line, baseItem);
    if (!m_temporarySectionLine) {
        return;
    }
    setCustomComplexSection(true);
    setPositioningActive(false);
}

void TaskSectionView::setBaseGraphicsItem(QGIViewPart* baseItem)
{
    QObject::disconnect(m_baseItemPositionConnection);
    m_baseItem = baseItem;
    m_baseItemPositionConnection = {};
    if (m_baseItem) {
        m_baseItemPositionConnection = connect(
            m_baseItem, &QGIView::positionChanged,
            this, &TaskSectionView::updateInteractiveOverlayPosition);
    }
}

void TaskSectionView::updateInteractiveOverlayPosition()
{
    if (m_customComplexSection) {
        updateCustomProfilePreview();
    }
    else {
        drawTemporarySingleOffset();
    }
}

TaskSectionView::PositionMode TaskSectionView::positionMode() const
{
    if (ui->tbSectionHorizontal->isChecked()) {
        return PositionMode::Horizontal;
    }
    if (ui->tbSectionVertical->isChecked()) {
        return PositionMode::Vertical;
    }
    if (ui->tbSectionCenterPoint->isChecked()) {
        return PositionMode::CenterAndPoint;
    }
    if (ui->tbSectionTwoPointsPartial->isChecked()) {
        return PositionMode::TwoPointsPartial;
    }
    if (ui->tbSectionCenterTwoPoints->isChecked()) {
        return PositionMode::CenterAndTwoPoints;
    }
    if (ui->tbSectionTwoPoints->isChecked()) {
        return PositionMode::TwoPoints;
    }
    if (ui->tbSectionSketchBased->isChecked()) {
        return PositionMode::SketchBased;
    }
    return PositionMode::CenterAndViewDirection;
}

void TaskSectionView::setPositioningActive(bool active)
{
    ui->sectionLinePositionButtons->setVisible(active);
    ui->pbResetSectionLine->setVisible(!active);
}

void TaskSectionView::resetInteractivePreview()
{
    if (m_customComplexSection) {
        setCustomComplexSection(false);
    }
    setSectionControlsEnabled(false);
    clearSectionControls();
    m_centerHandle.reset();
    m_offsetHandles.clear();
    m_arcBendHandles.clear();
    m_selectableOffsetEdges.clear();
    m_temporarySectionLine.reset();
    setBaseGraphicsItem(nullptr);
    m_offsetSteps.clear();
    m_arcBends.clear();
    m_hasPendingOffset = false;
    m_hasPendingArc = false;
    m_hasPendingArcNotch = false;
    m_startOffset = 0.0;
    m_startRotation = 0.0;
    m_endRotation = 0.0;
    m_endpointsEdited = false;
    m_startEndpointEdited = false;
    m_endEndpointEdited = false;
    m_halfSection = false;
    m_halfSectionTowardStart = false;
    updatePartialDisplayVisibility();

    if (m_createMode && m_section) {
        const std::string sectionName =
            m_section->getNameInDocument();
        Command::doCommand(
            Command::Doc,
            "App.ActiveDocument.%s.removeView(App.ActiveDocument.%s)",
            m_savePageName.c_str(), sectionName.c_str());
        Command::doCommand(
            Command::Doc,
            "App.ActiveDocument.removeObject('%s')",
            sectionName.c_str());
        m_section = nullptr;
        m_sectionName.clear();
    }
    if (m_createMode && isGeneratedProfile(m_profileObject)) {
        const std::string profileName =
            m_profileObject->getNameInDocument();
        Command::doCommand(
            Command::Doc,
            "App.ActiveDocument.removeObject('%s')",
            profileName.c_str());
    }
    if (m_createMode) {
        m_profileObject = nullptr;
        m_profileName.clear();
    }
    m_hasModernProfileState = false;

    setDirectionControlsVisible(false);
    setInteractiveBase(nullptr);
}

void TaskSectionView::onSectionLinePositionChanged(int index)
{
    Q_UNUSED(index);
    resetInteractivePreview();
    Q_EMIT restartPositioningRequested();
}

void TaskSectionView::onResetSectionLineClicked()
{
    resetInteractivePreview();
    Q_EMIT restartPositioningRequested();
}

void TaskSectionView::onShowManualControlsToggled(bool checked)
{
    Preferences::getPreferenceGroup("General")->SetBool(
        "ShowSectionViewManualControls", checked);
    updateManualControlsVisibility();
}

void TaskSectionView::updateManualControlsVisibility()
{
    const bool visible = ui->cbShowManualControls->isChecked();
    ui->gbPlane->setVisible(visible);
    ui->gbOrientation->setVisible(visible);
}

void TaskSectionView::configureTemporarySectionLine()
{
    if (!m_temporarySectionLine || !m_base) {
        return;
    }
    m_temporarySectionLine->setSymbol(const_cast<char*>(""));
    m_temporarySectionLine->setPathMode(true);
    m_temporarySectionLine->setSectionColor(Qt::black);
    m_temporarySectionLine->setArrowColor(QColor(35, 95, 210));
    m_temporarySectionLine->setArrowClickCallback([this]() {
        if (m_compass) {
            m_compass->reverseDirection();
        }
    });

    double lineWidth = 0.5;
    size_t lineStyle = 1;
    bool showLine = true;
    auto* viewProvider = dynamic_cast<ViewProviderViewPart*>(
        Gui::Application::Instance->getViewProvider(m_base));
    if (viewProvider) {
        lineWidth = viewProvider->HiddenWidth.getValue();
        lineStyle =
            static_cast<size_t>(viewProvider->SectionLineStyle.getValue());
        showLine = viewProvider->IncludeCutLine.getValue();
    }
    LineGenerator lineGenerator;
    QPen pen = lineGenerator.getLinePen(lineStyle, lineWidth);
    pen.setColor(Qt::black);
    m_temporarySectionLine->setLinePen(pen);
    m_temporarySectionLine->setWidth(Rez::guiX(lineWidth));
    m_temporarySectionLine->setShowLine(showLine);
    m_temporarySectionLine->setArrowSize(Preferences::dimArrowSize());
    m_temporarySectionLine->setFont(
        QFont(), Preferences::dimFontSizeMM());
    m_temporarySectionLine->setZValue(1000.0);
}

void TaskSectionView::initializeEditPreview(QGVPage* graphicsView)
{
    if (!graphicsView || !m_base) {
        return;
    }
    m_graphicsView = graphicsView;
    auto* pageScene = graphicsView->getScene();
    setBaseGraphicsItem(dynamic_cast<QGIViewPart*>(
        pageScene->findQViewForDocObj(m_base)));
    if (!m_baseItem) {
        return;
    }

    m_temporarySectionLine = makeSceneItem<QGISectionLine>(*pageScene);
    configureTemporarySectionLine();
    if (m_section) {
        setGeneratedSectionVisible(false);
    }

    if (m_customComplexSection) {
        updateCustomProfilePreview();
    }
    else {
        if (m_hasModernProfileState) {
            m_temporaryCenter =
                sectionOriginToView(m_modernCenterOrigin);
            drawTemporarySingleOffset();
        }
        else {
            updateTemporarySectionLineFromUi();
        }
        setSectionControlsEnabled(true);
        showSectionHandles();
    }
    m_temporarySectionLine->show();
    setPositioningActive(false);
}

void TaskSectionView::updateCustomProfilePreview()
{
    if (!m_temporarySectionLine || !m_baseItem || !m_base
        || !m_profileObject) {
        return;
    }

    const TopoDS_Wire profile =
        TechDraw::DrawComplexSection::makeProfileWire(m_profileObject);
    if (profile.IsNull()) {
        return;
    }
    const TopoDS_Shape projected =
        TechDraw::GeometryObject::projectSimpleShape(
            profile, m_base->getProjectionCS());
    PathBuilder pathBuilder(m_baseItem);
    QPainterPath localPath;
    for (TopExp_Explorer edges(projected, TopAbs_EDGE);
         edges.More(); edges.Next()) {
        const BaseGeomPtr geometry =
            BaseGeom::baseFactory(TopoDS::Edge(edges.Current()));
        const QPainterPath edgePath =
            pathBuilder.geomToPainterPath(geometry, 0.0);
        if (localPath.isEmpty()) {
            localPath = edgePath;
        }
        else {
            localPath.connectPath(edgePath);
        }
    }
    if (localPath.isEmpty()) {
        return;
    }

    const QPainterPath scenePath =
        m_baseItem->sceneTransform().map(localPath);
    const QPointF start = scenePath.pointAtPercent(0.0);
    const QPointF end = scenePath.pointAtPercent(1.0);
    m_temporarySectionLine->setPathMode(true);
    QPainterPath mutablePath(scenePath);
    m_temporarySectionLine->setPath(mutablePath);
    m_temporarySectionLine->setEnds(
        Base::Vector3d(start.x(), start.y(), 0.0),
        Base::Vector3d(end.x(), end.y(), 0.0));

    Base::Vector3d direction = m_viewDirectionWidget->value();
    if (direction.Sqr() <= std::numeric_limits<double>::epsilon()) {
        direction = Base::Vector3d(1.0, 0.0, 0.0);
    }
    direction.Normalize();
    const QPointF directionStart =
        m_baseItem->mapToScene(QPointF());
    const QPointF directionEnd =
        m_baseItem->mapToScene(QPointF(direction.x, -direction.y));
    const QPointF sceneDirection = directionEnd - directionStart;
    m_temporarySectionLine->setArrowDirections(
        Base::Vector3d(sceneDirection.x(), -sceneDirection.y(), 0.0),
        Base::Vector3d(sceneDirection.x(), -sceneDirection.y(), 0.0));
    m_temporarySectionLine->draw();
    m_temporarySectionLine->show();
}

void TaskSectionView::updateTemporarySectionLine(
    const QPointF& center,
    const Base::Vector3d& displayedDirection)
{
    if (!m_temporarySectionLine || !m_baseItem) {
        return;
    }

    m_temporaryCenter = center;
    m_displayedDirection = displayedDirection;
    if (!m_endpointsEdited) {
        const auto bounds = sectionLineBounds(
            m_baseItem, center, displayedDirection);
        m_temporaryStartAlong = bounds.first;
        m_temporaryEndAlong = bounds.second;
    }
    drawTemporarySingleOffset();
}

void TaskSectionView::updateTemporarySectionLineFromEndpoints(
    const QPointF& first,
    const QPointF& second,
    const Base::Vector3d& displayedDirection)
{
    const double length = std::hypot(
        second.x() - first.x(), second.y() - first.y());
    if (length <= std::numeric_limits<double>::epsilon()) {
        return;
    }

    // Unlike the center-based placement modes, Two points defines the actual
    // finite extent of the section line. Mark both ends as explicitly placed
    // before using the common updater so it does not replace them with the
    // padded bounds of the base view.
    m_temporaryStartAlong = -length * 0.5;
    m_temporaryEndAlong = length * 0.5;
    m_endpointsEdited = true;
    m_startEndpointEdited = true;
    m_endEndpointEdited = true;
    updateTemporarySectionLine(
        (first + second) * 0.5, displayedDirection);
}

void TaskSectionView::updateTemporarySectionLineEndpoint(
    const QPointF& center,
    const QPointF& endpoint,
    const Base::Vector3d& displayedDirection,
    bool startEndpoint)
{
    QPointF tangent(-displayedDirection.y, -displayedDirection.x);
    const double tangentLength = std::hypot(tangent.x(), tangent.y());
    if (tangentLength <= std::numeric_limits<double>::epsilon()) {
        return;
    }
    tangent /= tangentLength;

    const auto automaticBounds = sectionLineBounds(
        m_baseItem, center, displayedDirection);
    const double endpointAlong = QPointF::dotProduct(
        endpoint - center, tangent);
    constexpr double minimumHalfLength = 1.0e-3;

    m_temporaryCenter = center;
    m_displayedDirection = displayedDirection;
    if (startEndpoint) {
        m_temporaryStartAlong = std::min(
            endpointAlong, -minimumHalfLength);
        m_temporaryEndAlong = automaticBounds.second;
        m_startEndpointEdited = true;
        m_endEndpointEdited = false;
    }
    else {
        m_temporaryStartAlong = automaticBounds.first;
        m_temporaryEndAlong = std::max(
            endpointAlong, minimumHalfLength);
        m_startEndpointEdited = false;
        m_endEndpointEdited = true;
    }
    m_endpointsEdited = true;
    drawTemporarySingleOffset();
}

void TaskSectionView::updateTemporaryHalfSection(
    const QPointF& center,
    const QPointF& endpoint,
    const Base::Vector3d& displayedDirection)
{
    QPointF tangent(-displayedDirection.y, -displayedDirection.x);
    const double tangentLength = std::hypot(tangent.x(), tangent.y());
    if (tangentLength <= std::numeric_limits<double>::epsilon()) {
        return;
    }
    tangent /= tangentLength;

    const double endpointAlong = QPointF::dotProduct(endpoint - center, tangent);
    m_temporaryCenter = center;
    m_displayedDirection = displayedDirection;
    m_halfSection = true;
    m_halfSectionTowardStart = endpointAlong < 0.0;
    const auto automaticBounds = sectionLineBounds(
        m_baseItem, center, displayedDirection);
    m_temporaryStartAlong = automaticBounds.first;
    m_temporaryEndAlong = automaticBounds.second;
    // The second click selects which half and its line direction. The actual
    // endpoint remains automatic so a newly-created half section spans all
    // model geometry, just like the non-partial Two points mode.
    m_endpointsEdited = true;
    m_startEndpointEdited = !m_halfSectionTowardStart;
    m_endEndpointEdited = m_halfSectionTowardStart;
    drawTemporarySingleOffset();
}

void TaskSectionView::updateTemporaryRotatedSectionEndpoint(
    const QPointF& endpoint,
    bool startEndpoint)
{
    QPointF tangent(-m_displayedDirection.y, -m_displayedDirection.x);
    const double tangentLength = std::hypot(tangent.x(), tangent.y());
    if (tangentLength <= std::numeric_limits<double>::epsilon()) {
        return;
    }
    tangent /= tangentLength;

    const double rotation = startEndpoint
        ? m_startRotation : m_endRotation;
    const double cosine = std::cos(rotation);
    const double sine = std::sin(rotation);
    tangent = QPointF(
        tangent.x() * cosine - tangent.y() * sine,
        tangent.x() * sine + tangent.y() * cosine);
    const QPointF outward = startEndpoint ? -tangent : tangent;
    constexpr double minimumHalfLength = 1.0e-3;
    const double length = std::max(
        QPointF::dotProduct(endpoint - m_temporaryCenter, outward),
        minimumHalfLength);

    if (startEndpoint) {
        m_temporaryStartAlong = -length;
        m_startEndpointEdited = true;
    }
    else {
        m_temporaryEndAlong = length;
        m_endEndpointEdited = true;
    }
    m_endpointsEdited = true;
    drawTemporarySingleOffset();
}

std::pair<double, double>
TaskSectionView::projectOffsetStart(const QPointF& point) const
{
    const QPointF tangent(-m_displayedDirection.y, -m_displayedDirection.x);
    const double along = std::clamp(
        QPointF::dotProduct(point - m_temporaryCenter, tangent),
        m_temporaryStartAlong,
        m_temporaryEndAlong);
    double offset = m_startOffset;
    for (const auto& step : m_offsetSteps) {
        if (step.along > along) {
            break;
        }
        offset = step.offset;
    }
    return {along, offset};
}

void TaskSectionView::beginSingleOffset(double along,
                                        double currentOffset,
                                        bool towardStart)
{
    m_hasPendingOffset = true;
    m_pendingOffset.along =
        std::clamp(along, m_temporaryStartAlong, m_temporaryEndAlong);
    m_pendingOffset.offset = currentOffset;
    m_pendingOffset.towardStart = towardStart;
    m_pendingBaseOffset = currentOffset;
    m_pendingTowardStart = towardStart;
    m_pendingNotchOuterAlong = m_pendingOffset.along;
    drawTemporarySingleOffset();
}

void TaskSectionView::beginArcOffset(double along, bool towardStart)
{
    m_pendingArc = {along, 0.0, towardStart};
    m_hasPendingArc = true;
    drawTemporarySingleOffset();
}

void TaskSectionView::beginArcNotch(double along, bool towardStart)
{
    m_pendingArcNotchFirst = {along, 0.0, towardStart};
    m_pendingArcNotchSecond = {along, 0.0, towardStart};
    m_hasPendingArcNotch = true;
    drawTemporarySingleOffset();
}

void TaskSectionView::updatePendingSingleOffset(double offset)
{
    if (!m_hasPendingOffset) {
        return;
    }
    m_pendingOffset.offset = offset;
    drawTemporarySingleOffset();
    showUnappliedPreview(false);
}

void TaskSectionView::updatePendingNotchOuterAlong(double along)
{
    if (!m_hasPendingOffset
        || m_pendingOffsetKind != PendingOffsetKind::Notch) {
        return;
    }
    if (m_pendingTowardStart) {
        m_pendingNotchOuterAlong =
            std::clamp(along, m_pendingMinimumAlong,
                       m_pendingOffset.along);
    }
    else {
        m_pendingNotchOuterAlong =
            std::clamp(along, m_pendingOffset.along,
                       m_pendingMaximumAlong);
    }
    drawTemporarySingleOffset();
    showUnappliedPreview(false);
}

void TaskSectionView::updatePendingArcAngle(double angle)
{
    if (!m_hasPendingArc) {
        return;
    }
    m_pendingArc.angle = angle;
    drawTemporarySingleOffset();
    showUnappliedPreview(false);
}

void TaskSectionView::commitPendingArc()
{
    if (!m_hasPendingArc) {
        return;
    }
    if (std::abs(m_pendingArc.angle) > 1.0e-6) {
        m_arcBends.push_back(m_pendingArc);
    }
    m_hasPendingArc = false;
    drawTemporarySingleOffset();
}

void TaskSectionView::cancelPendingArc()
{
    m_hasPendingArc = false;
    drawTemporarySingleOffset();
}

void TaskSectionView::updatePendingArcNotch(double angle,
                                            double secondAlong)
{
    if (!m_hasPendingArcNotch) {
        return;
    }
    m_pendingArcNotchFirst.angle = angle;
    m_pendingArcNotchSecond.along = secondAlong;
    m_pendingArcNotchSecond.angle = -angle;
    drawTemporarySingleOffset();
    showUnappliedPreview(false);
}

void TaskSectionView::commitPendingArcNotch()
{
    if (!m_hasPendingArcNotch) {
        return;
    }
    if (std::abs(m_pendingArcNotchFirst.angle) > 1.0e-6
        && std::abs(m_pendingArcNotchSecond.along
                    - m_pendingArcNotchFirst.along) > 1.0e-6) {
        m_arcBends.push_back(m_pendingArcNotchFirst);
        m_arcBends.push_back(m_pendingArcNotchSecond);
    }
    m_hasPendingArcNotch = false;
    drawTemporarySingleOffset();
}

void TaskSectionView::cancelPendingArcNotch()
{
    m_hasPendingArcNotch = false;
    drawTemporarySingleOffset();
}

void TaskSectionView::commitPendingSingleOffset()
{
    if (!m_hasPendingOffset) {
        return;
    }
    const double delta = m_pendingOffset.offset - m_pendingBaseOffset;
    if (std::abs(delta) < 1.0e-6) {
        m_hasPendingOffset = false;
        drawTemporarySingleOffset();
        return;
    }
    const double maximumRun = m_pendingTowardStart
        ? m_pendingOffset.along - m_pendingMinimumAlong
        : m_pendingMaximumAlong - m_pendingOffset.along;
    const double run = std::min(std::abs(delta), std::max(0.0, maximumRun));
    const double outerAlong =
        m_pendingOffsetKind == PendingOffsetKind::Notch
        ? m_pendingNotchOuterAlong
        : m_pendingOffset.along + (m_pendingTowardStart ? -run : run);
    if (m_pendingOffsetKind == PendingOffsetKind::Notch) {
        const int modifierId = m_nextModifierId++;
        if (m_pendingTowardStart) {
            m_offsetSteps.push_back(
                {outerAlong, m_pendingOffset.offset,
                 OffsetStep::Transition::Sharp, 0.0,
                 OffsetStep::Role::NotchOuter, modifierId, true});
            m_offsetSteps.push_back(
                {m_pendingOffset.along, m_pendingBaseOffset,
                 OffsetStep::Transition::Sharp, 0.0,
                 OffsetStep::Role::NotchAnchor, modifierId, true});
        }
        else {
            m_offsetSteps.push_back(
                {m_pendingOffset.along, m_pendingOffset.offset,
                 OffsetStep::Transition::Sharp, 0.0,
                 OffsetStep::Role::NotchAnchor, modifierId, false});
            m_offsetSteps.push_back(
                {outerAlong, m_pendingBaseOffset,
                 OffsetStep::Transition::Sharp, 0.0,
                 OffsetStep::Role::NotchOuter, modifierId, false});
        }
    }
    else if (m_pendingOffsetKind == PendingOffsetKind::Arc) {
        if (m_pendingTowardStart) {
            m_startOffset += delta;
            for (auto& step : m_offsetSteps) {
                if (step.along < m_pendingOffset.along) {
                    step.offset += delta;
                }
            }
            m_offsetSteps.push_back(
                {m_pendingOffset.along, m_pendingBaseOffset,
                 OffsetStep::Transition::Arc, outerAlong,
                 OffsetStep::Role::Generic, 0, true});
        }
        else {
            for (auto& step : m_offsetSteps) {
                if (step.along >= outerAlong) {
                    step.offset += delta;
                }
            }
            m_offsetSteps.push_back(
                {outerAlong, m_pendingOffset.offset,
                 OffsetStep::Transition::Arc, m_pendingOffset.along,
                 OffsetStep::Role::Generic, 0, false});
        }
    }
    else {
        if (m_pendingTowardStart) {
            m_startOffset += delta;
            for (auto& step : m_offsetSteps) {
                if (step.along < m_pendingOffset.along) {
                    step.offset += delta;
                }
            }
            m_offsetSteps.push_back(
                {m_pendingOffset.along, m_pendingBaseOffset,
                 OffsetStep::Transition::Sharp, 0.0,
                 OffsetStep::Role::Generic, 0, true});
        }
        else {
            for (auto& step : m_offsetSteps) {
                if (step.along >= m_pendingOffset.along) {
                    step.offset += delta;
                }
            }
            m_offsetSteps.push_back(m_pendingOffset);
        }
    }
    std::sort(
        m_offsetSteps.begin(), m_offsetSteps.end(),
        [](const OffsetStep& left, const OffsetStep& right) {
            return left.along < right.along;
        });
    m_hasPendingOffset = false;
    m_offsetHandles.clear();
    drawTemporarySingleOffset();
}

void TaskSectionView::cancelPendingSingleOffset()
{
    m_hasPendingOffset = false;
    if (m_offsetSteps.empty()) {
        updateTemporarySectionLine(m_temporaryCenter, m_displayedDirection);
    }
    else {
        drawTemporarySingleOffset();
    }
}

QPointF TaskSectionView::rotateSectionPoint(const QPointF& point,
                                            double along) const
{
    const double angle = along < 0.0 ? m_startRotation
                                    : (along > 0.0 ? m_endRotation : 0.0);
    if (std::abs(angle) <= std::numeric_limits<double>::epsilon()) {
        return point;
    }
    const QPointF delta = point - m_temporaryCenter;
    const double cosine = std::cos(angle);
    const double sine = std::sin(angle);
    return m_temporaryCenter
        + QPointF(delta.x() * cosine - delta.y() * sine,
                  delta.x() * sine + delta.y() * cosine);
}

QPointF TaskSectionView::mapToSectionHalf(const QPointF& point,
                                          bool startHalf) const
{
    return rotateSectionPoint(point, startHalf ? -1.0 : 1.0);
}

QPointF TaskSectionView::mapFromSectionHalf(const QPointF& point,
                                            bool startHalf) const
{
    const double angle = startHalf ? -m_startRotation : -m_endRotation;
    const QPointF delta = point - m_temporaryCenter;
    return m_temporaryCenter
        + QPointF(delta.x() * std::cos(angle) - delta.y() * std::sin(angle),
                  delta.x() * std::sin(angle) + delta.y() * std::cos(angle));
}

QPointF TaskSectionView::sectionHalfEndpoint(bool startHalf) const
{
    const auto points = sectionPathPoints(false);
    if (points.empty()) {
        return m_temporaryCenter;
    }
    return startHalf ? points.front().first : points.back().first;
}

std::vector<std::pair<QPointF, double>>
TaskSectionView::sectionPathPoints(bool includePending) const
{
    const QPointF tangent(-m_displayedDirection.y, -m_displayedDirection.x);
    const QPointF normal(m_displayedDirection.x, -m_displayedDirection.y);
    std::vector<OffsetStep> steps = m_offsetSteps;
    double startOffset = m_startOffset;
    if (includePending && m_hasPendingOffset) {
        const double delta = m_pendingOffset.offset - m_pendingBaseOffset;
        const double maximumRun = m_pendingTowardStart
            ? m_pendingOffset.along - m_pendingMinimumAlong
            : m_pendingMaximumAlong - m_pendingOffset.along;
        const double run =
            std::min(std::abs(delta), std::max(0.0, maximumRun));
        const double outerAlong =
            m_pendingOffsetKind == PendingOffsetKind::Notch
            ? m_pendingNotchOuterAlong
            : m_pendingOffset.along + (m_pendingTowardStart ? -run : run);
        if (m_pendingOffsetKind == PendingOffsetKind::Notch) {
            if (m_pendingTowardStart) {
                steps.push_back({outerAlong, m_pendingOffset.offset});
                steps.push_back(
                    {m_pendingOffset.along, m_pendingBaseOffset});
            }
            else {
                steps.push_back(m_pendingOffset);
                steps.push_back({outerAlong, m_pendingBaseOffset});
            }
        }
        else if (m_pendingOffsetKind == PendingOffsetKind::Arc) {
            if (m_pendingTowardStart) {
                startOffset += delta;
                for (auto& step : steps) {
                    if (step.along < m_pendingOffset.along) {
                        step.offset += delta;
                    }
                }
                steps.push_back(
                    {m_pendingOffset.along, m_pendingBaseOffset,
                     OffsetStep::Transition::Arc, outerAlong});
            }
            else {
                for (auto& step : steps) {
                    if (step.along >= outerAlong) {
                        step.offset += delta;
                    }
                }
                steps.push_back(
                    {outerAlong, m_pendingOffset.offset,
                     OffsetStep::Transition::Arc,
                     m_pendingOffset.along});
            }
        }
        else {
            if (m_pendingTowardStart) {
                startOffset += delta;
                for (auto& step : steps) {
                    if (step.along < m_pendingOffset.along) {
                        step.offset += delta;
                    }
                }
                steps.push_back(
                    {m_pendingOffset.along, m_pendingBaseOffset});
            }
            else {
                for (auto& step : steps) {
                    if (step.along >= m_pendingOffset.along) {
                        step.offset += delta;
                    }
                }
                steps.push_back(m_pendingOffset);
            }
        }
        std::sort(
            steps.begin(), steps.end(),
            [](const OffsetStep& left, const OffsetStep& right) {
                return left.along < right.along;
            });
    }

    std::vector<std::pair<QPointF, double>> points;
    double currentOffset = startOffset;
    const QPointF start =
        m_temporaryCenter + tangent * m_temporaryStartAlong
        + normal * currentOffset;
    points.emplace_back(rotateSectionPoint(start, m_temporaryStartAlong),
                        m_temporaryStartAlong);
    bool addedCenter = false;
    for (const auto& step : steps) {
        if (!addedCenter && step.along >= 0.0) {
            const QPointF centerPoint =
                m_temporaryCenter + normal * currentOffset;
            points.emplace_back(centerPoint, 0.0);
            addedCenter = true;
        }
        if (step.transition == OffsetStep::Transition::Arc) {
            const auto arc = tessellateQuarterArc(
                step.startAlong, step.along, currentOffset, step.offset);
            for (const auto& sample : arc) {
                const QPointF arcPoint =
                    m_temporaryCenter + tangent * sample.along
                    + normal * sample.offset;
                points.emplace_back(
                    rotateSectionPoint(arcPoint, sample.along),
                    sample.along);
            }
            const double radius = std::abs(step.along - step.startAlong);
            const double offsetSign = step.offset >= currentOffset ? 1.0 : -1.0;
            const double curvedEndOffset =
                currentOffset + offsetSign * radius;
            if (std::abs(step.offset - curvedEndOffset) > 1.0e-6) {
                const QPointF arcEnd =
                    m_temporaryCenter + tangent * step.along
                    + normal * step.offset;
                points.emplace_back(rotateSectionPoint(arcEnd, step.along),
                                    step.along);
            }
            currentOffset = step.offset;
            continue;
        }
        const QPointF p1 =
            m_temporaryCenter + tangent * step.along + normal * currentOffset;
        const QPointF p2 =
            m_temporaryCenter + tangent * step.along + normal * step.offset;
        points.emplace_back(rotateSectionPoint(p1, step.along), step.along);
        points.emplace_back(rotateSectionPoint(p2, step.along), step.along);
        currentOffset = step.offset;
    }
    if (!addedCenter) {
        const QPointF centerPoint =
            m_temporaryCenter + normal * currentOffset;
        points.emplace_back(centerPoint, 0.0);
    }
    const QPointF end =
        m_temporaryCenter + tangent * m_temporaryEndAlong
        + normal * currentOffset;
    points.emplace_back(rotateSectionPoint(end, m_temporaryEndAlong),
                        m_temporaryEndAlong);

    std::vector<ArcBend> bends = m_arcBends;
    if (includePending && m_hasPendingArc) {
        bends.push_back(m_pendingArc);
    }
    if (includePending && m_hasPendingArcNotch) {
        bends.push_back(m_pendingArcNotchFirst);
        if (std::abs(m_pendingArcNotchSecond.along
                     - m_pendingArcNotchFirst.along) > 1.0e-6) {
            bends.push_back(m_pendingArcNotchSecond);
        }
    }
    auto rotateAroundCenter = [this](const QPointF& point, double angle) {
        const QPointF delta = point - m_temporaryCenter;
        return m_temporaryCenter
            + QPointF(delta.x() * std::cos(angle)
                          - delta.y() * std::sin(angle),
                      delta.x() * std::sin(angle)
                          + delta.y() * std::cos(angle));
    };
    for (const ArcBend& bend : bends) {
        std::vector<std::pair<QPointF, double>> withAnchor = points;
        const bool hasAnchor = std::any_of(
            withAnchor.begin(), withAnchor.end(),
            [&bend](const auto& point) {
                return std::abs(point.second - bend.along) < 1.0e-6;
            });
        if (!hasAnchor) {
            for (size_t i = 1; i < withAnchor.size(); ++i) {
                if (withAnchor[i - 1].second < bend.along
                    && withAnchor[i].second > bend.along) {
                    const double ratio =
                        (bend.along - withAnchor[i - 1].second)
                        / (withAnchor[i].second
                           - withAnchor[i - 1].second);
                    const QPointF anchor =
                        withAnchor[i - 1].first
                        + (withAnchor[i].first
                           - withAnchor[i - 1].first) * ratio;
                    withAnchor.insert(withAnchor.begin() + i,
                                      {anchor, bend.along});
                    break;
                }
            }
        }

        std::vector<std::pair<QPointF, double>> bentPoints;
        bool emittedArc = false;
        constexpr int arcSegments = 16;
        for (const auto& point : withAnchor) {
            if (point.second < bend.along - 1.0e-6) {
                bentPoints.emplace_back(
                    bend.towardStart
                        ? rotateAroundCenter(point.first, bend.angle)
                        : point.first,
                    point.second);
            }
            else if (!emittedArc
                     && std::abs(point.second - bend.along) < 1.0e-6) {
                if (bend.towardStart) {
                    for (int segment = arcSegments;
                         segment >= 0; --segment) {
                        const double angle =
                            bend.angle * static_cast<double>(segment)
                            / arcSegments;
                        bentPoints.emplace_back(
                            rotateAroundCenter(point.first, angle),
                            bend.along);
                    }
                }
                else {
                    for (int segment = 0;
                         segment <= arcSegments; ++segment) {
                        const double angle =
                            bend.angle * static_cast<double>(segment)
                            / arcSegments;
                        bentPoints.emplace_back(
                            rotateAroundCenter(point.first, angle),
                            bend.along);
                    }
                }
                emittedArc = true;
            }
            else {
                bentPoints.emplace_back(
                    !bend.towardStart
                        ? rotateAroundCenter(point.first, bend.angle)
                        : point.first,
                    point.second);
            }
        }
        points = std::move(bentPoints);
    }
    if (m_halfSection) {
        std::erase_if(points, [this](const auto& point) {
            return m_halfSectionTowardStart
                ? point.second > 1.0e-6
                : point.second < -1.0e-6;
        });
    }
    return points;
}

QPointF TaskSectionView::halfSectionLeaderEnd() const
{
    QPointF arrowDirection(m_displayedDirection.x,
                           -m_displayedDirection.y);
    const double length = std::hypot(
        arrowDirection.x(), arrowDirection.y());
    if (length <= std::numeric_limits<double>::epsilon()) {
        arrowDirection = QPointF(1.0, 0.0);
    }
    else {
        arrowDirection /= length;
    }

    QRectF bounds = m_baseItem ? m_baseItem->contentBoundingRect() : QRectF();
    if (bounds.isEmpty() && m_baseItem) {
        bounds = m_baseItem->boundingRect();
    }
    double minimumProjection = 0.0;
    if (!bounds.isEmpty()) {
        const std::array<QPointF, 4> corners{
            bounds.topLeft(), bounds.topRight(),
            bounds.bottomLeft(), bounds.bottomRight()};
        minimumProjection = std::numeric_limits<double>::max();
        for (const QPointF& corner : corners) {
            minimumProjection = std::min(
                minimumProjection,
                QPointF::dotProduct(corner - m_temporaryCenter,
                                    arrowDirection));
        }
    }
    return m_temporaryCenter
        + arrowDirection * (minimumProjection - Rez::guiX(10.0));
}

void TaskSectionView::drawTemporarySingleOffset()
{
    updateParallelToVisibility();
    if (!m_temporarySectionLine || !m_baseItem) {
        return;
    }

    const auto points = sectionPathPoints(true);
    if (points.size() < 2) {
        return;
    }
    QPainterPath path;
    path.moveTo(m_baseItem->mapToScene(points.front().first));
    for (size_t i = 1; i < points.size(); ++i) {
        path.lineTo(m_baseItem->mapToScene(points[i].first));
    }
    QPointF sceneLeaderEnd;
    if (m_halfSection) {
        sceneLeaderEnd = m_baseItem->mapToScene(halfSectionLeaderEnd());
        path.moveTo(m_baseItem->mapToScene(m_temporaryCenter));
        path.lineTo(sceneLeaderEnd);
    }
    m_temporarySectionLine->setPathMode(true);
    m_temporarySectionLine->setPath(path);

    const QPointF sceneStart = m_baseItem->mapToScene(points.front().first);
    const QPointF sceneEnd = m_baseItem->mapToScene(points.back().first);
    const QPointF arrowStart = m_baseItem->mapToScene(m_temporaryCenter);
    double startAngle = m_startRotation;
    double endAngle = m_endRotation;
    for (const ArcBend& bend : m_arcBends) {
        (bend.towardStart ? startAngle : endAngle) += bend.angle;
    }
    if (m_hasPendingArc) {
        (m_pendingArc.towardStart ? startAngle : endAngle)
            += m_pendingArc.angle;
    }
    if (m_hasPendingArcNotch) {
        (m_pendingArcNotchFirst.towardStart ? startAngle : endAngle)
            += m_pendingArcNotchFirst.angle;
        if (std::abs(m_pendingArcNotchSecond.along
                     - m_pendingArcNotchFirst.along) > 1.0e-6) {
            (m_pendingArcNotchSecond.towardStart ? startAngle : endAngle)
                += m_pendingArcNotchSecond.angle;
        }
    }
    const QPointF arrowVector(
        m_displayedDirection.x, -m_displayedDirection.y);
    auto rotatedArrow = [this, &arrowVector](double angle) {
        return m_temporaryCenter
            + QPointF(arrowVector.x() * std::cos(angle)
                          - arrowVector.y() * std::sin(angle),
                      arrowVector.x() * std::sin(angle)
                          + arrowVector.y() * std::cos(angle));
    };
    const QPointF startArrowEnd =
        m_baseItem->mapToScene(rotatedArrow(startAngle));
    const QPointF endArrowEnd =
        m_baseItem->mapToScene(rotatedArrow(endAngle));
    const QPointF startDirection = startArrowEnd - arrowStart;
    const QPointF endDirection = endArrowEnd - arrowStart;
    if (m_halfSection) {
        const QPointF retainedEnd =
            m_halfSectionTowardStart ? sceneStart : sceneEnd;
        const QPointF retainedDirection = m_halfSectionTowardStart
            ? startDirection : endDirection;
        m_temporarySectionLine->setEnds(
            Base::Vector3d(sceneLeaderEnd.x(), sceneLeaderEnd.y(), 0.0),
            Base::Vector3d(retainedEnd.x(), retainedEnd.y(), 0.0));
        m_temporarySectionLine->setArrowDirections(
            Base::Vector3d(retainedDirection.x(), -retainedDirection.y(), 0.0),
            Base::Vector3d(retainedDirection.x(), -retainedDirection.y(), 0.0));
    }
    else {
        m_temporarySectionLine->setEnds(
            Base::Vector3d(sceneStart.x(), sceneStart.y(), 0.0),
            Base::Vector3d(sceneEnd.x(), sceneEnd.y(), 0.0));
        m_temporarySectionLine->setArrowDirections(
            Base::Vector3d(startDirection.x(), -startDirection.y(), 0.0),
            Base::Vector3d(endDirection.x(), -endDirection.y(), 0.0));
    }
    m_temporarySectionLine->draw();
    rebuildSelectableOffsetEdges();
    updateSectionHandles();
    updateSectionControls();
}

void TaskSectionView::rebuildSelectableOffsetEdges()
{
    m_selectableOffsetEdges.clear();
    if (m_customComplexSection || !m_sectionControlsEnabled
        || !m_baseItem || !m_baseItem->scene()
        || m_hasPendingOffset || m_hasPendingArc
        || m_hasPendingArcNotch) {
        return;
    }

    const QPointF tangent(-m_displayedDirection.y,
                          -m_displayedDirection.x);
    const QPointF normal(m_displayedDirection.x,
                         -m_displayedDirection.y);
    double currentOffset = m_startOffset;
    for (size_t i = 0; i < m_offsetSteps.size(); ++i) {
        const OffsetStep& step = m_offsetSteps[i];
        QPainterPath edgePath;
        if (step.transition == OffsetStep::Transition::Arc) {
            const auto arc = tessellateQuarterArc(
                step.startAlong, step.along, currentOffset, step.offset);
            for (const auto& sample : arc) {
                const QPointF point =
                    m_temporaryCenter + tangent * sample.along
                    + normal * sample.offset;
                const QPointF scenePoint = m_baseItem->mapToScene(
                    rotateSectionPoint(point, sample.along));
                if (edgePath.isEmpty()) {
                    edgePath.moveTo(scenePoint);
                }
                else {
                    edgePath.lineTo(scenePoint);
                }
            }
        }
        else {
            const QPointF first =
                m_temporaryCenter + tangent * step.along
                + normal * currentOffset;
            const QPointF second =
                m_temporaryCenter + tangent * step.along
                + normal * step.offset;
            edgePath.moveTo(m_baseItem->mapToScene(
                rotateSectionPoint(first, step.along)));
            edgePath.lineTo(m_baseItem->mapToScene(
                rotateSectionPoint(second, step.along)));
        }
        auto edge = makeSceneItem<SelectableOffsetEdge>(
            *m_baseItem->scene(), [this, i]() {
            QTimer::singleShot(
                0, this, [this, i]() { removeOffsetStep(i); });
        });
        edge->setPath(edgePath);
        m_selectableOffsetEdges.push_back(std::move(edge));
        currentOffset = step.offset;
    }

    const auto path = sectionPathPoints(false);
    for (size_t i = 0; i < m_arcBends.size(); ++i) {
        const double along = m_arcBends[i].along;
        QPainterPath edgePath;
        bool found = false;
        for (const auto& point : path) {
            if (std::abs(point.second - along) < 1.0e-6) {
                const QPointF scenePoint =
                    m_baseItem->mapToScene(point.first);
                if (!found) {
                    edgePath.moveTo(scenePoint);
                    found = true;
                }
                else {
                    edgePath.lineTo(scenePoint);
                }
            }
            else if (found) {
                break;
            }
        }
        if (!found || edgePath.elementCount() < 2) {
            continue;
        }
        auto edge = makeSceneItem<SelectableOffsetEdge>(
            *m_baseItem->scene(), [this, i]() {
            QTimer::singleShot(
                0, this, [this, i]() { removeArcBend(i); });
        });
        edge->setPath(edgePath);
        m_selectableOffsetEdges.push_back(std::move(edge));
    }

    if (m_halfSection) {
        QPainterPath leaderPath;
        leaderPath.moveTo(m_baseItem->mapToScene(m_temporaryCenter));
        leaderPath.lineTo(m_baseItem->mapToScene(halfSectionLeaderEnd()));
        auto leader = makeSceneItem<SelectableOffsetEdge>(
            *m_baseItem->scene(), [this]() {
                QTimer::singleShot(0, this,
                                   [this]() { clearHalfSection(); });
            },
            QObject::tr(
                "Select and press Delete to restore the full section"));
        leader->setPath(leaderPath);
        m_selectableOffsetEdges.push_back(std::move(leader));
        return;
    }

    // The two ordinary center-to-end segments are selectable independently.
    // Deleting one converts the section to a half section retaining the
    // opposite side.
    const auto fullPath = sectionPathPoints(false);
    auto addHalfSelector = [this, &fullPath](bool startHalf) {
        QPainterPath selectablePath;
        bool hasPoint = false;
        for (const auto& point : fullPath) {
            if ((startHalf && point.second <= 1.0e-6)
                || (!startHalf && point.second >= -1.0e-6)) {
                const QPointF scenePoint =
                    m_baseItem->mapToScene(point.first);
                if (!hasPoint) {
                    selectablePath.moveTo(scenePoint);
                    hasPoint = true;
                }
                else {
                    selectablePath.lineTo(scenePoint);
                }
            }
        }
        if (selectablePath.elementCount() < 2) {
            return;
        }
        auto selector = makeSceneItem<SelectableOffsetEdge>(
            *m_baseItem->scene(), [this, startHalf]() {
                QTimer::singleShot(0, this, [this, startHalf]() {
                    makeHalfSection(!startHalf);
                });
            },
            QObject::tr(
                "Select and press Delete to remove this half"));
        selector->setPath(selectablePath);
        m_selectableOffsetEdges.push_back(std::move(selector));
    };
    addHalfSelector(true);
    addHalfSelector(false);
}

void TaskSectionView::makeHalfSection(bool retainStartHalf)
{
    m_halfSection = true;
    m_halfSectionTowardStart = retainStartHalf;
    m_endpointsEdited = true;
    m_startEndpointEdited = !retainStartHalf;
    m_endEndpointEdited = retainStartHalf;
    showUnappliedPreview(false);
    drawTemporarySingleOffset();
    showSectionHandles();
    apply();
}

void TaskSectionView::clearHalfSection()
{
    m_halfSection = false;
    m_halfSectionTowardStart = false;
    m_endpointsEdited = false;
    m_startEndpointEdited = false;
    m_endEndpointEdited = false;
    const auto bounds = sectionLineBounds(
        m_baseItem, m_temporaryCenter, m_displayedDirection);
    m_temporaryStartAlong = bounds.first;
    m_temporaryEndAlong = bounds.second;
    showUnappliedPreview(false);
    drawTemporarySingleOffset();
    showSectionHandles();
    apply();
}

void TaskSectionView::removeOffsetStep(size_t index)
{
    if (index >= m_offsetSteps.size()) {
        return;
    }
    const OffsetStep selected = m_offsetSteps[index];
    if (selected.role != OffsetStep::Role::Generic
        && selected.modifierId != 0) {
        std::erase_if(
            m_offsetSteps,
            [&selected](const OffsetStep& step) {
                return step.modifierId == selected.modifierId;
            });
    }
    else {
        const double precedingOffset =
            index == 0 ? m_startOffset
                       : m_offsetSteps[index - 1].offset;
        if (selected.towardStart) {
            const double delta =
                precedingOffset - selected.offset;
            m_startOffset -= delta;
            for (size_t i = 0; i < index; ++i) {
                m_offsetSteps[i].offset -= delta;
            }
        }
        else {
            const double delta =
                selected.offset - precedingOffset;
            for (size_t i = index + 1;
                 i < m_offsetSteps.size(); ++i) {
                m_offsetSteps[i].offset -= delta;
            }
        }
        m_offsetSteps.erase(m_offsetSteps.begin() + index);
    }

    m_offsetHandles.clear();
    showUnappliedPreview(false);
    drawTemporarySingleOffset();
    showSectionHandles();
    apply();
}

void TaskSectionView::removeArcBend(size_t index)
{
    if (index >= m_arcBends.size()) {
        return;
    }
    m_arcBends.erase(m_arcBends.begin() + index);
    m_arcBendHandles.clear();
    showUnappliedPreview(false);
    drawTemporarySingleOffset();
    showSectionHandles();
    apply();
}

void TaskSectionView::ensureSectionHandles()
{
    if (m_customComplexSection || !m_baseItem || !m_baseItem->scene()) {
        return;
    }
    if (!m_centerHandle) {
        m_centerHandle = makeSceneItem<QGraphicsEllipseItem, SectionDragHandle>(
            *m_baseItem->scene(),
            Qt::black,
            [this](const QPointF& point, Qt::KeyboardModifiers modifiers) {
                moveSectionCenter(point, modifiers);
            });
    }
    while (m_offsetHandles.size() < m_offsetSteps.size()) {
        const size_t index = m_offsetHandles.size();
        const QColor offsetColor(35, 95, 210);
        const bool notchAnchor =
            m_offsetSteps[index].role == OffsetStep::Role::NotchAnchor;
        const bool reverseNotch = notchAnchor
            && m_offsetSteps[index].towardStart;
        const bool reverseSingle = !notchAnchor
            && m_offsetSteps[index].towardStart;
        auto p1Handle = makeSceneItem<QGraphicsEllipseItem, SectionDragHandle>(
            *m_baseItem->scene(),
            offsetColor,
            [this, index, notchAnchor, reverseNotch, reverseSingle](
                const QPointF& point, Qt::KeyboardModifiers modifiers) {
                if (notchAnchor) {
                    if (reverseNotch) {
                        moveNotchP2(index, point, modifiers);
                    }
                    else {
                        moveNotchP1(index, point, modifiers);
                    }
                }
                else if (reverseSingle) {
                    moveSingleOffsetP2(index, point, modifiers);
                }
                else {
                    moveSingleOffsetP1(index, point, modifiers);
                }
            });
        auto p2Handle = makeSceneItem<QGraphicsEllipseItem, SectionDragHandle>(
            *m_baseItem->scene(),
            offsetColor,
            [this, index, notchAnchor, reverseNotch, reverseSingle](
                const QPointF& point, Qt::KeyboardModifiers modifiers) {
                if (notchAnchor) {
                    if (reverseNotch) {
                        moveNotchP1(index, point, modifiers);
                    }
                    else {
                        moveNotchP2(index, point, modifiers);
                    }
                }
                else if (reverseSingle) {
                    moveSingleOffsetP1(index, point, modifiers);
                }
                else {
                    moveSingleOffsetP2(index, point, modifiers);
                }
            });
        m_offsetHandles.emplace_back(std::move(p1Handle),
                                     std::move(p2Handle));
    }
    while (m_arcBendHandles.size() < m_arcBends.size()) {
        const size_t index = m_arcBendHandles.size();
        const QColor arcColor(145, 70, 190);
        auto bendHandle = makeSceneItem<QGraphicsEllipseItem, SectionDragHandle>(
            *m_baseItem->scene(),
            arcColor,
            [this, index](const QPointF& point,
                          Qt::KeyboardModifiers modifiers) {
                moveArcBendPoint(index, false, point, modifiers);
            });
        auto endpointHandle = makeSceneItem<QGraphicsEllipseItem, SectionDragHandle>(
            *m_baseItem->scene(),
            arcColor,
            [this, index](const QPointF& point,
                          Qt::KeyboardModifiers modifiers) {
                moveArcBendPoint(index, true, point, modifiers);
            });
        m_arcBendHandles.emplace_back(std::move(bendHandle),
                                      std::move(endpointHandle));
    }
    updateSectionHandles();
}

void TaskSectionView::updateSectionHandles()
{
    if (m_customComplexSection) {
        hideSectionHandles();
        return;
    }
    if (!m_baseItem) {
        return;
    }
    if (m_centerHandle) {
        m_centerHandle->setPos(m_baseItem->mapToScene(m_temporaryCenter));
    }
    if (!m_offsetSteps.empty()) {
        const QPointF tangent(-m_displayedDirection.y, -m_displayedDirection.x);
        const QPointF normal(m_displayedDirection.x, -m_displayedDirection.y);
        double currentOffset = m_startOffset;
        const size_t handleCount =
            std::min(m_offsetSteps.size(), m_offsetHandles.size());
        for (size_t i = 0; i < handleCount; ++i) {
            const double p1Along =
                m_offsetSteps[i].transition == OffsetStep::Transition::Arc
                ? m_offsetSteps[i].startAlong : m_offsetSteps[i].along;
            const QPointF p1 =
                m_temporaryCenter + tangent * p1Along
                + normal * currentOffset;
            const QPointF p2 =
                m_temporaryCenter + tangent * m_offsetSteps[i].along
                + normal * m_offsetSteps[i].offset;
            m_offsetHandles[i].first->setPos(m_baseItem->mapToScene(
                rotateSectionPoint(p1, p1Along)));
            m_offsetHandles[i].second->setPos(m_baseItem->mapToScene(
                rotateSectionPoint(p2, m_offsetSteps[i].along)));
            currentOffset = m_offsetSteps[i].offset;
        }
    }
    if (!m_arcBends.empty()) {
        const auto path = sectionPathPoints(false);
        const size_t handleCount =
            std::min(m_arcBends.size(), m_arcBendHandles.size());
        for (size_t i = 0; i < handleCount; ++i) {
            const ArcBend& bend = m_arcBends[i];
            auto firstArcPoint = std::find_if(
                path.begin(), path.end(), [&bend](const auto& point) {
                    return std::abs(point.second - bend.along) < 1.0e-6;
                });
            if (firstArcPoint != path.end()) {
                auto lastArcPoint = firstArcPoint;
                for (auto point = std::next(firstArcPoint);
                     point != path.end()
                         && std::abs(point->second - bend.along) < 1.0e-6;
                     ++point) {
                    lastArcPoint = point;
                }
                const auto anchorPoint =
                    bend.towardStart ? lastArcPoint : firstArcPoint;
                const auto rotatedPoint =
                    bend.towardStart ? firstArcPoint : lastArcPoint;
                m_arcBendHandles[i].first->setPos(
                    m_baseItem->mapToScene(anchorPoint->first));
                m_arcBendHandles[i].second->setPos(
                    m_baseItem->mapToScene(rotatedPoint->first));
            }
        }
    }
}

void TaskSectionView::ensureSectionControls()
{
    if (m_customComplexSection || !m_baseItem || !m_baseItem->scene()) {
        return;
    }
    if (!m_startRotationHandle) {
        m_startRotationHandle = makeSceneItem<QGraphicsItem, SectionRotationHandle>(
            *m_baseItem->scene(),
            [this](const QPointF& point, Qt::KeyboardModifiers modifiers,
                   bool commit) {
                rotateSectionHalf(true, point, modifiers, commit);
            },
            [this](bool visible) {
                if (m_baseItem) {
                    m_baseItem->setFrameForcedVisible(visible);
                }
            });
    }
    if (!m_endRotationHandle) {
        m_endRotationHandle = makeSceneItem<QGraphicsItem, SectionRotationHandle>(
            *m_baseItem->scene(),
            [this](const QPointF& point, Qt::KeyboardModifiers modifiers,
                   bool commit) {
                rotateSectionHalf(false, point, modifiers, commit);
            },
            [this](bool visible) {
                if (m_baseItem) {
                    m_baseItem->setFrameForcedVisible(visible);
                }
            });
    }
    if (!m_startEndpointHandle) {
        m_startEndpointHandle = makeSceneItem<QGraphicsItem, SectionDragHandle>(
            *m_baseItem->scene(), QColor(35, 95, 210),
            [this](const QPointF& point, Qt::KeyboardModifiers modifiers) {
                moveSectionEndpoint(true, point, modifiers);
            });
        m_startEndpointHandle->setToolTip(
            tr("Drag to shorten or extend the section view"));
    }
    if (!m_endEndpointHandle) {
        m_endEndpointHandle = makeSceneItem<QGraphicsItem, SectionDragHandle>(
            *m_baseItem->scene(), QColor(35, 95, 210),
            [this](const QPointF& point, Qt::KeyboardModifiers modifiers) {
                moveSectionEndpoint(false, point, modifiers);
            });
        m_endEndpointHandle->setToolTip(
            tr("Drag to shorten or extend the section view"));
    }
}

void TaskSectionView::updateSectionControls()
{
    if (m_customComplexSection) {
        clearSectionControls();
        return;
    }
    if (!m_sectionControlsEnabled || !m_baseItem || !m_baseItem->scene()
        || m_hasPendingOffset || m_hasPendingArc
        || m_hasPendingArcNotch) {
        return;
    }
    ensureSectionControls();
    m_sectionControls.clear();

    std::vector<double> bounds{m_temporaryStartAlong, 0.0,
                               m_temporaryEndAlong};
    for (const auto& step : m_offsetSteps) {
        bounds.push_back(step.along);
        if (step.transition == OffsetStep::Transition::Arc) {
            bounds.push_back(step.startAlong);
        }
    }
    for (const ArcBend& bend : m_arcBends) {
        bounds.push_back(bend.along);
    }
    if (m_halfSection) {
        std::erase_if(bounds, [this](double along) {
            return m_halfSectionTowardStart
                ? along > 1.0e-6 : along < -1.0e-6;
        });
    }
    std::sort(bounds.begin(), bounds.end());
    bounds.erase(std::unique(bounds.begin(), bounds.end(),
                             [](double left, double right) {
                                 return std::abs(left - right) < 1.0e-6;
                             }),
                 bounds.end());
    const QPointF tangent(-m_displayedDirection.y, -m_displayedDirection.x);
    const QPointF normal(m_displayedDirection.x, -m_displayedDirection.y);
    auto applyArcBends = [this](QPointF point, double along) {
        for (const ArcBend& bend : m_arcBends) {
            const bool isOuter = bend.towardStart
                ? along < bend.along : along > bend.along;
            if (!isOuter) {
                continue;
            }
            const QPointF delta = point - m_temporaryCenter;
            point = m_temporaryCenter
                + QPointF(delta.x() * std::cos(bend.angle)
                              - delta.y() * std::sin(bend.angle),
                          delta.x() * std::sin(bend.angle)
                              + delta.y() * std::cos(bend.angle));
        }
        return point;
    };
    for (size_t i = 1; i < bounds.size(); ++i) {
        if (bounds[i] - bounds[i - 1] < 1.0e-3) {
            continue;
        }
        const bool isArcInterval = std::any_of(
            m_offsetSteps.begin(), m_offsetSteps.end(),
            [lower = bounds[i - 1], upper = bounds[i]](const OffsetStep& step) {
                return step.transition == OffsetStep::Transition::Arc
                    && lower >= std::min(step.startAlong, step.along) - 1.0e-6
                    && upper <= std::max(step.startAlong, step.along) + 1.0e-6;
            });
        if (isArcInterval) {
            continue;
        }
        const double midpoint = (bounds[i - 1] + bounds[i]) * 0.5;
        const bool towardStart = midpoint < 0.0;
        const double offset = projectOffsetStart(
            m_temporaryCenter + tangent * midpoint).second;
        const QPointF position = applyArcBends(
            rotateSectionPoint(
                m_temporaryCenter + tangent * midpoint + normal * offset,
                midpoint),
            midpoint);
        const QPointF scenePosition = m_baseItem->mapToScene(position);
        const double minimumAlong = bounds[i - 1];
        const double maximumAlong = bounds[i];
        auto item = makeSceneItem<QGraphicsItem, SectionAddOffsetButton>(
            *m_baseItem->scene(),
            [this, midpoint, minimumAlong, maximumAlong, towardStart]() {
                startSingleOffset(midpoint, minimumAlong,
                                  maximumAlong, towardStart);
            },
            [this, midpoint, minimumAlong, maximumAlong, towardStart]() {
                startNotchOffset(midpoint, minimumAlong,
                                 maximumAlong, towardStart);
            },
            [this, midpoint, minimumAlong, maximumAlong, towardStart]() {
                startArcOffset(midpoint, minimumAlong,
                               maximumAlong, towardStart);
            },
            [this, midpoint, minimumAlong, maximumAlong, towardStart]() {
                startArcNotchOffset(midpoint, minimumAlong,
                                    maximumAlong, towardStart);
            });
        item->setPos(scenePosition);
        m_sectionControls.push_back(std::move(item));
    }

    const auto points = sectionPathPoints(false);
    if (points.size() >= 2) {
        const QPointF sceneCenter =
            m_baseItem->mapToScene(m_temporaryCenter);
        static_cast<SectionRotationHandle*>(m_startRotationHandle.get())
            ->setRotationArc(sceneCenter,
                             m_baseItem->mapToScene(points.front().first),
                             true);
        static_cast<SectionRotationHandle*>(m_endRotationHandle.get())
            ->setRotationArc(sceneCenter,
                             m_baseItem->mapToScene(points.back().first),
                             false);
        m_startEndpointHandle->setPos(
            m_baseItem->mapToScene(points.front().first));
        m_endEndpointHandle->setPos(
            m_baseItem->mapToScene(points.back().first));
    }
}

void TaskSectionView::clearSectionControls()
{
    m_sectionControls.clear();
    m_startRotationHandle.reset();
    m_endRotationHandle.reset();
    m_startEndpointHandle.reset();
    m_endEndpointHandle.reset();
}

void TaskSectionView::showSectionHandles()
{
    if (m_customComplexSection) {
        hideSectionHandles();
        return;
    }
    ensureSectionHandles();
    updateSectionControls();
    if (m_centerHandle) {
        m_centerHandle->show();
    }
    for (auto& handles : m_offsetHandles) {
        handles.first->show();
        handles.second->show();
    }
    for (auto& handles : m_arcBendHandles) {
        handles.first->show();
        handles.second->show();
    }
    rebuildSelectableOffsetEdges();
    for (const auto& item : m_sectionControls) {
        item->show();
    }
    if (m_startRotationHandle) {
        m_startRotationHandle->setVisible(
            !m_halfSection || m_halfSectionTowardStart);
    }
    if (m_endRotationHandle) {
        m_endRotationHandle->setVisible(
            !m_halfSection || !m_halfSectionTowardStart);
    }
    if (m_startEndpointHandle) {
        m_startEndpointHandle->setVisible(
            !m_halfSection || m_halfSectionTowardStart);
    }
    if (m_endEndpointHandle) {
        m_endEndpointHandle->setVisible(
            !m_halfSection || !m_halfSectionTowardStart);
    }
}

void TaskSectionView::hideSectionHandles()
{
    if (m_centerHandle) {
        m_centerHandle->hide();
    }
    for (auto& handles : m_offsetHandles) {
        handles.first->hide();
        handles.second->hide();
    }
    for (auto& handles : m_arcBendHandles) {
        handles.first->hide();
        handles.second->hide();
    }
    for (const auto& item : m_selectableOffsetEdges) {
        item->hide();
    }
    for (const auto& item : m_sectionControls) {
        item->hide();
    }
    if (m_startRotationHandle) {
        m_startRotationHandle->hide();
    }
    if (m_endRotationHandle) {
        m_endRotationHandle->hide();
    }
    if (m_startEndpointHandle) {
        m_startEndpointHandle->hide();
    }
    if (m_endEndpointHandle) {
        m_endEndpointHandle->hide();
    }
}

void TaskSectionView::moveSectionEndpoint(
    bool startEndpoint,
    const QPointF& scenePoint,
    Qt::KeyboardModifiers modifiers)
{
    if (!m_baseItem) {
        return;
    }

    const auto path = sectionPathPoints(false);
    if (path.size() < 2) {
        return;
    }
    const QPointF endpoint = startEndpoint
        ? path.front().first : path.back().first;
    QPointF inwardPoint = startEndpoint
        ? path[1].first : path[path.size() - 2].first;
    if (QPointF::dotProduct(inwardPoint - endpoint,
                            inwardPoint - endpoint) < 1.0e-12) {
        inwardPoint = m_temporaryCenter;
    }
    QPointF outward = endpoint - inwardPoint;
    const double length = std::hypot(outward.x(), outward.y());
    if (length < 1.0e-8) {
        return;
    }
    outward /= length;

    QPointF local = m_baseItem->mapFromScene(scenePoint);
    local = snapViewPoint(
        local, modifiers.testFlag(Qt::ControlModifier));
    const double outwardDelta =
        QPointF::dotProduct(local - endpoint, outward);

    constexpr double minimumSegmentLength = 1.0e-3;
    if (startEndpoint) {
        double upper = -minimumSegmentLength;
        for (const OffsetStep& step : m_offsetSteps) {
            upper = std::min(upper, step.along - minimumSegmentLength);
            if (step.transition == OffsetStep::Transition::Arc) {
                upper = std::min(upper,
                                 step.startAlong - minimumSegmentLength);
            }
        }
        for (const ArcBend& bend : m_arcBends) {
            upper = std::min(upper, bend.along - minimumSegmentLength);
        }
        m_temporaryStartAlong = std::min(
            m_temporaryStartAlong - outwardDelta, upper);
    }
    else {
        double lower = minimumSegmentLength;
        for (const OffsetStep& step : m_offsetSteps) {
            lower = std::max(lower, step.along + minimumSegmentLength);
            if (step.transition == OffsetStep::Transition::Arc) {
                lower = std::max(lower,
                                 step.startAlong + minimumSegmentLength);
            }
        }
        for (const ArcBend& bend : m_arcBends) {
            lower = std::max(lower, bend.along + minimumSegmentLength);
        }
        m_temporaryEndAlong = std::max(
            m_temporaryEndAlong + outwardDelta, lower);
    }
    m_endpointsEdited = true;
    if (startEndpoint) {
        m_startEndpointEdited = true;
    }
    else {
        m_endEndpointEdited = true;
    }
    drawTemporarySingleOffset();
    showUnappliedPreview(false);
    apply();
}

void TaskSectionView::moveSectionCenter(
    const QPointF& scenePoint,
    Qt::KeyboardModifiers modifiers)
{
    if (!m_baseItem || !m_base) {
        return;
    }
    const QPointF local = m_baseItem->mapFromScene(scenePoint);
    m_temporaryCenter =
        snapViewPoint(local, modifiers.testFlag(Qt::ControlModifier));
    setCenterUi(m_temporaryCenter);
    updateTemporarySectionLine(m_temporaryCenter, m_displayedDirection);
    showUnappliedPreview(false);
    apply();
}

void TaskSectionView::moveSingleOffsetP1(
    size_t index,
    const QPointF& scenePoint,
    Qt::KeyboardModifiers modifiers)
{
    if (index >= m_offsetSteps.size() || !m_baseItem) {
        return;
    }
    QPointF local = m_baseItem->mapFromScene(scenePoint);
    local = snapViewPoint(local, modifiers.testFlag(Qt::ControlModifier));
    const double angle = m_offsetSteps[index].along < 0.0
        ? -m_startRotation : -m_endRotation;
    const QPointF delta = local - m_temporaryCenter;
    local = m_temporaryCenter
        + QPointF(delta.x() * std::cos(angle) - delta.y() * std::sin(angle),
                  delta.x() * std::sin(angle) + delta.y() * std::cos(angle));
    const QPointF tangent(-m_displayedDirection.y, -m_displayedDirection.x);
    const double lower = index == 0
        ? m_temporaryStartAlong
        : m_offsetSteps[index - 1].along + 1.0e-6;
    const double upper = index + 1 == m_offsetSteps.size()
        ? m_temporaryEndAlong
        : m_offsetSteps[index + 1].along - 1.0e-6;
    const double along = std::clamp(
        QPointF::dotProduct(local - m_temporaryCenter, tangent),
        lower, upper);
    if (m_offsetSteps[index].transition == OffsetStep::Transition::Arc) {
        m_offsetSteps[index].startAlong = along;
    }
    else {
        m_offsetSteps[index].along = along;
    }
    drawTemporarySingleOffset();
    showUnappliedPreview(false);
    apply();
}

void TaskSectionView::moveSingleOffsetP2(
    size_t index,
    const QPointF& scenePoint,
    Qt::KeyboardModifiers modifiers)
{
    if (index >= m_offsetSteps.size() || !m_baseItem) {
        return;
    }
    QPointF local = m_baseItem->mapFromScene(scenePoint);
    local = snapViewPoint(local, modifiers.testFlag(Qt::ControlModifier));
    const double angle = m_offsetSteps[index].along < 0.0
        ? -m_startRotation : -m_endRotation;
    const QPointF delta = local - m_temporaryCenter;
    local = m_temporaryCenter
        + QPointF(delta.x() * std::cos(angle) - delta.y() * std::sin(angle),
                  delta.x() * std::sin(angle) + delta.y() * std::cos(angle));
    const QPointF tangent(-m_displayedDirection.y, -m_displayedDirection.x);
    const QPointF normal(m_displayedDirection.x, -m_displayedDirection.y);
    const QPointF p1 =
        m_temporaryCenter + tangent * m_offsetSteps[index].along;
    const double offset = QPointF::dotProduct(local - p1, normal);
    if (m_offsetSteps[index].towardStart) {
        if (index == 0) {
            m_startOffset = offset;
        }
        else {
            m_offsetSteps[index - 1].offset = offset;
        }
    }
    else {
        m_offsetSteps[index].offset = offset;
    }
    drawTemporarySingleOffset();
    showUnappliedPreview(false);
    apply();
}

void TaskSectionView::moveNotchP1(
    size_t index,
    const QPointF& scenePoint,
    Qt::KeyboardModifiers modifiers)
{
    if (index >= m_offsetSteps.size() || !m_baseItem) {
        return;
    }
    QPointF local = m_baseItem->mapFromScene(scenePoint);
    local = snapViewPoint(local, modifiers.testFlag(Qt::ControlModifier));
    local = mapFromSectionHalf(local, m_offsetSteps[index].towardStart);
    const QPointF tangent(-m_displayedDirection.y, -m_displayedDirection.x);
    const double lower = index == 0
        ? m_temporaryStartAlong
        : m_offsetSteps[index - 1].along + 1.0e-6;
    const double upper = index + 1 == m_offsetSteps.size()
        ? m_temporaryEndAlong
        : m_offsetSteps[index + 1].along - 1.0e-6;
    m_offsetSteps[index].along = std::clamp(
        QPointF::dotProduct(local - m_temporaryCenter, tangent),
        lower, upper);
    drawTemporarySingleOffset();
    showUnappliedPreview(false);
    apply();
}

void TaskSectionView::moveNotchP2(
    size_t index,
    const QPointF& scenePoint,
    Qt::KeyboardModifiers modifiers)
{
    if (index >= m_offsetSteps.size() || !m_baseItem) {
        return;
    }
    QPointF local = m_baseItem->mapFromScene(scenePoint);
    local = snapViewPoint(local, modifiers.testFlag(Qt::ControlModifier));
    local = mapFromSectionHalf(local, m_offsetSteps[index].towardStart);
    const QPointF tangent(-m_displayedDirection.y, -m_displayedDirection.x);
    const QPointF normal(m_displayedDirection.x, -m_displayedDirection.y);
    const QPointF anchor =
        m_temporaryCenter + tangent * m_offsetSteps[index].along;
    const double offset = QPointF::dotProduct(local - anchor, normal);
    if (m_offsetSteps[index].towardStart) {
        const int modifierId = m_offsetSteps[index].modifierId;
        auto outer = std::find_if(
            m_offsetSteps.begin(), m_offsetSteps.end(),
            [modifierId](const OffsetStep& step) {
                return step.modifierId == modifierId
                    && step.role == OffsetStep::Role::NotchOuter;
            });
        if (outer != m_offsetSteps.end()) {
            outer->offset = offset;
        }
    }
    else {
        m_offsetSteps[index].offset = offset;
    }
    drawTemporarySingleOffset();
    showUnappliedPreview(false);
    apply();
}

void TaskSectionView::moveArcBendPoint(
    size_t index,
    bool endpoint,
    const QPointF& scenePoint,
    Qt::KeyboardModifiers modifiers)
{
    if (!m_baseItem || index >= m_arcBends.size()) {
        return;
    }
    ArcBend& bend = m_arcBends[index];
    QPointF local = m_baseItem->mapFromScene(scenePoint);
    local = snapViewPoint(
        local, modifiers.testFlag(Qt::ControlModifier));

    if (endpoint) {
        const auto path = sectionPathPoints(false);
        auto firstArcPoint = std::find_if(
            path.begin(), path.end(), [&bend](const auto& point) {
                return std::abs(point.second - bend.along) < 1.0e-6;
            });
        if (firstArcPoint == path.end()) {
            return;
        }
        auto lastArcPoint = firstArcPoint;
        for (auto point = std::next(firstArcPoint);
             point != path.end()
                 && std::abs(point->second - bend.along) < 1.0e-6;
             ++point) {
            lastArcPoint = point;
        }
        const QPointF reference =
            (bend.towardStart ? firstArcPoint : lastArcPoint)->first
            - m_temporaryCenter;
        const QPointF target = local - m_temporaryCenter;
        if (reference.manhattanLength() < 1.0e-6
            || target.manhattanLength() < 1.0e-6) {
            return;
        }
        bend.angle += std::atan2(
            reference.x() * target.y() - reference.y() * target.x(),
            QPointF::dotProduct(reference, target));
    }
    else {
        local = mapFromSectionHalf(local, bend.towardStart);
        const QPointF tangent(
            -m_displayedDirection.y, -m_displayedDirection.x);
        const double along =
            QPointF::dotProduct(local - m_temporaryCenter, tangent);
        bend.along = bend.towardStart
            ? std::clamp(along, m_temporaryStartAlong, 0.0)
            : std::clamp(along, 0.0, m_temporaryEndAlong);
    }

    drawTemporarySingleOffset();
    showUnappliedPreview(false);
    apply();
}

void TaskSectionView::rotateSectionHalf(bool startHalf,
                                        const QPointF& scenePoint,
                                        Qt::KeyboardModifiers modifiers,
                                        bool commit)
{
    if (!m_baseItem) {
        return;
    }
    const bool snapAngle = modifiers.testFlag(Qt::ControlModifier);
    QPointF local = m_baseItem->mapFromScene(scenePoint);
    local = snapViewPoint(local, snapAngle);
    const QPointF reference =
        sectionHalfEndpoint(startHalf) - m_temporaryCenter;
    const QPointF target = local - m_temporaryCenter;
    if (reference.manhattanLength() < 1.0e-6
        || target.manhattanLength() < 1.0e-6) {
        if (commit) {
            apply();
        }
        return;
    }
    const double angleDelta = std::atan2(
        reference.x() * target.y() - reference.y() * target.x(),
        QPointF::dotProduct(reference, target));
    double& rotation = startHalf ? m_startRotation : m_endRotation;
    rotation += angleDelta;
    if (snapAngle) {
        constexpr double angleIncrement = std::numbers::pi / 36.0;
        rotation = std::round(rotation / angleIncrement) * angleIncrement;
    }
    drawTemporarySingleOffset();
    showUnappliedPreview(false);
    if (commit) {
        apply();
    }
}

QPointF TaskSectionView::snapViewPoint(
    const QPointF& point,
    bool disableSnapping) const
{
    if (disableSnapping || !m_graphicsView || !m_baseItem || !m_base) {
        return point;
    }

    return DrawGuiUtil::snapToViewGeometry(
        point, *m_base, *m_baseItem, *m_graphicsView);
}

Base::Vector3d TaskSectionView::viewPointToSectionOrigin(const QPointF& point) const
{
    if (!m_base) {
        return Base::Vector3d();
    }
    const Base::Vector3d centroid = m_base->getOriginalCentroid();
    const gp_Ax2 cs = m_base->getRotatedCS(centroid);
    const gp_Dir xAxis = cs.XDirection();
    const gp_Dir yAxis = cs.YDirection();
    const double scale = m_base->getScale();
    const double x = Rez::appX(point.x()) / scale;
    const double y = -Rez::appX(point.y()) / scale;
    return centroid
        + Base::Vector3d(xAxis.X(), xAxis.Y(), xAxis.Z()) * x
        + Base::Vector3d(yAxis.X(), yAxis.Y(), yAxis.Z()) * y;
}

QPointF TaskSectionView::sectionOriginToView(const Base::Vector3d& origin) const
{
    if (!m_base) {
        return {};
    }
    Base::Vector3d projected =
        m_base->projectPoint(origin - m_base->getOriginalCentroid());
    projected *= m_base->getScale();
    const double angle = Base::toRadians(m_base->Rotation.getValue());
    const double x = Rez::guiX(projected.x);
    const double y = Rez::guiX(projected.y);
    return {
        std::cos(angle) * x + std::sin(angle) * y,
        -std::sin(angle) * x + std::cos(angle) * y
    };
}

Base::Vector3d TaskSectionView::baseToDisplayedDirection(
    const Base::Vector3d& direction) const
{
    if (!m_base) {
        return direction;
    }
    const double angle = Base::toRadians(m_base->Rotation.getValue());
    return Base::Vector3d(
        std::cos(angle) * direction.x - std::sin(angle) * direction.y,
        std::sin(angle) * direction.x + std::cos(angle) * direction.y,
        0.0
    );
}

void TaskSectionView::updateTemporarySectionLineFromUi()
{
    if (!m_temporarySectionLine) {
        return;
    }
    if (m_customComplexSection) {
        updateCustomProfilePreview();
        return;
    }
    Base::Vector3d direction = m_viewDirectionWidget->value();
    if (direction.Sqr() <= std::numeric_limits<double>::epsilon()) {
        return;
    }
    direction.Normalize();
    updateTemporarySectionLine(centerFromUi(),
                               baseToDisplayedDirection(direction));
}

QPointF TaskSectionView::centerFromUi() const
{
    return {
        Rez::guiX(ui->sbCenterX->value().getValue()),
        -Rez::guiX(ui->sbCenterY->value().getValue())
    };
}

void TaskSectionView::setCenterUi(const QPointF& center)
{
    const QSignalBlocker blockX(ui->sbCenterX);
    const QSignalBlocker blockY(ui->sbCenterY);
    ui->sbCenterX->setValue(Rez::appX(center.x()));
    ui->sbCenterY->setValue(-Rez::appX(center.y()));
}

void TaskSectionView::setGeneratedSectionVisible(bool visible)
{
    if (!m_section || !m_baseItem) {
        return;
    }
    m_baseItem->setSectionLineVisible(m_section, visible);
    auto* pageScene = dynamic_cast<QGSPage*>(m_baseItem->scene());
    if (pageScene) {
        if (QGIView* sectionItem = pageScene->findQViewForDocObj(m_section)) {
            sectionItem->setVisible(visible);
        }
    }
}

void TaskSectionView::showGeneratedSectionWhenReady(int request)
{
    if (request != m_generationRequest || !m_section || !m_temporarySectionLine) {
        return;
    }
    if (m_section->waitingForResult()) {
        QTimer::singleShot(
            50, this, [this, request]() { showGeneratedSectionWhenReady(request); });
        return;
    }

    setGeneratedSectionVisible(true);
    if (m_base) {
        m_base->requestPaint();
    }
    // A completed recompute with no geometry is a final result. No generated
    // section line will be created, so keep the editable preview visible and
    // stop polling until another generation request is made.
    if (!m_section->hasGeometry()) {
        return;
    }
    if (!m_baseItem || !m_baseItem->hasSectionLine(m_section)) {
        QTimer::singleShot(
            50, this, [this, request]() { showGeneratedSectionWhenReady(request); });
        return;
    }
    m_temporarySectionLine->hide();
}

void TaskSectionView::showUnappliedPreview(bool updateFromUi)
{
    if (!m_temporarySectionLine) {
        return;
    }
    ++m_generationRequest;
    if (updateFromUi) {
        updateTemporarySectionLineFromUi();
    }
    m_temporarySectionLine->show();
    setGeneratedSectionVisible(false);
}

//the VectorEditWidget reports a change in direction
void TaskSectionView::slotViewDirectionChanged(Base::Vector3d newDirection)
{
    if (!m_base) {
        return;
    }
    Base::Vector3d projectedViewDirection = m_base->projectPoint(newDirection, false);
    projectedViewDirection.Normalize();
    double viewAngle = atan2(projectedViewDirection.y, projectedViewDirection.x);
    m_compass->setDialAngle(Base::toDegrees(viewAngle));
    directionChanged(true);
    showUnappliedPreview();
    applyAligned();
}

//the CompassWidget reports that the view direction angle has changed
void TaskSectionView::slotChangeAngle(double newAngle)
{
    double angleRadians = Base::toRadians(newAngle);
    double unitX = cos(angleRadians);
    double unitY = sin(angleRadians);
    Base::Vector3d localUnit(unitX, unitY, 0.0);
    m_viewDirectionWidget->setValueNoNotify(localUnit);
    directionChanged(true);
    showUnappliedPreview();
    applyAligned();
}

void TaskSectionView::reverseSectionDirection(double newAngle)
{
    const double angleRadians = Base::toRadians(newAngle);
    Base::Vector3d localUnit(std::cos(angleRadians),
                            std::sin(angleRadians), 0.0);
    m_viewDirectionWidget->setValueNoNotify(localUnit);

    if (m_customComplexSection) {
        directionChanged(true);
        showUnappliedPreview();
        applyAligned();
        return;
    }

    // Reversing the view direction also reverses both axes used to describe
    // a modern section path.  Re-express the modifiers in that new basis so
    // that the path stays fixed on the page.
    std::vector<double> precedingOffsets;
    precedingOffsets.reserve(m_offsetSteps.size());
    double finalOffset = m_startOffset;
    for (const OffsetStep& step : m_offsetSteps) {
        precedingOffsets.push_back(finalOffset);
        finalOffset = step.offset;
    }

    std::vector<OffsetStep> reversedSteps;
    reversedSteps.reserve(m_offsetSteps.size());
    for (size_t i = m_offsetSteps.size(); i-- > 0;) {
        OffsetStep step = m_offsetSteps[i];
        if (step.transition == OffsetStep::Transition::Arc) {
            const double oldAlong = step.along;
            step.along = -step.startAlong;
            step.startAlong = -oldAlong;
        }
        else {
            step.along = -step.along;
        }
        step.offset = -precedingOffsets[i];
        step.towardStart = !step.towardStart;
        reversedSteps.push_back(step);
    }
    std::sort(
        reversedSteps.begin(), reversedSteps.end(),
        [](const OffsetStep& left, const OffsetStep& right) {
            return left.along < right.along;
        });
    m_offsetSteps = std::move(reversedSteps);
    m_startOffset = -finalOffset;

    for (ArcBend& bend : m_arcBends) {
        bend.along = -bend.along;
        bend.towardStart = !bend.towardStart;
    }
    std::swap(m_startRotation, m_endRotation);
    std::swap(m_startEndpointEdited, m_endEndpointEdited);
    const double oldStartAlong = m_temporaryStartAlong;
    m_temporaryStartAlong = -m_temporaryEndAlong;
    m_temporaryEndAlong = -oldStartAlong;
    m_displayedDirection *= -1.0;

    // Offset handle callbacks encode which side of a modifier they edit, so
    // rebuild them after the sides have been exchanged.
    m_offsetHandles.clear();

    directionChanged(true);
    showUnappliedPreview(false);
    drawTemporarySingleOffset();
    if (m_sectionControlsEnabled) {
        showSectionHandles();
    }
    applyAligned();
}

void TaskSectionView::onIdentifierChanged()
{
    apply();
}

void TaskSectionView::onScaleChanged()
{
    m_scaleEdited = true;
    apply();

}

void TaskSectionView::onCenterXChanged()
{
    showUnappliedPreview();
    apply();
}
void TaskSectionView::onCenterYChanged()
{
    showUnappliedPreview();
    apply();
}
void TaskSectionView::scaleTypeChanged(int index)
{
    if (index == 0) {
        // Page Scale Type
        ui->sbScale->setEnabled(false);
        if (m_base->findParentPage()) {
            ui->sbScale->setValue(m_base->findParentPage()->Scale.getValue());
            ui->sbScale->setEnabled(false);
        }
    }
    else if (index == 1) {
        // Automatic Scale Type
        ui->sbScale->setEnabled(false);
        if (m_section) {
            ui->sbScale->setValue(m_section->autoScale());
        }
    }
    else if (index == 2) {
        // Custom Scale Type
        ui->sbScale->setEnabled(true);
        if (m_section) {
            ui->sbScale->setValue(m_section->Scale.getValue());
            ui->sbScale->setEnabled(true);
        }
    }
    else {
        return;
    }
}

void TaskSectionView::enableAll(bool enable)
{
    ui->leSymbol->setEnabled(enable);
    ui->sbScale->setEnabled(enable);
    ui->sbCenterX->setEnabled(enable);
    ui->sbCenterY->setEnabled(enable);
    ui->cmbScaleType->setEnabled(enable);
    ui->cbSectionCutOnly->setEnabled(enable);
    ui->cbShowOutsidePartialBoundaries->setEnabled(enable);
    ui->cbConnectionLine->setEnabled(enable);
    ui->gbOrientation->setEnabled(enable);
    ui->gbPlane->setEnabled(enable);
    QString qScaleType = ui->cmbScaleType->currentText();
    //Allow or prevent scale changing initially
    if (qScaleType == QStringLiteral("Custom")) {
        ui->sbScale->setEnabled(true);
    }
    else {
        ui->sbScale->setEnabled(false);
    }
}

void TaskSectionView::liveUpdateClicked()
{
    apply(true);
}

//******************************************************************************
bool TaskSectionView::apply(bool forceUpdate)
{
    if (!ui->cbLiveUpdate->isChecked() && !forceUpdate) {
        //nothing to do
        return false;
    }

    Gui::WaitCursor wc;
    if (!isBaseValid()) {
        return false;
    }
    if (m_customComplexSection) {
        if (!m_profileObject) {
            QMessageBox::warning(
                Gui::getMainWindow(), QObject::tr("Missing profile"),
                QObject::tr("Select a wire or sketch for the sketch based complex section."));
            return false;
        }
        Base::Vector3d localUnit = m_viewDirectionWidget->value();
        if (localUnit.Sqr() <= std::numeric_limits<double>::epsilon()
            || !TechDraw::DrawComplexSection::canBuild(
                m_base->localVectorToCS(localUnit), m_profileObject)) {
            QMessageBox::warning(
                Gui::getMainWindow(), QObject::tr("Invalid complex section"),
                QObject::tr("The selected profile and view direction cannot build a complex section."));
            return false;
        }
    }

    if (m_dirName.empty()) {
        //this should never happen
        std::string msg =
            tr("Nothing to apply. No section direction picked yet").toStdString();
        Base::Console().error((msg + "\n").c_str());
        return false;
    }
    convertSectionToComplex();
    if (!m_section) {
        m_section = createSectionView();
    }

    if (isSectionValid()) {
        updateSectionView();
    }
    else {
        failNoObject();
    }

    if (!m_section) {
        return false;
    }
    m_section->recomputeFeature();
    if (isBaseValid()) {
        m_base->requestPaint();
    }

    if (m_temporarySectionLine) {
        m_temporarySectionLine->show();
        setGeneratedSectionVisible(false);
        const int request = ++m_generationRequest;
        QTimer::singleShot(
            0, this, [this, request]() { showGeneratedSectionWhenReady(request); });
    }
    else {
        setGeneratedSectionVisible(true);
    }
    enableAll(true);

    wc.restoreCursor();
    return true;
}

void TaskSectionView::applyQuick(std::string dir)
{
    m_dirName = dir;
    enableAll(true);
    apply();
}

void TaskSectionView::applyAligned()
{
    m_dirName = "Aligned";
    enableAll(true);
    apply();
}

bool TaskSectionView::hasComplexPath() const
{
    return (m_customComplexSection && m_profileObject)
        || isGeneratedProfile(m_profileObject)
        || !m_offsetSteps.empty()
        || !m_arcBends.empty()
        || m_endpointsEdited
        || std::abs(m_startRotation) > 1.0e-8
        || std::abs(m_endRotation) > 1.0e-8;
}

bool TaskSectionView::hasAlignedPath() const
{
    if (m_customComplexSection) {
        return ui->cmbStrategy->currentIndex() == 1;
    }
    return std::abs(m_startRotation) > 1.0e-8
        || std::abs(m_endRotation) > 1.0e-8
        || !m_arcBends.empty()
        || std::any_of(
            m_offsetSteps.begin(), m_offsetSteps.end(),
            [](const OffsetStep& step) {
                return step.transition == OffsetStep::Transition::Arc;
            });
}

bool TaskSectionView::hasBentAxis() const
{
    return !m_customComplexSection
        && (std::abs(m_startRotation) > 1.0e-8
            || std::abs(m_endRotation) > 1.0e-8);
}

void TaskSectionView::updateParallelToVisibility()
{
    ui->parallelToWidget->setVisible(hasBentAxis());
    updatePartialDisplayVisibility();
}

void TaskSectionView::updatePartialDisplayVisibility()
{
    bool partial = m_halfSection;
    if (!partial && m_endpointsEdited) {
        const auto status = partialEndpointStatus(
            sectionPathPoints(false));
        partial = status.first || status.second;
    }
    ui->cbShowOutsidePartialBoundaries->setVisible(partial);
}

//*********************************************************************

TechDraw::DrawViewSection* TaskSectionView::createSectionView(void)
{
    if (!isBaseValid()) {
        failNoObject();
        return nullptr;
    }

    std::string baseName = m_base->getNameInDocument();

    if (!m_section) {
        if (hasComplexPath()) {
            createOffsetProfile();
        }
        const std::string objectName(
            hasComplexPath() ? "ComplexSection" : "SectionView");
        m_sectionName = m_base->getDocument()->getUniqueObjectName(objectName.c_str());
        Command::doCommand(
            Command::Doc,
            "App.ActiveDocument.addObject('%s', '%s')",
            hasComplexPath() ? "TechDraw::DrawComplexSection"
                             : "TechDraw::DrawViewSection",
            m_sectionName.c_str());

        // section labels (Section A-A) are not unique, and are not the same as the object name (SectionView)
        // we pluck the generated suffix from the object name and append it to "Section" to generate
        // unique Labels
        QString qTemp = ui->leSymbol->text();
        if (m_page
            && qTemp == QString::fromStdString(
                m_page->getNextViewIdentifier(false))) {
            m_page->getNextViewIdentifier();
        }
        std::string temp = Base::Tools::escapeEncodeString(qTemp.toStdString());
        std::string sectionLabel = Base::Tools::escapeEncodeString(makeSectionLabel(qTemp));
        const std::string rawSectionCaption =
            m_captionForConversion.empty()
            ? makeSectionCaption(qTemp).toStdString()
            : m_captionForConversion;
        const std::string sectionCaption =
            Base::Tools::escapeEncodeString(rawSectionCaption);
        Command::doCommand(Command::Doc, "App.ActiveDocument.%s.SectionSymbol = '%s'",
                           m_sectionName.c_str(), temp.c_str());
        Command::doCommand(Command::Doc,
                           "App.ActiveDocument.%s.Caption = '%s'",
                           m_sectionName.c_str(), sectionCaption.c_str());
        m_captionForConversion.clear();

        Command::doCommand(Command::Doc, "App.ActiveDocument.%s.Label = '%s'",
                           m_sectionName.c_str(),
                           sectionLabel.c_str());
        Command::doCommand(Command::Doc, "App.activeDocument().%s.translateLabel('DrawViewSection', 'Section', '%s')",
              m_sectionName.c_str(), sectionLabel.c_str());


        Command::doCommand(Command::Doc, "App.ActiveDocument.%s.addView(App.ActiveDocument.%s)",
                           m_savePageName.c_str(), m_sectionName.c_str());
        Command::doCommand(Command::Doc, "App.ActiveDocument.%s.BaseView = App.ActiveDocument.%s",
                           m_sectionName.c_str(), baseName.c_str());
        Command::doCommand(Command::Doc,
                           "App.ActiveDocument.%s.Source = App.ActiveDocument.%s.Source",
                           m_sectionName.c_str(), baseName.c_str());
        if (hasComplexPath()) {
            Command::doCommand(
                Command::Doc,
                "App.ActiveDocument.%s.SectionOrigin = FreeCAD.Vector(0.0, 0.0, 0.0)",
                m_sectionName.c_str());
        }
        else {
            const Base::Vector3d origin =
                viewPointToSectionOrigin(centerFromUi());
            Command::doCommand(
                Command::Doc,
                "App.ActiveDocument.%s.SectionOrigin = FreeCAD.Vector(%.6f, %.6f, %.6f)",
                m_sectionName.c_str(), origin.x, origin.y, origin.z);
        }

        Command::doCommand(Command::Doc, "App.ActiveDocument.%s.Scale = %0.7f",
                           m_sectionName.c_str(), ui->sbScale->value());

        int scaleType = ui->cmbScaleType->currentIndex();
        Command::doCommand(Command::Doc, "App.ActiveDocument.%s.ScaleType = %d",
                           m_sectionName.c_str(), scaleType);
        Command::doCommand(Command::Doc, "App.ActiveDocument.%s.SectionDirection = '%s'",
                           m_sectionName.c_str(), m_dirName.c_str());
        Command::doCommand(Command::Doc,
                           "App.ActiveDocument.%s.SectionCutOnly = %s",
                           m_sectionName.c_str(),
                           ui->cbSectionCutOnly->isChecked() ? "True" : "False");
        Command::doCommand(
            Command::Doc,
            "App.ActiveDocument.%s.ShowOutsidePartialBoundaries = %s",
            m_sectionName.c_str(),
            ui->cbShowOutsidePartialBoundaries->isChecked()
                ? "True" : "False");
        Command::doCommand(Command::Doc,
                           "App.ActiveDocument.%s.ConnectionLine = %s",
                           m_sectionName.c_str(),
                           ui->cbConnectionLine->isChecked()
                               ? "True" : "False");

        App::DocumentObject* newObj = m_base->getDocument()->getObject(m_sectionName.c_str());
        m_section = dynamic_cast<TechDraw::DrawViewSection*>(newObj);
        if (!newObj || !m_section) {
            throw Base::RuntimeError("TaskSectionView - new section object not found");
        }
        if (hasComplexPath()) {
            auto* complexSection = dynamic_cast<TechDraw::DrawComplexSection*>(m_section);
            App::DocumentObject* profile =
                m_base->getDocument()->getObject(m_profileName.c_str());
            if (!complexSection || !profile) {
                throw Base::RuntimeError("TaskSectionView - complex section profile not found");
            }
            m_profileObject = profile;
            complexSection->CuttingToolWireObject.setValue(profile);
            complexSection->ProjectionStrategy.setValue(
                m_customComplexSection
                    ? static_cast<long>(ui->cmbStrategy->currentIndex())
                    : (hasAlignedPath() ? 1L : 0L));
            complexSection->SectionViewAlignment.setValue(
                static_cast<long>(ui->cmbParallelTo->currentIndex()));
            if (m_customComplexSection) {
                complexSection->Source.setValues(m_shapes);
                complexSection->XSource.setValues(m_xShapes);
            }
        }
        Base::Vector3d localUnit = m_viewDirectionWidget->value();
        localUnit.Normalize();
        if (m_dirName == "Aligned") {
            //localUnit is a view direction so we need to reverse it to make a
            //section normal
            m_section->setCSFromBase(localUnit * -1.0);
        }
        else {
            //Note: DirectionName is to be deprecated in the future
            m_section->setCSFromBase(m_dirName.c_str());
        }
        //auto orientation of view relative to base view
        const double rotation = sectionViewRotation();
        Command::doCommand(Command::Doc, "App.ActiveDocument.%s.Rotation = %.6f",
                           m_sectionName.c_str(), rotation);
    }
    return m_section;
}

std::pair<bool, bool> TaskSectionView::partialEndpointStatus(
    const std::vector<std::pair<QPointF, double>>& pathPoints) const
{
    if (!m_baseItem || !m_base || pathPoints.size() < 2) {
        return {false, false};
    }

    PathBuilder pathBuilder(m_baseItem);
    auto endpointIsPartial = [&](bool startEndpoint) {
        const QPointF endpoint = startEndpoint
            ? pathPoints.front().first : pathPoints.back().first;
        QPointF inwardPoint = startEndpoint
            ? pathPoints[1].first
            : pathPoints[pathPoints.size() - 2].first;
        if (QPointF::dotProduct(inwardPoint - endpoint,
                                inwardPoint - endpoint) < 1.0e-12) {
            inwardPoint = m_temporaryCenter;
        }
        QPointF outward = endpoint - inwardPoint;
        const double outwardLength = std::hypot(outward.x(), outward.y());
        if (outwardLength < 1.0e-8) {
            return true;
        }
        outward /= outwardLength;

        const QTransform projectAlongOutward(
            outward.x(), -outward.y(),
            outward.y(), outward.x(), 0.0, 0.0);
        const QPointF projectedEndpoint =
            projectAlongOutward.map(endpoint);
        const double rayY = projectedEndpoint.y();
        const double intersectionTolerance = Rez::guiX(0.01);
        double furthestIntersection = std::numeric_limits<double>::lowest();
        for (const BaseGeomPtr& geometry : m_base->getEdgeGeometry()) {
            if (!geometry
                || geometry->source() != SourceType::GEOMETRY) {
                continue;
            }
            const QPainterPath edgePath =
                pathBuilder.geomToPainterPath(geometry, 0.0);
            if (edgePath.isEmpty()) {
                continue;
            }
            const QPainterPath projectedPath =
                projectAlongOutward.map(edgePath);
            const QList<QPolygonF> polylines =
                projectedPath.toSubpathPolygons();
            for (const QPolygonF& polyline : polylines) {
                for (qsizetype i = 1; i < polyline.size(); ++i) {
                    const QPointF first = polyline[i - 1];
                    const QPointF second = polyline[i];
                    const double minimumY =
                        std::min(first.y(), second.y());
                    const double maximumY =
                        std::max(first.y(), second.y());
                    if (rayY < minimumY - intersectionTolerance
                        || rayY > maximumY + intersectionTolerance) {
                        continue;
                    }
                    const double deltaY = second.y() - first.y();
                    if (std::abs(deltaY) <= intersectionTolerance) {
                        if (std::abs(rayY - first.y())
                            <= intersectionTolerance) {
                            furthestIntersection = std::max(
                                furthestIntersection,
                                std::max(first.x(), second.x()));
                        }
                        continue;
                    }
                    const double ratio = std::clamp(
                        (rayY - first.y()) / deltaY, 0.0, 1.0);
                    const double intersectionX =
                        first.x() + ratio * (second.x() - first.x());
                    furthestIntersection = std::max(
                        furthestIntersection, intersectionX);
                }
            }
        }

        const double endpointPosition = projectedEndpoint.x();
        const double endpointTolerance = Rez::guiX(0.1);
        const bool hasOutwardIntersection =
            furthestIntersection > std::numeric_limits<double>::lowest();
        return hasOutwardIntersection
            && endpointPosition
                < furthestIntersection - endpointTolerance;
    };

    return {endpointIsPartial(true), endpointIsPartial(false)};
}

void TaskSectionView::createOffsetProfile()
{
    if (m_customComplexSection || !hasComplexPath() || !m_base
        || !m_profileName.empty()) {
        return;
    }
    m_profileName =
        m_base->getDocument()->getUniqueObjectName("SectionProfile");
    Command::doCommand(Command::Doc, "import Part");
    Command::doCommand(
        Command::Doc,
        "App.ActiveDocument.addObject('Part::Feature', '%s')",
        m_profileName.c_str());
    updateOffsetProfile();
    Command::doCommand(
        Command::Gui,
        "Gui.ActiveDocument.getObject('%s').Visibility = False",
        m_profileName.c_str());
}

void TaskSectionView::updateOffsetProfile()
{
    if (m_customComplexSection
        || m_profileName.rfind("SectionProfile", 0) != 0
        || !hasComplexPath()
        || m_profileName.empty() || !m_base) {
        return;
    }

    auto pathPoints = sectionPathPoints(false);
    // The endpoint closes the cutting tool with a plane perpendicular to its
    // outermost profile segment. It is partial exactly when projected source
    // geometry exists beyond that plane. Use the support of the real projected
    // edges, rather than the rectangular view bounds, so curved outlines and
    // rotated section halves are classified correctly.
    const auto partialStatus = partialEndpointStatus(pathPoints);
    if (m_halfSection) {
        m_startEndpointEdited = m_halfSectionTowardStart
            ? partialStatus.first : true;
        m_endEndpointEdited = m_halfSectionTowardStart
            ? true : partialStatus.second;
        m_endpointsEdited = true;
    }
    else {
        m_startEndpointEdited = partialStatus.first;
        m_endEndpointEdited = partialStatus.second;
        m_endpointsEdited = partialStatus.first || partialStatus.second;
    }
    updatePartialDisplayVisibility();
    const bool straightPartialPath =
        m_endpointsEdited && m_offsetSteps.empty() && m_arcBends.empty()
        && std::abs(m_startRotation) <= 1.0e-8
        && std::abs(m_endRotation) <= 1.0e-8;
    if (straightPartialPath && pathPoints.size() > 2) {
        // sectionPathPoints() includes the center so each half can be
        // rotated independently.  For an unrotated partial section that
        // point is redundant, and keeping it splits the coplanar cut face
        // into two faces with a visible seam at the center.
        pathPoints = {pathPoints.front(), pathPoints.back()};
    }

    QPainterPath projectedMaterial;
    if (m_baseItem && m_base) {
        PathBuilder pathBuilder(m_baseItem);
        for (const FacePtr& face : m_base->getFaceGeometry()) {
            QPainterPath facePath;
            for (const Wire* wire : face->wires) {
                QPainterPath wirePath;
                for (const BaseGeomPtr& geometry : wire->geoms) {
                    const QPainterPath edgePath =
                        pathBuilder.geomToPainterPath(geometry, 0.0);
                    if (wirePath.isEmpty()) {
                        wirePath = edgePath;
                    }
                    else {
                        wirePath.connectPath(edgePath);
                    }
                }
                facePath.addPath(wirePath);
            }
            facePath.setFillRule(Qt::OddEvenFill);
            projectedMaterial = projectedMaterial.united(facePath);
        }
    }
    auto segmentContributesToView = [&](const QPointF& first,
                                        const QPointF& second) {
        if (projectedMaterial.isEmpty()) {
            return true;
        }
        QPainterPath segment;
        segment.moveTo(first);
        segment.lineTo(second);
        QPainterPathStroker stroker;
        stroker.setWidth(Rez::guiX(0.01));
        const QPainterPath segmentBand = stroker.createStroke(segment);
        return projectedMaterial.intersects(segmentBand)
            || projectedMaterial.contains(first)
            || projectedMaterial.contains(second)
            || projectedMaterial.contains((first + second) * 0.5);
    };

    std::vector<long> transitionEdges;
    std::vector<long> collapsedEdges;
    auto isTransitionEdge = [this](double firstAlong,
                                   double secondAlong) {
        // A sharp offset connector changes the cutting-plane position but
        // does not represent a plane that contributes area to an aligned
        // section view. Both of its endpoints have the same along value.
        if (std::abs(firstAlong - secondAlong) < 1.0e-6) {
            return true;
        }
        for (const ArcBend& bend : m_arcBends) {
            if (std::abs(firstAlong - bend.along) < 1.0e-6
                && std::abs(secondAlong - bend.along) < 1.0e-6) {
                return true;
            }
        }
        for (const OffsetStep& step : m_offsetSteps) {
            if (step.transition != OffsetStep::Transition::Arc) {
                continue;
            }
            const double lower =
                std::min(step.startAlong, step.along);
            const double upper =
                std::max(step.startAlong, step.along);
            if (firstAlong >= lower - 1.0e-6
                && firstAlong <= upper + 1.0e-6
                && secondAlong >= lower - 1.0e-6
                && secondAlong <= upper + 1.0e-6) {
                return true;
            }
        }
        return false;
    };
    for (size_t i = 1; i < pathPoints.size(); ++i) {
        const bool contributes = segmentContributesToView(
            pathPoints[i - 1].first, pathPoints[i].first);
        const bool collapsed = isTransitionEdge(
            pathPoints[i - 1].second, pathPoints[i].second);
        const bool transition = collapsed || !contributes;
        if (transition) {
            transitionEdges.push_back(
                static_cast<long>(i - 1));
        }
        if (collapsed) {
            collapsedEdges.push_back(
                static_cast<long>(i - 1));
        }
    }

    std::vector<Base::Vector3d> points;
    points.reserve(pathPoints.size());
    for (const auto& point : pathPoints) {
        points.push_back(viewPointToSectionOrigin(point.first));
    }

    std::ostringstream command;
    command.precision(12);
    command << "App.ActiveDocument." << m_profileName
            << ".Shape = Part.makePolygon([";
    for (size_t i = 0; i < points.size(); ++i) {
        if (i > 0) {
            command << ", ";
        }
        command << "FreeCAD.Vector(" << points[i].x << ", "
                << points[i].y << ", " << points[i].z << ")";
    }
    command << "])";
    Command::doCommand(Command::Doc, command.str().c_str());

    App::DocumentObject* profile =
        m_base->getDocument()->getObject(m_profileName.c_str());
    if (!profile) {
        return;
    }
    auto* transitionProperty =
        dynamic_cast<App::PropertyIntegerList*>(
            profile->getPropertyByName(
                "SectionTransitionEdges"));
    if (!transitionProperty) {
        transitionProperty =
            dynamic_cast<App::PropertyIntegerList*>(
                profile->addDynamicProperty(
                    "App::PropertyIntegerList",
                    "SectionTransitionEdges",
                    "Section",
                    "Profile edges that change cutting direction without contributing view area",
                    App::Prop_Hidden));
    }
    if (transitionProperty) {
        transitionProperty->setValues(
            std::move(transitionEdges));
    }
    auto* collapsedProperty =
        dynamic_cast<App::PropertyIntegerList*>(
            profile->getPropertyByName(
                "SectionCollapsedEdges"));
    if (!collapsedProperty) {
        collapsedProperty =
            dynamic_cast<App::PropertyIntegerList*>(
                profile->addDynamicProperty(
                    "App::PropertyIntegerList",
                    "SectionCollapsedEdges",
                    "Section",
                    "Profile edges that do not consume distance in the unfolded section view",
                    App::Prop_Hidden));
    }
    if (collapsedProperty) {
        collapsedProperty->setValues(
            std::move(collapsedEdges));
    }
    auto* partialProperty = dynamic_cast<App::PropertyBool*>(
        profile->getPropertyByName("PartialSection"));
    if (!partialProperty) {
        partialProperty = dynamic_cast<App::PropertyBool*>(
            profile->addDynamicProperty(
                "App::PropertyBool", "PartialSection", "Section",
                "Limit the section view to the editable profile endpoints",
                App::Prop_Hidden));
    }
    if (partialProperty) {
        partialProperty->setValue(m_endpointsEdited);
    }
    auto* alignedCenterProperty = dynamic_cast<App::PropertyVector*>(
        profile->getPropertyByName("AlignedSectionCenter"));
    if (!alignedCenterProperty) {
        alignedCenterProperty = dynamic_cast<App::PropertyVector*>(
            profile->addDynamicProperty(
                "App::PropertyVector", "AlignedSectionCenter", "Section",
                "Profile point separating the two aligned section halves",
                App::Prop_Hidden));
    }
    if (alignedCenterProperty && !pathPoints.empty()) {
        const auto centerPoint = std::min_element(
            pathPoints.begin(), pathPoints.end(),
            [](const auto& left, const auto& right) {
                return std::abs(left.second) < std::abs(right.second);
            });
        alignedCenterProperty->setValue(
            viewPointToSectionOrigin(centerPoint->first));
    }
    auto setEndpointProperty = [profile](const char* name,
                                         const char* description,
                                         bool value) {
        auto* property = dynamic_cast<App::PropertyBool*>(
            profile->getPropertyByName(name));
        if (!property) {
            property = dynamic_cast<App::PropertyBool*>(
                profile->addDynamicProperty(
                    "App::PropertyBool", name, "Section", description,
                    App::Prop_Hidden));
        }
        if (property) {
            property->setValue(value);
        }
    };
    setEndpointProperty("PartialSectionStart",
                        "The start endpoint limits the section view",
                        m_startEndpointEdited);
    setEndpointProperty("PartialSectionEnd",
                        "The end endpoint limits the section view",
                        m_endEndpointEdited);
    setEndpointProperty("HalfSection",
                        "The profile represents a half section",
                        m_halfSection);
    setEndpointProperty("HalfSectionTowardStart",
                        "The half section retains the start side",
                        m_halfSectionTowardStart);
    auto* halfLeaderProperty = dynamic_cast<App::PropertyVector*>(
        profile->getPropertyByName("HalfSectionLeaderEnd"));
    if (!halfLeaderProperty) {
        halfLeaderProperty = dynamic_cast<App::PropertyVector*>(
            profile->addDynamicProperty(
                "App::PropertyVector", "HalfSectionLeaderEnd", "Section",
                "Outer endpoint of the half-section center leader",
                App::Prop_Hidden));
    }
    if (halfLeaderProperty) {
        halfLeaderProperty->setValue(
            viewPointToSectionOrigin(halfSectionLeaderEnd()));
    }
    auto* stateProperty = dynamic_cast<App::PropertyString*>(
        profile->getPropertyByName("ModernSectionData"));
    if (!stateProperty) {
        stateProperty = dynamic_cast<App::PropertyString*>(
            profile->addDynamicProperty(
                "App::PropertyString",
                "ModernSectionData",
                "Section",
                "Editable state for a modern complex section",
                App::Prop_Hidden));
    }
    if (stateProperty) {
        stateProperty->setValue(serializeModernProfileState());
        m_hasModernProfileState = true;
        m_modernCenterOrigin =
            viewPointToSectionOrigin(m_temporaryCenter);
    }
}

std::string TaskSectionView::serializeModernProfileState() const
{
    const Base::Vector3d centerOrigin =
        viewPointToSectionOrigin(m_temporaryCenter);
    std::ostringstream state;
    state.precision(std::numeric_limits<double>::max_digits10);
    state << 5 << ' ' << centerOrigin.x << ' ' << centerOrigin.y << ' '
          << centerOrigin.z << ' ' << m_displayedDirection.x << ' '
          << m_displayedDirection.y << ' ' << m_displayedDirection.z << ' '
          << m_temporaryStartAlong << ' ' << m_temporaryEndAlong << ' '
          << static_cast<int>(m_endpointsEdited) << ' '
          << static_cast<int>(m_startEndpointEdited) << ' '
          << static_cast<int>(m_endEndpointEdited) << ' '
          << static_cast<int>(m_halfSection) << ' '
          << static_cast<int>(m_halfSectionTowardStart) << ' '
          << m_startOffset << ' ' << m_startRotation << ' ' << m_endRotation
          << ' ' << m_nextModifierId << ' ' << m_offsetSteps.size();
    for (const OffsetStep& step : m_offsetSteps) {
        state << ' ' << step.along << ' ' << step.offset << ' '
              << static_cast<int>(step.transition) << ' ' << step.startAlong
              << ' ' << static_cast<int>(step.role) << ' ' << step.modifierId
              << ' ' << static_cast<int>(step.towardStart);
    }
    state << ' ' << m_arcBends.size();
    for (const ArcBend& bend : m_arcBends) {
        state << ' ' << bend.along << ' ' << bend.angle << ' '
              << static_cast<int>(bend.towardStart);
    }
    return state.str();
}

namespace
{

struct SerializedOffsetStep
{
    double along{0.0};
    double offset{0.0};
    int transition{0};
    double startAlong{0.0};
    int role{0};
    int modifierId{0};
    bool towardStart{false};
};

struct SerializedArcBend
{
    double along{0.0};
    double angle{0.0};
    bool towardStart{false};
};

struct ModernProfileState
{
    Base::Vector3d centerOrigin;
    Base::Vector3d displayedDirection{1.0, 0.0, 0.0};
    double startAlong{0.0};
    double endAlong{0.0};
    bool endpointsEdited{false};
    bool startEndpointEdited{false};
    bool endEndpointEdited{false};
    bool halfSection{false};
    bool halfSectionTowardStart{false};
    double startOffset{0.0};
    double startRotation{0.0};
    double endRotation{0.0};
    int nextModifierId{1};
    std::vector<SerializedOffsetStep> offsetSteps;
    std::vector<SerializedArcBend> arcBends;
};

std::unique_ptr<ModernProfileState>
parseModernProfileState(const std::string& serialized)
{
    constexpr std::size_t maximumModifierCount = 10000;
    auto restored = std::make_unique<ModernProfileState>();
    std::istringstream input(serialized);
    int version = 0;
    std::size_t offsetCount = 0;
    std::size_t arcCount = 0;
    if (!(input >> version)
        || (version < 1 || version > 5)
        || !(input >> restored->centerOrigin.x >> restored->centerOrigin.y
                   >> restored->centerOrigin.z >> restored->displayedDirection.x
                   >> restored->displayedDirection.y
                   >> restored->displayedDirection.z)) {
        return {};
    }

    if (version == 1) {
        double halfLength = 0.0;
        if (!(input >> halfLength)) {
            return {};
        }
        restored->startAlong = -halfLength;
        restored->endAlong = halfLength;
    }
    else if (!(input >> restored->startAlong >> restored->endAlong)) {
        return {};
    }

    if (version >= 3) {
        int endpointsEdited = 0;
        if (!(input >> endpointsEdited)
            || (endpointsEdited != 0 && endpointsEdited != 1)) {
            return {};
        }
        restored->endpointsEdited = endpointsEdited != 0;
    }

    if (version >= 4) {
        int startEdited = 0;
        int endEdited = 0;
        if (!(input >> startEdited >> endEdited)
            || (startEdited != 0 && startEdited != 1)
            || (endEdited != 0 && endEdited != 1)) {
            return {};
        }
        restored->startEndpointEdited = startEdited != 0;
        restored->endEndpointEdited = endEdited != 0;
    }
    else {
        restored->startEndpointEdited = restored->endpointsEdited;
        restored->endEndpointEdited = restored->endpointsEdited;
    }

    if (version >= 5) {
        int halfSection = 0;
        int towardStart = 0;
        if (!(input >> halfSection >> towardStart)
            || (halfSection != 0 && halfSection != 1)
            || (towardStart != 0 && towardStart != 1)) {
            return {};
        }
        restored->halfSection = halfSection != 0;
        restored->halfSectionTowardStart = towardStart != 0;
    }

    if (!(input >> restored->startOffset >> restored->startRotation
                >> restored->endRotation >> restored->nextModifierId
                >> offsetCount)
        || offsetCount > maximumModifierCount) {
        return {};
    }
    restored->offsetSteps.reserve(offsetCount);
    for (std::size_t i = 0; i < offsetCount; ++i) {
        SerializedOffsetStep step;
        int towardStart = 0;
        if (!(input >> step.along >> step.offset >> step.transition
                    >> step.startAlong >> step.role >> step.modifierId
                    >> towardStart)
            || step.transition < 0 || step.transition > 1
            || step.role < 0 || step.role > 2) {
            return {};
        }
        step.towardStart = towardStart != 0;
        restored->offsetSteps.push_back(step);
    }

    if (!(input >> arcCount) || arcCount > maximumModifierCount) {
        return {};
    }
    restored->arcBends.reserve(arcCount);
    for (std::size_t i = 0; i < arcCount; ++i) {
        SerializedArcBend bend;
        int towardStart = 0;
        if (!(input >> bend.along >> bend.angle >> towardStart)) {
            return {};
        }
        bend.towardStart = towardStart != 0;
        restored->arcBends.push_back(bend);
    }
    input >> std::ws;
    if (!input.eof()) {
        return {};
    }
    return restored;
}

}  // namespace

bool TaskSectionView::restoreModernProfileState()
{
    if (!isGeneratedProfile(m_profileObject)) {
        return false;
    }
    const auto* stateProperty = dynamic_cast<const App::PropertyString*>(
        m_profileObject->getPropertyByName("ModernSectionData"));
    if (!stateProperty || stateProperty->getStrValue().empty()) {
        return false;
    }

    auto restored = parseModernProfileState(stateProperty->getValue());
    if (!restored) {
        return false;
    }

    std::vector<OffsetStep> offsetSteps;
    offsetSteps.reserve(restored->offsetSteps.size());
    for (const SerializedOffsetStep& serialized : restored->offsetSteps) {
        OffsetStep step;
        step.along = serialized.along;
        step.offset = serialized.offset;
        step.transition = static_cast<OffsetStep::Transition>(
            serialized.transition);
        step.startAlong = serialized.startAlong;
        step.role = static_cast<OffsetStep::Role>(serialized.role);
        step.modifierId = serialized.modifierId;
        step.towardStart = serialized.towardStart;
        offsetSteps.push_back(step);
    }
    std::vector<ArcBend> arcBends;
    arcBends.reserve(restored->arcBends.size());
    for (const SerializedArcBend& serialized : restored->arcBends) {
        arcBends.push_back(
            {serialized.along, serialized.angle, serialized.towardStart});
    }

    // Commit only after the complete payload has been validated. A malformed
    // property must leave the task in the state initialized by setUiEdit().
    m_modernCenterOrigin = restored->centerOrigin;
    m_displayedDirection = restored->displayedDirection;
    m_temporaryStartAlong = restored->startAlong;
    m_temporaryEndAlong = restored->endAlong;
    m_endpointsEdited = restored->endpointsEdited;
    m_startEndpointEdited = restored->startEndpointEdited;
    m_endEndpointEdited = restored->endEndpointEdited;
    m_halfSection = restored->halfSection;
    m_halfSectionTowardStart = restored->halfSectionTowardStart;
    m_startOffset = restored->startOffset;
    m_startRotation = restored->startRotation;
    m_endRotation = restored->endRotation;
    m_nextModifierId = restored->nextModifierId;
    m_offsetSteps = std::move(offsetSteps);
    m_arcBends = std::move(arcBends);
    m_hasModernProfileState = true;

    // The generated profile stores the direction as it is displayed on the
    // page.  Keep the base-view direction controls synchronized with that
    // value so a later apply does not replace it with the opposite section
    // normal reconstructed by setUiEdit().
    const double angle =
        Base::toRadians(-m_base->Rotation.getValue());
    Base::Vector3d baseDirection(
        std::cos(angle) * m_displayedDirection.x
            - std::sin(angle) * m_displayedDirection.y,
        std::sin(angle) * m_displayedDirection.x
            + std::cos(angle) * m_displayedDirection.y,
        0.0);
    if (baseDirection.Sqr() > std::numeric_limits<double>::epsilon()) {
        baseDirection.Normalize();
        m_viewDirectionWidget->setValueNoNotify(baseDirection);
        const QSignalBlocker blockCompass(m_compass);
        m_compass->setDialAngle(
            Base::toDegrees(std::atan2(baseDirection.y,
                                      baseDirection.x)));
    }
    return true;
}

void TaskSectionView::convertSectionToComplex()
{
    if (!hasComplexPath() || !m_section
        || m_section->isDerivedFrom<TechDraw::DrawComplexSection>()) {
        return;
    }

    setGeneratedSectionVisible(true);
    m_captionForConversion = m_section->Caption.getValue();
    const std::string oldSectionName = m_section->getNameInDocument();
    Command::doCommand(
        Command::Doc,
        "App.ActiveDocument.%s.removeView(App.ActiveDocument.%s)",
        m_savePageName.c_str(), oldSectionName.c_str());
    Command::doCommand(
        Command::Doc,
        "App.ActiveDocument.removeObject('%s')",
        oldSectionName.c_str());
    m_section = nullptr;
    m_sectionName.clear();
}

void TaskSectionView::updateSectionView()
{
    if (!isSectionValid()) {
        failNoObject();
        return;
    }

    const std::string objectName("SectionView");
    std::string baseName = m_base->getNameInDocument();

    if (m_section) {
        if (hasComplexPath()) {
            if (!m_customComplexSection
                && !isGeneratedProfile(m_profileObject)) {
                m_profileName.clear();
                createOffsetProfile();
                m_profileObject =
                    m_base->getDocument()->getObject(
                        m_profileName.c_str());
            }
            updateOffsetProfile();
            if (auto* complexSection =
                    dynamic_cast<TechDraw::DrawComplexSection*>(m_section)) {
                complexSection->ProjectionStrategy.setValue(
                    m_customComplexSection
                        ? static_cast<long>(ui->cmbStrategy->currentIndex())
                        : (hasAlignedPath() ? 1L : 0L));
                complexSection->SectionViewAlignment.setValue(
                    static_cast<long>(ui->cmbParallelTo->currentIndex()));
                if (!m_customComplexSection && m_profileObject) {
                    complexSection->CuttingToolWireObject.setValue(
                        m_profileObject);
                }
                if (m_customComplexSection) {
                    complexSection->CuttingToolWireObject.setValue(
                        m_profileObject);
                    complexSection->Source.setValues(m_shapes);
                    complexSection->XSource.setValues(m_xShapes);
                }
            }
        }
        if (m_section->BaseView.getValue() != m_base) {
            Command::doCommand(Command::Doc,
                               "App.ActiveDocument.%s.BaseView = App.ActiveDocument.%s",
                               m_sectionName.c_str(), baseName.c_str());
            Command::doCommand(Command::Doc,
                               "App.ActiveDocument.%s.Source = App.ActiveDocument.%s.Source",
                               m_sectionName.c_str(), baseName.c_str());
        }
        Command::doCommand(Command::Doc, "App.ActiveDocument.%s.SectionDirection = '%s'",
                           m_sectionName.c_str(), m_dirName.c_str());
        Command::doCommand(Command::Doc,
                           "App.ActiveDocument.%s.SectionCutOnly = %s",
                           m_sectionName.c_str(),
                           ui->cbSectionCutOnly->isChecked() ? "True" : "False");
        Command::doCommand(
            Command::Doc,
            "App.ActiveDocument.%s.ShowOutsidePartialBoundaries = %s",
            m_sectionName.c_str(),
            ui->cbShowOutsidePartialBoundaries->isChecked()
                ? "True" : "False");
        Command::doCommand(Command::Doc,
                           "App.ActiveDocument.%s.ConnectionLine = %s",
                           m_sectionName.c_str(),
                           ui->cbConnectionLine->isChecked()
                               ? "True" : "False");
        if (!hasComplexPath()) {
            const Base::Vector3d origin =
                viewPointToSectionOrigin(centerFromUi());
            Command::doCommand(
                Command::Doc,
                "App.ActiveDocument.%s.SectionOrigin = FreeCAD.Vector(%.6f, %.6f, %.6f)",
                m_sectionName.c_str(), origin.x, origin.y, origin.z);
        }

        QString qTemp = ui->leSymbol->text();
        const QString oldSymbol = QString::fromUtf8(
            m_section->SectionSymbol.getValue());
        const QString oldCaption = QString::fromUtf8(
            m_section->Caption.getValue());
        const bool updateDefaultCaption = oldCaption.isEmpty()
            || oldCaption == makeSectionCaption(oldSymbol);
        std::string temp = Base::Tools::escapeEncodeString(qTemp.toStdString());
        std::string sectionLabel = Base::Tools::escapeEncodeString(makeSectionLabel(qTemp));
        Command::doCommand(Command::Doc, "App.ActiveDocument.%s.SectionSymbol = '%s'",
                           m_sectionName.c_str(), temp.c_str());
        if (updateDefaultCaption) {
            const std::string sectionCaption =
                Base::Tools::escapeEncodeString(
                    makeSectionCaption(qTemp).toStdString());
            Command::doCommand(Command::Doc,
                               "App.ActiveDocument.%s.Caption = '%s'",
                               m_sectionName.c_str(),
                               sectionCaption.c_str());
        }

        Command::doCommand(Command::Doc, "App.ActiveDocument.%s.Label = '%s'",
                           m_sectionName.c_str(),
                           sectionLabel.c_str());
        Command::doCommand(Command::Doc, "App.activeDocument().%s.translateLabel('DrawViewSection', 'Section', '%s')",
              m_sectionName.c_str(), sectionLabel.c_str());

        Command::doCommand(Command::Doc, "App.ActiveDocument.%s.Scale = %0.7f",
                           m_sectionName.c_str(), ui->sbScale->value());

        int scaleType = ui->cmbScaleType->currentIndex();
        Command::doCommand(Command::Doc, "App.ActiveDocument.%s.ScaleType = %d",
                           m_sectionName.c_str(), scaleType);
        Command::doCommand(Command::Doc, "App.ActiveDocument.%s.SectionDirection = '%s'",
                           m_sectionName.c_str(), m_dirName.c_str());
        Base::Vector3d localUnit = m_viewDirectionWidget->value();
        localUnit.Normalize();
        if (m_dirName == "Aligned") {
            //localUnit is a view direction so we need to reverse it to make a
            //section normal
            m_section->setCSFromBase(localUnit * -1.0);
        }
        else {
            //Note: DirectionName is to be deprecated in the future
            m_section->setCSFromBase(m_dirName.c_str());
        }

        //auto orientation of view relative to base view
        if (directionChanged() || hasBentAxis()) {
            const double rotation = sectionViewRotation();
            Command::doCommand(Command::Doc, "App.ActiveDocument.%s.Rotation = %.6f",
                               m_sectionName.c_str(), rotation);
            directionChanged(false);
        }
    }
}

std::string TaskSectionView::makeSectionLabel(QString symbol)
{
    std::string temp = symbol.toStdString();
    return "Section " + temp + "-" + temp;
}

QString TaskSectionView::makeSectionCaption(const QString& symbol) const
{
    return tr("Section %1 - %1").arg(symbol);
}

void TaskSectionView::failNoObject(void)
{
    QString qsectionName = QString::fromStdString(m_sectionName);
    QString qbaseName = QString::fromStdString(m_baseName);
    QString msg = tr("Can not continue. Object * %1 or %2 not found.").arg(qsectionName, qbaseName);
    QMessageBox::critical(Gui::getMainWindow(), QObject::tr("Operation Failed"), msg);
    Gui::Control().closeDialog();
}

bool TaskSectionView::isBaseValid()
{
    if (!m_base)
        return false;

    App::DocumentObject* baseObj = m_doc->getObject(m_saveBaseName.c_str());
    if (!baseObj)
        return false;

    return true;
}

bool TaskSectionView::isSectionValid()
{
    if (!m_section)
        return false;

    App::DocumentObject* sectionObj = m_doc->getObject(m_sectionName.c_str());
    if (!sectionObj)
        return false;

    return true;
}

//get required rotation from input angle in [0, 360]
double TaskSectionView::requiredRotation(double inputAngle) const
{
    double rotation = inputAngle - 90.0;
    if (rotation == 180.0) {
        //if the view direction is 90/270, then the section is drawn properly and no
        //rotation is needed.  90.0 becomes 0.0, but 270.0 needs special handling.
        rotation = 0.0;
    }
    return rotation;
}

double TaskSectionView::sectionViewRotation() const
{
    const double initialRotation =
        requiredRotation(m_compass->positiveValue());
    if (!hasBentAxis()) {
        return initialRotation;
    }

    switch (ui->cmbParallelTo->currentIndex()) {
        case 0:
            return initialRotation - Base::toDegrees(m_endRotation);
        case 1:
            return initialRotation - Base::toDegrees(m_startRotation);
        case 2:
            return 0.0;
        case 3:
            return 90.0;
        default:
            return initialRotation;
    }
}

//******************************************************************************

bool TaskSectionView::accept()
{
    if (!apply(true)) {
        return false;
    }
    setGeneratedSectionVisible(true);
    Gui::Command::commitCommand(m_transactionId);
    m_transactionId = 0;
    Gui::Command::doCommand(Gui::Command::Gui, "Gui.ActiveDocument.resetEdit()");
    return true;
}

bool TaskSectionView::reject()
{
    if (m_section) {
        setGeneratedSectionVisible(true);
    }
    Gui::Command::abortCommand(m_transactionId);
    m_transactionId = 0;
    m_section = nullptr;
    m_profileObject = nullptr;
    if (isBaseValid()) {
        m_base->requestPaint();
    }
    Gui::Command::updateActive();
    Gui::Command::doCommand(Gui::Command::Gui, "Gui.ActiveDocument.resetEdit()");

    return false;
}

void TaskSectionView::changeEvent(QEvent* event)
{
    if (event->type() == QEvent::LanguageChange) {
        ui->retranslateUi(this);
    }
}

namespace
{

class SectionViewHandler final : public TechDrawHandler
{
public:
    explicit SectionViewHandler(TaskSectionView* task) : m_task(task) {}

    ~SectionViewHandler() override
    {
        clearPreview();
    }

    void mouseMoveEvent(QMouseEvent* event) override
    {
        if (m_completed) {
            event->accept();
            return;
        }
        if (!viewPage || !m_task) {
            return;
        }

        const QPointF scenePoint = viewPage->mapToScene(event->pos());
        if (m_mode == Mode::SeekCenter) {
            QGIViewPart* hovered = findView(event->pos());
            if (hovered != m_baseItem) {
                clearPreview();
                m_baseItem = hovered;
                m_base = hovered
                    ? dynamic_cast<DrawViewPart*>(hovered->getViewObject())
                    : nullptr;
                m_task->setInteractiveBase(m_base);
                if (m_baseItem) {
                    makePreview();
                }
            }

            if (!m_baseItem || !m_base) {
                return;
            }

            if (m_task->positionMode()
                == TaskSectionView::PositionMode::SketchBased) {
                event->accept();
                return;
            }

            const QPointF localPoint = m_baseItem->mapFromScene(scenePoint);
            m_center = event->modifiers().testFlag(Qt::ControlModifier)
                ? localPoint
                : snapPoint(localPoint);
            if (m_task->positionMode()
                == TaskSectionView::PositionMode::Horizontal) {
                m_direction = Base::Vector3d(0.0, 1.0, 0.0);
            }
            else if (m_task->positionMode()
                     == TaskSectionView::PositionMode::Vertical) {
                m_direction = Base::Vector3d(1.0, 0.0, 0.0);
            }
            if (m_task->positionMode()
                == TaskSectionView::PositionMode::TwoPointsPartial) {
                updateFirstEndpointPreview(m_center, m_direction);
            }
            else if (m_task->positionMode()
                     == TaskSectionView::PositionMode::CenterAndPoint) {
                updateHalfPreview(m_center, m_direction);
            }
            else {
                updatePreview(m_center, m_direction);
            }
            m_task->setInteractiveParameters(
                toSectionOrigin(m_center),
                toBaseDirection(m_direction));
        }
        else {
            QPointF localPoint = m_baseItem->mapFromScene(scenePoint);
            Base::Vector3d lineDirection;
            const DirectionSnap snap = preselectedDirectionSnap(
                event->pos(), localPoint, lineDirection);
            const TaskSectionView::PositionMode positionMode =
                m_task->positionMode();
            if (m_mode == Mode::SeekSecondDirection) {
                if (positionMode
                    == TaskSectionView::PositionMode::CenterAndPoint) {
                    QPointF requestedDirection = localPoint - m_center;
                    const double requestedLength = std::hypot(
                        requestedDirection.x(), requestedDirection.y());
                    if (requestedLength < 1.0) {
                        return;
                    }
                    requestedDirection /= requestedLength;
                    const QPointF currentDirection(
                        m_direction.x, -m_direction.y);
                    if (QPointF::dotProduct(requestedDirection,
                                            currentDirection) < 0.0) {
                        m_direction *= -1.0;
                    }
                    m_task->updateTemporaryHalfSection(
                        m_center, m_secondPoint, m_direction);
                    m_task->setInteractiveParameters(
                        m_lockedOrigin, toBaseDirection(m_direction));
                    event->accept();
                    return;
                }
                const QPointF delta = localPoint - m_center;
                if (std::hypot(delta.x(), delta.y()) < 1.0) {
                    return;
                }

                QPointF target;
                if (snap == DirectionSnap::Line) {
                    target = QPointF(lineDirection.x, -lineDirection.y);
                    if (QPointF::dotProduct(target, delta) < 0.0) {
                        target *= -1.0;
                    }
                }
                else {
                    target = delta / std::hypot(delta.x(), delta.y());
                    if (event->modifiers().testFlag(Qt::ControlModifier)) {
                        double angle = Base::toDegrees(
                            std::atan2(target.y(), target.x()));
                        angle = std::round(angle / 5.0) * 5.0;
                        const double radians = Base::toRadians(angle);
                        target = QPointF(std::cos(radians), std::sin(radians));
                    }
                }

                const QPointF firstTangent(-m_direction.y, -m_direction.x);
                const QPointF reference = -firstTangent;
                const double rotation = std::atan2(
                    reference.x() * target.y() - reference.y() * target.x(),
                    QPointF::dotProduct(reference, target));
                m_task->setTemporaryHalfRotations(rotation, 0.0);
                m_task->updateTemporaryRotatedSectionEndpoint(
                    localPoint, true);
                event->accept();
                return;
            }
            if (positionMode == TaskSectionView::PositionMode::Horizontal
                || positionMode == TaskSectionView::PositionMode::Vertical) {
                const QPointF delta = localPoint - m_center;
                if (positionMode == TaskSectionView::PositionMode::Horizontal) {
                    if (std::abs(delta.y()) < 1.0) {
                        return;
                    }
                    m_direction = Base::Vector3d(
                        0.0, delta.y() < 0.0 ? 1.0 : -1.0, 0.0);
                }
                else {
                    if (std::abs(delta.x()) < 1.0) {
                        return;
                    }
                    m_direction = Base::Vector3d(
                        delta.x() < 0.0 ? -1.0 : 1.0, 0.0, 0.0);
                }
                updatePreview(m_center, m_direction);
                m_task->setInteractiveParameters(
                    m_lockedOrigin, toBaseDirection(m_direction));
                event->accept();
                return;
            }
            if (snap == DirectionSnap::Line
                && positionMode
                    == TaskSectionView::PositionMode::
                        CenterAndViewDirection) {
                // preselectedDirectionSnap returns the normal used by the
                // point-based construction modes. Center and view direction
                // needs the view direction itself: updatePreview will derive
                // the perpendicular section line from it.
                m_direction = Base::Vector3d(
                    -lineDirection.y, lineDirection.x, 0.0);
                const QPointF displayedDirection(
                    m_direction.x, -m_direction.y);
                if (QPointF::dotProduct(
                        displayedDirection, localPoint - m_center) < 0.0) {
                    m_direction *= -1.0;
                }
                updatePreview(m_center, m_direction);
                m_task->setInteractiveParameters(
                    m_lockedOrigin, toBaseDirection(m_direction));
                event->accept();
                return;
            }

            const QPointF reference =
                (positionMode
                    == TaskSectionView::PositionMode::TwoPoints
                 || positionMode
                    == TaskSectionView::PositionMode::TwoPointsPartial)
                ? m_firstPoint : m_center;
            QPointF delta = localPoint - reference;
            if (std::hypot(delta.x(), delta.y()) < 1.0) {
                return;
            }

            if (positionMode
                == TaskSectionView::PositionMode::
                    CenterAndViewDirection) {
                double angle =
                    Base::toDegrees(std::atan2(-delta.y(), delta.x()));
                if (event->modifiers().testFlag(
                        Qt::ControlModifier)) {
                    angle = std::round(angle / 5.0) * 5.0;
                }
                const double radians = Base::toRadians(angle);
                m_direction = Base::Vector3d(
                    std::cos(radians), std::sin(radians), 0.0);
                updatePreview(m_center, m_direction);
                m_task->setInteractiveParameters(
                    m_lockedOrigin, toBaseDirection(m_direction));
                event->accept();
                return;
            }

            QPointF tangent;
            if (snap == DirectionSnap::Line) {
                tangent = QPointF(
                    lineDirection.x, -lineDirection.y);
                if (QPointF::dotProduct(tangent, delta) < 0.0) {
                    tangent *= -1.0;
                }
            }
            else {
                tangent = delta
                    / std::hypot(delta.x(), delta.y());
                if (event->modifiers().testFlag(
                        Qt::ControlModifier)) {
                    double angle = Base::toDegrees(
                        std::atan2(tangent.y(), tangent.x()));
                    angle = std::round(angle / 5.0) * 5.0;
                    const double radians = Base::toRadians(angle);
                    tangent = QPointF(
                        std::cos(radians), std::sin(radians));
                }
            }

            if (positionMode
                    == TaskSectionView::PositionMode::TwoPoints
                || positionMode
                    == TaskSectionView::PositionMode::TwoPointsPartial) {
                double length = QPointF::dotProduct(delta, tangent);
                if (length < 0.0) {
                    tangent *= -1.0;
                    length *= -1.0;
                }
                const QPointF secondPoint =
                    m_firstPoint + tangent * length;
                m_center = (m_firstPoint + secondPoint) * 0.5;
                m_lockedOrigin = toSectionOrigin(m_center);
                tangent *= -1.0;
                m_direction = Base::Vector3d(
                    -tangent.y(), -tangent.x(), 0.0);
                if (positionMode
                    == TaskSectionView::PositionMode::TwoPointsPartial) {
                    m_task->updateTemporarySectionLineFromEndpoints(
                        m_firstPoint, secondPoint, m_direction);
                }
                else {
                    m_task->updateTemporarySectionLine(
                        m_center, m_direction);
                }
            }
            else {
                m_direction = Base::Vector3d(
                    -tangent.y(), -tangent.x(), 0.0);
                if (positionMode
                        == TaskSectionView::PositionMode::CenterAndPoint) {
                    m_secondPoint = localPoint;
                    m_task->updateTemporaryHalfSection(
                        m_center, localPoint, m_direction);
                }
                else if (positionMode
                        == TaskSectionView::PositionMode::
                            CenterAndTwoPoints) {
                    m_task->updateTemporarySectionLineEndpoint(
                        m_center, localPoint, m_direction, false);
                }
                else {
                    updatePreview(m_center, m_direction);
                }
            }
            m_task->setInteractiveParameters(
                m_lockedOrigin, toBaseDirection(m_direction));
        }
        event->accept();
    }

    void mousePressEvent(QMouseEvent* event) override
    {
        if (event->button() != Qt::LeftButton || !m_baseItem || !m_base) {
            return;
        }

        if (m_mode == Mode::SeekCenter) {
            if (m_task->positionMode()
                == TaskSectionView::PositionMode::SketchBased) {
                m_completed = true;
                m_task->adoptSketchBasedBase(
                    m_preview.release(), m_baseItem);
                event->accept();
                viewPage->deactivateHandler();  // Deletes this handler.
                return;
            }
            m_mode = Mode::SeekDirection;
            m_firstPoint = m_center;
            m_lockedOrigin = toSectionOrigin(m_center);
            m_task->adoptTemporarySectionLine(m_preview.release(), m_baseItem);
            if (m_task->positionMode()
                    != TaskSectionView::PositionMode::TwoPoints
                && m_task->positionMode()
                    != TaskSectionView::PositionMode::TwoPointsPartial) {
                if (m_task->positionMode()
                    == TaskSectionView::PositionMode::CenterAndPoint) {
                    const QPointF tangent(-m_direction.y, -m_direction.x);
                    m_task->updateTemporaryHalfSection(
                        m_center, m_center + tangent, m_direction);
                }
                else {
                    m_task->updateTemporarySectionLine(
                        m_center, m_direction);
                }
            }
            m_task->setDirectionControlsVisible(true);
            m_task->setInteractiveParameters(m_lockedOrigin, toBaseDirection(m_direction));
            updateHint();
        }
        else {
            // A click is not guaranteed to be preceded by a mouse-move at
            // precisely the same coordinates. Re-evaluate the construction
            // state here to prevent a stale endpoint/rotation from collapsing
            // one half of a center-based line to the center point.
            mouseMoveEvent(event);
            if (m_mode == Mode::SeekDirection
                && (m_task->positionMode()
                        == TaskSectionView::PositionMode::CenterAndTwoPoints
                    || m_task->positionMode()
                        == TaskSectionView::PositionMode::CenterAndPoint)) {
                m_mode = Mode::SeekSecondDirection;
                updateHint();
                event->accept();
                return;
            }
            m_task->setInteractiveParameters(m_lockedOrigin, toBaseDirection(m_direction));
            m_task->setSectionControlsEnabled(true);
            m_task->showSectionHandles();
            m_task->setPositioningActive(false);
            m_completed = true;
            event->accept();
            viewPage->deactivateHandler();  // Deletes this handler.
            return;
        }
        event->accept();
    }

    void mouseReleaseEvent(QMouseEvent* event) override
    {
        if (event->button() == Qt::RightButton) {
            if (m_mode == Mode::SeekCenter) {
                event->accept();
                QTimer::singleShot(
                    0, Gui::getMainWindow(),
                    []() { Gui::Control().closeDialog(); });
                return;
            }

            m_task->resetInteractivePreview();
            clearPreview();
            m_baseItem = nullptr;
            m_base = nullptr;
            m_center = {};
            m_firstPoint = {};
            m_lockedOrigin = Base::Vector3d();
            m_direction = Base::Vector3d(1.0, 0.0, 0.0);
            m_mode = Mode::SeekCenter;
            updateHint();
            event->accept();
            return;
        }
        if (event->button() == Qt::LeftButton) {
            viewPage->getScene()->clearSelection();
            Gui::Selection().clearSelection();
            event->accept();
            return;
        }
        TechDrawHandler::mouseReleaseEvent(event);
    }

    void keyPressEvent(QKeyEvent* event) override
    {
        Q_UNUSED(event);
    }

    void deactivate() override
    {
        clearPreview();
        if (!m_completed && m_mode == Mode::SeekCenter && m_task) {
            m_task->setInteractiveBase(nullptr);
        }
        TechDrawHandler::deactivate();
    }

private:
    enum class Mode
    {
        SeekCenter,
        SeekDirection,
        SeekSecondDirection
    };

    enum class DirectionSnap
    {
        None,
        Point,
        Line
    };

    std::list<Gui::InputHint> getToolHints() const override
    {
        using enum Gui::InputHint::UserInput;
        if (m_mode == Mode::SeekCenter) {
            QString action = QObject::tr("%1 place section-line center");
            if (m_task) {
                if (m_task->positionMode()
                    == TaskSectionView::PositionMode::SketchBased) {
                    action = QObject::tr(
                        "%1 select base view for sketch-based section");
                }
                else if (m_task->positionMode()
                             == TaskSectionView::PositionMode::TwoPoints
                         || m_task->positionMode()
                             == TaskSectionView::PositionMode::TwoPointsPartial) {
                    action = QObject::tr(
                        "%1 place first section-line point");
                }
            }
            return {
                {action, {MouseLeft}},
                {QObject::tr("%1 move without snapping"),
                 {{ModifierCtrl, MouseMove}}},
                {QObject::tr("%1 close section view tool"), {MouseRight}},
            };
        }
        if (m_mode == Mode::SeekSecondDirection) {
            if (m_task && m_task->positionMode()
                == TaskSectionView::PositionMode::CenterAndPoint) {
                return {
                    {QObject::tr("%1 choose half-section direction"),
                     {MouseLeft}},
                    {QObject::tr("%1 restart section-line placement"),
                     {MouseRight}},
                };
            }
            return {
                {QObject::tr("%1 place second section-line point"), {MouseLeft}},
                {QObject::tr("%1 snap direction angle"),
                 {{ModifierCtrl, MouseMove}}},
                {QObject::tr("%1 restart section-line placement"), {MouseRight}},
            };
        }
        QString action = QObject::tr("%1 confirm section-line direction");
        if (m_task) {
            switch (m_task->positionMode()) {
                case TaskSectionView::PositionMode::Horizontal:
                    action = QObject::tr("%1 choose up or down section direction");
                    break;
                case TaskSectionView::PositionMode::Vertical:
                    action = QObject::tr("%1 choose left or right section direction");
                    break;
                case TaskSectionView::PositionMode::CenterAndPoint:
                    action = QObject::tr("%1 place half-section endpoint");
                    break;
                case TaskSectionView::PositionMode::CenterAndTwoPoints:
                    action = QObject::tr("%1 place first section-line point");
                    break;
                case TaskSectionView::PositionMode::TwoPoints:
                case TaskSectionView::PositionMode::TwoPointsPartial:
                    action = QObject::tr("%1 place second section-line point");
                    break;
                default:
                    break;
            }
        }
        return {
            {action, {MouseLeft}},
            {QObject::tr("%1 snap direction angle"),
             {{ModifierCtrl, MouseMove}}},
            {QObject::tr("%1 restart section-line placement"), {MouseRight}},
        };
    }

    QString getCrosshairCursorSVGName() const override
    {
        return QStringLiteral("TechDraw_SectionView_Pointer");
    }

    QGIViewPart* findView(const QPoint& viewportPoint)
    {
        const QList<QGraphicsItem*> items = viewPage->items(viewportPoint);
        for (QGraphicsItem* item : items) {
            for (QGraphicsItem* parent = item; parent; parent = parent->parentItem()) {
                auto* view = dynamic_cast<QGIViewPart*>(parent);
                if (!view || !view->isVisible()) {
                    continue;
                }
                auto* base = dynamic_cast<DrawViewPart*>(view->getViewObject());
                if (base && base->findParentPage() == getPage()) {
                    return view;
                }
            }
        }
        return nullptr;
    }

    QPointF snapPoint(const QPointF& point) const
    {
        return DrawGuiUtil::snapToViewGeometry(
            point, *m_base, *m_baseItem, *viewPage);
    }

    DirectionSnap preselectedDirectionSnap(
        const QPoint& viewportPoint,
        QPointF& localPoint,
        Base::Vector3d& lineDirection) const
    {
        QGIEdge* preselectedEdge = nullptr;
        const QList<QGraphicsItem*> items =
            viewPage->items(viewportPoint);
        auto belongsToBase = [this](QGraphicsItem* item) {
            for (QGraphicsItem* parent = item;
                 parent; parent = parent->parentItem()) {
                if (parent == m_baseItem) {
                    return true;
                }
            }
            return false;
        };

        for (QGraphicsItem* item : items) {
            if (!belongsToBase(item)) {
                continue;
            }
            if (auto* vertex = dynamic_cast<QGIVertex*>(item)) {
                localPoint = m_baseItem->mapFromScene(
                    vertex->mapToScene(
                        vertex->boundingRect().center()));
                return DirectionSnap::Point;
            }
            if (!preselectedEdge) {
                preselectedEdge = dynamic_cast<QGIEdge*>(item);
            }
        }

        if (!preselectedEdge) {
            return DirectionSnap::None;
        }
        const auto edgeGeometry = m_base->getEdgeGeometry();
        const int edgeIndex = preselectedEdge->getProjIndex();
        if (edgeIndex >= 0
            && static_cast<size_t>(edgeIndex) < edgeGeometry.size()) {
            const BaseGeomPtr& geometry = edgeGeometry[edgeIndex];
            if (geometry->getGeomType() == GeomType::CIRCLE
                || geometry->getGeomType() == GeomType::ARCOFCIRCLE) {
                const Base::Vector3d center =
                    std::static_pointer_cast<TechDraw::Circle>(
                        geometry)->center;
                localPoint = QPointF(
                    Rez::guiX(center.x), Rez::guiX(center.y));
                return DirectionSnap::Point;
            }
        }

        const QPainterPath edgePath = preselectedEdge->path();
        if (edgePath.elementCount() != 2
            || !edgePath.elementAt(0).isMoveTo()
            || !edgePath.elementAt(1).isLineTo()) {
            return DirectionSnap::None;
        }

        const auto first = edgePath.elementAt(0);
        const auto second = edgePath.elementAt(1);
        const QPointF firstLocal = m_baseItem->mapFromScene(
            preselectedEdge->mapToScene(
                QPointF(first.x, first.y)));
        const QPointF secondLocal = m_baseItem->mapFromScene(
            preselectedEdge->mapToScene(
                QPointF(second.x, second.y)));
        QPointF edgeDirection = secondLocal - firstLocal;
        const double edgeLength =
            std::hypot(edgeDirection.x(), edgeDirection.y());
        if (edgeLength < 1.0e-6) {
            return DirectionSnap::None;
        }
        edgeDirection /= edgeLength;

        lineDirection = Base::Vector3d(
            -edgeDirection.y(), -edgeDirection.x(), 0.0);
        const QPointF arrowDirection(
            lineDirection.x, -lineDirection.y);
        if (QPointF::dotProduct(
                arrowDirection, localPoint - m_center) < 0.0) {
            lineDirection *= -1.0;
        }
        return DirectionSnap::Line;
    }

    Base::Vector3d toSectionOrigin(const QPointF& localPoint) const
    {
        const Base::Vector3d centroid = m_base->getOriginalCentroid();
        const gp_Ax2 cs = m_base->getRotatedCS(centroid);
        const gp_Dir xAxis = cs.XDirection();
        const gp_Dir yAxis = cs.YDirection();
        const double scale = m_base->getScale();
        const double x = Rez::appX(localPoint.x()) / scale;
        const double y = -Rez::appX(localPoint.y()) / scale;
        return centroid
            + Base::Vector3d(xAxis.X(), xAxis.Y(), xAxis.Z()) * x
            + Base::Vector3d(yAxis.X(), yAxis.Y(), yAxis.Z()) * y;
    }

    Base::Vector3d toBaseDirection(const Base::Vector3d& displayedDirection) const
    {
        const double angle = Base::toRadians(-m_base->Rotation.getValue());
        return Base::Vector3d(
            std::cos(angle) * displayedDirection.x - std::sin(angle) * displayedDirection.y,
            std::sin(angle) * displayedDirection.x + std::cos(angle) * displayedDirection.y,
            0.0);
    }

    void makePreview()
    {
        m_preview = makeSceneItem<QGISectionLine>(*viewPage->getScene());
        m_preview->setSymbol(const_cast<char*>(""));
        m_preview->setPathMode(false);
        m_preview->setSectionColor(Qt::black);
        m_preview->setArrowColor(QColor(35, 95, 210));

        double lineWidth = 0.5;
        size_t lineStyle = 1;
        bool showLine = true;
        auto* vp = dynamic_cast<ViewProviderViewPart*>(
            Gui::Application::Instance->getViewProvider(m_base));
        if (vp) {
            lineWidth = vp->HiddenWidth.getValue();
            lineStyle = static_cast<size_t>(vp->SectionLineStyle.getValue());
            showLine = vp->IncludeCutLine.getValue();
        }
        LineGenerator lineGenerator;
        QPen pen = lineGenerator.getLinePen(lineStyle, lineWidth);
        pen.setColor(Qt::black);
        m_preview->setLinePen(pen);
        m_preview->setWidth(Rez::guiX(lineWidth));
        m_preview->setShowLine(showLine);
        m_preview->setArrowSize(Preferences::dimArrowSize());
        m_preview->setFont(QFont(), Preferences::dimFontSizeMM());
        m_preview->setZValue(1000.0);

    }

    void updatePreview(const QPointF& center, const Base::Vector3d& direction)
    {
        if (!m_preview) {
            m_task->updateTemporarySectionLine(center, direction);
            return;
        }
        const QPointF tangent(-direction.y, -direction.x);
        const auto bounds = sectionLineBounds(m_baseItem, center, direction);
        const QPointF localStart = center + tangent * bounds.first;
        const QPointF localEnd = center + tangent * bounds.second;
        const QPointF start = m_baseItem->mapToScene(localStart);
        const QPointF end = m_baseItem->mapToScene(localEnd);
        const QPointF arrowEnd =
            m_baseItem->mapToScene(center + QPointF(direction.x, -direction.y));
        const QPointF arrowStart = m_baseItem->mapToScene(center);
        const QPointF sceneDirection = arrowEnd - arrowStart;
        m_preview->setEnds(Base::Vector3d(start.x(), start.y(), 0.0),
                           Base::Vector3d(end.x(), end.y(), 0.0));
        m_preview->setDirection(sceneDirection.x(), -sceneDirection.y());
        m_preview->draw();
    }

    void updateFirstEndpointPreview(
        const QPointF& endpoint,
        const Base::Vector3d& direction)
    {
        if (!m_preview || !m_baseItem) {
            return;
        }

        QPointF tangent(-direction.y, -direction.x);
        const double tangentLength = std::hypot(
            tangent.x(), tangent.y());
        if (tangentLength
            <= std::numeric_limits<double>::epsilon()) {
            return;
        }
        tangent /= tangentLength;
        const auto bounds = sectionLineBounds(
            m_baseItem, endpoint, direction);
        const double length = bounds.second - bounds.first;
        // Keep the visual left end under the cursor while seeking the first
        // point. Reordering identical endpoints does not change the displayed
        // segment; the provisional far end must be placed on the opposite
        // side of the cursor. The committed Two points ordering is unchanged.
        const QPointF start = m_baseItem->mapToScene(
            endpoint - tangent * length);
        const QPointF end = m_baseItem->mapToScene(endpoint);
        const QPointF directionStart = end;
        const QPointF arrowEnd = m_baseItem->mapToScene(
            endpoint + QPointF(direction.x, -direction.y));
        const QPointF sceneDirection = arrowEnd - directionStart;
        m_preview->setEnds(
            Base::Vector3d(start.x(), start.y(), 0.0),
            Base::Vector3d(end.x(), end.y(), 0.0));
        m_preview->setDirection(
            sceneDirection.x(), -sceneDirection.y());
        m_preview->draw();
    }

    void updateHalfPreview(const QPointF& center,
                           const Base::Vector3d& direction)
    {
        if (!m_preview || !m_baseItem) {
            return;
        }
        QPointF tangent(-direction.y, -direction.x);
        const double tangentLength = std::hypot(tangent.x(), tangent.y());
        if (tangentLength <= std::numeric_limits<double>::epsilon()) {
            return;
        }
        tangent /= tangentLength;
        const auto bounds = sectionLineBounds(m_baseItem, center, direction);
        const QPointF retainedEnd = center + tangent * bounds.second;

        QPointF arrowDirection(direction.x, -direction.y);
        const double arrowLength = std::hypot(
            arrowDirection.x(), arrowDirection.y());
        if (arrowLength > std::numeric_limits<double>::epsilon()) {
            arrowDirection /= arrowLength;
        }
        QRectF viewBounds = m_baseItem->contentBoundingRect();
        if (viewBounds.isEmpty()) {
            viewBounds = m_baseItem->boundingRect();
        }
        double outside = std::numeric_limits<double>::max();
        const std::array<QPointF, 4> corners{
            viewBounds.topLeft(), viewBounds.topRight(),
            viewBounds.bottomLeft(), viewBounds.bottomRight()};
        for (const QPointF& corner : corners) {
            outside = std::min(outside,
                QPointF::dotProduct(corner - center, arrowDirection));
        }
        const QPointF leaderEnd = center
            + arrowDirection * (outside - Rez::guiX(10.0));
        const QPointF sceneCenter = m_baseItem->mapToScene(center);
        const QPointF sceneRetainedEnd =
            m_baseItem->mapToScene(retainedEnd);
        const QPointF sceneLeaderEnd = m_baseItem->mapToScene(leaderEnd);
        const QPointF sceneArrowEnd = m_baseItem->mapToScene(
            center + QPointF(direction.x, -direction.y));
        const QPointF sceneDirection = sceneArrowEnd - sceneCenter;

        QPainterPath path;
        path.moveTo(sceneCenter);
        path.lineTo(sceneRetainedEnd);
        path.moveTo(sceneCenter);
        path.lineTo(sceneLeaderEnd);
        m_preview->setPathMode(true);
        m_preview->setPath(path);
        m_preview->setEnds(
            Base::Vector3d(sceneLeaderEnd.x(), sceneLeaderEnd.y(), 0.0),
            Base::Vector3d(sceneRetainedEnd.x(), sceneRetainedEnd.y(), 0.0));
        m_preview->setArrowDirections(
            Base::Vector3d(sceneDirection.x(), -sceneDirection.y(), 0.0),
            Base::Vector3d(sceneDirection.x(), -sceneDirection.y(), 0.0));
        m_preview->draw();
    }

    void clearPreview()
    {
        m_preview.reset();
    }

    TaskSectionView* m_task{nullptr};
    QGIViewPart* m_baseItem{nullptr};
    DrawViewPart* m_base{nullptr};
    SceneItemPtr<QGISectionLine> m_preview;
    Mode m_mode{Mode::SeekCenter};
    QPointF m_center;
    QPointF m_firstPoint;
    QPointF m_secondPoint;
    Base::Vector3d m_lockedOrigin;
    Base::Vector3d m_direction{1.0, 0.0, 0.0};
    bool m_completed{false};
};

class SingleOffsetHandler final : public SectionOffsetHandler
{
public:
    SingleOffsetHandler(TaskSectionView* task,
                        double along,
                        double startOffset,
                        double minimumAlong,
                        double maximumAlong,
                        bool towardStart) :
        SectionOffsetHandler(task),
        m_along(along),
        m_offset(startOffset),
        m_minimumAlong(minimumAlong),
        m_maximumAlong(maximumAlong),
        m_towardStart(towardStart)
    {}

    void mouseMoveEvent(QMouseEvent* event) override
    {
        QPointF local;
        if (!snappedLocalPoint(event, local)) {
            return;
        }
        local = m_task->mapFromSectionHalf(local, m_towardStart);
        const Base::Vector3d direction = m_task->displayedDirection();
        const QPointF tangent(-direction.y, -direction.x);
        const QPointF normal(direction.x, -direction.y);

        if (m_mode == Mode::SeekP1) {
            const auto projected =
                m_task->projectOffsetStart(local);
            m_along = std::clamp(
                projected.first, m_minimumAlong, m_maximumAlong);
            m_offset = projected.second;
            updateMarker(m_task->mapToSectionHalf(
                m_task->temporaryCenter() + tangent * m_along
                    + normal * m_offset,
                m_towardStart));
        }
        else {
            const QPointF p1 =
                m_task->temporaryCenter() + tangent * m_along;
            m_offset = QPointF::dotProduct(local - p1, normal);
            updateMarker(m_task->mapToSectionHalf(
                p1 + normal * m_offset, m_towardStart));
            m_task->updatePendingSingleOffset(m_offset);
        }
        event->accept();
    }

    void mousePressEvent(QMouseEvent* event) override
    {
        if (!acceptsLeftPress(event)) {
            return;
        }
        if (m_mode == Mode::SeekP1) {
            m_task->beginSingleOffset(
                m_along, m_offset, m_towardStart);
            m_mode = Mode::SeekP2;
            updateHint();
            event->accept();
            return;
        }
        m_task->updatePendingSingleOffset(m_offset);
        m_task->commitPendingSingleOffset();
        event->accept();
        completeAndApply();
    }

private:
    enum class Mode
    {
        SeekP1,
        SeekP2
    };

    std::list<Gui::InputHint> getToolHints() const override
    {
        using enum Gui::InputHint::UserInput;
        const QString action = m_mode == Mode::SeekP1
            ? QObject::tr("%1 place offset start")
            : QObject::tr("%1 place offset end");
        return {
            {action, {MouseLeft}},
            {QObject::tr("%1 move without snapping"),
             {{ModifierCtrl, MouseMove}}},
            {QObject::tr("%1 cancel offset"), {MouseRight}},
        };
    }

    void cancelPending() override
    {
        m_task->cancelPendingSingleOffset();
    }

    Mode m_mode{Mode::SeekP1};
    double m_along{0.0};
    double m_offset{0.0};
    double m_minimumAlong{0.0};
    double m_maximumAlong{0.0};
    bool m_towardStart{false};
};

class NotchOffsetHandler final : public SectionOffsetHandler
{
public:
    NotchOffsetHandler(TaskSectionView* task,
                       double initialAlong,
                       double initialOffset,
                       double minimumAlong,
                       double maximumAlong,
                       bool towardStart) :
        SectionOffsetHandler(task),
        m_along(initialAlong),
        m_outerAlong(initialAlong),
        m_offset(initialOffset),
        m_minimumAlong(minimumAlong),
        m_maximumAlong(maximumAlong),
        m_towardStart(towardStart)
    {}

    void mouseMoveEvent(QMouseEvent* event) override
    {
        QPointF local;
        if (!snappedLocalPoint(event, local)) {
            return;
        }
        local = m_task->mapFromSectionHalf(local, m_towardStart);
        const Base::Vector3d direction = m_task->displayedDirection();
        const QPointF tangent(-direction.y, -direction.x);
        const QPointF normal(direction.x, -direction.y);

        if (m_mode == Mode::SeekP1) {
            const auto projected = m_task->projectOffsetStart(local);
            m_along = std::clamp(projected.first,
                                 m_minimumAlong, m_maximumAlong);
            m_offset = projected.second;
            updateMarker(m_task->mapToSectionHalf(
                m_task->temporaryCenter() + tangent * m_along
                    + normal * m_offset,
                m_towardStart));
        }
        else if (m_mode == Mode::SeekP2) {
            const QPointF anchor =
                m_task->temporaryCenter() + tangent * m_along;
            m_offset = QPointF::dotProduct(local - anchor, normal);
            updateMarker(m_task->mapToSectionHalf(
                anchor + normal * m_offset, m_towardStart));
            m_task->updatePendingSingleOffset(m_offset);
        }
        else {
            const double projected =
                QPointF::dotProduct(local - m_task->temporaryCenter(),
                                    tangent);
            m_outerAlong = m_towardStart
                ? std::clamp(projected, m_minimumAlong, m_along)
                : std::clamp(projected, m_along, m_maximumAlong);
            updateMarker(m_task->mapToSectionHalf(
                m_task->temporaryCenter() + tangent * m_outerAlong
                    + normal * m_offset,
                m_towardStart));
            m_task->updatePendingNotchOuterAlong(m_outerAlong);
        }
        event->accept();
    }

    void mousePressEvent(QMouseEvent* event) override
    {
        if (!acceptsLeftPress(event)) {
            return;
        }
        if (m_mode == Mode::SeekP1) {
            m_task->beginSingleOffset(m_along, m_offset, m_towardStart);
            m_mode = Mode::SeekP2;
            updateHint();
        }
        else if (m_mode == Mode::SeekP2) {
            m_task->updatePendingSingleOffset(m_offset);
            m_mode = Mode::SeekP3;
            updateHint();
        }
        else {
            m_task->updatePendingNotchOuterAlong(m_outerAlong);
            m_task->commitPendingSingleOffset();
            completeAndApply();
        }
        event->accept();
    }

private:
    enum class Mode
    {
        SeekP1,
        SeekP2,
        SeekP3
    };

    std::list<Gui::InputHint> getToolHints() const override
    {
        using enum Gui::InputHint::UserInput;
        QString action;
        if (m_mode == Mode::SeekP1) {
            action = QObject::tr("%1 place notch start");
        }
        else if (m_mode == Mode::SeekP2) {
            action = QObject::tr("%1 set notch depth");
        }
        else {
            action = QObject::tr("%1 place notch end");
        }
        return {
            {action, {MouseLeft}},
            {QObject::tr("%1 move without snapping"),
             {{ModifierCtrl, MouseMove}}},
            {QObject::tr("%1 cancel notch"), {MouseRight}},
        };
    }

    void cancelPending() override
    {
        m_task->cancelPendingSingleOffset();
    }

    Mode m_mode{Mode::SeekP1};
    double m_along{0.0};
    double m_outerAlong{0.0};
    double m_offset{0.0};
    double m_minimumAlong{0.0};
    double m_maximumAlong{0.0};
    bool m_towardStart{false};
};

class ArcNotchHandler final : public SectionOffsetHandler
{
public:
    ArcNotchHandler(TaskSectionView* task,
                    double along,
                    double minimumAlong,
                    double maximumAlong,
                    bool towardStart) :
        SectionOffsetHandler(task),
        m_firstAlong(along),
        m_secondAlong(along),
        m_minimumAlong(minimumAlong),
        m_maximumAlong(maximumAlong),
        m_towardStart(towardStart)
    {}

    void mouseMoveEvent(QMouseEvent* event) override
    {
        QPointF local;
        if (!snappedLocalPoint(event, local)) {
            return;
        }

        const QPointF center = m_task->temporaryCenter();
        const Base::Vector3d direction = m_task->displayedDirection();
        const QPointF tangent(-direction.y, -direction.x);
        const QPointF normal(direction.x, -direction.y);
        if (m_mode == Mode::SeekP1) {
            local = m_task->mapFromSectionHalf(local, m_towardStart);
            const auto projected = m_task->projectOffsetStart(local);
            m_firstAlong = std::clamp(
                projected.first, m_minimumAlong, m_maximumAlong);
            updateMarker(m_task->mapToSectionHalf(
                center + tangent * m_firstAlong
                    + normal * projected.second,
                m_towardStart));
        }
        else if (m_mode == Mode::SeekAngle) {
            const QPointF reference =
                m_task->sectionHalfEndpoint(m_towardStart) - center;
            const QPointF target = local - center;
            if (reference.manhattanLength() < 1.0e-6
                || target.manhattanLength() < 1.0e-6) {
                return;
            }
            m_angle = std::atan2(
                reference.x() * target.y() - reference.y() * target.x(),
                QPointF::dotProduct(reference, target));
            updateMarker(rotateAroundCenter(
                center + reference, m_angle));
            m_task->updatePendingArcNotch(m_angle, m_firstAlong);
        }
        else {
            local = m_task->mapFromSectionHalf(local, m_towardStart);
            local = rotateAroundCenter(local, -m_angle);
            const double projected = QPointF::dotProduct(
                local - center, tangent);
            m_secondAlong = m_towardStart
                ? std::clamp(projected, m_minimumAlong, m_firstAlong)
                : std::clamp(projected, m_firstAlong, m_maximumAlong);
            const double offset = m_task->projectOffsetStart(
                center + tangent * m_secondAlong).second;
            QPointF marker = m_task->mapToSectionHalf(
                center + tangent * m_secondAlong + normal * offset,
                m_towardStart);
            marker = rotateAroundCenter(marker, m_angle);
            updateMarker(marker);
            m_task->updatePendingArcNotch(m_angle, m_secondAlong);
        }
        event->accept();
    }

    void mousePressEvent(QMouseEvent* event) override
    {
        if (!acceptsLeftPress(event)) {
            return;
        }
        if (m_mode == Mode::SeekP1) {
            m_task->beginArcNotch(m_firstAlong, m_towardStart);
            m_mode = Mode::SeekAngle;
            updateHint();
        }
        else if (m_mode == Mode::SeekAngle) {
            m_task->updatePendingArcNotch(m_angle, m_firstAlong);
            m_mode = Mode::SeekP2;
            updateHint();
        }
        else {
            m_task->updatePendingArcNotch(m_angle, m_secondAlong);
            m_task->commitPendingArcNotch();
            completeAndApply();
        }
        event->accept();
    }

private:
    enum class Mode
    {
        SeekP1,
        SeekAngle,
        SeekP2
    };

    std::list<Gui::InputHint> getToolHints() const override
    {
        using enum Gui::InputHint::UserInput;
        QString action;
        if (m_mode == Mode::SeekP1) {
            action = QObject::tr("%1 place first arc bend");
        }
        else if (m_mode == Mode::SeekAngle) {
            action = QObject::tr("%1 set arc-notch direction");
        }
        else {
            action = QObject::tr("%1 place return arc bend");
        }
        return {
            {action, {MouseLeft}},
            {QObject::tr("%1 move without snapping"),
             {{ModifierCtrl, MouseMove}}},
            {QObject::tr("%1 cancel arc notch"), {MouseRight}},
        };
    }

    void cancelPending() override
    {
        m_task->cancelPendingArcNotch();
    }

    QPointF rotateAroundCenter(const QPointF& point, double angle) const
    {
        const QPointF center = m_task->temporaryCenter();
        const QPointF delta = point - center;
        return center
            + QPointF(delta.x() * std::cos(angle)
                          - delta.y() * std::sin(angle),
                      delta.x() * std::sin(angle)
                          + delta.y() * std::cos(angle));
    }

    Mode m_mode{Mode::SeekP1};
    double m_firstAlong{0.0};
    double m_secondAlong{0.0};
    double m_angle{0.0};
    double m_minimumAlong{0.0};
    double m_maximumAlong{0.0};
    bool m_towardStart{false};
};

class ArcOffsetHandler final : public SectionOffsetHandler
{
public:
    ArcOffsetHandler(TaskSectionView* task,
                     double along,
                     double minimumAlong,
                     double maximumAlong,
                     bool towardStart) :
        SectionOffsetHandler(task),
        m_along(along),
        m_minimumAlong(minimumAlong),
        m_maximumAlong(maximumAlong),
        m_towardStart(towardStart)
    {}

    void mouseMoveEvent(QMouseEvent* event) override
    {
        QPointF local;
        if (!snappedLocalPoint(event, local)) {
            return;
        }
        if (m_mode == Mode::SeekP1) {
            local = m_task->mapFromSectionHalf(
                local, m_towardStart);
            const Base::Vector3d direction =
                m_task->displayedDirection();
            const QPointF tangent(
                -direction.y, -direction.x);
            const QPointF normal(
                direction.x, -direction.y);
            const auto projected =
                m_task->projectOffsetStart(local);
            m_along = std::clamp(
                projected.first, m_minimumAlong, m_maximumAlong);
            updateMarker(m_task->mapToSectionHalf(
                m_task->temporaryCenter() + tangent * m_along
                    + normal * projected.second,
                m_towardStart));
            event->accept();
            return;
        }
        const QPointF center = m_task->temporaryCenter();
        const QPointF reference =
            m_task->sectionHalfEndpoint(m_towardStart) - center;
        const QPointF target = local - center;
        if (reference.manhattanLength() < 1.0e-6
            || target.manhattanLength() < 1.0e-6) {
            return;
        }
        m_angle = std::atan2(
            reference.x() * target.y() - reference.y() * target.x(),
            QPointF::dotProduct(reference, target));
        const double cosine = std::cos(m_angle);
        const double sine = std::sin(m_angle);
        const QPointF marker =
            center
            + QPointF(reference.x() * cosine - reference.y() * sine,
                      reference.x() * sine + reference.y() * cosine);
        updateMarker(marker);
        m_task->updatePendingArcAngle(m_angle);
        event->accept();
    }

    void mousePressEvent(QMouseEvent* event) override
    {
        if (!acceptsLeftPress(event)) {
            return;
        }
        if (m_mode == Mode::SeekP1) {
            m_task->beginArcOffset(m_along, m_towardStart);
            m_mode = Mode::SeekAngle;
            updateHint();
            event->accept();
            return;
        }
        m_task->updatePendingArcAngle(m_angle);
        m_task->commitPendingArc();
        event->accept();
        completeAndApply();
    }

private:
    enum class Mode
    {
        SeekP1,
        SeekAngle
    };

    std::list<Gui::InputHint> getToolHints() const override
    {
        using enum Gui::InputHint::UserInput;
        const QString action = m_mode == Mode::SeekP1
            ? QObject::tr("%1 place arc bend")
            : QObject::tr("%1 set arc direction");
        return {
            {action, {MouseLeft}},
            {QObject::tr("%1 move without snapping"),
             {{ModifierCtrl, MouseMove}}},
            {QObject::tr("%1 cancel arc offset"), {MouseRight}},
        };
    }

    void cancelPending() override
    {
        m_task->cancelPendingArc();
    }

    Mode m_mode{Mode::SeekP1};
    double m_along{0.0};
    double m_angle{0.0};
    double m_minimumAlong{0.0};
    double m_maximumAlong{0.0};
    bool m_towardStart{false};
};

}  // namespace

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void TaskSectionView::startSingleOffset(double along,
                                        double minimumAlong,
                                        double maximumAlong,
                                        bool towardStart)
{
    if (!m_graphicsView || !m_temporarySectionLine) {
        return;
    }
    m_sectionControlsEnabled = false;
    hideSectionHandles();
    m_pendingOffsetKind = PendingOffsetKind::Single;
    m_pendingMinimumAlong = minimumAlong;
    m_pendingMaximumAlong = maximumAlong;
    const QPointF tangent(-m_displayedDirection.y, -m_displayedDirection.x);
    const double startOffset = projectOffsetStart(
        m_temporaryCenter + tangent * along).second;
    showUnappliedPreview(false);
    m_graphicsView->activateHandler(
        new SingleOffsetHandler(this, along, startOffset,
                                minimumAlong, maximumAlong,
                                towardStart));
}

void TaskSectionView::startNotchOffset(double along,
                                       double minimumAlong,
                                       double maximumAlong,
                                       bool towardStart)
{
    if (!m_graphicsView || !m_temporarySectionLine) {
        return;
    }
    m_sectionControlsEnabled = false;
    hideSectionHandles();
    m_pendingOffsetKind = PendingOffsetKind::Notch;
    m_pendingMinimumAlong = minimumAlong;
    m_pendingMaximumAlong = maximumAlong;
    const QPointF tangent(-m_displayedDirection.y, -m_displayedDirection.x);
    const double startOffset = projectOffsetStart(
        m_temporaryCenter + tangent * along).second;
    showUnappliedPreview(false);
    m_graphicsView->activateHandler(
        new NotchOffsetHandler(this, along, startOffset,
                               minimumAlong, maximumAlong,
                               towardStart));
}

void TaskSectionView::startArcNotchOffset(double along,
                                          double minimumAlong,
                                          double maximumAlong,
                                          bool towardStart)
{
    if (!m_graphicsView || !m_temporarySectionLine) {
        return;
    }
    m_sectionControlsEnabled = false;
    hideSectionHandles();
    showUnappliedPreview(false);
    m_graphicsView->activateHandler(
        new ArcNotchHandler(this, along, minimumAlong,
                            maximumAlong, towardStart));
}

void TaskSectionView::startArcOffset(double along,
                                     double minimumAlong,
                                     double maximumAlong,
                                     bool towardStart)
{
    if (!m_graphicsView || !m_temporarySectionLine) {
        return;
    }
    m_sectionControlsEnabled = false;
    hideSectionHandles();
    showUnappliedPreview(false);
    m_graphicsView->activateHandler(
        new ArcOffsetHandler(this, along, minimumAlong,
                             maximumAlong, towardStart));
}

TaskDlgSectionView::TaskDlgSectionView(TechDraw::DrawPage* page, QGVPage* graphicsView) :
    TaskDialog(),
    m_graphicsView(graphicsView),
    m_startInteractiveHandler(true)
{
    widget = new TaskSectionView(page, graphicsView);
    connect(widget, &TaskSectionView::restartPositioningRequested,
            this, &TaskDlgSectionView::restartPositioningHandler);
    taskbox =
        new Gui::TaskView::TaskBox(Gui::BitmapFactory().pixmap("actions/TechDraw_SectionView"),
                                   widget->windowTitle(), true, nullptr);
    taskbox->groupLayout()->addWidget(widget);
    Content.push_back(taskbox);
    suppressFaceSelection(true);
}

TaskDlgSectionView::TaskDlgSectionView(TechDraw::DrawViewPart* base) : TaskDialog()
{
    widget = new TaskSectionView(base);
    taskbox =
        new Gui::TaskView::TaskBox(Gui::BitmapFactory().pixmap("actions/TechDraw_SectionView"),
                                   widget->windowTitle(), true, nullptr);
    taskbox->groupLayout()->addWidget(widget);
    Content.push_back(taskbox);

    auto* mdi = qobject_cast<MDIViewPage*>(
        Gui::getMainWindow()->activeWindow());
    if (mdi && mdi->getPage() == base->findParentPage()) {
        m_graphicsView =
            mdi->getViewProviderPage()->getQGVPage();
        suppressFaceSelection(true);
    }
}

TaskDlgSectionView::TaskDlgSectionView(TechDraw::DrawViewSection* section) : TaskDialog()
{
    widget = new TaskSectionView(section);
    connect(widget, &TaskSectionView::restartPositioningRequested,
            this, &TaskDlgSectionView::restartPositioningHandler);
    taskbox =
        new Gui::TaskView::TaskBox(Gui::BitmapFactory().pixmap("actions/TechDraw_SectionView"),
                                   widget->windowTitle(), true, nullptr);

    taskbox->groupLayout()->addWidget(widget);
    Content.push_back(taskbox);

    auto* mdi = qobject_cast<MDIViewPage*>(
        Gui::getMainWindow()->activeWindow());
    if (mdi && mdi->getPage() == section->findParentPage()) {
        m_graphicsView =
            mdi->getViewProviderPage()->getQGVPage();
        suppressFaceSelection(true);
        widget->initializeEditPreview(m_graphicsView);
    }
}
TaskDlgSectionView::~TaskDlgSectionView()
{
    if (m_graphicsView && m_graphicsView->isHandlerActive()) {
        m_graphicsView->deactivateHandler();
    }
    suppressFaceSelection(false);
}


//==== calls from the TaskView ===============================================================
void TaskDlgSectionView::open()
{
    if (m_startInteractiveHandler && m_graphicsView) {
        restartPositioningHandler();
    }
}

void TaskDlgSectionView::restartPositioningHandler()
{
    if (!m_graphicsView) {
        return;
    }
    if (m_graphicsView->isHandlerActive()) {
        m_graphicsView->deactivateHandler();
    }
    widget->setPositioningActive(true);
    m_graphicsView->activateHandler(new SectionViewHandler(widget));
}

void TaskDlgSectionView::suppressFaceSelection(bool suppress)
{
    if (!m_graphicsView || !m_graphicsView->getScene()) {
        return;
    }
    auto* scene = m_graphicsView->getScene();
    constexpr auto propertyName =
        "TechDrawSuppressFaceSelection";
    if (suppress) {
        if (!m_faceSelectionSuppressionSet) {
            m_previousFaceSelectionSuppression =
                scene->property(propertyName);
            m_faceSelectionSuppressionSet = true;
        }
        scene->setProperty(propertyName, true);
        Gui::Selection().rmvPreselect();
        for (QGraphicsItem* item : scene->items()) {
            auto* face = dynamic_cast<QGIFace*>(item);
            if (face && !face->isSelected()) {
                face->setPrettyNormal();
            }
        }
    }
    else if (m_faceSelectionSuppressionSet) {
        scene->setProperty(
            propertyName, m_previousFaceSelectionSuppression);
        m_faceSelectionSuppressionSet = false;
    }
}

bool TaskDlgSectionView::accept()
{
    return widget->accept();
}

bool TaskDlgSectionView::reject()
{
    widget->reject();
    return true;
}

void TaskDlgSectionView::clicked(int id)
{
    if (id == QDialogButtonBox::Apply) {
        widget->apply(true);
    }
}

void TaskDlgSectionView::modifyStandardButtons(QDialogButtonBox* buttonBox)
{
    if (QPushButton* applyButton = buttonBox->button(QDialogButtonBox::Apply)) {
        auto updateApplyButton = [applyButton](bool valid) {
            applyButton->setEnabled(valid);
            applyButton->setToolTip(
                valid ? QObject::tr("Rebuilds the display now. May be slow for complex models.")
                      : QObject::tr("No base view set."));
        };
        connect(widget, &TaskSectionView::baseViewChanged, applyButton, updateApplyButton);
        updateApplyButton(widget->hasBase());
    }
}

#include <Mod/TechDraw/Gui/moc_TaskSectionView.cpp>
