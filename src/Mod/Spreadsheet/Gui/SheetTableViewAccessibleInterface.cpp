// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileNotice: Part of the FreeCAD project.

/***************************************************************************
 *   Copyright (c) 2023 Adrian Popescu                                     *
 *   <adrian-constantin.popescu@outlook.com>                               *
 *                                                                         *
 *   This library is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU Library General Public           *
 *   License as published by the Free Software Foundation; either          *
 *   version 2 of the License, or (at your option) any later version.      *
 *                                                                         *
 *   This library  is distributed in the hope that it will be useful,      *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU Library General Public License for more details.                  *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this library; see the file COPYING.LIB. If not,    *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,         *
 *   Suite 330, Boston, MA  02111-1307, USA                                *
 *                                                                         *
 ***************************************************************************/


#include <QAction>
#include <QApplication>
#include <QClipboard>
#include <QKeyEvent>
#include <QMenu>
#include <QMessageBox>
#include <QMimeData>


#include "SheetTableViewAccessibleInterface.h"

namespace SpreadsheetGui
{

SheetTableViewAccessibleInterface::SheetTableViewAccessibleInterface(
    SpreadsheetGui::SheetTableView* view
)
    : QAccessibleWidget(view)
{}

QString SheetTableViewAccessibleInterface::text(QAccessible::Text txt) const
{
    if (txt == QAccessible::Help) {
        return QStringLiteral("Implement me");
    }
    return QAccessibleWidget::text(txt);
}

QAccessibleInterface* SheetTableViewAccessibleInterface::childAt(int x, int y) const
{
    Q_UNUSED(x)
    Q_UNUSED(y)
    return (QAccessibleInterface*)this;
}

int SheetTableViewAccessibleInterface::indexOfChild(const QAccessibleInterface*) const
{
    return 0;
}

int SheetTableViewAccessibleInterface::childCount() const
{
    return 0;
}

QAccessibleInterface* SheetTableViewAccessibleInterface::focusChild() const
{
    return (QAccessibleInterface*)this;
}

QAccessibleInterface* SheetTableViewAccessibleInterface::child(int index) const
{
    Q_UNUSED(index)
    return (QAccessibleInterface*)this;
}

QAccessibleInterface* SheetTableViewAccessibleInterface::ifactory(const QString& key, QObject* obj)
{
    if (key == QStringLiteral("SpreadsheetGui::SheetTableView")) {
        return new SheetTableViewAccessibleInterface(static_cast<SpreadsheetGui::SheetTableView*>(obj));
    }
    return nullptr;
}
}  // namespace SpreadsheetGui
