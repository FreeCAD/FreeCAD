// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2026 David Kaufman
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

#include "ShapeAnalysis_FreeBoundsFix.h"

#include <Standard_Version.hxx>

#include <BRep_Builder.hxx>
#include <ShapeAnalysis_FreeBounds.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Iterator.hxx>
#include <TopoDS_Wire.hxx>
#include <TopTools_HSequenceOfShape.hxx>

namespace Part
{

void Fix_ShapeAnalysis_FreeBounds_ConnectEdgesToWires(
    Handle(TopTools_HSequenceOfShape) & edges,
    double toler,
    bool shared,
    Handle(TopTools_HSequenceOfShape) & wires
)
{
    // Wrapper of ShapeAnalysis_FreeBounds::ConnectEdgesToWires to work around
    // OCCT bug 1330: filter out any edges with INTERNAL or EXTERNAL
    // orientation. They do not contribute to the boundary, and until 1330 is
    // fixed they are handled improperly.

    // Check if filtering is required
    bool needsFiltering = false;
    for (int i = 1; i <= edges->Length(); ++i) {
        TopAbs_Orientation ori = edges->Value(i).Orientation();
        if (ori == TopAbs_INTERNAL || ori == TopAbs_EXTERNAL) {
            needsFiltering = true;
            break;
        }
    }

#if OCC_VERSION_HEX >= 0x080100
    needsFiltering = false;
#endif

    // If filtering is not required, run the wrapped method with no changes
    if (!needsFiltering) {
        ShapeAnalysis_FreeBounds::ConnectEdgesToWires(edges, toler, shared, wires);
        return;
    }

    // If filtering is required, filter the edges and then run the wrapped method
    Handle(TopTools_HSequenceOfShape) filtered = new TopTools_HSequenceOfShape;
    for (int i = 1; i <= edges->Length(); ++i) {
        TopAbs_Orientation ori = edges->Value(i).Orientation();
        if (!(ori == TopAbs_INTERNAL || ori == TopAbs_EXTERNAL)) {
            filtered->Append(edges->Value(i));
        }
    }

    ShapeAnalysis_FreeBounds::ConnectEdgesToWires(filtered, toler, shared, wires);
}

void Fix_ShapeAnalysis_FreeBounds_ConnectWiresToWires(
    Handle(TopTools_HSequenceOfShape) & iwires,
    double toler,
    bool shared,
    Handle(TopTools_HSequenceOfShape) & owires
)
{
    // Wrapper of ShapeAnalysis_FreeBounds::ConnectWiresToWires to work around
    // OCCT bug 1330: filter out any edges with INTERNAL or EXTERNAL orientation
    // from each input wire. They do not contribute to the boundary, and until
    // 1330 is fixed they are handled improperly.

    // Check if filtering is required
    bool needsFiltering = false;
    for (int i = 1; i <= iwires->Length() && !needsFiltering; ++i) {
        for (TopoDS_Iterator it(TopoDS::Wire(iwires->Value(i))); it.More(); it.Next()) {
            TopAbs_Orientation ori = it.Value().Orientation();
            if (ori == TopAbs_INTERNAL || ori == TopAbs_EXTERNAL) {
                needsFiltering = true;
                break;
            }
        }
    }

#if OCC_VERSION_HEX >= 0x080100
    needsFiltering = false;
#endif

    // If filtering is not required, run the wrapped method with no changes
    if (!needsFiltering) {
        ShapeAnalysis_FreeBounds::ConnectWiresToWires(iwires, toler, shared, owires);
        return;
    }

    // If filtering is required, rebuild each wire without the offending edges,
    // then run the wrapped method
    Handle(TopTools_HSequenceOfShape) filteredWires = new TopTools_HSequenceOfShape;
    for (int i = 1; i <= iwires->Length(); ++i) {
        const TopoDS_Wire& orig = TopoDS::Wire(iwires->Value(i));

        BRep_Builder builder;
        TopoDS_Wire filteredWire;
        builder.MakeWire(filteredWire);
        bool hasEdges = false;
        for (TopoDS_Iterator it(orig); it.More(); it.Next()) {
            TopAbs_Orientation ori = it.Value().Orientation();
            if (!(ori == TopAbs_INTERNAL || ori == TopAbs_EXTERNAL)) {
                builder.Add(filteredWire, it.Value());
                hasEdges = true;
            }
        }

        // Wires reduced to zero edges are dropped.
        if (hasEdges) {
            filteredWires->Append(filteredWire);
        }
    }

    ShapeAnalysis_FreeBounds::ConnectWiresToWires(filteredWires, toler, shared, owires);
}

}  // namespace Part
