// SPDX-License-Identifier: LGPL-2.1-or-later

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

#include "FeatureRevolution.h"

#include <Base/ProgramVersion.h>

using namespace PartDesign;

namespace PartDesign
{

/* TRANSLATOR PartDesign::Revolution */

const char* Revolution::TypeEnums[]
    = {"Angle", "UpToLast", "UpToFirst", "UpToFace", "TwoAngles", nullptr};

const char* Revolution::FuseOrderEnums[] = {"BaseFirst", "FeatureFirst", nullptr};

PROPERTY_SOURCE(PartDesign::Revolution, PartDesign::Revolved)

Revolution::Revolution()
{
    addSubType = FeatureAddSub::Additive;
    const double fullAngle = 360.0;
    const double emptyAngle = 0.0;

    ADD_PROPERTY_TYPE(Type, (0L), "Revolution", App::Prop_None, "Revolution type");
    Type.setEnums(TypeEnums);
    ADD_PROPERTY_TYPE(
        Base,
        (Base::Vector3d()),
        "Revolution",
        App::PropertyType(App::Prop_ReadOnly | App::Prop_Hidden),
        "Base"
    );
    ADD_PROPERTY_TYPE(
        Axis,
        (Base::Vector3d::UnitY),
        "Revolution",
        App::PropertyType(App::Prop_ReadOnly | App::Prop_Hidden),
        "Axis"
    );
    ADD_PROPERTY_TYPE(Angle, (fullAngle), "Revolution", App::Prop_None, "Angle");
    ADD_PROPERTY_TYPE(
        Angle2,
        (emptyAngle),
        "Revolution",
        App::Prop_None,
        "Revolution length in 2nd direction"
    );
    ADD_PROPERTY_TYPE(UpToFace, (nullptr), "Revolution", App::Prop_None, "Face where revolution will end");
    ADD_PROPERTY_TYPE(
        ReferenceAxis,
        (nullptr),
        "Revolution",
        (App::Prop_None),
        "Reference axis of revolution"
    );
    ADD_PROPERTY_TYPE(
        FuseOrder,
        (BaseFirst),
        "Compatibility",
        App::Prop_Hidden,
        "Order of fuse operation to preserve compatibility with files created using FreeCAD 1.0"
    );
    FuseOrder.setEnums(FuseOrderEnums);
}

short Revolution::mustExecute() const
{
    if (FuseOrder.isTouched()) {
        return 1;
    }
    return Revolved::mustExecute();
}

App::DocumentObjectExecReturn* Revolution::execute()
{
    return executeRevolved(Part::RevolMode::FuseWithBase);
}

TopoShape Revolution::makeShape(const TopoShape& base, const TopoShape& revolve) const
{
    // In 1.0 there was a bug that caused the order of operations to be reversed.
    // Changing the order may impact geometry order and the results of refine operation,
    // hence we need to support both ways to ensure compatibility.
    if (FuseOrder.getValue() == FeatureFirst) {
        return revolve.makeElementFuse(base);
    }
    return base.makeElementFuse(revolve);
}

bool Revolution::suggestReversedAngle(double angle) const
{
    return angle < 0.0;
}

void Revolution::Restore(Base::XMLReader& reader)
{
    Revolved::Restore(reader);

    // For 1.0 and 1.0 only the order was feature first due to a bug
    if (Base::getVersion(reader.ProgramVersion) == Base::Version::v1_0) {
        FuseOrder.setValue(FeatureFirst);
    }
}

}  // namespace PartDesign
