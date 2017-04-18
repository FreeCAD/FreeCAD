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
# include <cassert>
# include <gp_Pln.hxx>
# include <gp_Lin.hxx>
# include <Adaptor3d_HCurveOnSurface.hxx>
# include <Geom_BSplineSurface.hxx>
# include <Geom_Plane.hxx>
# include <GeomAdaptor_HCurve.hxx>
# include <GeomAPI_IntSS.hxx>
# include <Geom_Line.hxx>
# include <Geom_Point.hxx>
# include <GeomPlate_BuildPlateSurface.hxx>
# include <GeomPlate_CurveConstraint.hxx>
# include <GeomPlate_MakeApprox.hxx>
# include <GeomPlate_PlateG0Criterion.hxx>
# include <GeomPlate_PointConstraint.hxx>
# include <Precision.hxx>
# include <Standard_Mutex.hxx>
# include <Standard_TypeMismatch.hxx>
# include <TColStd_ListOfTransient.hxx>
# include <TColStd_ListIteratorOfListOfTransient.hxx>
# include <TColgp_SequenceOfXY.hxx>
# include <TColgp_SequenceOfXYZ.hxx>
#endif

#include <Base/Vector3D.h>
#include "Tools.h"

void Part::closestPointsOnLines(const gp_Lin& lin1, const gp_Lin& lin2, gp_Pnt& p1, gp_Pnt& p2)
{
    // they might be the same point
    gp_Vec v1(lin1.Direction());
    gp_Vec v2(lin2.Direction());
    gp_Vec v3(lin2.Location(), lin1.Location());

    double a = v1*v1;
    double b = v1*v2;
    double c = v2*v2;
    double d = v1*v3;
    double e = v2*v3;
    double D = a*c - b*b;
    double s, t;

    // D = (v1 x v2) * (v1 x v2)
    if (D < Precision::Angular()){
        // the lines are considered parallel
        s = 0.0;
        t = (b>c ? d/b : e/c);
    }
    else {
        s = (b*e - c*d) / D;
        t = (a*e - b*d) / D;
    }

    p1 = lin1.Location().XYZ() + s * v1.XYZ();
    p2 = lin2.Location().XYZ() + t * v2.XYZ();
}

bool Part::intersect(const gp_Pln& pln1, const gp_Pln& pln2, gp_Lin& lin)
{
    bool found = false;
    Handle (Geom_Plane) gp1 = new Geom_Plane(pln1);
    Handle (Geom_Plane) gp2 = new Geom_Plane(pln2);

    GeomAPI_IntSS intSS(gp1, gp2, Precision::Confusion());
    if (intSS.IsDone()) {
        int numSol = intSS.NbLines();
        if (numSol > 0) {
            Handle(Geom_Curve) curve = intSS.Line(1);
            lin = Handle(Geom_Line)::DownCast(curve)->Lin();
            found = true;
        }
    }

    return found;
}

/*! The objects in \a theBoundaries must be of the type Adaptor3d_HCurveOnSurface or
GeomAdaptor_HCurve or Geom_Point indicating type of a constraint. Otherwise an exception
Standard_TypeMismatch is thrown.

If the \a theBoundaries list is empty then Standard_ConstructionError is thrown.

If the algorithm fails it returns a null surface.
\see http://opencascade.blogspot.com/2010/03/surface-modeling-part6.html
*/
Handle(Geom_Surface)
Part::Tools::makeSurface(const TColStd_ListOfTransient &theBoundaries,
                         const Standard_Real theTol,
                         const Standard_Integer theNbPnts,
                         const Standard_Integer theNbIter,
                         const Standard_Integer theMaxDeg)
{
    (void)theTol;
    //constants for algorithm
    const Standard_Integer aNbIter = theNbIter; //number of algorithm iterations
    const Standard_Integer aNbPnts = theNbPnts; //sample points per each constraint
    const Standard_Integer aDeg = 3; //requested surface degree ?
    const Standard_Integer aMaxDeg = theMaxDeg;
    const Standard_Integer aMaxSeg = 10000;
    const Standard_Real aTol3d = 1.e-04;
    const Standard_Real aTol2d = 1.e-05;
    const Standard_Real anAngTol = 1.e-02; //angular
    const Standard_Real aCurvTol = 1.e-01; //curvature

    Handle(Geom_Surface) aRes;
    GeomPlate_BuildPlateSurface aPlateBuilder (aDeg, aNbPnts, aNbIter, aTol2d, aTol3d, anAngTol, aCurvTol);

    TColStd_ListIteratorOfListOfTransient anIt (theBoundaries);
    if (anIt.More()) {
        int i = 1;
        for (; anIt.More(); anIt.Next(), i++) {
            const Handle(Standard_Transient)& aCur = anIt.Value();
            if (aCur.IsNull()) {
                assert (0);
                Standard_ConstructionError::Raise ("Tools::makeSurface()");
            }
            else if (aCur->IsKind (STANDARD_TYPE (Adaptor3d_HCurveOnSurface))) {
                //G1 constraint
                Handle(Adaptor3d_HCurveOnSurface) aHCOS (Handle(Adaptor3d_HCurveOnSurface)::DownCast (aCur));
                Handle (GeomPlate_CurveConstraint) aConst = new GeomPlate_CurveConstraint (aHCOS, 1 /*GeomAbs_G1*/,aNbPnts, aTol3d, anAngTol, aCurvTol);
                aPlateBuilder.Add (aConst);
            }
            else if (aCur->IsKind (STANDARD_TYPE (GeomAdaptor_HCurve))) {
                //G0 constraint
                Handle(GeomAdaptor_HCurve) aHC (Handle(GeomAdaptor_HCurve)::DownCast (aCur));
                Handle (GeomPlate_CurveConstraint) aConst = new GeomPlate_CurveConstraint (aHC, 0 /*GeomAbs_G0*/, aNbPnts, aTol3d);
                aPlateBuilder.Add (aConst);
            }
            else if (aCur->IsKind (STANDARD_TYPE (Geom_Point))) {
                //Point constraint
                Handle(Geom_Point) aGP (Handle(Geom_Point)::DownCast (aCur));
                Handle(GeomPlate_PointConstraint) aConst = new GeomPlate_PointConstraint(aGP->Pnt(),0);
                aPlateBuilder.Add(aConst);
            }
            else {
                Standard_TypeMismatch::Raise ("Tools::makeSurface()");
            }
        }
    }
    else {
        Standard_ConstructionError::Raise ("Tools::makeSurface()");
    }

    //construct
    aPlateBuilder.Perform();

    if (!aPlateBuilder.IsDone()) {
        return aRes;
    }

    const Handle(GeomPlate_Surface)& aPlate = aPlateBuilder.Surface();
    //approximation (see BRepFill_Filling - when no initial surface was given)
    Standard_Real aDMax = aPlateBuilder.G0Error();
    TColgp_SequenceOfXY aS2d;
    TColgp_SequenceOfXYZ aS3d;
    aPlateBuilder.Disc2dContour (4, aS2d);
    aPlateBuilder.Disc3dContour (4, 0, aS3d);
    Standard_Real aMax = Max (aTol3d, 10. * aDMax);
    GeomPlate_PlateG0Criterion aCriterion (aS2d, aS3d, aMax);
    {
        //data races in AdvApp2Var used by GeomApprox_Surface, use global mutex
        //Standard_Mutex::Sentry aSentry (theBSMutex);
        GeomPlate_MakeApprox aMakeApprox (aPlate, aCriterion, aTol3d, aMaxSeg, aMaxDeg);
        aRes = aMakeApprox.Surface();
    }

    return aRes;
}
