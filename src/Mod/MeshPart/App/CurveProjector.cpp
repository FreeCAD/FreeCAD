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
#  include <unistd.h>
# endif
# include <Bnd_Box.hxx>
# include <BndLib_Add3dCurve.hxx>
# include <BRepAdaptor_Curve.hxx>
# include <GCPnts_UniformDeflection.hxx>
# include <GCPnts_UniformAbscissa.hxx>
# include <gp_Pln.hxx>
# include <TopExp_Explorer.hxx>
# include <TopoDS.hxx>
# include <TopoDS_Edge.hxx>
# include <Geom_Curve.hxx>
# include <Geom_Plane.hxx>
# include <BRep_Tool.hxx>
# include <GeomAPI_IntCS.hxx>
#endif


#include "MeshAlgos.h"
#include "CurveProjector.h"

#include <Mod/Mesh/App/Core/MeshIO.h>
#include <Mod/Mesh/App/Core/MeshKernel.h>
#include <Mod/Mesh/App/Core/Iterator.h>
#include <Mod/Mesh/App/Core/Algorithm.h>
#include <Mod/Mesh/App/Core/Projection.h>
#include <Mod/Mesh/App/Core/Grid.h>
#include <Mod/Mesh/App/Mesh.h>

#include <Base/Exception.h>
#include <Base/Console.h>
#include <Base/Sequencer.h>


using namespace MeshPart;
using MeshCore::MeshKernel;
using MeshCore::MeshFacetIterator;
using MeshCore::MeshPointIterator;
using MeshCore::MeshAlgorithm;
using MeshCore::MeshFacetGrid;
using MeshCore::MeshFacet;

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
// Separator for additional classes
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
          Base::Console().Log("MeshAlgos::projectCurve(): More then one intersection in Facet %lu, Edge %d\n",uCurFacetIdx,i);
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
      Base::Console().Log("MeshAlgos::projectCurve(): Possible reentry in Facet %lu\n", uCurFacetIdx);
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
// Separator for CurveProjectorSimple classes
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
                                         const std::vector<Base::Vector3f> &/*rclPoints*/,
                                         std::vector<FaceSplitEdge> &/*vSplitEdges*/)
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
          // try to project next interval
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
// Separator for CurveProjectorSimple classes
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

// ----------------------------------------------------------------------------

MeshProjection::MeshProjection(const MeshKernel& rMesh)
  : _rcMesh(rMesh)
{
}

MeshProjection::~MeshProjection()
{
}

void MeshProjection::discretize(const TopoDS_Edge& aEdge, std::vector<Base::Vector3f>& polyline, std::size_t minPoints) const
{
    BRepAdaptor_Curve clCurve(aEdge);

    Standard_Real fFirst = clCurve.FirstParameter();
    Standard_Real fLast  = clCurve.LastParameter();

    GCPnts_UniformDeflection clDefl(clCurve, 0.01f, fFirst, fLast);
    if (clDefl.IsDone() == Standard_True) {
        Standard_Integer nNbPoints = clDefl.NbPoints();
        for (Standard_Integer i = 1; i <= nNbPoints; i++) {
            gp_Pnt gpPt = clCurve.Value(clDefl.Parameter(i));
            polyline.push_back( Base::Vector3f( (float)gpPt.X(), (float)gpPt.Y(), (float)gpPt.Z() ) );
        }
    }

    if (polyline.size() < minPoints) {
        GCPnts_UniformAbscissa clAbsc(clCurve, static_cast<Standard_Integer>(minPoints), fFirst, fLast);
        if (clAbsc.IsDone() == Standard_True) {
            polyline.clear();
            Standard_Integer nNbPoints = clAbsc.NbPoints();
            for (Standard_Integer i = 1; i <= nNbPoints; i++) {
                gp_Pnt gpPt = clCurve.Value(clAbsc.Parameter(i));
                polyline.push_back( Base::Vector3f( (float)gpPt.X(), (float)gpPt.Y(), (float)gpPt.Z() ) );
            }
        }
    }
}

void MeshProjection::splitMeshByShape ( const TopoDS_Shape &aShape, float fMaxDist ) const
{
    std::vector<PolyLine> rPolyLines;
    projectToMesh( aShape, fMaxDist, rPolyLines );

    std::ofstream str("output.asc", std::ios::out | std::ios::binary);
    str.precision(4);
    str.setf(std::ios::fixed | std::ios::showpoint);
    for (std::vector<PolyLine>::const_iterator it = rPolyLines.begin();it!=rPolyLines.end();++it) {
        for (std::vector<Base::Vector3f>::const_iterator jt = it->points.begin();jt != it->points.end();++jt)
            str << jt->x << " " << jt->y << " " << jt->z << std::endl;
    }
    str.close();
}

