//  Copyright (C) 2007-2008  CEA/DEN, EDF R&D, OPEN CASCADE
//
//  Copyright (C) 2003-2007  OPEN CASCADE, EADS/CCR, LIP6, CEA/DEN,
//  CEDRAT, EDF R&D, LEG, PRINCIPIA R&D, BUREAU VERITAS
//
//  This library is free software; you can redistribute it and/or
//  modify it under the terms of the GNU Lesser General Public
//  License as published by the Free Software Foundation; either
//  version 2.1 of the License.
//
//  This library is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
//  Lesser General Public License for more details.
//
//  You should have received a copy of the GNU Lesser General Public
//  License along with this library; if not, write to the Free Software
//  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
//
//  See http://www.salome-platform.org/ or email : webmaster.salome@opencascade.com
//
//  SMESH SMESH : implementaion of SMESH idl descriptions
// File      : StdMeshers_QuadToTriaAdaptor.cxx
// Module    : SMESH
// Created   : Wen May 07 16:37:07 2008
// Author    : Sergey KUUL (skl)
//
#ifdef _MSC_VER
#define _USE_MATH_DEFINES
#endif // _MSC_VER
#include <cmath>

#include "StdMeshers_QuadToTriaAdaptor.hxx"

#include <SMDS_FaceOfNodes.hxx>
#include <SMESH_Algo.hxx>
#include <SMESH_MesherHelper.hxx>

#include <IntAna_IntConicQuad.hxx>
#include <IntAna_Quadric.hxx>
#include <TColStd_SequenceOfInteger.hxx>
#include <TColgp_HSequenceOfPnt.hxx>
#include <TopExp_Explorer.hxx>
#include <TopoDS.hxx>
#include <gp_Lin.hxx>
#include <gp_Pln.hxx>

#ifndef PI
#define PI M_PI
#endif

#ifndef __BORLANDC__
#include <NCollection_Array1.hxx>
typedef NCollection_Array1<TColStd_SequenceOfInteger> StdMeshers_Array1OfSequenceOfInteger;
#else
#include <SMESH_Array1.hxx>
typedef SMESH_Array1<TColStd_SequenceOfInteger> StdMeshers_Array1OfSequenceOfInteger;
#endif


//=======================================================================
//function : StdMeshers_QuadToTriaAdaptor
//purpose  : 
//=======================================================================

StdMeshers_QuadToTriaAdaptor::StdMeshers_QuadToTriaAdaptor()
{
}


//================================================================================
/*!
 * \brief Destructor
 */
//================================================================================

StdMeshers_QuadToTriaAdaptor::~StdMeshers_QuadToTriaAdaptor()
{
  // delete temporary faces
  map< const SMDS_MeshElement*, list<const SMDS_FaceOfNodes*> >::iterator
    f_f = myResMap.begin(), ffEnd = myResMap.end();
  for ( ; f_f != ffEnd; ++f_f )
  {
    list<const SMDS_FaceOfNodes*>& fList = f_f->second;
    list<const SMDS_FaceOfNodes*>::iterator f = fList.begin(), fEnd = fList.end();
    for ( ; f != fEnd; ++f )
      delete *f;
  }
  myResMap.clear();

//   TF2PyramMap::iterator itp = myMapFPyram.begin();
//   for(; itp!=myMapFPyram.end(); itp++)
//     cout << itp->second << endl;
}


//=======================================================================
//function : FindBestPoint
//purpose  : Auxilare for Compute()
//           V - normal to (P1,P2,PC)
//=======================================================================
static gp_Pnt FindBestPoint(const gp_Pnt& P1, const gp_Pnt& P2,
                            const gp_Pnt& PC, const gp_Vec& V)
{
  double a = P1.Distance(P2);
  double b = P1.Distance(PC);
  double c = P2.Distance(PC);
  if( a < (b+c)/2 )
    return PC;
  else {
    // find shift along V in order to a became equal to (b+c)/2
    double shift = sqrt( a*a + (b*b-c*c)*(b*b-c*c)/16/a/a - (b*b+c*c)/2 );
    gp_Dir aDir(V);
    gp_Pnt Pbest( PC.X() + aDir.X()*shift,  PC.Y() + aDir.Y()*shift,
                  PC.Z() + aDir.Z()*shift );
    return Pbest;
  }
}


