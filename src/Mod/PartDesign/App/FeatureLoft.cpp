/***************************************************************************
 *   Copyright (c) 2015 Stefan Tröger <stefantroeger@gmx.net>              *
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



// NOLINTNEXTLINE(CppUnusedIncludeDirective)
#include "PreCompiled.h"    // NOLINT(misc-include-cleaner)
#ifndef _PreComp_
# include <BRepBuilderAPI_Sewing.hxx>
# include <BRepClass3d_SolidClassifier.hxx>
# include <TopoDS.hxx>
# include <Precision.hxx>
#endif

#include <boost/core/ignore_unused.hpp>

#include <App/Document.h>
#include <Base/Exception.h>
#include <Base/Reader.h>
#include <Mod/Part/App/FaceMakerCheese.h>

#include "Mod/Part/App/TopoShapeOpCode.h"

#include "FeatureLoft.h"
using namespace PartDesign;

PROPERTY_SOURCE(PartDesign::Loft, PartDesign::ProfileBased)

Loft::Loft()
{
    ADD_PROPERTY_TYPE(Sections,(nullptr),"Loft",App::Prop_None,"List of sections");
    Sections.setValue(nullptr);
    ADD_PROPERTY_TYPE(Ruled,(false),"Loft",App::Prop_None,"Create ruled surface");
    ADD_PROPERTY_TYPE(Closed,(false),"Loft",App::Prop_None,"Close Last to First Profile");
}

short Loft::mustExecute() const
{
    if (Sections.isTouched())
        return 1;
    if (Ruled.isTouched())
        return 1;
    if (Closed.isTouched())
        return 1;

    return ProfileBased::mustExecute();
}

std::vector<Part::TopoShape>
Loft::getSectionShape(const char *name,
                      App::DocumentObject *obj,
                      const std::vector<std::string> &subs,
                      size_t expected_size)
{
    std::vector<TopoShape> shapes;
    // Be smart. If part of a sketch is selected, use the entire sketch unless it is a single vertex - 
    // backward compatibility (#16630)
    auto subName = subs.empty() ? "" : subs.front();
    auto useEntireSketch = obj->isDerivedFrom(Part::Part2DObject::getClassTypeId()) &&  subName.find("Vertex") != 0;
    if (subs.empty() || std::find(subs.begin(), subs.end(), std::string()) != subs.end() || useEntireSketch ) {
        shapes.push_back(Part::Feature::getTopoShape(obj));
        if (shapes.back().isNull())
            FC_THROWM(Part::NullShapeException, "Failed to get shape of "
                          << name << " " << App::SubObjectT(obj, "").getSubObjectFullName(obj->getDocument()->getName()));
    } else {
        for (const auto &sub : subs) {
            shapes.push_back(Part::Feature::getTopoShape(obj, sub.c_str(), /*needSubElement*/true));
            if (shapes.back().isNull())
                FC_THROWM(Part::NullShapeException, "Failed to get shape of " << name << " "
                                                                              << App::SubObjectT(obj, sub.c_str()).getSubObjectFullName(obj->getDocument()->getName()));
        }
    }
    auto compound = TopoShape(0).makeElementCompound(shapes, "", TopoShape::SingleShapeCompoundCreationPolicy::returnShape);
    auto wires = compound.getSubTopoShapes(TopAbs_WIRE);
    auto edges = compound.getSubTopoShapes(TopAbs_EDGE, TopAbs_WIRE); // get free edges and make wires from it
    if ( ! edges.empty()) {
        auto extra = TopoShape(0).makeElementWires(edges).getSubTopoShapes(TopAbs_WIRE);
        wires.insert(wires.end(), extra.begin(), extra.end());
    }
    const char *msg = "Sections need to have the same amount of wires or vertices as the base section";
    if (!wires.empty()) {
        if (expected_size && expected_size != wires.size())
            FC_THROWM(Base::CADKernelError, msg);
        return wires;
    }
    auto vertices = compound.getSubTopoShapes(TopAbs_VERTEX);
    if (vertices.empty())
        FC_THROWM(Base::CADKernelError, "Invalid " << name << " shape, expecting either wires or vertices");
    if (expected_size && expected_size != vertices.size())
        FC_THROWM(Base::CADKernelError, msg);
    return vertices;
}