void MeshProjection::projectToMesh (const TopoDS_Shape &aShape, float fMaxDist, std::vector<PolyLine>& rPolyLines) const
{
    // calculate the average edge length and create a grid
    MeshAlgorithm clAlg( _rcMesh );
    float fAvgLen = clAlg.GetAverageEdgeLength();
    MeshFacetGrid cGrid( _rcMesh, 5.0f*fAvgLen );

    TopExp_Explorer Ex;

    int iCnt=0;
    for (Ex.Init(aShape, TopAbs_EDGE); Ex.More(); Ex.Next())
        iCnt++;

    Base::SequencerLauncher seq( "Project curve on mesh", iCnt );

    for (Ex.Init(aShape, TopAbs_EDGE); Ex.More(); Ex.Next()) {
        const TopoDS_Edge& aEdge = TopoDS::Edge(Ex.Current());
        std::vector<SplitEdge> rSplitEdges;
        projectEdgeToEdge(aEdge, fMaxDist, cGrid, rSplitEdges);
        PolyLine polyline;
        polyline.points.reserve(rSplitEdges.size());
        for (auto it : rSplitEdges)
            polyline.points.push_back(it.cPt);
        rPolyLines.push_back(polyline);
        seq.next();
    }
}

void MeshProjection::projectParallelToMesh (const TopoDS_Shape &aShape, const Base::Vector3f& dir, std::vector<PolyLine>& rPolyLines) const
{
    // calculate the average edge length and create a grid
    MeshAlgorithm clAlg(_rcMesh);
    float fAvgLen = clAlg.GetAverageEdgeLength();
    MeshFacetGrid cGrid(_rcMesh, 5.0f*fAvgLen);
    TopExp_Explorer Ex;

    int iCnt=0;
    for (Ex.Init(aShape, TopAbs_EDGE); Ex.More(); Ex.Next())
        iCnt++;

    Base::SequencerLauncher seq( "Project curve on mesh", iCnt );

    for (Ex.Init(aShape, TopAbs_EDGE); Ex.More(); Ex.Next()) {
        const TopoDS_Edge& aEdge = TopoDS::Edge(Ex.Current());
        std::vector<Base::Vector3f> points;
        discretize(aEdge, points, 5);

        typedef std::pair<Base::Vector3f, unsigned long> HitPoint;
        std::vector<HitPoint> hitPoints;
        typedef std::pair<HitPoint, HitPoint> HitPoints;
        std::vector<HitPoints> hitPointPairs;
        for (auto it : points) {
            Base::Vector3f result;
            unsigned long index;
            if (clAlg.NearestFacetOnRay(it, dir, cGrid, result, index)) {
                hitPoints.push_back(std::make_pair(result, index));

                if (hitPoints.size() > 1) {
                    HitPoint p1 = hitPoints[hitPoints.size()-2];
                    HitPoint p2 = hitPoints[hitPoints.size()-1];
                    hitPointPairs.push_back(std::make_pair(p1, p2));
                }
            }
        }

        MeshCore::MeshProjection meshProjection(_rcMesh);
        PolyLine polyline;
        for (auto it : hitPointPairs) {
            points.clear();
            if (meshProjection.projectLineOnMesh(cGrid, it.first.first, it.first.second,
                                                 it.second.first, it.second.second, dir, points)) {
                polyline.points.insert(polyline.points.end(), points.begin(), points.end());
            }
        }
        rPolyLines.push_back(polyline);

        seq.next();
    }
}

void MeshProjection::projectParallelToMesh (const std::vector<PolyLine> &aEdges, const Base::Vector3f& dir, std::vector<PolyLine>& rPolyLines) const
{
    // calculate the average edge length and create a grid
    MeshAlgorithm clAlg(_rcMesh);
    float fAvgLen = clAlg.GetAverageEdgeLength();
    MeshFacetGrid cGrid(_rcMesh, 5.0f*fAvgLen);

    Base::SequencerLauncher seq( "Project curve on mesh", aEdges.size() );

    for (auto it : aEdges) {
        std::vector<Base::Vector3f> points = it.points;

        typedef std::pair<Base::Vector3f, unsigned long> HitPoint;
        std::vector<HitPoint> hitPoints;
        typedef std::pair<HitPoint, HitPoint> HitPoints;
        std::vector<HitPoints> hitPointPairs;
        for (auto it : points) {
            Base::Vector3f result;
            unsigned long index;
            if (clAlg.NearestFacetOnRay(it, dir, cGrid, result, index)) {
                hitPoints.push_back(std::make_pair(result, index));

                if (hitPoints.size() > 1) {
                    HitPoint p1 = hitPoints[hitPoints.size()-2];
                    HitPoint p2 = hitPoints[hitPoints.size()-1];
                    hitPointPairs.push_back(std::make_pair(p1, p2));
                }
            }
        }

        MeshCore::MeshProjection meshProjection(_rcMesh);
        PolyLine polyline;
        for (auto it : hitPointPairs) {
            points.clear();
            if (meshProjection.projectLineOnMesh(cGrid, it.first.first, it.first.second,
                                                 it.second.first, it.second.second, dir, points)) {
                polyline.points.insert(polyline.points.end(), points.begin(), points.end());
            }
        }
        rPolyLines.push_back(polyline);

        seq.next();
    }
}

