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

#include "PreCompiled.h"
#ifndef _PreComp_
# include <QDragMoveEvent>
#endif

#include "ListWidgetDragBugFix.h"


ListWidgetDragBugFix::ListWidgetDragBugFix(QWidget * parent)
  : QListWidget(parent)
{
}

ListWidgetDragBugFix::~ListWidgetDragBugFix()
{
}

void ListWidgetDragBugFix::dragMoveEvent(QDragMoveEvent *e)
{
    if ((row(itemAt(e->pos())) == currentRow() + 1)
        || (currentRow() == count() - 1 && row(itemAt(e->pos())) == -1)) {
        e->ignore();
        return;
    }
    QListWidget::dragMoveEvent(e);
}

#include "moc_ListWidgetDragBugFix.cpp"
