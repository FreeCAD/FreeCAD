/***************************************************************************
 *   Copyright (c) 2014 Nathan Miller         <Nathan.A.Mill[at]gmail.com> *
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
#include <BRepBuilderAPI_MakeFace.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Face.hxx>
#include <TopoDS_Edge.hxx>
#include <TopoDS_Wire.hxx>
#include <Geom_BSplineCurve.hxx>
#include <Precision.hxx>
#endif

#include "FeatureBSplineSurf.h"
#include <GeomFill.hxx>
#include <GeomFill_BSplineCurves.hxx>
#include <ShapeFix_Wire.hxx>
#include <ShapeExtend_WireData.hxx>
#include <BRep_Tool.hxx>
#include <BRepBuilderAPI_MakeFace.hxx>
#include <BRepBuilderAPI_MakeWire.hxx>
#include <Base/Tools.h>
#include <Base/Exception.h>
#include <TopExp_Explorer.hxx>

using namespace Surface;

PROPERTY_SOURCE(Surface::BSplineSurf, Part::Feature)

//Initial values

BSplineSurf::BSplineSurf()
{
    ADD_PROPERTY(aBSplineList,(0,"Geom_BSplineCurve"));
    ADD_PROPERTY(filltype,(1));

}

//Structures

struct crvs{

    Handle_Geom_BSplineCurve C1;
    Handle_Geom_BSplineCurve C2;
    Handle_Geom_BSplineCurve C3;
    Handle_Geom_BSplineCurve C4;

};

//Functions

void getCurves(GeomFill_BSplineCurves& aBuilder,TopoDS_Wire& aWire, const App::PropertyLinkSubList& anEdge, GeomFill_FillingStyle fstyle);
//bool orderCurves(crvs& Cs, int size);

//Check if any components of the surface have been modified

short BSplineSurf::mustExecute() const
{
    if (aBSplineList.isTouched() ||
        filltype.isTouched())
        return 1;
    return 0;
}

App::DocumentObjectExecReturn *BSplineSurf::execute(void)
{

    //Set Variables

    int ftype = filltype.getValue();

    //Begin Construction
    try{

        //Identify filling style

        GeomFill_FillingStyle fstyle;

        if(ftype==1){fstyle = GeomFill_StretchStyle;}
        else if(ftype==2){fstyle = GeomFill_CoonsStyle;}
        else if(ftype==3){fstyle = GeomFill_CurvedStyle;}
        else{return new App::DocumentObjectExecReturn("Filling style must be 1 (Stretch), 2 (Coons), or 3 (Curved).");}

        //Create BSpline Surface

        GeomFill_BSplineCurves aSurfBuilder; //Create Surface Builder
//        BRepBuilderAPI_MakeWire aWireBuilder; //Create Wire Builder
        TopoDS_Wire aWire; //Create empty wire

        //Get BSpline Curves from edges and initialize the builder

        printf("Entering getCurves\n");
        getCurves(aSurfBuilder,aWire,aBSplineList,fstyle);

        //Create the surface
        printf("Creating the Surface\n");
        const Handle_Geom_BSplineSurface aSurface = aSurfBuilder.Surface();

        printf("Creating the Face\n");
        BRepBuilderAPI_MakeFace aFaceBuilder(aSurface,aWire,Standard_True); //Create Face Builder
//        Standard_Real u0 = 0.;
//        Standard_Real u1 = 2.;
//        Standard_Real v0 = 0.;
//        Standard_Real v1 = 2.;
//        aFaceBuilder.Init(aSurface,u0,u1,v0,v1,Precision::Confusion());

        printf("Returning the Face\n");
        TopoDS_Face aFace = aFaceBuilder.Face(); //Returned Face
        if(!aFaceBuilder.IsDone()){return new App::DocumentObjectExecReturn("Face unable to be constructed");}

        if (aFace.IsNull()){
            return new App::DocumentObjectExecReturn("Resulting Face is null");
        }
        this->Shape.setValue(aFace);

        return App::DocumentObject::StdReturn;

    } //End Try
    catch (Standard_Failure) {
        Handle_Standard_Failure e = Standard_Failure::Caught();
        return new App::DocumentObjectExecReturn(e->GetMessageString());
    } //End Catch

} //End execute

void getCurves(GeomFill_BSplineCurves& aBuilder,TopoDS_Wire& aWire, const App::PropertyLinkSubList& anEdge, GeomFill_FillingStyle fstyle){
//void getCurves(TopoDS_Wire& aWire, const App::PropertyLinkSubList& anEdge){

    crvs bcrv;

    Standard_Real u0 = 0.;
    Standard_Real u1 = 1.;

    Handle(ShapeFix_Wire) aShFW = new ShapeFix_Wire;
    Handle(ShapeExtend_WireData) aWD = new ShapeExtend_WireData;

    if(anEdge.getSize()>4){Standard_Failure::Raise("Only 2-4 continuous BSpline Curves are allowed");return;}
    if(anEdge.getSize()<2){Standard_Failure::Raise("Only 2-4 continuous BSpline Curves are allowed");return;}

    for(int i=0; i<anEdge.getSize(); i++){
        
        Part::TopoShape ts; //Curve TopoShape
        TopoDS_Shape sub;   //Curve TopoDS_Shape
        TopoDS_Edge etmp;   //Curve TopoDS_Edge
        

        //Get Edge
        App::PropertyLinkSubList::SubSet set = anEdge[i];

        if(set.obj->getTypeId().isDerivedFrom(Part::Feature::getClassTypeId())) {
       
            ts = static_cast<Part::Feature*>(set.obj)->Shape.getShape();
               
            //we want only the subshape which is linked
            sub = ts.getSubShape(set.sub);
            
            if(sub.ShapeType() == TopAbs_EDGE) {etmp = TopoDS::Edge(sub);} //Check Shape type and assign edge
            else{Standard_Failure::Raise("Curves must be type TopoDS_Edge");return;} //Raise exception

            aWD->Add(etmp);         

        }

        else{Standard_Failure::Raise("Curve not from Part::Feature");return;}

    }

    //Reorder the curves and fix the wire if required

    aShFW->Load(aWD); //Load in the wire
    aShFW->FixReorder(); //Fix the order of the edges if required
    aShFW->ClosedWireMode() = Standard_True; //Enables closed wire mode
    aShFW->FixConnected(); //Fix connection between wires
    aShFW->FixSelfIntersection(); //Fix Self Intersection
    aShFW->Perform(); //Perform the fixes

    aWire = aShFW->Wire(); //Healed Wire

    if(aWire.IsNull()){Standard_Failure::Raise("Wire unable to be constructed");return;}

    //Create BSpline Surface

    TopExp_Explorer anExp (aWire, TopAbs_EDGE);
    int it = 0;
    for (; anExp.More(); anExp.Next()) {
        const TopoDS_Edge hedge = TopoDS::Edge (anExp.Current());
        TopLoc_Location heloc = hedge.Location();
        Handle_Geom_Curve c_geom = BRep_Tool::Curve(hedge,heloc,u0,u1); //The geometric curve
        Handle_Geom_BSplineCurve b_geom = Handle_Geom_BSplineCurve::DownCast(c_geom); //Try to get BSpline curve
        
        if (!b_geom.IsNull()) {
            //Store Underlying Geometry
            if(it==0){bcrv.C1 = b_geom;}
            else if(it==1){bcrv.C2 = b_geom;}
            else if(it==2){bcrv.C3 = b_geom;}
            else if(it==3){bcrv.C4 = b_geom;}

        }
        else {
            Standard_Failure::Raise("Curve not a BSpline Curve");
            return;
        }      

        it++;
    }

    int ncrv = anEdge.getSize();

    if(ncrv==2){aBuilder.Init(bcrv.C1,bcrv.C2,fstyle);}
    else if(ncrv==3){aBuilder.Init(bcrv.C1,bcrv.C2,bcrv.C3,fstyle);}
    else if(ncrv==4){aBuilder.Init(bcrv.C1,bcrv.C2,bcrv.C3,bcrv.C4,fstyle);}

    return;
}
