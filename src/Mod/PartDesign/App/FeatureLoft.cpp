// SPDX-License-Identifier: LGPL-2.1-or-later

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

#include <BRepBuilderAPI_Sewing.hxx>
#include <BRepClass3d_SolidClassifier.hxx>
#include <Geom_BSplineSurface.hxx>
#include <TopoDS.hxx>
#include <Precision.hxx>


#include <boost/core/ignore_unused.hpp>

#include <App/Application.h>
#include <App/Document.h>
#include <Base/Console.h>
#include <Base/Exception.h>
#include <Base/Reader.h>
#include <Mod/Part/App/FaceMakerCheese.h>

#include "Mod/Part/App/TopoShapeOpCode.h"

#include "FeatureLoft.h"
using namespace PartDesign;

PROPERTY_SOURCE(PartDesign::Loft, PartDesign::ProfileBased)

namespace
{
Part::Smoothing loftSmoothing(long loftType)
{
    switch (loftType) {
        case 0:
            return Part::Smoothing::bspline;
        case 1:
            return Part::Smoothing::ruled;
        case 2:
            return Part::Smoothing::variational;
        default:
            return Part::Smoothing::bspline;
    }
}
}  // namespace

App::PropertyIntegerConstraint::Constraints Loft::Degrees = {2, Geom_BSplineSurface::MaxDegree(), 1};
const char* Loft::LoftTypeEnums[]
    = {"Standard B-Spline", "Ruled Surface", "Variational B-Spline", nullptr};
const char* Loft::ParametrizationEnums[] = {"Chord length", "Centripetal", "Uniform", nullptr};
const char* Loft::ContinuityEnums[] = {"C0", "C1", "C2", nullptr};

Loft::Loft()
{
    ADD_PROPERTY_TYPE(Sections, (nullptr), "Loft", App::Prop_None, "List of sections");
    Sections.setValue(nullptr);
    ADD_PROPERTY_TYPE(LoftType, (long(0)), "Loft", App::Prop_None, "Loft surface generation algorithm");
    ADD_PROPERTY_TYPE(Ruled, (false), "Compatibility", App::Prop_Hidden, "Deprecated: use LoftType instead");
    ADD_PROPERTY_TYPE(Closed, (false), "Loft", App::Prop_None, "Close Last to First Profile");
    ADD_PROPERTY_TYPE(MaxDegree, (5), "Loft", App::Prop_None, "Maximum B-Spline degree");
    ADD_PROPERTY_TYPE(Parametrization, (long(0)), "Loft", App::Prop_None, "B-Spline parametrization type");
    ADD_PROPERTY_TYPE(Continuity, (long(2)), "Loft", App::Prop_None, "B-Spline continuity");
    ADD_PROPERTY_TYPE(
        CheckCompatibility,
        (true),
        "Loft",
        App::Prop_None,
        "Align profile origins, orientations, and edge counts"
    );
    ADD_PROPERTY_TYPE(
        Adaptive,
        (App::GetApplication().isRestoring() ? false : true),
        "Loft",
        App::Prop_None,
        "Retry failed selected settings with deterministic loft alternatives"
    );
    MaxDegree.setConstraints(&Degrees);
    LoftType.setEnums(LoftTypeEnums);
    Parametrization.setEnums(ParametrizationEnums);
    Continuity.setEnums(ContinuityEnums);
    synchronizingLoftType = false;
}

void Loft::onChanged(const App::Property* prop)
{
    if (!isRestoring() && !synchronizingLoftType) {
        if (prop == &Ruled) {
            Base::Console().warning(
                "The 'Ruled' property of PartDesign lofts is deprecated; use LoftType = "
                "'Standard B-Spline', 'Ruled Surface', or 'Variational B-Spline' instead.\n"
            );
            synchronizingLoftType = true;
            LoftType.setValue(Ruled.getValue() ? 1 : 0);
            synchronizingLoftType = false;
        }
        else if (prop == &LoftType) {
            synchronizingLoftType = true;
            Ruled.setValue(LoftType.getValue() == 1);
            synchronizingLoftType = false;
        }
    }

    ProfileBased::onChanged(prop);
}

