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
#include <BRepBndLib.hxx>
#include <Bnd_Box.hxx>
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
#include <cmath>
#include <cstring>
#include <functional>
#include <utility>
#include <vector>

#include <App/GeoFeatureGroupExtension.h>
#include <App/GroupExtension.h>
#include <Base/Console.h>

#include "FaceMakerBullseye.h"
#include "FeatureSectionAnalysis.h"


using namespace Part;

PROPERTY_SOURCE(Part::SectionAnalysis, Part::Feature)

namespace
{
// Display first: it is the mode that stays usable on the assemblies people
// actually complain about.
const char* ResultModeEnums[] = {"Display", "Geometry", nullptr};
}  // namespace


bool SectionAnalysis::wantsSolidGeometry() const
{
    return ResultMode.getValue() != 0;
}

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
        SourceParts,
        (nullptr),
        "Section Analysis",
        static_cast<App::PropertyType>(App::Prop_Output | App::Prop_Hidden),
        "Distinct source objects that contributed solids, in collection order"
    );
    ADD_PROPERTY_TYPE(
        FaceSourceIndex,
        ({}),
        "Section Analysis",
        static_cast<App::PropertyType>(App::Prop_Output | App::Prop_Hidden),
        "Per-face index into SourceParts (authoritative face-to-source mapping)"
    );

    ADD_PROPERTY_TYPE(
        ResultMode,
        ((long)0),
        "Section Analysis",
        App::Prop_None,
        "Display: fast preview only."
        "Geometry: builds real section faces in Shape, much slower."
    );
    ResultMode.setEnums(ResultModeEnums);

    Source.setScope(App::LinkScope::Global);
    SourceParts.setScope(App::LinkScope::Global);

    // The cutting plane is PlaneNormal/PlaneOffset, so this object's own
    // Placement is not an input. Left editable it would slide the section faces
    // away from the plane actually being cut.
    Placement.setStatus(App::Property::ReadOnly, true);
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

void SectionAnalysis::handleChangedPropertyName(
    Base::XMLReader& reader,
    const char* TypeName,
    const char* PropName
)
{
    // Per-solid face counts and source indices, superseded by the per-face
    // FaceSourceIndex. Both are recomputed, so the old values can be dropped.
    if (strcmp(PropName, "SolidFaceCounts") == 0 || strcmp(PropName, "SolidSourceIndex") == 0) {
        return;
    }
    Part::Feature::handleChangedPropertyName(reader, TypeName, PropName);
}

bool SectionAnalysis::invalidatesHarvest(const App::Property& prop) const
{
    // By address, so renaming Source is a compile error rather than a rule that
    // quietly stops matching.
    return &prop == &Source;
}

// The properties of the source objects that can change the triangles harvested
bool SectionAnalysis::isHarvestStaleAfter(const App::DocumentObject& obj, const App::Property& prop)
{
    // Visibility is declared on DocumentObject itself, so it is the one case
    // that can be settled by address rather than by name.
    if (&prop == &obj.Visibility) {
        return true;
    }

    // Null for a property the object does not own, which must not be read as
    // "something changed".
    const char* propertyName = obj.getPropertyName(&prop);
    if (propertyName == nullptr) {
        return false;
    }

    // Placement covers being moved. The harvest holds world coordinates, so a
    // source that moves leaves the cap sliced from triangles at the old
    // position - which for a Part::Feature is masked by the placement being
    // written back through Shape, but nothing writes it back for an App::Part,
    // a group or a link.
    return std::strcmp(propertyName, "Shape") == 0 || std::strcmp(propertyName, "Placement") == 0;
}


bool SectionAnalysis::isEffectivelyVisible(const App::DocumentObject* obj)
{
    for (const auto* o = obj; o; o = App::GeoFeatureGroupExtension::getGroupOfObject(o)) {
        if (!o->Visibility.getValue()) {
            return false;
        }
    }
    return true;
}

bool SectionAnalysis::cutPlane(Base::Vector3d& normal, double& offset) const
{
    Base::Vector3d n = PlaneNormal.getValue();
    const double len = n.Length();
    double d = PlaneOffset.getValue();

    // Catch corner cases out
    if (!std::isfinite(len) || !std::isfinite(d) || len < Precision::Confusion()) {
        return false;
    }
    n = n / len;

    // FlipCut swaps which half-space survives. Negating the offset with the
    // normal keeps n * d on the same point of the plane.
    if (FlipCut.getValue()) {
        n = -n;
        d = -d;
    }

    normal = n;
    offset = d;
    return true;
}

