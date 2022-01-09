/***************************************************************************
 *   Copyright (c) 2020 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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
# include <BRep_Builder.hxx>
# include <BRep_Tool.hxx>
# include <BRepAlgoAPI_Common.hxx>
# include <BRepAlgoAPI_Fuse.hxx>
# include <BRepAdaptor_Surface.hxx>
# include <BRepBndLib.hxx>
# include <BRepBuilderAPI_MakeFace.hxx>
# include <BRepFeat_MakePrism.hxx>
# include <BRepLProp_SLProps.hxx>
# include <BRepPrimAPI_MakeHalfSpace.hxx>
# include <Geom_Surface.hxx>
# include <GeomAPI_ProjectPointOnSurf.hxx>
# include <GeomLib_IsPlanarSurface.hxx>
# include <gp_Pln.hxx>
# include <Precision.hxx>
# include <TopoDS.hxx>
# include <TopoDS_Compound.hxx>
# include <TopoDS_Face.hxx>
# include <TopoDS_Solid.hxx>
# include <TopoDS_Wire.hxx>
# include <TopExp_Explorer.hxx>
#endif

#include <App/Document.h>
#include <Base/Console.h>
#include <Base/Exception.h>
#include <Base/Placement.h>
#include <Base/Reader.h>

#include "FeatureExtrude.h"

using namespace PartDesign;


PROPERTY_SOURCE(PartDesign::FeatureExtrude, PartDesign::ProfileBased)

App::PropertyQuantityConstraint::Constraints FeatureExtrude::signedLengthConstraint = { -DBL_MAX, DBL_MAX, 1.0 };

FeatureExtrude::FeatureExtrude()
{
}

short FeatureExtrude::mustExecute() const
{
    if (Placement.isTouched() ||
        Type.isTouched() ||
        Length.isTouched() ||
        Length2.isTouched() ||
        UseCustomVector.isTouched() ||
        Direction.isTouched() ||
        ReferenceAxis.isTouched() ||
        AlongSketchNormal.isTouched() ||
        Offset.isTouched() ||
        UpToFace.isTouched())
        return 1;
    return ProfileBased::mustExecute();
}

Base::Vector3d FeatureExtrude::computeDirection(const Base::Vector3d& sketchVector)
{
    Base::Vector3d extrudeDirection;

    if (!UseCustomVector.getValue()) {
        if (!ReferenceAxis.getValue()) {
            // use sketch's normal vector for direction
            extrudeDirection = sketchVector;
            AlongSketchNormal.setReadOnly(true);
        }
        else {
            // update Direction from ReferenceAxis
            App::DocumentObject* pcReferenceAxis = ReferenceAxis.getValue();
            const std::vector<std::string>& subReferenceAxis = ReferenceAxis.getSubValues();
            Base::Vector3d base;
            Base::Vector3d dir;
            getAxis(pcReferenceAxis, subReferenceAxis, base, dir, ForbiddenAxis::NotPerpendicularWithNormal);
            switch (addSubType) {
            case Type::Additive:
                extrudeDirection = dir;
                break;
            case Type::Subtractive:
                extrudeDirection = -dir;
                break;
            }
        }
    }
    else {
        // use the given vector
        // if null vector, use sketchVector
        if ((fabs(Direction.getValue().x) < Precision::Confusion())
            && (fabs(Direction.getValue().y) < Precision::Confusion())
            && (fabs(Direction.getValue().z) < Precision::Confusion())) {
            Direction.setValue(sketchVector);
        }
        extrudeDirection = Direction.getValue();
    }

    // disable options of UseCustomVector
    Direction.setReadOnly(!UseCustomVector.getValue());
    ReferenceAxis.setReadOnly(UseCustomVector.getValue());
    // UseCustomVector allows AlongSketchNormal but !UseCustomVector does not forbid it
    if (UseCustomVector.getValue())
        AlongSketchNormal.setReadOnly(false);

    // explicitly set the Direction so that the dialog shows also the used direction
    // if the sketch's normal vector was used
    Direction.setValue(extrudeDirection);
    return extrudeDirection;
}
