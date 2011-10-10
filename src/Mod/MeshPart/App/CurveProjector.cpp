/***************************************************************************
 *   Copyright (c) Juergen Riegel         <juergen.riegel@web.de>          *
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
# ifdef FC_OS_LINUX
#	  include <unistd.h>
# endif
#endif


#include "MeshAlgos.h"
#include "CurveProjector.h"

#include <Mod/Mesh/App/Core/MeshIO.h>
#include <Mod/Mesh/App/Core/MeshKernel.h>
#include <Mod/Mesh/App/Core/Iterator.h>
#include <Mod/Mesh/App/Core/Algorithm.h>
#include <Mod/Mesh/App/Mesh.h>

#include <Base/Exception.h>
#include <Base/Console.h>
#include <Base/Sequencer.h>

#include <TopExp_Explorer.hxx>
#include <TopoDS.hxx>
#include <Geom_Curve.hxx>
#include <Geom_Plane.hxx>
#include <BRep_Tool.hxx>
#include <GeomAPI_IntCS.hxx>

using namespace MeshPart;
using namespace MeshCore;




CurveProjector::CurveProjector(const TopoDS_Shape &aShape, const MeshKernel &pMesh)
: _Shape(aShape), _Mesh(pMesh)
{
}

void CurveProjector::writeIntersectionPointsToFile(const char *name)
{
  // export points
  std::ofstream str(name, std::ios::out | std::ios::binary);
  str.precision(4);
  str.setf(std::ios::fixed | std::ios::showpoint);
  for (result_type::const_iterator it1 = mvEdgeSplitPoints.begin();it1!=mvEdgeSplitPoints.end();++it1) {
      for (std::vector<FaceSplitEdge>::const_iterator it2 = it1->second.begin();it2!=it1->second.end();++it2) {
        str << it2->p1.x << " " << it2->p1.y << " " << it2->p1.z << std::endl;
      }
  }
  str.close();
}


//**************************************************************************
//**************************************************************************
// Seperator for additional classes
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

CurveProjectorShape::CurveProjectorShape(const TopoDS_Shape &aShape, const MeshKernel &pMesh)
: CurveProjector(aShape,pMesh)
{
  Do();
}

void CurveProjectorShape::Do(void)
{
  TopExp_Explorer Ex;
  TopoDS_Shape Edge;

  for (Ex.Init(_Shape, TopAbs_EDGE); Ex.More(); Ex.Next())
  {
	  const TopoDS_Edge& aEdge = TopoDS::Edge(Ex.Current());

    //std::vector<FaceSplitEdge> vSplitEdges;
    projectCurve(aEdge, mvEdgeSplitPoints[aEdge]);

  }

}


void CurveProjectorShape::projectCurve( const TopoDS_Edge& aEdge,
                                        std::vector<FaceSplitEdge> &vSplitEdges)
{
  Standard_Real fFirst, fLast;
  Handle(Geom_Curve) hCurve = BRep_Tool::Curve( aEdge,fFirst,fLast );
  
  // getting start point
  gp_Pnt gpPt = hCurve->Value(fFirst);

  // projection of the first point 
  Base::Vector3f cStartPoint = Base::Vector3f((float)gpPt.X(),
                                              (float)gpPt.Y(),
                                              (float)gpPt.Z());
  Base::Vector3f cResultPoint, cSplitPoint, cPlanePnt, cPlaneNormal;
  unsigned long uStartFacetIdx,uCurFacetIdx;
  unsigned long uLastFacetIdx=ULONG_MAX-1; // use another value as ULONG_MAX
  unsigned long auNeighboursIdx[3];
  bool GoOn;
  
  if( !findStartPoint(_Mesh,cStartPoint,cResultPoint,uStartFacetIdx) )
    return;

  uCurFacetIdx = uStartFacetIdx;
  do{
    MeshGeomFacet cCurFacet= _Mesh.GetFacet(uCurFacetIdx);
    _Mesh.GetFacetNeighbours ( uCurFacetIdx, auNeighboursIdx[0], auNeighboursIdx[1], auNeighboursIdx[2]);
    Base::Vector3f PointOnEdge[3];

    GoOn = false;
    int NbrOfHits = 0,HitIdx=0;
    
    for(int i=0; i<3; i++)
    {
      // ignore last visited facet
      if ( auNeighboursIdx[i] == uLastFacetIdx ) 
        continue;

      // get points of the edge i
      const Base::Vector3f& cP0 = cCurFacet._aclPoints[i];
      const Base::Vector3f& cP1 = cCurFacet._aclPoints[(i+1)%3];

      if ( auNeighboursIdx[i] != ULONG_MAX )
      {
        // calculate the normal by the edge vector and the middle between the two face normals
        MeshGeomFacet N = _Mesh.GetFacet( auNeighboursIdx[i] );
        cPlaneNormal = ( N.GetNormal() + cCurFacet.GetNormal() ) % ( cP1 - cP0 );
        cPlanePnt    = cP0;
      }else{
        // with no neighbours the face normal is used
        cPlaneNormal = cCurFacet.GetNormal() % ( cP1 - cP0 );
        cPlanePnt    = cP0;
      }

      Handle(Geom_Plane) hPlane = new Geom_Plane(gp_Pln(gp_Pnt(cPlanePnt.x,cPlanePnt.y,cPlanePnt.z), 
                                                        gp_Dir(cPlaneNormal.x,cPlaneNormal.y,cPlaneNormal.z)));
 
      GeomAPI_IntCS Alg(hCurve,hPlane); 

      if ( Alg.IsDone() )
      {
        // deciding by the number of result points (intersections)
        if( Alg.NbPoints() == 1)
        {
          gp_Pnt P = Alg.Point(1);
          float l = ((Base::Vector3f((float)P.X(),(float)P.Y(),(float)P.Z()) - cP0)
                  * (cP1 - cP0) ) / ((cP1 - cP0) * (cP1 - cP0));
          // is the Point on the Edge of the facet?
          if(l<0.0 || l>1.0)
            PointOnEdge[i] = Base::Vector3f(FLOAT_MAX,0,0);
          else{
            cSplitPoint = (1-l) * cP0 + l * cP1;
            PointOnEdge[i] = (1-l)*cP0 + l * cP1;
            NbrOfHits ++;
            HitIdx = i;
          }
        // no intersection
        }else if(Alg.NbPoints() == 0){
          PointOnEdge[i] = Base::Vector3f(FLOAT_MAX,0,0);
        // more the one intersection (@ToDo)
        }else if(Alg.NbPoints() > 1){
          PointOnEdge[i] = Base::Vector3f(FLOAT_MAX,0,0);
          Base::Console().Log("MeshAlgos::projectCurve(): More then one intersection in Facet %ld, Edge %d\n",uCurFacetIdx,i);
        }
      }
    }

    uLastFacetIdx = uCurFacetIdx;

    if(NbrOfHits == 1)
    {
      uCurFacetIdx = auNeighboursIdx[HitIdx];
      FaceSplitEdge splitEdge;
      splitEdge.ulFaceIndex = uCurFacetIdx;
      splitEdge.p1 = cResultPoint;
      splitEdge.p2 = cSplitPoint;
      vSplitEdges.push_back( splitEdge );
      cResultPoint = cSplitPoint;
      GoOn = true;
    }else{
      Base::Console().Log("MeshAlgos::projectCurve(): Posibel reentry in Facet %ld\n", uCurFacetIdx);
    }

    if( uCurFacetIdx == uStartFacetIdx )
      GoOn = false;

  }while(GoOn);

}

bool CurveProjectorShape::findStartPoint(const MeshKernel &MeshK,const Base::Vector3f &Pnt,Base::Vector3f &Rslt,unsigned long &FaceIndex)
{
  Base::Vector3f TempResultPoint;
  float MinLength = FLOAT_MAX;
  bool bHit = false;

  // go through the whole Mesh
  MeshFacetIterator It(MeshK);
  for(It.Init();It.More();It.Next())
  {
    // try to project (with angle) to the face
    if(It->Foraminate (Pnt, It->GetNormal(), TempResultPoint) )
    {
      // distance to the projected point
      float Dist = (Pnt-TempResultPoint).Length();
      if(Dist < MinLength)
      {
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
// Seperator for CurveProjectorSimple classe
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++


CurveProjectorSimple::CurveProjectorSimple(const TopoDS_Shape &aShape, const MeshKernel &pMesh)
: CurveProjector(aShape,pMesh)
{
  Do();
}


void CurveProjectorSimple::Do(void)
{
  TopExp_Explorer Ex;
  TopoDS_Shape Edge;

  std::vector<Base::Vector3f> vEdgePolygon;

  for (Ex.Init(_Shape, TopAbs_EDGE); Ex.More(); Ex.Next())
  {
	  const TopoDS_Edge& aEdge = TopoDS::Edge(Ex.Current());
//    GetSampledCurves(aEdge,vEdgePolygon,2000);

    //std::vector<FaceSplitEdge> vSplitEdges;
    projectCurve(aEdge,vEdgePolygon, mvEdgeSplitPoints[aEdge]);

  }

}


void CurveProjectorSimple::GetSampledCurves( const TopoDS_Edge& aEdge, std::vector<Base::Vector3f>& rclPoints, unsigned long ulNbOfPoints)
{
  rclPoints.clear();

    Standard_Real fBegin, fEnd;

    Handle(Geom_Curve) hCurve = BRep_Tool::Curve(aEdge,fBegin,fEnd);
    float fLen   = float(fEnd - fBegin);

    for (unsigned long i = 0; i < ulNbOfPoints; i++)
    {
      gp_Pnt gpPt = hCurve->Value(fBegin + (fLen * float(i)) / float(ulNbOfPoints-1));
      rclPoints.push_back(Base::Vector3f((float)gpPt.X(),
                                         (float)gpPt.Y(),
                                         (float)gpPt.Z()));
    }
}


//projectToNeighbours(Handle(Geom_Curve) hCurve,float pos

void CurveProjectorSimple::projectCurve( const TopoDS_Edge& aEdge,
                                         const std::vector<Base::Vector3f> &rclPoints,
                                         std::vector<FaceSplitEdge> &vSplitEdges)
{
  Base::Vector3f /*cResultPoint, cSplitPoint, cPlanePnt, cPlaneNormal,*/TempResultPoint;
  bool bFirst = true;
  //unsigned long auNeighboursIdx[3];
  //std::map<unsigned long,std::vector<Base::Vector3f> >::iterator N1,N2,N3;
  
  Standard_Real fBegin, fEnd;
  Handle(Geom_Curve) hCurve = BRep_Tool::Curve(aEdge,fBegin,fEnd);
  float fLen   = float(fEnd - fBegin);
  
  unsigned long ulNbOfPoints = 1000,PointCount=0;
  
  MeshFacetIterator It(_Mesh);

  Base::SequencerLauncher seq("Building up projection map...", ulNbOfPoints+1);
  std::ofstream str("projected.asc", std::ios::out | std::ios::binary);
  str.precision(4);
  str.setf(std::ios::fixed | std::ios::showpoint);

  std::map<unsigned long,std::vector<Base::Vector3f> > FaceProjctMap;
 
  for (unsigned long i = 0; i <= ulNbOfPoints; i++)
  {
    seq.next();
    gp_Pnt gpPt = hCurve->Value(fBegin + (fLen * float(i)) / float(ulNbOfPoints-1));

    // go through the whole Mesh
    for(It.Init();It.More();It.Next())
    {
      // try to project (with angle) to the face
      if (It->IntersectWithLine (Base::Vector3f((float)gpPt.X(),(float)gpPt.Y(),(float)gpPt.Z()), 
          It->GetNormal(), TempResultPoint))
      {
        FaceProjctMap[It.Position()].push_back(TempResultPoint);
        str << TempResultPoint.x << " " 
            << TempResultPoint.y << " " 
            << TempResultPoint.z << std::endl;
        Base::Console().Log("IDX %d\n",It.Position());

        if(bFirst){
          bFirst = false;
        }

        PointCount++;
      }
    }
  }

  str.close();
  Base::Console().Log("Projection map [%d facets with %d points]\n",FaceProjctMap.size(),PointCount);

  // estimate the first face