//=======================================================================
//function : HasIntersection3
//purpose  : Auxilare for HasIntersection()
//           find intersection point between triangle (P1,P2,P3)
//           and segment [PC,P]
//=======================================================================
static bool HasIntersection3(const gp_Pnt& P, const gp_Pnt& PC, gp_Pnt& Pint,
                             const gp_Pnt& P1, const gp_Pnt& P2, const gp_Pnt& P3)
{
  //cout<<"HasIntersection3"<<endl;
  //cout<<"  PC("<<PC.X()<<","<<PC.Y()<<","<<PC.Z()<<")"<<endl;
  //cout<<"  P("<<P.X()<<","<<P.Y()<<","<<P.Z()<<")"<<endl;
  //cout<<"  P1("<<P1.X()<<","<<P1.Y()<<","<<P1.Z()<<")"<<endl;
  //cout<<"  P2("<<P2.X()<<","<<P2.Y()<<","<<P2.Z()<<")"<<endl;
  //cout<<"  P3("<<P3.X()<<","<<P3.Y()<<","<<P3.Z()<<")"<<endl;
  gp_Vec VP1(P1,P2);
  gp_Vec VP2(P1,P3);
  IntAna_Quadric IAQ(gp_Pln(P1,VP1.Crossed(VP2)));
  IntAna_IntConicQuad IAICQ(gp_Lin(PC,gp_Dir(gp_Vec(PC,P))),IAQ);
  if(IAICQ.IsDone()) {
    if( IAICQ.IsInQuadric() )
      return false;
    if( IAICQ.NbPoints() == 1 ) {
      gp_Pnt PIn = IAICQ.Point(1);
      double preci = 1.e-6;
      // check if this point is internal for segment [PC,P]
      bool IsExternal =
        ( (PC.X()-PIn.X())*(P.X()-PIn.X()) > preci ) ||
        ( (PC.Y()-PIn.Y())*(P.Y()-PIn.Y()) > preci ) ||
        ( (PC.Z()-PIn.Z())*(P.Z()-PIn.Z()) > preci );
      if(IsExternal) {
        return false;
      }
      // check if this point is internal for triangle (P1,P2,P3)
      gp_Vec V1(PIn,P1);
      gp_Vec V2(PIn,P2);
      gp_Vec V3(PIn,P3);
      if( V1.Magnitude()<preci || V2.Magnitude()<preci ||
          V3.Magnitude()<preci ) {
        Pint = PIn;
        return true;
      }
      gp_Vec VC1 = V1.Crossed(V2);
      gp_Vec VC2 = V2.Crossed(V3);
      gp_Vec VC3 = V3.Crossed(V1);
      if(VC1.Magnitude()<preci) {
        if(VC2.IsOpposite(VC3,preci)) {
          return false;
        }
      }
      else if(VC2.Magnitude()<preci) {
        if(VC1.IsOpposite(VC3,preci)) {
          return false;
        }
      }
      else if(VC3.Magnitude()<preci) {
        if(VC1.IsOpposite(VC2,preci)) {
          return false;
        }
      }
      else {
        if( VC1.IsOpposite(VC2,preci) || VC1.IsOpposite(VC3,preci) ||
            VC2.IsOpposite(VC3,preci) ) {
          return false;
        }
      }
      Pint = PIn;
      return true;
    }
  }

  return false;
}


//=======================================================================
//function : HasIntersection
//purpose  : Auxilare for CheckIntersection()
//=======================================================================
static bool HasIntersection(const gp_Pnt& P, const gp_Pnt& PC, gp_Pnt& Pint,
                            Handle(TColgp_HSequenceOfPnt)& aContour)
{
  if(aContour->Length()==3) {
    return HasIntersection3( P, PC, Pint, aContour->Value(1),
                             aContour->Value(2), aContour->Value(3) );
  }
  else {
    bool check = false;
    if( (aContour->Value(1).Distance(aContour->Value(2)) > 1.e-6) &&
        (aContour->Value(1).Distance(aContour->Value(3)) > 1.e-6) &&
        (aContour->Value(2).Distance(aContour->Value(3)) > 1.e-6) ) {
      check = HasIntersection3( P, PC, Pint, aContour->Value(1),
                                aContour->Value(2), aContour->Value(3) );
    }
    if(check) return true;
    if( (aContour->Value(1).Distance(aContour->Value(4)) > 1.e-6) &&
        (aContour->Value(1).Distance(aContour->Value(3)) > 1.e-6) &&
        (aContour->Value(4).Distance(aContour->Value(3)) > 1.e-6) ) {
      check = HasIntersection3( P, PC, Pint, aContour->Value(1),
                                aContour->Value(3), aContour->Value(4) );
    }
    if(check) return true;
  }

  return false;
}


//=======================================================================
//function : CheckIntersection
//purpose  : Auxilare for Compute()
//           NotCheckedFace - for optimization
//=======================================================================
bool StdMeshers_QuadToTriaAdaptor::CheckIntersection
                       (const gp_Pnt& P, const gp_Pnt& PC,
                        gp_Pnt& Pint, SMESH_Mesh& aMesh,
                        const TopoDS_Shape& aShape,
                        const TopoDS_Shape& NotCheckedFace)
{
  SMESHDS_Mesh * meshDS = aMesh.GetMeshDS();
  //cout<<"    CheckIntersection: meshDS->NbFaces() = "<<meshDS->NbFaces()<<endl;
  bool res = false;
  double dist = RealLast();
  gp_Pnt Pres;
  for (TopExp_Explorer exp(aShape,TopAbs_FACE);exp.More();exp.Next()) {
    const TopoDS_Shape& aShapeFace = exp.Current();
    if(aShapeFace==NotCheckedFace)
      continue;
    const SMESHDS_SubMesh * aSubMeshDSFace = meshDS->MeshElements(aShapeFace);
    if ( aSubMeshDSFace ) {
      SMDS_ElemIteratorPtr iteratorElem = aSubMeshDSFace->GetElements();
      while ( iteratorElem->more() ) { // loop on elements on a face
        const SMDS_MeshElement* face = iteratorElem->next();
        Handle(TColgp_HSequenceOfPnt) aContour = new TColgp_HSequenceOfPnt;
        SMDS_ElemIteratorPtr nodeIt = face->nodesIterator();
        int nbN = face->NbNodes();
        if( face->IsQuadratic() )
          nbN /= 2;
        for ( int i = 0; i < nbN; ++i ) {
          const SMDS_MeshNode* node = static_cast<const SMDS_MeshNode*>( nodeIt->next() );
          aContour->Append(gp_Pnt(node->X(), node->Y(), node->Z()));
        }
        if( HasIntersection(P, PC, Pres, aContour) ) {
          res = true;
          double tmp = PC.Distance(Pres);
          if(tmp<dist) {
            Pint = Pres;
            dist = tmp;
          }
        }
      }
    }
  }
  return res;
}


//=======================================================================
//function : CompareTrias
//purpose  : Auxilare for Compute()
//=======================================================================
static bool CompareTrias(const SMDS_MeshElement* F1,const SMDS_MeshElement* F2)
{
  return
    ( F1->GetNode(1)==F2->GetNode(2) && F1->GetNode(2)==F2->GetNode(1) ) ||
    ( F1->GetNode(1)==F2->GetNode(1) && F1->GetNode(2)==F2->GetNode(2) );
}


