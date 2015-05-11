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
#endif


#include "FeaturePrimitive.h"
#include <Base/Exception.h>
#include <App/Document.h>
#include <BRepPrimAPI_MakeBox.hxx>
#include <BRepBuilderAPI_GTransform.hxx>
#include <BRepAlgoAPI_Fuse.hxx>
#include <BRepAlgoAPI_Cut.hxx>

using namespace PartDesign;

namespace PartDesign {


PROPERTY_SOURCE(PartDesign::FeaturePrimitive, PartDesign::FeatureAddSub)

FeaturePrimitive::FeaturePrimitive()
  :  primitiveType(Box)
{
    ADD_PROPERTY(References, (0,0));//, "Primitive", (App::PropertyType)(App::Prop_None), "References to build the location of the primitive");
}

App::DocumentObjectExecReturn* FeaturePrimitive::execute(const TopoDS_Shape& primitiveShape)
{
    try {
        //transform the primitive in the correct coordinance
        BRepBuilderAPI_GTransform mkTrf(primitiveShape, getLocation().Transformation());
        const TopoDS_Shape primitiveShape = mkTrf.Shape();
        
        //if we have no base we just add the standart primitive shape
        TopoDS_Shape base;
        try{
             base = getBaseShape();
        }
        catch(const Base::Exception&) {

             if(getAddSubType() == FeatureAddSub::Additive)
                 Shape.setValue(primitiveShape);
             else 
                 return new App::DocumentObjectExecReturn("Cannot subtract primitive feature without base feature");
                 
             AddSubShape.setValue(base);
             return  App::DocumentObject::StdReturn;
        }
         
        if(getAddSubType() == FeatureAddSub::Additive) {
            
            BRepAlgoAPI_Fuse mkFuse(base, primitiveShape);
            if (!mkFuse.IsDone())
                return new App::DocumentObjectExecReturn("Adding the primitive failed");
            // we have to get the solids (fuse sometimes creates compounds)
            TopoDS_Shape boolOp = this->getSolid(mkFuse.Shape());
            // lets check if the result is a solid
            if (boolOp.IsNull())
                return new App::DocumentObjectExecReturn("Resulting shape is not a solid");
            
            Shape.setValue(boolOp);
            AddSubShape.setValue(primitiveShape);
        }
        else if(getAddSubType() == FeatureAddSub::Subtractive) {
            
            BRepAlgoAPI_Cut mkCut(base, primitiveShape);
            if (!mkCut.IsDone())
                return new App::DocumentObjectExecReturn("Subtracting the primitive failed");
            // we have to get the solids (fuse sometimes creates compounds)
            TopoDS_Shape boolOp = this->getSolid(mkCut.Shape());
            // lets check if the result is a solid
            if (boolOp.IsNull())
                return new App::DocumentObjectExecReturn("Resulting shape is not a solid");
            
            Shape.setValue(boolOp);
            AddSubShape.setValue(primitiveShape);
        }
        
        
    }
    catch (Standard_Failure) {
        Handle_Standard_Failure e = Standard_Failure::Caught();
        return new App::DocumentObjectExecReturn(e->GetMessageString());
    }

    return App::DocumentObject::StdReturn;
}

TopLoc_Location FeaturePrimitive::calculateLocation()
{
    return TopLoc_Location();
}

PROPERTY_SOURCE(PartDesign::Box, PartDesign::FeaturePrimitive)

Box::Box()
{
    ADD_PROPERTY_TYPE(Length,(10.0f),"Box",App::Prop_None,"The length of the box");
    ADD_PROPERTY_TYPE(Width ,(10.0f),"Box",App::Prop_None,"The width of the box");
    ADD_PROPERTY_TYPE(Height,(10.0f),"Box",App::Prop_None,"The height of the box");
    
    primitiveType = FeaturePrimitive::Box;
}

App::DocumentObjectExecReturn* Box::execute(void)
{
    double L = Length.getValue();
    double W = Width.getValue();
    double H = Height.getValue();

    if (L < Precision::Confusion())
        return new App::DocumentObjectExecReturn("Length of box too small");

    if (W < Precision::Confusion())
        return new App::DocumentObjectExecReturn("Width of box too small");

    if (H < Precision::Confusion())
        return new App::DocumentObjectExecReturn("Height of box too small");

    try {
        // Build a box using the dimension attributes
        BRepPrimAPI_MakeBox mkBox(L, W, H);
        return FeaturePrimitive::execute(mkBox.Shape());
    }
    catch (Standard_Failure) {
        Handle_Standard_Failure e = Standard_Failure::Caught();
        return new App::DocumentObjectExecReturn(e->GetMessageString());
    }
}

short int Box::mustExecute() const
{
     if ( Length.isTouched() ||
          Height.isTouched() ||
          Width.isTouched() )
        return 1;
     
    return FeaturePrimitive::mustExecute();
}

PROPERTY_SOURCE(PartDesign::AdditiveBox, PartDesign::Box)
PROPERTY_SOURCE(PartDesign::SubtractiveBox, PartDesign::Box)

}
