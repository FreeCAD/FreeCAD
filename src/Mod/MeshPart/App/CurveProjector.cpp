/***************************************************************************
 *   Copyright (c) 2008 Juergen Riegel <juergen.riegel@web.de>             *
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
#ifdef FC_OS_LINUX
#include <unistd.h>
#endif
#include <BRepAdaptor_Curve.hxx>
#include <BRepBuilderAPI_MakeVertex.hxx>
#include <BRepExtrema_DistShapeShape.hxx>
#include <BRep_Tool.hxx>
#include <BndLib_Add3dCurve.hxx>
#include <Bnd_Box.hxx>
#include <GCPnts_AbscissaPoint.hxx>
#include <GCPnts_UniformAbscissa.hxx>
#include <GCPnts_UniformDeflection.hxx>
#include <GeomAPI_IntCS.hxx>
#include <Geom_Curve.hxx>
#include <Geom_Plane.hxx>
#include <Standard_Failure.hxx>
#include <TopExp_Explorer.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Edge.hxx>
#include <gp_Pln.hxx>
#endif

#include <Base/Console.h>
#include <Base/FileInfo.h>
#include <Base/Sequencer.h>
#include <Base/Stream.h>

#include <Mod/Mesh/App/Core/Algorithm.h>
#include <Mod/Mesh/App/Core/Grid.h>
#include <Mod/Mesh/App/Core/Iterator.h>
#include <Mod/Mesh/App/Core/MeshKernel.h>
#include <Mod/Mesh/App/Core/Projection.h>

#include "MeshAlgos.h"


using namespace MeshPart;
using MeshCore::MeshAlgorithm;
using MeshCore::MeshFacet;
using MeshCore::MeshFacetGrid;
using MeshCore::MeshFacetIterator;
using MeshCore::MeshKernel;
using MeshCore::MeshPointIterator;

CurveProjector::CurveProjector(const TopoDS_Shape& aShape, const MeshKernel& pMesh)
    : _Shape(aShape)
    , _Mesh(pMesh)
{}

void CurveProjector::writeIntersectionPointsToFile(const char* name)
{
    // export points
    Base::FileInfo fi(name);
    Base::ofstream str(fi, std::ios::out | std::ios::binary);
    str.precision(4);
    str.setf(std::ios::fixed | std::ios::showpoint);
    for (const auto& it1 : mvEdgeSplitPoints) {
        for (const auto& it2 : it1.second) {
            str << it2.p1.x << " " << it2.p1.y << " " << it2.p1.z << std::endl;
        }
    }
    str.close();
}


//**************************************************************************
//**************************************************************************
// Separator for additional classes
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

CurveProjectorShape::CurveProjectorShape(const TopoDS_Shape& aShape, const MeshKernel& pMesh)
    : CurveProjector(aShape, pMesh)
{
    CurveProjectorShape::Do();
}

void CurveProjectorShape::Do()
{
    TopExp_Explorer Ex;
    for (Ex.Init(_Shape, TopAbs_EDGE); Ex.More(); Ex.Next()) {
        const TopoDS_Edge& aEdge = TopoDS::Edge(Ex.Current());

        // std::vector<FaceSplitEdge> vSplitEdges;
        projectCurve(aEdge, mvEdgeSplitPoints[aEdge]);
    }
}


void CurveProjectorShape::projectCurve(const TopoDS_Edge& aEdge,
                                       std::vector<FaceSplitEdge>& vSplitEdges)
{
    Standard_Real fFirst, fLast;
    Handle(Geom_Curve) hCurve = BRep_Tool::Curve(aEdge, fFirst, fLast);

    // getting start point
    gp_Pnt gpPt = hCurve->Value(fFirst);

    // projection of the first point
    Base::Vector3f cStartPoint = Base::Vector3f((float)gpPt.X(), (float)gpPt.Y(), (float)gpPt.Z());
    Base::Vector3f cResultPoint, cSplitPoint, cPlanePnt, cPlaneNormal;
    MeshCore::FacetIndex uStartFacetIdx, uCurFacetIdx;
    MeshCore::FacetIndex uLastFacetIdx =
        MeshCore::FACET_INDEX_MAX - 1;  // use another value as FACET_INDEX_MAX
    MeshCore::FacetIndex auNeighboursIdx[3];
    bool GoOn;

    if (!findStartPoint(_Mesh, cStartPoint, cResultPoint, uStartFacetIdx)) {
        return;
    }

    uCurFacetIdx = uStartFacetIdx;
    do {
        MeshGeomFacet cCurFacet = _Mesh.GetFacet(uCurFacetIdx);
        _Mesh.GetFacetNeighbours(uCurFacetIdx,
                                 auNeighboursIdx[0],
                                 auNeighboursIdx[1],
                                 auNeighboursIdx[2]);
        Base::Vector3f PointOnEdge[3];

        GoOn = false;
        int NbrOfHits = 0, HitIdx = 0;

        for (int i = 0; i < 3; i++) {
            // ignore last visited facet
            if (auNeighboursIdx[i] == uLastFacetIdx) {
                continue;
            }

            // get points of the edge i
            const Base::Vector3f& cP0 = cCurFacet._aclPoints[i];
            const Base::Vector3f& cP1 = cCurFacet._aclPoints[(i + 1) % 3];

            if (auNeighboursIdx[i] != MeshCore::FACET_INDEX_MAX) {
                // calculate the normal by the edge vector and the middle between the two face
                // normals
                MeshGeomFacet N = _Mesh.GetFacet(auNeighboursIdx[i]);
                cPlaneNormal = (N.GetNormal() + cCurFacet.GetNormal()) % (cP1 - cP0);
                cPlanePnt = cP0;
            }
            else {
                // with no neighbours the face normal is used
                cPlaneNormal = cCurFacet.GetNormal() % (cP1 - cP0);
                cPlanePnt = cP0;
            }

            Handle(Geom_Plane) hPlane =
                new Geom_Plane(gp_Pln(gp_Pnt(cPlanePnt.x, cPlanePnt.y, cPlanePnt.z),
                                      gp_Dir(cPlaneNormal.x, cPlaneNormal.y, cPlaneNormal.z)));

            GeomAPI_IntCS Alg(hCurve, hPlane);

            if (Alg.IsDone()) {
                // deciding by the number of result points (intersections)
                if (Alg.NbPoints() == 1) {
                    gp_Pnt P = Alg.Point(1);
                    float l = ((Base::Vector3f((float)P.X(), (float)P.Y(), (float)P.Z()) - cP0)
                               * (cP1 - cP0))
                        / ((cP1 - cP0) * (cP1 - cP0));
                    // is the Point on the Edge of the facet?
                    if (l < 0.0 || l > 1.0) {
                        PointOnEdge[i] = Base::Vector3f(FLOAT_MAX, 0, 0);
                    }
                    else {
                        cSplitPoint = (1 - l) * cP0 + l * cP1;
                        PointOnEdge[i] = (1 - l) * cP0 + l * cP1;
                        NbrOfHits++;
                        HitIdx = i;
                    }
                    // no intersection
                }
                else if (Alg.NbPoints() == 0) {
                    PointOnEdge[i] = Base::Vector3f(FLOAT_MAX, 0, 0);
                    // more the one intersection (@ToDo)
                }
                else if (Alg.NbPoints() > 1) {
                    PointOnEdge[i] = Base::Vector3f(FLOAT_MAX, 0, 0);
                    Base::Console().Log("MeshAlgos::projectCurve(): More then one intersection in "
                                        "Facet %lu, Edge %d\n",
                                        uCurFacetIdx,
                                        i);
                }
            }
        }

        uLastFacetIdx = uCurFacetIdx;

        if (NbrOfHits == 1) {
            uCurFacetIdx = auNeighboursIdx[HitIdx];
            FaceSplitEdge splitEdge;
            splitEdge.ulFaceIndex = uCurFacetIdx;
            splitEdge.p1 = cResultPoint;
            splitEdge.p2 = cSplitPoint;
            vSplitEdges.push_back(splitEdge);
            cResultPoint = cSplitPoint;
            GoOn = true;
        }
        else {
            Base::Console().Log("MeshAlgos::projectCurve(): Possible reentry in Facet %lu\n",
                                uCurFacetIdx);
        }

        if (uCurFacetIdx == uStartFacetIdx) {
            GoOn = false;
        }

    } while (GoOn);
}

bool CurveProjectorShape::findStartPoint(const MeshKernel& MeshK,
                                         const Base::Vector3f& Pnt,
                                         Base::Vector3f& Rslt,
                                         MeshCore::FacetIndex& FaceIndex)
{
    Base::Vector3f TempResultPoint;
    float MinLength = FLOAT_MAX;
    bool bHit = false;

    // go through the whole Mesh
    MeshFacetIterator It(MeshK);
    for (It.Init(); It.More(); It.Next()) {
        // try to project (with angle) to the face
        if (It->Foraminate(Pnt, It->GetNormal(), TempResultPoint)) {
            // distance to the projected point
            float Dist = (Pnt - TempResultPoint).Length();
            if (Dist < MinLength) {
                // remember the point with the closest distance
                bHit = true;
                MinLength = Dist;
                Rslt = TempResultPoint;
                FaceIndex = It.Position();
            }
        }
    }
    return bHit;
}


//**************************************************************************
//**************************************************************************
// Separator for CurveProjectorSimple classes
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++


CurveProjectorSimple::CurveProjectorSimple(const TopoDS_Shape& aShape, const MeshKernel& pMesh)
    : CurveProjector(aShape, pMesh)
{
    Do();
}


void CurveProjectorSimple::Do()
{
    TopExp_Explorer Ex;

    std::vector<Base::Vector3f> vEdgePolygon;

    for (Ex.Init(_Shape, TopAbs_EDGE); Ex.More(); Ex.Next()) {
        const TopoDS_Edge& aEdge = TopoDS::Edge(Ex.Current());
        //    GetSampledCurves(aEdge,vEdgePolygon,2000);

        // std::vector<FaceSplitEdge> vSplitEdges;
        projectCurve(aEdge, vEdgePolygon, mvEdgeSplitPoints[aEdge]);
    }
}

void CurveProjectorSimple::GetSampledCurves(const TopoDS_Edge& aEdge,
                                            std::vector<Base::Vector3f>& rclPoints,
                                            unsigned long ulNbOfPoints)
{
    rclPoints.clear();

    Standard_Real fBegin, fEnd;

    Handle(Geom_Curve) hCurve = BRep_Tool::Curve(aEdge, fBegin, fEnd);
    float fLen = float(fEnd - fBegin);

    for (unsigned long i = 0; i < ulNbOfPoints; i++) {
        gp_Pnt gpPt = hCurve->Value(fBegin + (fLen * float(i)) / float(ulNbOfPoints - 1));
        rclPoints.emplace_back((float)gpPt.X(), (float)gpPt.Y(), (float)gpPt.Z());
    }
}


// projectToNeighbours(Handle(Geom_Curve) hCurve,float pos

void CurveProjectorSimple::projectCurve(const TopoDS_Edge& aEdge,
                                        const std::vector<Base::Vector3f>& /*rclPoints*/,
                                        std::vector<FaceSplitEdge>& /*vSplitEdges*/)
{
    Base::Vector3f /*cResultPoint, cSplitPoint, cPlanePnt, cPlaneNormal,*/ TempResultPoint;
    bool bFirst = true;
    // unsigned long auNeighboursIdx[3];
    // std::map<unsigned long,std::vector<Base::Vector3f> >::iterator N1,N2,N3;

    Standard_Real fBegin, fEnd;
    Handle(Geom_Curve) hCurve = BRep_Tool::Curve(aEdge, fBegin, fEnd);
    float fLen = float(fEnd - fBegin);

    unsigned long ulNbOfPoints = 1000, PointCount = 0;

    MeshFacetIterator It(_Mesh);

    Base::SequencerLauncher seq("Building up projection map...", ulNbOfPoints + 1);
    Base::FileInfo fi("projected.asc");
    Base::ofstream str(fi, std::ios::out | std::ios::binary);
    str.precision(4);
    str.setf(std::ios::fixed | std::ios::showpoint);

    std::map<MeshCore::FacetIndex, std::vector<Base::Vector3f>> FaceProjctMap;

    for (unsigned long i = 0; i <= ulNbOfPoints; i++) {
        seq.next();
        gp_Pnt gpPt = hCurve->Value(fBegin + (fLen * float(i)) / float(ulNbOfPoints - 1));

        // go through the whole Mesh
        for (It.Init(); It.More(); It.Next()) {
            // try to project (with angle) to the face
            if (It->IntersectWithLine(
                    Base::Vector3f((float)gpPt.X(), (float)gpPt.Y(), (float)gpPt.Z()),
                    It->GetNormal(),
                    TempResultPoint)) {
                FaceProjctMap[It.Position()].push_back(TempResultPoint);
                str << TempResultPoint.x << " " << TempResultPoint.y << " " << TempResultPoint.z
                    << std::endl;
                Base::Console().Log("IDX %d\n", It.Position());

                if (bFirst) {
                    bFirst = false;
                }

                PointCount++;
            }
        }
    }

    str.close();
    Base::Console().Log("Projection map [%d facets with %d points]\n",
                        FaceProjctMap.size(),
                        PointCount);

    // estimate the first face
    //  gp_Pnt gpPt = hCurve->Value(fBegin);
    //  if(
    //  !findStartPoint(MeshK,Base::Vector3f(gpPt.X(),gpPt.Y(),gpPt.Z()),cResultPoint,uCurFacetIdx)
    //  )
    //    uCurFacetIdx = FaceProjctMap.begin()->first;

    /*
      do{
        Base::Console().Log("Grow on %d %d left\n",uCurFacetIdx,FaceProjctMap.size());

        if(FaceProjctMap[uCurFacetIdx].size() == 1)
        {
          Base::Console().Log("Single hit\n");
        }else{


        }

        FaceProjctMap.erase(uCurFacetIdx);

        // estimate next facet
        MeshGeomFacet cCurFacet= MeshK.GetFacet(uCurFacetIdx);
        MeshK.GetFacetNeighbours ( uCurFacetIdx, auNeighboursIdx[0], auNeighboursIdx[1],
      auNeighboursIdx[2]);

        uCurFacetIdx = MeshCore::FACET_INDEX_MAX;
        PointCount = 0;

        for(int i=0; i<3; i++)
        {
          N1 = FaceProjctMap.find(auNeighboursIdx[i]);
          // if the i'th neighbour is valid
          if ( N1 != FaceProjctMap.end() )
          {
            unsigned long temp = N1->second.size();
            if(temp >= PointCount){
              PointCount = N1->second.size();
              uCurFacetIdx = auNeighboursIdx[i];
            }
          }
        }


      }while(uCurFacetIdx != MeshCore::FACET_INDEX_MAX);
    */
}

