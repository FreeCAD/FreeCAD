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

    /// Number of section faces produced per solid (for per-solid coloring).
    /// Index i = number of faces from solid i.  Sum = total faces in Shape.
    App::PropertyIntegerList SolidFaceCounts;

    /// Distinct source objects that contributed solids, in collection order.
    /// A single object (e.g. a PartDesign Body) may contribute several solids;
    /// it appears here once so all its solids share one colour and hatch angle.
    App::PropertyLinkList SourceParts;

    /// For each solid (same order/length as SolidFaceCounts), the index into
    /// SourceParts of the object it came from.  This is the authoritative
    /// solid-to-source mapping; consumers must not re-derive it from getOutList.
    App::PropertyIntegerList SolidSourceIndex;

    App::DocumentObjectExecReturn* execute() override;
    short mustExecute() const override;
    void handleChangedPropertyType(
        Base::XMLReader& reader,
        const char* TypeName,
        App::Property* prop
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
