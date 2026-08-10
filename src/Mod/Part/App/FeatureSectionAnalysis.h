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

    /// The cutting plane after a drag of the gizmo. `rotation` turns the plane,
    /// `shift` then slides it along its own normal. The gizmo sits on the plane
    /// at startNormal * startOffset, and the plane must keep passing through
    /// that point as it turns.
    ///
    /// Pure, so the interaction maths is testable without a 3D view.
    static void planeAfterDrag(
        const Base::Vector3d& startNormal,
        double startOffset,
        const Base::Rotation& rotation,
        double shift,
        Base::Vector3d& normal,
        double& offset
    );

    /// Orthonormal frame spanning a plane with the given normal.
    static void planeFrame(const Base::Vector3d& normal, Base::Vector3d& u, Base::Vector3d& v);

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
