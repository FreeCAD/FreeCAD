// SPDX-License-Identifier: LGPL-2.1-or-later
/****************************************************************************
 *                                                                          *
 *   Copyright (c) 2026 AstoCAD     <hello@astocad.com>                     *
 *                                                                          *
 *   This file is part of FreeCAD.                                          *
 *                                                                          *
 *   FreeCAD is free software: you can redistribute it and/or modify it     *
 *   under the terms of the GNU Lesser General Public License as            *
 *   published by the Free Software Foundation, either version 2.1 of the   *
 *   License, or (at your option) any later version.                        *
 *                                                                          *
 *   FreeCAD is distributed in the hope that it will be useful, but         *
 *   WITHOUT ANY WARRANTY; without even the implied warranty of             *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU       *
 *   Lesser General Public License for more details.                        *
 *                                                                          *
 *   You should have received a copy of the GNU Lesser General Public       *
 *   License along with FreeCAD. If not, see                                *
 *   <https://www.gnu.org/licenses/>.                                       *
 *                                                                          *
 ***************************************************************************/

#include <QComboBox>
#include <QDoubleSpinBox>
#include <QGraphicsPathItem>
#include <QGraphicsScene>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QPainterPath>
#include <QPen>
#include <QTimer>

#include <cmath>
#include <list>

#include <App/Document.h>
#include <Base/Converter.h>
#include <Base/Tools.h>
#include <Gui/BitmapFactory.h>
#include <Gui/Control.h>
#include <Gui/InputHint.h>
#include <Gui/MainWindow.h>
#include <Gui/Selection/Selection.h>
#include <Gui/TaskView/TaskView.h>
#include <Mod/TechDraw/App/DrawPage.h>
#include <Mod/TechDraw/App/DrawUtil.h>
#include <Mod/TechDraw/App/DrawViewPart.h>

#include "ui_TaskBrokenView.h"
#include "QGIBreakLine.h"
#include "QGIViewPart.h"
#include "QGSPage.h"
#include "QGVPage.h"
#include "Rez.h"
#include "TaskBrokenView.h"
#include "TechDrawHandler.h"

using namespace TechDraw;
using namespace TechDrawGui;

namespace
{

class BrokenViewHandler final : public TechDrawHandler
{
public:
    explicit BrokenViewHandler(TaskBrokenView* task) : m_task(task) {}

    ~BrokenViewHandler() override
    {
        clearPreview(m_hoverPreview);
        clearPreview(m_fixedPreview);
    }

    void mouseMoveEvent(QMouseEvent* event) override
    {
        if (!viewPage || !m_task) {
            return;
        }
        QGIViewPart* hovered = findView(event->pos());
        if (m_pending && hovered != m_baseItem) {
            clearPreview(m_hoverPreview);
            return;
        }
        if (!m_pending) {
            m_baseItem = hovered;
            m_base = hovered
                ? dynamic_cast<DrawViewPart*>(hovered->getViewObject())
                : nullptr;
        }
        if (!m_baseItem || !m_base) {
            clearPreview(m_hoverPreview);
            return;
        }

        const QPointF local = m_baseItem->mapFromScene(
            viewPage->mapToScene(event->pos()));
        updatePreview(m_hoverPreview,
                      local,
                      m_pending ? m_angle : m_task->angle(),
                      m_pending ? m_type : m_task->breakType());
        event->accept();
    }

    void mousePressEvent(QMouseEvent* event) override
    {
        if (event->button() != Qt::LeftButton || !m_baseItem || !m_base) {
            return;
        }
        const QPointF local = m_baseItem->mapFromScene(
            viewPage->mapToScene(event->pos()));
        if (!m_pending) {
            const double angle = m_task->angle();
            const auto type = m_task->breakType();
            if (!updatePreview(m_fixedPreview, local, angle, type)) {
                event->accept();
                return;
            }
            m_firstPoint = local;
            m_angle = angle;
            m_gap = m_task->gap();
            m_type = type;
            m_pending = true;
            updateHint();
        }
        else {
            const double radians = Base::toRadians(m_angle);
            const QPointF tangent(std::cos(radians), -std::sin(radians));
            if (!QGIBreakLine::clipLine(
                    local, tangent, QGIBreakLine::clipBounds(m_base))) {
                event->accept();
                return;
            }
            createBreak(local);
        }
        event->accept();
    }