/*
void CurveProjectorSimple::projectCurve( const TopoDS_Edge& aEdge,
                                   const std::vector<Base::Vector3f> &rclPoints,
                                   std::vector<FaceSplitEdge> &vSplitEdges)
{
  const MeshKernel &MeshK = *(_Mesh.getKernel());

  Standard_Real fFirst, fLast, fAct;
  Handle(Geom_Curve) hCurve = BRep_Tool::Curve( aEdge,fFirst,fLast );

  // getting start point
  gp_Pnt gpPt = hCurve->Value(fFirst);
  fAct = fFirst;
  // projection of the first point
  Base::Vector3f cStartPoint = Base::Vector3f(gpPt.X(),gpPt.Y(),gpPt.Z());
  Base::Vector3f cResultPoint, cSplitPoint, cPlanePnt, cPlaneNormal,TempResultPoint;
  MeshCore::FacetIndex uStartFacetIdx,uCurFacetIdx;
  MeshCore::FacetIndex uLastFacetIdx=MeshCore::FACET_INDEX_MAX-1; // use another value as
FACET_INDEX_MAX MeshCore::FacetIndex auNeighboursIdx[3]; bool GoOn;

  // go through the whole Mesh, find the first projection
  MeshFacetIterator It(MeshK);
  GoOn = false;
  for(It.Init();It.More();It.Next())
  {
    // try to project (with angle) to the face
    if(MeshFacetFunc::IntersectWithLine (*It, cStartPoint, It->GetNormal(), cResultPoint) )
    {
      uCurFacetIdx = It.Position();
      GoOn = true;
      break;
    }
  }

  if(!GoOn)
  {
    Base::Console().Log("Starting point not projectable\n");
    return;
  }
  {
    float fStep = (fLast-fFirst)/20;
    unsigned long HitCount,Sentinel = 0 ;
    MeshGeomFacet cCurFacet= MeshK.GetFacet(uCurFacetIdx);
    MeshK.GetFacetNeighbours ( uCurFacetIdx, auNeighboursIdx[0], auNeighboursIdx[1],
auNeighboursIdx[2]);

    do{
      // lower the step until you find a neigbourfacet to project...
      fStep /= 2.0;
      // still on the same facet?
      gpPt = hCurve->Value(fAct+fStep);
      if(MeshFacetFunc::IntersectWithLine (cCurFacet, Base::Vector3f(gpPt.X(),gpPt.Y(),gpPt.Z()),
cCurFacet.GetNormal(), cResultPoint) )
      {
        fAct += fStep;
        fStep *= 2.0;
        continue;
      }

      HitCount = 0;
      for(int i=0; i<3; i++)
      {
        // if the i'th neighbour is valid
        if ( auNeighboursIdx[i] != MeshCore::FACET_INDEX_MAX )
        {
          // try to project next interval
          MeshGeomFacet N = MeshK.GetFacet( auNeighboursIdx[i] );
          gpPt = hCurve->Value(fAct+fStep);
          if(MeshFacetFunc::IntersectWithLine (*It, Base::Vector3f(gpPt.X(),gpPt.Y(),gpPt.Z()),
It->GetNormal(), cResultPoint) )
          {
            HitCount++;
            uStartFacetIdx = auNeighboursIdx[i];
          }

        }
      }

      Sentinel++;

    }while(HitCount!=1 && Sentinel < 20);

  }


}
*/
/*

void CurveProjectorSimple::projectCurve( const TopoDS_Edge& aEdge,
                                   const std::vector<Base::Vector3f> &rclPoints,
                                   std::vector<FaceSplitEdge> &vSplitEdges)
{
  const MeshKernel &MeshK = *(_Mesh.getKernel());

  Standard_Real fFirst, fLast;
  Handle(Geom_Curve) hCurve = BRep_Tool::Curve( aEdge,fFirst,fLast );

  // getting start point
  gp_Pnt gpPt = hCurve->Value(fFirst);

  // projection of the first point
  Base::Vector3f cStartPoint = Base::Vector3f(gpPt.X(),gpPt.Y(),gpPt.Z());
  Base::Vector3f cResultPoint, cSplitPoint, cPlanePnt, cPlaneNormal;
  MeshCore::FacetIndex uStartFacetIdx,uCurFacetIdx;
  MeshCore::FacetIndex uLastFacetIdx=MeshCore::FACET_INDEX_MAX-1; // use another value as
FACET_INDEX_MAX MeshCore::FacetIndex auNeighboursIdx[3]; bool GoOn;

  if( !findStartPoint(MeshK,cStartPoint,cResultPoint,uStartFacetIdx) )
    return;

  FILE* file = fopen("projected.asc", "w");

  // go through the whole Mesh
  MeshFacetIterator It1(MeshK);
  for(It1.Init();It1.More();It1.Next())
  {
    // cycling through the points and find the first projecteble point ( if the curve starts outside
the mesh) for( std::vector<Base::Vector3f>::const_iterator It =
rclPoints.begin()+1;It!=rclPoints.end();++It)
    {
//      MeshGeomFacet facet = MeshK.GetFacet(uStartFacetIdx);
      MeshGeomFacet facet = *It1;

      if(MeshFacetFunc::IntersectWithLine(facet, *It, facet.GetNormal(), cResultPoint) )
        fprintf(file, "%.4f %.4f %.4f\n", cResultPoint.x, cResultPoint.y, cResultPoint.z);

    }
  }

  fclose(file);

}
*/

