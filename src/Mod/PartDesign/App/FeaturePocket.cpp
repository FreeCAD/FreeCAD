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
#ifndef _PreComp_
# include <Mod/Part/App/FCBRepAlgoAPI_Cut.h>
# include <gp_Dir.hxx>
# include <Precision.hxx>
# include <TopExp_Explorer.hxx>
# include <TopoDS_Face.hxx>
#endif

#include <App/DocumentObject.h>
#include <Base/Exception.h>

#include "FeaturePocket.h"

using namespace PartDesign;

/* TRANSLATOR PartDesign::Pocket */

const char* Pocket::TypeEnums[]= {"Length", "ThroughAll", "UpToFirst", "UpToFace", "TwoLengths", "UpToShape", nullptr};

PROPERTY_SOURCE(PartDesign::Pocket, PartDesign::FeatureExtrude)

Pocket::Pocket()
{
    addSubType = FeatureAddSub::Subtractive;

    ADD_PROPERTY_TYPE(Type, ((long)0), "Pocket", App::Prop_None, "Pocket type");
    Type.setEnums(TypeEnums);
    ADD_PROPERTY_TYPE(Length, (5.0), "Pocket", App::Prop_None, "Pocket length");
    ADD_PROPERTY_TYPE(Length2, (5.0), "Pocket", App::Prop_None, "Pocket length in 2nd direction");
    ADD_PROPERTY_TYPE(UseCustomVector, (false), "Pocket", App::Prop_None, "Use custom vector for pocket direction");
    ADD_PROPERTY_TYPE(Direction, (Base::Vector3d(1.0, 1.0, 1.0)), "Pocket", App::Prop_None, "Pocket direction vector");
    ADD_PROPERTY_TYPE(ReferenceAxis, (nullptr), "Pocket", App::Prop_None, "Reference axis of direction");
    ADD_PROPERTY_TYPE(AlongSketchNormal, (true), "Pocket", App::Prop_None, "Measure pocket length along the sketch normal direction");
    ADD_PROPERTY_TYPE(UpToFace, (nullptr), "Pocket", App::Prop_None, "Face where pocket will end");
    ADD_PROPERTY_TYPE(UpToShape, (nullptr), "Pocket", App::Prop_None, "Face(s) or shape(s) where pocket will end");
    ADD_PROPERTY_TYPE(Offset, (0.0), "Pocket", App::Prop_None, "Offset from face in which pocket will end");
    Offset.setConstraints(&signedLengthConstraint);
    ADD_PROPERTY_TYPE(TaperAngle, (0.0), "Pocket", App::Prop_None, "Taper angle");
    TaperAngle.setConstraints(&floatAngle);
    ADD_PROPERTY_TYPE(TaperAngle2, (0.0), "Pocket", App::Prop_None, "Taper angle for 2nd direction");
    TaperAngle2.setConstraints(&floatAngle);

    // Remove the constraints and keep the type to allow to accept negative values
    // https://forum.freecad.org/viewtopic.php?f=3&t=52075&p=448410#p447636
    Length2.setConstraints(nullptr);
}

App::DocumentObjectExecReturn *Pocket::execute()
{
    // MakeFace|MakeFuse: because we want a solid.
    // InverseDirection: to inverse the auto detected extrusion direction for
    // backward compatibility to upstream
    ExtrudeOptions options(ExtrudeOption::MakeFace | ExtrudeOption::MakeFuse
                           | ExtrudeOption::InverseDirection);
    return buildExtrusion(options);
}

Base::Vector3d Pocket::getProfileNormal() const
{
    auto res = FeatureExtrude::getProfileNormal();
    // turn around for pockets
    return res * -1;
}
