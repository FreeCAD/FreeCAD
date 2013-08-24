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
# include <BRepAdaptor_Surface.hxx>
# include <BRepAlgoAPI_Cut.hxx>
# include <BRepAlgoAPI_Section.hxx>
# include <BRepBuilderAPI_MakeFace.hxx>
# include <BRepBuilderAPI_MakeWire.hxx>
# include <BRepGProp_Face.hxx>
# include <BRepPrimAPI_MakeHalfSpace.hxx>
# include <gp_Pln.hxx>
# include <Precision.hxx>
# include <ShapeFix_Wire.hxx>
# include <TopExp.hxx>
# include <TopExp_Explorer.hxx>
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

std::list<TopoDS_Wire> CrossSection::section(double d) const
{
    std::list<TopoDS_Wire> wires;
#if 0
    // Fixes: 0001228: Cross section of Torus in Part Workbench fails or give wrong results
    if (s.ShapeType() == TopAbs_SOLID) {
        gp_Pln slicePlane(a,b,c,-d);
        BRepBuilderAPI_MakeFace mkFace(slicePlane);
        TopoDS_Face face = mkFace.Face();
        BRepPrimAPI_MakeHalfSpace mkSolid(face, gp_Pnt(0,0,d-1));
        TopoDS_Solid solid = mkSolid.Solid();
        BRepAlgoAPI_Cut mkCut(s,solid);

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
                        TopTools_IndexedMapOfShape mapOfWires;
                        TopExp::MapShapes(face, TopAbs_WIRE, mapOfWires);
                        for (int j=1; j<=mapOfWires.Extent(); j++) {
                            const TopoDS_Wire& wire = TopoDS::Wire(mapOfWires.FindKey(j));
                            wires.push_back(wire);
                        }
                    }
                }
            }
        }
    }
    else
#else
    {
        BRepAlgoAPI_Section cs(s, gp_Pln(a,b,c,-d));
        if (cs.IsDone()) {
            std::list<TopoDS_Edge> edges;
            TopExp_Explorer xp;
            for (xp.Init(cs.Shape(), TopAbs_EDGE); xp.More(); xp.Next())
                edges.push_back(TopoDS::Edge(xp.Current()));
            connectEdges(edges, wires);
        }
    }
#endif
    return wires;
}

void CrossSection::connectEdges (const std::list<TopoDS_Edge>& edges, std::list<TopoDS_Wire>& wires) const
{
    // FIXME: Use ShapeAnalysis_FreeBounds::ConnectEdgesToWires() as an alternative
    std::list<TopoDS_Edge> edge_list = edges;
    while (edge_list.size() > 0) {
        BRepBuilderAPI_MakeWire mkWire;
        // add and erase first edge
        mkWire.Add(edge_list.front());
        edge_list.erase(edge_list.begin());

        TopoDS_Wire new_wire = mkWire.Wire();  // current new wire

        // try to connect each edge to the wire, the wire is complete if no more egdes are connectible
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
        ShapeFix_Wire aFix;
        aFix.SetPrecision(Precision::Confusion());
        aFix.Load(new_wire);
        aFix.FixReorder();
        aFix.FixConnected();
        aFix.FixClosed();
        wires.push_back(aFix.Wire());
    }
}