bool CurveProjectorSimple::findStartPoint(const MeshKernel& MeshK,
                                          const Base::Vector3f& Pnt,
                                          Base::Vector3f& Rslt,
                                          MeshCore::FacetIndex& FaceIndex)
{
    Base::Vector3f TempResultPoint;
    float MinLength = FLOAT_MAX;
    bool bHit = false;

    // go through the whole Mesh
    MeshFacetIterator It(MeshK);
    for (It.Init(); It.More(); It.Next()) {
        // try to project (with angle) to the face
        if (It->Foraminate(Pnt, It->GetNormal(), TempResultPoint)) {
            // distance to the projected point
            float Dist = (Pnt - TempResultPoint).Length();
            if (Dist < MinLength) {
                // remember the point with the closest distance
                bHit = true;
                MinLength = Dist;
                Rslt = TempResultPoint;
                FaceIndex = It.Position();
            }
        }
    }
    return bHit;
}

//**************************************************************************
//**************************************************************************
// Separator for CurveProjectorSimple classes
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++


CurveProjectorWithToolMesh::CurveProjectorWithToolMesh(const TopoDS_Shape& aShape,
                                                       const MeshKernel& pMesh,
                                                       MeshKernel& rToolMesh)
    : CurveProjector(aShape, pMesh)
    , ToolMesh(rToolMesh)
{
    Do();
}


