// SPDX-License-Identifier: LGPL-2.1-or-later

/******************************************************************************
 *                                                                            *
 *   FreeCAD is free software: you can redistribute it and/or modify          *
 *   it under the terms of the GNU Lesser General Public License as           *
 *   published by the Free Software Foundation, either version 2.1 of the     *
 *   License, or (at your option) any later version.                          *
 *                                                                            *
 *   FreeCAD is distributed in the hope that it will be useful, but           *
 *   WITHOUT ANY WARRANTY; without even the implied warranty of               *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the            *
 *   GNU Lesser General Public License for more details.                      *
 *                                                                            *
 *   You should have received a copy of the GNU Lesser General Public         *
 *   License along with FreeCAD.  If not, see                                *
 *   <https://www.gnu.org/licenses/>.                                         *
 *                                                                            *
 ******************************************************************************/

#include <QAbstractScrollArea>
#include <QDockWidget>
#include <QEvent>
#include <QHBoxLayout>
#include <QScrollBar>
#include <QSplitter>
#include <QTimer>

#include "LeftScrollBarController.h"

using namespace Gui;

namespace
{

// The overlay panels hide the native bar of the tree and property view (see the "Hide scroll bar"
// options) with a style sheet. Respect that instead of adding a bar of our own.
bool isHiddenByOverlay(QAbstractScrollArea* area)
{
    return area->verticalScrollBar()->styleSheet().contains(QStringLiteral("width:0"));
}

// Scroll areas embedded in another scroll area (e.g. item editors) are left alone
bool isNested(QAbstractScrollArea* area)
{
    for (QWidget* w = area->parentWidget(); w; w = w->parentWidget()) {
        if (qobject_cast<QAbstractScrollArea*>(w)) {
            return true;
        }
    }
    return false;
}

QScrollBar* barOf(QAbstractScrollArea* area)
{
    return qobject_cast<QScrollBar*>(qvariant_cast<QObject*>(area->property("LeftScrollBar")));
}

void sync(QAbstractScrollArea* area, QScrollBar* bar)
{
    QScrollBar* source = area->verticalScrollBar();
    {
        QSignalBlocker blocker(bar);
        bar->setRange(source->minimum(), source->maximum());
        bar->setPageStep(source->pageStep());
        bar->setSingleStep(source->singleStep());
        bar->setValue(source->value());
    }
    bar->setVisible(bar->property("LeftScrollBarOn").toBool() && source->maximum() > source->minimum());
}

void apply(QAbstractScrollArea* area, QScrollBar* bar, bool left)
{
    bool wasOn = bar->property("LeftScrollBarOn").toBool();
    if (left && !wasOn) {
        area->setProperty("LeftScrollBarPolicy", static_cast<int>(area->verticalScrollBarPolicy()));
        area->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    }
    else if (!left && wasOn) {
        area->setVerticalScrollBarPolicy(
            static_cast<Qt::ScrollBarPolicy>(area->property("LeftScrollBarPolicy").toInt())
        );
    }
    bar->setProperty("LeftScrollBarOn", left);
    sync(area, bar);
}

}  // namespace

LeftScrollBarController::LeftScrollBarController(QDockWidget* dock, Predicate wantLeft)
    : QObject(dock)
    , dock(dock)
    , wantLeft(std::move(wantLeft))
{
    dock->installEventFilter(this);
    connect(dock, &QDockWidget::dockLocationChanged, this, [this]() { schedule(); });
}

void LeftScrollBarController::update()
{
    QWidget* content = dock->widget();
    if (!content) {
        return;
    }
    bool left = wantLeft();
    QList<QAbstractScrollArea*> areas = content->findChildren<QAbstractScrollArea*>();
    if (auto self = qobject_cast<QAbstractScrollArea*>(content)) {
        areas.prepend(self);
    }
    for (auto area : areas) {
        if (isNested(area)) {
            continue;
        }
        // watch for the overlay panels hiding or showing the native bar
        area->verticalScrollBar()->installEventFilter(this);
        bool areaLeft = left && !isHiddenByOverlay(area);
        QScrollBar* bar = barOf(area);
        if (!bar && areaLeft) {
            bar = wrap(area);
        }
        if (bar) {
            apply(area, bar, areaLeft);
        }
    }
}

bool LeftScrollBarController::eventFilter(QObject* obj, QEvent* ev)
{
    if (obj == dock) {
        if (ev->type() == QEvent::Show || ev->type() == QEvent::ParentChange) {
            schedule();
        }
    }
    else if (qobject_cast<QScrollBar*>(obj)) {
        if (ev->type() == QEvent::StyleChange) {
            schedule();
        }
    }
    else if (ev->type() == QEvent::Resize) {
        auto area = qobject_cast<QAbstractScrollArea*>(static_cast<QWidget*>(obj)->parentWidget());
        if (area && barOf(area)) {
            sync(area, barOf(area));
        }
    }
    return false;
}

void LeftScrollBarController::schedule()
{
    QTimer::singleShot(0, this, [this]() { update(); });
}

QScrollBar* LeftScrollBarController::wrap(QAbstractScrollArea* area)
{
    QWidget* parent = area->parentWidget();
    auto splitter = qobject_cast<QSplitter*>(parent);
    if (!parent || (!parent->layout() && !splitter)) {
        return nullptr;
    }

    auto container = new QWidget;
    auto layout = new QHBoxLayout(container);
    layout->setDirection(QBoxLayout::LeftToRight);
    layout->setSpacing(0);
    layout->setContentsMargins(0, 0, 0, 0);
    auto bar = new QScrollBar(Qt::Vertical, container);
    bar->hide();

    bool hidden = area->isHidden();
    bool replaced = false;
    if (splitter) {
        int index = splitter->indexOf(area);
        if (index >= 0) {
            splitter->replaceWidget(index, container);
            replaced = true;
        }
    }
    else {
        replaced = parent->layout()->replaceWidget(area, container, Qt::FindChildrenRecursively)
            != nullptr;
    }
    if (!replaced) {
        delete container;
        return nullptr;
    }

    layout->addWidget(bar);
    layout->addWidget(area);
    area->setHidden(hidden);
    container->setSizePolicy(area->sizePolicy());

    area->setProperty("LeftScrollBar", QVariant::fromValue<QObject*>(bar));

    QScrollBar* source = area->verticalScrollBar();
    connect(source, &QScrollBar::rangeChanged, bar, [area, bar]() { sync(area, bar); });
    connect(source, &QScrollBar::valueChanged, bar, &QScrollBar::setValue);
    connect(bar, &QScrollBar::valueChanged, source, &QScrollBar::setValue);
    area->viewport()->installEventFilter(this);
    return bar;
}

#include "moc_LeftScrollBarController.cpp"
