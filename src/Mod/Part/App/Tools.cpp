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
# include <BRep_Tool.hxx>
# include <Geom_BSplineSurface.hxx>
# include <Geom_Plane.hxx>
# include <GeomAPI_IntSS.hxx>
# include <GeomAPI_ProjectPointOnSurf.hxx>
# include <Geom_Line.hxx>
# include <Geom_Point.hxx>
# include <GeomAdaptor_Curve.hxx>
# include <GeomLib.hxx>
# include <GeomPlate_BuildPlateSurface.hxx>
# include <GeomPlate_CurveConstraint.hxx>
# include <GeomPlate_MakeApprox.hxx>
# include <GeomPlate_PlateG0Criterion.hxx>
# include <GeomPlate_PointConstraint.hxx>
# include <Poly_Connect.hxx>
# include <Poly_Triangulation.hxx>
# include <Precision.hxx>
# include <Standard_Mutex.hxx>
# include <Standard_TypeMismatch.hxx>
# include <Standard_Version.hxx>
# include <TColStd_ListOfTransient.hxx>
# include <TColStd_ListIteratorOfListOfTransient.hxx>
# include <TColgp_SequenceOfXY.hxx>
# include <TColgp_SequenceOfXYZ.hxx>
# include <TopoDS.hxx>
# if OCC_VERSION_HEX < 0x070600
# include <Adaptor3d_HCurveOnSurface.hxx>
# include <GeomAdaptor_HCurve.hxx>
# endif
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
#if OCC_VERSION_HEX >= 0x070600
            else if (aCur->IsKind (STANDARD_TYPE (Adaptor3d_CurveOnSurface))) {
                //G1 constraint
                Handle(Adaptor3d_CurveOnSurface) aHCOS (Handle(Adaptor3d_CurveOnSurface)::DownCast (aCur));
                Handle (GeomPlate_CurveConstraint) aConst = new GeomPlate_CurveConstraint (aHCOS, 1 /*GeomAbs_G1*/,aNbPnts, aTol3d, anAngTol, aCurvTol);
                aPlateBuilder.Add (aConst);
            }
            else if (aCur->IsKind (STANDARD_TYPE (GeomAdaptor_Curve))) {
                //G0 constraint
                Handle(GeomAdaptor_Curve) aHC (Handle(GeomAdaptor_Curve)::DownCast (aCur));
                Handle (GeomPlate_CurveConstraint) aConst = new GeomPlate_CurveConstraint (aHC, 0 /*GeomAbs_G0*/, aNbPnts, aTol3d);
                aPlateBuilder.Add (aConst);
            }
#else
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
#endif
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

bool Part::Tools::getTriangulation(const TopoDS_Face& face, std::vector<gp_Pnt>& points, std::vector<Poly_Triangle>& facets)
{
    TopLoc_Location loc;
    Handle(Poly_Triangulation) hTria = BRep_Tool::Triangulation(face, loc);
    if (hTria.IsNull())
        return false;

    // getting the transformation of the face
    gp_Trsf transf;
    bool identity = true;
    if (!loc.IsIdentity()) {
        identity = false;
        transf = loc.Transformation();
    }

    // check orientation
    TopAbs_Orientation orient = face.Orientation();

    Standard_Integer nbNodes = hTria->NbNodes();
    Standard_Integer nbTriangles = hTria->NbTriangles();
#if OCC_VERSION_HEX < 0x070600
    const TColgp_Array1OfPnt& nodes = hTria->Nodes();
    const Poly_Array1OfTriangle& triangles = hTria->Triangles();
#endif

    points.reserve(nbNodes);
    facets.reserve(nbTriangles);

    // cycling through the poly mesh
    //
    for (int i = 1; i <= nbNodes; i++) {
#if OCC_VERSION_HEX < 0x070600
        gp_Pnt p = nodes(i);
#else
        gp_Pnt p = hTria->Node(i);
#endif

        // transform the vertices to the location of the face
        if (!identity) {
            p.Transform(transf);
        }

        points.push_back(p);
    }

    for (int i = 1; i <= nbTriangles; i++) {
        // Get the triangle
        Standard_Integer n1,n2,n3;
#if OCC_VERSION_HEX < 0x070600
        triangles(i).Get(n1, n2, n3);
#else
        hTria->Triangle(i).Get(n1, n2, n3);
#endif
        --n1; --n2; --n3;

        // change orientation of the triangles
        if (orient != TopAbs_FORWARD) {
            std::swap(n1, n2);
        }

        facets.emplace_back(n1, n2, n3);
    }

    return true;
}