//=======================================================================
//function : IsDegenarate
//purpose  : Auxilare for Preparation()
//=======================================================================
// static int IsDegenarate(const Handle(TColgp_HArray1OfPnt)& PN)
// {
//   int i = 1;
//   for(; i<4; i++) {
//     int j = i+1;
//     for(; j<=4; j++) {
//       if( PN->Value(i).Distance(PN->Value(j)) < 1.e-6 )
//         return j;
//     }
//   }
//   return 0;
// }


//=======================================================================
//function : Preparation
//purpose  : Auxilare for Compute()
//         : Return 0 if given face is not quad,
//                  1 if given face is quad,
//                  2 if given face is degenerate quad (two nodes are coincided)
//=======================================================================
int StdMeshers_QuadToTriaAdaptor::Preparation(const SMDS_MeshElement* face,
                                              Handle(TColgp_HArray1OfPnt)& PN,
                                              Handle(TColgp_HArray1OfVec)& VN,
                                              std::vector<const SMDS_MeshNode*>& FNodes,
                                              gp_Pnt& PC, gp_Vec& VNorm)
{
  int i = 0;
  double xc=0., yc=0., zc=0.;
  SMDS_ElemIteratorPtr nodeIt = face->nodesIterator();
  if( !face->IsQuadratic() ) {
    if( face->NbNodes() != 4 )
      return 0;
    while ( nodeIt->more() ) {
      i++;
      const SMDS_MeshNode* node = static_cast<const SMDS_MeshNode*>( nodeIt->next() );
      FNodes[i-1] = node;
      PN->SetValue( i, gp_Pnt(node->X(), node->Y(), node->Z()) );
      xc += node->X();
      yc += node->Y();
      zc += node->Z();
    }
  }
  else {
    if( face->NbNodes() != 8)
      return 0;
    while ( nodeIt->more() ) {
      i++;
      const SMDS_MeshNode* node = static_cast<const SMDS_MeshNode*>( nodeIt->next() );
      FNodes[i-1] = node;
      PN->SetValue( i, gp_Pnt(node->X(), node->Y(), node->Z()) );
      xc += node->X();
      yc += node->Y();
      zc += node->Z();
      if(i==4) break;
    }
  }

  int nbp = 4;

  int j = 0;
  for(i=1; i<4; i++) {
    j = i+1;
    for(; j<=4; j++) {
      if( PN->Value(i).Distance(PN->Value(j)) < 1.e-6 )
        break;
    }
    if(j<=4) break;
  }
  //int deg_num = IsDegenarate(PN);
  //if(deg_num>0) {
  bool hasdeg = false;
  if(i<4) {
    //cout<<"find degeneration"<<endl;
    hasdeg = true;
    gp_Pnt Pdeg = PN->Value(i);

    std::list< const SMDS_MeshNode* >::iterator itdg = myDegNodes.begin();
    const SMDS_MeshNode* DegNode = 0;
    for(; itdg!=myDegNodes.end(); itdg++) {
      const SMDS_MeshNode* N = (*itdg);
      gp_Pnt Ptmp(N->X(),N->Y(),N->Z());
      if(Pdeg.Distance(Ptmp)<1.e-6) {
        DegNode = N;
        //DegNode = const_cast<SMDS_MeshNode*>(N);
        break;
      }
    }
    if(!DegNode) {
      DegNode = FNodes[i-1];
      myDegNodes.push_back(DegNode);
    }
    else {
      FNodes[i-1] = DegNode;
    }
    for(i=j; i<4; i++) {
      PN->SetValue(i,PN->Value(i+1));
      FNodes[i-1] = FNodes[i];
    }
    nbp = 3;
    //PC = gp_Pnt( PN->Value(1).X() + PN.Value
  }

  PC = gp_Pnt(xc/4., yc/4., zc/4.);
  //cout<<"  PC("<<PC.X()<<","<<PC.Y()<<","<<PC.Z()<<")"<<endl;

  //PN->SetValue(5,PN->Value(1));
  PN->SetValue(nbp+1,PN->Value(1));
  //FNodes[4] = FNodes[0];
  FNodes[nbp] = FNodes[0];
  // find normal direction
  //gp_Vec V1(PC,PN->Value(4));
  gp_Vec V1(PC,PN->Value(nbp));
  gp_Vec V2(PC,PN->Value(1));
  VNorm = V1.Crossed(V2);
  //VN->SetValue(4,VNorm);
  VN->SetValue(nbp,VNorm);
  //for(i=1; i<4; i++) {
  for(i=1; i<nbp; i++) {
    V1 = gp_Vec(PC,PN->Value(i));
    V2 = gp_Vec(PC,PN->Value(i+1));
    gp_Vec Vtmp = V1.Crossed(V2);
    VN->SetValue(i,Vtmp);
    VNorm += Vtmp;
  }
  //cout<<"  VNorm("<<VNorm.X()<<","<<VNorm.Y()<<","<<VNorm.Z()<<")"<<endl;
  if(hasdeg) return 2;
  return 1;
}


//=======================================================================
//function : Compute
//purpose  : 
//=======================================================================

