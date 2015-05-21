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
#include <Inventor/nodes/SoTranslation.h>
#include <Inventor/nodes/SoAsciiText.h>
#include <Inventor/nodes/SoFont.h>
#include <Inventor/nodes/SoRotation.h>
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
#include <math.h>

using namespace PartDesignGui;

PROPERTY_SOURCE(PartDesignGui::ViewProviderDatumCoordinateSystem,PartDesignGui::ViewProviderDatum)

ViewProviderDatumCoordinateSystem::ViewProviderDatumCoordinateSystem()
{
    sPixmap = "PartDesign_CoordinateSystem.svg";
    
    SoMaterial* material = new SoMaterial();
    material->diffuseColor.setNum(4);
    material->diffuseColor.set1Value(0, SbColor(0.f, 0.f, 0.f));
    material->diffuseColor.set1Value(1, SbColor(1.f, 0.f, 0.f));
    material->diffuseColor.set1Value(2, SbColor(0.f, 1.f, 0.f));
    material->diffuseColor.set1Value(3, SbColor(0.f, 0.f, 1.f));
    SoMaterialBinding* binding = new SoMaterialBinding();
    binding->value = SoMaterialBinding::PER_FACE_INDEXED;
    pShapeSep->addChild(binding);
    pShapeSep->addChild(material);
    
    font = new SoFont();
    font->ref();
    transX = new SoTranslation();
    transX->ref();
    transY = new SoTranslation();
    transY->ref();
    transZ = new SoTranslation();
    transZ->ref();
}

ViewProviderDatumCoordinateSystem::~ViewProviderDatumCoordinateSystem()
{
    font->unref();
    transX->unref();
    transY->unref();
    transZ->unref();
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
        
        //normalize all to equal lengths
        Base::Vector3d axis = (x.Sqr() > y.Sqr()) ? x : y;
        axis = (axis.Sqr() > z.Sqr()) ? axis : z;
        x = x.Normalize()*axis.Length();
        y = y.Normalize()*axis.Length();
        z = z.Normalize()*axis.Length();


        // Display the line
        PartGui::SoBrepEdgeSet* lineSet;
        SoCoordinate3* coord;

        if (pShapeSep->getNumChildren() == 2) {
            coord = new SoCoordinate3();
            coord->point.setNum(4);
            coord->point.set1Value(0, base.x, base.y, base.z);
            coord->point.set1Value(1, x.x, x.y, x.z);
            coord->point.set1Value(2, y.x, y.y, y.z);
            coord->point.set1Value(3, z.x, z.y, z.z);

            pShapeSep->addChild(coord);
            lineSet = new PartGui::SoBrepEdgeSet();
            lineSet->coordIndex.setNum(9);
            lineSet->coordIndex.set1Value(0, 0);
            lineSet->coordIndex.set1Value(1, 1);
            lineSet->coordIndex.set1Value(2, -1);
            lineSet->coordIndex.set1Value(3, 0);
            lineSet->coordIndex.set1Value(4, 2);
            lineSet->coordIndex.set1Value(5, -1);
            lineSet->coordIndex.set1Value(6, 0);
            lineSet->coordIndex.set1Value(7, 3);
            lineSet->coordIndex.set1Value(8, -1);
            lineSet->materialIndex.setNum(3);
            lineSet->materialIndex.set1Value(0,1);
            lineSet->materialIndex.set1Value(1,2);
            lineSet->materialIndex.set1Value(2,3);
            pShapeSep->addChild(lineSet);
            
            pShapeSep->addChild(font);
            font->size = axis.Length()/10.;
            pShapeSep->addChild(transX);
            transX->translation.setValue(SbVec3f(x.x,x.y,x.z));
            SoAsciiText* t = new SoAsciiText();
            t->string = "X";
            pShapeSep->addChild(t);
            pShapeSep->addChild(transY);
            transY->translation.setValue(SbVec3f(-x.x + y.x, x.y + y.y, -x.z + y.z));
            t = new SoAsciiText();
            t->string = "Y";
            pShapeSep->addChild(t);
            pShapeSep->addChild(transZ);
            auto* rot = new SoRotation();
            rot->rotation = SbRotation(SbVec3f(0,1,0), M_PI/2);
            pShapeSep->addChild(rot);
            transZ->translation.setValue(SbVec3f(-y.x + z.x, -y.y + z.y, -y.z + z.z));
            t = new SoAsciiText();
            t->string = "Z";
            pShapeSep->addChild(t);
            
        } else {
            coord = static_cast<SoCoordinate3*>(pShapeSep->getChild(2));
            coord->point.set1Value(0, base.x, base.y, base.z);
            coord->point.set1Value(1, x.x, x.y, x.z);
            coord->point.set1Value(2, y.x, y.y, y.z);
            coord->point.set1Value(3, z.x, z.y, z.z);
            
            x = 9./10.*x;
            y = 9./10.*y;
            font->size = axis.Length()/10.;
            transX->translation.setValue(SbVec3f(x.x,x.y,x.z));
            transY->translation.setValue(SbVec3f(-x.x + y.x, x.y + y.y, -x.z + y.z));
            transZ->translation.setValue(SbVec3f(-y.x + z.x, -y.y + z.y, -y.z + z.z));
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