bool Part::Tools::getPolygonOnTriangulation(const TopoDS_Edge& edge, const TopoDS_Face& face, std::vector<gp_Pnt>& points)
{
    TopLoc_Location loc;
    Handle(Poly_Triangulation) hTria = BRep_Tool::Triangulation(face, loc);
    if (hTria.IsNull())
        return false;

    // this holds the indices of the edge's triangulation to the actual points
    Handle(Poly_PolygonOnTriangulation) hPoly = BRep_Tool::PolygonOnTriangulation(edge, hTria, loc);
    if (hPoly.IsNull())
        return false;

    // getting the transformation of the edge
    gp_Trsf transf;
    bool identity = true;
    if (!loc.IsIdentity()) {
        identity = false;
        transf = loc.Transformation();
    }

    // getting size and create the array
    Standard_Integer nbNodes = hPoly->NbNodes();
    points.reserve(nbNodes);
    const TColStd_Array1OfInteger& indices = hPoly->Nodes();
#if OCC_VERSION_HEX < 0x070600
    const TColgp_Array1OfPnt& Nodes = hTria->Nodes();
#endif

    // go through the index array
    for (Standard_Integer i = indices.Lower(); i <= indices.Upper(); i++) {
#if OCC_VERSION_HEX < 0x070600
        gp_Pnt p = Nodes(indices(i));
#else
        gp_Pnt p = hTria->Node(indices(i));
#endif
        if (!identity) {
            p.Transform(transf);
        }

        points.push_back(p);
    }

    return true;
}

bool Part::Tools::getPolygon3D(const TopoDS_Edge& edge, std::vector<gp_Pnt>& points)
{
    TopLoc_Location loc;
    Handle(Poly_Polygon3D) hPoly = BRep_Tool::Polygon3D(edge, loc);
    if (hPoly.IsNull())
        return false;

    // getting the transformation of the edge
    gp_Trsf transf;
    bool identity = true;
    if (!loc.IsIdentity()) {
        identity = false;
        transf = loc.Transformation();
    }

    // getting size and create the array
    Standard_Integer nbNodes = hPoly->NbNodes();
    points.reserve(nbNodes);
    const TColgp_Array1OfPnt& nodes = hPoly->Nodes();

    for (int i = 1; i <= nbNodes; i++) {
        gp_Pnt p = nodes(i);

        // transform the vertices to the location of the face
        if (!identity) {
            p.Transform(transf);
        }

        points.push_back(p);
    }

    return true;
}

void Part::Tools::getPointNormals(const std::vector<gp_Pnt>& points, const std::vector<Poly_Triangle>& facets, std::vector<gp_Vec>& vertexnormals)
{
    vertexnormals.resize(points.size());

    for (const auto& it : facets) {
        // Get the triangle
        Standard_Integer n1,n2,n3;
        it.Get(n1,n2,n3);

        // Calculate triangle normal
        gp_Vec v1(points[n1].XYZ());
        gp_Vec v2(points[n2].XYZ());
        gp_Vec v3(points[n3].XYZ());
        gp_Vec n = (v2 - v1) ^ (v3 - v1);

        // add the triangle normal to the vertex normal for all points of this triangle
        vertexnormals[n1] += n;
        vertexnormals[n2] += n;
        vertexnormals[n3] += n;
    }

    for (auto& it : vertexnormals)
        it.Normalize();
}

void Part::Tools::getPointNormals(const std::vector<gp_Pnt>& points, const TopoDS_Face& face, std::vector<gp_Vec>& vertexnormals)
{
    if (points.size() != vertexnormals.size())
        return;

    Handle(Geom_Surface) hSurface = BRep_Tool::Surface(face);
    if (hSurface.IsNull())
        return;

    // normalize all vertex normals
    for (std::size_t i = 0; i < points.size(); i++) {
        try {
            GeomAPI_ProjectPointOnSurf ProPntSrf(points[i], hSurface);
            Standard_Real u, v;
            ProPntSrf.Parameters(1, u, v);

            GeomLProp_SLProps propOfFace(hSurface, u, v, 2, gp::Resolution());

            gp_Dir normal = propOfFace.Normal();
            gp_Vec temp = normal;
            if (temp * vertexnormals[i] < 0.0)
                temp = -temp;
            vertexnormals[i] = temp;

        }
        catch (...) {
        }

        vertexnormals[i].Normalize();
    }
}

