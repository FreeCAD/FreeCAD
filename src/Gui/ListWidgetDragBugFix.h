/***************************************************************************
 *   Copyright (c) 2023 Boyer Pierre-louis <pierrelouis.boyer@gmail.com>   *
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

#ifndef QLISTWIDGETDRAGBUGFIX_HPP
#define QLISTWIDGETDRAGBUGFIX_HPP

#include <QDragMoveEvent>
#include <QListWidget>

 /* Qt has a recent bug (2023, https://bugreports.qt.io/browse/QTBUG-100128)
 * where the items disappears in certain conditions when drag and dropping.
 * Here we prevent the situation where this happens.
 * 1 - If the item is dropped on the item below such that the item doesn't move (ie superior half of the below item)
 * 2 - The item is the last one and user drop it on the empty space below.
 * In both those cases the item widget was lost.
 * When Qt solve this bug, this class should not be required anymore.
  */
class ListWidgetDragBugFix : public QListWidget
{
    Q_OBJECT

public:
    explicit ListWidgetDragBugFix(QWidget *parent);
    ~ListWidgetDragBugFix() override;

protected:
    void dragMoveEvent(QDragMoveEvent *e) override;
};

#endif
