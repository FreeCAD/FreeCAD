/***************************************************************************
 *   Copyright (c) 2014 Nathan Miller         <Nathan.A.Mill[at]gmail.com> *
 *                      Balázs Bámer                                       *
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
#include <Geom_BezierCurve.hxx>
#include <Precision.hxx>
#include <gp_Trsf.hxx>
#include <GeomFill.hxx>
#include <GeomFill_BezierCurves.hxx>
#include <BRepBuilderAPI_MakeFace.hxx>
#include <BRep_Tool.hxx>
#include <TopExp_Explorer.hxx>
#include <Standard_ConstructionError.hxx>
#include <Base/Tools.h>
#include <Base/Exception.h>
#endif

#include "FeatureBezSurf.h"


using namespace Surface;

PROPERTY_SOURCE(Surface::BezSurf, Part::Feature)

//Initial values

BezSurf::BezSurf()
{
    ADD_PROPERTY(aBList,(0,"Geom_BezierCurve"));
    ADD_PROPERTY(filltype,(1));

}

//Structures

struct crvs{

    Handle_Geom_BezierCurve C1;
    Handle_Geom_BezierCurve C2;
    Handle_Geom_BezierCurve C3;
    Handle_Geom_BezierCurve C4;

};

//Functions

App::DocumentObjectExecReturn *BezSurf::execute(void)
{
    //Begin Construction
    try{
        GeomFill_FillingStyle fstyle = getFillingStyle();
        GeomFill_BezierCurves aSurfBuilder; //Create Surface Builder
        TopoDS_Wire aWire; //Create empty wire

        //Gets the healed wire
        getWire(aWire);

        //Create Bezier Surface builder
        crvs bcrv;
        Standard_Real u0;// contains output
        Standard_Real u1;// contains output
        TopExp_Explorer anExp (aWire, TopAbs_EDGE);
        int it = 0;
        for (; anExp.More(); anExp.Next()) {
            const TopoDS_Edge hedge = TopoDS::Edge (anExp.Current());
            TopLoc_Location heloc; // this will be output
            Handle_Geom_Curve c_geom = BRep_Tool::Curve(hedge,heloc,u0,u1); //The geometric curve
            Handle_Geom_BezierCurve b_geom = Handle_Geom_BezierCurve::DownCast(c_geom); //Try to get Bezier curve

            if (!b_geom.IsNull()) {
                gp_Trsf transf = heloc.Transformation();
                b_geom->Transform(transf); // apply original transformation to control points
                //Store Underlying Geometry
                if(it==0){bcrv.C1 = b_geom;}
                else if(it==1){bcrv.C2 = b_geom;}
                else if(it==2){bcrv.C3 = b_geom;}
                else if(it==3){bcrv.C4 = b_geom;}
            }
            else {
                Standard_Failure::Raise("Curve not a Bezier Curve");
            }
            it++;
        }

        int ncrv = aBList.getSize();
        if(ncrv==2){aSurfBuilder.Init(bcrv.C1,bcrv.C2,fstyle);}
        else if(ncrv==3){aSurfBuilder.Init(bcrv.C1,bcrv.C2,bcrv.C3,fstyle);}
        else if(ncrv==4){aSurfBuilder.Init(bcrv.C1,bcrv.C2,bcrv.C3,bcrv.C4,fstyle);}

        //Create the surface
        const Handle_Geom_BezierSurface aSurface = aSurfBuilder.Surface();

        BRepBuilderAPI_MakeFace aFaceBuilder;//(aSurface,aWire,Standard_True); //Create Face Builder
        u0 = 0.;
        u1 = 1.;
        Standard_Real v0 = 0.;
        Standard_Real v1 = 1.;
        aFaceBuilder.Init(aSurface,u0,u1,v0,v1,Precision::Confusion());

        TopoDS_Face aFace = aFaceBuilder.Face(); //Returned Face
        if(!aFaceBuilder.IsDone()){return new App::DocumentObjectExecReturn("Face unable to be constructed");}

        if (aFace.IsNull()){
            return new App::DocumentObjectExecReturn("Resulting Face is null");
        }
        this->Shape.setValue(aFace);

        return App::DocumentObject::StdReturn;

    } //End Try
    catch(Standard_ConstructionError) {
        // message is in a Latin language, show a normal one
        return new App::DocumentObjectExecReturn("Curves are disjoint.");
    }
    catch (Standard_Failure) {
        Handle_Standard_Failure e = Standard_Failure::Caught();
        return new App::DocumentObjectExecReturn(e->GetMessageString());
    } //End Catch

} //End execute


