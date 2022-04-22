/***************************************************************************
 *   Copyright (c) 2019 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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
# include <BRep_Builder.hxx>
# include <BRepAdaptor_Curve.hxx>
# include <BRepBuilderAPI_MakeEdge.hxx>
# include <BRepBuilderAPI_MakeWire.hxx>
# include <Geom_BSplineCurve.hxx>
# include <Precision.hxx>
# include <ShapeConstruct_Curve.hxx>
# include <Standard_Version.hxx>
# include <Standard_ConstructionError.hxx>
# include <TopExp_Explorer.hxx>
# include <TopoDS.hxx>
# include <TopoDS_Compound.hxx>
# include <TopoDS_Edge.hxx>
# include <TopoDS_Wire.hxx>
# include <TopLoc_Location.hxx>
# include <TopTools_ListOfShape.hxx>
# include <TopTools_ListIteratorOfListOfShape.hxx>
# include <TopTools_MapOfShape.hxx>
#endif
#include "BRepOffsetAPI_MakeOffsetFix.h"

using namespace Part;

BRepOffsetAPI_MakeOffsetFix::BRepOffsetAPI_MakeOffsetFix()
{
}

BRepOffsetAPI_MakeOffsetFix::BRepOffsetAPI_MakeOffsetFix(const GeomAbs_JoinType Join, const Standard_Boolean IsOpenResult)
{
#if OCC_VERSION_HEX >= 0x060900
    mkOffset.Init(Join, IsOpenResult);
#else
    (void)IsOpenResult;
    mkOffset.Init(Join);
#endif
}

BRepOffsetAPI_MakeOffsetFix::~BRepOffsetAPI_MakeOffsetFix()
{
}

void BRepOffsetAPI_MakeOffsetFix::AddWire(const TopoDS_Wire& Spine)
{
    TopoDS_Wire wire = Spine;
    int numEdges = 0;
    TopExp_Explorer xp(wire, TopAbs_EDGE);
    while (xp.More()) {
        numEdges++;
        xp.Next();
    }
    if (numEdges == 1) {
        TopLoc_Location edgeLocation;

        BRepBuilderAPI_MakeWire mkWire;
        TopExp_Explorer xp(wire, TopAbs_EDGE);
        while (xp.More()) {
            // The trick is to reset the placement of an edge before
            // passing it to BRepOffsetAPI_MakeOffset because then it
            // will create the expected result.
            // Afterwards apply the placement again on the result shape.
            // See the method MakeWire()
            TopoDS_Edge edge = TopoDS::Edge(xp.Current());
            edgeLocation = edge.Location();
            edge.Location(TopLoc_Location());
            mkWire.Add(edge);
            myLocations.emplace_back(edge, edgeLocation);
            xp.Next();
        }

        wire = mkWire.Wire();

        // Make sure that the new wire has the same orientation as the source wire
        // because otherwise the offset will shrink the wire
        wire.Orientation(Spine.Orientation());
    }
    mkOffset.AddWire(wire);
    myResult.Nullify();
}

void BRepOffsetAPI_MakeOffsetFix::Perform (const Standard_Real Offset, const Standard_Real Alt)
{
    mkOffset.Perform(Offset, Alt);
}

void BRepOffsetAPI_MakeOffsetFix::Build()
{
    mkOffset.Build();
}

void BRepOffsetAPI_MakeOffsetFix::Init(const TopoDS_Face& Spine, const GeomAbs_JoinType Join,
                                       const Standard_Boolean IsOpenResult)
{
    mkOffset.Init(Spine, Join, IsOpenResult);
}

void BRepOffsetAPI_MakeOffsetFix::Init(const GeomAbs_JoinType Join, const Standard_Boolean IsOpenResult)
{
    mkOffset.Init(Join, IsOpenResult);
}

Standard_Boolean BRepOffsetAPI_MakeOffsetFix::IsDone() const
{
    return mkOffset.IsDone();
}

void BRepOffsetAPI_MakeOffsetFix::MakeWire(TopoDS_Shape& wire)
{
    // get the edges of the wire and check which of the stored edges
    // serve as input of the wire
    TopTools_MapOfShape aMap;
    TopExp_Explorer xp(wire, TopAbs_EDGE);
    while (xp.More()) {
        aMap.Add(xp.Current());
        xp.Next();
    }

    std::list<TopoDS_Edge> edgeList;
    for (auto itLoc : myLocations) {
        const TopTools_ListOfShape& newShapes = mkOffset.Generated(itLoc.first);
        for (TopTools_ListIteratorOfListOfShape it(newShapes); it.More(); it.Next()) {
            TopoDS_Shape newShape = it.Value();

            if (aMap.Contains(newShape)) {
                newShape.Move(itLoc.second);
                edgeList.push_back(TopoDS::Edge(newShape));
            }
        }
    }

    if (!edgeList.empty()) {
        BRepBuilderAPI_MakeWire mkWire;
        mkWire.Add(edgeList.front());
        edgeList.pop_front();
        wire = mkWire.Wire();

        bool found = false;
        do {
            found = false;
            for (std::list<TopoDS_Edge>::iterator pE = edgeList.begin(); pE != edgeList.end(); ++pE) {
                mkWire.Add(*pE);
                if (mkWire.Error() != BRepBuilderAPI_DisconnectedWire) {
                    // edge added ==> remove it from list
                    found = true;
                    edgeList.erase(pE);
                    wire = mkWire.Wire();
                    break;
                }
            }
        }
        while (found);
    }
}

const TopoDS_Shape& BRepOffsetAPI_MakeOffsetFix::Shape()
{
    if (myResult.IsNull()) {
        TopoDS_Shape result = mkOffset.Shape();
        if (result.IsNull()) {
            myResult = result;
            return myResult;
        }

        if (result.ShapeType() == TopAbs_WIRE) {
            MakeWire(result);
        }
        else if (result.ShapeType() == TopAbs_COMPOUND) {
            BRep_Builder builder;
            TopoDS_Compound comp;
            builder.MakeCompound(comp);

            TopExp_Explorer xp(result, TopAbs_WIRE);
            while (xp.More()) {
                TopoDS_Wire wire = TopoDS::Wire(xp.Current());
                MakeWire(wire);
                builder.Add(comp, wire);
                xp.Next();
            }

            result = comp;
        }

        myResult = result;
    }
    return myResult;
}

const TopTools_ListOfShape& BRepOffsetAPI_MakeOffsetFix::Generated(const TopoDS_Shape& S)
{
    return mkOffset.Generated(S);
}

const TopTools_ListOfShape& BRepOffsetAPI_MakeOffsetFix::Modified (const TopoDS_Shape& S)
{
    return mkOffset.Modified(S);
}

Standard_Boolean BRepOffsetAPI_MakeOffsetFix::IsDeleted (const TopoDS_Shape& S)
{
    return mkOffset.IsDeleted(S);
}

TopoDS_Shape BRepOffsetAPI_MakeOffsetFix::Replace(GeomAbs_CurveType type, const TopoDS_Shape& S) const
{
    if (S.IsNull())
        throw Standard_ConstructionError("Input shape is null");

    // Nothing to do
    if (type == GeomAbs_BSplineCurve)
        return S;

    if (S.ShapeType() == TopAbs_COMPOUND) {
        BRep_Builder builder;
        TopoDS_Compound comp;
        builder.MakeCompound(comp);
        comp.Location(S.Location());

        TopExp_Explorer xp(S, TopAbs_WIRE);
        while (xp.More()) {
            TopoDS_Wire wire = TopoDS::Wire(xp.Current());
            wire = ReplaceEdges(type, wire);
            builder.Add(comp, wire);
            xp.Next();
        }

        return comp;
    }
    else if (S.ShapeType() == TopAbs_WIRE) {
        return ReplaceEdges(type, TopoDS::Wire(S));
    }
    else {
        throw Standard_ConstructionError("Wrong shape type");
    }
}

TopoDS_Wire BRepOffsetAPI_MakeOffsetFix::ReplaceEdges(GeomAbs_CurveType type, const TopoDS_Wire& wire) const
{
    BRepBuilderAPI_MakeWire mkWire;
    for (TopExp_Explorer xp(wire, TopAbs_EDGE); xp.More(); xp.Next()) {
        TopoDS_Edge edge = TopoDS::Edge(xp.Current());
        TopLoc_Location edgeLocation = edge.Location();

        BRepAdaptor_Curve curve(edge);

        if (curve.GetType() == type) {
            ShapeConstruct_Curve scc;
            double u = curve.FirstParameter();
            double v = curve.LastParameter();
            Handle(Geom_BSplineCurve) spline = scc.ConvertToBSpline(curve.Curve().Curve(), u, v, Precision::Confusion());
            if (!spline.IsNull()) {
                BRepBuilderAPI_MakeEdge mkEdge(spline, u, v);
                edge = mkEdge.Edge();
                edge.Location(edgeLocation);
            }
        }

        mkWire.Add(edge);
    }

    return mkWire.Wire();
}
