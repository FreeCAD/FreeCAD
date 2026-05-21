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
#include <QStatusBar>
#include <QAction>
#include <QContextMenuEvent>
#include <QHideEvent>

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
    // Desired display order in the context menu, identified by object name.
    // Any titled widget not listed here is appended after these entries.
    static const QStringList order = {
        QStringLiteral("actionLabel"),
        QStringLiteral("hintLabel"),
        QStringLiteral("rightSideLabel"),
        QStringLiteral("progressBar"),
        QStringLiteral("toggleBottomPanelsButton"),
        QStringLiteral("notificationArea"),
        QStringLiteral("NavigationIndicator"),
        QStringLiteral("sizeLabel"),
    };

    // Collect all titled children keyed by object name for O(1) lookup.
    QHash<QString, QWidget*> byName;
    for (QObject* child : statusBar->children()) {
        auto* widget = qobject_cast<QWidget*>(child);
        if (widget && !widget->windowTitle().isEmpty()) {
            byName.insert(widget->objectName(), widget);
        }
    }

    // Emit entries in the declared order first.
    for (const QString& name : order) {
        auto it = byName.find(name);
        if (it != byName.end()) {
            addToggleAction(menu, it.value());
            byName.erase(it);
        }
    }

    // Append any remaining titled widgets not covered by the order list.
    for (auto* widget : std::as_const(byName)) {
        addToggleAction(menu, widget);
    }
}

void StatusBarLabel::contextMenuEvent(QContextMenuEvent* event)
{
    QMenu menu(this);

    if (auto* statusBar = qobject_cast<QStatusBar*>(parentWidget())) {
        buildToggleMenu(menu, statusBar);
    }

    if (textInteractionFlags() & Qt::TextSelectableByMouse) {
        menu.addSeparator();  // ----------

        // Copy + Select All
        menu.addAction(tr("Copy"), [this]() {
            QApplication::clipboard()->setText(this->selectedText());
        });
        menu.addAction(tr("Select All"), [this]() { this->setSelection(0, this->text().length()); });
    }

    menu.exec(event->globalPos());
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
