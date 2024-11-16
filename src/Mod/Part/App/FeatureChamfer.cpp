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
# include <BRepFilletAPI_MakeChamfer.hxx>
# include <Precision.hxx>
# include <TopExp.hxx>
# include <TopoDS.hxx>
# include <TopoDS_Edge.hxx>
# include <TopTools_IndexedDataMapOfShapeListOfShape.hxx>
# include <TopTools_IndexedMapOfShape.hxx>
#endif

#include "FeatureChamfer.h"
#include "TopoShapeOpCode.h"


using namespace Part;

PROPERTY_SOURCE(Part::Chamfer, Part::FilletBase)

Chamfer::Chamfer() = default;

App::DocumentObjectExecReturn *Chamfer::execute()
{
    App::DocumentObject* link = Base.getValue();
    if (!link)
        return new App::DocumentObjectExecReturn("No object linked");

    try {
        TopoShape baseTopoShape = Feature::getTopoShape(link);
        auto baseShape = Feature::getShape(link);
        BRepFilletAPI_MakeChamfer mkChamfer(baseShape);
        TopTools_IndexedDataMapOfShapeListOfShape mapEdgeFace;
        TopExp::MapShapesAndAncestors(baseShape, TopAbs_EDGE, TopAbs_FACE, mapEdgeFace);
        TopTools_IndexedMapOfShape mapOfEdges;
        TopExp::MapShapes(baseShape, TopAbs_EDGE, mapOfEdges);

        const auto &vals = EdgeLinks.getSubValues();
        const auto &subs = EdgeLinks.getShadowSubs();
        if(subs.size()!=(size_t)Edges.getSize())
            return new App::DocumentObjectExecReturn("Edge link size mismatch");
        size_t i=0;
        for(const auto &info : Edges.getValues()) {
            auto &sub = subs[i];
            auto &ref = sub.newName.size()?sub.newName:vals[i];
            ++i;
            // Toponaming project March 2024:  Replaced this code because it wouldn't work:
//            TopoDS_Shape edge;
//            try {
//                edge = baseTopoShape.getSubShape(ref.c_str());
//            }catch(...){}
            auto id = Data::MappedName(ref.c_str()).toIndexedName().getIndex();
            const TopoDS_Edge& edge = TopoDS::Edge(mapOfEdges.FindKey(id));
            if(edge.IsNull())
                return new App::DocumentObjectExecReturn("Invalid edge link");
            double radius1 = info.radius1;
            double radius2 = info.radius2;
            const TopoDS_Face& face = TopoDS::Face(mapEdgeFace.FindFromKey(edge).First());
            mkChamfer.Add(radius1, radius2, TopoDS::Edge(edge), face);
        }

        TopoDS_Shape shape = mkChamfer.Shape();
        if (shape.IsNull())
            return new App::DocumentObjectExecReturn("Resulting shape is null");

        TopoShape res(0);
        this->Shape.setValue(res.makeElementShape(mkChamfer,baseTopoShape,Part::OpCodes::Chamfer));
        return Part::FilletBase::execute();
    }
    catch (Standard_Failure& e) {
        return new App::DocumentObjectExecReturn(e.GetMessageString());
    }
}

