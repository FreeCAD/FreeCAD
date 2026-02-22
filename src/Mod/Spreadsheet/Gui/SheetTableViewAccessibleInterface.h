// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2023 Adrian Popescu                                     *
 *   <adrian-constantin.popescu@outlook.com>                               *
 *                                                                         *
 *   This file is part of the FreeCAD CAx development system.              *
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

#pragma once

#include <Mod/Spreadsheet/Gui/SheetTableView.h>
#include <QtWidgets/qaccessiblewidget.h>

namespace SpreadsheetGui
{

// Currently SheetTableViewAccessibleInterface below deactivates the
// built-in QAccessibleTable interface, and all the accessibility
// features.
//
// For a proper implementation, start by extending that
// and ensure you're not queue-ing empty cells, or counting empty cells
//
// Otherwise it will hang - https://github.com/FreeCAD/FreeCAD/issues/8265

class SheetTableViewAccessibleInterface: public QAccessibleWidget
{
public:
    explicit SheetTableViewAccessibleInterface(SpreadsheetGui::SheetTableView* view);

    QString text(QAccessible::Text txt) const override;

    QAccessibleInterface* childAt(int x, int y) const override;
    int indexOfChild(const QAccessibleInterface*) const override;
    int childCount() const override;
    QAccessibleInterface* focusChild() const override;
    QAccessibleInterface* child(int index) const override;

    static QAccessibleInterface* ifactory(const QString& key, QObject* obj);
};
}  // namespace SpreadsheetGui