bool StdMeshers_QuadToTriaAdaptor::Compute(SMESH_Mesh& aMesh, const TopoDS_Shape& aShape)
{
  myResMap.clear();
  myMapFPyram.clear();

  SMESHDS_Mesh * meshDS = aMesh.GetMeshDS();
  SMESH_MesherHelper helper(aMesh);
  helper.IsQuadraticSubMesh(aShape);
  helper.SetElementsOnShape( true );

  for (TopExp_Explorer exp(aShape,TopAbs_FACE);exp.More();exp.Next()) {
    const TopoDS_Shape& aShapeFace = exp.Current();
    const SMESHDS_SubMesh * aSubMeshDSFace = meshDS->MeshElements( aShapeFace );
    if ( aSubMeshDSFace ) {
      bool isRev = SMESH_Algo::IsReversedSubMesh( TopoDS::Face(aShapeFace), meshDS );

      SMDS_ElemIteratorPtr iteratorElem = aSubMeshDSFace->GetElements();
      while ( iteratorElem->more() ) { // loop on elements on a face
        const SMDS_MeshElement* face = iteratorElem->next();
        //cout<<endl<<"================= face->GetID() = "<<face->GetID()<<endl;
        // preparation step using face info
        Handle(TColgp_HArray1OfPnt) PN = new TColgp_HArray1OfPnt(1,5);
        Handle(TColgp_HArray1OfVec) VN = new TColgp_HArray1OfVec(1,4);
        std::vector<const SMDS_MeshNode*> FNodes(5);
        gp_Pnt PC;
        gp_Vec VNorm;
        int stat =  Preparation(face, PN, VN, FNodes, PC, VNorm);
        if(stat==0)
          continue;

        if(stat==2) {
          // degenerate face
          // add triangles to result map
          std::list<const SMDS_FaceOfNodes*> aList;
          SMDS_FaceOfNodes* NewFace;
          if(!isRev)
            NewFace = new SMDS_FaceOfNodes( FNodes[0], FNodes[1], FNodes[2] );
          else
            NewFace = new SMDS_FaceOfNodes( FNodes[0], FNodes[2], FNodes[1] );
          aList.push_back(NewFace);
          myResMap.insert(make_pair(face,aList));
          continue;
        }

        if(!isRev) VNorm.Reverse();
        double xc = 0., yc = 0., zc = 0.;
        int i = 1;
        for(; i<=4; i++) {
          gp_Pnt Pbest;
          if(!isRev)
            Pbest = FindBestPoint(PN->Value(i), PN->Value(i+1), PC, VN->Value(i).Reversed());
          else
            Pbest = FindBestPoint(PN->Value(i), PN->Value(i+1), PC, VN->Value(i));
          xc += Pbest.X();
          yc += Pbest.Y();
          zc += Pbest.Z();
        }
        gp_Pnt PCbest(xc/4., yc/4., zc/4.);

        // check PCbest
        double height = PCbest.Distance(PC);
        if(height<1.e-6) {
          // create new PCbest using a bit shift along VNorm
          PCbest = PC.XYZ() + VNorm.XYZ() * 0.001;
        }
        else {
          // check possible intersection with other faces
          gp_Pnt Pint;
          bool check = CheckIntersection(PCbest, PC, Pint, aMesh, aShape, aShapeFace);
          if(check) {
            //cout<<"--PC("<<PC.X()<<","<<PC.Y()<<","<<PC.Z()<<")"<<endl;
            //cout<<"  PCbest("<<PCbest.X()<<","<<PCbest.Y()<<","<<PCbest.Z()<<")"<<endl;
            double dist = PC.Distance(Pint)/3.;
            gp_Dir aDir(gp_Vec(PC,PCbest));
            PCbest = PC.XYZ() + aDir.XYZ() * dist;
          }
          else {
            gp_Vec VB(PC,PCbest);
            gp_Pnt PCbestTmp = PC.XYZ() + VB.XYZ() * 3.0;
            bool check = CheckIntersection(PCbestTmp, PC, Pint, aMesh, aShape, aShapeFace);
            if(check) {
              double dist = PC.Distance(Pint)/3.;
              if(dist<height) {
                gp_Dir aDir(gp_Vec(PC,PCbest));
                PCbest = PC.XYZ() + aDir.XYZ() * dist;
              }
            }
          }
        }
        // create node for PCbest
        SMDS_MeshNode* NewNode = helper.AddNode( PCbest.X(), PCbest.Y(), PCbest.Z() );
        // add triangles to result map
        std::list<const SMDS_FaceOfNodes*> aList;
        for(i=0; i<4; i++) {
          SMDS_FaceOfNodes* NewFace = new SMDS_FaceOfNodes( NewNode, FNodes[i], FNodes[i+1] );
          aList.push_back(NewFace);
        }
        myResMap.insert(make_pair(face,aList));
        // create pyramid
        SMDS_MeshVolume* aPyram =
          helper.AddVolume( FNodes[0], FNodes[1], FNodes[2], FNodes[3], NewNode );
        myMapFPyram.insert(make_pair(face,aPyram));
      } // end loop on elements on a face
    }
  } // end for(TopExp_Explorer exp(aShape,TopAbs_FACE);exp.More();exp.Next()) {

  return Compute2ndPart(aMesh);
}


//=======================================================================
//function : Compute
//purpose  : 
//=======================================================================

