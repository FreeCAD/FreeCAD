/***************************************************************************
 *   Copyright (c) 2012-2013 Luke Parry <l.parry@warwick.ac.uk>            *
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
# include <QAction>
# include <QContextMenuEvent>
# include <QGraphicsScene>
# include <QGraphicsSceneMouseEvent>
# include <QMenu>
# include <QMessageBox>
# include <QMouseEvent>
# include <QPainter>
#endif

#include <App/Document.h>
#include <Base/Console.h>
#include <Gui/Selection.h>
#include <Gui/Command.h>

#include <Mod/TechDraw/App/DrawViewCollection.h>
#include "QGIViewCollection.h"

using namespace TechDrawGui;

QGIViewCollection::QGIViewCollection()
{
    setFlags(QGraphicsItem::ItemIsSelectable);

    setHandlesChildEvents(false);

    //setCacheMode(QGraphicsItem::NoCache);
    setAcceptHoverEvents(true);
    setFlag(QGraphicsItem::ItemIsMovable, true);
}


QVariant QGIViewCollection::itemChange(GraphicsItemChange change, const QVariant &value)
{

    return QGIView::itemChange(change, value);
}

void QGIViewCollection::mouseReleaseEvent(QGraphicsSceneMouseEvent * event)
{
    //TODO: should MouseMove logic go here instead of QGIView?
    QGIView::mouseReleaseEvent(event);
}

void QGIViewCollection::updateView(bool update)
{
    return QGIView::updateView(update);
}
