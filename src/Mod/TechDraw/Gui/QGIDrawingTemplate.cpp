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
# include <QPainterPath>
#endif

#include <Mod/TechDraw/App/Geometry.h>
#include <Mod/TechDraw/App/DrawParametricTemplate.h>

#include "QGIDrawingTemplate.h"


using namespace TechDrawGui;

QGIDrawingTemplate::QGIDrawingTemplate(QGSPage* scene) : QGITemplate(scene),
                                                                                    pathItem(nullptr)
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
    pathItem = nullptr;
}

void QGIDrawingTemplate::clearContents()
{

}

TechDraw::DrawParametricTemplate * QGIDrawingTemplate::getParametricTemplate()
{
    if(pageTemplate && pageTemplate->isDerivedFrom(TechDraw::DrawParametricTemplate::getClassTypeId()))
        return static_cast<TechDraw::DrawParametricTemplate *>(pageTemplate);
    else
        return nullptr;
}

void QGIDrawingTemplate::draw()
{

    TechDraw::DrawParametricTemplate *tmplte = getParametricTemplate();
    if(!tmplte) {
        throw Base::RuntimeError("Template Feuature not set for QGIDrawingTemplate");
    }


    // Clear the previous geometry stored

    // Get a list of geometry and iterate
    const TechDraw::BaseGeomPtrVector &geoms =  tmplte->getGeometry();

    TechDraw::BaseGeomPtrVector::const_iterator it = geoms.begin();

    QPainterPath path;

    // Draw Edges
    // iterate through all the geometries
    for(; it != geoms.end(); ++it) {
        switch((*it)->getGeomType()) {
          case TechDraw::GENERIC: {

            TechDraw::GenericPtr geom = std::static_pointer_cast<TechDraw::Generic>(*it);

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
