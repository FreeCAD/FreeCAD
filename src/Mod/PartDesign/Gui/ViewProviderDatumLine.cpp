/***************************************************************************
 *   Copyright (c) 2013 Jan Rheinlaender <jrheinlaender@users.sourceforge.net>        *
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
# include <Inventor/nodes/SoSeparator.h>
# include <Inventor/nodes/SoMaterial.h>
# include <Inventor/nodes/SoCoordinate3.h>
# include <TopoDS_Vertex.hxx>
# include <TopoDS.hxx>
# include <BRep_Tool.hxx>
# include <gp_Pnt.hxx>
# include <Precision.hxx>
# include <Geom_Plane.hxx>
# include <Geom_Line.hxx>
# include <GeomAPI_IntCS.hxx>
#endif

#include "ViewProviderDatumLine.h"
#include "TaskDatumParameters.h"
#include "Workbench.h"
#include <Mod/Part/Gui/SoBrepFaceSet.h>
#include <Mod/Part/Gui/SoBrepEdgeSet.h>
#include <Mod/Part/Gui/SoBrepPointSet.h>
#include <Mod/PartDesign/App/DatumLine.h>
#include <Gui/Control.h>
#include <Gui/Command.h>
#include <Gui/Application.h>
#include <Mod/PartDesign/App/Body.h>

using namespace PartDesignGui;

PROPERTY_SOURCE(PartDesignGui::ViewProviderDatumLine,PartDesignGui::ViewProviderDatum)

ViewProviderDatumLine::ViewProviderDatumLine()
{
    SoMaterial* material = new SoMaterial();
    material->diffuseColor.setValue(0.9f, 0.9f, 0.13f);
    material->transparency.setValue(0.2f);
    pShapeSep->addChild(material);
}

ViewProviderDatumLine::~ViewProviderDatumLine()
{

}

void ViewProviderDatumLine::updateData(const App::Property* prop)
{
    // Gets called whenever a property of the attached object changes
    PartDesign::Line* pcDatum = static_cast<PartDesign::Line*>(this->getObject());

    if (strcmp(prop->getName(),"Placement") == 0) {
        Base::Placement plm = pcDatum->Placement.getValue();
        plm.invert();
        Base::Vector3d base(0,0,0);
        Base::Vector3d dir(0,0,1);

        // Get limits of the line from bounding box of the body
        PartDesign::Body* body = static_cast<PartDesign::Body*>(Part::BodyBase::findBodyOf(this->getObject()));
        if (body == NULL)
            return;
        Base::BoundBox3d bbox = body->getBoundBox();
        bbox = bbox.Transformed(plm.toMatrix());
        bbox.Enlarge(0.1 * bbox.CalcDiagonalLength());
        Base::Vector3d p1, p2;
        if (bbox.IsInBox(base)) {
            bbox.IntersectionPoint(base, dir, p1, Precision::Confusion());
            bbox.IntersectionPoint(base, -dir, p2, Precision::Confusion());
        } else {
            bbox.IntersectWithLine(base, dir, p1, p2);
            if ((p1 == Base::Vector3d(0,0,0)) && (p2 == Base::Vector3d(0,0,0)))
                bbox.IntersectWithLine(base, -dir, p1, p2);
        }

        // Display the line
        PartGui::SoBrepEdgeSet* lineSet;
        SoCoordinate3* coord;

        if (pShapeSep->getNumChildren() == 1) {
            coord = new SoCoordinate3();
            coord->point.setNum(2);
            coord->point.set1Value(0, p1.x, p1.y, p1.z);
            coord->point.set1Value(1, p2.x, p2.y, p2.z);
            pShapeSep->addChild(coord);
            lineSet = new PartGui::SoBrepEdgeSet();
            lineSet->coordIndex.setNum(2);
            lineSet->coordIndex.set1Value(0, 0);
            lineSet->coordIndex.set1Value(1, 1);
            pShapeSep->addChild(lineSet);
        } else {
            coord = static_cast<SoCoordinate3*>(pShapeSep->getChild(1));
            coord->point.set1Value(0, p1.x, p1.y, p1.z);
            coord->point.set1Value(1, p2.x, p2.y, p2.z);
        }
    }

    ViewProviderDatum::updateData(prop);
}