void Part::Tools::getPointNormals(const TopoDS_Face& theFace, Handle(Poly_Triangulation) aPolyTri, TColgp_Array1OfDir& theNormals)
{
    const TColgp_Array1OfPnt& aNodes = aPolyTri->Nodes();

    if(aPolyTri->HasNormals())
    {
        // normals pre-computed in triangulation structure
        const TShort_Array1OfShortReal& aNormals = aPolyTri->Normals();
        const Standard_ShortReal*       aNormArr = &(aNormals.Value(aNormals.Lower()));

        for(Standard_Integer aNodeIter = aNodes.Lower(); aNodeIter <= aNodes.Upper(); ++aNodeIter)
        {
            const Standard_Integer anId = 3 * (aNodeIter - aNodes.Lower());
            const gp_Dir aNorm(aNormArr[anId + 0],
                               aNormArr[anId + 1],
                               aNormArr[anId + 2]);
            theNormals(aNodeIter) = aNorm;
        }

        if(theFace.Orientation() == TopAbs_REVERSED)
        {
            for(Standard_Integer aNodeIter = aNodes.Lower(); aNodeIter <= aNodes.Upper(); ++aNodeIter)
            {
                theNormals.ChangeValue(aNodeIter).Reverse();
            }
        }
    }
    else {
        // take in face the surface location
        Poly_Connect thePolyConnect(aPolyTri);
        const TopoDS_Face      aZeroFace = TopoDS::Face(theFace.Located(TopLoc_Location()));
        Handle(Geom_Surface)   aSurf     = BRep_Tool::Surface(aZeroFace);
        const Standard_Real    aTol      = Precision::Confusion();
        Handle(TShort_HArray1OfShortReal) aNormals = new TShort_HArray1OfShortReal(1, aPolyTri->NbNodes() * 3);
        const Poly_Array1OfTriangle& aTriangles = aPolyTri->Triangles();
        const TColgp_Array1OfPnt2d*  aNodesUV   = aPolyTri->HasUVNodes() && !aSurf.IsNull()
                ? &aPolyTri->UVNodes()
                : nullptr;
        Standard_Integer aTri[3];

        for(Standard_Integer aNodeIter = aNodes.Lower(); aNodeIter <= aNodes.Upper(); ++aNodeIter)
        {
            // try to retrieve normal from real surface first, when UV coordinates are available
            if (!aNodesUV || GeomLib::NormEstim(aSurf, aNodesUV->Value(aNodeIter), aTol, theNormals(aNodeIter)) > 1)
            {
                // compute flat normals
                gp_XYZ eqPlan(0.0, 0.0, 0.0);

                for(thePolyConnect.Initialize(aNodeIter); thePolyConnect.More(); thePolyConnect.Next())
                {
                    aTriangles(thePolyConnect.Value()).Get(aTri[0], aTri[1], aTri[2]);
                    const gp_XYZ v1(aNodes(aTri[1]).Coord() - aNodes(aTri[0]).Coord());
                    const gp_XYZ v2(aNodes(aTri[2]).Coord() - aNodes(aTri[1]).Coord());
                    const gp_XYZ vv = v1 ^ v2;
                    const Standard_Real aMod = vv.Modulus();

                    if(aMod >= aTol)
                    {
                        eqPlan += vv / aMod;
                    }
                }

                const Standard_Real aModMax = eqPlan.Modulus();
                theNormals(aNodeIter) = (aModMax > aTol) ? gp_Dir(eqPlan) : gp::DZ();
            }

            const Standard_Integer anId = (aNodeIter - aNodes.Lower()) * 3;
            aNormals->SetValue(anId + 1, (Standard_ShortReal)theNormals(aNodeIter).X());
            aNormals->SetValue(anId + 2, (Standard_ShortReal)theNormals(aNodeIter).Y());
            aNormals->SetValue(anId + 3, (Standard_ShortReal)theNormals(aNodeIter).Z());
        }

        aPolyTri->SetNormals(aNormals);

        if(theFace.Orientation() == TopAbs_REVERSED)
        {
            for(Standard_Integer aNodeIter = aNodes.Lower(); aNodeIter <= aNodes.Upper(); ++aNodeIter)
            {
                theNormals.ChangeValue(aNodeIter).Reverse();
            }
        }
    }
}
