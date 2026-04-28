// SPDX-License-Identifier: LGPL-2.1-or-later

/****************************************************************************
 *   Copyright (c) 2023 Boyer Pierre-louis <pierrelouis.boyer@gmail.com>    *
 *   Copyright (c) 2023 FreeCAD Project Association                         *
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

#include <QDragMoveEvent>
#include <QListWidget>

/* Qt has a recent bug (2023, https://bugreports.qt.io/browse/QTBUG-100128)
 * where the items disappears in certain conditions when drag and dropping.
 * Here we prevent the situation where this happens.
 * 1 - If the item is dropped on the item below such that the item doesn't move (ie superior half of
 * the below item) 2 - The item is the last one and user drop it on the empty space below. In both
 * those cases the item widget was lost. When Qt solve this bug, this class should not be required
 * anymore.
 */
class ListWidgetDragBugFix: public QListWidget
{
    Q_OBJECT

public:
    explicit ListWidgetDragBugFix(QWidget* parent);
    ~ListWidgetDragBugFix() override;

protected:
    void dragMoveEvent(QDragMoveEvent* e) override;
};