void CurveProjectorWithToolMesh::Do()
{
    TopExp_Explorer Ex;
    std::vector<MeshGeomFacet> cVAry;

    for (Ex.Init(_Shape, TopAbs_EDGE); Ex.More(); Ex.Next()) {
        const TopoDS_Edge& aEdge = TopoDS::Edge(Ex.Current());
        makeToolMesh(aEdge, cVAry);
    }

    ToolMesh.AddFacets(cVAry);
}

// projectToNeighbours(Handle(Geom_Curve) hCurve,float pos

void CurveProjectorWithToolMesh::makeToolMesh(const TopoDS_Edge& aEdge,
                                              std::vector<MeshGeomFacet>& cVAry)
{
    Standard_Real fBegin, fEnd;
    Handle(Geom_Curve) hCurve = BRep_Tool::Curve(aEdge, fBegin, fEnd);
    float fLen = float(fEnd - fBegin);
    Base::Vector3f cResultPoint;

    unsigned long ulNbOfPoints = 15, PointCount = 0 /*,uCurFacetIdx*/;

    std::vector<LineSeg> LineSegs;

    MeshFacetIterator It(_Mesh);

    Base::SequencerLauncher seq("Building up tool mesh...", ulNbOfPoints + 1);

    std::map<MeshCore::FacetIndex, std::vector<Base::Vector3f>> FaceProjctMap;

    for (unsigned long i = 0; i < ulNbOfPoints; i++) {
        seq.next();
        gp_Pnt gpPt = hCurve->Value(fBegin + (fLen * float(i)) / float(ulNbOfPoints - 1));
        Base::Vector3f LinePoint((float)gpPt.X(), (float)gpPt.Y(), (float)gpPt.Z());

        Base::Vector3f ResultNormal;

        // go through the whole Mesh
        for (It.Init(); It.More(); It.Next()) {
            // try to project (with angle) to the face
            if (It->IntersectWithLine(
                    Base::Vector3f((float)gpPt.X(), (float)gpPt.Y(), (float)gpPt.Z()),
                    It->GetNormal(),
                    cResultPoint)) {
                if (Base::Distance(LinePoint, cResultPoint) < 0.5) {
                    ResultNormal += It->GetNormal();
                }
            }
        }
        LineSeg s;
        s.p = Base::Vector3f((float)gpPt.X(), (float)gpPt.Y(), (float)gpPt.Z());
        s.n = ResultNormal.Normalize();
        LineSegs.push_back(s);
    }

    Base::Console().Log("Projection map [%d facets with %d points]\n",
                        FaceProjctMap.size(),
                        PointCount);


    // build up the new mesh
    Base::Vector3f lp(FLOAT_MAX, 0, 0), ln, p1, p2, p3, p4, p5, p6;
    float ToolSize = 0.2f;

    for (const auto& It2 : LineSegs) {
        if (lp.x != FLOAT_MAX) {
            p1 = lp + (ln * (-ToolSize));
            p2 = lp + (ln * ToolSize);
            p3 = lp;
            p4 = It2.p;
            p5 = It2.p + (It2.n * (-ToolSize));
            p6 = It2.p + (It2.n * ToolSize);

            cVAry.emplace_back(p3, p2, p6);
            cVAry.emplace_back(p3, p6, p4);
            cVAry.emplace_back(p1, p3, p4);
            cVAry.emplace_back(p1, p4, p5);
        }

        lp = It2.p;
        ln = It2.n;
    }
}

