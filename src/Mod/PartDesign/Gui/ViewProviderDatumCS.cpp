/***************************************************************************
 *   Copyright (c) 2015 Stefan Tröger <stefantroeger@gmx.net>              *
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
# include <Inventor/nodes/SoLineSet.h>
# include <TopoDS_Vertex.hxx>
# include <TopoDS.hxx>
# include <BRep_Tool.hxx>
# include <gp_Pnt.hxx>
# include <Precision.hxx>
# include <Geom_Plane.hxx>
# include <Geom_Line.hxx>
# include <GeomAPI_IntCS.hxx>
#endif

#include "ViewProviderDatumCS.h"
#include "TaskDatumParameters.h"
#include "Workbench.h"
#include <Mod/PartDesign/App/DatumCS.h>
#include <Mod/Part/Gui/SoBrepFaceSet.h>
#include <Mod/Part/Gui/SoBrepEdgeSet.h>
#include <Mod/Part/Gui/SoBrepPointSet.h>
#include <Gui/Control.h>
#include <Gui/Command.h>
#include <Gui/Application.h>
#include <Mod/PartDesign/App/Body.h>
#include <Mod/PartDesign/App/FeaturePrimitive.h>

using namespace PartDesignGui;

PROPERTY_SOURCE(PartDesignGui::ViewProviderDatumCoordinateSystem,PartDesignGui::ViewProviderDatum)

ViewProviderDatumCoordinateSystem::ViewProviderDatumCoordinateSystem()
{
    sPixmap = "PartDesign_CoordinateSystem.svg";
    
    SoMaterial* material = new SoMaterial();
    material->diffuseColor.setValue(0.9f, 0.9f, 0.13f);
    material->transparency.setValue(0.5f);
    pShapeSep->addChild(material);
}

ViewProviderDatumCoordinateSystem::~ViewProviderDatumCoordinateSystem()
{

}

void ViewProviderDatumCoordinateSystem::updateData(const App::Property* prop)
{
    if (strcmp(prop->getName(),"Placement") == 0) {
        
        
        Base::Vector3d base(0,0,0);
        Base::Vector3d dir(0,0,1);
        Base::Vector3d x, y, z;
        getPointForDirection(Base::Vector3d(1,0,0), x);
        getPointForDirection(Base::Vector3d(0,1,0), y);
        getPointForDirection(Base::Vector3d(0,0,1), z);


        // Display the line
        PartGui::SoBrepEdgeSet* lineSet;
        SoCoordinate3* coord;

        if (pShapeSep->getNumChildren() == 1) {
            coord = new SoCoordinate3();
            coord->point.setNum(4);
            coord->point.set1Value(0, base.x, base.y, base.z);
            coord->point.set1Value(1, x.x, x.y, x.z);
            coord->point.set1Value(2, y.x, y.y, y.z);
            coord->point.set1Value(3, z.x, z.y, z.z);

            pShapeSep->addChild(coord);
            lineSet = new PartGui::SoBrepEdgeSet();
            lineSet->coordIndex.setNum(6);
            lineSet->coordIndex.set1Value(0, 0);
            lineSet->coordIndex.set1Value(1, 1);
            lineSet->coordIndex.set1Value(2, 0);
            lineSet->coordIndex.set1Value(3, 2);
            lineSet->coordIndex.set1Value(4, 0);
            lineSet->coordIndex.set1Value(5, 3);
            pShapeSep->addChild(lineSet);
        } else {
            coord = static_cast<SoCoordinate3*>(pShapeSep->getChild(1));
            coord->point.set1Value(0, base.x, base.y, base.z);
            coord->point.set1Value(1, x.x, x.y, x.z);
            coord->point.set1Value(2, y.x, y.y, y.z);
            coord->point.set1Value(3, z.x, z.y, z.z);
        }
    }
    
    ViewProviderDatum::updateData(prop);
}

void ViewProviderDatumCoordinateSystem::getPointForDirection(Base::Vector3d dir, Base::Vector3d& p) {

    // Gets called whenever a property of the attached object changes
    PartDesign::CoordinateSystem* pcDatum = static_cast<PartDesign::CoordinateSystem*>(this->getObject());
    Base::Placement plm = pcDatum->Placement.getValue();
    plm.invert();
    
    Base::Vector3d base(0,0,0);
    // Get limits of the line from bounding box of the body
    PartDesign::Body* body = static_cast<PartDesign::Body*>(Part::BodyBase::findBodyOf(this->getObject()));
    if (body == NULL)
        return;
    Base::BoundBox3d bbox = body->getBoundBox();
    bbox = bbox.Transformed(plm.toMatrix());
    bbox.Enlarge(0.1 * bbox.CalcDiagonalLength());
    if (bbox.IsInBox(base)) {
        bbox.IntersectionPoint(base, dir, p, Precision::Confusion());
    } else {
        Base::Vector3d p2;
        if(!bbox.IntersectWithLine(base, dir, p, p2)) {
            p = dir*bbox.CalcDiagonalLength();
        }
    }
}

