/***************************************************************************
 *   Copyright (c) 2016 wandererfan <WandererFan@gmail.com>                *
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
#include <cmath>
#include <QGraphicsItem>
#include <QGraphicsScene>
#include <QGraphicsSceneHoverEvent>
#include <QMenu>
#include <QMouseEvent>
#include <QString>
#include <sstream>
#endif

#include <qmath.h>

#include <App/Application.h>
#include <App/Material.h>
#include <Base/Console.h>
#include <Base/Parameter.h>

#include "../App/DrawView.h"
#include "../App/DrawViewSpreadsheet.h"
#include "QGIViewSpreadsheet.h"

using namespace TechDrawGui;

QGIViewSpreadsheet::QGIViewSpreadsheet() : QGIViewSymbol(QPoint(), nullptr)
{
    setHandlesChildEvents(false);
    setCacheMode(QGraphicsItem::NoCache);
    setAcceptHoverEvents(true);
    setFlag(QGraphicsItem::ItemIsMovable, true);
}

QGIViewSpreadsheet::~QGIViewSpreadsheet()
{
}

void QGIViewSpreadsheet::setViewFeature(TechDraw::DrawViewSpreadsheet *obj)
{
    // called from QGVPage. (once)
    QGIView::setViewFeature(static_cast<TechDraw::DrawView *>(obj));
}


#include "moc_QGIViewSpreadsheet.cpp"