void Loft::onDocumentRestored()
{
    // Saved documents store only Ruled true/false
    // This provides backwards compatability
    synchronizingLoftType = true;
    if (Ruled.getValue()) {
        LoftType.setValue(1);
    }
    Ruled.setValue(LoftType.getValue() == 1);
    synchronizingLoftType = false;

    ProfileBased::onDocumentRestored();
}

short Loft::mustExecute() const
{
    if (Sections.isTouched()) {
        return 1;
    }
    if (LoftType.isTouched()) {
        return 1;
    }
    if (Closed.isTouched()) {
        return 1;
    }
    if (MaxDegree.isTouched()) {
        return 1;
    }
    if (Parametrization.isTouched()) {
        return 1;
    }
    if (Continuity.isTouched()) {
        return 1;
    }
    if (CheckCompatibility.isTouched()) {
        return 1;
    }
    if (Adaptive.isTouched()) {
        return 1;
    }

    return ProfileBased::mustExecute();
}

std::vector<Part::TopoShape> Loft::getSectionShape(
    const char* name,
    App::DocumentObject* obj,
    const std::vector<std::string>& subs,
    size_t expected_size
)
{
    auto useSketch = [](App::DocumentObject* obj, const std::vector<std::string>& subs) {
        // Be smart. If part of a sketch is selected, use the entire sketch unless it is a single
        // vertex - backward compatibility (#16630)
        if (!obj) {
            return false;
        }

        auto subName = subs.empty() ? "" : subs.front();
        return obj->isDerivedFrom<Part::Part2DObject>() && subName.find("Vertex") != 0;
    };

    std::vector<TopoShape> shapes;
    auto useEntireSketch = useSketch(obj, subs);
    if (subs.empty() || std::ranges::find(subs, std::string()) != subs.end() || useEntireSketch) {
        shapes.push_back(
            Part::Feature::getTopoShape(obj, Part::ShapeOption::ResolveLink | Part::ShapeOption::Transform)
        );
        if (shapes.back().isNull()) {
            std::stringstream str;
            str << "Failed to get shape of " << name;
            if (obj) {
                auto doc = obj->getDocument();
                str << " " << App::SubObjectT(obj, "").getSubObjectFullName(doc->getName());
            }
            THROWM(Part::NullShapeException, str.str());
        }
    }
    else {
        for (const auto& sub : subs) {
            shapes.push_back(
                Part::Feature::getTopoShape(
                    obj,
                    Part::ShapeOption::NeedSubElement | Part::ShapeOption::ResolveLink
                        | Part::ShapeOption::Transform,
                    sub.c_str()
                )
            );
            if (shapes.back().isNull()) {
                std::stringstream str;
                str << "Failed to get shape of " << name;
                if (obj) {
                    auto doc = obj->getDocument();
                    App::SubObjectT subObj(obj, sub.c_str());
                    str << " " << subObj.getSubObjectFullName(doc->getName());
                }
                THROWM(Part::NullShapeException, str.str());
            }
        }
    }
    auto compound = TopoShape(0).makeElementCompound(
        shapes,
        "",
        TopoShape::SingleShapeCompoundCreationPolicy::returnShape
    );
    auto wires = compound.getSubTopoShapes(TopAbs_WIRE);
    auto edges = compound.getSubTopoShapes(TopAbs_EDGE, TopAbs_WIRE);  // get free edges and make
                                                                       // wires from it
    if (!edges.empty()) {
        auto extra = TopoShape(0).makeElementWires(edges).getSubTopoShapes(TopAbs_WIRE);
        wires.insert(wires.end(), extra.begin(), extra.end());
    }
    const char* msg
        = "Sections need to have the same amount of wires or vertices as the base section";
    if (!wires.empty()) {
        if (expected_size && expected_size != wires.size()) {
            FC_THROWM(Base::CADKernelError, msg);
        }
        return wires;
    }
    auto vertices = compound.getSubTopoShapes(TopAbs_VERTEX);
    if (vertices.empty()) {
        FC_THROWM(
            Base::CADKernelError,
            "Invalid " << name << " shape, expecting either wires or vertices"
        );
    }
    if (expected_size && expected_size != vertices.size()) {
        FC_THROWM(Base::CADKernelError, msg);
    }
    return vertices;
}

