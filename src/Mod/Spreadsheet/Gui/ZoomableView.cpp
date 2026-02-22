// SPDX-License-Identifier: LGPL-2.1-or-later
/****************************************************************************
 *                                                                          *
 *   Copyright (c) 2024 xtemp09 <xtemp09@gmail.com>                         *
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

#include <QInputDialog>
#include <QGraphicsProxyWidget>

#include "ZoomableView.h"
#include "ui_Sheet.h"


ZoomableView::ZoomableView(Ui::Sheet* ui)
    : zoomMin(ui->zoomSpinBox->minimum())
    , zoomMax(ui->zoomSpinBox->maximum())
    , stv {ui->cells}
{
    if (!stv) {
        Base::Console().developerWarning("ZoomableView", "Failed to find a SheetTableView object");
        deleteLater();
        return;
    }

    QLayoutItem* li_stv = stv->parentWidget()->layout()->replaceWidget(stv, this);
    if (li_stv == nullptr) {
        Base::Console().developerWarning("ZoomableView", "Failed to replace the SheetTableView object");
        deleteLater();
        return;
    }
    delete li_stv;

    stv->setParent(nullptr);
    qpw = m_scene.addWidget(stv);
    setScene(&m_scene);

    setBackgroundBrush(Qt::transparent);
    setFrameStyle(QFrame::NoFrame);

    setSizePolicy(QSizePolicy {QSizePolicy::Expanding, QSizePolicy::Expanding});

    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    stv->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    stv->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);

    connect(ui->zoomSpinBox, &QSpinBox::valueChanged, this, &ZoomableView::setZoomLevel);
    connect(
        this,
        &ZoomableView::zoomLevelChanged,
        ui->zoomSpinBox,
        [zoomSpinBox = ui->zoomSpinBox](int new_zoomLevel) {
            zoomSpinBox->blockSignals(true);
            zoomSpinBox->setValue(new_zoomLevel);
            zoomSpinBox->blockSignals(false);
        }
    );

    resetZoom();

    auto connectCursorChangedSignal = [this](QHeaderView* hv) {
        auto header = qobject_cast<SpreadsheetGui::SheetViewHeader*>(hv);
        connect(
            header,
            &SpreadsheetGui::SheetViewHeader::cursorChanged,
            this,
            [this](const QCursor& newerCursor) { qpw->setCursor(newerCursor); }
        );
    };
    connectCursorChangedSignal(stv->horizontalHeader());
    connectCursorChangedSignal(stv->verticalHeader());
}

int ZoomableView::zoomLevel() const
{
    return m_zoomLevel;
}

void ZoomableView::setZoomLevel(int zoomLevel)
{
    zoomLevel = qBound(zoomMin, zoomLevel, zoomMax);

    if (m_zoomLevel == zoomLevel) {
        return;
    }

    m_zoomLevel = zoomLevel;
    updateView();
    Q_EMIT zoomLevelChanged(m_zoomLevel);
}

void ZoomableView::zoomIn(void)
{
    setZoomLevel(m_zoomLevel + zoom_step_kb);
}

void ZoomableView::zoomOut(void)
{
    setZoomLevel(m_zoomLevel - zoom_step_kb);
}

void ZoomableView::resetZoom(void)
{
    constexpr const char* path = "User parameter:BaseApp/Preferences/Mod/Spreadsheet";
    ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath(path);
    const int defaultZoomLevel = static_cast<int>(hGrp->GetInt("DefaultZoomLevel", 100));

    setZoomLevel(defaultZoomLevel);
}

void ZoomableView::updateView(void)
{
    /* QGraphicsView has hardcoded margins therefore we have to avoid fitInView
     * Find more information at https://bugreports.qt.io/browse/QTBUG-42331 */

    const qreal scale_factor = static_cast<qreal>(m_zoomLevel) / 100.0,
                new_w = static_cast<qreal>(viewport()->rect().width()) / scale_factor,
                new_h = static_cast<qreal>(viewport()->rect().height()) / scale_factor;

    const QRectF new_geometry {0.0, 0.0, new_w, new_h};

    const QRect old_geometry {stv->geometry()};
    stv->setGeometry(1, 1, old_geometry.width() - 1, old_geometry.height() - 1);

    resetTransform();
    qpw->setGeometry(new_geometry);
    setSceneRect(new_geometry);
    scale(scale_factor, scale_factor);
    centerOn(new_geometry.center());
}

void ZoomableView::focusOutEvent(QFocusEvent* event)
{
    Q_UNUSED(event);
}

void ZoomableView::keyPressEvent(QKeyEvent* event)
{
    if (event->modifiers() & Qt::ControlModifier) {
        switch (event->key()) {
            case Qt::Key_Plus:
                zoomIn();
                event->accept();
                break;
            case Qt::Key_Minus:
                zoomOut();
                event->accept();
                break;
            case Qt::Key_0:
                resetZoom();
                event->accept();
                break;
            default:
                QGraphicsView::keyPressEvent(event);
        }
    }
    else {
        QGraphicsView::keyPressEvent(event);
    }
}

void ZoomableView::resizeEvent(QResizeEvent* event)
{
    QGraphicsView::resizeEvent(event);
    updateView();
}

void ZoomableView::wheelEvent(QWheelEvent* event)
{
    if (event->modifiers() & Qt::ControlModifier) {
        const int y = event->angleDelta().y();
        setZoomLevel(m_zoomLevel + (y > 0 ? zoom_step_mwheel : -zoom_step_mwheel));
        event->accept();
    }
    else {
        QGraphicsView::wheelEvent(event);
    }
}