//  gp_Pnt gpPt = hCurve->Value(fBegin);
//  if( !findStartPoint(MeshK,Base::Vector3f(gpPt.X(),gpPt.Y(),gpPt.Z()),cResultPoint,uCurFacetIdx) )
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
    MeshK.GetFacetNeighbours ( uCurFacetIdx, auNeighboursIdx[0], auNeighboursIdx[1], auNeighboursIdx[2]);
    
    uCurFacetIdx = ULONG_MAX;
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


  }while(uCurFacetIdx != ULONG_MAX);
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
  unsigned long uStartFacetIdx,uCurFacetIdx;
  unsigned long uLastFacetIdx=ULONG_MAX-1; // use another value as ULONG_MAX
  unsigned long auNeighboursIdx[3];
  bool GoOn;

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
    MeshK.GetFacetNeighbours ( uCurFacetIdx, auNeighboursIdx[0], auNeighboursIdx[1], auNeighboursIdx[2]);

    do{
      // lower the step until you find a neigbourfacet to project...
      fStep /= 2.0;
      // still on the same facet?
      gpPt = hCurve->Value(fAct+fStep);
      if(MeshFacetFunc::IntersectWithLine (cCurFacet, Base::Vector3f(gpPt.X(),gpPt.Y(),gpPt.Z()), cCurFacet.GetNormal(), cResultPoint) )
      {
        fAct += fStep;
        fStep *= 2.0;
        continue;
      }

      HitCount = 0;
      for(int i=0; i<3; i++)
      {
        // if the i'th neighbour is valid
        if ( auNeighboursIdx[i] != ULONG_MAX )
        {
          // try to project next intervall
          MeshGeomFacet N = MeshK.GetFacet( auNeighboursIdx[i] );
          gpPt = hCurve->Value(fAct+fStep);
          if(MeshFacetFunc::IntersectWithLine (*It, Base::Vector3f(gpPt.X(),gpPt.Y(),gpPt.Z()), It->GetNormal(), cResultPoint) )
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
  unsigned long uStartFacetIdx,uCurFacetIdx;
  unsigned long uLastFacetIdx=ULONG_MAX-1; // use another value as ULONG_MAX
  unsigned long auNeighboursIdx[3];
  bool GoOn;
  
  if( !findStartPoint(MeshK,cStartPoint,cResultPoint,uStartFacetIdx) )
    return;

  FILE* file = fopen("projected.asc", "w");
    
  // go through the whole Mesh
  MeshFacetIterator It1(MeshK);
  for(It1.Init();It1.More();It1.Next())
  {
    // cycling through the points and find the first projecteble point ( if the curve starts outside the mesh)
    for( std::vector<Base::Vector3f>::const_iterator It = rclPoints.begin()+1;It!=rclPoints.end();++It)
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

bool CurveProjectorSimple::findStartPoint(const MeshKernel &MeshK,const Base::Vector3f &Pnt,Base::Vector3f &Rslt,unsigned long &FaceIndex)
{
  Base::Vector3f TempResultPoint;
  float MinLength = FLOAT_MAX;
  bool bHit = false;

  // go through the whole Mesh
  MeshFacetIterator It(MeshK);
  for(It.Init();It.More();It.Next())
  {
    // try to project (with angle) to the face
    if(It->Foraminate (Pnt, It->GetNormal(), TempResultPoint) )
    {
      // distance to the projected point
      float Dist = (Pnt-TempResultPoint).Length();
      if(Dist < MinLength)
      {
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
// Seperator for CurveProjectorSimple classe
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++


CurveProjectorWithToolMesh::CurveProjectorWithToolMesh(const TopoDS_Shape &aShape, const MeshKernel &pMesh,MeshKernel &rToolMesh)
: CurveProjector(aShape,pMesh),ToolMesh(rToolMesh)
{
  Do();
}


void CurveProjectorWithToolMesh::Do(void)
{
  TopExp_Explorer Ex;
  TopoDS_Shape Edge;
  std::vector<MeshGeomFacet> cVAry;

  std::vector<Base::Vector3f> vEdgePolygon;

  for (Ex.Init(_Shape, TopAbs_EDGE); Ex.More(); Ex.Next())
  {
	  const TopoDS_Edge& aEdge = TopoDS::Edge(Ex.Current());

    makeToolMesh(aEdge,cVAry);

  }

  ToolMesh.AddFacets(cVAry);

}


//projectToNeighbours(Handle(Geom_Curve) hCurve,float pos

void CurveProjectorWithToolMesh::makeToolMesh( const TopoDS_Edge& aEdge,std::vector<MeshGeomFacet> &cVAry )
{
  Standard_Real fBegin, fEnd;
  Handle(Geom_Curve) hCurve = BRep_Tool::Curve(aEdge,fBegin,fEnd);
  float fLen   = float(fEnd - fBegin);
  Base::Vector3f cResultPoint;

  unsigned long ulNbOfPoints = 15,PointCount=0/*,uCurFacetIdx*/;

  std::vector<LineSeg> LineSegs;

  MeshFacetIterator It(_Mesh);

  Base::SequencerLauncher seq("Building up tool mesh...", ulNbOfPoints+1);  

  std::map<unsigned long,std::vector<Base::Vector3f> > FaceProjctMap;
 
  for (unsigned long i = 0; i < ulNbOfPoints; i++)
  {
    seq.next();
    gp_Pnt gpPt = hCurve->Value(fBegin + (fLen * float(i)) / float(ulNbOfPoints-1));
    Base::Vector3f LinePoint((float)gpPt.X(),
                             (float)gpPt.Y(),
                             (float)gpPt.Z());

    Base::Vector3f ResultNormal;

    // go through the whole Mesh
    for(It.Init();It.More();It.Next())
    {
      // try to project (with angle) to the face
      if (It->IntersectWithLine (Base::Vector3f((float)gpPt.X(),(float)gpPt.Y(),(float)gpPt.Z()),
          It->GetNormal(), cResultPoint) )
      {
        if(Base::Distance(LinePoint,cResultPoint) < 0.5)
          ResultNormal += It->GetNormal();
      }
    }
    LineSeg s;
    s.p = Base::Vector3f((float)gpPt.X(),
                         (float)gpPt.Y(),
                         (float)gpPt.Z());
    s.n = ResultNormal.Normalize();
    LineSegs.push_back(s);
  }

  Base::Console().Log("Projection map [%d facets with %d points]\n",FaceProjctMap.size(),PointCount);


  // build up the new mesh
  Base::Vector3f lp(FLOAT_MAX,0,0), ln, p1, p2, p3, p4,p5,p6;
  float ToolSize = 0.2f;

  for (std::vector<LineSeg>::iterator It2=LineSegs.begin(); It2!=LineSegs.end();++It2)
  {
    if(lp.x != FLOAT_MAX)
    {
      p1 = lp       + (ln       * (-ToolSize));
      p2 = lp       + (ln       *  ToolSize);
      p3 = lp;
      p4 = (*It2).p;
      p5 = (*It2).p + ((*It2).n * (-ToolSize));
      p6 = (*It2).p + ((*It2).n *  ToolSize);

      cVAry.push_back(MeshGeomFacet(p3,p2,p6));
      cVAry.push_back(MeshGeomFacet(p3,p6,p4));
      cVAry.push_back(MeshGeomFacet(p1,p3,p4));
      cVAry.push_back(MeshGeomFacet(p1,p4,p5));

    }

    lp = (*It2).p;
    ln = (*It2).n;
  }



}
