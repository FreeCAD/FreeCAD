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

#pragma once

#include <functional>
#include <string>
#include <vector>

#include <Bnd_Box.hxx>

#include <Base/Rotation.h>

#include <App/PropertyLinks.h>
#include <App/PropertyStandard.h>
#include <App/PropertyUnits.h>

#include <Mod/Part/PartGlobal.h>

#include "PartFeature.h"


namespace Part
{

class PartExport SectionAnalysis: public Part::Feature
{
    PROPERTY_HEADER_WITH_OVERRIDE(Part::SectionAnalysis);

public:
    SectionAnalysis();

    /// Objects to section. A container (App::Part, Body) is walked recursively,
    /// so listing several roots is only needed for objects that sit side by
    /// side at document level.
    App::PropertyLinkList Source;
    App::PropertyVector PlaneNormal;
    App::PropertyDistance PlaneOffset;
    App::PropertyBool FlipCut;

    /// What the section produces.
    ///
    /// "Display" draws the cap from the triangles the 3D view already holds,
    /// which is interactive even on a large assembly. "Geometry" additionally
    /// builds real B-rep faces with OCCT, which is what anything downstream of
    /// Shape needs - measurement, export, further modelling - but costs a
    /// boolean per solid and can run into minutes on an assembly.
    App::PropertyEnumeration ResultMode;

    /// True when Shape is expected to hold real section faces.
    bool wantsSolidGeometry() const;

    /// Distinct source objects that contributed solids, in collection order.
    /// A single object (e.g. a PartDesign Body) may contribute several solids;
    /// it appears here once so all its solids share one colour and hatch angle.
    App::PropertyLinkList SourceParts;

    /// For each face of Shape, in face order, the index into SourceParts of the
    /// object it came from.  This is the authoritative face-to-source mapping;
    /// consumers must not re-derive it from getOutList.
    App::PropertyIntegerList FaceSourceIndex;

    /// The cutting plane as the rest of the code needs it: a unit normal
    /// pointing away from the material that survives the cut, and the offset
    /// along it. FlipCut is already applied. False if PlaneNormal is degenerate.
    ///
    /// Every consumer must go through this. The surviving half-space is decided
    /// here and nowhere else - the caps, the clip planes, the hatching and the
    /// dragger all have to agree on it.
    bool cutPlane(Base::Vector3d& normal, double& offset) const;

    /// Where the gizmo should sit for the given plane: `hint` projected onto it.
    /// Pass the centre of the geometry being cut, so the handle stays with what
    /// the user is looking at.
    static Base::Vector3d draggerAnchor(
        const Base::Vector3d& normal,
        double offset,
        const Base::Vector3d& hint
    );

    /// Does a change to this section's own `prop` mean the triangles harvested
    /// from the 3D view are stale?
    ///
    /// Only the input list can. Moving the plane changes nothing about the
    /// geometry being cut, and the section's own outputs - Shape, SourceParts,
    /// FaceSourceIndex - are republished by every recompute. Keying the cache
    /// off an output throws it away on every plane move, which has now happened
    /// twice: once via Shape, once via SourceParts.
    bool invalidatesHarvest(const App::Property& prop) const;

    /// Does `prop` having just changed on `obj` mean the triangles harvested
    /// from the 3D view are stale?
    ///
    /// Appearing or disappearing changes which triangles exist, being edited
    /// changes what they are, and being moved changes where they are - the
    /// harvest is in world coordinates, so a placement is baked into it.
    /// Nothing else does.
    ///
    /// Takes the object as well as the property because visibility is a
    /// property every DocumentObject owns, so it can be compared by address;
    /// the rest cannot. Placement is declared on App::GeoFeature but also
    /// separately on App::Link, and Shape only exists on Part::Feature, so
    /// matching those by address would mean a dynamic_cast per type that might
    /// be a source - and silently missing any type not listed. Matching by name
    /// is uniform across all of them, which is what keeps links working.
    static bool isHarvestStaleAfter(const App::DocumentObject& obj, const App::Property& prop);

    /// Orthonormal frame spanning a plane with the given normal.
    static void planeFrame(const Base::Vector3d& normal, Base::Vector3d& u, Base::Vector3d& v);

    /// Visit every visible object under `sources` that contributes geometry,
    /// recursing into containers via subname paths so nested placements compose.
    /// `visit` gets the leaf object and the shape it yielded, in collection
    /// order. `exclude` is skipped - pass the section itself.
    ///
    /// This is the single definition of what counts as one "part", and both
    /// result modes go through it. Geometry mode turns the visit into
    /// SourceParts; Display mode uses it only for the identities, to group the
    /// triangles it harvests from the 3D view. They disagreed before: Display
    /// mode never recursed, so an assembly linked in as one object came out as a
    /// single body and per-part colouring had nothing to colour.
    ///
    /// A leaf is whatever yields a shape, which is why this asks OCCT even when
    /// only the identities are wanted. That is a fetch and a transform, not a
    /// boolean - nothing like the cost Display mode exists to avoid - and it is
    /// what keeps the two modes agreeing on the answer.
    static void forEachSourcePart(
        const std::vector<App::DocumentObject*>& sources,
        const App::DocumentObject* exclude,
        const std::function<void(App::DocumentObject*, const TopoDS_Shape&)>& visit
    );

    /// The distinct objects forEachSourcePart visits, in collection order.
    ///
    /// An object appears once however many times it is visited, because a part
    /// placed several times is still one part - the same rule execute() applies
    /// when it builds SourceParts, so a colour belongs to a part rather than to
    /// one of its instances.
    ///
    /// Prefer this to reading SourceParts when you need the count: SourceParts
    /// is an output, and Display mode deliberately leaves it empty.
    static std::vector<App::DocumentObject*> distinctSourceParts(
        const std::vector<App::DocumentObject*>& sources,
        const App::DocumentObject* exclude
    );

    /// Combined bounding box of the visible source shapes. False if empty.
    bool sourceBoundingBox(Bnd_Box& bbox) const;
    static bool sourceBoundingBox(const std::vector<App::DocumentObject*>& objs, Bnd_Box& bbox);

    /// True if the object and every claiming ancestor (Body, Part, ...) are
    /// visible. An object's own Visibility stays true when its container is
    /// hidden, so the plain property is not enough.
    static bool isEffectivelyVisible(const App::DocumentObject* obj);

    App::DocumentObjectExecReturn* execute() override;
    short mustExecute() const override;
    void handleChangedPropertyType(
        Base::XMLReader& reader,
        const char* TypeName,
        App::Property* prop
    ) override;
    void handleChangedPropertyName(
        Base::XMLReader& reader,
        const char* TypeName,
        const char* PropName
    ) override;
    const char* getViewProviderName() const override
    {
        return "PartGui::ViewProviderSectionAnalysis";
    }

private:
    /// Fallback capping: half-space Boolean cut, collect the on-plane faces.
    void collectSectionFaces(
        const TopoDS_Shape& solid,
        const gp_Pln& slicePlane,
        std::vector<TopoDS_Face>& faces
    ) const;

    /// Repair a solid whose degenerate edges would crash the OCCT boolean
    /// engine; returns a null shape if unrepairable so the caller can skip it.
    TopoDS_Shape prepareSolidForSection(const TopoDS_Shape& solid) const;
};

}  // namespace Part
