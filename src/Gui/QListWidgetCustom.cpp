/***************************************************************************
 *   Copyright (c) 2015 FreeCAD Developers                                 *
 *   Author: Przemo Firszt <przemo@firszt.eu>                              *
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
# include <QString>
#endif

#include "QListWidgetCustom.h"


QListWidgetCustom::QListWidgetCustom(QWidget * parent)
  : QListWidget(parent)
{
}

QListWidgetCustom::~QListWidgetCustom() = default;

/* Overridden dragMoveEvent prevents dragging items that originated
 * from the same list for "disabled workbenches". Dragging from outside
 * is still allowed. Also it blocks dragging from another instance of FreeCAD
 */
void QListWidgetCustom::dragMoveEvent(QDragMoveEvent *e)
{
    if (e->source()) {
        QVariant prop = this->property("OnlyAcceptFrom");
        if (prop.isValid()) {
            QStringList filter = prop.toStringList();
            QString sender = e->source()->objectName();
            if (!filter.contains(sender)) {
                e->ignore();
            } else {
                e->accept();
            }
        } else {
            e->accept();
        }
    } else {
        e->ignore();
    }
}

#include "moc_QListWidgetCustom.cpp"
