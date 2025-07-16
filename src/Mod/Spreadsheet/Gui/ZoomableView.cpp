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
    : QGraphicsView {}
    , stv {ui->cells}
{
    if (!stv) {
        Base::Console().developerWarning("ZoomableView", "Failed to find a SheetTableView object");
        deleteLater();
        return;
    }
    else {
        QLayoutItem* li_stv = stv->parentWidget()->layout()->replaceWidget(stv, this);
        if (li_stv == nullptr) {
            Base::Console().developerWarning("ZoomableView",
                                             "Failed to replace the SheetTableView object");
            deleteLater();
            return;
        }
        delete li_stv;
    }

    stv->setParent(nullptr);
    qpw = m_scene.addWidget(stv);
    setScene(&m_scene);

    setBackgroundBrush(Qt::transparent);
    setFrameStyle(QFrame::NoFrame);

    setSizePolicy(QSizePolicy {QSizePolicy::Expanding, QSizePolicy::Expanding});

    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    stv->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    stv->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    QPointer<QScrollBar> dummySB_h {stv->horizontalScrollBar()},
        dummySB_v {stv->verticalScrollBar()}, realSB_h {ui->realSB_h}, realSB_v {ui->realSB_v};

    if (!dummySB_h || !dummySB_v || !realSB_h || !realSB_v) {
        Base::Console().developerWarning("ZoomableView", "Failed to identify the scrollbars");
        deleteLater();
        return;
    }

    realSB_h->setRange(dummySB_h->minimum(), dummySB_h->maximum());
    realSB_v->setRange(dummySB_v->minimum(), dummySB_v->maximum());

    realSB_h->setPageStep(dummySB_h->pageStep());
    realSB_v->setPageStep(dummySB_v->pageStep());


    connect(realSB_h, &QAbstractSlider::valueChanged, dummySB_h, &QAbstractSlider::setValue);
    connect(realSB_v, &QAbstractSlider::valueChanged, dummySB_v, &QAbstractSlider::setValue);

    connect(dummySB_h, &QAbstractSlider::rangeChanged, realSB_h, &QAbstractSlider::setRange);
    connect(dummySB_v, &QAbstractSlider::rangeChanged, realSB_v, &QAbstractSlider::setRange);

    connect(dummySB_h, &QAbstractSlider::valueChanged, this, &ZoomableView::updateView);
    connect(dummySB_v, &QAbstractSlider::valueChanged, this, &ZoomableView::updateView);

    connect(this,
            &ZoomableView::zoomLevelChanged,
            ui->zoomTB,
            [zoomTB = ui->zoomTB](int new_zoomLevel) {
                zoomTB->setText(QStringLiteral("%1%").arg(new_zoomLevel));
            });

    connect(this,
            &ZoomableView::zoomLevelChanged,
            ui->zoomSlider,
            [zoomSlider = ui->zoomSlider](int new_zoomLevel) {
                zoomSlider->blockSignals(true);
                zoomSlider->setValue(new_zoomLevel);
                zoomSlider->blockSignals(false);
            });

    connect(ui->zoomPlus, &QToolButton::clicked, this, &ZoomableView::zoomIn);
    connect(ui->zoomSlider, &QSlider::valueChanged, this, &ZoomableView::setZoomLevel);
    connect(ui->zoomMinus, &QToolButton::clicked, this, &ZoomableView::zoomOut);

    connect(ui->zoomTB, &QToolButton::clicked, ui->zoomSlider, [zoomSlider = ui->zoomSlider]() {
        const QString title = tr("Zoom Level"), label = tr("New zoom level:");
        constexpr int min = ZoomableView::min, max = ZoomableView::max, step = 10;
        const int val = zoomSlider->value();
        bool ok;
        const int new_val =
            QInputDialog::getInt(zoomSlider, title, label, val, min, max, step, &ok);

        if (ok) {
            zoomSlider->setValue(new_val);
        }
    });

    resetZoom();

    auto connectCursorChangedSignal = [this](QHeaderView* hv) {
        auto header = qobject_cast<SpreadsheetGui::SheetViewHeader*>(hv);
        connect(header,
                &SpreadsheetGui::SheetViewHeader::cursorChanged,
                this,
                [this](const QCursor& newerCursor) {
                    qpw->setCursor(newerCursor);
                });
    };
    connectCursorChangedSignal(stv->horizontalHeader());
    connectCursorChangedSignal(stv->verticalHeader());
}

int ZoomableView::zoomLevel() const
{
    return m_zoomLevel;
}

void ZoomableView::setZoomLevel(int new_zoomLevel)
{
    checkLimits(new_zoomLevel);

    if (m_zoomLevel == new_zoomLevel) {
        return;
    }

    m_zoomLevel = new_zoomLevel;
    updateView();
    Q_EMIT zoomLevelChanged(m_zoomLevel);
}

inline void ZoomableView::checkLimits(int& zoom_level)
{
    zoom_level = qBound(ZoomableView::min, zoom_level, ZoomableView::max);
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
