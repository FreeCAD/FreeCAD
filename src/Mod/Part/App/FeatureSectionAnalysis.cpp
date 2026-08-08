// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2026 Gregg Jaskiewicz
// SPDX-FileNotice: Part of the FreeCAD project.

/******************************************************************************
 *                                                                            *
 *   FreeCAD is free software: you can redistribute it and/or modify          *
 *   it under the terms of the GNU Lesser General Public License as           *
 *   published by the Free Software Foundation, either version 2.1            *
 *   of the License, or (at your option) any later version.                   *
 *                                                                            *
 *   FreeCAD is distributed in the hope that it will be useful,               *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty              *
 *   of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.                  *
 *   See the GNU Lesser General Public License for more details.              *
 *                                                                            *
 *   You should have received a copy of the GNU Lesser General Public         *
 *   License along with FreeCAD. If not, see https://www.gnu.org/licenses     *
 *                                                                            *
 ******************************************************************************/

#include <BRepAdaptor_Surface.hxx>
#include <BRepBuilderAPI_MakeFace.hxx>
#include <BRepBuilderAPI_MakeWire.hxx>
#include <BRepPrimAPI_MakeHalfSpace.hxx>
#include <Precision.hxx>
#include <TopExp.hxx>
#include <TopExp_Explorer.hxx>
#include <TopTools_IndexedMapOfShape.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Compound.hxx>
#include <TopoDS_Edge.hxx>
#include <TopoDS_Face.hxx>
#include <BRep_Builder.hxx>
#include <BRep_Tool.hxx>
#include <Geom2d_Curve.hxx>
#include <gp_Pln.hxx>
#include <gp_Vec.hxx>
#include <ShapeAnalysis_FreeBounds.hxx>
#include <ShapeFix_Shape.hxx>
#include <ShapeFix_Wire.hxx>
#include <Standard_Failure.hxx>
#include <TopTools_HSequenceOfShape.hxx>

#include <BRepAlgoAPI_Cut.hxx>
#include <BRepAlgoAPI_Section.hxx>

#include <algorithm>
#include <functional>
#include <utility>
#include <vector>

#include <App/GeoFeatureGroupExtension.h>
#include <Base/Console.h>

#include "FaceMakerBullseye.h"
#include "FeatureSectionAnalysis.h"


using namespace Part;

PROPERTY_SOURCE(Part::SectionAnalysis, Part::Feature)

SectionAnalysis::SectionAnalysis()
{
    ADD_PROPERTY_TYPE(Source, (nullptr), "Section Analysis", App::Prop_None, "Source shapes to section");
    ADD_PROPERTY_TYPE(
        PlaneNormal,
        (Base::Vector3d(0, 0, 1)),
        "Section Analysis",
        App::Prop_None,
        "Normal of the cutting plane"
    );
    ADD_PROPERTY_TYPE(
        PlaneOffset,
        (0.0),
        "Section Analysis",
        App::Prop_None,
        "Distance of cutting plane from origin along the normal direction"
    );
    ADD_PROPERTY_TYPE(
        FlipCut,
        (false),
        "Section Analysis",
        App::Prop_None,
        "Flip which side of the plane is visible"
    );
    ADD_PROPERTY_TYPE(
        SolidFaceCounts,
        ({}),
        "Section Analysis",
        static_cast<App::PropertyType>(App::Prop_Output | App::Prop_Hidden),
        "Number of section faces per solid (for per-solid coloring)"
    );
    ADD_PROPERTY_TYPE(
        SourceParts,
        (nullptr),
        "Section Analysis",
        static_cast<App::PropertyType>(App::Prop_Output | App::Prop_Hidden),
        "Distinct source objects that contributed solids, in collection order"
    );
    ADD_PROPERTY_TYPE(
        SolidSourceIndex,
        ({}),
        "Section Analysis",
        static_cast<App::PropertyType>(App::Prop_Output | App::Prop_Hidden),
        "Per-solid index into SourceParts (authoritative solid-to-source mapping)"
    );

    Source.setScope(App::LinkScope::Global);
    SourceParts.setScope(App::LinkScope::Global);
}

void SectionAnalysis::handleChangedPropertyType(
    Base::XMLReader& reader,
    const char* TypeName,
    App::Property* prop
)
{
    // Source used to hold a single object before it could section several
    if (prop == &Source && strcmp(TypeName, "App::PropertyLink") == 0) {
        App::PropertyLink single;
        single.setContainer(this);
        single.Restore(reader);
        if (single.getValue()) {
            Source.setValues({single.getValue()});
        }
    }
    else {
        Part::Feature::handleChangedPropertyType(reader, TypeName, prop);
    }
}