bool StdMeshers_QuadToTriaAdaptor::Compute(SMESH_Mesh& aMesh)
{
  myResMap.clear();
  myMapFPyram.clear();
  SMESH_MesherHelper helper(aMesh);
  helper.IsQuadraticSubMesh(aMesh.GetShapeToMesh());
  helper.SetElementsOnShape( true );

  SMESHDS_Mesh * meshDS = aMesh.GetMeshDS();

  SMDS_FaceIteratorPtr fIt = meshDS->facesIterator();
  TIDSortedElemSet sortedFaces; //  0020279: control the "random" use when using mesh algorithms
  while( fIt->more()) sortedFaces.insert( fIt->next() );

  TIDSortedElemSet::iterator itFace = sortedFaces.begin(), fEnd = sortedFaces.end();
  for ( ; itFace != fEnd; ++itFace )
  {
    const SMDS_MeshElement* face = *itFace;
    if ( !face ) continue;
    //cout<<endl<<"================= face->GetID() = "<<face->GetID()<<endl;
    // preparation step using face info
    Handle(TColgp_HArray1OfPnt) PN = new TColgp_HArray1OfPnt(1,5);
    Handle(TColgp_HArray1OfVec) VN = new TColgp_HArray1OfVec(1,4);
    std::vector<const SMDS_MeshNode*> FNodes(5);
    gp_Pnt PC;
    gp_Vec VNorm;

    int stat =  Preparation(face, PN, VN, FNodes, PC, VNorm);
    if(stat==0)
      continue;

    if(stat==2) {
      // degenerate face
      // add triangles to result map
      std::list<const SMDS_FaceOfNodes*> aList;
      SMDS_FaceOfNodes* NewFace;
      // check orientation

      double tmp = PN->Value(1).Distance(PN->Value(2)) +
        PN->Value(2).Distance(PN->Value(3));
      gp_Pnt Ptmp1 = PC.XYZ() + VNorm.XYZ() * tmp * 1.e6;
      gp_Pnt Ptmp2 = PC.XYZ() - VNorm.XYZ() * tmp * 1.e6;
      // check intersection for Ptmp1 and Ptmp2
      bool IsRev = false;
      bool IsOK1 = false;
      bool IsOK2 = false;
      double dist1 = RealLast();
      double dist2 = RealLast();
      gp_Pnt Pres1,Pres2;
      for (TIDSortedElemSet::iterator itF = sortedFaces.begin(); itF != fEnd; ++itF ) {
        const SMDS_MeshElement* F = *itF;
        if(F==face) continue;
        Handle(TColgp_HSequenceOfPnt) aContour = new TColgp_HSequenceOfPnt;
        SMDS_ElemIteratorPtr nodeIt = F->nodesIterator();
        if( !F->IsQuadratic() ) {
          while ( nodeIt->more() ) {
            const SMDS_MeshNode* node = static_cast<const SMDS_MeshNode*>( nodeIt->next() );
            aContour->Append(gp_Pnt(node->X(), node->Y(), node->Z()));
          }
        }
        else {
          int nn = 0;
          while ( nodeIt->more() ) {
            nn++;
            const SMDS_MeshNode* node = static_cast<const SMDS_MeshNode*>( nodeIt->next() );
            aContour->Append(gp_Pnt(node->X(), node->Y(), node->Z()));
            if(nn==face->NbNodes()/2) break;
          }
        }
        gp_Pnt PPP;
        if( HasIntersection(Ptmp1, PC, PPP, aContour) ) {
          IsOK1 = true;
          double tmp = PC.Distance(PPP);
          if(tmp<dist1) {
            Pres1 = PPP;
            dist1 = tmp;
          }
        }
        if( HasIntersection(Ptmp2, PC, PPP, aContour) ) {
          IsOK2 = true;
          double tmp = PC.Distance(PPP);
          if(tmp<dist2) {
            Pres2 = PPP;
            dist2 = tmp;
          }
        }
      }

      if( IsOK1 && !IsOK2 ) {
        // using existed direction
      }
      else if( !IsOK1 && IsOK2 ) {
        // using opposite direction
        IsRev = true;
      }
      else { // IsOK1 && IsOK2
        double tmp1 = PC.Distance(Pres1)/3.;
        double tmp2 = PC.Distance(Pres2)/3.;
        if(tmp1<tmp2) {
          // using existed direction
        }
        else {
          // using opposite direction
          IsRev = true;
        }
      }
      if(!IsRev)
        NewFace = new SMDS_FaceOfNodes( FNodes[0], FNodes[1], FNodes[2] );
      else
        NewFace = new SMDS_FaceOfNodes( FNodes[0], FNodes[2], FNodes[1] );
      aList.push_back(NewFace);
      myResMap.insert(make_pair(face,aList));
      continue;
    }
    
    double xc = 0., yc = 0., zc = 0.;
    int i = 1;
    for(; i<=4; i++) {
      gp_Pnt Pbest = FindBestPoint(PN->Value(i), PN->Value(i+1), PC, VN->Value(i));
      xc += Pbest.X();
      yc += Pbest.Y();
      zc += Pbest.Z();
    }
    gp_Pnt PCbest(xc/4., yc/4., zc/4.);
    double height = PCbest.Distance(PC);
    if(height<1.e-6) {
      // create new PCbest using a bit shift along VNorm
      PCbest = PC.XYZ() + VNorm.XYZ() * 0.001;
      height = PCbest.Distance(PC);
    }
    //cout<<"  PCbest("<<PCbest.X()<<","<<PCbest.Y()<<","<<PCbest.Z()<<")"<<endl;

    gp_Vec V1(PC,PCbest);
    double tmp = PN->Value(1).Distance(PN->Value(3)) +
      PN->Value(2).Distance(PN->Value(4));
    gp_Dir tmpDir(V1);
    gp_Pnt Ptmp1 = PC.XYZ() + tmpDir.XYZ() * tmp * 1.e6;
    gp_Pnt Ptmp2 = PC.XYZ() - tmpDir.XYZ() * tmp * 1.e6;
    // check intersection for Ptmp1 and Ptmp2
    bool IsRev = false;
    bool IsOK1 = false;
    bool IsOK2 = false;
    double dist1 = RealLast();
    double dist2 = RealLast();
    gp_Pnt Pres1,Pres2;
    for (TIDSortedElemSet::iterator itF = sortedFaces.begin(); itF != fEnd; ++itF ) {
      const SMDS_MeshElement* F = *itF;
      if(F==face) continue;
      Handle(TColgp_HSequenceOfPnt) aContour = new TColgp_HSequenceOfPnt;
      SMDS_ElemIteratorPtr nodeIt = F->nodesIterator();
      if( !F->IsQuadratic() ) {
        while ( nodeIt->more() ) {
          const SMDS_MeshNode* node = static_cast<const SMDS_MeshNode*>( nodeIt->next() );
          aContour->Append(gp_Pnt(node->X(), node->Y(), node->Z()));
        }
      }
      else {
        int nn = 0;
        while ( nodeIt->more() ) {
          nn++;
          const SMDS_MeshNode* node = static_cast<const SMDS_MeshNode*>( nodeIt->next() );
          aContour->Append(gp_Pnt(node->X(), node->Y(), node->Z()));
          if(nn==face->NbNodes()/2) break;
        }
      }
      gp_Pnt PPP;
      if( HasIntersection(Ptmp1, PC, PPP, aContour) ) {
        IsOK1 = true;
        double tmp = PC.Distance(PPP);
        if(tmp<dist1) {
          Pres1 = PPP;
          dist1 = tmp;
        }
      }
      if( HasIntersection(Ptmp2, PC, PPP, aContour) ) {
        IsOK2 = true;
        double tmp = PC.Distance(PPP);
        if(tmp<dist2) {
          Pres2 = PPP;
          dist2 = tmp;
        }
      }
    }

    if( IsOK1 && !IsOK2 ) {
      // using existed direction
      double tmp = PC.Distance(Pres1)/3.;
      if( height > tmp ) {
        height = tmp;
        PCbest = PC.XYZ() + tmpDir.XYZ() * height;
      }
    }
    else if( !IsOK1 && IsOK2 ) {
      // using opposite direction
      IsRev = true;
      double tmp = PC.Distance(Pres2)/3.;
      if( height > tmp ) height = tmp;
      PCbest = PC.XYZ() - tmpDir.XYZ() * height;
    }
    else { // IsOK1 && IsOK2
      double tmp1 = PC.Distance(Pres1)/3.;
      double tmp2 = PC.Distance(Pres2)/3.;
      if(tmp1<tmp2) {
        // using existed direction
        if( height > tmp1 ) {
          height = tmp1;
          PCbest = PC.XYZ() + tmpDir.XYZ() * height;
        }
      }
      else {
        // using opposite direction
        IsRev = true;
        if( height > tmp2 ) height = tmp2;
        PCbest = PC.XYZ() - tmpDir.XYZ() * height;
      }
    }

    // create node for PCbest
    SMDS_MeshNode* NewNode = helper.AddNode( PCbest.X(), PCbest.Y(), PCbest.Z() );
    // add triangles to result map
    std::list<const SMDS_FaceOfNodes*> aList;
    for(i=0; i<4; i++) {
      SMDS_FaceOfNodes* NewFace;
      if(IsRev)
        NewFace = new SMDS_FaceOfNodes( NewNode, FNodes[i], FNodes[i+1] );
      else
        NewFace = new SMDS_FaceOfNodes( NewNode, FNodes[i+1], FNodes[i] );
      aList.push_back(NewFace);
    }
    myResMap.insert(make_pair(face,aList));
    // create pyramid
    SMDS_MeshVolume* aPyram;
    if(IsRev)
     aPyram = helper.AddVolume( FNodes[0], FNodes[1], FNodes[2], FNodes[3], NewNode );
    else
     aPyram = helper.AddVolume( FNodes[0], FNodes[3], FNodes[2], FNodes[1], NewNode );
    myMapFPyram.insert(make_pair(face,aPyram));
  } // end loop on elements on a face

  return Compute2ndPart(aMesh);
}