App::DocumentObjectExecReturn* Loft::execute()
{
    if (onlyHaveRefined()) {
        return App::DocumentObject::StdReturn;
    }

    std::vector<TopoShape> wires;
    try {
        wires = getSectionShape("Profile", Profile.getValue(), Profile.getSubValues());
    }
    catch (const Base::Exception& e) {
        return new App::DocumentObjectExecReturn(e.what());
    }

    // if the Base property has a valid shape, fuse the pipe into it
    TopoShape base;
    try {
        base = getBaseTopoShape();
    }
    catch (const Base::Exception&) {
    }

    auto hasher = getDocument()->getStringHasher();

    try {
        // setup the location
        this->positionByPrevious();
        auto invObjLoc = this->getLocation().Inverted();
        if (!base.isNull()) {
            base.move(invObjLoc);
        }

        // build up multisections
        auto multisections = Sections.getSubListValues();
        if (multisections.empty()) {
            return new App::DocumentObjectExecReturn(
                QT_TRANSLATE_NOOP("Exception", "Loft: At least one section is needed")
            );
        }

        std::vector<std::vector<TopoShape>> wiresections;
        wiresections.reserve(wires.size());
        for (auto& wire : wires) {
            wiresections.emplace_back(1, wire);
        }

        for (const auto& subSet : multisections) {
            int i = 0;
            for (const auto& s :
                 getSectionShape("Section", subSet.first, subSet.second, wiresections.size())) {
                wiresections[i++].push_back(s);
            }
        }

        bool closed = Closed.getValue();
        // invalid for less then 3 sections
        if (multisections.size() < 2) {
            closed = false;
        }

        TopoShape result(0, hasher);
        std::vector<TopoShape> shapes;

        // build all shells
        std::vector<TopoShape> shells;
        for (auto& sectionWires : wiresections) {
            for (auto& wire : sectionWires) {
                wire.move(invObjLoc);
            }
            const auto smoothing = loftSmoothing(LoftType.getValue());
            shells.push_back(TopoShape(0, hasher).makeElementLoft(
                sectionWires,
                Part::IsSolid::notSolid,
                smoothing,
                closed ? Part::IsClosed::closed : Part::IsClosed::notClosed,
                MaxDegree.getValue(),
                nullptr,
                static_cast<Part::LoftParametrization>(Parametrization.getValue()),
                static_cast<Part::LoftContinuity>(Continuity.getValue()),
                CheckCompatibility.getValue(),
                Adaptive.getValue()
            ));
        }

        // build the top and bottom face, sew the shell and build the final solid
        TopoShape front;
        if (wiresections[0].front().shapeType() != TopAbs_VERTEX) {
            front = getTopoShapeVerifiedFace();
            if (front.isNull()) {
                return new App::DocumentObjectExecReturn(
                    QT_TRANSLATE_NOOP("Exception", "Loft: Creating a face from sketch failed")
                );
            }
            front.move(invObjLoc);
        }

        TopoShape back;
        if (wiresections[0].back().shapeType() != TopAbs_VERTEX) {
            std::vector<TopoShape> backwires;
            for (auto& sectionWires : wiresections) {
                backwires.push_back(sectionWires.back());
            }
            const char* faceMaker[] = {
                "Part::FaceMakerBullseye",
                "Part::FaceMakerCheese",
                "Part::FaceMakerSimple",
                "Part::FaceMakerUnified",
            };
            for (size_t i = 0; i < std::size(faceMaker); i++) {
                try {
                    back = TopoShape(0).makeElementFace(backwires, nullptr, faceMaker[i]);
                    break;
                }
                catch (...) {
                    if (i == std::size(faceMaker) - 1) {
                        throw;
                    }
                    continue;
                }
            }
        }

        if (!front.isNull() || !back.isNull()) {
            BRepBuilderAPI_Sewing sewer;
            sewer.SetTolerance(Precision::Confusion());
            if (!front.isNull()) {
                sewer.Add(front.getShape());
            }
            if (!back.isNull()) {
                sewer.Add(back.getShape());
            }
            for (auto& s : shells) {
                sewer.Add(s.getShape());
            }

            sewer.Perform();

            if (!front.isNull()) {
                shells.push_back(front);
            }
            if (!back.isNull()) {
                shells.push_back(back);
            }
            // equivalent of the removed: result = result.makeElementShape(sewer,shells);
            result = result.makeShapeWithElementMap(
                sewer.SewedShape(),
                Part::MapperSewing(sewer),
                shells,
                Part::OpCodes::Sewing
            );
        }

        if (!result.countSubShapes(TopAbs_SHELL)) {
            return new App::DocumentObjectExecReturn(
                QT_TRANSLATE_NOOP("Exception", "Loft: Failed to create shell")
            );
        }
        shapes = result.getSubTopoShapes(TopAbs_SHELL);

        for (auto& s : shapes) {
            // build the solid
            s = s.makeElementSolid();
            BRepClass3d_SolidClassifier SC(s.getShape());
            SC.PerformInfinitePoint(Precision::Confusion());
            if (SC.State() == TopAbs_IN) {
                s.setShape(s.getShape().Reversed(), false);
            }
        }

        AddSubShape.setValue(result.makeElementCompound(
            shapes,
            nullptr,
            Part::TopoShape::SingleShapeCompoundCreationPolicy::returnShape
        ));

        if (shapes.size() > 1) {
            result.makeElementFuse(shapes);
        }
        else {
            result = shapes.front();
        }

        if (base.isNull()) {
            if (!isSingleSolidRuleSatisfied(result.getShape())) {
                return new App::DocumentObjectExecReturn(QT_TRANSLATE_NOOP(
                    "Exception",
                    "Result has multiple solids: enable 'Allow Compound' in the active body."
                ));
            }
            Shape.setValue(getSolid(result));
            Base::Console().message("%s: Loft created successfully.\n", getNameInDocument());
            return App::DocumentObject::StdReturn;
        }

        result.Tag = -getID();
        TopoShape boolOp(0, getDocument()->getStringHasher());

        const char* maker;
        switch (getAddSubType()) {
            case Additive:
                maker = Part::OpCodes::Fuse;
                break;
            case Subtractive:
                maker = Part::OpCodes::Cut;
                break;
            default:
                return new App::DocumentObjectExecReturn(
                    QT_TRANSLATE_NOOP("Exception", "Unknown operation type")
                );
        }
        try {
            boolOp.makeElementBoolean(maker, {base, result}, nullptr, FuzzyTolerance.getValue());
        }
        catch (Standard_Failure&) {
            return new App::DocumentObjectExecReturn(
                QT_TRANSLATE_NOOP("Exception", "Failed to perform boolean operation")
            );
        }
        TopoShape solid = getSolid(boolOp);
        // lets check if the result is a solid
        if (solid.isNull()) {
            return new App::DocumentObjectExecReturn(
                QT_TRANSLATE_NOOP("Exception", "Resulting shape is not a solid")
            );
        }
        // store shape before refinement
        this->rawShape = boolOp;
        boolOp = refineShapeIfActive(boolOp);
        if (!isSingleSolidRuleSatisfied(boolOp.getShape())) {
            return new App::DocumentObjectExecReturn(QT_TRANSLATE_NOOP(
                "Exception",
                "Result has multiple solids: enable 'Allow Compound' in the active body."
            ));
        }
        boolOp = getSolid(boolOp);
        Shape.setValue(boolOp);
        Base::Console().message("%s: Loft created successfully.\n", getNameInDocument());
        return App::DocumentObject::StdReturn;
    }
    catch (Standard_Failure& e) {
        return new App::DocumentObjectExecReturn(e.GetMessageString());
    }
    catch (const Base::Exception& e) {
        return new App::DocumentObjectExecReturn(e.what());
    }
    catch (...) {
        return new App::DocumentObjectExecReturn(
            QT_TRANSLATE_NOOP("Exception", "Loft: A fatal error occurred when making the loft")
        );
    }
}

PROPERTY_SOURCE(PartDesign::AdditiveLoft, PartDesign::Loft)
AdditiveLoft::AdditiveLoft()
{
    addSubType = Additive;
}

PROPERTY_SOURCE(PartDesign::SubtractiveLoft, PartDesign::Loft)
SubtractiveLoft::SubtractiveLoft()
{
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