// ----------------------------------------------------------------------------

MeshProjection::MeshProjection(const MeshKernel& rMesh)
    : _rcMesh(rMesh)
{}

void MeshProjection::discretize(const TopoDS_Edge& aEdge,
                                std::vector<Base::Vector3f>& polyline,
                                std::size_t minPoints) const
{
    BRepAdaptor_Curve clCurve(aEdge);

    Standard_Real fFirst = clCurve.FirstParameter();
    Standard_Real fLast = clCurve.LastParameter();

    GCPnts_UniformDeflection clDefl(clCurve, 0.01f, fFirst, fLast);
    if (clDefl.IsDone() == Standard_True) {
        Standard_Integer nNbPoints = clDefl.NbPoints();
        for (Standard_Integer i = 1; i <= nNbPoints; i++) {
            gp_Pnt gpPt = clCurve.Value(clDefl.Parameter(i));
            polyline.emplace_back((float)gpPt.X(), (float)gpPt.Y(), (float)gpPt.Z());
        }
    }

    if (polyline.size() < minPoints) {
        GCPnts_UniformAbscissa clAbsc(clCurve,
                                      static_cast<Standard_Integer>(minPoints),
                                      fFirst,
                                      fLast);
        if (clAbsc.IsDone() == Standard_True) {
            polyline.clear();
            Standard_Integer nNbPoints = clAbsc.NbPoints();
            for (Standard_Integer i = 1; i <= nNbPoints; i++) {
                gp_Pnt gpPt = clCurve.Value(clAbsc.Parameter(i));
                polyline.emplace_back((float)gpPt.X(), (float)gpPt.Y(), (float)gpPt.Z());
            }
        }
    }
}

void MeshProjection::splitMeshByShape(const TopoDS_Shape& aShape, float fMaxDist) const
{
    std::vector<PolyLine> rPolyLines;
    projectToMesh(aShape, fMaxDist, rPolyLines);

    Base::FileInfo fi("output.asc");
    Base::ofstream str(fi, std::ios::out | std::ios::binary);
    str.precision(4);
    str.setf(std::ios::fixed | std::ios::showpoint);
    for (const auto& it : rPolyLines) {
        for (const auto& jt : it.points) {
            str << jt.x << " " << jt.y << " " << jt.z << std::endl;
        }
    }
    str.close();
}

bool MeshProjection::findIntersection(const Edge& edgeSegm,
                                      const Edge& meshEdge,
                                      const Base::Vector3f& dir,
                                      Base::Vector3f& res) const
{
    Base::Vector3f planeNormal;
    planeNormal = dir.Cross(edgeSegm.cPt2 - edgeSegm.cPt1);
    float dist1 = planeNormal.Dot(meshEdge.cPt1 - edgeSegm.cPt1);
    float dist2 = planeNormal.Dot(meshEdge.cPt2 - edgeSegm.cPt1);
    if (dist1 * dist2 < 0) {
        planeNormal = dir.Cross(meshEdge.cPt2 - meshEdge.cPt1);
        dist1 = planeNormal.Dot(edgeSegm.cPt1 - meshEdge.cPt1);
        dist2 = planeNormal.Dot(edgeSegm.cPt2 - meshEdge.cPt1);
        if (dist1 * dist2 < 0) {
            // intersection detected
            float t = planeNormal.Dot(meshEdge.cPt1 - edgeSegm.cPt1)
                / planeNormal.Dot(edgeSegm.cPt2 - edgeSegm.cPt1);
            res = edgeSegm.cPt1 * (1 - t) + edgeSegm.cPt2 * t;
            return true;
        }
    }
    return false;
}

