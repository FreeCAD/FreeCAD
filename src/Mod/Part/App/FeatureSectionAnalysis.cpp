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

#include <Base/Console.h>

#include "FaceMakerBullseye.h"
#include "FeatureSectionAnalysis.h"


using namespace Part;

PROPERTY_SOURCE(Part::SectionAnalysis, Part::Feature)

SectionAnalysis::SectionAnalysis()
{
    ADD_PROPERTY_TYPE(Source, (nullptr), "Section Analysis", App::Prop_None, "Source shape to section");
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
    // Extract plane coefficients: ax + by + cz + d_coeff = 0
    // The offset-from-origin is -d_coeff.
    double a, b, c, d_coeff;
    slicePlane.Coefficients(a, b, c, d_coeff);
    double d = -d_coeff;

    // Create a face on the cutting plane
    BRepBuilderAPI_MakeFace mkFace(slicePlane);
    TopoDS_Face planeFace = mkFace.Face();

    // Create a reference point on the positive normal side
    gp_Vec tempVector(a, b, c);
    tempVector.Normalize();
    tempVector *= (d + 1.0);
    gp_Pnt refPoint(0.0, 0.0, 0.0);
    refPoint.Translate(tempVector);

    // Create half-space containing the reference point
    BRepPrimAPI_MakeHalfSpace mkSolid(planeFace, refPoint);
    TopoDS_Solid halfSpace = mkSolid.Solid();

    // Cut the solid with the half-space (use raw OCCT, not FC wrapper which logs errors)
    BRepAlgoAPI_Cut mkCut(solid, halfSpace);
    if (!mkCut.IsDone()) {
        return;
    }

    // Find faces that lie on the cutting plane
    TopTools_IndexedMapOfShape mapOfFaces;
    TopExp::MapShapes(mkCut.Shape(), TopAbs_FACE, mapOfFaces);
    for (int i = 1; i <= mapOfFaces.Extent(); i++) {
        const TopoDS_Face& face = TopoDS::Face(mapOfFaces.FindKey(i));
        BRepAdaptor_Surface adapt(face);
        if (adapt.GetType() == GeomAbs_Plane) {
            gp_Pln plane = adapt.Plane();
            if (plane.Axis().IsParallel(slicePlane.Axis(), Precision::Confusion())
                && plane.Distance(slicePlane.Location()) < Precision::Confusion()) {
                // Ensure consistent face orientation — the effective normal
                // (geometric normal ± topological orientation) should match
                // the slice plane direction so lighting/hatching is stable.
                // BRepAdaptor_Surface::Plane() returns the geometric normal
                // which ignores face topology, so check Orientation() too.
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
    // Common case: the solid is already safe to section, pass it through.
    if (!hasDegenerateEdgeWithoutPCurve(solid)) {
        return solid;
    }

    // Try to repair the missing pcurves; ShapeFix_Shape rebuilds them as part
    // of its wire/edge fixes.
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

    // Still unsafe — return null so the caller skips this solid rather than
    // risk a hard crash in the boolean engine.
    return {};
}

App::DocumentObjectExecReturn* SectionAnalysis::execute()
{
    App::DocumentObject* source = Source.getValue();
    if (!source) {
        return new App::DocumentObjectExecReturn("No source shape linked.");
    }

    // Collect shapes recursively — handles nested App::Part containers
    // (e.g., STEP imports from Fusion 360 with sub-assemblies).  Each shape is
    // kept paired with the object it came from so every resulting solid can be
    // attributed to a source object for per-body colouring and hatching.
    std::vector<std::pair<TopoDS_Shape, App::DocumentObject*>> parts;

    TopoDS_Shape sourceShape
        = Feature::getShape(source, ShapeOption::ResolveLink | ShapeOption::Transform);

    if (!sourceShape.IsNull()) {
        // Single resolved shape (e.g. a Body or compound). Every solid it
        // contains is attributed to the source object itself.
        parts.emplace_back(sourceShape, source);
    }
    else {
        // Recursive collection for nested Part containers — attribute each
        // leaf shape to the leaf object that produced it.
        std::function<void(App::DocumentObject*)> collectShapes;
        collectShapes = [&](App::DocumentObject* obj) {
            TopoDS_Shape shape
                = Feature::getShape(obj, ShapeOption::ResolveLink | ShapeOption::Transform);
            if (!shape.IsNull()) {
                parts.emplace_back(shape, obj);
                return;
            }
            // Recurse into children (App::Part, App::DocumentObjectGroup, etc.)
            for (auto* child : obj->getOutList()) {
                collectShapes(child);
            }
        };

        for (auto* child : source->getOutList()) {
            collectShapes(child);
        }
    }

    if (parts.empty()) {
        return new App::DocumentObjectExecReturn("Source shape is empty.");
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
    TopExp_Explorer xp;
    std::vector<TopoDS_Face> sectionFaces;
    std::vector<long> faceCounts;

    // Flatten all collected parts into solids, keeping each solid attributed to
    // its source object. MAgic. This is the single source of truth for the
    // solid-to-source mapping; SolidFaceCounts and SolidSourceIndex below are
    // built in lockstep with this order so consumers never have to re-derive it.
    std::vector<std::pair<TopoDS_Shape, App::DocumentObject*>> solids;
    for (const auto& part : parts) {
        for (xp.Init(part.first, TopAbs_SOLID); xp.More(); xp.Next()) {
            // Repair (or drop) solids whose degenerate edges would crash the
            // OCCT boolean engine; skipped solids simply produce no section.
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
        Base::Console().warning(
            "SectionAnalysis: no solids found in source shape. "
            "For nested Part containers (e.g. STEP imports), try selecting "
            "individual bodies or creating a compound first.\n"
        );
    }

    // Build the distinct source-object list and the per-solid index into it.
    // A single object contributing several solids appears once, so all its
    // solids share one colour and hatch angle downstream.
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

    // Section each solid, with a per-solid fallback. The primary path
    // (BRepAlgoAPI_Section + FaceMakerBullseye) yields clean planar faces with
    // proper hole nesting, but complex profiles can defeat FaceMakerBullseye.
    // When a solid produces no face that way, fall back to the half-space
    // Boolean cut for that solid alone. Doing this per solid (rather than only
    // when every solid failed) ensures bodies whose profile beats the primary
    // path still get a filled cap instead of being left clipped-but-uncapped.
    const gp_Dir sliceNormal = slicePlane.Axis().Direction();
    long cappedPrimary = 0;
    long cappedFallback = 0;
    long uncapped = 0;

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

        bool viaPrimary = sectionFaces.size() > facesBefore;

        // Per-solid fallback: half-space Boolean cut caps closed solids robustly
        // even when the section profile defeats the primary path.
        if (!viaPrimary) {
            try {
                collectSectionFaces(currentSolid, slicePlane, sectionFaces);
            }
            catch (...) {
            }
        }

        if (sectionFaces.size() > facesBefore) {
            (viaPrimary ? cappedPrimary : cappedFallback)++;
        }
        else {
            uncapped++;
        }
        faceCounts.push_back(static_cast<long>(sectionFaces.size() - facesBefore));
    }

    if (uncapped > 0) {
        Base::Console().log(
            "SectionAnalysis: %d solids capped (primary), %d via fallback, "
            "%d produced no cross-section face.\n",
            cappedPrimary,
            cappedFallback,
            uncapped
        );
    }

    // faceCounts, solidSourceIdx and the solids list are all in the same order
    // and length, giving consumers an authoritative solid-to-source mapping.
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
