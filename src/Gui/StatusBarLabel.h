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

#include <string>

#include <QAction>
#include <QApplication>
#include <QClipboard>
#include <QContextMenuEvent>
#include <QLabel>
#include <QStatusBar>
#include <QMenu>

#include <FCGlobal.h>

#include <Base/Parameter.h>

namespace Gui
{
/**
 * @brief Label for displaying information in the status bar
 *
 * A QLabel subclass that provides a context menu with additional actions
 * similar to the standard status bar widgets.
 */
class GuiExport StatusBarLabel: public QLabel
{
    Q_OBJECT
public:
    explicit StatusBarLabel(QWidget* parent, const std::string& parameterName = {});

protected:
    void contextMenuEvent(QContextMenuEvent* event) override;
    void setVisible(bool visible) override;

private:
    ParameterGrp::handle hGrp;
    std::string parameterName;
};

}  // Namespace Gui