void MeshProjection::findSectionParameters(const TopoDS_Edge& edge,
                                           const Base::Vector3f& dir,
                                           std::set<double>& parameters) const
{
    MeshAlgorithm clAlg(_rcMesh);
    float fAvgLen = clAlg.GetAverageEdgeLength();
    BRepAdaptor_Curve adapt(edge);
    double edgeLen = GCPnts_AbscissaPoint::Length(adapt, Precision::Confusion());

    std::vector<Base::Vector3f> polyline;
    discretize(edge, polyline, std::max<size_t>(10, static_cast<size_t>(edgeLen / fAvgLen)));
    if (polyline.empty()) {
        return;
    }

    std::vector<Edge> lines;
    Base::Vector3f start = polyline.front();
    for (auto it = polyline.begin() + 1; it != polyline.end(); ++it) {
        Edge line;
        line.cPt1 = start;
        line.cPt2 = *it;
        start = line.cPt2;
        lines.push_back(line);
    }

    const MeshCore::MeshFacetArray& facets = _rcMesh.GetFacets();
    const MeshCore::MeshPointArray& points = _rcMesh.GetPoints();

    Base::Vector3f res;
    for (const auto& it : facets) {
        for (int i = 0; i < 3; i++) {
            Base::Vector3f pt1 = points[it._aulPoints[i]];
            Base::Vector3f pt2 = points[it._aulPoints[(i + 1) % 3]];
            Edge line;
            line.cPt1 = pt1;
            line.cPt2 = pt2;

            for (auto jt : lines) {
                if (findIntersection(jt, line, dir, res)) {
                    try {
                        BRepBuilderAPI_MakeVertex aBuilder(gp_Pnt(res.x, res.y, res.z));
                        BRepExtrema_DistShapeShape extss(aBuilder.Vertex(), edge);
                        if (extss.NbSolution() == 1) {
                            Standard_Real par;
                            // gp_pnt pnt = extss.PointOnShape2(1);
                            // Standard_Real par = BRep_Tool::Parameter(aBuilder.Vertex(), edge);
                            extss.ParOnEdgeS2(1, par);
                            parameters.insert(par);
                            break;
                        }
                    }
                    catch (const Standard_Failure&) {
                        // ignore
                    }
                }
            }
        }
    }
}

void MeshProjection::projectToMesh(const TopoDS_Shape& aShape,
                                   float fMaxDist,
                                   std::vector<PolyLine>& rPolyLines) const
{
    // calculate the average edge length and create a grid
    MeshAlgorithm clAlg(_rcMesh);
    float fAvgLen = clAlg.GetAverageEdgeLength();
    MeshFacetGrid cGrid(_rcMesh, 5.0f * fAvgLen);

    TopExp_Explorer Ex;

    int iCnt = 0;
    for (Ex.Init(aShape, TopAbs_EDGE); Ex.More(); Ex.Next()) {
        iCnt++;
    }

    Base::SequencerLauncher seq("Project curve on mesh", iCnt);

    for (Ex.Init(aShape, TopAbs_EDGE); Ex.More(); Ex.Next()) {
        const TopoDS_Edge& aEdge = TopoDS::Edge(Ex.Current());
        std::vector<SplitEdge> rSplitEdges;
        projectEdgeToEdge(aEdge, fMaxDist, cGrid, rSplitEdges);
        PolyLine polyline;
        polyline.points.reserve(rSplitEdges.size());
        for (auto it : rSplitEdges) {
            polyline.points.push_back(it.cPt);
        }
        rPolyLines.push_back(polyline);
        seq.next();
    }
}

void MeshProjection::projectOnMesh(const std::vector<Base::Vector3f>& pointsIn,
                                   const Base::Vector3f& dir,
                                   float tolerance,
                                   std::vector<Base::Vector3f>& pointsOut) const
{
    // calculate the average edge length and create a grid
    MeshAlgorithm clAlg(_rcMesh);
    float fAvgLen = clAlg.GetAverageEdgeLength();
    MeshFacetGrid cGrid(_rcMesh, 5.0f * fAvgLen);

    // get all boundary points and edges of the mesh
    std::vector<Base::Vector3f> boundaryPoints;
    std::vector<MeshCore::MeshGeomEdge> boundaryEdges;

    const MeshCore::MeshFacetArray& facets = _rcMesh.GetFacets();
    const MeshCore::MeshPointArray& points = _rcMesh.GetPoints();
    for (const auto& it : facets) {
        for (int i = 0; i < 3; i++) {
            if (!it.HasNeighbour(i)) {
                boundaryPoints.push_back(points[it._aulPoints[i]]);

                MeshCore::MeshGeomEdge edge;
                edge._bBorder = true;
                edge._aclPoints[0] = points[it._aulPoints[i]];
                edge._aclPoints[1] = points[it._aulPoints[(i + 1) % 3]];
                boundaryEdges.push_back(edge);
            }
        }
    }

    Base::SequencerLauncher seq("Project points on mesh", pointsIn.size());

    for (auto it : pointsIn) {
        Base::Vector3f result;
        MeshCore::FacetIndex index;
        if (clAlg.NearestFacetOnRay(it, dir, cGrid, result, index)) {
            MeshCore::MeshGeomFacet geomFacet = _rcMesh.GetFacet(index);
            if (tolerance > 0 && geomFacet.IntersectPlaneWithLine(it, dir, result)) {
                if (geomFacet.IsPointOfFace(result, tolerance)) {
                    pointsOut.push_back(result);
                }
            }
            else {
                pointsOut.push_back(result);
            }
        }
        else {
            // go through the boundary points and check if the point can be directly projected
            // onto one of them
            auto boundaryPnt = std::find_if(boundaryPoints.begin(),
                                            boundaryPoints.end(),
                                            [&it, &dir](const Base::Vector3f& pnt) -> bool {
                                                Base::Vector3f vec = pnt - it;
                                                float angle = vec.GetAngle(dir);
                                                return angle < 1e-6f;
                                            });

            if (boundaryPnt != boundaryPoints.end()) {
                pointsOut.push_back(*boundaryPnt);
            }
            else {
                // go through the boundary edges and check if the point can be directly projected
                // onto one of them
                Base::Vector3f result1, result2;
                for (auto jt : boundaryEdges) {
                    jt.ClosestPointsToLine(it, dir, result1, result2);
                    float dot = (result1 - jt._aclPoints[0]).Dot(result1 - jt._aclPoints[1]);
                    // float distance = Base::Distance(result1, result2);
                    Base::Vector3f vec = result1 - it;
                    float angle = vec.GetAngle(dir);
                    if (dot <= 0 && angle < 1e-6f) {
                        pointsOut.push_back(result1);
                        break;
                    }
                }
            }
        }

        seq.next();
    }
}

