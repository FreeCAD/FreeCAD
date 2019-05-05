/***************************************************************************
 *   Copyright (c) 2016 Victor Titov (DeepSOIC)      <vv.titov@gmail.com>  *
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
# include <BRepBndLib.hxx>
# include <BRep_Builder.hxx>
# include <BRep_Tool.hxx>
# include <BRepAdaptor_Surface.hxx>
# include <BRepCheck_Analyzer.hxx>
# include <BRepBuilderAPI_MakeFace.hxx>
# include <Geom_Plane.hxx>
# include <gp_Pln.hxx>
# include <IntTools_FClass2d.hxx>
# include <Precision.hxx>
# include <ShapeAnalysis.hxx>
# include <ShapeAnalysis_Surface.hxx>
# include <ShapeExtend_Explorer.hxx>
# include <ShapeFix_Shape.hxx>
# include <ShapeFix_Wire.hxx>
# include <TopoDS.hxx>
# include <TopExp_Explorer.hxx>
# include <TopTools_IndexedMapOfShape.hxx>
# include <TopTools_HSequenceOfShape.hxx>
# include <QtGlobal>
#endif

#include "FaceMakerCheese.h"



using namespace Part;

TYPESYSTEM_SOURCE(Part::FaceMakerCheese, Part::FaceMakerPublic);


TopoDS_Face FaceMakerCheese::validateFace(const TopoDS_Face& face)
{
    BRepCheck_Analyzer aChecker(face);
    if (!aChecker.IsValid()) {
        TopoDS_Wire outerwire = ShapeAnalysis::OuterWire(face);
        TopTools_IndexedMapOfShape myMap;
        myMap.Add(outerwire);

        TopExp_Explorer xp(face,TopAbs_WIRE);
        ShapeFix_Wire fix;
        fix.SetFace(face);
        fix.Load(outerwire);
        fix.Perform();
        BRepBuilderAPI_MakeFace mkFace(fix.WireAPIMake());
        while (xp.More()) {
            if (!myMap.Contains(xp.Current())) {
                fix.Load(TopoDS::Wire(xp.Current()));
                fix.Perform();
                mkFace.Add(fix.WireAPIMake());
            }
            xp.Next();
        }

        aChecker.Init(mkFace.Face());
        if (!aChecker.IsValid()) {
            ShapeFix_Shape fix(mkFace.Face());
            fix.SetPrecision(Precision::Confusion());
            fix.SetMaxTolerance(Precision::Confusion());
            fix.SetMaxTolerance(Precision::Confusion());
            fix.Perform();
            fix.FixWireTool()->Perform();
            fix.FixFaceTool()->Perform();
            TopoDS_Face fixedFace = TopoDS::Face(fix.Shape());
            aChecker.Init(fixedFace);
            if (!aChecker.IsValid())
                Standard_Failure::Raise("Failed to validate broken face");
            return fixedFace;
        }
        return mkFace.Face();
    }

    return face;
}

bool FaceMakerCheese::Wire_Compare::operator() (const TopoDS_Wire& w1, const TopoDS_Wire& w2)
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

bool FaceMakerCheese::isInside(const TopoDS_Wire& wire1, const TopoDS_Wire& wire2)
{
    Bnd_Box box1;
    BRepBndLib::Add(wire1, box1);
    box1.SetGap(0.0);

    Bnd_Box box2;
    BRepBndLib::Add(wire2, box2);
    box2.SetGap(0.0);

    if (box1.IsOut(box2))
        return false;

    double prec = Precision::Confusion();

    BRepBuilderAPI_MakeFace mkFace(wire1);
    if (!mkFace.IsDone())
        Standard_Failure::Raise("Failed to create a face from wire in sketch");
    TopoDS_Face face = validateFace(mkFace.Face());
    BRepAdaptor_Surface adapt(face);
    IntTools_FClass2d class2d(face, prec);
    Handle(Geom_Surface) surf = new Geom_Plane(adapt.Plane());
    ShapeAnalysis_Surface as(surf);

    TopExp_Explorer xp(wire2,TopAbs_VERTEX);
    while (xp.More())  {
        TopoDS_Vertex v = TopoDS::Vertex(xp.Current());
        gp_Pnt p = BRep_Tool::Pnt(v);
        gp_Pnt2d uv = as.ValueOfUV(p, prec);
        if (class2d.Perform(uv) == TopAbs_IN)
            return true;
        // TODO: We can make a check to see if all points are inside or all outside
        // because otherwise we have some intersections which is not allowed
        else
            return false;
        //xp.Next();
    }

    return false;
}

TopoDS_Shape FaceMakerCheese::makeFace(std::list<TopoDS_Wire>& wires)
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
        if (inner_face.IsNull())
            return inner_face; // failure
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
    return validateFace(mkFace.Face());
}

TopoDS_Shape FaceMakerCheese::makeFace(const std::vector<TopoDS_Wire>& w)
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

        std::list<TopoDS_Wire>::iterator it = wire_list.begin();
        while (it != wire_list.end()) {
            if (isInside(wire, *it)) {
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


std::string FaceMakerCheese::getUserFriendlyName() const
{
    return std::string(QT_TRANSLATE_NOOP("Part_FaceMaker","Cheese facemaker"));
}

std::string FaceMakerCheese::getBriefExplanation() const
{
    return std::string(QT_TRANSLATE_NOOP("Part_FaceMaker","Supports making planar faces with holes, but no islands inside holes."));
}

void FaceMakerCheese::Build_Essence()
{
    TopoDS_Shape faces = makeFace(this->myWires);
    ShapeExtend_Explorer xp;
    Handle(TopTools_HSequenceOfShape) seq = xp.SeqFromCompound(faces, Standard_True);
    for(int i = 0   ;   i < seq->Length()   ;   i++){
        this->myShapesToReturn.push_back(seq->Value(i+1));
    }
}
