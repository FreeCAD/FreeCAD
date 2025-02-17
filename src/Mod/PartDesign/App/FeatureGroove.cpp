/******************************************************************************
 *   Copyright (c) 2012 Jan Rheinländer <jrheinlaender@users.sourceforge.net> *
 *                                                                            *
 *   This file is part of the FreeCAD CAx development system.                 *
 *                                                                            *
 *   This library is free software; you can redistribute it and/or            *
 *   modify it under the terms of the GNU Library General Public              *
 *   License as published by the Free Software Foundation; either             *
 *   version 2 of the License, or (at your option) any later version.         *
 *                                                                            *
 *   This library  is distributed in the hope that it will be useful,         *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of           *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the            *
 *   GNU Library General Public License for more details.                     *
 *                                                                            *
 *   You should have received a copy of the GNU Library General Public        *
 *   License along with this library; see the file COPYING.LIB. If not,       *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,            *
 *   Suite 330, Boston, MA  02111-1307, USA                                   *
 *                                                                            *
 ******************************************************************************/


#include "PreCompiled.h"

#include "FeatureGroove.h"

using namespace PartDesign;

// NOLINTBEGIN
namespace PartDesign {

/* TRANSLATOR PartDesign::Groove */

Groove::MethodsArray Groove::TypeEnums = {"Angle",
                                          "ThroughAll",
                                          "UpToFirst",
                                          "UpToFace",
                                          "TwoAngles",
                                          nullptr};

PROPERTY_SOURCE(PartDesign::Groove, PartDesign::Revolved)

Groove::Groove()
{
    addSubType = FeatureAddSub::Subtractive;
    const double defAngle1 = 360.0;
    const double defAngle2 = 0.0;

    ADD_PROPERTY_TYPE(Type, (0L), "Groove", App::Prop_None, "Groove type");
    Type.setEnums(TypeEnums.data());
    ADD_PROPERTY_TYPE(Base, (Base::Vector3d(0.0,0.0,0.0)), "Groove", App::PropertyType(App::Prop_ReadOnly | App::Prop_Hidden), "Base");
    ADD_PROPERTY_TYPE(Axis, (Base::Vector3d(0.0,1.0,0.0)), "Groove", App::PropertyType(App::Prop_ReadOnly | App::Prop_Hidden), "Axis");
    ADD_PROPERTY_TYPE(Angle, (defAngle1),"Groove", App::Prop_None, "Angle");
    ADD_PROPERTY_TYPE(Angle2, (defAngle2), "Groove", App::Prop_None, "Groove length in 2nd direction");
    ADD_PROPERTY_TYPE(UpToFace, (nullptr), "Groove", App::Prop_None, "Face where groove will end");
    ADD_PROPERTY_TYPE(ReferenceAxis, (nullptr), "Groove", (App::Prop_None), "Reference axis of groove");
}

App::DocumentObjectExecReturn *Groove::execute()
{
    return executeRevolved(Part::RevolMode::CutFromBase);
}

TopoShape Groove::makeShape(const TopoShape& base, const TopoShape& revolve) const
{
    return base.makeElementCut(revolve);
}

bool Groove::suggestReversedAngle(double angle) const
{
    return angle > 0.0;
}

}
// NOLINTEND
