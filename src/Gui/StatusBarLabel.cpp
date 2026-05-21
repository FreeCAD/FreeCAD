// SPDX-License-Identifier: LGPL-2.1-or-later
/****************************************************************************
 *                                                                          *
 *   Copyright (c) 2025 Benjamin Nauck <benjamin@nauck.se>                  *
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

#include <QMenu>
#include <QClipboard>
#include <QApplication>
#include <QHash>
#include <QSet>
#include <QStatusBar>
#include <QAction>
#include <QContextMenuEvent>
#include <QHideEvent>
#include <QPointer>
#include <QTimer>

#include <utility>

#include "StatusBarLabel.h"
#include <App/Application.h>

namespace Gui
{

StatusBarLabel::StatusBarLabel(QWidget* parent, const std::string& parameterName)
    : QLabel(parent)
{
    if (!parameterName.empty()) {
        hGrp = App::GetApplication().GetParameterGroupByPath(
            "User parameter:BaseApp/Preferences/MainWindow"
        );

        // set visibility before storing parameterName to avoid saving it immediately
        setVisible(hGrp->GetBool(parameterName.c_str(), true));

        // now we can store parameterName
        this->parameterName = parameterName;
    }
}

static void addToggleAction(QMenu& menu, QWidget* widget)
{
    QAction* action = menu.addAction(widget->windowTitle());
    action->setCheckable(true);

    // Widgets such as the progress bar manage their own show/hide lifecycle and
    // expose a userEnabled Q_PROPERTY so the checked state reflects the user's
    // preference rather than the transient isVisible() state.
    QVariant userEnabled = widget->property("userEnabled");
    if (userEnabled.isValid()) {
        action->setChecked(userEnabled.toBool());
        QObject::connect(action, &QAction::toggled, widget, [widget](bool checked) {
            widget->setProperty("userEnabled", checked);
        });
    }
    else {
        action->setChecked(widget->isVisible());
        QObject::connect(action, &QAction::toggled, widget, &QWidget::setVisible);
    }
}

// static
void StatusBarLabel::buildToggleMenu(QMenu& menu, QStatusBar* statusBar)
{
    // Explicit display order for known widgets.  Workbench-added widgets (Draft,
    // BIM, etc.) keep their position in the actual status-bar layout: each one is
    // emitted just before the first known widget that follows it on the bar.
    static const QStringList order = {
        QStringLiteral("actionLabel"),
        QStringLiteral("hintLabel"),
        QStringLiteral("progressBar"),
        QStringLiteral("rightSideLabel"),
        QStringLiteral("toggleBottomPanelsButton"),
        QStringLiteral("notificationArea"),
        QStringLiteral("NavigationIndicator"),
        QStringLiteral("sizeLabel"),
    };

    // Collect titled widgets in actual status-bar layout order.  QStatusBar
    // doesn't expose a QLayout, so use widget x-position to get visual order.
    QList<QWidget*> layoutOrder;
    for (QObject* child : statusBar->children()) {
        auto* widget = qobject_cast<QWidget*>(child);
        if (widget && !widget->windowTitle().isEmpty()) {
            layoutOrder.append(widget);
        }
    }
    std::stable_sort(layoutOrder.begin(), layoutOrder.end(), [](QWidget* a, QWidget* b) {
        return a->x() < b->x();
    });

    auto isKnown = [&](QWidget* w) {
        return order.contains(w->objectName());
    };

    QHash<QString, QWidget*> byName;
    for (QWidget* w : layoutOrder) {
        byName.insert(w->objectName(), w);
    }

    QSet<QWidget*> emitted;
    for (const QString& name : order) {
        QWidget* known = byName.value(name);
        if (!known) {
            continue;
        }
        const int idx = layoutOrder.indexOf(known);
        for (int i = 0; i < idx; ++i) {
            QWidget* w = layoutOrder[i];
            if (!isKnown(w) && !emitted.contains(w)) {
                addToggleAction(menu, w);
                emitted.insert(w);
            }
        }
        addToggleAction(menu, known);
        emitted.insert(known);
    }

    // Unknown widgets that sit after every known one (or when no layout was found).
    for (QWidget* w : layoutOrder) {
        if (!emitted.contains(w)) {
            addToggleAction(menu, w);
        }
    }
}

void StatusBarLabel::contextMenuEvent(QContextMenuEvent* event)
{
    event->accept();

    const QPoint globalPos = event->globalPos();
    QPointer<StatusBarLabel> self(this);
    QTimer::singleShot(0, this, [self, globalPos]() {
        if (!self) {
            return;
        }
        QMenu menu(self);
        if (auto* statusBar = qobject_cast<QStatusBar*>(self->parentWidget())) {
            buildToggleMenu(menu, statusBar);
        }
        if (self->textInteractionFlags() & Qt::TextSelectableByMouse) {
            menu.addSeparator();
            menu.addAction(tr("Copy"), [self]() {
                if (self) {
                    QApplication::clipboard()->setText(self->selectedText());
                }
            });
            menu.addAction(tr("Select All"), [self]() {
                if (self) {
                    self->setSelection(0, self->text().length());
                }
            });
        }
        menu.exec(globalPos);
    });
}

void StatusBarLabel::setVisible(bool visible)
{
    if (!parameterName.empty() && hGrp) {
        hGrp->SetBool(parameterName.c_str(), visible);
    }
    if (!visible) {
        clear();  // Clear text
    }
    QLabel::setVisible(visible);
}

}  // namespace Gui
