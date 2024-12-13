/***************************************************************************
 *   Copyright (c) 2008 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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
# include <BRepAlgo.hxx>
# include <BRepFilletAPI_MakeFillet.hxx>
# include <TopoDS.hxx>
# include <TopoDS_Edge.hxx>
# include <TopTools_ListOfShape.hxx>
# include <ShapeFix_Shape.hxx>
# include <ShapeFix_ShapeTolerance.hxx>
#endif

#include <Base/Exception.h>
#include <Base/Reader.h>
#include <Mod/Part/App/TopoShape.h>

#include "FeatureFillet.h"


using namespace PartDesign;


PROPERTY_SOURCE(PartDesign::Fillet, PartDesign::DressUp)

const App::PropertyQuantityConstraint::Constraints floatRadius = {0.0,FLT_MAX,0.1};

Fillet::Fillet()
{
    ADD_PROPERTY_TYPE(Radius, (1.0), "Fillet", App::Prop_None, "Fillet radius.");
    Radius.setUnit(Base::Unit::Length);
    Radius.setConstraints(&floatRadius);
    ADD_PROPERTY_TYPE(UseAllEdges, (false), "Fillet", App::Prop_None,
      "Fillet all edges if true, else use only those edges in Base property.\n"
      "If true, then this overrides any edge changes made to the Base property or in the dialog.\n");
}

short Fillet::mustExecute() const
{
    if (Placement.isTouched() || Radius.isTouched())
        return 1;
    return DressUp::mustExecute();
}

App::DocumentObjectExecReturn *Fillet::execute()
{
    if (onlyHasToRefine()){
        TopoShape result = refineShapeIfActive(rawShape);
        Shape.setValue(result);
        return App::DocumentObject::StdReturn;
    }

    Part::TopoShape baseShape;
    try {
        baseShape = getBaseTopoShape();
    }
    catch (Base::Exception& e) {
        return new App::DocumentObjectExecReturn(e.what());
    }
    baseShape.setTransform(Base::Matrix4D());

    auto edges = UseAllEdges.getValue() ? baseShape.getSubTopoShapes(TopAbs_EDGE)
                                        : getContinuousEdges(baseShape);
    if (edges.empty()) {
        return new App::DocumentObjectExecReturn(
            QT_TRANSLATE_NOOP("Exception", "Fillet not possible on selected shapes"));
    }

    double radius = Radius.getValue();

    if (radius <= 0) {
        return new App::DocumentObjectExecReturn(
            QT_TRANSLATE_NOOP("Exception", "Fillet radius must be greater than zero"));
    }

    this->positionByBaseFeature();

    try {
        TopoShape shape(0);  //,getDocument()->getStringHasher());
        shape.makeElementFillet(baseShape, edges, Radius.getValue(), Radius.getValue());
        if (shape.isNull()) {
            return new App::DocumentObjectExecReturn(
                QT_TRANSLATE_NOOP("Exception", "Resulting shape is null"));
        }

        TopTools_ListOfShape aLarg;
        aLarg.Append(baseShape.getShape());
        bool failed = false;
        if (!BRepAlgo::IsValid(aLarg, shape.getShape(), Standard_False, Standard_False)) {
            ShapeFix_ShapeTolerance aSFT;
            aSFT.LimitTolerance(shape.getShape(),
                                Precision::Confusion(),
                                Precision::Confusion(),
                                TopAbs_SHAPE);
        }

        if (!failed) {
            // store shape before refinement
            this->rawShape = shape;
            shape = refineShapeIfActive(shape);
            shape = getSolid(shape);
        }
        if (!isSingleSolidRuleSatisfied(shape.getShape())) {
            return new App::DocumentObjectExecReturn(QT_TRANSLATE_NOOP("Exception", "Result has multiple solids: that is not currently supported."));
        }
        this->Shape.setValue(shape);

        if (failed) {
            return new App::DocumentObjectExecReturn("Resulting shape is invalid");
        }
        return App::DocumentObject::StdReturn;
    }
    catch (Standard_Failure& e) {
        return new App::DocumentObjectExecReturn(e.GetMessageString());
    }
}

void Fillet::Restore(Base::XMLReader &reader)
{
    DressUp::Restore(reader);
}

void Fillet::handleChangedPropertyType(Base::XMLReader &reader, const char * TypeName, App::Property * prop)
{
    if (prop && strcmp(TypeName,"App::PropertyFloatConstraint") == 0 &&
        strcmp(prop->getTypeId().getName(), "App::PropertyQuantityConstraint") == 0) {
        App::PropertyFloatConstraint p;
        p.Restore(reader);
        static_cast<App::PropertyQuantityConstraint*>(prop)->setValue(p.getValue());
    }
    else {
        DressUp::handleChangedPropertyType(reader, TypeName, prop);
    }
}
