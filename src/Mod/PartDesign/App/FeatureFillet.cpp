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
    Part::TopoShape TopShape;
    try {
        TopShape = getBaseShape();
    }
    catch (Base::Exception& e) {
        return new App::DocumentObjectExecReturn(e.what());
    }
    std::vector<std::string> SubNames = std::vector<std::string>(Base.getSubValues());

    if (UseAllEdges.getValue()){
        SubNames.clear();
        std::string edgeTypeName = Part::TopoShape::shapeName(TopAbs_EDGE); //"Edge"
        int count = TopShape.countSubElements(edgeTypeName.c_str());
        for (int ii = 0; ii < count; ii++){
            std::ostringstream edgeName;
            edgeName << edgeTypeName << ii+1;
            SubNames.push_back(edgeName.str());
        }
    }

    getContinuousEdges(TopShape, SubNames);

    double radius = Radius.getValue();

    if(radius <= 0)
        return new App::DocumentObjectExecReturn(QT_TRANSLATE_NOOP("Exception", "Fillet radius must be greater than zero"));

    this->positionByBaseFeature();

    //If no element is selected, then we use a copy of previous feature.
    if (SubNames.empty()) {
        this->Shape.setValue(TopShape);
        return App::DocumentObject::StdReturn;
    }

    // create an untransformed copy of the base shape
    Part::TopoShape baseShape(TopShape);
    baseShape.setTransform(Base::Matrix4D());
    try {
        BRepFilletAPI_MakeFillet mkFillet(baseShape.getShape());

        for (const auto & it : SubNames) {
            TopoDS_Edge edge = TopoDS::Edge(baseShape.getSubShape(it.c_str()));
            mkFillet.Add(radius, edge);
        }

        mkFillet.Build();
        if (!mkFillet.IsDone())
            return new App::DocumentObjectExecReturn(QT_TRANSLATE_NOOP("Exception", "Failed to create fillet"));

        TopoDS_Shape shape = mkFillet.Shape();
        if (shape.IsNull())
            return new App::DocumentObjectExecReturn(QT_TRANSLATE_NOOP("Exception", "Resulting shape is null"));

        TopTools_ListOfShape aLarg;
        aLarg.Append(baseShape.getShape());
        if (!BRepAlgo::IsValid(aLarg, shape, Standard_False, Standard_False)) {
            ShapeFix_ShapeTolerance aSFT;
            aSFT.LimitTolerance(shape, Precision::Confusion(), Precision::Confusion(), TopAbs_SHAPE);
            Handle(ShapeFix_Shape) aSfs = new ShapeFix_Shape(shape);
            aSfs->Perform();
            shape = aSfs->Shape();
            if (!BRepAlgo::IsValid(aLarg, shape, Standard_False, Standard_False)) {
                return new App::DocumentObjectExecReturn(QT_TRANSLATE_NOOP("Exception", "Resulting shape is invalid"));
            }
        }

        int solidCount = countSolids(shape);
        if (solidCount > 1) {
            return new App::DocumentObjectExecReturn(QT_TRANSLATE_NOOP("Exception", "Result has multiple solids: that is not currently supported."));
        }

        shape = refineShapeIfActive(shape);
        this->Shape.setValue(getSolid(shape));
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
