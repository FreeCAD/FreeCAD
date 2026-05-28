// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2026 Max Wilfinger
// SPDX-FileNotice: Part of the FreeCAD project.

/******************************************************************************
 *                                                                            *
 *   FreeCAD is free software: you can redistribute it and/or modify          *
 *   it under the terms of the GNU Lesser General Public License as           *
 *   published by the Free Software Foundation, either version 2.1            *
 *   of the License, or (at your option) any later version.                   *
 *                                                                            *
 *   FreeCAD is distributed in the hope that it will be useful,               *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty              *
 *   of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.                  *
 *   See the GNU Lesser General Public License for more details.              *
 *                                                                            *
 *   You should have received a copy of the GNU Lesser General Public         *
 *   License along with FreeCAD. If not, see https://www.gnu.org/licenses     *
 *                                                                            *
 ******************************************************************************/

#include <algorithm>

#include <QAction>
#include <QMenu>
#include <QStatusBar>
#include <QWidget>

#include "StatusBarManager.h"
#include "MainWindow.h"

#include <App/Application.h>

using namespace Gui;

StatusBarManager* StatusBarManager::_instance = nullptr;

StatusBarManager* StatusBarManager::instance()
{
    if (!_instance) {
        _instance = new StatusBarManager();
    }
    return _instance;
}

void StatusBarManager::destruct()
{
    delete _instance;
    _instance = nullptr;
}

StatusBarManager::StatusBarManager()
    : hGrp(App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/MainWindow"))
{}

QStatusBar* StatusBarManager::statusBar() const
{
    auto* mw = getMainWindow();
    return mw ? mw->statusBar() : nullptr;
}

bool StatusBarManager::sortsBefore(const Entry& a, const Entry& b)
{
    if (a.spec.order != b.spec.order) {
        return a.spec.order < b.spec.order;
    }
    return a.sequence < b.sequence;
}

void StatusBarManager::place(QWidget* widget, const StatusBarItemSpec& spec)
{
    auto* sb = statusBar();
    if (!sb) {
        return;
    }

    Entry incoming {spec, widget, sequenceCounter};

    // Collect the already-registered widgets in the same slot that should sit to
    // the right of the new one, in left-to-right order.
    QList<Entry> rightOf;
    for (const Entry& e : items) {
        if (e.widget && e.spec.slot == spec.slot && sortsBefore(incoming, e)) {
            rightOf.append(e);
        }
    }
    std::sort(rightOf.begin(), rightOf.end(), &StatusBarManager::sortsBefore);

    // Detach them (preserving visibility), insert the new widget, then re-attach
    // so the new widget lands at its ordered position without touching widgets to
    // its left or any widget the manager does not own.
    QList<bool> wasVisible;
    wasVisible.reserve(rightOf.size());
    for (const Entry& e : rightOf) {
        wasVisible.append(e.widget->isVisible());
        sb->removeWidget(e.widget);
    }

    if (spec.slot == StatusBarSlot::Left) {
        sb->addWidget(widget, spec.stretch);
    }
    else {
        sb->addPermanentWidget(widget, spec.stretch);
    }

    for (int i = 0; i < rightOf.size(); ++i) {
        QWidget* w = rightOf[i].widget;
        if (rightOf[i].spec.slot == StatusBarSlot::Left) {
            sb->addWidget(w, rightOf[i].spec.stretch);
        }
        else {
            sb->addPermanentWidget(w, rightOf[i].spec.stretch);
        }
        w->setVisible(wasVisible[i]);
    }
}

void StatusBarManager::addItem(QWidget* widget, const StatusBarItemSpec& spec)
{
    if (!widget || spec.id.isEmpty() || !statusBar()) {
        return;
    }

    // Replace any previous registration with the same id.
    removeItem(spec.id);

    // Restore the persisted visibility before the widget becomes visible on the
    // bar.  Widgets that own their visibility (e.g. the progress bar) opt out.
    if (spec.persistentVisibility && !spec.ownVisibility) {
        widget->setVisible(hGrp->GetBool(spec.id.constData(), true));
    }

    place(widget, spec);
    items.append(Entry {spec, widget, sequenceCounter++});
}

void StatusBarManager::removeItem(const QByteArray& id)
{
    auto* sb = statusBar();
    for (int i = 0; i < items.size(); ++i) {
        if (items[i].spec.id == id) {
            if (sb && items[i].widget) {
                sb->removeWidget(items[i].widget);
            }
            items.removeAt(i);
            return;
        }
    }
}

void StatusBarManager::populateToggleMenu(QMenu& menu)
{
    QList<Entry> ordered;
    for (const Entry& e : items) {
        if (e.widget && !e.spec.title.isEmpty()) {
            ordered.append(e);
        }
    }
    // Read the bar left-to-right: Left slot before Right slot, each by order.
    std::sort(ordered.begin(), ordered.end(), [](const Entry& a, const Entry& b) {
        if (a.spec.slot != b.spec.slot) {
            return a.spec.slot == StatusBarSlot::Left;
        }
        return sortsBefore(a, b);
    });

    for (const Entry& e : ordered) {
        QWidget* widget = e.widget;
        QAction* action = menu.addAction(e.spec.title);
        action->setCheckable(true);

        if (e.spec.ownVisibility) {
            // The widget tracks its own user-facing enabled state (a transient
            // widget such as the progress bar is hidden while idle).
            action->setChecked(widget->property("userEnabled").toBool());
            QObject::connect(action, &QAction::toggled, widget, [widget](bool checked) {
                widget->setProperty("userEnabled", checked);
            });
        }
        else {
            action->setChecked(widget->isVisible());
            const QByteArray id = e.spec.id;
            const bool persist = e.spec.persistentVisibility;
            ParameterGrp::handle grp = hGrp;
            QPointer<QWidget> guard(widget);
            QObject::connect(action, &QAction::toggled, widget, [guard, id, persist, grp](bool checked) {
                if (!guard) {
                    return;
                }
                guard->setVisible(checked);
                if (persist) {
                    grp->SetBool(id.constData(), checked);
                }
            });
        }
    }
}
