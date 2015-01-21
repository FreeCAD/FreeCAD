/***************************************************************************
 *   Copyright (c) 2014-2015 Nathan Miller    <Nathan.A.Mill[at]gmail.com> *
 *                           Balázs Bámer                                  *
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
#include <TopoDS.hxx>
#include <TopoDS_Face.hxx>
#include <TopoDS_Edge.hxx>
#include <TopoDS_Wire.hxx>
#include <Geom_BezierCurve.hxx>
#include <Precision.hxx>
#include <gp_Trsf.hxx>
#include <BRep_Tool.hxx>
#include <TopExp_Explorer.hxx>
#include <Standard_ConstructionError.hxx>
#include <Base/Tools.h>
#include <Base/Exception.h>
#include <GeomFill_BezierCurves.hxx>
#include <GeomFill.hxx>
#endif

#include "FeatureBezSurf.h"


using namespace Surface;

PROPERTY_SOURCE(Surface::BezSurf, Surface::BSurf)

//Initial values

BezSurf::BezSurf() : BSurf()
{
}

//Functions

App::DocumentObjectExecReturn *BezSurf::execute(void)
{
    //Begin Construction
    try{
        Handle_Geom_BezierCurve crvs[4];
        TopoDS_Wire aWire; //Create empty wire

        //Gets the healed wire
        getWire(aWire);

        Standard_Real u1, u2; // contains output
        TopExp_Explorer anExp (aWire, TopAbs_EDGE);
        int it = 0;
        for (; anExp.More(); anExp.Next()) {
            const TopoDS_Edge hedge = TopoDS::Edge (anExp.Current());
            TopLoc_Location heloc; // this will be output
            Handle_Geom_Curve c_geom = BRep_Tool::Curve(hedge, heloc, u1, u2); //The geometric curve
            Handle_Geom_BezierCurve b_geom = Handle_Geom_BezierCurve::DownCast(c_geom); //Try to get Bezier curve

            if (!b_geom.IsNull()) {
                gp_Trsf transf = heloc.Transformation();
                b_geom->Transform(transf); // apply original transformation to control points
                //Store Underlying Geometry
                crvs[it] = b_geom;
            }
            else {
                Standard_Failure::Raise("Curve not a Bezier Curve");
            }
            it++;
        }

        GeomFill_FillingStyle fstyle = getFillingStyle();
        GeomFill_BezierCurves aSurfBuilder; //Create Surface Builder
        int ncrv = aBList.getSize();
        if(ncrv==2) {aSurfBuilder.Init(crvs[0], crvs[1], fstyle);}
        else if(ncrv==3) {aSurfBuilder.Init(crvs[0], crvs[1], crvs[2], fstyle);}
        else if(ncrv==4) {aSurfBuilder.Init(crvs[0], crvs[1], crvs[2], crvs[3], fstyle);}

        createFace(aSurfBuilder.Surface());

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


