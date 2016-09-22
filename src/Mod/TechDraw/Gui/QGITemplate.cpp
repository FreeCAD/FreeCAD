/***************************************************************************
 *   Copyright (c) 2012-2014 Luke Parry <l.parry@warwick.ac.uk>            *
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
# include <QGraphicsScene>
#endif

#include <Base/Console.h>
#include <Base/Exception.h>

#include <Mod/TechDraw/App/DrawTemplate.h>

#include "ZVALUE.h"
#include "TemplateTextField.h"
#include "QGITemplate.h"

using namespace TechDrawGui;

QGITemplate::QGITemplate(QGraphicsScene *scene) : QGraphicsItemGroup(),
    pageTemplate(0)
{
    setHandlesChildEvents(false);
    setCacheMode(QGraphicsItem::NoCache);
    setZValue(ZVALUE::TEMPLATE); //Template is situated in background

    scene->addItem(this);
}

QGITemplate::~QGITemplate()
{
    pageTemplate = 0;
}

QVariant QGITemplate::itemChange(GraphicsItemChange change, const QVariant &value)
{
    return QGraphicsItemGroup::itemChange(change, value);
}

void QGITemplate::setTemplate(TechDraw::DrawTemplate *obj)
{
    if(obj == 0)
        return;

    pageTemplate = obj;
}

void QGITemplate::clearContents()
{

}

void QGITemplate::updateView(bool update)
{
    Q_UNUSED(update);
    draw();
}

#include <Mod/TechDraw/Gui/moc_QGITemplate.cpp>
