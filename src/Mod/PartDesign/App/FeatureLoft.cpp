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


#include "PreCompiled.h"
#ifndef _PreComp_
# include <TopoDS_Wire.hxx>
# include <TopExp_Explorer.hxx>
# include <BRepAlgoAPI_Cut.hxx>
# include <BRepAlgoAPI_Fuse.hxx>
# include <BRepBuilderAPI_MakeSolid.hxx>
# include <BRepBuilderAPI_Sewing.hxx>
# include <BRepClass3d_SolidClassifier.hxx>
# include <BRepOffsetAPI_ThruSections.hxx>
# include <TopoDS.hxx>
# include <Precision.hxx>
#endif

#include <boost/core/ignore_unused.hpp>

#include <App/Document.h>
#include <Base/Exception.h>
#include <Base/Reader.h>
#include <Mod/Part/App/FaceMakerCheese.h>

#include "FeatureLoft.h"
#include "Mod/Part/App/TopoShapeOpCode.h"


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

#ifndef FC_USE_TNP_FIX
App::DocumentObjectExecReturn *Loft::execute()
{
    auto getSectionShape =
        [](App::DocumentObject* feature, const std::vector<std::string> &subs) -> TopoDS_Shape {
            if (!feature ||
                !feature->isDerivedFrom(Part::Feature::getClassTypeId()))
                throw Base::TypeError("Loft: Invalid profile/section");

            auto subName = subs.empty() ? "" : subs.front();

            // only take the entire shape when we have a sketch selected, but
            // not a point of the sketch
            if (feature->isDerivedFrom(Part::Part2DObject::getClassTypeId()) &&
                subName.compare(0, 6, "Vertex") != 0)
                return static_cast<Part::Part2DObject*>(feature)->Shape.getValue();
            else {
                if(subName.empty())
                    throw Base::ValueError("No valid subelement linked in Part::Feature");
                return static_cast<Part::Feature*>(feature)->Shape.getShape().getSubShape(subName.c_str());
            }
        };

    auto addWiresToWireSections =
        [](TopoDS_Shape& section,
           std::vector<std::vector<TopoDS_Shape>>& wiresections) -> size_t {
            TopExp_Explorer ex;
            size_t i=0;
            bool initialWireSectionsEmpty = wiresections.empty();
            for (ex.Init(section, TopAbs_WIRE); ex.More(); ex.Next(), ++i) {
                // if profile was just a point then this is where we can first set our list
                if (i>=wiresections.size()) {
                    if (initialWireSectionsEmpty)
                        wiresections.emplace_back(1, ex.Current());
                    else
                        throw Base::ValueError("Loft: Sections need to have the same amount of inner wires (except profile and last section, which can be points)");
                }
                else
                    wiresections[i].push_back(TopoDS::Wire(ex.Current()));
            }
            return i;
        };

    std::vector<TopoDS_Wire> wires;
    TopoDS_Shape profilePoint;

    // if the Base property has a valid shape, fuse the pipe into it
    TopoDS_Shape base;
    try {
        base = getBaseShape();
    } catch (const Base::Exception&) {
        base = TopoDS_Shape();
    }

    try {
        // setup the location
        this->positionByPrevious();
        TopLoc_Location invObjLoc = this->getLocation().Inverted();
        if (!base.IsNull())
            base.Move(invObjLoc);

        // build up multisections
        auto multisections = Sections.getSubListValues();
        if (multisections.empty())
            return new App::DocumentObjectExecReturn(QT_TRANSLATE_NOOP("Exception", "Loft: At least one section is needed"));

        TopoDS_Shape profileShape = getSectionShape(Profile.getValue(),
                                                    Profile.getSubValues());
        if (profileShape.IsNull())
            return new App::DocumentObjectExecReturn(QT_TRANSLATE_NOOP("Exception", "Loft: Could not obtain profile shape"));

        std::vector<std::vector<TopoDS_Shape>> wiresections;

        size_t numWires = addWiresToWireSections(profileShape, wiresections);
        if (numWires == 0) {
            // profileShape had no wires so only other valid option is point section
            TopExp_Explorer ex;
            size_t i = 0;
            for (ex.Init(profileShape, TopAbs_VERTEX); ex.More(); ex.Next(), ++i) {
                profilePoint = ex.Current();
            }
            if (i > 1)
                return new App::DocumentObjectExecReturn(QT_TRANSLATE_NOOP("Exception", "Loft: When using points for profile/sections, the sketch should have a single point"));
        }

        bool isLastSectionVertex = false;

        size_t subSetCnt=0;
        for (const auto & subSet : multisections) {
            if (!subSet.first->isDerivedFrom(Part::Feature::getClassTypeId()))
                return new App::DocumentObjectExecReturn(QT_TRANSLATE_NOOP("Exception", "Loft: All sections need to be part features"));

            // if the selected subvalue is a point, pick that even if we have a sketch
            TopoDS_Shape shape = getSectionShape(subSet.first, subSet.second);
            if (shape.IsNull())
                return new App::DocumentObjectExecReturn(QT_TRANSLATE_NOOP("Exception", "Loft: Could not obtain section shape"));

            size_t numWiresAdded = addWiresToWireSections(shape, wiresections);
            if (numWiresAdded == 0) {
                // The shape of the given object doesn't contain any wires, though it still might be valid if it is a vertex (or a COMPOUND consisting a single vertex)
                TopoDS_Shape vertexShape;
                TopExp_Explorer ex{shape, TopAbs_VERTEX};
                if (ex.More()) {
                    vertexShape = ex.Current();
                    ex.Next();
                    if (ex.More()) { // some additional vertexes in the shape, we shouldn't use it
                        vertexShape.Nullify();
                    }
                }

                if (vertexShape.IsNull())
                    return new App::DocumentObjectExecReturn(QT_TRANSLATE_NOOP("Exception", "Loft: A section doesn't contain any wires nor is a single vertex"));
                if (subSetCnt != multisections.size()-1)
                    return new App::DocumentObjectExecReturn(QT_TRANSLATE_NOOP("Exception", "Loft: Only the profile and the last section can be vertices"));
                if (Closed.getValue())
                    return new App::DocumentObjectExecReturn(QT_TRANSLATE_NOOP("Exception", "Loft: For closed lofts only the profile can be a vertex"));

                // all good; push vertex to all wiresection list
                for (auto &wires : wiresections)
                    wires.push_back(vertexShape);
                isLastSectionVertex = true;
            } else if (numWiresAdded != wiresections.size())
                return new App::DocumentObjectExecReturn(QT_TRANSLATE_NOOP("Exception", "Loft: all loft sections need to have the same amount of inner wires"));
            subSetCnt++;
        }

        if (Closed.getValue()) {
            // For a closed loft add starting sketch again at the end
            if (profilePoint.IsNull()) {
                size_t numWiresAdded = addWiresToWireSections(profileShape, wiresections);
                assert (numWiresAdded == wiresections.size());
                boost::ignore_unused(numWiresAdded);
            } else { // !profilePoint.IsNull()
                for (auto &wires : wiresections)
                    wires.push_back(profilePoint);
            }
        }

        // build all shells
        std::vector<TopoDS_Shape> shells;

        TopoDS_Shape copyProfilePoint(profilePoint);
        if (!profilePoint.IsNull())
            copyProfilePoint.Move(invObjLoc);

        for (auto& wires : wiresections) {
            BRepOffsetAPI_ThruSections mkTS(false, Ruled.getValue(), Precision::Confusion());

            if (!profilePoint.IsNull())
                mkTS.AddVertex(TopoDS::Vertex(copyProfilePoint));

            for (auto& shape : wires) {
                shape.Move(invObjLoc);
                if (shape.ShapeType() == TopAbs_VERTEX)
                    mkTS.AddVertex(TopoDS::Vertex(shape));
                else
                    mkTS.AddWire(TopoDS::Wire(shape));
            }

            mkTS.Build();
            if (!mkTS.IsDone())
                return new App::DocumentObjectExecReturn(QT_TRANSLATE_NOOP("Exception", "Loft could not be built"));

            // build the shell use simulate to get the top and bottom wires in an easy way
            shells.push_back(mkTS.Shape());
        }

        // build the top and bottom faces (where possible), sew the shell,
        // and build the final solid
        BRepBuilderAPI_Sewing sewer;
        sewer.SetTolerance(Precision::Confusion());
        if (!Closed.getValue()) {
            if (profilePoint.IsNull()) {
                TopoDS_Shape front = getVerifiedFace();
                front.Move(invObjLoc);
                sewer.Add(front);
            }
            if (!isLastSectionVertex) {
                std::vector<TopoDS_Wire> backwires;
                for (auto& wires : wiresections)
                    backwires.push_back(TopoDS::Wire(wires.back()));
                TopoDS_Shape back = Part::FaceMakerCheese::makeFace(backwires);
                sewer.Add(back);
            }
        }
        for (TopoDS_Shape& s : shells)
            sewer.Add(s);

        sewer.Perform();

        // build the solid
        BRepBuilderAPI_MakeSolid mkSolid;
        mkSolid.Add(TopoDS::Shell(sewer.SewedShape()));
        if (!mkSolid.IsDone())
            return new App::DocumentObjectExecReturn(QT_TRANSLATE_NOOP("Exception", "Loft: Result is not a solid"));

        TopoDS_Shape result = mkSolid.Shape();
        BRepClass3d_SolidClassifier SC(result);
        SC.PerformInfinitePoint(Precision::Confusion());
        if ( SC.State() == TopAbs_IN) {
            result.Reverse();
        }

        AddSubShape.setValue(result);

        if (base.IsNull()) {
            if (getAddSubType() == FeatureAddSub::Subtractive)
                return new App::DocumentObjectExecReturn(QT_TRANSLATE_NOOP("Exception", "Loft: There is nothing to subtract from"));

            Shape.setValue(getSolid(result));
            return App::DocumentObject::StdReturn;
        }

        if (getAddSubType() == FeatureAddSub::Additive) {

            BRepAlgoAPI_Fuse mkFuse(base, result);
            if (!mkFuse.IsDone())
                return new App::DocumentObjectExecReturn(QT_TRANSLATE_NOOP("Exception", "Loft: Adding the loft failed"));
                
            // we have to get the solids (fuse sometimes creates compounds)
            TopoDS_Shape boolOp = this->getSolid(mkFuse.Shape());

            // lets check if the result is a solid
            if (boolOp.IsNull())
                return new App::DocumentObjectExecReturn(QT_TRANSLATE_NOOP("Exception", "Resulting shape is not a solid"));

            if (!isSingleSolidRuleSatisfied(boolOp)) {
                return new App::DocumentObjectExecReturn(QT_TRANSLATE_NOOP("Exception", "Result has multiple solids: that is not currently supported."));
            }

            boolOp = refineShapeIfActive(boolOp);
            Shape.setValue(getSolid(boolOp));
        }
        else if (getAddSubType() == FeatureAddSub::Subtractive) {

            BRepAlgoAPI_Cut mkCut(base, result);
            if (!mkCut.IsDone())
                return new App::DocumentObjectExecReturn(QT_TRANSLATE_NOOP("Exception", "Loft: Subtracting the loft failed"));

            // we have to get the solids (fuse sometimes creates compounds)
            TopoDS_Shape boolOp = this->getSolid(mkCut.Shape());

            // lets check if the result is a solid
            if (boolOp.IsNull())
                return new App::DocumentObjectExecReturn(QT_TRANSLATE_NOOP("Exception", "Resulting shape is not a solid"));

            if (!isSingleSolidRuleSatisfied(boolOp)) {
                return new App::DocumentObjectExecReturn(QT_TRANSLATE_NOOP("Exception", "Result has multiple solids: that is not currently supported."));
            }

            boolOp = refineShapeIfActive(boolOp);
            Shape.setValue(getSolid(boolOp));
        }

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
#else
std::vector<Part::TopoShape>
Loft::getSectionShape(const char *name,
                      App::DocumentObject *obj,
                      const std::vector<std::string> &subs,
                      size_t expected_size)
{
    std::vector<TopoShape> shapes;
    if (subs.empty() || std::find(subs.begin(), subs.end(), std::string()) != subs.end()) {
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
    auto compound = TopoShape().makeElementCompound(shapes, "", TopoShape::SingleShapeCompoundCreationPolicy::returnShape);
    auto wires = compound.getSubTopoShapes(TopAbs_WIRE);
    auto edges = compound.getSubTopoShapes(TopAbs_EDGE, TopAbs_WIRE); // get free edges and make wires from it
    if (edges.size()) {
        auto extra = TopoShape().makeElementWires(edges).getSubTopoShapes(TopAbs_WIRE);
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

App::DocumentObjectExecReturn *Loft::execute(void)
{
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
        //setup the location
        this->positionByPrevious();
        auto invObjLoc = this->getLocation().Inverted();
        if(!base.isNull())
            base.move(invObjLoc);

        //build up multisections
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

        TopoShape result(0,hasher);
        std::vector<TopoShape> shapes;

//        if (SplitProfile.getValue()) {
//            for (auto &wires : wiresections) {
//                for(auto& wire : wires)
//                    wire.move(invObjLoc);
//                shapes.push_back(TopoShape(0).makeElementLoft(
//                    wires, Part::IsSolid::solid, Ruled.getValue(), Closed.getValue()));
//            }
//        } else {
            //build all shells
            std::vector<TopoShape> shells;
            for (auto &wires : wiresections) {
                for(auto& wire : wires)
                    wire.move(invObjLoc);
                shells.push_back(TopoShape(0, hasher).makeElementLoft(
                    wires, Part::IsSolid::notSolid, Ruled.getValue()? Part::IsRuled::ruled : Part::IsRuled::notRuled, Closed.getValue() ? Part::IsClosed::closed : Part::IsClosed::notClosed));
//            }

            //build the top and bottom face, sew the shell and build the final solid
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
                for(auto& wires : wiresections)
                    backwires.push_back(wires.back());
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
//                result = result.makeElementShape(sewer,shells);
                result = result.makeShapeWithElementMap(sewer.SewedShape(), Part::MapperSewing(sewer), shells, Part::OpCodes::Sewing);
            }

            if(!result.countSubShapes(TopAbs_SHELL))
                return new App::DocumentObjectExecReturn(QT_TRANSLATE_NOOP("Exception", "Loft: Failed to create shell"));
            shapes = result.getSubTopoShapes(TopAbs_SHELL);
        }

        for (auto &s : shapes) {
            //build the solid
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
//            case Intersecting:
//                maker = Part::OpCodes::Common;
//                break;
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

#endif

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
