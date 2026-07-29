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
#include <QMouseEvent>
#include <QKeyEvent>
#include <QPainter>
#include <QPainterPathStroker>
#include <QPushButton>
#include <QGraphicsItem>
#include <QGraphicsEllipseItem>
#include <QGraphicsPathItem>
#include <QGraphicsSceneMouseEvent>
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

namespace
{

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
    explicit SelectableOffsetEdge(std::function<void()> deleteCallback) :
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
        setToolTip(QObject::tr("Select and press Delete to remove this offset"));
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

class SectionActionGlyph final : public QGraphicsPathItem
{
public:
    enum class Kind
    {
        Single,
        Notch,
        Arc
    };

    SectionActionGlyph(Kind kind,
                       double alongOffset,
                       std::function<void()> callback) :
        m_callback(std::move(callback))
    {
        QPainterPath glyph;
        if (kind == Kind::Single) {
            glyph.moveTo(0.0, -3.0);
            glyph.lineTo(0.0, -9.0);
            glyph.lineTo(10.0, -9.0);
            glyph.moveTo(6.0, -12.0);
            glyph.lineTo(10.0, -9.0);
            glyph.lineTo(6.0, -6.0);
        }
        else if (kind == Kind::Notch) {
            glyph.moveTo(0.0, -3.0);
            glyph.lineTo(0.0, -9.0);
            glyph.lineTo(-8.0, -9.0);
            glyph.lineTo(-8.0, -3.0);
            glyph.moveTo(-11.0, -6.0);
            glyph.lineTo(-8.0, -3.0);
            glyph.lineTo(-5.0, -6.0);
        }
        glyph.translate(alongOffset, 0.0);
        setPath(glyph);
        const bool active = true;
        QPen pen(active ? QColor(35, 95, 210) : QColor(130, 150, 185),
                 2.0, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
        pen.setCosmetic(true);
        setPen(pen);
        setZValue(1003.0);
        setFlag(QGraphicsItem::ItemIgnoresTransformations);
        setAcceptedMouseButtons(active ? Qt::LeftButton : Qt::NoButton);
        setCursor(active ? Qt::PointingHandCursor : Qt::ArrowCursor);
        if (kind == Kind::Single) {
            setToolTip(QObject::tr("Add a single offset to this section-line segment"));
        }
        else if (kind == Kind::Notch) {
            setToolTip(QObject::tr("Add a notched offset to this section-line segment"));
        }
        else {
            setToolTip(QObject::tr("Add an arc offset to this section-line segment"));
        }
    }

    void setArcGeometry(const QPointF& anchor,
                        const QPointF& segmentDirection)
    {
        const double directionLength = std::hypot(
            segmentDirection.x(), segmentDirection.y());
        if (directionLength < 1.0e-6) {
            setPath({});
            return;
        }

        const double radiusLength = Rez::guiX(5.0);
        const QPointF radius(radiusLength, 0.0);
        const QPointF arcCenter = -radius;
        const double arcLength = Rez::guiX(2.0);
        const double sweep = arcLength / radiusLength;
        const double startAngle = 3.0 / radiusLength;
        constexpr int arcSegments = 8;
        auto rotate = [](const QPointF& point, double angle) {
            return QPointF(point.x() * std::cos(angle)
                               - point.y() * std::sin(angle),
                           point.x() * std::sin(angle)
                               + point.y() * std::cos(angle));
        };

        QPainterPath glyph;
        glyph.moveTo(arcCenter + rotate(radius, startAngle));
        QPointF arcEnd = glyph.currentPosition();
        for (int segment = 1; segment <= arcSegments; ++segment) {
            const double angle = startAngle
                + sweep * static_cast<double>(segment) / arcSegments;
            arcEnd = arcCenter + rotate(radius, angle);
            glyph.lineTo(arcEnd);
        }

        const QPointF outward = (arcEnd - arcCenter) / radiusLength;
        const QPointF perpendicular(-outward.y(), outward.x());
        const double outwardLength = Rez::guiX(4.0 / 3.0);
        const double headLength = Rez::guiX(0.5);
        const double headWidth = Rez::guiX(0.8 / 3.0);
        const QPointF arrowTip = arcEnd + outward * outwardLength;
        glyph.lineTo(arrowTip);
        glyph.moveTo(arrowTip - outward * headLength
                     + perpendicular * headWidth);
        glyph.lineTo(arrowTip);
        glyph.lineTo(arrowTip - outward * headLength
                     - perpendicular * headWidth);

        setFlag(QGraphicsItem::ItemIgnoresTransformations);
        setPos(anchor);
        setRotation(Base::toDegrees(std::atan2(
            segmentDirection.y(), segmentDirection.x())));
        setPath(glyph);
    }

protected:
    void mousePressEvent(QGraphicsSceneMouseEvent* event) override
    {
        if (m_callback) {
            m_callback();
        }
        event->accept();
    }

private:
    std::function<void()> m_callback;
};

class SectionRotationHandle final : public QGraphicsPathItem
{
public:
    using MoveCallback = std::function<void(const QPointF&)>;

    explicit SectionRotationHandle(MoveCallback callback) :
        m_callback(std::move(callback))
    {
        QPen pen(QColor(35, 95, 210), 2.0, Qt::SolidLine,
                 Qt::RoundCap, Qt::RoundJoin);
        pen.setCosmetic(true);
        setPen(pen);
        setZValue(1003.0);
        setFlag(QGraphicsItem::ItemIgnoresTransformations);
        setAcceptedMouseButtons(Qt::LeftButton);
        setCursor(Qt::OpenHandCursor);
        setToolTip(QObject::tr("Drag to rotate this half of the section line"));
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
        const double glyphRadius = outsideDistance / 3.0;
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
        setCursor(Qt::ClosedHandCursor);
        event->accept();
    }

    void mouseMoveEvent(QGraphicsSceneMouseEvent* event) override
    {
        if (m_callback) {
            m_callback(event->scenePos());
        }
        event->accept();
    }

    void mouseReleaseEvent(QGraphicsSceneMouseEvent* event) override
    {
        if (m_callback) {
            m_callback(event->scenePos());
        }
        setCursor(Qt::OpenHandCursor);
        event->accept();
    }

private:
    MoveCallback m_callback;
};

}  // namespace

TaskSectionView::~TaskSectionView()
{
    clearSectionControls();
    delete m_centerHandle;
    for (auto& handles : m_offsetHandles) {
        delete handles.first;
        delete handles.second;
    }
    for (auto& handles : m_arcBendHandles) {
        delete handles.first;
        delete handles.second;
    }
    for (QGraphicsItem* item : m_selectableOffsetEdges) {
        delete item;
    }
    delete m_temporarySectionLine;
}

// ctor for interactive create without a preselected base view
TaskSectionView::TaskSectionView(TechDraw::DrawPage* page, QGVPage* graphicsView) :
    ui(new Ui_TaskSectionView),
    m_base(nullptr),
    m_section(nullptr),
    m_saveScale(1.0),
    m_dirName("Aligned"),
    m_doc(page ? page->getDocument() : nullptr),
    m_page(page),
    m_graphicsView(graphicsView),
    m_createMode(true),
    m_saved(false),
    m_modelIsDirty(false),
    m_scaleEdited(false),
    m_directionChanged(false)
{
    m_sectionName = std::string();
    if (m_page) {
        m_savePageName = m_page->getNameInDocument();
    }

    ui->setupUi(this);
    setUiPrimary();
    setPositioningActive(true);
}

//ctor for create
TaskSectionView::TaskSectionView(TechDraw::DrawViewPart* base) :
    ui(new Ui_TaskSectionView),
    m_base(base),
    m_section(nullptr),
    m_saveScale(1.0),
    m_dirName("Aligned"),
    m_doc(nullptr),
    m_page(base ? base->findParentPage() : nullptr),
    m_graphicsView(nullptr),
    m_createMode(true),
    m_saved(false),
    m_modelIsDirty(false),
    m_scaleEdited(false),
    m_directionChanged(false)
{
    //existence of base is guaranteed by CmdTechDrawSectionView (Command.cpp)

    m_sectionName = std::string();
    m_doc = m_base->getDocument();

    m_saveBaseName = m_base->getNameInDocument();
    m_savePageName = m_base->findParentPage()->getNameInDocument();

    ui->setupUi(this);
    setUiPrimary();
}

//ctor for edit
TaskSectionView::TaskSectionView(TechDraw::DrawViewSection* section) :
    ui(new Ui_TaskSectionView),
    m_base(nullptr),
    m_section(section),
    m_saveScale(1.0),
    m_doc(nullptr),
    m_page(section ? section->findParentPage() : nullptr),
    m_graphicsView(nullptr),
    m_createMode(false),
    m_saved(false),
    m_modelIsDirty(false),
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
    saveSectionState();
    setUiEdit();
    if (isGeneratedProfile(m_profileObject)) {
        restoreModernProfileState();
    }
}

