// SPDX-License-Identifier: LGPL-2.1-or-later

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

#include "FeatureGroove.h"

using namespace PartDesign;

namespace PartDesign
{

/* TRANSLATOR PartDesign::Groove */

const char* Groove::TypeEnums[]
    = {"Angle", "ThroughAll", "UpToFirst", "UpToFace", "TwoAngles", nullptr};

PROPERTY_SOURCE(PartDesign::Groove, PartDesign::Revolved)

Groove::Groove()
{
    addSubType = FeatureAddSub::Subtractive;
    const double fullAngle = 360.0;
    const double emptyAngle = 0.0;

    ADD_PROPERTY_TYPE(Type, (0L), "Groove", App::Prop_None, "Groove type");
    Type.setEnums(TypeEnums);
    ADD_PROPERTY_TYPE(
        Base,
        (Base::Vector3d()),
        "Groove",
        App::PropertyType(App::Prop_ReadOnly | App::Prop_Hidden),
        "Base"
    );
    ADD_PROPERTY_TYPE(
        Axis,
        (Base::Vector3d::UnitY),
        "Groove",
        App::PropertyType(App::Prop_ReadOnly | App::Prop_Hidden),
        "Axis"
    );
    ADD_PROPERTY_TYPE(Angle, (fullAngle), "Groove", App::Prop_None, "Angle");
    ADD_PROPERTY_TYPE(Angle2, (emptyAngle), "Groove", App::Prop_None, "Groove length in 2nd direction");
    ADD_PROPERTY_TYPE(UpToFace, (nullptr), "Groove", App::Prop_None, "Face where groove will end");
    ADD_PROPERTY_TYPE(ReferenceAxis, (nullptr), "Groove", (App::Prop_None), "Reference axis of groove");
}

App::DocumentObjectExecReturn* Groove::execute()
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

}  // namespace PartDesign
