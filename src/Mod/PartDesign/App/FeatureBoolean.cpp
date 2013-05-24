/******************************************************************************
 *   Copyright (c)2013 Jan Rheinlaender <jrheinlaender@users.sourceforge.net> *
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
#ifndef _PreComp_
# include <BRepAlgoAPI_Fuse.hxx>
# include <BRepAlgoAPI_Cut.hxx>
# include <BRepAlgoAPI_Common.hxx>
# include <BRepAlgoAPI_Section.hxx>
#endif

#include "Body.h"
#include "FeatureBoolean.h"

#include <Base/Console.h>
#include <Base/Exception.h>

using namespace PartDesign;

namespace PartDesign {

PROPERTY_SOURCE(PartDesign::Boolean, PartDesign::Feature)

const char* Boolean::TypeEnums[]= {"Fuse","Cut","Common","Section",NULL};

Boolean::Boolean()
{
    ADD_PROPERTY(Type,((long)0));
    Type.setEnums(TypeEnums);
    ADD_PROPERTY(Bodies,(0));
    Bodies.setSize(0);
}

short Boolean::mustExecute() const
{
    if (Bodies.isTouched())
        return 1;
    return PartDesign::Feature::mustExecute();
}

App::DocumentObjectExecReturn *Boolean::execute(void)
{
    // Get the base shape to operate on
    Part::TopoShape baseTopShape;
    try {
        baseTopShape = getBaseTopoShape();
    } catch (const Base::Exception&) {
        return new App::DocumentObjectExecReturn("Cannot do boolean operation with invalid base shape");
    }

    std::vector<App::DocumentObject*> bodies = Bodies.getValues();
    if (bodies.empty())
        return App::DocumentObject::StdReturn;

    // create an untransformed copy of the base shape
    Part::TopoShape baseShape(baseTopShape);
    baseShape.setTransform(Base::Matrix4D());
    TopoDS_Shape result = baseShape._Shape;

    // Position this feature by the first body
    const Part::Feature* baseFeature;
    try {
        baseFeature = getBaseObject();
    } catch (const Base::Exception&) {
        return new App::DocumentObjectExecReturn("Cannot do boolean operation with invalid BaseFeature");
    }
    this->Placement.setValue(baseFeature->Placement.getValue());

    // Get the operation type
    std::string type = Type.getValueAsString();

    for (std::vector<App::DocumentObject*>::const_iterator b = bodies.begin(); b != bodies.end(); b++)
    {
        // Extract the body shape
        PartDesign::Body* body = static_cast<PartDesign::Body*>(*b);
        TopoDS_Shape shape = body->Shape.getValue();
        TopoDS_Shape boolOp;

        if (type == "Fuse") {
            BRepAlgoAPI_Fuse mkFuse(result, shape);
            if (!mkFuse.IsDone())
                return new App::DocumentObjectExecReturn("Fusion of bodies failed", *b);
            // we have to get the solids (fuse sometimes creates compounds)
            boolOp = this->getSolid(mkFuse.Shape());
            // lets check if the result is a solid
            if (boolOp.IsNull())
                return new App::DocumentObjectExecReturn("Resulting shape is not a solid", *b);
        } else if (type == "Cut") {
            BRepAlgoAPI_Cut mkCut(result, shape);
            if (!mkCut.IsDone())
                return new App::DocumentObjectExecReturn("Cut out of first body failed", *b);
            boolOp = mkCut.Shape();
        } else if (type == "Common") {
            BRepAlgoAPI_Common mkCommon(result, shape);
            if (!mkCommon.IsDone())
                return new App::DocumentObjectExecReturn("Common operation with first body failed", *b);
            boolOp = mkCommon.Shape();
        } else if (type == "Section") {
            BRepAlgoAPI_Section mkSection(result, shape);
            if (!mkSection.IsDone())
                return new App::DocumentObjectExecReturn("Section out of first body failed", *b);
            // we have to get the solids
            boolOp = this->getSolid(mkSection.Shape());
            // lets check if the result is a solid
            if (boolOp.IsNull())
                return new App::DocumentObjectExecReturn("Resulting shape is not a solid", *b);
        }

        result = boolOp; // Use result of this operation for fuse/cut of next body
    }

    this->Shape.setValue(result);
    return App::DocumentObject::StdReturn;
}

}
