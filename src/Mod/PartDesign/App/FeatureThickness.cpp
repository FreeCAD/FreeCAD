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
# include <TopoDS.hxx>
# include <Precision.hxx>
#endif

#include "FeatureThickness.h"
#include <Base/Exception.h>


using namespace PartDesign;

const char* PartDesign::Thickness::ModeEnums[] = {"Skin","Pipe", "RectoVerso",NULL};
const char* PartDesign::Thickness::JoinEnums[] = {"Arc", "Intersection",NULL};

PROPERTY_SOURCE(PartDesign::Thickness, PartDesign::DressUp)

Thickness::Thickness()
{
    ADD_PROPERTY_TYPE(Value,(1.0),"Thickness",App::Prop_None,"Thickness value");
    ADD_PROPERTY_TYPE(Mode,(long(0)),"Thickness",App::Prop_None,"Mode");
    Mode.setEnums(ModeEnums);
    ADD_PROPERTY_TYPE(Join,(long(0)),"Thickness",App::Prop_None,"Join type");
    Join.setEnums(JoinEnums);
    ADD_PROPERTY_TYPE(Reversed,(false),"Thickness",App::Prop_None,"Apply the thickness towards the solids interior");
}

short Thickness::mustExecute() const
{
    if (Placement.isTouched() ||
        Value.isTouched() ||
        Mode.isTouched() ||
        Join.isTouched())
        return 1;
    return DressUp::mustExecute();
}

App::DocumentObjectExecReturn *Thickness::execute(void)
{
    // Base shape
    Part::TopoShape TopShape;
    try {
        TopShape = getBaseShape();
    } catch (Base::Exception& e) {
        return new App::DocumentObjectExecReturn(e.what());
    }

    TopTools_ListOfShape closingFaces;
    const std::vector<std::string>& subStrings = Base.getSubValues();
    for (std::vector<std::string>::const_iterator it = subStrings.begin(); it != subStrings.end(); ++it) {
        TopoDS_Face face = TopoDS::Face(TopShape.getSubShape(it->c_str()));
        closingFaces.Append(face);
    }

    bool reversed = Reversed.getValue();
    double thickness =  (reversed ? -1. : 1. )*Value.getValue();
    double tol = Precision::Confusion();
    short mode = (short)Mode.getValue();
    short join = (short)Join.getValue();
    //we do not offer tangent join type
    if(join == 1)
        join = 2;

    if (fabs(thickness) > 2*tol)
        this->Shape.setValue(getSolid(TopShape.makeThickSolid(closingFaces, thickness, tol, false, false, mode, join)));
    else
        this->Shape.setValue(getSolid(TopShape.getShape()));
    return App::DocumentObject::StdReturn;
}
