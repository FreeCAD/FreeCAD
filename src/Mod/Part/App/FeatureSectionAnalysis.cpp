// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2026 FreeCAD contributors                              *
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
#include <TopoDS_Face.hxx>
#include <BRep_Builder.hxx>
#include <gp_Pln.hxx>
#include <gp_Vec.hxx>
#include <ShapeAnalysis_FreeBounds.hxx>
#include <ShapeFix_Wire.hxx>
#include <TopTools_HSequenceOfShape.hxx>

#include <BRepAlgoAPI_Cut.hxx>
#include <BRepAlgoAPI_Section.hxx>

#include <Base/Console.h>

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

    Source.setScope(App::LinkScope::Global);
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

App::DocumentObjectExecReturn* SectionAnalysis::execute()
{
    App::DocumentObject* source = Source.getValue();
    if (!source) {
        return new App::DocumentObjectExecReturn("No source shape linked.");
    }

    TopoDS_Shape sourceShape
        = Feature::getShape(source, ShapeOption::ResolveLink | ShapeOption::Transform);
    if (sourceShape.IsNull()) {
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

    std::vector<TopoDS_Face> sectionFaces;
    gp_Pln slicePlane(a, b, c, -d);

    // Try boolean-cut approach first for each solid (produces correct
    // faces with holes).  Fall back to BRepAlgoAPI_Section for solids
    // where the boolean cut fails or produces no section faces.
    TopExp_Explorer xp;
    int solidCount = 0;
    std::vector<TopoDS_Shape> unhandledSolids;

    for (xp.Init(sourceShape, TopAbs_SOLID); xp.More(); xp.Next()) {
        solidCount++;
        size_t facesBefore = sectionFaces.size();
        try {
            collectSectionFaces(xp.Current(), slicePlane, sectionFaces);
        }
        catch (...) {
            // Boolean cut failed — will try Section fallback
        }
        if (sectionFaces.size() == facesBefore) {
            unhandledSolids.push_back(xp.Current());
        }
    }

    // Fallback: use BRepAlgoAPI_Section for solids where the boolean cut
    // produced nothing, and for non-solid shapes (shells, faces).
    // Section gives intersection edges; we build faces from the wires.
    TopoDS_Shape sectionSource;
    if (solidCount == 0) {
        // No solids at all — section the entire source
        sectionSource = sourceShape;
    }
    else if (!unhandledSolids.empty()) {
        // Build a compound of the solids that need the fallback
        BRep_Builder bb;
        TopoDS_Compound comp;
        bb.MakeCompound(comp);
        for (const auto& s : unhandledSolids) {
            bb.Add(comp, s);
        }
        sectionSource = comp;
    }

    if (!sectionSource.IsNull()) {
        try {
            BRepAlgoAPI_Section cs(sectionSource, slicePlane);
            if (cs.IsDone()) {
                Handle(TopTools_HSequenceOfShape) hEdges = new TopTools_HSequenceOfShape();
                for (xp.Init(cs.Shape(), TopAbs_EDGE); xp.More(); xp.Next()) {
                    hEdges->Append(xp.Current());
                }

                Handle(TopTools_HSequenceOfShape) hWires = new TopTools_HSequenceOfShape();
                ShapeAnalysis_FreeBounds::ConnectEdgesToWires(hEdges, Precision::Confusion(), false, hWires);

                for (int i = 1; i <= hWires->Length(); i++) {
                    TopoDS_Wire wire = TopoDS::Wire(hWires->Value(i));
                    ShapeFix_Wire aFix;
                    aFix.SetPrecision(Precision::Confusion());
                    aFix.Load(wire);
                    aFix.FixReorder();
                    aFix.FixConnected();
                    aFix.FixClosed();
                    wire = aFix.Wire();

                    BRepBuilderAPI_MakeFace mkFace(slicePlane, wire);
                    if (mkFace.IsDone()) {
                        sectionFaces.push_back(mkFace.Face());
                    }
                }
            }
        }
        catch (...) {
            // Section fallback also failed
        }
    }

    Base::Console().log(
        "SectionAnalysis: %d solids, %d fallback, %d faces\n",
        solidCount,
        (int)unhandledSolids.size(),
        (int)sectionFaces.size()
    );

    if (sectionFaces.empty()) {
        this->Shape.setValue(TopoDS_Shape());
    }
    else if (sectionFaces.size() == 1) {
        this->Shape.setValue(sectionFaces.front());
    }
    else {
        // Create a compound of all section faces
        BRep_Builder builder;
        TopoDS_Compound compound;
        builder.MakeCompound(compound);
        for (const auto& face : sectionFaces) {
            builder.Add(compound, face);
        }
        this->Shape.setValue(compound);
    }

    return App::DocumentObject::StdReturn;
}
