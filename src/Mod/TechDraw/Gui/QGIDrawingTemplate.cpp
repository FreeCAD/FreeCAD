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
#include <Base/Exception.h>
#include <Base/Tools2D.h>
#include <Gui/Selection.h>
#include <Gui/Command.h>

#include <Mod/TechDraw/App/Geometry.h>
#include <Mod/TechDraw/App/DrawParametricTemplate.h>

#include "QGIDrawingTemplate.h"

using namespace TechDrawGui;

QGIDrawingTemplate::QGIDrawingTemplate(QGraphicsScene *scene) : QGITemplate(scene),
                                                                                    pathItem(0)
{
    pathItem = new QGraphicsPathItem;

    // Invert the Y for the QGraphicsPathItem with Y pointing upwards
    QTransform qtrans;
    qtrans.scale(1., -1.);

    pathItem->setTransform(qtrans);

    addToGroup(pathItem);
}

QGIDrawingTemplate::~QGIDrawingTemplate()
{
    pathItem = 0;
}

QVariant QGIDrawingTemplate::itemChange(GraphicsItemChange change, const QVariant &value)
{
    return QGraphicsItemGroup::itemChange(change, value);
}


void QGIDrawingTemplate::clearContents()
{

}

TechDraw::DrawParametricTemplate * QGIDrawingTemplate::getParametricTemplate()
{
    if(pageTemplate && pageTemplate->isDerivedFrom(TechDraw::DrawParametricTemplate::getClassTypeId()))
        return static_cast<TechDraw::DrawParametricTemplate *>(pageTemplate);
    else
        return 0;
}

void QGIDrawingTemplate::draw()
{

    TechDraw::DrawParametricTemplate *tmplte = getParametricTemplate();
    if(!tmplte) {
        throw Base::RuntimeError("Template Feuature not set for QGIDrawingTemplate");
    }


    // Clear the previous geometry stored

    // Get a list of geometry and iterate
    const std::vector<TechDraw::BaseGeom *> &geoms =  tmplte->getGeometry();

    std::vector<TechDraw::BaseGeom *>::const_iterator it = geoms.begin();

    QPainterPath path;

    // Draw Edges
    // iterate through all the geometries
    for(; it != geoms.end(); ++it) {
        switch((*it)->geomType) {
          case TechDraw::GENERIC: {

            TechDraw::Generic *geom = static_cast<TechDraw::Generic *>(*it);

            path.moveTo(geom->points[0].x, geom->points[0].y);
            std::vector<Base::Vector3d>::const_iterator it = geom->points.begin();

            for(++it; it != geom->points.end(); ++it) {
                path.lineTo((*it).x, (*it).y);
            }
            break;
          }
          default:
            break;
        }
    }

    pathItem->setPath(path);
}

void QGIDrawingTemplate::updateView(bool update)
{
    Q_UNUSED(update);
    draw();
}

#include <Mod/TechDraw/Gui/moc_QGIDrawingTemplate.cpp>
