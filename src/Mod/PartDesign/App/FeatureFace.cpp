/***************************************************************************
 *   Copyright (c) 2011 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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
# include <Bnd_Box.hxx>
# include <gp_Pln.hxx>
# include <BRep_Builder.hxx>
# include <BRepBndLib.hxx>
# include <BRepBuilderAPI_Copy.hxx>
# include <BRepBuilderAPI_MakeFace.hxx>
# include <BRepAdaptor_Surface.hxx>
# include <Geom_Plane.hxx>
# include <Handle_Geom_Surface.hxx>
# include <TopoDS.hxx>
# include <TopoDS_Compound.hxx>
# include <TopoDS_Face.hxx>
# include <TopoDS_Wire.hxx>
# include <TopExp_Explorer.hxx>
# include <BRepAlgoAPI_Fuse.hxx>
# include <gp_Pln.hxx>
#endif

#include <Base/Placement.h>

#include "FeatureFace.h"


using namespace PartDesign;


PROPERTY_SOURCE(PartDesign::Face, Part::Part2DObject)

Face::Face()
{
    ADD_PROPERTY(Sources,(0));
    Sources.setSize(0);
}

short Face::mustExecute() const
{
    if (Sources.isTouched())
        return 1;
    return Part::Part2DObject::mustExecute();
}

App::DocumentObjectExecReturn *Face::execute(void)
{
    std::vector<App::DocumentObject*> links = Sources.getValues();
    if (links.empty())
        return new App::DocumentObjectExecReturn("No shapes linked");

    std::vector<TopoDS_Wire> wires;
    for (std::vector<App::DocumentObject*>::iterator it = links.begin(); it != links.end(); ++it) {
        if (!(*it && (*it)->isDerivedFrom(Part::Part2DObject::getClassTypeId())))
            return new App::DocumentObjectExecReturn("Linked object is not a Sketch or Part2DObject");
        TopoDS_Shape shape = static_cast<Part::Part2DObject*>(*it)->Shape.getShape()._Shape;
        if (shape.IsNull())
            return new App::DocumentObjectExecReturn("Linked shape object is empty");

        // this is a workaround for an obscure OCC bug which leads to empty tessellations
        // for some faces. Making an explicit copy of the linked shape seems to fix it.
        // The error only happens when re-computing the shape.
        if (!this->Shape.getValue().IsNull()) {
            BRepBuilderAPI_Copy copy(shape);
            shape = copy.Shape();
            if (shape.IsNull())
                return new App::DocumentObjectExecReturn("Linked shape object is empty");
        }

        TopExp_Explorer ex;
        for (ex.Init(shape, TopAbs_WIRE); ex.More(); ex.Next()) {
            wires.push_back(TopoDS::Wire(ex.Current()));
        }
    }

    if (wires.empty()) // there can be several wires
        return new App::DocumentObjectExecReturn("Linked shape object is not a wire");

    TopoDS_Shape aFace = makeFace(wires);
    if (aFace.IsNull())
        return new App::DocumentObjectExecReturn("Creating a face from sketch failed");
    this->Shape.setValue(aFace);

    return App::DocumentObject::StdReturn;
}

// sort bounding boxes according to diagonal length
class Face::Wire_Compare {
public:
    bool operator() (const TopoDS_Wire& w1, const TopoDS_Wire& w2)
    {
        Bnd_Box box1, box2;
        if (!w1.IsNull()) {
            BRepBndLib::Add(w1, box1);
            box1.SetGap(0.0);
        }

        if (!w2.IsNull()) {
            BRepBndLib::Add(w2, box2);
            box2.SetGap(0.0);
        }

        return box1.SquareExtent() < box2.SquareExtent();
    }
};

TopoDS_Shape Face::makeFace(std::list<TopoDS_Wire>& wires) const
{
    BRepBuilderAPI_MakeFace mkFace(wires.front());
    const TopoDS_Face& face = mkFace.Face();
    if (face.IsNull())
        return face;
    gp_Dir axis(0,0,1);
    BRepAdaptor_Surface adapt(face);
    if (adapt.GetType() == GeomAbs_Plane) {
        axis = adapt.Plane().Axis().Direction();
    }

    wires.pop_front();
    for (std::list<TopoDS_Wire>::iterator it = wires.begin(); it != wires.end(); ++it) {
        BRepBuilderAPI_MakeFace mkInnerFace(*it);
        const TopoDS_Face& inner_face = mkInnerFace.Face();
        gp_Dir inner_axis(0,0,1);
        BRepAdaptor_Surface adapt(inner_face);
        if (adapt.GetType() == GeomAbs_Plane) {
            inner_axis = adapt.Plane().Axis().Direction();
        }
        // It seems that orientation is always 'Forward' and we only have to reverse
        // if the underlying plane have opposite normals. 
        if (axis.Dot(inner_axis) < 0)
            it->Reverse();
        mkFace.Add(*it);
    }
    return mkFace.Face();
}

TopoDS_Shape Face::makeFace(const std::vector<TopoDS_Wire>& w) const
{
    if (w.empty())
        return TopoDS_Shape();

    //FIXME: Need a safe method to sort wire that the outermost one comes last
    // Currently it's done with the diagonal lengths of the bounding boxes
    std::vector<TopoDS_Wire> wires = w;
    std::sort(wires.begin(), wires.end(), Wire_Compare());
    std::list<TopoDS_Wire> wire_list;
    wire_list.insert(wire_list.begin(), wires.rbegin(), wires.rend());

    // separate the wires into several independent faces
    std::list< std::list<TopoDS_Wire> > sep_wire_list;
    while (!wire_list.empty()) {
        std::list<TopoDS_Wire> sep_list;
        TopoDS_Wire wire = wire_list.front();
        wire_list.pop_front();
        sep_list.push_back(wire);

        Bnd_Box box;
        BRepBndLib::Add(wire, box);
        box.SetGap(0.0);

        std::list<TopoDS_Wire>::iterator it = wire_list.begin();
        while (it != wire_list.end()) {
            Bnd_Box box2;
            BRepBndLib::Add(*it, box2);
            box2.SetGap(0.0);
            if (!box.IsOut(box2)) {
                sep_list.push_back(*it);
                it = wire_list.erase(it);
            }
            else {
                ++it;
            }
        }

        sep_wire_list.push_back(sep_list);
    }

    if (sep_wire_list.size() == 1) {
        std::list<TopoDS_Wire>& wires = sep_wire_list.front();
        return makeFace(wires);
    }
    else if (sep_wire_list.size() > 1) {
        TopoDS_Compound comp;
        BRep_Builder builder;
        builder.MakeCompound(comp);
        for (std::list< std::list<TopoDS_Wire> >::iterator it = sep_wire_list.begin(); it != sep_wire_list.end(); ++it) {
            TopoDS_Shape aFace = makeFace(*it);
            if (!aFace.IsNull())
                builder.Add(comp, aFace);
        }

        return comp;
    }
    else {
        return TopoDS_Shape(); // error
    }
}