void TaskSectionView::setUiPrimary()
{
    setWindowTitle(QObject::tr("Create Section View"));

    Base::Vector3d origin;
    if (m_base) {
        ui->sbScale->setValue(m_base->getScale());
        ui->cmbScaleType->setCurrentIndex(m_base->getScaleType());
        origin = m_base->getOriginalCentroid();
    }
    else if (m_page) {
        ui->sbScale->setValue(m_page->Scale.getValue());
        ui->cmbScaleType->setCurrentIndex(0);
    }
    setUiCommon(origin);

    m_compass->setDialAngle(0.0);
    m_viewDirectionWidget->setValueNoNotify(Base::Vector3d(1.0, 0.0, 0.0));
    ui->gbOrientation->setVisible(m_base != nullptr);
    setInteractiveBase(m_base);
}

void TaskSectionView::setUiEdit()
{
    setWindowTitle(QObject::tr("Edit Section View"));
    ui->gbOrientation->show();
    std::string temp = m_section->SectionSymbol.getValue();
    QString qTemp = QString::fromStdString(temp);
    ui->leSymbol->setText(qTemp);

    ui->sbScale->setValue(m_section->getScale());
    ui->cmbScaleType->setCurrentIndex(m_section->getScaleType());
    //Allow or prevent scale changing initially
    if (m_section->ScaleType.isValue("Custom")) {
        ui->sbScale->setEnabled(true);
    }
    else {
        ui->sbScale->setEnabled(false);
    }

    auto origin = m_section->SectionOrigin.getValue();
    setUiCommon(origin);

    // convert section normal to view angle
    Base::Vector3d sectionNormalVec = m_section->SectionNormal.getValue();
    sectionNormalVec.Normalize();
    Base::Vector3d projectedViewDirection = m_base->projectPoint(sectionNormalVec, false);
    projectedViewDirection.Normalize();
    double viewAngle = atan2(-projectedViewDirection.y, -projectedViewDirection.x);
    m_compass->setDialAngle(Base::toDegrees(viewAngle));
    m_viewDirectionWidget->setValueNoNotify(sectionNormalVec * -1.0);

    if (auto* complexSection =
            dynamic_cast<TechDraw::DrawComplexSection*>(m_section)) {
        ui->cmbStrategy->setCurrentIndex(
            static_cast<int>(complexSection->ProjectionStrategy.getValue()));
        ui->leSectionObjects->setText(sourcesToString());
        setProfileObject(m_profileObject);
        setCustomComplexSection(m_customComplexSection);
    }
}