void MeshProjection::projectEdgeToEdge( const TopoDS_Edge &aEdge, float fMaxDist, const MeshFacetGrid& rGrid,
                                         std::vector<SplitEdge>& rSplitEdges ) const
{
    std::vector<unsigned long> auFInds;
    std::map<std::pair<unsigned long, unsigned long>, std::list<unsigned long> > pEdgeToFace;
    const std::vector<MeshFacet>& rclFAry = _rcMesh.GetFacets();

    // search the facets in the local area of the curve
    std::vector<Base::Vector3f> acPolyLine;
    discretize(aEdge, acPolyLine);

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
    std::map<Standard_Real, SplitEdge> rParamSplitEdges;

    BRepAdaptor_Curve clCurve(aEdge);
    Standard_Real fFirst = clCurve.FirstParameter();
    Standard_Real fLast  = clCurve.LastParameter();
    Handle(Geom_Curve) hCurve = BRep_Tool::Curve( aEdge,fFirst,fLast );

    // bounds of curve
//  Bnd_Box clBB;
//  BndLib_Add3dCurve::Add( BRepAdaptor_Curve(aEdge), 0.0, clBB );

    MeshPointIterator cPI( _rcMesh );
    MeshFacetIterator cFI( _rcMesh );

    Base::SequencerLauncher seq( "Project curve on mesh", pEdgeToFace.size() );
    std::map<std::pair<unsigned long, unsigned long>, std::list<unsigned long> >::iterator it;
    for ( it = pEdgeToFace.begin(); it != pEdgeToFace.end(); ++it ) {
        seq.next();

        // edge points
        unsigned long uE0 = it->first.first;
        cPI.Set( uE0 );
        Base::Vector3f cE0 = *cPI;
        unsigned long uE1 = it->first.second;
        cPI.Set( uE1 );
        Base::Vector3f cE1 = *cPI;

        const std::list<unsigned long>& auFaces = it->second;
        if ( auFaces.size() > 2 )
            continue; // non-manifold edge -> don't handle this
//      if ( clBB.IsOut( gp_Pnt(cE0.x, cE0.y, cE0.z) ) && clBB.IsOut( gp_Pnt(cE1.x, cE1.y, cE1.z) ) )
//          continue;

        Base::Vector3f cEdgeNormal;
        for ( std::list<unsigned long>::const_iterator itF = auFaces.begin(); itF != auFaces.end(); ++itF ) {
            cFI.Set( *itF );
            cEdgeNormal += cFI->GetNormal();
        }

        // create a plane from the edge normal and point
        Base::Vector3f cPlaneNormal = cEdgeNormal % (cE1 - cE0);
        Handle(Geom_Plane) hPlane = new Geom_Plane(gp_Pln(gp_Pnt(cE0.x,cE0.y,cE0.z),
                                    gp_Dir(cPlaneNormal.x,cPlaneNormal.y,cPlaneNormal.z)));

        // get intersection of curve and plane
        GeomAPI_IntCS Alg(hCurve,hPlane);
        if ( Alg.IsDone() ) {
            Standard_Integer nNbPoints = Alg.NbPoints();
            if ( nNbPoints == 1 ) {
                Standard_Real fU, fV, fW;
                Alg.Parameters( 1, fU, fV, fW);

                gp_Pnt P = Alg.Point(1);
                Base::Vector3f cP0((float)P.X(), (float)P.Y(), (float)P.Z());

                float l = ( (cP0 - cE0) * (cE1 - cE0) ) / ( (cE1 - cE0) * ( cE1 - cE0) );

                // lies the point inside the edge?
                if ( l>=0.0f && l<=1.0f ) {
                    Base::Vector3f cSplitPoint = (1-l) * cE0 + l * cE1;
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
                Base::Vector3f cSplitPoint;
                Standard_Real fSol;
                Base::Vector3f cP0;
                for ( int j=1; j<=nNbPoints; j++ ) {
                    Standard_Real fU, fV, fW;
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
    for (std::map<Standard_Real, SplitEdge>::iterator itS =
         rParamSplitEdges.begin(); itS != rParamSplitEdges.end(); ++itS) {
         rSplitEdges.push_back( itS->second );
    }
}