Base::Vector3d SectionAnalysis::draggerAnchor(
    const Base::Vector3d& normal,
    double offset,
    const Base::Vector3d& hint
)
{
    // Drop the hint onto the plane. Using the world origin instead - which is
    // what projecting nothing amounts to - puts the handle wherever the plane
    // happens to pass closest to (0, 0, 0), and an imported assembly can sit
    // far enough away for that to be off screen entirely.
    return hint - normal * (hint * normal - offset);
}

void SectionAnalysis::planeFrame(const Base::Vector3d& normal, Base::Vector3d& u, Base::Vector3d& v)
{
    // Cross with whichever global axis is furthest from the normal, so the
    // product never degenerates
    u = (std::abs(normal.x) < 0.9 ? Base::Vector3d(1, 0, 0) : Base::Vector3d(0, 1, 0)).Cross(normal);
    u.Normalize();
    v = normal.Cross(u);
    v.Normalize();
}

void SectionAnalysis::forEachSourcePart(
    const std::vector<App::DocumentObject*>& sources,
    const App::DocumentObject* exclude,
    const std::function<void(App::DocumentObject*, const TopoDS_Shape&)>& visit
)
{
    App::DocumentObject* root = nullptr;
    std::function<void(const std::string&)> descend = [&](const std::string& sub) {
        App::DocumentObject* obj = sub.empty() ? root : root->getSubObject(sub.c_str());
        // The root must be effectively visible (its containers too); nested
        // objects only need their own flag, their path is already checked.
        if (!obj || (sub.empty() ? !isEffectivelyVisible(obj) : !obj->Visibility.getValue())) {
            return;
        }
        // Containers are descended into, never taken whole. getShape() on an
        // App::Part hands back a compound of everything inside it, which is not
        // null - so asking first and descending only on failure meant the
        // recursion stopped at the first container it met. A whole assembly then
        // arrived as one part, with one colour and one hatch angle for the lot.
        //
        // Tested before the shape is fetched rather than after, which also
        // avoids building that compound just to throw it away.
        // Carrying a group extension is not enough to make something a
        // container. A PartDesign Body owns an Origin, so it inherits
        // GeoFeatureGroupExtension through OriginGroupExtension - but it is a
        // Part::Feature with a shape of its own, and it is one part, not a bag
        // of them. Descending into it finds sketches and datums, no shape comes
        // back, and the Body is simply not sectioned.
        //
        // So being a shape-bearing feature wins: those terminate the recursion.
        // App::Part and plain groups are not features, and still get descended.
        const bool isContainer = !obj->isDerivedFrom(Feature::getClassTypeId())
            && (obj->hasExtension(App::GeoFeatureGroupExtension::getExtensionClassTypeId())
                || obj->hasExtension(App::GroupExtension::getExtensionClassTypeId()));

        if (!isContainer) {
            TopoDS_Shape shape = Feature::getShape(
                root,
                ShapeOption::ResolveLink | ShapeOption::Transform,
                sub.empty() ? nullptr : sub.c_str()
            );
            if (!shape.IsNull()) {
                visit(obj, shape);
                return;
            }
        }
        for (const auto& child : obj->getSubObjects()) {
            descend(sub + child);
        }
    };

    for (App::DocumentObject* src : sources) {
        if (!src || src == exclude) {
            continue;
        }
        root = src;
        descend({});
    }
}

std::vector<App::DocumentObject*> SectionAnalysis::distinctSourceParts(
    const std::vector<App::DocumentObject*>& sources,
    const App::DocumentObject* exclude
)
{
    std::vector<App::DocumentObject*> parts;
    forEachSourcePart(sources, exclude, [&parts](App::DocumentObject* obj, const TopoDS_Shape&) {
        if (std::find(parts.begin(), parts.end(), obj) == parts.end()) {
            parts.push_back(obj);
        }
    });
    return parts;
}

bool SectionAnalysis::sourceBoundingBox(Bnd_Box& bbox) const
{
    return sourceBoundingBox(Source.getValues(), bbox);
}

