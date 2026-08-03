// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2004 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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

#include <QMenu>

namespace Gui
{
namespace Dialog
{

/** This class implements the undo dialog.
 * \author Werner Mayer
 */
class UndoDialog: public QMenu
{
    Q_OBJECT

public:
    explicit UndoDialog(QWidget* parent = nullptr);
    ~UndoDialog() override;

protected Q_SLOTS:
    void onSelected();
    void onFetchInfo();
};

/** This class implements the redo dialog.
 * \author Werner Mayer
 */
class RedoDialog: public QMenu
{
    Q_OBJECT

public:
    explicit RedoDialog(QWidget* parent = nullptr);
    ~RedoDialog() override;

protected Q_SLOTS:
    void onSelected();
    void onFetchInfo();
};

}  // namespace Dialog
}  // namespace Gui