App::DocumentObjectExecReturn *Loft::execute()
{
    if (onlyHasToRefine()){
        TopoShape result = refineShapeIfActive(rawShape);
        Shape.setValue(result);
        return App::DocumentObject::StdReturn;
    }

    std::vector<TopoShape> wires;
    try {
        wires = getSectionShape("Profile", Profile.getValue(), Profile.getSubValues());
    } catch (const Base::Exception& e) {
        return new App::DocumentObjectExecReturn(e.what());
    }

    // if the Base property has a valid shape, fuse the pipe into it
    TopoShape base;
    try {
        base = getBaseTopoShape();
    } catch (const Base::Exception&) {
    }

    auto hasher = getDocument()->getStringHasher();

    try {
        // setup the location
        this->positionByPrevious();
        auto invObjLoc = this->getLocation().Inverted();
        if(!base.isNull())
            base.move(invObjLoc);

        // build up multisections
        auto multisections = Sections.getSubListValues();
        if(multisections.empty())
            return new App::DocumentObjectExecReturn(QT_TRANSLATE_NOOP("Exception", "Loft: At least one section is needed"));

        std::vector<std::vector<TopoShape>> wiresections;
        wiresections.reserve(wires.size());
        for(auto& wire : wires)
            wiresections.emplace_back(1, wire);

        for (const auto &subSet : multisections) {
            int i=0;
            for (const auto &s : getSectionShape("Section", subSet.first, subSet.second, wiresections.size()))
                wiresections[i++].push_back(s);
        }

        bool closed = Closed.getValue();
        // invalid for less then 3 sections
        if (multisections.size() < 2) {
            closed = false;
        }

        TopoShape result(0,hasher);
        std::vector<TopoShape> shapes;

        // build all shells
        std::vector<TopoShape> shells;
        for (auto &sectionWires : wiresections) {
            for(auto& wire : sectionWires)
                wire.move(invObjLoc);
            shells.push_back(TopoShape(0, hasher).makeElementLoft(
                sectionWires, Part::IsSolid::notSolid, Ruled.getValue()? Part::IsRuled::ruled : Part::IsRuled::notRuled, closed ? Part::IsClosed::closed : Part::IsClosed::notClosed));
        }

        // build the top and bottom face, sew the shell and build the final solid
        TopoShape front;
        if (wiresections[0].front().shapeType() != TopAbs_VERTEX) {
            front = getTopoShapeVerifiedFace();
            if (front.isNull())
                return new App::DocumentObjectExecReturn(
                    QT_TRANSLATE_NOOP("Exception", "Loft: Creating a face from sketch failed"));
            front.move(invObjLoc);
        }

        TopoShape back;
        if (wiresections[0].back().shapeType() != TopAbs_VERTEX) {
            std::vector<TopoShape> backwires;
            for(auto& sectionWires : wiresections)
                backwires.push_back(sectionWires.back());
            back = TopoShape(0).makeElementFace(backwires);
        }

        if (!front.isNull() || !back.isNull()) {
            BRepBuilderAPI_Sewing sewer;
            sewer.SetTolerance(Precision::Confusion());
            if (!front.isNull())
                sewer.Add(front.getShape());
            if (!back.isNull())
                sewer.Add(back.getShape());
            for(auto& s : shells)
                sewer.Add(s.getShape());

            sewer.Perform();

            if (!front.isNull())
                shells.push_back(front);
            if (!back.isNull())
                shells.push_back(back);
            // equivalent of the removed: result = result.makeElementShape(sewer,shells);
            result = result.makeShapeWithElementMap(sewer.SewedShape(), Part::MapperSewing(sewer), shells, Part::OpCodes::Sewing);
        }

        if(!result.countSubShapes(TopAbs_SHELL))
            return new App::DocumentObjectExecReturn(QT_TRANSLATE_NOOP("Exception", "Loft: Failed to create shell"));
        shapes = result.getSubTopoShapes(TopAbs_SHELL);

        for (auto &s : shapes) {
            // build the solid
            s = s.makeElementSolid();
            BRepClass3d_SolidClassifier SC(s.getShape());
            SC.PerformInfinitePoint(Precision::Confusion());
            if ( SC.State() == TopAbs_IN)
                s.setShape(s.getShape().Reversed(),false);
        }

        AddSubShape.setValue(result.makeElementCompound(shapes, nullptr, Part::TopoShape::SingleShapeCompoundCreationPolicy::returnShape));

        if (shapes.size() > 1)
            result.makeElementFuse(shapes);
        else
            result = shapes.front();

        if(base.isNull()) {
            Shape.setValue(getSolid(result));
            return App::DocumentObject::StdReturn;
        }

        result.Tag = -getID();
        TopoShape boolOp(0,getDocument()->getStringHasher());

        const char *maker;
        switch(getAddSubType()) {
            case Additive:
                maker = Part::OpCodes::Fuse;
                break;
            case Subtractive:
                maker = Part::OpCodes::Cut;
                break;
            default:
                return new App::DocumentObjectExecReturn(QT_TRANSLATE_NOOP("Exception", "Unknown operation type"));
        }
        try {
            boolOp.makeElementBoolean(maker, {base,result});
        }
        catch(Standard_Failure&) {
            return new App::DocumentObjectExecReturn(QT_TRANSLATE_NOOP("Exception", "Failed to perform boolean operation"));
        }
        boolOp = this->getSolid(boolOp);
        // lets check if the result is a solid
        if (boolOp.isNull())
            return new App::DocumentObjectExecReturn(QT_TRANSLATE_NOOP("Exception", "Resulting shape is not a solid"));

        // store shape before refinement
        this->rawShape = boolOp;
        boolOp = refineShapeIfActive(boolOp);
        boolOp = getSolid(boolOp);
        if (!isSingleSolidRuleSatisfied(boolOp.getShape())) {
            return new App::DocumentObjectExecReturn(QT_TRANSLATE_NOOP("Exception", "Result has multiple solids: that is not currently supported."));
        }
        Shape.setValue(boolOp);
        return App::DocumentObject::StdReturn;
    }
    catch (Standard_Failure& e) {
        return new App::DocumentObjectExecReturn(e.GetMessageString());
    }
    catch (const Base::Exception& e) {
        return new App::DocumentObjectExecReturn(e.what());
    }
    catch (...) {
        return new App::DocumentObjectExecReturn(QT_TRANSLATE_NOOP("Exception", "Loft: A fatal error occurred when making the loft"));
    }
}

PROPERTY_SOURCE(PartDesign::AdditiveLoft, PartDesign::Loft)
AdditiveLoft::AdditiveLoft() {
    addSubType = Additive;
}

PROPERTY_SOURCE(PartDesign::SubtractiveLoft, PartDesign::Loft)
SubtractiveLoft::SubtractiveLoft() {
    addSubType = Subtractive;
}

void Loft::handleChangedPropertyType(Base::XMLReader& reader, const char* TypeName, App::Property* prop)
{
    // property Sections had the App::PropertyLinkList and was changed to App::PropertyXLinkSubList
    if (prop == &Sections && strcmp(TypeName, "App::PropertyLinkList") == 0) {
        Sections.upgrade(reader, TypeName);
    }
    else {
        ProfileBased::handleChangedPropertyType(reader, TypeName, prop);
    }
}
