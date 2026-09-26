// SPDX-License-Identifier: LGPL-2.1-or-later

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

#include <limits>

#include <BRepAlgo.hxx>
#include <BRepFilletAPI_MakeFillet.hxx>
#include <BRep_Tool.hxx>
#include <Geom_Circle.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Edge.hxx>
#include <TopExp_Explorer.hxx>
#include <TopTools_ListOfShape.hxx>
#include <ShapeFix_Shape.hxx>
#include <ShapeFix_ShapeTolerance.hxx>

#include <Base/Exception.h>
#include <Base/Reader.h>
#include <Mod/Part/App/TopoShape.h>

#include "FeatureFillet.h"


using namespace PartDesign;


PROPERTY_SOURCE(PartDesign::Fillet, PartDesign::DressUp)

const char* Fillet::SelectionTypeEnums[]
    = {"Selected edges & faces", "Selected solids", "All solids", nullptr};

const App::PropertyQuantityConstraint::Constraints floatRadius
    = {0.0, std::numeric_limits<float>::max(), 0.1};

Fillet::Fillet()
{
    ADD_PROPERTY_TYPE(Radius, (1.0), "Fillet", App::Prop_None, "Fillet radius.");
    Radius.setUnit(Base::Unit::Length);
    Radius.setConstraints(&floatRadius);

    // TODO: Remove UseAllEdges property
    ADD_PROPERTY_TYPE(
        UseAllEdges,
        (false),
        "Fillet",
        App::Prop_Hidden,
        "Fillet all edges if true, else use only those edges in Base property.\n"
        "If true, then this overrides any edge changes made to the Base property or in the "
        "dialog.\n"
    );

    ADD_PROPERTY_TYPE(SelectionType, (0L), "Fillet", App::Prop_None, "Selection Type");
    SelectionType.setEnums(SelectionTypeEnums);
}

short Fillet::mustExecute() const
{
    if (Placement.isTouched() || Radius.isTouched() || SelectionType.isTouched()) {
        return 1;
    }
    return DressUp::mustExecute();
}

App::DocumentObjectExecReturn* Fillet::execute()
{
    if (onlyHaveRefined()) {
        return App::DocumentObject::StdReturn;
    }

    // NOTE: Normally the Base property and the BaseFeature property should point to the same object.
    // The only difference is that the Base property also stores the edges that are to be filleted.
    Part::TopoShape TopShape;
    try {
        TopShape = getBaseTopoShape();
    }
    catch (Base::Exception& e) {
        return new App::DocumentObjectExecReturn(e.what());
    }

    TopShape.setTransform(Base::Matrix4D());

    std::vector<TopoShape> edges;

    switch (static_cast<SelectionMode>(SelectionType.getValue())) {
        case SelectionMode::SelectedEdges: {
            edges = getContinuousEdges(TopShape);
            break;
        }

        case SelectionMode::SelectedSolids: {
            for (const std::string& ref : Base.getSubValues()) {
                const TopoDS_Shape solid = TopShape.getSubShape(ref.c_str(), true);

                if (solid.IsNull()) {
                    continue;
                }

                for (TopExp_Explorer exp(solid, TopAbs_EDGE); exp.More(); exp.Next()) {
                    edges.emplace_back(exp.Current());
                }
            }
            break;
        }

        case SelectionMode::AllSolids: {
            edges = TopShape.getSubTopoShapes(TopAbs_EDGE);
            break;
        }
    }

    if (edges.empty()) {
        return new App::DocumentObjectExecReturn(
            QT_TRANSLATE_NOOP("Exception", "Fillet not possible on selected shapes")
        );
    }

    const double radius = Radius.getValue();

    if (radius <= 0) {
        return new App::DocumentObjectExecReturn(
            QT_TRANSLATE_NOOP("Exception", "Fillet radius must be greater than zero")
        );
    }

    this->positionByBaseFeature();

    try {
        TopoShape shape(0);

#if defined(__GNUC__) && defined(FC_OS_LINUX)
        Base::SignalException se;
#endif

        shape.makeElementFillet(TopShape, edges, radius, radius);

        if (shape.isNull()) {
            return new App::DocumentObjectExecReturn(
                QT_TRANSLATE_NOOP("Exception", "Resulting shape is null")
            );
        }

        TopTools_ListOfShape aLarg;
        aLarg.Append(TopShape.getShape());

        if (!BRepAlgo::IsValid(aLarg, shape.getShape(), Standard_False, Standard_False)) {
            ShapeFix_ShapeTolerance aSFT;
            aSFT.LimitTolerance(
                shape.getShape(),
                Precision::Confusion(),
                Precision::Confusion(),
                TopAbs_SHAPE
            );
        }

        // Store shape before refinement.
        this->rawShape = shape;

        shape = refineShapeIfActive(shape);

        if (!isSingleSolidRuleSatisfied(shape.getShape())) {
            return new App::DocumentObjectExecReturn(QT_TRANSLATE_NOOP(
                "Exception",
                "Result has multiple solids: enable 'Allow Compound' in the active body."
            ));
        }

        shape = getSolid(shape);
        this->Shape.setValue(shape);

        return App::DocumentObject::StdReturn;
    }
    catch (Base::Exception& e) {
        return new App::DocumentObjectExecReturn(e.what());
    }
    catch (Standard_Failure& e) {
        return new App::DocumentObjectExecReturn(e.GetMessageString());
    }
    catch (...) {
        return new App::DocumentObjectExecReturn(QT_TRANSLATE_NOOP(
            "Exception",
            "Fillet operation failed. The selected edges may contain geometry that cannot be "
            "filleted together. "
            "Try filleting edges individually or with a smaller radius."
        ));
    }
}

void Fillet::Restore(Base::XMLReader& reader)
{
    DressUp::Restore(reader);

    if (UseAllEdges.getValue()) {
        SelectionType.setValue(SelectionMode::AllSolids);
    }
}

void Fillet::handleChangedPropertyType(Base::XMLReader& reader, const char* TypeName, App::Property* prop)
{
    if (prop && strcmp(TypeName, "App::PropertyFloatConstraint") == 0
        && prop->getTypeId().getName() == "App::PropertyQuantityConstraint") {
        App::PropertyFloatConstraint p;
        p.Restore(reader);
        static_cast<App::PropertyQuantityConstraint*>(prop)->setValue(p.getValue());
    }
    else {
        DressUp::handleChangedPropertyType(reader, TypeName, prop);
    }
}