short SectionAnalysis::mustExecute() const
{
    if (Source.isTouched() || PlaneNormal.isTouched() || PlaneOffset.isTouched()
        || FlipCut.isTouched()) {
        return 1;
    }
    return Feature::mustExecute();
}

void SectionAnalysis::collectSectionFaces(
    const TopoDS_Shape& solid,
    const gp_Pln& slicePlane,
    std::vector<TopoDS_Face>& faces
) const
{
    double a, b, c, d_coeff;
    slicePlane.Coefficients(a, b, c, d_coeff);
    double d = -d_coeff;

    BRepBuilderAPI_MakeFace mkFace(slicePlane);
    TopoDS_Face planeFace = mkFace.Face();

    // Half-space on the positive-normal side of the plane
    gp_Vec tempVector(a, b, c);
    tempVector.Normalize();
    tempVector *= (d + 1.0);
    gp_Pnt refPoint(0.0, 0.0, 0.0);
    refPoint.Translate(tempVector);
    BRepPrimAPI_MakeHalfSpace mkSolid(planeFace, refPoint);
    TopoDS_Solid halfSpace = mkSolid.Solid();

    BRepAlgoAPI_Cut mkCut(solid, halfSpace);
    if (!mkCut.IsDone()) {
        return;
    }

    // Collect the cut faces lying on the cutting plane
    TopTools_IndexedMapOfShape mapOfFaces;
    TopExp::MapShapes(mkCut.Shape(), TopAbs_FACE, mapOfFaces);
    for (int i = 1; i <= mapOfFaces.Extent(); i++) {
        const TopoDS_Face& face = TopoDS::Face(mapOfFaces.FindKey(i));
        BRepAdaptor_Surface adapt(face);
        if (adapt.GetType() == GeomAbs_Plane) {
            gp_Pln plane = adapt.Plane();
            // The boolean leaves the cap face only approximately coincident
            // with the slice plane on geometry's angular/positional drift.

            const double angTol = 1.0e-3;   // radians (~0.06°)
            const double distTol = 1.0e-3;  // mm
            if (plane.Axis().IsParallel(slicePlane.Axis(), angTol)
                && plane.Distance(slicePlane.Location()) < distTol) {

                // Orient the face along the slice normal so lighting/hatching
                // is stable; the geometric normal ignores face topology.
                gp_Dir effectiveNormal = plane.Axis().Direction();
                if (face.Orientation() == TopAbs_REVERSED) {
                    effectiveNormal.Reverse();
                }
                gp_Dir sliceNormal = slicePlane.Axis().Direction();
                if (effectiveNormal.Dot(sliceNormal) < 0) {
                    faces.push_back(TopoDS::Face(face.Reversed()));
                }
                else {
                    faces.push_back(face);
                }
            }
        }
    }
}

namespace
{
/// True if the shape has a degenerate edge that lacks a pcurve on its face.
/// This is the exact condition that makes OCCT's boolean ProcessDE step
/// dereference a null Geom2d_Curve and crash with a signal we cannot catch
/// portably, so it is what we detect and repair before sectioning.
/// True if the object and every claiming ancestor (Body, Part, ...) are
/// visible. An object's own Visibility stays true when its container is
/// hidden, so the plain property is not enough.
bool isEffectivelyVisible(const App::DocumentObject* obj)
{
    for (const auto* o = obj; o; o = App::GeoFeatureGroupExtension::getGroupOfObject(o)) {
        if (!o->Visibility.getValue()) {
            return false;
        }
    }
    return true;
}

bool hasDegenerateEdgeWithoutPCurve(const TopoDS_Shape& shape)
{
    for (TopExp_Explorer faceXp(shape, TopAbs_FACE); faceXp.More(); faceXp.Next()) {
        const TopoDS_Face& face = TopoDS::Face(faceXp.Current());
        for (TopExp_Explorer edgeXp(face, TopAbs_EDGE); edgeXp.More(); edgeXp.Next()) {
            const TopoDS_Edge& edge = TopoDS::Edge(edgeXp.Current());
            if (!BRep_Tool::Degenerated(edge)) {
                continue;
            }
            Standard_Real first = 0.0;
            Standard_Real last = 0.0;
            Handle(Geom2d_Curve) pcurve = BRep_Tool::CurveOnSurface(edge, face, first, last);
            if (pcurve.IsNull()) {
                return true;
            }
        }
    }
    return false;
}

}  // namespace

