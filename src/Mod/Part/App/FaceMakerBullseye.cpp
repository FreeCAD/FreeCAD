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
# include <BRepBuilderAPI_MakeFace.hxx>
# include <BRepCheck_Analyzer.hxx>
# include <BRepClass_FaceClassifier.hxx>
# include <BRepLib_FindSurface.hxx>
# include <Geom_Plane.hxx>
# include <GeomAPI_ProjectPointOnSurf.hxx>
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

#include "FaceMakerBullseye.h"
#include "FaceMakerCheese.h"

#include "TopoShape.h"



using namespace Part;

TYPESYSTEM_SOURCE(Part::FaceMakerBullseye, Part::FaceMakerPublic)

void FaceMakerBullseye::setPlane(const gp_Pln &plane)
{
    this->myPlane = gp_Pln(plane);
    this->planeSupplied = true;
}

std::string FaceMakerBullseye::getUserFriendlyName() const
{
    return std::string(QT_TRANSLATE_NOOP("Part_FaceMaker","Bull's-eye facemaker"));
}

std::string FaceMakerBullseye::getBriefExplanation() const
{
    return std::string(QT_TRANSLATE_NOOP("Part_FaceMaker","Supports making planar faces with holes with islands."));
}

void FaceMakerBullseye::Build_Essence()
{
    if(myWires.empty())
        return;

    //validity check
    for(TopoDS_Wire &w : myWires){
        if (!BRep_Tool::IsClosed(w))
            throw Base::ValueError("Wire is not closed.");
    }


    //find plane (at the same time, test that all wires are on the same plane)
    gp_Pln plane;
    if(this->planeSupplied){
        plane = this->myPlane;
    } else {
        TopoDS_Builder builder;
        TopoDS_Compound comp;
        builder.MakeCompound(comp);
        for(TopoDS_Wire &w : myWires){
            builder.Add(comp, w);
        }
        BRepLib_FindSurface planeFinder(comp,-1, /*OnlyPlane=*/Standard_True);
        if (!planeFinder.Found())
            throw Base::ValueError("Wires are not coplanar.");
        plane = GeomAdaptor_Surface(planeFinder.Surface()).Plane();
    }

    //sort wires by length of diagonal of bounding box.
    std::vector<TopoDS_Wire> wires = this->myWires;
    std::stable_sort(wires.begin(), wires.end(), FaceMakerCheese::Wire_Compare());

    //add wires one by one to current set of faces.
    //We go from last to first, to make it so that outer wires come before inner wires.
    std::vector< std::unique_ptr<FaceDriller> > faces;
    for (int i = static_cast<int>(wires.size())-1; i >= 0; --i) {
        TopoDS_Wire &w = wires[i];

        //test if this wire is on any of existing faces (if yes, it's a hole;
        // if no, it's a beginning of a new face).
        //Since we are assuming the wires do not intersect, testing if one vertex of wire is in a face is enough.
        gp_Pnt p = BRep_Tool::Pnt(TopoDS::Vertex(TopExp_Explorer(w, TopAbs_VERTEX).Current()));
        FaceDriller* foundFace = nullptr;
        for(std::unique_ptr<FaceDriller> &ff : faces){
            if(ff->hitTest(p)){
                foundFace = &(*ff);
                break;
            }
        }

        if(foundFace){
            //wire is on a face.
            foundFace->addHole(w);
        } else {
            //wire is not on a face. Start a new face.
            faces.push_back(std::unique_ptr<FaceDriller>(
                                new FaceDriller(plane, w)
                           ));
        }
    }

    //and we are done!
    for(std::unique_ptr<FaceDriller> &ff : faces){
        this->myShapesToReturn.push_back(ff->Face());
    }
}


FaceMakerBullseye::FaceDriller::FaceDriller(gp_Pln plane, TopoDS_Wire outerWire)
{
    this->myPlane = plane;
    this->myFace = TopoDS_Face();

    //Ensure correct orientation of the wire.
    if (getWireDirection(myPlane, outerWire) < 0)
        outerWire.Reverse();

    myHPlane = new Geom_Plane(this->myPlane);
    BRep_Builder builder;
    builder.MakeFace(this->myFace, myHPlane, Precision::Confusion());
    builder.Add(this->myFace, outerWire);
}

bool FaceMakerBullseye::FaceDriller::hitTest(gp_Pnt point) const
{
    double u,v;
    GeomAPI_ProjectPointOnSurf(point, myHPlane).LowerDistanceParameters(u,v);
    BRepClass_FaceClassifier cl(myFace, gp_Pnt2d(u,v), Precision::Confusion());
    TopAbs_State ret = cl.State();
    switch(ret){
        case TopAbs_UNKNOWN:
            throw Base::ValueError("FaceMakerBullseye::FaceDriller::hitTest: result unknown.");
        break;
        default:
            return ret == TopAbs_IN || ret == TopAbs_ON;
    }

}

void FaceMakerBullseye::FaceDriller::addHole(TopoDS_Wire w)
{
    //Ensure correct orientation of the wire.
    if (getWireDirection(myPlane, w) > 0) //if wire is CCW..
        w.Reverse();   //.. we want CW!

    BRep_Builder builder;
    builder.Add(this->myFace, w);
}

int FaceMakerBullseye::FaceDriller::getWireDirection(const gp_Pln& plane, const TopoDS_Wire& wire)
{
    //make a test face
    BRepBuilderAPI_MakeFace mkFace(wire, /*onlyplane=*/Standard_True);
    TopoDS_Face tmpFace = mkFace.Face();
    //compare face surface normal with our plane's one
    BRepAdaptor_Surface surf(tmpFace);
    bool normal_co = surf.Plane().Axis().Direction().Dot(plane.Axis().Direction()) > 0;

    //unlikely, but just in case OCC decided to reverse our wire for the face...  take that into account!
    TopoDS_Iterator it(tmpFace, /*CumOri=*/Standard_False);
    normal_co ^= it.Value().Orientation() != wire.Orientation();

    return normal_co ? 1 : -1;
}
