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

#include <Mod/TechDraw/App/DrawTemplate.h>

#include "QGITemplate.h"
#include "QGSPage.h"
#include "ZVALUE.h"


using namespace TechDrawGui;

QGITemplate::QGITemplate(QGSPage *scene) : QGraphicsItemGroup(),
    pageTemplate(nullptr)
{
    setHandlesChildEvents(false);
    setCacheMode(QGraphicsItem::NoCache);
    setZValue(ZVALUE::TEMPLATE); //Template is situated in background

    scene->addItem(this);
}

QGITemplate::~QGITemplate()
{
    pageTemplate = nullptr;
}

void QGITemplate::setTemplate(TechDraw::DrawTemplate *obj)
{
    if (!obj)
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