void TaskSectionView::setUiCommon(Base::Vector3d origin)
{
    ui->gbSectionLinePosition->hide();
    ui->gbCustomComplexSection->hide();
    ui->cbCustomComplexSection->show();
    ui->leSectionObjects->setText(sourcesToString());

    ui->sbOrgX->setUnit(Base::Unit::Length);
    ui->sbOrgX->setValue(origin.x);
    ui->sbOrgY->setUnit(Base::Unit::Length);
    ui->sbOrgY->setValue(origin.y);
    ui->sbOrgZ->setUnit(Base::Unit::Length);
    ui->sbOrgZ->setValue(origin.z);

    enableAll(false);

    connect(ui->leSymbol, &QLineEdit::editingFinished, this, &TaskSectionView::onIdentifierChanged);

    //TODO: use event filter instead of keyboard tracking to capture enter/return keys
    // the UI file uses keyboardTracking = false so that a recomputation
    // will only be triggered when the arrow keys of the spinboxes are used
    //if this is not done, recomputes are triggered on each key press giving
    //unaccceptable UX
    connect(ui->sbScale, qOverload<double>(&QDoubleSpinBox::valueChanged), this, &TaskSectionView::onScaleChanged);
    connect(ui->sbOrgX, qOverload<double>(&QuantitySpinBox::valueChanged), this, &TaskSectionView::onXChanged);
    connect(ui->sbOrgY, qOverload<double>(&QuantitySpinBox::valueChanged), this, &TaskSectionView::onYChanged);
    connect(ui->sbOrgZ, qOverload<double>(&QuantitySpinBox::valueChanged), this, &TaskSectionView::onZChanged);

    connect(ui->cmbScaleType, qOverload<int>(&QComboBox::currentIndexChanged), this, &TaskSectionView::scaleTypeChanged);

    connect(ui->cbLiveUpdate, &QToolButton::clicked, this, &TaskSectionView::liveUpdateClicked);
    connect(ui->cbCustomComplexSection, &QCheckBox::clicked,
            this, &TaskSectionView::onCustomComplexSectionClicked);
    connect(ui->gbCustomComplexSection, &QGroupBox::toggled,
            this, &TaskSectionView::onCustomComplexSectionToggled);
    connect(ui->pbSectionObjects, &QPushButton::clicked,
            this, &TaskSectionView::onSectionObjectsUseSelectionClicked);
    connect(ui->pbProfileObject, &QPushButton::clicked,
            this, &TaskSectionView::onProfileObjectUseSelectionClicked);
    connect(ui->cmbStrategy, qOverload<int>(&QComboBox::currentIndexChanged),
            this, &TaskSectionView::onProjectionStrategyChanged);
    connect(ui->cmbSectionLinePosition,
            qOverload<int>(&QComboBox::currentIndexChanged), this,
            &TaskSectionView::onSectionLinePositionChanged);
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
        const Base::Vector3d origin = m_base->getOriginalCentroid();
        ui->sbOrgX->setValue(origin.x);
        ui->sbOrgY->setValue(origin.y);
        ui->sbOrgZ->setValue(origin.z);
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

    const QSignalBlocker blockX(ui->sbOrgX);
    const QSignalBlocker blockY(ui->sbOrgY);
    const QSignalBlocker blockZ(ui->sbOrgZ);
    ui->sbOrgX->setValue(origin.x);
    ui->sbOrgY->setValue(origin.y);
    ui->sbOrgZ->setValue(origin.z);

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

void TaskSectionView::setDirectionControlsVisible(bool visible)
{
    ui->gbOrientation->setVisible(visible);
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
    if (enabled && m_graphicsView
        && m_graphicsView->isHandlerActive()) {
        m_graphicsView->deactivateHandler();
    }
    if (enabled) {
        m_hasPendingOffset = false;
        m_hasPendingArc = false;
        ui->gbOrientation->setVisible(true);
        ui->gbOrientation->setEnabled(m_base != nullptr);
        ui->gbPlane->setVisible(true);
        ui->gbPlane->setEnabled(m_base != nullptr);
    }
    const QSignalBlocker blockCheck(ui->cbCustomComplexSection);
    const QSignalBlocker blockGroup(ui->gbCustomComplexSection);
    ui->cbCustomComplexSection->setChecked(enabled);
    ui->cbCustomComplexSection->setVisible(!enabled);
    ui->gbCustomComplexSection->setChecked(enabled);
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

void TaskSectionView::onCustomComplexSectionClicked()
{
    setCustomComplexSection(true);
}

void TaskSectionView::onCustomComplexSectionToggled(bool checked)
{
    if (!checked) {
        setCustomComplexSection(false);
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

void TaskSectionView::adoptTemporarySectionLine(QGISectionLine* line,
                                               QGIViewPart* baseItem,
                                               double halfLength)
{
    delete m_temporarySectionLine;
    m_temporarySectionLine = line;
    m_baseItem = baseItem;
    m_temporaryHalfLength = halfLength;
    ui->cbCustomComplexSection->setEnabled(m_base != nullptr);
}

TaskSectionView::PositionMode TaskSectionView::positionMode() const
{
    return static_cast<PositionMode>(
        ui->cmbSectionLinePosition->currentIndex());
}

void TaskSectionView::setPositioningActive(bool active)
{
    ui->gbSectionLinePosition->setVisible(
        m_graphicsView && !m_customComplexSection);
    ui->cmbSectionLinePosition->setVisible(active);
    ui->pbResetSectionLine->setVisible(!active);
}

void TaskSectionView::resetInteractivePreview()
{
    setSectionControlsEnabled(false);
    clearSectionControls();
    delete m_centerHandle;
    m_centerHandle = nullptr;
    for (auto& handles : m_offsetHandles) {
        delete handles.first;
        delete handles.second;
    }
    m_offsetHandles.clear();
    for (auto& handles : m_arcBendHandles) {
        delete handles.first;
        delete handles.second;
    }
    m_arcBendHandles.clear();
    for (QGraphicsItem* item : m_selectableOffsetEdges) {
        delete item;
    }
    m_selectableOffsetEdges.clear();
    delete m_temporarySectionLine;
    m_temporarySectionLine = nullptr;
    m_baseItem = nullptr;
    m_offsetSteps.clear();
    m_arcBends.clear();
    m_startOffset = 0.0;
    m_startRotation = 0.0;
    m_endRotation = 0.0;

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

void TaskSectionView::configureTemporarySectionLine()
{
    if (!m_temporarySectionLine || !m_base) {
        return;
    }
    m_temporarySectionLine->setSymbol(const_cast<char*>(""));
    m_temporarySectionLine->setPathMode(true);
    m_temporarySectionLine->setSectionColor(Qt::black);

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
    m_baseItem = dynamic_cast<QGIViewPart*>(
        pageScene->findQViewForDocObj(m_base));
    if (!m_baseItem) {
        return;
    }

    m_temporarySectionLine = new QGISectionLine();
    pageScene->addItem(m_temporarySectionLine);
    configureTemporarySectionLine();
    if (!m_hasModernProfileState) {
        m_temporaryHalfLength = std::max(
            Rez::guiX(10.0),
            std::max(m_baseItem->boundingRect().width(),
                     m_baseItem->boundingRect().height()) * 0.65);
    }
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
    drawTemporarySingleOffset();
}

void TaskSectionView::updateTemporarySectionLine(
    const QPointF& center,
    const Base::Vector3d& displayedDirection,
    double halfLength)
{
    m_temporaryHalfLength = halfLength;
    updateTemporarySectionLine(center, displayedDirection);
}

std::pair<double, double>
TaskSectionView::projectOffsetStart(const QPointF& point) const
{
    const QPointF tangent(-m_displayedDirection.y, -m_displayedDirection.x);
    const double along = std::clamp(
        QPointF::dotProduct(point - m_temporaryCenter, tangent),
        -m_temporaryHalfLength,
        m_temporaryHalfLength);
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
        std::clamp(along, -m_temporaryHalfLength, m_temporaryHalfLength);
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
    for (auto& handles : m_offsetHandles) {
        delete handles.first;
        delete handles.second;
    }
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
        m_temporaryCenter - tangent * m_temporaryHalfLength
        + normal * currentOffset;
    points.emplace_back(rotateSectionPoint(start, -m_temporaryHalfLength),
                        -m_temporaryHalfLength);
    bool addedCenter = false;
    for (const auto& step : steps) {
        if (!addedCenter && step.along >= 0.0) {
            const QPointF centerPoint =
                m_temporaryCenter + normal * currentOffset;
            points.emplace_back(centerPoint, 0.0);
            addedCenter = true;
        }
        if (step.transition == OffsetStep::Transition::Arc) {
            const QPointF arcStart =
                m_temporaryCenter + tangent * step.startAlong
                + normal * currentOffset;
            points.emplace_back(
                rotateSectionPoint(arcStart, step.startAlong),
                step.startAlong);
            constexpr int arcSegments = 10;
            const double radius =
                std::abs(step.along - step.startAlong);
            const double alongSign =
                step.along >= step.startAlong ? 1.0 : -1.0;
            const double offsetSign =
                step.offset >= currentOffset ? 1.0 : -1.0;
            for (int segment = 1; segment <= arcSegments; ++segment) {
                const double ratio =
                    static_cast<double>(segment) / arcSegments;
                const double angle = ratio * std::numbers::pi / 2.0;
                const double along =
                    step.startAlong + alongSign * radius * std::sin(angle);
                const double offset =
                    currentOffset
                    + offsetSign * radius * (1.0 - std::cos(angle));
                const QPointF arcPoint =
                    m_temporaryCenter + tangent * along + normal * offset;
                points.emplace_back(rotateSectionPoint(arcPoint, along),
                                    along);
            }
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
        m_temporaryCenter + tangent * m_temporaryHalfLength
        + normal * currentOffset;
    points.emplace_back(rotateSectionPoint(end, m_temporaryHalfLength),
                        m_temporaryHalfLength);

    std::vector<ArcBend> bends = m_arcBends;
    if (includePending && m_hasPendingArc) {
        bends.push_back(m_pendingArc);
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
    return points;
}

void TaskSectionView::drawTemporarySingleOffset()
{
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
    m_temporarySectionLine->setEnds(
        Base::Vector3d(sceneStart.x(), sceneStart.y(), 0.0),
        Base::Vector3d(sceneEnd.x(), sceneEnd.y(), 0.0));
    m_temporarySectionLine->setArrowDirections(
        Base::Vector3d(startDirection.x(), -startDirection.y(), 0.0),
        Base::Vector3d(endDirection.x(), -endDirection.y(), 0.0));
    m_temporarySectionLine->draw();
    rebuildSelectableOffsetEdges();
    updateSectionHandles();
    updateSectionControls();
}

void TaskSectionView::rebuildSelectableOffsetEdges()
{
    for (QGraphicsItem* item : m_selectableOffsetEdges) {
        delete item;
    }
    m_selectableOffsetEdges.clear();
    if (m_customComplexSection || !m_sectionControlsEnabled
        || !m_baseItem || !m_baseItem->scene()
        || m_hasPendingOffset || m_hasPendingArc) {
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
            const QPointF start =
                m_temporaryCenter + tangent * step.startAlong
                + normal * currentOffset;
            edgePath.moveTo(m_baseItem->mapToScene(
                rotateSectionPoint(start, step.startAlong)));
            constexpr int arcSegments = 10;
            const double radius =
                std::abs(step.along - step.startAlong);
            const double alongSign =
                step.along >= step.startAlong ? 1.0 : -1.0;
            const double offsetSign =
                step.offset >= currentOffset ? 1.0 : -1.0;
            for (int segment = 1; segment <= arcSegments; ++segment) {
                const double ratio =
                    static_cast<double>(segment) / arcSegments;
                const double angle = ratio * std::numbers::pi / 2.0;
                const double along =
                    step.startAlong
                    + alongSign * radius * std::sin(angle);
                const double offset =
                    currentOffset
                    + offsetSign * radius * (1.0 - std::cos(angle));
                const QPointF point =
                    m_temporaryCenter + tangent * along + normal * offset;
                edgePath.lineTo(m_baseItem->mapToScene(
                    rotateSectionPoint(point, along)));
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
        auto* edge = new SelectableOffsetEdge([this, i]() {
            QTimer::singleShot(
                0, this, [this, i]() { removeOffsetStep(i); });
        });
        edge->setPath(edgePath);
        m_baseItem->scene()->addItem(edge);
        m_selectableOffsetEdges.push_back(edge);
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
        auto* edge = new SelectableOffsetEdge([this, i]() {
            QTimer::singleShot(
                0, this, [this, i]() { removeArcBend(i); });
        });
        edge->setPath(edgePath);
        m_baseItem->scene()->addItem(edge);
        m_selectableOffsetEdges.push_back(edge);
    }
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

    for (auto& handles : m_offsetHandles) {
        delete handles.first;
        delete handles.second;
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
    for (auto& handles : m_arcBendHandles) {
        delete handles.first;
        delete handles.second;
    }
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
        m_centerHandle = new SectionDragHandle(
            Qt::black,
            [this](const QPointF& point, Qt::KeyboardModifiers modifiers) {
                moveSectionCenter(point, modifiers);
            });
        m_baseItem->scene()->addItem(m_centerHandle);
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
        auto* p1Handle = new SectionDragHandle(
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
        auto* p2Handle = new SectionDragHandle(
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
        m_baseItem->scene()->addItem(p1Handle);
        m_baseItem->scene()->addItem(p2Handle);
        m_offsetHandles.emplace_back(p1Handle, p2Handle);
    }
    while (m_arcBendHandles.size() < m_arcBends.size()) {
        const size_t index = m_arcBendHandles.size();
        const QColor arcColor(145, 70, 190);
        auto* bendHandle = new SectionDragHandle(
            arcColor,
            [this, index](const QPointF& point,
                          Qt::KeyboardModifiers modifiers) {
                moveArcBendPoint(index, false, point, modifiers);
            });
        auto* endpointHandle = new SectionDragHandle(
            arcColor,
            [this, index](const QPointF& point,
                          Qt::KeyboardModifiers modifiers) {
                moveArcBendPoint(index, true, point, modifiers);
            });
        m_baseItem->scene()->addItem(bendHandle);
        m_baseItem->scene()->addItem(endpointHandle);
        m_arcBendHandles.emplace_back(bendHandle, endpointHandle);
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
        m_startRotationHandle = new SectionRotationHandle(
            [this](const QPointF& point) { rotateSectionHalf(true, point); });
        m_baseItem->scene()->addItem(m_startRotationHandle);
    }
    if (!m_endRotationHandle) {
        m_endRotationHandle = new SectionRotationHandle(
            [this](const QPointF& point) { rotateSectionHalf(false, point); });
        m_baseItem->scene()->addItem(m_endRotationHandle);
    }
}

void TaskSectionView::updateSectionControls()
{
    if (m_customComplexSection) {
        clearSectionControls();
        return;
    }
    if (!m_sectionControlsEnabled || !m_baseItem || !m_baseItem->scene()
        || m_hasPendingOffset || m_hasPendingArc) {
        return;
    }
    ensureSectionControls();
    for (QGraphicsItem* item : m_sectionControls) {
        delete item;
    }
    m_sectionControls.clear();

    std::vector<double> bounds{-m_temporaryHalfLength, 0.0,
                               m_temporaryHalfLength};
    for (const auto& step : m_offsetSteps) {
        bounds.push_back(step.along);
        if (step.transition == OffsetStep::Transition::Arc) {
            bounds.push_back(step.startAlong);
        }
    }
    for (const ArcBend& bend : m_arcBends) {
        bounds.push_back(bend.along);
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
        const std::array<SectionActionGlyph::Kind, 3> kinds{
            SectionActionGlyph::Kind::Single,
            SectionActionGlyph::Kind::Notch,
            SectionActionGlyph::Kind::Arc};
        const double offset = projectOffsetStart(
            m_temporaryCenter + tangent * midpoint).second;
        const QPointF position = applyArcBends(
            rotateSectionPoint(
                m_temporaryCenter + tangent * midpoint + normal * offset,
                midpoint),
            midpoint);
        const QPointF directionPoint = applyArcBends(
            rotateSectionPoint(
                m_temporaryCenter
                    + tangent * (midpoint + (towardStart ? -1.0 : 1.0))
                    + normal * offset,
                midpoint),
            midpoint);
        const QPointF scenePosition = m_baseItem->mapToScene(position);
        const QPointF sceneDirection =
            m_baseItem->mapToScene(directionPoint) - scenePosition;
        for (size_t kind = 0; kind < kinds.size(); ++kind) {
            constexpr std::array<double, 3> alongOffsets{0.0, -15.0, 15.0};
            std::function<void()> callback;
            if (kind == 0) {
                callback = [this, midpoint,
                            minimumAlong = bounds[i - 1],
                            maximumAlong = bounds[i], towardStart]() {
                    startSingleOffset(midpoint, minimumAlong,
                                      maximumAlong, towardStart);
                };
            }
            else if (kind == 1) {
                callback = [this, midpoint, minimumAlong = bounds[i - 1],
                            maximumAlong = bounds[i], towardStart]() {
                    startNotchOffset(midpoint, minimumAlong, maximumAlong,
                                     towardStart);
                };
            }
            else {
                callback = [this, midpoint, minimumAlong = bounds[i - 1],
                            maximumAlong = bounds[i], towardStart]() {
                    startArcOffset(midpoint, minimumAlong, maximumAlong,
                                   towardStart);
                };
            }
            auto* item = new SectionActionGlyph(
                kinds[kind], alongOffsets[kind], std::move(callback));
            if (kinds[kind] == SectionActionGlyph::Kind::Arc) {
                item->setArcGeometry(
                    scenePosition, sceneDirection);
            }
            else {
                item->setPos(scenePosition);
                item->setRotation(
                    Base::toDegrees(std::atan2(sceneDirection.y(),
                                              sceneDirection.x())));
            }
            m_baseItem->scene()->addItem(item);
            m_sectionControls.push_back(item);
        }
    }

    const auto points = sectionPathPoints(false);
    if (points.size() >= 2) {
        const QPointF sceneCenter =
            m_baseItem->mapToScene(m_temporaryCenter);
        static_cast<SectionRotationHandle*>(m_startRotationHandle)
            ->setRotationArc(sceneCenter,
                             m_baseItem->mapToScene(points.front().first),
                             true);
        static_cast<SectionRotationHandle*>(m_endRotationHandle)
            ->setRotationArc(sceneCenter,
                             m_baseItem->mapToScene(points.back().first),
                             false);
    }
}

void TaskSectionView::clearSectionControls()
{
    for (QGraphicsItem* item : m_sectionControls) {
        delete item;
    }
    m_sectionControls.clear();
    delete m_startRotationHandle;
    m_startRotationHandle = nullptr;
    delete m_endRotationHandle;
    m_endRotationHandle = nullptr;
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
    for (QGraphicsItem* item : m_sectionControls) {
        item->show();
    }
    if (m_startRotationHandle) {
        m_startRotationHandle->show();
    }
    if (m_endRotationHandle) {
        m_endRotationHandle->show();
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
    for (QGraphicsItem* item : m_selectableOffsetEdges) {
        item->hide();
    }
    for (QGraphicsItem* item : m_sectionControls) {
        item->hide();
    }
    if (m_startRotationHandle) {
        m_startRotationHandle->hide();
    }
    if (m_endRotationHandle) {
        m_endRotationHandle->hide();
    }
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
    const Base::Vector3d origin =
        viewPointToSectionOrigin(m_temporaryCenter);
    const QSignalBlocker blockX(ui->sbOrgX);
    const QSignalBlocker blockY(ui->sbOrgY);
    const QSignalBlocker blockZ(ui->sbOrgZ);
    ui->sbOrgX->setValue(origin.x);
    ui->sbOrgY->setValue(origin.y);
    ui->sbOrgZ->setValue(origin.z);
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
        ? -m_temporaryHalfLength
        : m_offsetSteps[index - 1].along + 1.0e-6;
    const double upper = index + 1 == m_offsetSteps.size()
        ? m_temporaryHalfLength
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
        ? -m_temporaryHalfLength
        : m_offsetSteps[index - 1].along + 1.0e-6;
    const double upper = index + 1 == m_offsetSteps.size()
        ? m_temporaryHalfLength
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
            ? std::clamp(along, -m_temporaryHalfLength, 0.0)
            : std::clamp(along, 0.0, m_temporaryHalfLength);
    }

    drawTemporarySingleOffset();
    showUnappliedPreview(false);
    apply();
}

void TaskSectionView::rotateSectionHalf(bool startHalf,
                                        const QPointF& scenePoint)
{
    if (!m_baseItem) {
        return;
    }
    const QPointF local = m_baseItem->mapFromScene(scenePoint);
    const QPointF reference =
        sectionHalfEndpoint(startHalf) - m_temporaryCenter;
    const QPointF target = local - m_temporaryCenter;
    if (reference.manhattanLength() < 1.0e-6
        || target.manhattanLength() < 1.0e-6) {
        return;
    }
    const double angleDelta = std::atan2(
        reference.x() * target.y() - reference.y() * target.x(),
        QPointF::dotProduct(reference, target));
    if (startHalf) {
        m_startRotation += angleDelta;
    }
    else {
        m_endRotation += angleDelta;
    }
    drawTemporarySingleOffset();
    showUnappliedPreview(false);
    apply();
}

QPointF TaskSectionView::snapViewPoint(
    const QPointF& point,
    bool disableSnapping) const
{
    if (disableSnapping || !m_graphicsView || !m_baseItem || !m_base) {
        return point;
    }

    constexpr double snapPixels = 12.0;
    QPointF best = point;
    double bestDistance = snapPixels;
    const QPoint cursor =
        m_graphicsView->mapFromScene(m_baseItem->mapToScene(point));
    auto consider = [&](const Base::Vector3d& candidate) {
        const QPointF local(Rez::guiX(candidate.x), Rez::guiX(candidate.y));
        const QPoint viewport =
            m_graphicsView->mapFromScene(m_baseItem->mapToScene(local));
        const double distance =
            std::hypot(viewport.x() - cursor.x(), viewport.y() - cursor.y());
        if (distance < bestDistance) {
            bestDistance = distance;
            best = local;
        }
    };

    for (const auto& vertex : m_base->getVertexGeometry()) {
        if (vertex->getHlrVisible()) {
            consider(vertex->point());
        }
    }
    for (const auto& geometry : m_base->getEdgeGeometry()) {
        if (!geometry->getHlrVisible()) {
            continue;
        }
        if (geometry->getGeomType() == GeomType::CIRCLE
            || geometry->getGeomType() == GeomType::ARCOFCIRCLE) {
            consider(std::static_pointer_cast<TechDraw::Circle>(geometry)->center);
        }
        else if (geometry->getGeomType() == GeomType::ELLIPSE
                 || geometry->getGeomType() == GeomType::ARCOFELLIPSE) {
            consider(std::static_pointer_cast<TechDraw::Ellipse>(geometry)->center);
        }
    }
    return best;
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
    const Base::Vector3d origin(ui->sbOrgX->value().getValue(),
                                ui->sbOrgY->value().getValue(),
                                ui->sbOrgZ->value().getValue());
    Base::Vector3d direction = m_viewDirectionWidget->value();
    if (direction.Sqr() <= std::numeric_limits<double>::epsilon()) {
        return;
    }
    direction.Normalize();
    updateTemporarySectionLine(sectionOriginToView(origin),
                               baseToDisplayedDirection(direction));
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
    if (m_section->waitingForResult() || !m_section->hasGeometry()) {
        QTimer::singleShot(
            50, this, [this, request]() { showGeneratedSectionWhenReady(request); });
        return;
    }

    setGeneratedSectionVisible(true);
    if (m_base) {
        m_base->requestPaint();
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

//save the start conditions
void TaskSectionView::saveSectionState()
{
    if (m_section) {
        m_saveSymbol = m_section->SectionSymbol.getValue();
        m_saveScale = m_section->getScale();
        m_saveScaleType = m_section->getScaleType();
        m_saveNormal = m_section->SectionNormal.getValue();
        m_normal = m_saveNormal;
        m_saveDirection = m_section->Direction.getValue();
        m_saveOrigin = m_section->SectionOrigin.getValue();
        m_saveDirName = m_section->SectionDirection.getValueAsString();
        m_saveShapes = m_section->Source.getValues();
        m_saveXShapes = m_section->XSource.getValues();
        if (auto* complexSection =
                dynamic_cast<TechDraw::DrawComplexSection*>(m_section)) {
            m_saveProfileObject =
                complexSection->CuttingToolWireObject.getValue();
            m_saveProjectionStrategy =
                complexSection->ProjectionStrategy.getValue();
        }
        m_saved = true;
    }
}

//restore the start conditions
void TaskSectionView::restoreSectionState()
{
    if (!m_section)
        return;

    m_section->SectionSymbol.setValue(m_saveSymbol);
    m_section->Scale.setValue(m_saveScale);
    m_section->ScaleType.setValue(m_saveScaleType);
    m_section->SectionNormal.setValue(m_saveNormal);
    m_section->Direction.setValue(m_saveDirection);
    m_section->SectionOrigin.setValue(m_saveOrigin);
    m_section->SectionDirection.setValue(m_saveDirName.c_str());
    m_section->Source.setValues(m_saveShapes);
    m_section->XSource.setValues(m_saveXShapes);
    if (auto* complexSection =
            dynamic_cast<TechDraw::DrawComplexSection*>(m_section)) {
        complexSection->CuttingToolWireObject.setValue(
            m_saveProfileObject);
        complexSection->ProjectionStrategy.setValue(
            m_saveProjectionStrategy);
    }
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
    m_displayedDirection *= -1.0;

    // Offset handle callbacks encode which side of a modifier they edit, so
    // rebuild them after the sides have been exchanged.
    for (auto& handles : m_offsetHandles) {
        delete handles.first;
        delete handles.second;
    }
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

//SectionOrigin changed
void TaskSectionView::onXChanged()
{
    showUnappliedPreview();
    apply();
}
void TaskSectionView::onYChanged()
{
    showUnappliedPreview();
    apply();
}
void TaskSectionView::onZChanged()
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
    ui->sbOrgX->setEnabled(enable);
    ui->sbOrgY->setEnabled(enable);
    ui->sbOrgZ->setEnabled(enable);
    ui->cmbScaleType->setEnabled(enable);
    ui->gbOrientation->setEnabled(enable);
    ui->gbPlane->setEnabled(enable);
    ui->cbCustomComplexSection->setEnabled(
        enable
        && (!m_graphicsView
            || !m_graphicsView->isHandlerActive()
            || m_temporarySectionLine));
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
    m_modelIsDirty = true;

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

//*********************************************************************

TechDraw::DrawViewSection* TaskSectionView::createSectionView(void)
{
    if (!isBaseValid()) {
        failNoObject();
        return nullptr;
    }

    std::string baseName = m_base->getNameInDocument();

    int tid = Gui::Command::openActiveDocumentCommand(QT_TRANSLATE_NOOP("Command", "Create Section View"));
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
        std::string temp = Base::Tools::escapeEncodeString(qTemp.toStdString());
        std::string sectionLabel = Base::Tools::escapeEncodeString(makeSectionLabel(qTemp));
        Command::doCommand(Command::Doc, "App.ActiveDocument.%s.SectionSymbol = '%s'",
                           m_sectionName.c_str(), temp.c_str());

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
            Command::doCommand(
                Command::Doc,
                "App.ActiveDocument.%s.SectionOrigin = FreeCAD.Vector(%.6f, %.6f, %.6f)",
                m_sectionName.c_str(), ui->sbOrgX->value().getValue(),
                ui->sbOrgY->value().getValue(), ui->sbOrgZ->value().getValue());
        }

        Command::doCommand(Command::Doc, "App.ActiveDocument.%s.Scale = %0.7f",
                           m_sectionName.c_str(), ui->sbScale->value());

        int scaleType = ui->cmbScaleType->currentIndex();
        Command::doCommand(Command::Doc, "App.ActiveDocument.%s.ScaleType = %d",
                           m_sectionName.c_str(), scaleType);
        Command::doCommand(Command::Doc, "App.ActiveDocument.%s.SectionDirection = '%s'",
                           m_sectionName.c_str(), m_dirName.c_str());

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
        double viewDirectionAngle = m_compass->positiveValue();
        double rotation = requiredRotation(viewDirectionAngle);
        Command::doCommand(Command::Doc, "App.ActiveDocument.%s.Rotation = %.6f",
                           m_sectionName.c_str(), rotation);
    }
    Gui::Command::commitCommand(tid);
    return m_section;
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

    const auto pathPoints = sectionPathPoints(false);
    std::vector<long> transitionEdges;
    auto isTransitionEdge = [this](double firstAlong,
                                   double secondAlong) {
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
        if (isTransitionEdge(pathPoints[i - 1].second,
                             pathPoints[i].second)) {
            transitionEdges.push_back(
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
    const Base::Vector3d center =
        viewPointToSectionOrigin(m_temporaryCenter);
    std::ostringstream state;
    state.precision(17);
    state << 1 << ' '
          << center.x << ' ' << center.y << ' ' << center.z << ' '
          << m_displayedDirection.x << ' '
          << m_displayedDirection.y << ' '
          << m_displayedDirection.z << ' '
          << m_temporaryHalfLength << ' '
          << m_startOffset << ' '
          << m_startRotation << ' '
          << m_endRotation << ' '
          << m_nextModifierId << ' '
          << m_offsetSteps.size();
    for (const OffsetStep& step : m_offsetSteps) {
        state << ' ' << step.along
              << ' ' << step.offset
              << ' ' << static_cast<int>(step.transition)
              << ' ' << step.startAlong
              << ' ' << static_cast<int>(step.role)
              << ' ' << step.modifierId
              << ' ' << static_cast<int>(step.towardStart);
    }
    state << ' ' << m_arcBends.size();
    for (const ArcBend& bend : m_arcBends) {
        state << ' ' << bend.along
              << ' ' << bend.angle
              << ' ' << static_cast<int>(bend.towardStart);
    }
    return state.str();
}

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

    std::istringstream state(stateProperty->getValue());
    int version = 0;
    size_t offsetCount = 0;
    size_t arcCount = 0;
    if (!(state >> version) || version != 1
        || !(state >> m_modernCenterOrigin.x
                   >> m_modernCenterOrigin.y
                   >> m_modernCenterOrigin.z
                   >> m_displayedDirection.x
                   >> m_displayedDirection.y
                   >> m_displayedDirection.z
                   >> m_temporaryHalfLength
                   >> m_startOffset
                   >> m_startRotation
                   >> m_endRotation
                   >> m_nextModifierId
                   >> offsetCount)) {
        return false;
    }

    std::vector<OffsetStep> offsets;
    offsets.reserve(offsetCount);
    for (size_t i = 0; i < offsetCount; ++i) {
        OffsetStep step;
        int transition = 0;
        int role = 0;
        int towardStart = 0;
        if (!(state >> step.along
                   >> step.offset
                   >> transition
                   >> step.startAlong
                   >> role
                   >> step.modifierId
                   >> towardStart)) {
            return false;
        }
        if (transition < static_cast<int>(OffsetStep::Transition::Sharp)
            || transition > static_cast<int>(OffsetStep::Transition::Arc)
            || role < static_cast<int>(OffsetStep::Role::Generic)
            || role > static_cast<int>(OffsetStep::Role::NotchOuter)) {
            return false;
        }
        step.transition =
            static_cast<OffsetStep::Transition>(transition);
        step.role = static_cast<OffsetStep::Role>(role);
        step.towardStart = towardStart != 0;
        offsets.push_back(step);
    }

    if (!(state >> arcCount)) {
        return false;
    }
    std::vector<ArcBend> arcs;
    arcs.reserve(arcCount);
    for (size_t i = 0; i < arcCount; ++i) {
        ArcBend bend;
        int towardStart = 0;
        if (!(state >> bend.along >> bend.angle >> towardStart)) {
            return false;
        }
        bend.towardStart = towardStart != 0;
        arcs.push_back(bend);
    }

    m_offsetSteps = std::move(offsets);
    m_arcBends = std::move(arcs);
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
    const std::string oldSectionName = m_section->getNameInDocument();
    const int tid = Gui::Command::openActiveDocumentCommand(
        QT_TRANSLATE_NOOP("Command", "Convert to Complex Section"));
    Command::doCommand(
        Command::Doc,
        "App.ActiveDocument.%s.removeView(App.ActiveDocument.%s)",
        m_savePageName.c_str(), oldSectionName.c_str());
    Command::doCommand(
        Command::Doc,
        "App.ActiveDocument.removeObject('%s')",
        oldSectionName.c_str());
    Gui::Command::commitCommand(tid);
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

    int tid = Gui::Command::openActiveDocumentCommand(QT_TRANSLATE_NOOP("Command", "Edit Section View"));
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
        if (!hasComplexPath()) {
            Command::doCommand(
                Command::Doc,
                "App.ActiveDocument.%s.SectionOrigin = FreeCAD.Vector(%.3f, %.3f, %.3f)",
                m_sectionName.c_str(), ui->sbOrgX->value().getValue(),
                ui->sbOrgY->value().getValue(), ui->sbOrgZ->value().getValue());
        }

        QString qTemp = ui->leSymbol->text();
        std::string temp = Base::Tools::escapeEncodeString(qTemp.toStdString());
        std::string sectionLabel = Base::Tools::escapeEncodeString(makeSectionLabel(qTemp));
        Command::doCommand(Command::Doc, "App.ActiveDocument.%s.SectionSymbol = '%s'",
                           m_sectionName.c_str(), temp.c_str());

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
        if (directionChanged()) {
            double viewDirectionAngle = m_compass->positiveValue();
            double rotation = requiredRotation(viewDirectionAngle);
            Command::doCommand(Command::Doc, "App.ActiveDocument.%s.Rotation = %.6f",
                               m_sectionName.c_str(), rotation);
            directionChanged(false);
        }
    }
    Gui::Command::commitCommand(tid);
}

std::string TaskSectionView::makeSectionLabel(QString symbol)
{
    const std::string objectName(
        hasComplexPath() ? "ComplexSection" : "SectionView");
    std::string uniqueSuffix{m_sectionName.substr(objectName.length(), std::string::npos)};
    std::string uniqueLabel = "Section" + uniqueSuffix;
    std::string temp = symbol.toStdString();
    return ( uniqueLabel + " " + temp + " - " + temp );
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
double TaskSectionView::requiredRotation(double inputAngle)
{
    double rotation = inputAngle - 90.0;
    if (rotation == 180.0) {
        //if the view direction is 90/270, then the section is drawn properly and no
        //rotation is needed.  90.0 becomes 0.0, but 270.0 needs special handling.
        rotation = 0.0;
    }
    return rotation;
}

//******************************************************************************

bool TaskSectionView::accept()
{
    if (!apply(true)) {
        return false;
    }
    setGeneratedSectionVisible(true);
    Gui::Command::doCommand(Gui::Command::Gui, "Gui.ActiveDocument.resetEdit()");
    return true;
}

bool TaskSectionView::reject()
{
    if (!m_section) {//no section created, nothing to undo
        Gui::Command::doCommand(Gui::Command::Gui, "Gui.ActiveDocument.resetEdit()");
        return false;
    }

    if (!isSectionValid()) {//section !exist. nothing to undo
        if (isBaseValid()) {
            m_base->requestPaint();
        }
        Gui::Command::doCommand(Gui::Command::Gui, "Gui.ActiveDocument.resetEdit()");
        return false;
    }

    setGeneratedSectionVisible(true);

    if (m_createMode) {
        std::string SectionName = m_section->getNameInDocument();
        Gui::Command::doCommand(Gui::Command::Gui,
                                "App.ActiveDocument.%s.removeView(App.ActiveDocument.%s)",
                                m_savePageName.c_str(), SectionName.c_str());
        Gui::Command::doCommand(Gui::Command::Gui, "App.ActiveDocument.removeObject('%s')",
                                SectionName.c_str());
        if (isGeneratedProfile(m_profileObject)) {
            Gui::Command::doCommand(
                Gui::Command::Gui,
                "App.ActiveDocument.removeObject('%s')",
                m_profileName.c_str());
            m_profileName.clear();
        }

    } else {
        if (m_modelIsDirty) {
            restoreSectionState();
            m_section->recomputeFeature();
            m_section->requestPaint();
        }
    }

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

            const QPointF localPoint = m_baseItem->mapFromScene(scenePoint);
            m_center = event->modifiers().testFlag(Qt::ControlModifier)
                ? localPoint
                : snapPoint(localPoint);
            QPointF previewCenter = m_center;
            if (m_task->positionMode()
                == TaskSectionView::PositionMode::TwoPoints) {
                const QPointF tangent(
                    -m_direction.y, -m_direction.x);
                previewCenter -= tangent * m_halfLength;
            }
            updatePreview(previewCenter, m_direction);
            m_task->setInteractiveParameters(
                toSectionOrigin(previewCenter),
                toBaseDirection(m_direction));
        }
        else {
            QPointF localPoint = m_baseItem->mapFromScene(scenePoint);
            Base::Vector3d lineDirection;
            const DirectionSnap snap = preselectedDirectionSnap(
                event->pos(), localPoint, lineDirection);
            const TaskSectionView::PositionMode positionMode =
                m_task->positionMode();
            if (snap == DirectionSnap::Line
                && positionMode
                    == TaskSectionView::PositionMode::
                        CenterAndViewDirection) {
                m_direction = lineDirection;
                updatePreview(m_center, m_direction);
                m_task->setInteractiveParameters(
                    m_lockedOrigin, toBaseDirection(m_direction));
                event->accept();
                return;
            }

            const QPointF reference =
                positionMode
                    == TaskSectionView::PositionMode::TwoPoints
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
                == TaskSectionView::PositionMode::
                    CenterAndRightPoint) {
                tangent *= -1.0;
            }

            if (positionMode
                == TaskSectionView::PositionMode::TwoPoints) {
                double length = QPointF::dotProduct(delta, tangent);
                if (length < 0.0) {
                    tangent *= -1.0;
                    length *= -1.0;
                }
                const QPointF secondPoint =
                    m_firstPoint + tangent * length;
                m_center = (m_firstPoint + secondPoint) * 0.5;
                m_halfLength = length * 0.5;
                m_lockedOrigin = toSectionOrigin(m_center);
                tangent *= -1.0;
                m_direction = Base::Vector3d(
                    -tangent.y(), -tangent.x(), 0.0);
                m_task->updateTemporarySectionLine(
                    m_center, m_direction, m_halfLength);
            }
            else {
                m_direction = Base::Vector3d(
                    -tangent.y(), -tangent.x(), 0.0);
                updatePreview(m_center, m_direction);
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
            m_mode = Mode::SeekDirection;
            m_firstPoint = m_center;
            m_lockedOrigin = toSectionOrigin(m_center);
            m_task->adoptTemporarySectionLine(m_preview, m_baseItem, m_halfLength);
            m_preview = nullptr;
            if (m_task->positionMode()
                == TaskSectionView::PositionMode::TwoPoints) {
                const QPointF tangent(
                    -m_direction.y, -m_direction.x);
                const QPointF previewCenter =
                    m_center - tangent * m_halfLength;
                m_lockedOrigin =
                    toSectionOrigin(previewCenter);
                m_task->updateTemporarySectionLine(
                    previewCenter, m_direction);
            }
            else {
                m_task->updateTemporarySectionLine(
                    m_center, m_direction);
            }
            m_task->setDirectionControlsVisible(true);
            m_task->setInteractiveParameters(m_lockedOrigin, toBaseDirection(m_direction));
        }
        else {
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
            m_halfLength = 0.0;
            m_mode = Mode::SeekCenter;
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
        SeekDirection
    };

    enum class DirectionSnap
    {
        None,
        Point,
        Line
    };

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
        constexpr double snapPixels = 12.0;
        QPointF best = point;
        double bestDistance = snapPixels;

        auto consider = [&](const Base::Vector3d& candidate) {
            const QPointF local(Rez::guiX(candidate.x), Rez::guiX(candidate.y));
            const QPoint viewport =
                viewPage->mapFromScene(m_baseItem->mapToScene(local));
            const QPoint cursor =
                viewPage->mapFromScene(m_baseItem->mapToScene(point));
            const double distance =
                std::hypot(viewport.x() - cursor.x(), viewport.y() - cursor.y());
            if (distance < bestDistance) {
                bestDistance = distance;
                best = local;
            }
        };

        for (const auto& vertex : m_base->getVertexGeometry()) {
            if (vertex->getHlrVisible()) {
                consider(vertex->point());
            }
        }
        for (const auto& geometry : m_base->getEdgeGeometry()) {
            if (!geometry->getHlrVisible()) {
                continue;
            }
            if (geometry->getGeomType() == GeomType::CIRCLE
                || geometry->getGeomType() == GeomType::ARCOFCIRCLE) {
                consider(std::static_pointer_cast<TechDraw::Circle>(geometry)->center);
            }
            else if (geometry->getGeomType() == GeomType::ELLIPSE
                     || geometry->getGeomType() == GeomType::ARCOFELLIPSE) {
                consider(std::static_pointer_cast<TechDraw::Ellipse>(geometry)->center);
            }
        }
        return best;
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
        m_preview = new QGISectionLine();
        viewPage->getScene()->addItem(m_preview);
        m_preview->setSymbol(const_cast<char*>(""));
        m_preview->setPathMode(false);
        m_preview->setSectionColor(Qt::black);

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

        double minX = std::numeric_limits<double>::max();
        double minY = std::numeric_limits<double>::max();
        double maxX = std::numeric_limits<double>::lowest();
        double maxY = std::numeric_limits<double>::lowest();
        auto extendBounds = [&](const Base::Vector3d& point) {
            const double x = Rez::guiX(point.x);
            const double y = Rez::guiX(point.y);
            minX = std::min(minX, x);
            minY = std::min(minY, y);
            maxX = std::max(maxX, x);
            maxY = std::max(maxY, y);
        };
        for (const auto& vertex : m_base->getVertexGeometry()) {
            extendBounds(vertex->point());
        }
        for (const auto& geometry : m_base->getEdgeGeometry()) {
            for (const auto& point : geometry->findEndPoints()) {
                extendBounds(point);
            }
        }
        const double span = (minX <= maxX && minY <= maxY)
            ? std::max(maxX - minX, maxY - minY)
            : Rez::guiX(50.0);
        m_halfLength = std::max(Rez::guiX(10.0), span * 0.65);
    }

    void updatePreview(const QPointF& center, const Base::Vector3d& direction)
    {
        if (!m_preview) {
            m_task->updateTemporarySectionLine(center, direction);
            return;
        }
        const QPointF tangent(-direction.y, -direction.x);
        const QPointF localStart = center - tangent * m_halfLength;
        const QPointF localEnd = center + tangent * m_halfLength;
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

    void clearPreview()
    {
        delete m_preview;
        m_preview = nullptr;
    }

    TaskSectionView* m_task{nullptr};
    QGIViewPart* m_baseItem{nullptr};
    DrawViewPart* m_base{nullptr};
    QGISectionLine* m_preview{nullptr};
    Mode m_mode{Mode::SeekCenter};
    QPointF m_center;
    QPointF m_firstPoint;
    Base::Vector3d m_lockedOrigin;
    Base::Vector3d m_direction{1.0, 0.0, 0.0};
    double m_halfLength{0.0};
    bool m_completed{false};
};

class SingleOffsetHandler final : public TechDrawHandler
{
public:
    SingleOffsetHandler(TaskSectionView* task,
                        double along,
                        double startOffset,
                        double minimumAlong,
                        double maximumAlong,
                        bool towardStart) :
        m_task(task),
        m_along(along),
        m_offset(startOffset),
        m_minimumAlong(minimumAlong),
        m_maximumAlong(maximumAlong),
        m_towardStart(towardStart)
    {}

    ~SingleOffsetHandler() override
    {
        delete m_pointMarker;
    }

    void mouseMoveEvent(QMouseEvent* event) override
    {
        if (m_completed) {
            event->accept();
            return;
        }
        QGIViewPart* baseItem = m_task ? m_task->baseItem() : nullptr;
        if (!viewPage || !baseItem) {
            return;
        }
        QPointF local =
            baseItem->mapFromScene(viewPage->mapToScene(event->pos()));
        local = m_task->snapViewPoint(
            local, event->modifiers().testFlag(Qt::ControlModifier));
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
        if (event->button() != Qt::LeftButton || !m_task
            || !m_task->baseItem() || m_completed) {
            return;
        }
        if (m_mode == Mode::SeekP1) {
            m_task->beginSingleOffset(
                m_along, m_offset, m_towardStart);
            m_mode = Mode::SeekP2;
            event->accept();
            return;
        }
        m_task->updatePendingSingleOffset(m_offset);
        m_task->commitPendingSingleOffset();
        m_completed = true;
        event->accept();
        QGVPage* page = viewPage;
        TaskSectionView* task = m_task;
        QTimer::singleShot(0, page, [page, task]() {
            page->deactivateHandler();
            QTimer::singleShot(0, task, [task]() {
                task->apply();
            });
        });
        return;
    }

    void mouseReleaseEvent(QMouseEvent* event) override
    {
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
        if (!m_completed && m_task) {
            m_task->cancelPendingSingleOffset();
        }
        if (m_task) {
            m_task->setSectionControlsEnabled(true);
            m_task->showSectionHandles();
        }
        TechDrawHandler::deactivate();
    }

private:
    enum class Mode
    {
        SeekP1,
        SeekP2
    };

    QString getCrosshairCursorSVGName() const override
    {
        return QStringLiteral("TechDraw_SectionView_Pointer");
    }

    void updateMarker(const QPointF& localPoint)
    {
        QGIViewPart* baseItem = m_task->baseItem();
        if (!m_pointMarker) {
            constexpr double markerRadius = 4.0;
            m_pointMarker = new QGraphicsEllipseItem(
                -markerRadius, -markerRadius, markerRadius * 2.0, markerRadius * 2.0);
            m_pointMarker->setPen(QPen(Qt::black));
            m_pointMarker->setBrush(Qt::black);
            m_pointMarker->setZValue(1001.0);
            m_pointMarker->setFlag(QGraphicsItem::ItemIgnoresTransformations);
            viewPage->getScene()->addItem(m_pointMarker);
        }
        m_pointMarker->setPos(baseItem->mapToScene(localPoint));
    }

    TaskSectionView* m_task{nullptr};
    QGraphicsEllipseItem* m_pointMarker{nullptr};
    Mode m_mode{Mode::SeekP1};
    double m_along{0.0};
    double m_offset{0.0};
    double m_minimumAlong{0.0};
    double m_maximumAlong{0.0};
    bool m_towardStart{false};
    bool m_completed{false};
};

class NotchOffsetHandler final : public TechDrawHandler
{
public:
    NotchOffsetHandler(TaskSectionView* task,
                       double initialAlong,
                       double initialOffset,
                       double minimumAlong,
                       double maximumAlong,
                       bool towardStart) :
        m_task(task),
        m_along(initialAlong),
        m_outerAlong(initialAlong),
        m_offset(initialOffset),
        m_minimumAlong(minimumAlong),
        m_maximumAlong(maximumAlong),
        m_towardStart(towardStart)
    {}

    ~NotchOffsetHandler() override
    {
        delete m_pointMarker;
    }

    void mouseMoveEvent(QMouseEvent* event) override
    {
        if (m_completed) {
            event->accept();
            return;
        }
        QGIViewPart* baseItem = m_task ? m_task->baseItem() : nullptr;
        if (!viewPage || !baseItem) {
            return;
        }
        QPointF local =
            baseItem->mapFromScene(viewPage->mapToScene(event->pos()));
        local = m_task->snapViewPoint(
            local, event->modifiers().testFlag(Qt::ControlModifier));
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
        if (event->button() != Qt::LeftButton || !m_task
            || !m_task->baseItem() || m_completed) {
            return;
        }
        if (m_mode == Mode::SeekP1) {
            m_task->beginSingleOffset(m_along, m_offset, m_towardStart);
            m_mode = Mode::SeekP2;
        }
        else if (m_mode == Mode::SeekP2) {
            m_task->updatePendingSingleOffset(m_offset);
            m_mode = Mode::SeekP3;
        }
        else {
            m_task->updatePendingNotchOuterAlong(m_outerAlong);
            m_task->commitPendingSingleOffset();
            m_completed = true;
            QGVPage* page = viewPage;
            TaskSectionView* task = m_task;
            QTimer::singleShot(0, page, [page, task]() {
                page->deactivateHandler();
                QTimer::singleShot(0, task, [task]() {
                    task->apply();
                });
            });
        }
        event->accept();
    }

    void mouseReleaseEvent(QMouseEvent* event) override
    {
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
        if (!m_completed && m_task) {
            m_task->cancelPendingSingleOffset();
        }
        if (m_task) {
            m_task->setSectionControlsEnabled(true);
            m_task->showSectionHandles();
        }
        TechDrawHandler::deactivate();
    }

private:
    enum class Mode
    {
        SeekP1,
        SeekP2,
        SeekP3
    };

    QString getCrosshairCursorSVGName() const override
    {
        return QStringLiteral("TechDraw_SectionView_Pointer");
    }

    void updateMarker(const QPointF& localPoint)
    {
        QGIViewPart* baseItem = m_task->baseItem();
        if (!m_pointMarker) {
            constexpr double markerRadius = 4.0;
            m_pointMarker = new QGraphicsEllipseItem(
                -markerRadius, -markerRadius,
                markerRadius * 2.0, markerRadius * 2.0);
            m_pointMarker->setPen(QPen(Qt::black));
            m_pointMarker->setBrush(Qt::black);
            m_pointMarker->setZValue(1001.0);
            m_pointMarker->setFlag(
                QGraphicsItem::ItemIgnoresTransformations);
            viewPage->getScene()->addItem(m_pointMarker);
        }
        m_pointMarker->setPos(baseItem->mapToScene(localPoint));
    }

    TaskSectionView* m_task{nullptr};
    QGraphicsEllipseItem* m_pointMarker{nullptr};
    Mode m_mode{Mode::SeekP1};
    double m_along{0.0};
    double m_outerAlong{0.0};
    double m_offset{0.0};
    double m_minimumAlong{0.0};
    double m_maximumAlong{0.0};
    bool m_towardStart{false};
    bool m_completed{false};
};

class ArcOffsetHandler final : public TechDrawHandler
{
public:
    ArcOffsetHandler(TaskSectionView* task,
                     double along,
                     double minimumAlong,
                     double maximumAlong,
                     bool towardStart) :
        m_task(task),
        m_along(along),
        m_minimumAlong(minimumAlong),
        m_maximumAlong(maximumAlong),
        m_towardStart(towardStart)
    {}

    ~ArcOffsetHandler() override
    {
        delete m_pointMarker;
    }

    void mouseMoveEvent(QMouseEvent* event) override
    {
        if (m_completed) {
            event->accept();
            return;
        }
        QGIViewPart* baseItem = m_task ? m_task->baseItem() : nullptr;
        if (!viewPage || !baseItem) {
            return;
        }
        QPointF local =
            baseItem->mapFromScene(viewPage->mapToScene(event->pos()));
        local = m_task->snapViewPoint(
            local, event->modifiers().testFlag(Qt::ControlModifier));
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
        if (event->button() != Qt::LeftButton || !m_task
            || !m_task->baseItem() || m_completed) {
            return;
        }
        if (m_mode == Mode::SeekP1) {
            m_task->beginArcOffset(m_along, m_towardStart);
            m_mode = Mode::SeekAngle;
            event->accept();
            return;
        }
        m_task->updatePendingArcAngle(m_angle);
        m_task->commitPendingArc();
        m_completed = true;
        event->accept();
        QGVPage* page = viewPage;
        TaskSectionView* task = m_task;
        QTimer::singleShot(0, page, [page, task]() {
            page->deactivateHandler();
            QTimer::singleShot(0, task, [task]() {
                task->apply();
            });
        });
    }

    void mouseReleaseEvent(QMouseEvent* event) override
    {
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
        if (!m_completed && m_task) {
            m_task->cancelPendingArc();
        }
        if (m_task) {
            m_task->setSectionControlsEnabled(true);
            m_task->showSectionHandles();
        }
        TechDrawHandler::deactivate();
    }

private:
    enum class Mode
    {
        SeekP1,
        SeekAngle
    };

    QString getCrosshairCursorSVGName() const override
    {
        return QStringLiteral("TechDraw_SectionView_Pointer");
    }

    void updateMarker(const QPointF& localPoint)
    {
        QGIViewPart* baseItem = m_task->baseItem();
        if (!m_pointMarker) {
            constexpr double markerRadius = 4.0;
            m_pointMarker = new QGraphicsEllipseItem(
                -markerRadius, -markerRadius,
                markerRadius * 2.0, markerRadius * 2.0);
            m_pointMarker->setPen(QPen(Qt::black));
            m_pointMarker->setBrush(Qt::black);
            m_pointMarker->setZValue(1001.0);
            m_pointMarker->setFlag(
                QGraphicsItem::ItemIgnoresTransformations);
            viewPage->getScene()->addItem(m_pointMarker);
        }
        m_pointMarker->setPos(baseItem->mapToScene(localPoint));
    }

    TaskSectionView* m_task{nullptr};
    QGraphicsEllipseItem* m_pointMarker{nullptr};
    Mode m_mode{Mode::SeekP1};
    double m_along{0.0};
    double m_angle{0.0};
    double m_minimumAlong{0.0};
    double m_maximumAlong{0.0};
    bool m_towardStart{false};
    bool m_completed{false};
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
