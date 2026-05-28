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

#pragma once

#include <QContextMenuEvent>
#include <QLabel>

#include <FCGlobal.h>

namespace Gui
{
/**
 * @brief Label for displaying information in the status bar
 *
 * A QLabel subclass whose right-click context menu lets the user toggle the
 * status-bar items. Visibility/persistence and the menu contents are owned by
 * MainWindow's status-bar registry; this class only forwards the request.
 */
class GuiExport StatusBarLabel: public QLabel
{
    Q_OBJECT
public:
    explicit StatusBarLabel(QWidget* parent = nullptr);

protected:
    void contextMenuEvent(QContextMenuEvent* event) override;
    void setVisible(bool visible) override;
};

}  // Namespace Gui
