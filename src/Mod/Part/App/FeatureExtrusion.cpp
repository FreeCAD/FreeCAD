/***************************************************************************
 *   Copyright (c) 2008 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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
# include <cmath>
# include <Bnd_Box.hxx>
# include <BRepBndLib.hxx>
# include <gp_Pln.hxx>
# include <gp_Trsf.hxx>
# include <BRep_Tool.hxx>
# include <BRepAdaptor_Surface.hxx>
# include <BRepCheck_Analyzer.hxx>
# include <BRepOffsetAPI_MakeOffset.hxx>
# include <BRepBuilderAPI_Copy.hxx>
# include <BRepBuilderAPI_MakeFace.hxx>
# include <BRepBuilderAPI_MakeWire.hxx>
# include <BRepBuilderAPI_Transform.hxx>
# include <BRepOffsetAPI_ThruSections.hxx>
# include <BRepPrimAPI_MakePrism.hxx>
# include <Geom_Plane.hxx>
# include <IntTools_FClass2d.hxx>
# include <Precision.hxx>
# include <ShapeAnalysis.hxx>
# include <ShapeAnalysis_Surface.hxx>
# include <ShapeFix_Shape.hxx>
# include <ShapeFix_Wire.hxx>
# include <TopoDS.hxx>
# include <TopoDS_Iterator.hxx>
# include <TopExp.hxx>
# include <TopExp_Explorer.hxx>
# include <TopTools_IndexedMapOfShape.hxx>
#endif


#include "FeatureExtrusion.h"
#include <Base/Tools.h>
#include <Base/Exception.h>


using namespace Part;


PROPERTY_SOURCE(Part::Extrusion, Part::Feature)

Extrusion::Extrusion()
{
    ADD_PROPERTY(Base,(0));
    ADD_PROPERTY(Dir,(Base::Vector3d(0.0,0.0,1.0)));
    ADD_PROPERTY(Solid,(false));
    ADD_PROPERTY(TaperAngle,(0.0));
}

short Extrusion::mustExecute() const
{
    if (Base.isTouched() ||
        Dir.isTouched() ||
        Solid.isTouched() ||
        TaperAngle.isTouched())
        return 1;
    return 0;
}

App::DocumentObjectExecReturn *Extrusion::execute(void)
{
    App::DocumentObject* link = Base.getValue();
    if (!link)
        return new App::DocumentObjectExecReturn("No object linked");
    if (!link->getTypeId().isDerivedFrom(Part::Feature::getClassTypeId()))
        return new App::DocumentObjectExecReturn("Linked object is not a Part object");
    Part::Feature *base = static_cast<Part::Feature*>(Base.getValue());

    Base::Vector3d v = Dir.getValue();
    gp_Vec vec(v.x,v.y,v.z);
    double taperAngle = TaperAngle.getValue();
    bool makeSolid = Solid.getValue();

    try {
        if (std::fabs(taperAngle) >= Precision::Confusion()) {
#if defined(__GNUC__) && defined (FC_OS_LINUX)
            Base::SignalException se;
#endif
            double distance = std::tan(Base::toRadians(taperAngle)) * vec.Magnitude();
            TopoDS_Shape myShape = base->Shape.getValue();
            if (myShape.IsNull())
                Standard_Failure::Raise("Cannot extrude empty shape");
            // #0000910: Circles Extrude Only Surfaces, thus use BRepBuilderAPI_Copy
            myShape = BRepBuilderAPI_Copy(myShape).Shape();

            std::list<TopoDS_Shape> drafts;
            makeDraft(distance, vec, makeSolid, myShape, drafts);
            if (drafts.empty()) {
                Standard_Failure::Raise("Drafting shape failed");
            }
            else if (drafts.size() == 1) {
                this->Shape.setValue(drafts.front());
            }
            else {
                TopoDS_Compound comp;
                BRep_Builder builder;
                builder.MakeCompound(comp);
                for (std::list<TopoDS_Shape>::iterator it = drafts.begin(); it != drafts.end(); ++it)
                    builder.Add(comp, *it);
                this->Shape.setValue(comp);
            }
        }
        else {
            // Now, let's get the TopoDS_Shape
            TopoDS_Shape myShape = base->Shape.getValue();
            if (myShape.IsNull())
                Standard_Failure::Raise("Cannot extrude empty shape");
            // #0000910: Circles Extrude Only Surfaces, thus use BRepBuilderAPI_Copy
            myShape = BRepBuilderAPI_Copy(myShape).Shape();
            if (makeSolid && myShape.ShapeType() != TopAbs_FACE) {
                std::vector<TopoDS_Wire> wires;
                TopTools_IndexedMapOfShape mapOfWires;
                TopExp::MapShapes(myShape, TopAbs_WIRE, mapOfWires);

                // if there are no wires then check also for edges
                if (mapOfWires.IsEmpty()) {
                    TopTools_IndexedMapOfShape mapOfEdges;
                    TopExp::MapShapes(myShape, TopAbs_EDGE, mapOfEdges);
                    for (int i=1; i<=mapOfEdges.Extent(); i++) {
                        BRepBuilderAPI_MakeWire mkWire(TopoDS::Edge(mapOfEdges.FindKey(i)));
                        wires.push_back(mkWire.Wire());
                    }
                }
                else {
                    wires.reserve(mapOfWires.Extent());
                    for (int i=1; i<=mapOfWires.Extent(); i++) {
                        wires.push_back(TopoDS::Wire(mapOfWires.FindKey(i)));
                    }
                }

                if (!wires.empty()) {
                    try {
                        TopoDS_Shape res = makeFace(wires);
                        if (!res.IsNull())
                            myShape = res;
                    }
                    catch (...) {
                    }
                }
            }
            BRepPrimAPI_MakePrism mkPrism(myShape, vec);
            TopoDS_Shape swept = mkPrism.Shape();
            if (swept.IsNull())
                return new App::DocumentObjectExecReturn("Resulting shape is null");
            this->Shape.setValue(swept);
        }
        return App::DocumentObject::StdReturn;
    }
    catch (Standard_Failure) {
        Handle_Standard_Failure e = Standard_Failure::Caught();
        return new App::DocumentObjectExecReturn(e->GetMessageString());
    }
}

void Extrusion::makeDraft(double distance, const gp_Vec& vec, bool makeSolid, const TopoDS_Shape& shape, std::list<TopoDS_Shape>& drafts) const
{
    std::list<TopoDS_Wire> wire_list;
    if (shape.IsNull())
        Standard_Failure::Raise("Not a valid shape");
    if (shape.ShapeType() == TopAbs_WIRE) {
        ShapeFix_Wire aFix;
        aFix.Load(TopoDS::Wire(shape));
        aFix.FixReorder();
        aFix.FixConnected();
        aFix.FixClosed();
        wire_list.push_back(aFix.Wire());
    }
    else if (shape.ShapeType() == TopAbs_FACE) {
        TopoDS_Wire outerWire = ShapeAnalysis::OuterWire(TopoDS::Face(shape));
        wire_list.push_back(outerWire);
    }
    else if (shape.ShapeType() == TopAbs_COMPOUND) {
        TopoDS_Iterator it(shape);
        for (; it.More(); it.Next()) {
            makeDraft(distance, vec, makeSolid, it.Value(), drafts);
        }
    }
    else {
        Standard_Failure::Raise("Only a wire or a face is supported");
    }

    if (!wire_list.empty()) {
        BRepOffsetAPI_MakeOffset mkOffset;
#if OCC_VERSION_HEX >= 0x060800
        mkOffset.Init(GeomAbs_Arc);
#endif
        mkOffset.AddWire(wire_list.front());
        mkOffset.Perform(distance);

        gp_Trsf mat;
        mat.SetTranslation(vec);
        BRepBuilderAPI_Transform mkTransform(mkOffset.Shape(),mat);
        if (mkTransform.Shape().IsNull())
            Standard_Failure::Raise("Tapered shape is empty");
        TopAbs_ShapeEnum type = mkTransform.Shape().ShapeType();
        if (type == TopAbs_WIRE) {
            wire_list.push_back(TopoDS::Wire(mkTransform.Shape()));
        }
        else if (type == TopAbs_EDGE) {
            BRepBuilderAPI_MakeWire mkWire(TopoDS::Edge(mkTransform.Shape()));
            wire_list.push_back(mkWire.Wire());
        }
        else {
            Standard_Failure::Raise("Tapered shape type is not supported");
        }

        BRepOffsetAPI_ThruSections mkGenerator(makeSolid ? Standard_True : Standard_False, Standard_False);
        for (std::list<TopoDS_Wire>::const_iterator it = wire_list.begin(); it != wire_list.end(); ++it) {
            const TopoDS_Wire &wire = *it;
            mkGenerator.AddWire(wire);
        }

        try {
#if defined(__GNUC__) && defined (FC_OS_LINUX)
            Base::SignalException se;
#endif
            mkGenerator.Build();
            drafts.push_back(mkGenerator.Shape());
        }
        catch (...) {
        }
    }
}

TopoDS_Face Extrusion::validateFace(const TopoDS_Face& face) const
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

// sort bounding boxes according to diagonal length
class Extrusion::Wire_Compare : public std::binary_function<const TopoDS_Wire&,
                                                            const TopoDS_Wire&, bool> {
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

bool Extrusion::isInside(const TopoDS_Wire& wire1, const TopoDS_Wire& wire2) const
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
    Handle_Geom_Surface surf = new Geom_Plane(adapt.Plane());
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
        xp.Next();
    }

    return false;
}

TopoDS_Shape Extrusion::makeFace(std::list<TopoDS_Wire>& wires) const
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

TopoDS_Shape Extrusion::makeFace(const std::vector<TopoDS_Wire>& w) const
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