    void mouseReleaseEvent(QMouseEvent* event) override
    {
        if (event->button() == Qt::RightButton) {
            if (m_pending) {
                resetPending();
            }
            else {
                QTimer::singleShot(
                    0, Gui::getMainWindow(),
                    []() { Gui::Control().closeDialog(); });
            }
            event->accept();
            return;
        }
        if (event->button() == Qt::LeftButton) {
            if (viewPage->getScene()) {
                viewPage->getScene()->clearSelection();
            }
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
        clearPreview(m_hoverPreview);
        clearPreview(m_fixedPreview);
        TechDrawHandler::deactivate();
    }

private:
    std::list<Gui::InputHint> getToolHints() const override
    {
        using enum Gui::InputHint::UserInput;

        if (m_pending) {
            return {
                {QObject::tr("%1 place second break line"), {MouseLeft}},
                {QObject::tr("%1 cancel pending break"), {MouseRight}},
            };
        }
        return {
            {QObject::tr("%1 place first break line"), {MouseLeft}},
            {QObject::tr("%1 close broken view tool"), {MouseRight}},
        };
    }

    QGIViewPart* findView(const QPoint& viewportPoint)
    {
        const QList<QGraphicsItem*> items = viewPage->items(viewportPoint);
        for (QGraphicsItem* item : items) {
            for (QGraphicsItem* parent = item; parent;
                 parent = parent->parentItem()) {
                auto* view = dynamic_cast<QGIViewPart*>(parent);
                auto* object = view
                    ? dynamic_cast<DrawViewPart*>(view->getViewObject())
                    : nullptr;
                if (view && view->isVisible() && object
                    && object->supportsViewBreaks()
                    && object->findParentPage() == getPage()) {
                    return view;
                }
            }
        }
        return nullptr;
    }

    bool updatePreview(QGraphicsPathItem*& preview,
                       const QPointF& localPoint,
                       double angle,
                       DrawViewPart::BreakType type)
    {
        if (!preview) {
            preview = new QGraphicsPathItem();
            QPen pen(QColor(35, 95, 210), 1.5);
            pen.setCosmetic(true);
            preview->setPen(pen);
            preview->setZValue(1005.0);
            viewPage->getScene()->addItem(preview);
        }
        const double radians = Base::toRadians(angle);
        const QPointF tangent(std::cos(radians), -std::sin(radians));
        const auto ends = QGIBreakLine::clipLine(
            localPoint, tangent, QGIBreakLine::clipBounds(m_base));
        if (!ends) {
            preview->hide();
            return false;
        }
        const QPainterPath localPath =
            QGIBreakLine::styledLine(ends->first, ends->second, type);
        preview->setPath(m_baseItem->sceneTransform().map(localPath));
        preview->show();
        return true;
    }

    void clearPreview(QGraphicsPathItem*& preview)
    {
        delete preview;
        preview = nullptr;
    }

    Base::Vector3d modelPoint(const QPointF& local) const
    {
        const double scale = m_base->getScale();
        const Base::Vector3d displayed(
            Rez::appX(local.x()) / scale,
            -Rez::appX(local.y()) / scale,
            0.0);
        return m_base->mapPointFromBrokenView(displayed);
    }

    void createBreak(const QPointF& secondLocal)
    {
        Base::Vector3d first = modelPoint(m_firstPoint);
        const Base::Vector3d second = modelPoint(secondLocal);
        const double radians = Base::toRadians(m_angle);
        const Base::Vector3d normal2d(
            -std::sin(radians), std::cos(radians), 0.0);
        const gp_Ax2 axes = m_base->getRotatedCS();
        Base::Vector3d normal =
            Base::convertTo<Base::Vector3d>(axes.XDirection()) * normal2d.x
            + Base::convertTo<Base::Vector3d>(axes.YDirection()) * normal2d.y;
        normal.Normalize();
        const double separation = (second - first).Dot(normal);
        if (std::abs(separation) <= EWTOLERANCE) {
            return;
        }
        const Base::Vector3d alignedSecond = first + normal * separation;

        App::Document* document = m_base->getDocument();
        document->openTransaction(
            QT_TRANSLATE_NOOP("Command", "Create view break"));
        m_base->addBreak(first, alignedSecond, normal, m_gap, m_type);
        document->commitTransaction();
        document->recompute();
        m_base->requestPaint();
        resetPending();
    }

    void resetPending()
    {
        clearPreview(m_fixedPreview);
        clearPreview(m_hoverPreview);
        m_pending = false;
        m_baseItem = nullptr;
        m_base = nullptr;
        m_firstPoint = {};
        updateHint();
    }

    TaskBrokenView* m_task{nullptr};
    QGIViewPart* m_baseItem{nullptr};
    DrawViewPart* m_base{nullptr};
    QGraphicsPathItem* m_hoverPreview{nullptr};
    QGraphicsPathItem* m_fixedPreview{nullptr};
    QPointF m_firstPoint;
    double m_angle{0.0};
    double m_gap{10.0};
    DrawViewPart::BreakType m_type{DrawViewPart::BreakType::ZIGZAG};
    bool m_pending{false};
};

} // namespace

TaskBrokenView::TaskBrokenView(QWidget* parent)
    : QWidget(parent)
    , ui(new Ui_TaskBrokenView)
{
    ui->setupUi(this);
    ui->styleCombo->setItemData(
        0, static_cast<int>(DrawViewPart::BreakType::ZIGZAG));
    ui->styleCombo->setItemData(
        1, static_cast<int>(DrawViewPart::BreakType::SIMPLE));
    ui->styleCombo->setItemData(
        2, static_cast<int>(DrawViewPart::BreakType::SINUSOID));

    connect(ui->orientationCombo,
            qOverload<int>(&QComboBox::currentIndexChanged),
            this,
            [this](int index) {
                const bool custom = index == 2;
                ui->angleLabel->setVisible(custom);
                ui->angleSpin->setVisible(custom);
            });
}

TaskBrokenView::~TaskBrokenView() = default;

DrawViewPart::BreakType TaskBrokenView::breakType() const
{
    return static_cast<DrawViewPart::BreakType>(
        ui->styleCombo->currentData().toInt());
}

double TaskBrokenView::gap() const
{
    return ui->gapSpin->value();
}

double TaskBrokenView::angle() const
{
    if (ui->orientationCombo->currentIndex() == 0) {
        return 0.0;
    }
    if (ui->orientationCombo->currentIndex() == 1) {
        return 90.0;
    }
    return ui->angleSpin->value();
}

TaskDlgBrokenView::TaskDlgBrokenView(DrawPage* page, QGVPage* graphicsView) :
    m_page(page),
    m_graphicsView(graphicsView)
{
    m_widget = new TaskBrokenView();
    m_taskBox = new Gui::TaskView::TaskBox(
        Gui::BitmapFactory().pixmap("actions/TechDraw_BrokenView"),
        m_widget->windowTitle(),
        true,
        nullptr);
    m_taskBox->groupLayout()->addWidget(m_widget);
    Content.push_back(m_taskBox);
}

TaskDlgBrokenView::~TaskDlgBrokenView()
{
    if (m_graphicsView && m_graphicsView->isHandlerActive()) {
        m_graphicsView->deactivateHandler();
    }
}

void TaskDlgBrokenView::open()
{
    if (m_graphicsView) {
        m_graphicsView->activateHandler(new BrokenViewHandler(m_widget));
    }
}

bool TaskDlgBrokenView::accept()
{
    return true;
}

bool TaskDlgBrokenView::reject()
{
    return true;
}
