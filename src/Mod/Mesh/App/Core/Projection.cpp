/***************************************************************************
 *   Copyright (c) 2005 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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
# include <algorithm>
# include <map>
#endif
#ifdef FC_USE_OCC
# include <Bnd_Box.hxx>
# include <BndLib_Add3dCurve.hxx>
# include <BRep_Tool.hxx>
# include <BRepAdaptor_Curve.hxx>
# include <GCPnts_UniformDeflection.hxx>
# include <Geom_Curve.hxx>
# include <Geom_Plane.hxx>
# include <GeomAPI_IntCS.hxx>
# include <gp_Pln.hxx>
# include <TopExp_Explorer.hxx>
# include <TopoDS.hxx>
# include <TopoDS_Edge.hxx>
#endif

#include "Projection.h"
#include "MeshKernel.h"
#include "Iterator.h"
#include "Algorithm.h"
#include "Grid.h"

#include <Base/Exception.h>
#include <Base/Console.h>
#include <Base/Sequencer.h>


using namespace MeshCore;


#ifdef FC_USE_OCC
MeshProjection::MeshProjection(const MeshKernel& rMesh) 
  : _rcMesh(rMesh)
{
}

MeshProjection::~MeshProjection()
{
}

void MeshProjection::splitMeshByShape ( const TopoDS_Shape &aShape, float fMaxDist ) const
{
    std::vector<SplitEdge> cSplitEdges;
    projectToMesh( aShape, fMaxDist, cSplitEdges );

    std::ofstream str("output.asc", std::ios::out | std::ios::binary);
    str.precision(4);
    str.setf(std::ios::fixed | std::ios::showpoint);
    for (std::vector<SplitEdge>::const_iterator it = cSplitEdges.begin();it!=cSplitEdges.end();++it) {
        str << it->cPt.x << " " << it->cPt.y << " " << it->cPt.z << std::endl;
    }
    str.close();
}

void MeshProjection::projectToMesh ( const TopoDS_Shape &aShape, float fMaxDist, std::vector<SplitEdge>& rSplitEdges ) const
{
    // calculate the average edge length and create a grid
    MeshAlgorithm clAlg( _rcMesh );
    float fAvgLen = clAlg.GetAverageEdgeLength();
    MeshFacetGrid cGrid( _rcMesh, 5.0f*fAvgLen );

    TopExp_Explorer Ex;
    TopoDS_Shape Edge;

    int iCnt=0;
    for (Ex.Init(aShape, TopAbs_EDGE); Ex.More(); Ex.Next())
        iCnt++;

    Base::Sequencer().start( "Project curve on mesh", iCnt );

    for (Ex.Init(aShape, TopAbs_EDGE); Ex.More(); Ex.Next()) {
        const TopoDS_Edge& aEdge = TopoDS::Edge(Ex.Current());
        projectEdgeToEdge( aEdge, fMaxDist, cGrid, rSplitEdges );
        Base::Sequencer().next();
    }

    Base::Sequencer().stop();
}

void MeshProjection::projectEdgeToEdge( const TopoDS_Edge &aEdge, float fMaxDist, const MeshFacetGrid& rGrid,
                                         std::vector<SplitEdge>& rSplitEdges ) const
{
    std::vector<unsigned long> auFInds;
    std::map<std::pair<unsigned long, unsigned long>, std::list<unsigned long> > pEdgeToFace;
    const std::vector<MeshFacet>& rclFAry = _rcMesh.GetFacets();

    // search the facets in the local area of the curve
    std::vector<Vector3f> acPolyLine;

    BRepAdaptor_Curve clCurve( aEdge );

    Standard_Real fFirst = clCurve.FirstParameter();
    Standard_Real fLast  = clCurve.LastParameter();

    GCPnts_UniformDeflection clDefl(clCurve, 0.01f, fFirst, fLast);
    if (clDefl.IsDone() == Standard_True) {
        Standard_Integer nNbPoints = clDefl.NbPoints();
        for (Standard_Integer i = 1; i <= nNbPoints; i++) {
            gp_Pnt gpPt = clCurve.Value(clDefl.Parameter(i));
            acPolyLine.push_back( Vector3f( (float)gpPt.X(), (float)gpPt.Y(), (float)gpPt.Z() ) );
        }
    }

    MeshAlgorithm(_rcMesh).SearchFacetsFromPolyline( acPolyLine, fMaxDist, rGrid, auFInds);
    // remove duplicated elements
    std::sort(auFInds.begin(), auFInds.end());
    auFInds.erase(std::unique(auFInds.begin(), auFInds.end()), auFInds.end());

    // facet to edge
    for ( std::vector<unsigned long>::iterator pI = auFInds.begin(); pI != auFInds.end(); ++pI ) {
        const MeshFacet& rF = rclFAry[*pI];
        for (int i = 0; i < 3; i++) {
            unsigned long ulPt0 = std::min<unsigned long>(rF._aulPoints[i],  rF._aulPoints[(i+1)%3]);
            unsigned long ulPt1 = std::max<unsigned long>(rF._aulPoints[i],  rF._aulPoints[(i+1)%3]);
            pEdgeToFace[std::pair<unsigned long, unsigned long>(ulPt0, ulPt1)].push_front(*pI);
        }
    }

    // sort intersection points by parameter
    std::map<Quantity_Parameter, SplitEdge> rParamSplitEdges;

//  Standard_Real fFirst, fLast;
    Handle(Geom_Curve) hCurve = BRep_Tool::Curve( aEdge,fFirst,fLast );

    // bounds of curve
//  Bnd_Box clBB;
//  BndLib_Add3dCurve::Add( BRepAdaptor_Curve(aEdge), 0.0, clBB );

    MeshPointIterator cPI( _rcMesh );
    MeshFacetIterator cFI( _rcMesh );

    Base::Sequencer().start( "Project curve on mesh", pEdgeToFace.size() );
    std::map<std::pair<unsigned long, unsigned long>, std::list<unsigned long> >::iterator it;
    for ( it = pEdgeToFace.begin(); it != pEdgeToFace.end(); ++it ) {
        Base::Sequencer().next();

        // edge points
        unsigned long uE0 = it->first.first;
        cPI.Set( uE0 );
        Vector3f cE0 = *cPI;
        unsigned long uE1 = it->first.second;
        cPI.Set( uE1 );
        Vector3f cE1 = *cPI;

        const std::list<unsigned long>& auFaces = it->second;
        if ( auFaces.size() > 2 )
            continue; // non-manifold edge -> don't handle this
//      if ( clBB.IsOut( gp_Pnt(cE0.x, cE0.y, cE0.z) ) && clBB.IsOut( gp_Pnt(cE1.x, cE1.y, cE1.z) ) )
//          continue;

        Vector3f cEdgeNormal;
        for ( std::list<unsigned long>::const_iterator itF = auFaces.begin(); itF != auFaces.end(); ++itF ) {
            cFI.Set( *itF );
            cEdgeNormal += cFI->GetNormal();
        }

        // create a plane from the edge normal and point
        Vector3f cPlaneNormal = cEdgeNormal % (cE1 - cE0);
        Handle(Geom_Plane) hPlane = new Geom_Plane(gp_Pln(gp_Pnt(cE0.x,cE0.y,cE0.z), 
                                    gp_Dir(cPlaneNormal.x,cPlaneNormal.y,cPlaneNormal.z)));

        // get intersection of curve and plane
        GeomAPI_IntCS Alg(hCurve,hPlane); 
        if ( Alg.IsDone() ) {
            Standard_Integer nNbPoints = Alg.NbPoints();
            if ( nNbPoints == 1 ) {
                Quantity_Parameter fU, fV, fW;
                Alg.Parameters( 1, fU, fV, fW);

                gp_Pnt P = Alg.Point(1);
                Vector3f cP0((float)P.X(), (float)P.Y(), (float)P.Z());

                float l = ( (cP0 - cE0) * (cE1 - cE0) ) / ( (cE1 - cE0) * ( cE1 - cE0) );

                // lies the point inside the edge?
                if ( l>=0.0f && l<=1.0f ) {
                    Vector3f cSplitPoint = (1-l) * cE0 + l * cE1;
                    float fDist = Base::Distance( cP0, cSplitPoint );

                    if ( fDist <= fMaxDist ) {
                        SplitEdge splitEdge;
                        splitEdge.uE0 = uE0;
                        splitEdge.uE1 = uE1;
                        splitEdge.cPt = cSplitPoint;
                        rParamSplitEdges[fW] = splitEdge;
                    }
                }
            }
            // search for the right solution
            else if ( nNbPoints > 1 ) {
                int nCntSol=0;
                Vector3f cSplitPoint;
                Quantity_Parameter fSol;
                Vector3f cP0;
                for ( int j=1; j<=nNbPoints; j++ ) {
                    Quantity_Parameter fU, fV, fW;
                    Alg.Parameters( j, fU, fV, fW);
                    gp_Pnt P = Alg.Point(j);
                    cP0.Set((float)P.X(), (float)P.Y(), (float)P.Z());

                    float l = ( (cP0 - cE0) * (cE1 - cE0) ) / ( (cE1 - cE0) * ( cE1 - cE0) );

                    // lies the point inside the edge?
                    if ( l>=0.0 && l<=1.0 ) {
                        cSplitPoint = (1-l) * cE0 + l * cE1;
                        float fDist = Base::Distance( cP0, cSplitPoint );
  
                        if (fDist <= fMaxDist) {
                            nCntSol++;
                            fSol = fW;
                        }
                    }
                }

                // ok, only one sensible solution
                if ( nCntSol == 1 ) {
                    SplitEdge splitEdge;
                    splitEdge.uE0 = uE0;
                    splitEdge.uE1 = uE1;
                    splitEdge.cPt = cSplitPoint;
                    rParamSplitEdges[fSol] = splitEdge;
                }
                else if ( nCntSol > 1 ) {
                    Base::Console().Log("More than one possible intersection points\n");
                }
            }
        }
    }

    // sorted by parameter
    for (std::map<Quantity_Parameter, SplitEdge>::iterator itS = 
         rParamSplitEdges.begin(); itS != rParamSplitEdges.end(); ++itS) {
         rSplitEdges.push_back( itS->second );
    }

    Base::Sequencer().stop();
}

#endif

// ------------------------------------------------------------------------

MeshProjection::MeshProjection(const MeshKernel& mesh)
  : kernel(mesh)
{
}

MeshProjection::~MeshProjection()
{
}

bool MeshProjection::bboxInsideRectangle(const Base::BoundBox3f& bbox,
                                         const Base::Vector3f& p1,
                                         const Base::Vector3f& p2,
                                         const Base::Vector3f& view) const
{
    Base::Vector3f dir(p2 - p1);
    Base::Vector3f base(p1), normal(view % dir);
    normal.Normalize(); 

    if (bbox.IsCutPlane(base, normal)) {
        dir.Normalize();
        Base::Vector3f cnt(bbox.GetCenter());

        return (fabs(cnt.DistanceToPlane(p1, dir)) +  fabs(cnt.DistanceToPlane(p2, dir))) <=
               (bbox.CalcDiagonalLength() + (p2 - p1).Length());
    }

    return false;
}

bool MeshProjection::isPointInsideDistance (const Base::Vector3f& p1,
                                            const Base::Vector3f& p2,
                                            const Base::Vector3f& pt) const
{
    // project point on line
    Base::Vector3f proj, dir(p2 - p1);
    Base::Vector3f move(pt - p1);
    proj.ProjectToLine(move, dir);
    proj = pt + proj;
    return (((p1 - proj) * (p2 - proj)) < 0.0f);
}

bool MeshProjection::connectLines(std::list< std::pair<Base::Vector3f, Base::Vector3f> >& cutLines,
                                  const Base::Vector3f& startPoint, const Base::Vector3f& endPoint,
                                  std::vector<Base::Vector3f>& polyline) const
{
    const float fMaxDist = float(sqrt(FLOAT_MAX)); // max. length of a gap
    const float fMinEps  = 1.0e-4f;

    polyline.clear();
    polyline.push_back(startPoint);

    Base::Vector3f curr(startPoint);
    while ((curr != endPoint) && (!cutLines.empty())) {
        std::list< std::pair<Base::Vector3f, Base::Vector3f> >::iterator it, pCurr = cutLines.end();

        // get nearest line
        float  fMin  = fMaxDist * fMaxDist;

        bool  bPos = false;
        for (it = cutLines.begin(); it != cutLines.end(); ++it) {
            float fD1 = Base::DistanceP2(curr, it->first);
            float fD2 = Base::DistanceP2(curr, it->second);
            if (std::min<float>(fD1, fD2) < fMin) {
                pCurr = it;
                bPos  = fD1 < fD2;
                fMin  = std::min<float>(fD1, fD2);
                if (fMin < fMinEps) // abort because next line already found
                    break;
            }
        }

        if (pCurr != cutLines.end()) {
            if (bPos) {
                if (fMin > fMinEps) // gap, insert point
                    polyline.push_back(pCurr->first);
                polyline.push_back(pCurr->second);
                curr = pCurr->second;
            }
            else {
                if (fMin > fMinEps) // gap, insert point
                    polyline.push_back(pCurr->second);
                polyline.push_back(pCurr->first);
                curr = pCurr->first;
            }
        }
        else {
            return false;  // abort because no line was found
        }

        cutLines.erase(pCurr); 
    }

    return true;
}

bool MeshProjection::projectLineOnMesh(const MeshFacetGrid& grid,
                                       const Base::Vector3f& v1, unsigned long f1,
                                       const Base::Vector3f& v2, unsigned long f2,
                                       const Base::Vector3f& vd,
                                       std::vector<Base::Vector3f>& polyline)
{
    Base::Vector3f dir(v2 - v1);
    Base::Vector3f base(v1), normal(vd % dir);
    normal.Normalize();
    dir.Normalize();


    std::vector<unsigned long> facets;

    // special case: start and endpoint inside same facet
    if (f1 == f2) {
        polyline.push_back(v1);
        polyline.push_back(v2);
        return true;
    }

    // cut all facets between the two endpoints
    MeshGridIterator gridIter(grid);
    for (gridIter.Init(); gridIter.More(); gridIter.Next()) {
        // bbox cuts plane
        if (bboxInsideRectangle(gridIter.GetBoundBox(), v1, v2, vd))
            gridIter.GetElements(facets);
    }

    std::sort(facets.begin(), facets.end());
    facets.erase(std::unique(facets.begin(), facets.end()), facets.end());

    // cut all facets with plane
    std::list< std::pair<Base::Vector3f, Base::Vector3f> > cutLine;
    //unsigned long start = 0, end = 0;
    for (std::vector<unsigned long>::iterator it = facets.begin(); it != facets.end(); ++it) {
        Base::Vector3f e1, e2;
        MeshGeomFacet tria = kernel.GetFacet(*it);
        if (bboxInsideRectangle(tria.GetBoundBox(), v1, v2, vd)) {
            if (tria.IntersectWithPlane(base, normal, e1, e2)) {
                if ((*it != f1) && (*it != f2)) {
                    // inside cut line
                    if ((isPointInsideDistance(v1, v2, e1) == false) ||
                        (isPointInsideDistance(v1, v2, e2) == false)) {
                        continue;
                    }

                    cutLine.push_back(std::pair<Base::Vector3f, Base::Vector3f>(e1, e2));
                }
                else {
                    if (*it == f1) { // start facet
                        if (((e2 - v1) * dir) > 0.0f)
                            cutLine.push_back(std::pair<Base::Vector3f, Base::Vector3f>(v1, e2));
                        else
                            cutLine.push_back(std::pair<Base::Vector3f, Base::Vector3f>(v1, e1));

                        //start = it - facets.begin();
                    }

                    if (*it == f2) { // end facet
                        if (((e2 - v2) * -dir) > 0.0f)
                            cutLine.push_back(std::pair<Base::Vector3f, Base::Vector3f>(v2, e2));
                        else
                            cutLine.push_back(std::pair<Base::Vector3f, Base::Vector3f>(v2, e1));

                        //end = it - facets.begin();
                    }
                }
            }
        }
    }

    return connectLines(cutLine, v1, v2, polyline);
}