bool SectionAnalysis::sourceBoundingBox(const std::vector<App::DocumentObject*>& objs, Bnd_Box& bbox)
{
    for (App::DocumentObject* obj : objs) {
        // Skip what execute() would skip, so anything sized from this box -
        // the cutting plane visual, the preset offset - matches the section
        if (!obj || !isEffectivelyVisible(obj)) {
            continue;
        }
        const TopoDS_Shape shape
            = Feature::getShape(obj, ShapeOption::ResolveLink | ShapeOption::Transform);
        if (!shape.IsNull()) {
            BRepBndLib::Add(shape, bbox);
        }
    }
    return !bbox.IsVoid();
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

            const double angTol = 1.0e-3;   // radians (~0.06 deg)
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
    // Enforce what the constructor only advertises: Python, expressions and
    // older documents can all still carry a placement, and the faces below are
    // built in global coordinates.
    if (!Placement.getValue().isIdentity()) {
        Placement.setValue(Base::Placement());
    }

    const std::vector<App::DocumentObject*> sources = Source.getValues();
    if (sources.empty()) {
        return new App::DocumentObjectExecReturn("No source shape linked.");
    }

    // In Display mode the cap is drawn by the view provider straight from the
    // triangles the 3D view already holds, so none of the work below is needed.
    // Doing it anyway is what makes moving the plane cost a minute: the boolean
    // is over 95% of that, and it is thrown away as soon as the plane moves.
    if (!wantsSolidGeometry()) {
        // Only publish if something actually changes. Setting a property to the
        // value it already holds still signals, and every view provider
        // downstream then rebuilds - clip planes, plane visual, and the cached
        // triangle harvest - on a recompute that produced nothing new.
        if (!FaceSourceIndex.getValues().empty()) {
            FaceSourceIndex.setValues({});
        }
        if (!SourceParts.getValues().empty()) {
            SourceParts.setValues({});
        }
        if (!this->Shape.getValue().IsNull()) {
            this->Shape.setValue(TopoDS_Shape());
        }
        return App::DocumentObject::StdReturn;
    }

    // Each shape stays paired with the object that produced it, for per-body
    // colouring. The recursion itself lives in forEachSourcePart(), because the
    // Display path has to group its triangles by exactly the same parts.
    std::vector<std::pair<TopoDS_Shape, App::DocumentObject*>> parts;
    forEachSourcePart(sources, this, [&parts](App::DocumentObject* obj, const TopoDS_Shape& shape) {
        parts.emplace_back(shape, obj);
    });

    // Nothing visible is a valid state (e.g. all bodies hidden) - publish an
    // empty section rather than erroring out and leaving a stale Shape behind.
    if (parts.empty()) {
        FaceSourceIndex.setValues({});
        SourceParts.setValues({});
        this->Shape.setValue(TopoDS_Shape());
        return App::DocumentObject::StdReturn;
    }

    Base::Vector3d n;
    double d = 0.0;
    if (!cutPlane(n, d)) {
        return new App::DocumentObjectExecReturn("Plane normal is zero.");
    }

    gp_Pln slicePlane(n.x, n.y, n.z, -d);
    std::vector<TopoDS_Face> sectionFaces;
    std::vector<long> faceSourceIdx;

    // Flatten to solids, each attributed to its source object. FaceSourceIndex
    // is built in lockstep with the faces these solids produce.
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

    // Distinct source objects; one contributing several solids appears once so
    // they share a colour.
    std::vector<App::DocumentObject*> uniqueParts;
    auto sourceIndex = [&uniqueParts](App::DocumentObject* obj) {
        auto it = std::find(uniqueParts.begin(), uniqueParts.end(), obj);
        if (it == uniqueParts.end()) {
            uniqueParts.push_back(obj);
            return static_cast<long>(uniqueParts.size() - 1);
        }
        return static_cast<long>(std::distance(uniqueParts.begin(), it));
    };

    // Section each solid. The primary path (BRepAlgoAPI_Section +
    // FaceMakerBullseye) yields clean planar faces with proper hole nesting;
    // when a profile defeats it, fall back to a half-space Boolean cut for
    // that solid alone.
    const gp_Dir sliceNormal = slicePlane.Axis().Direction();
    for (const auto& solidEntry : solids) {
        const TopoDS_Shape& currentSolid = solidEntry.first;
        const size_t facesBefore = sectionFaces.size();

        // A solid the plane never reaches cannot contribute a cap, and both the
        // section and the half-space fallback below are expensive. Rejecting on
        // the bounding box first costs microseconds and skips most of the solids
        // in an assembly.
        Bnd_Box solidBox;
        BRepBndLib::Add(currentSolid, solidBox, false);
        if (!solidBox.IsVoid() && solidBox.IsOut(slicePlane)) {
            faceSourceIdx.resize(sectionFaces.size(), sourceIndex(solidEntry.second));
            continue;
        }
        try {
            BRepAlgoAPI_Section cs;
            cs.Init1(currentSolid);
            cs.Init2(slicePlane);
            // Oriented bounding boxes let the boolean reject non-intersecting
            // sub-shapes far more aggressively than the default axis-aligned
            // test. On a real assembly this is worth about 5x. RunParallel uses
            // the spare cores for the solids that genuinely have to be cut.
            cs.SetUseOBB(true);
            cs.SetRunParallel(true);
            cs.Build();
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

        // Attribute the faces this solid just produced, so the mapping cannot
        // drift out of step with the face order in the Shape below
        faceSourceIdx.resize(sectionFaces.size(), sourceIndex(solidEntry.second));
    }

    SourceParts.setValues(uniqueParts);
    FaceSourceIndex.setValues(faceSourceIdx);
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