void MeshProjection::projectParallelToMesh(const TopoDS_Shape& aShape,
                                           const Base::Vector3f& dir,
                                           std::vector<PolyLine>& rPolyLines) const
{
    // calculate the average edge length and create a grid
    MeshAlgorithm clAlg(_rcMesh);
    float fAvgLen = clAlg.GetAverageEdgeLength();
    MeshFacetGrid cGrid(_rcMesh, 5.0f * fAvgLen);
    TopExp_Explorer Ex;

    int iCnt = 0;
    for (Ex.Init(aShape, TopAbs_EDGE); Ex.More(); Ex.Next()) {
        iCnt++;
    }

    Base::SequencerLauncher seq("Project curve on mesh", iCnt);

    for (Ex.Init(aShape, TopAbs_EDGE); Ex.More(); Ex.Next()) {
        const TopoDS_Edge& aEdge = TopoDS::Edge(Ex.Current());
        std::vector<Base::Vector3f> points;
        discretize(aEdge, points, 5);

        using HitPoint = std::pair<Base::Vector3f, MeshCore::FacetIndex>;
        std::vector<HitPoint> hitPoints;
        using HitPoints = std::pair<HitPoint, HitPoint>;
        std::vector<HitPoints> hitPointPairs;
        for (auto it : points) {
            Base::Vector3f result;
            MeshCore::FacetIndex index;
            if (clAlg.NearestFacetOnRay(it, dir, cGrid, result, index)) {
                hitPoints.emplace_back(result, index);

                if (hitPoints.size() > 1) {
                    HitPoint p1 = hitPoints[hitPoints.size() - 2];
                    HitPoint p2 = hitPoints[hitPoints.size() - 1];
                    hitPointPairs.emplace_back(p1, p2);
                }
            }
        }

        MeshCore::MeshProjection meshProjection(_rcMesh);
        PolyLine polyline;
        for (auto it : hitPointPairs) {
            points.clear();
            if (meshProjection.projectLineOnMesh(cGrid,
                                                 it.first.first,
                                                 it.first.second,
                                                 it.second.first,
                                                 it.second.second,
                                                 dir,
                                                 points)) {
                polyline.points.insert(polyline.points.end(), points.begin(), points.end());
            }
        }
        rPolyLines.push_back(polyline);

        seq.next();
    }
}

void MeshProjection::projectParallelToMesh(const std::vector<PolyLine>& aEdges,
                                           const Base::Vector3f& dir,
                                           std::vector<PolyLine>& rPolyLines) const
{
    // calculate the average edge length and create a grid
    MeshAlgorithm clAlg(_rcMesh);
    float fAvgLen = clAlg.GetAverageEdgeLength();
    MeshFacetGrid cGrid(_rcMesh, 5.0f * fAvgLen);

    Base::SequencerLauncher seq("Project curve on mesh", aEdges.size());

    for (const auto& it : aEdges) {
        std::vector<Base::Vector3f> points = it.points;

        using HitPoint = std::pair<Base::Vector3f, MeshCore::FacetIndex>;
        std::vector<HitPoint> hitPoints;
        using HitPoints = std::pair<HitPoint, HitPoint>;
        std::vector<HitPoints> hitPointPairs;
        for (auto it : points) {
            Base::Vector3f result;
            MeshCore::FacetIndex index;
            if (clAlg.NearestFacetOnRay(it, dir, cGrid, result, index)) {
                hitPoints.emplace_back(result, index);

                if (hitPoints.size() > 1) {
                    HitPoint p1 = hitPoints[hitPoints.size() - 2];
                    HitPoint p2 = hitPoints[hitPoints.size() - 1];
                    hitPointPairs.emplace_back(p1, p2);
                }
            }
        }

        MeshCore::MeshProjection meshProjection(_rcMesh);
        PolyLine polyline;
        for (auto it : hitPointPairs) {
            points.clear();
            if (meshProjection.projectLineOnMesh(cGrid,
                                                 it.first.first,
                                                 it.first.second,
                                                 it.second.first,
                                                 it.second.second,
                                                 dir,
                                                 points)) {
                polyline.points.insert(polyline.points.end(), points.begin(), points.end());
            }
        }
        rPolyLines.push_back(polyline);

        seq.next();
    }
}

