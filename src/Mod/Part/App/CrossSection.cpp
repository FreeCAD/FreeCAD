/***************************************************************************
 *   Copyright (c) 2010 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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
# include <algorithm>
# include <BRepAdaptor_Surface.hxx>
# include <BRepAlgoAPI_Common.hxx>
# include <BRepAlgoAPI_Cut.hxx>
# include <BRepAlgoAPI_Section.hxx>
# include <BRepBuilderAPI_MakeFace.hxx>
# include <BRepBuilderAPI_MakeWire.hxx>
# include <BRepPrimAPI_MakeHalfSpace.hxx>
# include <gp_Pln.hxx>
# include <Precision.hxx>
# include <ShapeAnalysis_FreeBounds.hxx>
# include <ShapeFix_Wire.hxx>
# include <TopExp.hxx>
# include <TopExp_Explorer.hxx>
# include <TopTools_HSequenceOfShape.hxx>
# include <TopTools_IndexedMapOfShape.hxx>
# include <TopoDS.hxx>
# include <TopoDS_Edge.hxx>
# include <TopoDS_Wire.hxx>
#endif

#include "CrossSection.h"


using namespace Part;

CrossSection::CrossSection(double a, double b, double c, const TopoDS_Shape& s)
  : a(a), b(b), c(c), s(s)
{
}

std::list<TopoDS_Wire> CrossSection::slice(double d) const
{
    std::list<TopoDS_Wire> wires;
    // Fixes: 0001228: Cross section of Torus in Part Workbench fails or give wrong results
    // Fixes: 0001137: Incomplete slices when using Part.slice on a torus
    TopExp_Explorer xp;
    for (xp.Init(s, TopAbs_SOLID); xp.More(); xp.Next()) {
        sliceSolid(d, xp.Current(), wires);
    }
    for (xp.Init(s, TopAbs_SHELL, TopAbs_SOLID); xp.More(); xp.Next()) {
        sliceNonSolid(d, xp.Current(), wires);
    }
    for (xp.Init(s, TopAbs_FACE, TopAbs_SHELL); xp.More(); xp.Next()) {
        sliceNonSolid(d, xp.Current(), wires);
    }

    return removeDuplicates(wires);
}

std::list<TopoDS_Wire> CrossSection::removeDuplicates(const std::list<TopoDS_Wire>& wires) const
{
    std::list<TopoDS_Wire> wires_reduce;
    for (const auto& wire : wires) {
        TopTools_IndexedMapOfShape mapOfEdges1;
        TopExp::MapShapes(wire, TopAbs_EDGE, mapOfEdges1);

        // The wires are independent shapes but their edges might be shared
        auto it = std::find_if(wires_reduce.begin(), wires_reduce.end(), [&mapOfEdges1](const TopoDS_Wire& w) {
            // same TShape and same placement but different orientation
            TopTools_IndexedMapOfShape mapOfEdges2;
            TopExp::MapShapes(w, TopAbs_EDGE, mapOfEdges2);
            int numEdges1 = mapOfEdges1.Extent();
            int numEdges2 = mapOfEdges2.Extent();
            if (numEdges1 != numEdges2)
                return false;

            TopTools_IndexedMapOfShape::Iterator it1(mapOfEdges1);
            TopTools_IndexedMapOfShape::Iterator it2(mapOfEdges2);
            for (; it1.More() && it2.More(); it1.Next(), it2.Next()) {
                if (!it1.Value().IsSame(it2.Value()))
                    return false;
            }

            return true;
        });

        if (it == wires_reduce.end()) {
            wires_reduce.push_back(wire);
        }
    }
    return wires_reduce;
}

void CrossSection::sliceNonSolid(double d, const TopoDS_Shape& shape, std::list<TopoDS_Wire>& wires) const
{
    BRepAlgoAPI_Section cs(shape, gp_Pln(a,b,c,-d));
    if (cs.IsDone()) {
        std::list<TopoDS_Edge> edges;
        TopExp_Explorer xp;
        for (xp.Init(cs.Shape(), TopAbs_EDGE); xp.More(); xp.Next())
            edges.push_back(TopoDS::Edge(xp.Current()));
        connectEdges(edges, wires);
    }
}

void CrossSection::sliceSolid(double d, const TopoDS_Shape& shape, std::list<TopoDS_Wire>& wires) const
{
    gp_Pln slicePlane(a,b,c,-d);
    BRepBuilderAPI_MakeFace mkFace(slicePlane);
    TopoDS_Face face = mkFace.Face();

    // Make sure to choose a point that does not lie on the plane (fixes #0001228)
    gp_Vec tempVector(a,b,c);
    tempVector.Normalize();//just in case.
    tempVector *= (d+1.0);
    gp_Pnt refPoint(0.0, 0.0, 0.0);
    refPoint.Translate(tempVector);

    BRepPrimAPI_MakeHalfSpace mkSolid(face, refPoint);
    TopoDS_Solid solid = mkSolid.Solid();
    BRepAlgoAPI_Cut mkCut(shape, solid);

    if (mkCut.IsDone()) {
        TopTools_IndexedMapOfShape mapOfFaces;
        TopExp::MapShapes(mkCut.Shape(), TopAbs_FACE, mapOfFaces);
        for (int i=1; i<=mapOfFaces.Extent(); i++) {
            const TopoDS_Face& face = TopoDS::Face(mapOfFaces.FindKey(i));
            BRepAdaptor_Surface adapt(face);
            if (adapt.GetType() == GeomAbs_Plane) {
                gp_Pln plane = adapt.Plane();
                if (plane.Axis().IsParallel(slicePlane.Axis(), Precision::Confusion()) &&
                    plane.Distance(slicePlane.Location()) < Precision::Confusion()) {
                    // sort and repair the wires
                    TopTools_IndexedMapOfShape mapOfWires;
                    TopExp::MapShapes(face, TopAbs_WIRE, mapOfWires);
                    connectWires(mapOfWires, wires);
                }
            }
        }
    }
}

void CrossSection::connectEdges (const std::list<TopoDS_Edge>& edges, std::list<TopoDS_Wire>& wires) const
{
    // Hint: Use ShapeAnalysis_FreeBounds::ConnectEdgesToWires() as an alternative
    std::list<TopoDS_Edge> edge_list = edges;
    while (!edge_list.empty()) {
        BRepBuilderAPI_MakeWire mkWire;
        // add and erase first edge
        mkWire.Add(edge_list.front());
        edge_list.erase(edge_list.begin());

        TopoDS_Wire new_wire = mkWire.Wire();  // current new wire

        // try to connect each edge to the wire, the wire is complete if no more edges are connectible
        bool found = false;
        do {
            found = false;
            for (std::list<TopoDS_Edge>::iterator pE = edge_list.begin(); pE != edge_list.end();++pE) {
                mkWire.Add(*pE);
                if (mkWire.Error() != BRepBuilderAPI_DisconnectedWire) {
                    // edge added ==> remove it from list
                    found = true;
                    edge_list.erase(pE);
                    new_wire = mkWire.Wire();
                    break;
                }
            }
        }
        while (found);

        // Fix any topological issues of the wire
        wires.push_back(fixWire(new_wire));
    }
}

void CrossSection::connectWires (const TopTools_IndexedMapOfShape& wireMap, std::list<TopoDS_Wire>& wires) const
{
    Handle(TopTools_HSequenceOfShape) hWires = new TopTools_HSequenceOfShape();
    for (int i=1; i<=wireMap.Extent(); i++) {
        const TopoDS_Shape& wire = wireMap.FindKey(i);
        hWires->Append(wire);
    }

    Handle(TopTools_HSequenceOfShape) hSorted = new TopTools_HSequenceOfShape();
    ShapeAnalysis_FreeBounds::ConnectWiresToWires(hWires, Precision::Confusion(), false, hSorted);

    for (int i=1; i<=hSorted->Length(); i++) {
        const TopoDS_Wire& new_wire = TopoDS::Wire(hSorted->Value(i));
        // Fix any topological issues of the wire
        wires.push_back(fixWire(new_wire));
    }
}

TopoDS_Wire CrossSection::fixWire(const TopoDS_Wire& wire) const
{
    // Fix any topological issues of the wire
    ShapeFix_Wire aFix;
    aFix.SetPrecision(Precision::Confusion());
    aFix.Load(wire);
    aFix.FixReorder();
    aFix.FixConnected();
    aFix.FixClosed();
    return aFix.Wire();
}