TopoDS_Shape SectionAnalysis::prepareSolidForSection(const TopoDS_Shape& solid) const
{
    if (!hasDegenerateEdgeWithoutPCurve(solid)) {
        return solid;
    }

    try {
        ShapeFix_Shape fixer(solid);
        fixer.Perform();
        TopoDS_Shape fixed = fixer.Shape();
        if (!fixed.IsNull() && !hasDegenerateEdgeWithoutPCurve(fixed)) {
            return fixed;
        }
    }
    catch (const Standard_Failure&) {
    }
    catch (...) {
    }

    return {};
}

App::DocumentObjectExecReturn* SectionAnalysis::execute()
{
    const std::vector<App::DocumentObject*> sources = Source.getValues();
    if (sources.empty()) {
        return new App::DocumentObjectExecReturn("No source shape linked.");
    }

    // Collect shapes, recursing into containers via subname paths so nested
    // placements compose correctly. Hidden objects are excluded — the section
    // shows what the user sees. Each shape stays paired with the object that
    // produced it, for per-body colouring.
    std::vector<std::pair<TopoDS_Shape, App::DocumentObject*>> parts;
    App::DocumentObject* root = nullptr;
    std::function<void(const std::string&)> collectShapes = [&](const std::string& sub) {
        App::DocumentObject* obj = sub.empty() ? root : root->getSubObject(sub.c_str());
        // The root must be effectively visible (its containers too); nested
        // objects only need their own flag, their path is already checked.
        if (!obj || (sub.empty() ? !isEffectivelyVisible(obj) : !obj->Visibility.getValue())) {
            return;
        }
        TopoDS_Shape shape = Feature::getShape(
            root,
            ShapeOption::ResolveLink | ShapeOption::Transform,
            sub.empty() ? nullptr : sub.c_str()
        );
        if (!shape.IsNull()) {
            parts.emplace_back(shape, obj);
            return;
        }
        for (const auto& child : obj->getSubObjects()) {
            collectShapes(sub + child);
        }
    };
    for (App::DocumentObject* src : sources) {
        if (!src || src == this) {
            continue;
        }
        root = src;
        collectShapes({});
    }

    // Nothing visible is a valid state (e.g. all bodies hidden) — publish an
    // empty section rather than erroring out and leaving a stale Shape behind.
    if (parts.empty()) {
        SolidFaceCounts.setValues({});
        SolidSourceIndex.setValues({});
        SourceParts.setValues({});
        this->Shape.setValue(TopoDS_Shape());
        return App::DocumentObject::StdReturn;
    }

    Base::Vector3d n = PlaneNormal.getValue();
    double d = PlaneOffset.getValue();
    bool flip = FlipCut.getValue();

    // Normalize
    double len = n.Length();
    if (len < Precision::Confusion()) {
        return new App::DocumentObjectExecReturn("Plane normal is zero.");
    }
    n = n / len;

    double a = n.x, b = n.y, c = n.z;
    if (flip) {
        a = -a;
        b = -b;
        c = -c;
        d = -d;
    }

    gp_Pln slicePlane(a, b, c, -d);
    std::vector<TopoDS_Face> sectionFaces;
    std::vector<long> faceCounts;

    // Flatten to solids, each attributed to its source object. SolidFaceCounts
    // and SolidSourceIndex are built in lockstep with this order.
    std::vector<std::pair<TopoDS_Shape, App::DocumentObject*>> solids;
    for (const auto& part : parts) {
        for (TopExp_Explorer xp(part.first, TopAbs_SOLID); xp.More(); xp.Next()) {
            TopoDS_Shape prepared = prepareSolidForSection(xp.Current());
            if (prepared.IsNull()) {
                Base::Console().warning(
                    "SectionAnalysis: skipped a solid with unrepairable "
                    "degenerate geometry to avoid a boolean engine crash.\n"
                );
                continue;
            }
            solids.emplace_back(prepared, part.second);
        }
    }
    int solidCount = static_cast<int>(solids.size());

    if (solidCount == 0) {
        Base::Console().warning("SectionAnalysis: no solids found in source shape.\n");
    }

    // Distinct source objects and the per-solid index into them; an object
    // contributing several solids appears once so they share one colour.
    std::vector<App::DocumentObject*> uniqueParts;
    std::vector<long> solidSourceIdx;
    solidSourceIdx.reserve(solids.size());
    for (const auto& s : solids) {
        auto it = std::find(uniqueParts.begin(), uniqueParts.end(), s.second);
        long idx;
        if (it == uniqueParts.end()) {
            idx = static_cast<long>(uniqueParts.size());
            uniqueParts.push_back(s.second);
        }
        else {
            idx = static_cast<long>(std::distance(uniqueParts.begin(), it));
        }
        solidSourceIdx.push_back(idx);
    }

    // Section each solid. The primary path (BRepAlgoAPI_Section +
    // FaceMakerBullseye) yields clean planar faces with proper hole nesting;
    // when a profile defeats it, fall back to a half-space Boolean cut for
    // that solid alone.
    const gp_Dir sliceNormal = slicePlane.Axis().Direction();
    for (const auto& solidEntry : solids) {
        const TopoDS_Shape& currentSolid = solidEntry.first;
        const size_t facesBefore = sectionFaces.size();

        try {
            BRepAlgoAPI_Section cs(currentSolid, slicePlane);
            if (cs.IsDone()) {
                Handle(TopTools_HSequenceOfShape) hEdges = new TopTools_HSequenceOfShape();
                TopExp_Explorer edgeXp;
                for (edgeXp.Init(cs.Shape(), TopAbs_EDGE); edgeXp.More(); edgeXp.Next()) {
                    hEdges->Append(edgeXp.Current());
                }
                if (!hEdges->IsEmpty()) {
                    Handle(TopTools_HSequenceOfShape) hWires = new TopTools_HSequenceOfShape();
                    ShapeAnalysis_FreeBounds::ConnectEdgesToWires(
                        hEdges,
                        Precision::Confusion(),
                        false,
                        hWires
                    );

                    FaceMakerBullseye fm;
                    fm.setPlane(slicePlane);
                    for (int i = 1; i <= hWires->Length(); i++) {
                        TopoDS_Wire wire = TopoDS::Wire(hWires->Value(i));
                        ShapeFix_Wire aFix;
                        aFix.SetPrecision(Precision::Confusion());
                        aFix.Load(wire);
                        aFix.FixReorder();
                        aFix.FixConnected();
                        aFix.FixClosed();
                        fm.addWire(aFix.Wire());
                    }
                    fm.Build();

                    if (fm.IsDone()) {
                        for (edgeXp.Init(fm.Shape(), TopAbs_FACE); edgeXp.More(); edgeXp.Next()) {
                            TopoDS_Face face = TopoDS::Face(edgeXp.Current());
                            BRepAdaptor_Surface adapt(face);
                            if (adapt.GetType() == GeomAbs_Plane) {
                                gp_Dir effectiveNormal = adapt.Plane().Axis().Direction();
                                if (face.Orientation() == TopAbs_REVERSED) {
                                    effectiveNormal.Reverse();
                                }
                                if (effectiveNormal.Dot(sliceNormal) < 0) {
                                    face = TopoDS::Face(face.Reversed());
                                }
                            }
                            sectionFaces.push_back(face);
                        }
                    }
                }
            }
        }
        catch (...) {
        }

        if (sectionFaces.size() == facesBefore) {
            try {
                collectSectionFaces(currentSolid, slicePlane, sectionFaces);
            }
            catch (...) {
            }
        }

        faceCounts.push_back(static_cast<long>(sectionFaces.size() - facesBefore));
    }

    SolidFaceCounts.setValues(faceCounts);
    SolidSourceIndex.setValues(solidSourceIdx);
    SourceParts.setValues(uniqueParts);
    auto& faces = sectionFaces;

    if (faces.empty() && solidCount > 0) {
        Base::Console().warning(
            "SectionAnalysis: %d solids found but no cross-section faces generated. "
            "The cutting plane may not intersect the geometry.\n",
            solidCount
        );
    }

    if (faces.empty()) {
        this->Shape.setValue(TopoDS_Shape());
    }
    else if (faces.size() == 1) {
        this->Shape.setValue(faces.front());
    }
    else {
        BRep_Builder builder;
        TopoDS_Compound compound;
        builder.MakeCompound(compound);
        for (const auto& face : faces) {
            builder.Add(compound, face);
        }
        this->Shape.setValue(compound);
    }

    return App::DocumentObject::StdReturn;
}