void MeshProjection::projectEdgeToEdge(const TopoDS_Edge& aEdge,
                                       float fMaxDist,
                                       const MeshFacetGrid& rGrid,
                                       std::vector<SplitEdge>& rSplitEdges) const
{
    std::vector<MeshCore::FacetIndex> auFInds;
    std::map<std::pair<MeshCore::PointIndex, MeshCore::PointIndex>, std::list<MeshCore::FacetIndex>>
        pEdgeToFace;
    const std::vector<MeshFacet>& rclFAry = _rcMesh.GetFacets();

    // search the facets in the local area of the curve
    std::vector<Base::Vector3f> acPolyLine;
    discretize(aEdge, acPolyLine);

    MeshAlgorithm(_rcMesh).SearchFacetsFromPolyline(acPolyLine, fMaxDist, rGrid, auFInds);
    // remove duplicated elements
    std::sort(auFInds.begin(), auFInds.end());
    auFInds.erase(std::unique(auFInds.begin(), auFInds.end()), auFInds.end());

    // facet to edge
    for (MeshCore::FacetIndex index : auFInds) {
        const MeshFacet& rF = rclFAry[index];
        for (int i = 0; i < 3; i++) {
            MeshCore::PointIndex ulPt0 =
                std::min<MeshCore::PointIndex>(rF._aulPoints[i], rF._aulPoints[(i + 1) % 3]);
            MeshCore::PointIndex ulPt1 =
                std::max<MeshCore::PointIndex>(rF._aulPoints[i], rF._aulPoints[(i + 1) % 3]);
            pEdgeToFace[std::pair<MeshCore::PointIndex, MeshCore::PointIndex>(ulPt0, ulPt1)]
                .push_front(index);
        }
    }

    // sort intersection points by parameter
    std::map<Standard_Real, SplitEdge> rParamSplitEdges;

    BRepAdaptor_Curve clCurve(aEdge);
    Standard_Real fFirst = clCurve.FirstParameter();
    Standard_Real fLast = clCurve.LastParameter();
    Handle(Geom_Curve) hCurve = BRep_Tool::Curve(aEdge, fFirst, fLast);

    // bounds of curve
    //  Bnd_Box clBB;
    //  BndLib_Add3dCurve::Add( BRepAdaptor_Curve(aEdge), 0.0, clBB );

    MeshPointIterator cPI(_rcMesh);
    MeshFacetIterator cFI(_rcMesh);

    Base::SequencerLauncher seq("Project curve on mesh", pEdgeToFace.size());
    std::map<std::pair<MeshCore::PointIndex, MeshCore::PointIndex>,
             std::list<MeshCore::FacetIndex>>::iterator it;
    for (it = pEdgeToFace.begin(); it != pEdgeToFace.end(); ++it) {
        seq.next();

        // edge points
        MeshCore::PointIndex uE0 = it->first.first;
        cPI.Set(uE0);
        Base::Vector3f cE0 = *cPI;
        MeshCore::PointIndex uE1 = it->first.second;
        cPI.Set(uE1);
        Base::Vector3f cE1 = *cPI;

        const std::list<MeshCore::FacetIndex>& auFaces = it->second;
        if (auFaces.size() > 2) {
            continue;  // non-manifold edge -> don't handle this
        }
        //      if ( clBB.IsOut( gp_Pnt(cE0.x, cE0.y, cE0.z) ) && clBB.IsOut( gp_Pnt(cE1.x, cE1.y,
        //      cE1.z) ) )
        //          continue;

        Base::Vector3f cEdgeNormal;
        for (MeshCore::FacetIndex itF : auFaces) {
            cFI.Set(itF);
            cEdgeNormal += cFI->GetNormal();
        }

        // create a plane from the edge normal and point
        Base::Vector3f cPlaneNormal = cEdgeNormal % (cE1 - cE0);
        Handle(Geom_Plane) hPlane =
            new Geom_Plane(gp_Pln(gp_Pnt(cE0.x, cE0.y, cE0.z),
                                  gp_Dir(cPlaneNormal.x, cPlaneNormal.y, cPlaneNormal.z)));

        // get intersection of curve and plane
        GeomAPI_IntCS Alg(hCurve, hPlane);
        if (Alg.IsDone()) {
            Standard_Integer nNbPoints = Alg.NbPoints();
            if (nNbPoints == 1) {
                Standard_Real fU, fV, fW;
                Alg.Parameters(1, fU, fV, fW);

                gp_Pnt P = Alg.Point(1);
                Base::Vector3f cP0((float)P.X(), (float)P.Y(), (float)P.Z());

                float l = ((cP0 - cE0) * (cE1 - cE0)) / ((cE1 - cE0) * (cE1 - cE0));

                // lies the point inside the edge?
                if (l >= 0.0f && l <= 1.0f) {
                    Base::Vector3f cSplitPoint = (1 - l) * cE0 + l * cE1;
                    float fDist = Base::Distance(cP0, cSplitPoint);

                    if (fDist <= fMaxDist) {
                        SplitEdge splitEdge;
                        splitEdge.uE0 = uE0;
                        splitEdge.uE1 = uE1;
                        splitEdge.cPt = cSplitPoint;
                        rParamSplitEdges[fW] = splitEdge;
                    }
                }
            }
            // search for the right solution
            else if (nNbPoints > 1) {
                int nCntSol = 0;
                Base::Vector3f cSplitPoint;
                Standard_Real fSol;
                Base::Vector3f cP0;
                for (int j = 1; j <= nNbPoints; j++) {
                    Standard_Real fU, fV, fW;
                    Alg.Parameters(j, fU, fV, fW);
                    gp_Pnt P = Alg.Point(j);
                    cP0.Set((float)P.X(), (float)P.Y(), (float)P.Z());

                    float l = ((cP0 - cE0) * (cE1 - cE0)) / ((cE1 - cE0) * (cE1 - cE0));

                    // lies the point inside the edge?
                    if (l >= 0.0 && l <= 1.0) {
                        cSplitPoint = (1 - l) * cE0 + l * cE1;
                        float fDist = Base::Distance(cP0, cSplitPoint);

                        if (fDist <= fMaxDist) {
                            nCntSol++;
                            fSol = fW;
                        }
                    }
                }

                // ok, only one sensible solution
                if (nCntSol == 1) {
                    SplitEdge splitEdge;
                    splitEdge.uE0 = uE0;
                    splitEdge.uE1 = uE1;
                    splitEdge.cPt = cSplitPoint;
                    rParamSplitEdges[fSol] = splitEdge;
                }
                else if (nCntSol > 1) {
                    Base::Console().Log("More than one possible intersection points\n");
                }
            }
        }
    }

    // sorted by parameter
    for (const auto& itS : rParamSplitEdges) {
        rSplitEdges.push_back(itS.second);
    }
}
