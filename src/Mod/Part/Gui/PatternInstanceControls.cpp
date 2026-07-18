// SPDX-License-Identifier: LGPL-2.1-or-later

/****************************************************************************
 *   Copyright (c) 2026 Boyer Pierre-Louis <pierrelouis.boyer@gmail.com>    *
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

#include "PatternInstanceControls.h"

#include <Inventor/SbLinear.h>
#include <Inventor/nodes/SoCamera.h>
#include <Inventor/sensors/SoNodeSensor.h>

#include <QColor>
#include <QEvent>
#include <QIcon>
#include <QPainter>
#include <QPen>
#include <QPixmap>
#include <QPoint>
#include <QSize>
#include <QTimer>
#include <QToolButton>
#include <QWidget>

#include <Gui/View3DInventorViewer.h>

using namespace PartGui;

namespace
{

QIcon makeCrossIcon()
{
    QPixmap pixmap(18, 18);
    pixmap.fill(Qt::transparent);

    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setPen(QPen(QColor(208, 36, 36), 3, Qt::SolidLine, Qt::RoundCap));
    painter.drawLine(5, 5, 13, 13);
    painter.drawLine(13, 5, 5, 13);

    return QIcon(pixmap);
}

QIcon makePlusIcon()
{
    QPixmap pixmap(18, 18);
    pixmap.fill(Qt::transparent);

    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setPen(QPen(QColor(29, 132, 72), 3, Qt::SolidLine, Qt::RoundCap));
    painter.drawLine(9, 4, 9, 14);
    painter.drawLine(4, 9, 14, 9);

    return QIcon(pixmap);
}

const QIcon& suppressIcon()
{
    static const QIcon icon = makeCrossIcon();
    return icon;
}

const QIcon& restoreIcon()
{
    static const QIcon icon = makePlusIcon();
    return icon;
}

QString buttonStyleSheet()
{
    return QStringLiteral(
        "QToolButton {"
        "  background-color: rgba(255, 255, 255, 215);"
        "  border: 1px solid rgba(40, 40, 40, 120);"
        "  border-radius: 11px;"
        "  padding: 2px;"
        "}"
        "QToolButton:hover {"
        "  background-color: rgba(255, 255, 255, 245);"
        "}"
        "QToolButton:pressed {"
        "  background-color: rgba(232, 232, 232, 245);"
        "}"
    );
}

}  // namespace

PatternInstanceControls::PatternInstanceControls(Gui::View3DInventorViewer* viewer, QObject* parent)
    : QObject(parent)
{
    setViewer(viewer);
}

PatternInstanceControls::~PatternInstanceControls()
{
    detachCameraSensor();
    clear();
    if (hostWidget) {
        hostWidget->removeEventFilter(this);
    }
}

void PatternInstanceControls::setViewer(Gui::View3DInventorViewer* newViewer)
{
    if (viewer.data() == newViewer) {
        attachCameraSensor();
        refreshHostWidget();
        updatePositions();
        return;
    }

    if (viewer) {
        disconnect(viewer.data(), nullptr, this, nullptr);
    }
    detachCameraSensor();
    if (hostWidget) {
        hostWidget->removeEventFilter(this);
        hostWidget = nullptr;
    }

    viewer = newViewer;
    if (viewer) {
        connect(viewer.data(), &Gui::View3DInventorViewer::cameraChanged, this, [this]() {
            attachCameraSensor();
            updatePositions();
        });
    }
    attachCameraSensor();
    refreshHostWidget();
    updatePositions();
}

void PatternInstanceControls::setInstances(const std::vector<Instance>& instances)
{
    clear();
    refreshHostWidget();
    if (!viewer || !hostWidget) {
        return;
    }

    buttons.reserve(instances.size());
    for (const auto& instance : instances) {
        auto* button = new QToolButton(hostWidget);
        button->setAutoRaise(true);
        button->setCursor(Qt::ArrowCursor);
        button->setFixedSize(24, 24);
        button->setFocusPolicy(Qt::NoFocus);
        button->setIconSize(QSize(18, 18));
        button->setStyleSheet(buttonStyleSheet());

        ButtonInfo info;
        info.instance = instance;
        info.button = button;
        updateButton(info);

        connect(button, &QToolButton::clicked, this, [this, button]() {
            for (const auto& info : buttons) {
                if (info.button == button) {
                    Q_EMIT toggleRequested(info.instance.index, !info.instance.suppressed);
                    return;
                }
            }
        });

        buttons.push_back(info);
        button->show();
    }

    updatePositions();
    QTimer::singleShot(0, this, &PatternInstanceControls::updatePositions);
}

void PatternInstanceControls::clear()
{
    for (auto& info : buttons) {
        if (info.button) {
            info.button->hide();
            info.button->deleteLater();
        }
    }
    buttons.clear();
}

void PatternInstanceControls::updatePositions()
{
    refreshHostWidget();
    if (!viewer || !hostWidget) {
        return;
    }

    const QSize hostSize = hostWidget->size();
    for (auto& info : buttons) {
        if (!info.button) {
            continue;
        }

        if (hostSize.isEmpty()) {
            info.button->hide();
            continue;
        }

        const QSize buttonSize = info.button->size();
        QPoint pxCoord = viewer->toQPoint(viewer->getPointOnViewport(SbVec3f(
            static_cast<float>(info.instance.center.x),
            static_cast<float>(info.instance.center.y),
            static_cast<float>(info.instance.center.z)
        )));
        if (hostWidget.data() != viewer.data()) {
            pxCoord = viewer->mapTo(hostWidget.data(), pxCoord);
        }

        pxCoord.setX(pxCoord.x() - buttonSize.width() / 2);
        pxCoord.setY(pxCoord.y() - buttonSize.height() / 2);

        if (pxCoord.x() < 0 || pxCoord.y() < 0 || pxCoord.x() + buttonSize.width() > hostSize.width()
            || pxCoord.y() + buttonSize.height() > hostSize.height()) {
            info.button->hide();
            continue;
        }

        info.button->move(pxCoord);
        info.button->raise();
        info.button->show();
    }
}

bool PatternInstanceControls::eventFilter(QObject* watched, QEvent* event)
{
    if (watched == hostWidget.data()
        && (event->type() == QEvent::Move || event->type() == QEvent::Resize
            || event->type() == QEvent::Show)) {
        QTimer::singleShot(0, this, &PatternInstanceControls::updatePositions);
    }

    return QObject::eventFilter(watched, event);
}

void PatternInstanceControls::refreshHostWidget()
{
    QWidget* newHostWidget = viewer ? viewer->parentWidget() : nullptr;
    if (!newHostWidget && viewer) {
        newHostWidget = viewer.data();
    }

    if (hostWidget == newHostWidget) {
        return;
    }

    if (hostWidget) {
        hostWidget->removeEventFilter(this);
    }
    hostWidget = newHostWidget;
    if (hostWidget) {
        hostWidget->installEventFilter(this);
    }

    for (auto& info : buttons) {
        if (info.button) {
            info.button->setParent(hostWidget.data());
            info.button->setVisible(hostWidget.data() != nullptr);
        }
    }
}

void PatternInstanceControls::updateButton(ButtonInfo& info) const
{
    if (!info.button) {
        return;
    }

    if (info.instance.suppressed) {
        info.button->setIcon(restoreIcon());
        info.button->setToolTip(tr("Restore instance"));
        return;
    }

    info.button->setIcon(suppressIcon());
    info.button->setToolTip(tr("Suppress instance"));
}

void PatternInstanceControls::attachCameraSensor()
{
    if (!viewer || !viewer->getCamera()) {
        if (cameraSensor) {
            cameraSensor->detach();
        }
        return;
    }

    if (!cameraSensor) {
        cameraSensor = new SoNodeSensor(&PatternInstanceControls::cameraSensorCallback, this);
    }

    cameraSensor->detach();
    cameraSensor->attach(viewer->getCamera());
}

void PatternInstanceControls::detachCameraSensor()
{
    if (!cameraSensor) {
        return;
    }

    cameraSensor->detach();
    delete cameraSensor;
    cameraSensor = nullptr;
}

void PatternInstanceControls::cameraSensorCallback(void* data, SoSensor* /*sensor*/)
{
    auto* controls = static_cast<PatternInstanceControls*>(data);
    if (controls) {
        controls->updatePositions();
    }
}