//=======================================================================
//function : Compute2ndPart
//purpose  : 
//=======================================================================

bool StdMeshers_QuadToTriaAdaptor::Compute2ndPart(SMESH_Mesh& aMesh)
{
  SMESHDS_Mesh * meshDS = aMesh.GetMeshDS();

  // check intersections between created pyramids
  int NbPyram = myMapFPyram.size();
  //cout<<"NbPyram = "<<NbPyram<<endl;
  if(NbPyram==0)
    return true;

  vector< const SMDS_MeshElement* > Pyrams(NbPyram);
  vector< const SMDS_MeshElement* > Faces(NbPyram);
  TF2PyramMap::iterator itp = myMapFPyram.begin();
  int i = 0;
  for(; itp!=myMapFPyram.end(); itp++, i++) {
    Faces[i] = (*itp).first;
    Pyrams[i] = (*itp).second;
  }
  StdMeshers_Array1OfSequenceOfInteger MergesInfo(0,NbPyram-1);
  for(i=0; i<NbPyram; i++) {
    TColStd_SequenceOfInteger aMerges;
    aMerges.Append(i);
    MergesInfo.SetValue(i,aMerges);
  }
  for(i=0; i<NbPyram-1; i++) {
    const SMDS_MeshElement* Prm1 = Pyrams[i];
    SMDS_ElemIteratorPtr nIt = Prm1->nodesIterator();
    std::vector<gp_Pnt>            Ps1( Prm1->NbNodes() );
    vector< const SMDS_MeshNode* > Ns1( Prm1->NbNodes() );
    int k = 0;
    for ( ; k < Ns1.size(); ++k ) {
      const SMDS_MeshNode* node = static_cast<const SMDS_MeshNode*>( nIt->next() );
      Ns1[k] = node;
      Ps1[k] = gp_Pnt(node->X(), node->Y(), node->Z());
    }
    bool NeedMove = false;
    for(int j=i+1; j<NbPyram; j++) {
      //cout<<"  i="<<i<<" j="<<j<<endl;
      const TColStd_SequenceOfInteger& aMergesI = MergesInfo.Value(i);
      int nbI = aMergesI.Length();
      const TColStd_SequenceOfInteger& aMergesJ = MergesInfo.Value(j);
      int nbJ = aMergesJ.Length();
      // check if two pyramids already merged
      bool NeedCont = false;
      for( k = 2; k<=nbI; k++) {
        if(aMergesI.Value(k)==j) {
          NeedCont = true;
          break;
        }
      }
      if(NeedCont) continue; // already merged

      const SMDS_MeshElement* Prm2 = Pyrams[j];
      nIt = Prm2->nodesIterator();
      vector<gp_Pnt>               Ps2( Prm2->NbNodes() );
      vector<const SMDS_MeshNode*> Ns2( Prm2->NbNodes() );
      for ( k = 0; k < Ns2.size(); ++k ) {
        const SMDS_MeshNode* node = static_cast<const SMDS_MeshNode*>( nIt->next() );
        Ns2[k] = node;
        Ps2[k] = gp_Pnt(node->X(), node->Y(), node->Z());
      }

      bool hasInt = false;
      gp_Pnt Pint;
      for(k=0; k<4; k++) {
        gp_Vec Vtmp(Ps1[k],Ps1[4]);
        gp_Pnt Pshift = Ps1[k].XYZ() + Vtmp.XYZ() * 0.01;
        int m=0;
        for(; m<3; m++) {
          if( HasIntersection3( Pshift, Ps1[4], Pint, Ps2[m], Ps2[m+1], Ps2[4]) ) {
            hasInt = true;
            break;
          }
        }
        if( HasIntersection3( Pshift, Ps1[4], Pint, Ps2[3], Ps2[0], Ps2[4]) ) {
          hasInt = true;
        }
        if(hasInt) break;
      }
      if(!hasInt) {
        for(k=0; k<4; k++) {
          gp_Vec Vtmp(Ps2[k],Ps2[4]);
          gp_Pnt Pshift = Ps2[k].XYZ() + Vtmp.XYZ() * 0.01;
          int m=0;
          for(; m<3; m++) {
            if( HasIntersection3( Pshift, Ps2[4], Pint, Ps1[m], Ps1[m+1], Ps1[4]) ) {
              hasInt = true;
              break;
            }
          }
          if( HasIntersection3( Pshift, Ps2[4], Pint, Ps1[3], Ps1[0], Ps1[4]) ) {
            hasInt = true;
          }
          if(hasInt) break;
        }
      }

      if(hasInt) {
        //cout<<"    has intersec for i="<<i<<" j="<<j<<endl;
        // check if MeshFaces have 2 common node
        int nbc = 0;
        for(k=0; k<4; k++) {
          for(int m=0; m<4; m++) {
            if( Ns1[k]==Ns2[m] ) nbc++;
          }
        }
        //cout<<"      nbc = "<<nbc<<endl;
        if(nbc>0) {
          // create common node
          SMDS_MeshNode* CommonNode = const_cast<SMDS_MeshNode*>(Ns1[4]);
          CommonNode->setXYZ( ( nbI*Ps1[4].X() + nbJ*Ps2[4].X() ) / (nbI+nbJ),
                              ( nbI*Ps1[4].Y() + nbJ*Ps2[4].Y() ) / (nbI+nbJ),
                              ( nbI*Ps1[4].Z() + nbJ*Ps2[4].Z() ) / (nbI+nbJ) );
          NeedMove = true;
          //cout<<"       CommonNode: "<<CommonNode;
          const SMDS_MeshNode* Nrem = Ns2[4];
          Ns2[4] = CommonNode;
          meshDS->ChangeElementNodes(Prm2, &Ns2[0], Ns2.size());
          // update pyramids for J
          for(k=2; k<=nbJ; k++) {
            const SMDS_MeshElement* tmpPrm = Pyrams[aMergesJ.Value(k)];
            SMDS_ElemIteratorPtr tmpIt = tmpPrm->nodesIterator();
            vector< const SMDS_MeshNode* > Ns( tmpPrm->NbNodes() );
            for ( int m = 0; m < Ns.size(); ++m )
              Ns[m] = static_cast<const SMDS_MeshNode*>( tmpIt->next() );
            Ns[4] = CommonNode;
            meshDS->ChangeElementNodes(tmpPrm, &Ns[0], Ns.size());
          }

          // update MergesInfo
          for(k=1; k<=nbI; k++) {
            int num = aMergesI.Value(k);
            TColStd_SequenceOfInteger& aSeq = MergesInfo.ChangeValue(num);
            for(int m=1; m<=nbJ; m++)
              aSeq.Append(aMergesJ.Value(m));
          }
          for(k=1; k<=nbJ; k++) {
            int num = aMergesJ.Value(k);
            TColStd_SequenceOfInteger& aSeq = MergesInfo.ChangeValue(num);
            for(int m=1; m<=nbI; m++)
              aSeq.Append(aMergesI.Value(m));
          }

          // update triangles for aMergesJ
          for(k=1; k<=nbJ; k++) {
            list< list< const SMDS_MeshNode* > > aFNodes;
            list< const SMDS_MeshElement* > aFFaces;
            int num = aMergesJ.Value(k);
            map< const SMDS_MeshElement*,
              list<const SMDS_FaceOfNodes*> >::iterator itrm = myResMap.find(Faces[num]);
            list<const SMDS_FaceOfNodes*>& trias = itrm->second;
            list<const SMDS_FaceOfNodes*>::iterator itt = trias.begin();
            for(; itt!=trias.end(); itt++) {
              SMDS_ElemIteratorPtr nodeIt = (*itt)->nodesIterator();
              const SMDS_MeshNode* NF[3];
              int nn = 0;
              while ( nodeIt->more() )
                NF[nn++] = static_cast<const SMDS_MeshNode*>( nodeIt->next() );
              NF[0] = CommonNode;
              SMDS_FaceOfNodes* Ftria = const_cast< SMDS_FaceOfNodes*>( (*itt) );
              Ftria->ChangeNodes(NF, 3);
            }
          }

          // check and remove coincided faces
          //TColStd_SequenceOfInteger IdRemovedTrias;
          int i1 = 1;
          for(; i1<=nbI; i1++) {
            int numI = aMergesI.Value(i1);
            map< const SMDS_MeshElement*,
              list<const SMDS_FaceOfNodes*> >::iterator itrmI = myResMap.find(Faces[numI]);
            list<const SMDS_FaceOfNodes*>& triasI = (*itrmI).second;
            list<const SMDS_FaceOfNodes*>::iterator ittI = triasI.begin();
            int nbfI = triasI.size();
            vector<const SMDS_FaceOfNodes*> FsI(nbfI);
            k = 0;
            for(; ittI!=triasI.end(); ittI++) {
              FsI[k]  = (*ittI);
              k++;
            }
            int i2 = 0;
            for(; i2<nbfI; i2++) {
              const SMDS_FaceOfNodes* FI = FsI[i2];
              if(FI==0) continue;
              int j1 = 1;
              for(; j1<=nbJ; j1++) {
                int numJ = aMergesJ.Value(j1);
                map< const SMDS_MeshElement*,
                  list<const SMDS_FaceOfNodes*> >::iterator itrmJ = myResMap.find(Faces[numJ]);
                list<const SMDS_FaceOfNodes*>& triasJ = (*itrmJ).second;
                list<const SMDS_FaceOfNodes*>::iterator ittJ = triasJ.begin();
                int nbfJ = triasJ.size();
                vector<const SMDS_FaceOfNodes*> FsJ(nbfJ);
                k = 0;
                for(; ittJ!=triasJ.end(); ittJ++) {
                  FsJ[k]  = (*ittJ);
                  k++;
                }
                int j2 = 0;
                for(; j2<nbfJ; j2++) {
                  const SMDS_FaceOfNodes* FJ = FsJ[j2];
                  // compare triangles
                  if( CompareTrias(FI,FJ) ) {
                    //IdRemovedTrias.Append( FI->GetID() );
                    //IdRemovedTrias.Append( FJ->GetID() );
                    FsI[i2] = 0;
                    FsJ[j2] = 0;
                    list<const SMDS_FaceOfNodes*> new_triasI;
                    for(k=0; k<nbfI; k++) {
                      if( FsI[k]==0 ) continue;
                      new_triasI.push_back( FsI[k] );
                    }
                    (*itrmI).second = new_triasI;
                    triasI = new_triasI;
                    list<const SMDS_FaceOfNodes*> new_triasJ;
                    for(k=0; k<nbfJ; k++) {
                      if( FsJ[k]==0 ) continue;
                      new_triasJ.push_back( FsJ[k] );
                    }
                    (*itrmJ).second = new_triasJ;
                    triasJ = new_triasJ;
                    // remove faces
                    delete FI;
                    delete FJ;
                    // close for j2 and j1
                    j1 = nbJ;
                    break;
                  }
                } // j2
              } // j1
            } // i2
          } // i1
          // removing node
          meshDS->RemoveNode(Nrem);
        }
        else { // nbc==0
          //cout<<"decrease height of pyramids"<<endl;
          // decrease height of pyramids
          double xc1 = 0., yc1 = 0., zc1 = 0.;
          double xc2 = 0., yc2 = 0., zc2 = 0.;
          for(k=0; k<4; k++) {
            xc1 += Ps1[k].X();
            yc1 += Ps1[k].Y();
            zc1 += Ps1[k].Z();
            xc2 += Ps2[k].X();
            yc2 += Ps2[k].Y();
            zc2 += Ps2[k].Z();
          }
          gp_Pnt PC1(xc1/4.,yc1/4.,zc1/4.);
          gp_Pnt PC2(xc2/4.,yc2/4.,zc2/4.);
          gp_Vec VN1(PC1,Ps1[4]);
          gp_Vec VI1(PC1,Pint);
          gp_Vec VN2(PC2,Ps2[4]);
          gp_Vec VI2(PC2,Pint);
          double ang1 = fabs(VN1.Angle(VI1));
          double ang2 = fabs(VN2.Angle(VI2));
          double h1,h2;
          if(ang1>PI/3.)
            h1 = VI1.Magnitude()/2;
          else
            h1 = VI1.Magnitude()*cos(ang1);
          if(ang2>PI/3.)
            h2 = VI2.Magnitude()/2;
          else
            h2 = VI2.Magnitude()*cos(ang2);
          double coef1 = 0.5;
          if(ang1<PI/3)
            coef1 -= cos(ang1)*0.25;
          double coef2 = 0.5;
          if(ang2<PI/3)
            coef2 -= cos(ang1)*0.25;

          SMDS_MeshNode* aNode1 = const_cast<SMDS_MeshNode*>(Ns1[4]);
          VN1.Scale(coef1);
          aNode1->setXYZ( PC1.X()+VN1.X(), PC1.Y()+VN1.Y(), PC1.Z()+VN1.Z() );
          SMDS_MeshNode* aNode2 = const_cast<SMDS_MeshNode*>(Ns2[4]);
          VN2.Scale(coef2);
          aNode2->setXYZ( PC2.X()+VN2.X(), PC2.Y()+VN2.Y(), PC2.Z()+VN2.Z() );
          NeedMove = true;
        }
      } // end if(hasInt)
      else {
        //cout<<"    no intersec for i="<<i<<" j="<<j<<endl;
      }

    }
    if( NeedMove && !meshDS->IsEmbeddedMode() ) {
      meshDS->MoveNode( Ns1[4], Ns1[4]->X(), Ns1[4]->Y(), Ns1[4]->Z() );
    }
  }

  return true;
}


//================================================================================
/*!
 * \brief Return list of created triangles for given face
 */
//================================================================================
const list<const SMDS_FaceOfNodes*>* StdMeshers_QuadToTriaAdaptor::GetTriangles
                                                   (const SMDS_MeshElement* aFace)
{
  map< const SMDS_MeshElement*,
    list<const SMDS_FaceOfNodes*> >::iterator it = myResMap.find(aFace);
  if( it != myResMap.end() ) {
    return & it->second;
  }
  return 0;
}


//================================================================================
/*!
 * \brief Remove all create auxilary faces
 */
//================================================================================
//void StdMeshers_QuadToTriaAdaptor::RemoveFaces(SMESH_Mesh& aMesh)
//{
//  SMESHDS_Mesh * meshDS = aMesh.GetMeshDS();
//  map< const SMDS_MeshElement*,
//    list<const SMDS_MeshElement*> >::iterator it = myResMap.begin();
//  for(; it != myResMap.end(); it++ ) {
//    list<const SMDS_MeshElement*> aFaces = (*it).second;
//    list<const SMDS_MeshElement*>::iterator itf = aFaces.begin();
//    for(; itf!=aFaces.end(); itf++ ) {
//      meshDS->RemoveElement( (*itf) );
//    }
//  }
//}
