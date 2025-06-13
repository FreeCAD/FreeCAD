/***************************************************************************
 *   Copyright (c) 2010 Juergen Riegel <FreeCAD@juergen-riegel.net>        *
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

#include "FeatureRevolution.h"

using namespace PartDesign;

// NOLINTBEGIN
namespace PartDesign {

/* TRANSLATOR PartDesign::Revolution */

Revolution::MethodsArray Revolution::TypeEnums = {"Angle",
                                                  "UpToLast",
                                                  "UpToFirst",
                                                  "UpToFace",
                                                  "TwoAngles",
                                                  nullptr};

PROPERTY_SOURCE(PartDesign::Revolution, PartDesign::Revolved)

Revolution::Revolution()
{
    addSubType = FeatureAddSub::Additive;
    const double defAngle1 = 360.0;
    const double defAngle2 = 0.0;

    ADD_PROPERTY_TYPE(Type, (0L), "Revolution", App::Prop_None, "Revolution type");
    Type.setEnums(TypeEnums.data());
    ADD_PROPERTY_TYPE(Base, (Base::Vector3d(0.0,0.0,0.0)), "Revolution", App::PropertyType(App::Prop_ReadOnly | App::Prop_Hidden), "Base");
    ADD_PROPERTY_TYPE(Axis, (Base::Vector3d(0.0,1.0,0.0)), "Revolution", App::PropertyType(App::Prop_ReadOnly | App::Prop_Hidden), "Axis");
    ADD_PROPERTY_TYPE(Angle, (defAngle1), "Revolution", App::Prop_None, "Angle");
    ADD_PROPERTY_TYPE(Angle2, (defAngle2), "Revolution", App::Prop_None, "Revolution length in 2nd direction");
    ADD_PROPERTY_TYPE(UpToFace, (nullptr), "Revolution", App::Prop_None, "Face where revolution will end");
    ADD_PROPERTY_TYPE(ReferenceAxis, (nullptr), "Revolution", (App::Prop_None), "Reference axis of revolution");
}

App::DocumentObjectExecReturn *Revolution::execute()
{
    return executeRevolved(Part::RevolMode::FuseWithBase);
}

TopoShape Revolution::makeShape(const TopoShape& base, const TopoShape& revolve) const
{
    return revolve.makeElementFuse(base);
}

bool Revolution::suggestReversedAngle(double angle) const
{
    return angle < 0.0;
}

}
// NOLINTEND
