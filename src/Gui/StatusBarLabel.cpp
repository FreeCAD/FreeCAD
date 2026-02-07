// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileNotice: Part of the FreeCAD project.

/****************************************************************************
 *                                                                          *
 *   Copyright (c) 2025 Benjamin Nauck <benjamin@nauck.se>                  *
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
#include <QStatusBar>
#include <QAction>
#include <QContextMenuEvent>
#include <QHideEvent>

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

void StatusBarLabel::contextMenuEvent(QContextMenuEvent* event)
{
    QMenu menu(this);

    // Reproduce standard status bar widget menu
    if (auto* statusBar = qobject_cast<QStatusBar*>(parentWidget())) {
        for (QObject* child : statusBar->children()) {
            QWidget* widget = qobject_cast<QWidget*>(child);
            if (!widget) {
                continue;
            }
            auto title = widget->windowTitle();
            if (title.isEmpty()) {
                continue;
            }

            QAction* action = menu.addAction(title);
            action->setCheckable(true);
            action->setChecked(widget->isVisible());
            QObject::connect(action, &QAction::toggled, widget, &QWidget::setVisible);
        }
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
