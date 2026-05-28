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
#include <QContextMenuEvent>
#include <QPointer>
#include <QTimer>

#include "StatusBarLabel.h"
#include "MainWindow.h"

namespace Gui
{

StatusBarLabel::StatusBarLabel(QWidget* parent)
    : QLabel(parent)
{}

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
        if (auto* mw = getMainWindow()) {
            mw->buildStatusBarContextMenu(menu);
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
    if (!visible) {
        clear();  // Drop stale text while hidden
    }
    QLabel::setVisible(visible);
}

}  // namespace Gui
