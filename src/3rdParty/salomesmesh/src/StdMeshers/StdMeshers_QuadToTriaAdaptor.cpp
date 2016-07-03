// Copyright (C) 2007-2015  CEA/DEN, EDF R&D, OPEN CASCADE
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 2.1 of the License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
//
// See http://www.salome-platform.org/ or email : webmaster.salome@opencascade.com
//

// File      : StdMeshers_QuadToTriaAdaptor.cxx
// Module    : SMESH
// Created   : Wen May 07 16:37:07 2008
// Author    : Sergey KUUL (skl)

#include "StdMeshers_QuadToTriaAdaptor.hxx"

#include "SMDS_SetIterator.hxx"
#include "SMESHDS_GroupBase.hxx"
#include "SMESH_Algo.hxx"
#include "SMESH_Group.hxx"
#include "SMESH_MeshAlgos.hxx"
#include "SMESH_MesherHelper.hxx"

#include <IntAna_IntConicQuad.hxx>
#include <IntAna_Quadric.hxx>
#include <TColgp_HArray1OfPnt.hxx>
#include <TColgp_HArray1OfVec.hxx>
#include <TColgp_HSequenceOfPnt.hxx>
#include <TopExp_Explorer.hxx>
#include <TopoDS.hxx>
#include <gp_Lin.hxx>
#include <gp_Pln.hxx>

#include "utilities.h"

#include <string>
#include <numeric>
#include <limits>

using namespace std;

enum EQuadNature { NOT_QUAD, QUAD, DEGEN_QUAD, PYRAM_APEX = 4, TRIA_APEX = 0 };

// std-like iterator used to get coordinates of nodes of mesh element
typedef SMDS_StdIterator< SMESH_TNodeXYZ, SMDS_ElemIteratorPtr > TXyzIterator;

namespace
{
  //================================================================================
  /*!
   * \brief Return true if two nodes of triangles are equal
   */
  //================================================================================

  bool EqualTriangles(const SMDS_MeshElement* F1,const SMDS_MeshElement* F2)
  {
    return
      ( F1->GetNode(1)==F2->GetNode(2) && F1->GetNode(2)==F2->GetNode(1) ) ||
      ( F1->GetNode(1)==F2->GetNode(1) && F1->GetNode(2)==F2->GetNode(2) );
  }
  //================================================================================
  /*!
   * \brief Return true if two adjacent pyramids are too close one to another
   * so that a tetrahedron to built between them would have too poor quality
   */
  //================================================================================

  bool TooCloseAdjacent( const SMDS_MeshElement* PrmI,
                         const SMDS_MeshElement* PrmJ,
                         const bool              hasShape)
  {
    const SMDS_MeshNode* nApexI = PrmI->GetNode(4);
    const SMDS_MeshNode* nApexJ = PrmJ->GetNode(4);
    if ( nApexI == nApexJ ||
         nApexI->getshapeId() != nApexJ->getshapeId() )
      return false;

    // Find two common base nodes and their indices within PrmI and PrmJ
    const SMDS_MeshNode* baseNodes[2] = { 0,0 };
    int baseNodesIndI[2], baseNodesIndJ[2];
    for ( int i = 0; i < 4 ; ++i )
    {
      int j = PrmJ->GetNodeIndex( PrmI->GetNode(i));
      if ( j >= 0 )
      {
        int ind = baseNodes[0] ? 1:0;
        if ( baseNodes[ ind ])
          return false; // pyramids with a common base face
        baseNodes    [ ind ] = PrmI->GetNode(i);
        baseNodesIndI[ ind ] = i;
        baseNodesIndJ[ ind ] = j;
      }
    }
    if ( !baseNodes[1] ) return false; // not adjacent

    // Get normals of triangles sharing baseNodes
    gp_XYZ apexI = SMESH_TNodeXYZ( nApexI );
    gp_XYZ apexJ = SMESH_TNodeXYZ( nApexJ );
    gp_XYZ base1 = SMESH_TNodeXYZ( baseNodes[0]);
    gp_XYZ base2 = SMESH_TNodeXYZ( baseNodes[1]);
    gp_Vec baseVec( base1, base2 );
    gp_Vec baI( base1, apexI );
    gp_Vec baJ( base1, apexJ );
    gp_Vec nI = baseVec.Crossed( baI );
    gp_Vec nJ = baseVec.Crossed( baJ );

    // Check angle between normals
    double angle = nI.Angle( nJ );
    bool tooClose = ( angle < 15. * M_PI / 180. );

    // Check if pyramids collide
    if ( !tooClose && ( baI * baJ > 0 ) && ( nI * nJ > 0 ))
    {
      // find out if nI points outside of PrmI or inside
      int dInd = baseNodesIndI[1] - baseNodesIndI[0];
      bool isOutI = ( abs(dInd)==1 ) ? dInd < 0 : dInd > 0;

      // find out sign of projection of nJ to baI
      double proj = baI * nJ;

      tooClose = isOutI ? proj > 0 : proj < 0;
    }

    // Check if PrmI and PrmJ are in same domain
    if ( tooClose && !hasShape )
    {
      // check order of baseNodes within pyramids, it must be opposite
      int dInd;
      dInd = baseNodesIndI[1] - baseNodesIndI[0];
      bool isOutI = ( abs(dInd)==1 ) ? dInd < 0 : dInd > 0;
      dInd = baseNodesIndJ[1] - baseNodesIndJ[0];
      bool isOutJ = ( abs(dInd)==1 ) ? dInd < 0 : dInd > 0;
      if ( isOutJ == isOutI )
        return false; // other domain

      // direct both normals outside pyramid
      ( isOutI ? nJ : nI ).Reverse();

      // check absence of a face separating domains between pyramids
      TIDSortedElemSet emptySet, avoidSet;
      int i1, i2;
      while ( const SMDS_MeshElement* f =
              SMESH_MeshAlgos::FindFaceInSet( baseNodes[0], baseNodes[1],
                                              emptySet, avoidSet, &i1, &i2 ))
      {
        avoidSet.insert( f );

        // face node other than baseNodes
        int otherNodeInd = 0;
        while ( otherNodeInd == i1 || otherNodeInd == i2 ) otherNodeInd++;
        const SMDS_MeshNode* otherFaceNode = f->GetNode( otherNodeInd );

        if ( otherFaceNode == nApexI || otherFaceNode == nApexJ )
          continue; // f is a temporary triangle

        // check if f is a base face of either of pyramids
        if ( f->NbCornerNodes() == 4 &&
             ( PrmI->GetNodeIndex( otherFaceNode ) >= 0 ||
               PrmJ->GetNodeIndex( otherFaceNode ) >= 0 ))
          continue; // f is a base quadrangle

        // check projections of face direction (baOFN) to triange normals (nI and nJ)
        gp_Vec baOFN( base1, SMESH_TNodeXYZ( otherFaceNode ));
        if ( nI * baOFN > 0 && nJ * baOFN > 0 )
        {
          tooClose = false; // f is between pyramids
          break;
        }
      }
    }

    return tooClose;
  }

  //================================================================================
  /*!
   * \brief Move medium nodes of merged quadratic pyramids
   */
  //================================================================================

  void UpdateQuadraticPyramids(const set<const SMDS_MeshNode*>& commonApex,
                               SMESHDS_Mesh*                    meshDS)
  {
    typedef SMDS_StdIterator< const SMDS_MeshElement*, SMDS_ElemIteratorPtr > TStdElemIterator;
    TStdElemIterator itEnd;

    // shift of node index to get medium nodes between the 4 base nodes and the apex
    const int base2MediumShift = 9;

    set<const SMDS_MeshNode*>::const_iterator nIt = commonApex.begin();
    for ( ; nIt != commonApex.end(); ++nIt )
    {
      SMESH_TNodeXYZ apex( *nIt );

      vector< const SMDS_MeshElement* > pyrams // pyramids sharing the apex node
        ( TStdElemIterator( apex._node->GetInverseElementIterator( SMDSAbs_Volume )), itEnd );

      // Select medium nodes to keep and medium nodes to remove

      typedef map < const SMDS_MeshNode*, const SMDS_MeshNode*, TIDCompare > TN2NMap;
      TN2NMap base2medium; // to keep
      vector< const SMDS_MeshNode* > nodesToRemove;

      for ( unsigned i = 0; i < pyrams.size(); ++i )
        for ( int baseIndex = 0; baseIndex < PYRAM_APEX; ++baseIndex )
        {
          SMESH_TNodeXYZ         base = pyrams[i]->GetNode( baseIndex );
          const SMDS_MeshNode* medium = pyrams[i]->GetNode( baseIndex + base2MediumShift );
          TN2NMap::iterator b2m = base2medium.insert( make_pair( base._node, medium )).first;
          if ( b2m->second != medium )
          {
            nodesToRemove.push_back( medium );
          }
          else
          {
            // move the kept medium node
            gp_XYZ newXYZ = 0.5 * ( apex + base );
            meshDS->MoveNode( medium, newXYZ.X(), newXYZ.Y(), newXYZ.Z() );
          }
        }

      // Within pyramids, replace nodes to remove by nodes to keep

      for ( unsigned i = 0; i < pyrams.size(); ++i )
      {
        vector< const SMDS_MeshNode* > nodes( pyrams[i]->begin_nodes(),
                                              pyrams[i]->end_nodes() );
        for ( int baseIndex = 0; baseIndex < PYRAM_APEX; ++baseIndex )
        {
          const SMDS_MeshNode* base = pyrams[i]->GetNode( baseIndex );
          nodes[ baseIndex + base2MediumShift ] = base2medium[ base ];
        }
        meshDS->ChangeElementNodes( pyrams[i], &nodes[0], nodes.size());
      }

      // Remove the replaced nodes

      if ( !nodesToRemove.empty() )
      {
        SMESHDS_SubMesh * sm = meshDS->MeshElements( nodesToRemove[0]->getshapeId() );
        for ( unsigned i = 0; i < nodesToRemove.size(); ++i )
          meshDS->RemoveFreeNode( nodesToRemove[i], sm, /*fromGroups=*/false);
      }
    }
  }

}

//================================================================================
/*!
 * \brief Merge the two pyramids (i.e. fuse their apex) and others already merged with them
 */
//================================================================================

void StdMeshers_QuadToTriaAdaptor::MergePiramids( const SMDS_MeshElement*     PrmI,
                                                  const SMDS_MeshElement*     PrmJ,
                                                  set<const SMDS_MeshNode*> & nodesToMove)
{
  const SMDS_MeshNode* Nrem = PrmJ->GetNode(4); // node to remove
  //int nbJ = Nrem->NbInverseElements( SMDSAbs_Volume );
  SMESH_TNodeXYZ Pj( Nrem );

  // an apex node to make common to all merged pyramids
  SMDS_MeshNode* CommonNode = const_cast<SMDS_MeshNode*>(PrmI->GetNode(4));
  if ( CommonNode == Nrem ) return; // already merged
  //int nbI = CommonNode->NbInverseElements( SMDSAbs_Volume );
  SMESH_TNodeXYZ Pi( CommonNode );
  gp_XYZ Pnew = /*( nbI*Pi + nbJ*Pj ) / (nbI+nbJ);*/ 0.5 * ( Pi + Pj );
  CommonNode->setXYZ( Pnew.X(), Pnew.Y(), Pnew.Z() );

  nodesToMove.insert( CommonNode );
  nodesToMove.erase ( Nrem );

  typedef SMDS_StdIterator< const SMDS_MeshElement*, SMDS_ElemIteratorPtr > TStdElemIterator;
  TStdElemIterator itEnd;

  // find and remove coincided faces of merged pyramids
  vector< const SMDS_MeshElement* > inverseElems
    // copy inverse elements to avoid iteration on changing container
    ( TStdElemIterator( CommonNode->GetInverseElementIterator(SMDSAbs_Face)), itEnd);
  for ( unsigned i = 0; i < inverseElems.size(); ++i )
  {
    const SMDS_MeshElement* FI = inverseElems[i];
    const SMDS_MeshElement* FJEqual = 0;
    SMDS_ElemIteratorPtr triItJ = Nrem->GetInverseElementIterator(SMDSAbs_Face);
    while ( !FJEqual && triItJ->more() )
    {
      const SMDS_MeshElement* FJ = triItJ->next();
      if ( EqualTriangles( FJ, FI ))
        FJEqual = FJ;
    }
    if ( FJEqual )
    {
      removeTmpElement( FI );
      removeTmpElement( FJEqual );
      myRemovedTrias.insert( FI );
      myRemovedTrias.insert( FJEqual );
    }
  }

  // set the common apex node to pyramids and triangles merged with J
  inverseElems.assign( TStdElemIterator( Nrem->GetInverseElementIterator()), itEnd );
  for ( unsigned i = 0; i < inverseElems.size(); ++i )
  {
    const SMDS_MeshElement* elem = inverseElems[i];
    vector< const SMDS_MeshNode* > nodes( elem->begin_nodes(), elem->end_nodes() );
    nodes[ elem->GetType() == SMDSAbs_Volume ? PYRAM_APEX : TRIA_APEX ] = CommonNode;
    GetMeshDS()->ChangeElementNodes( elem, &nodes[0], nodes.size());
  }
  ASSERT( Nrem->NbInverseElements() == 0 );
  GetMeshDS()->RemoveFreeNode( Nrem,
                               GetMeshDS()->MeshElements( Nrem->getshapeId()),
                               /*fromGroups=*/false);
}

//================================================================================
/*!
 * \brief Merges adjacent pyramids
 */
//================================================================================

void StdMeshers_QuadToTriaAdaptor::MergeAdjacent(const SMDS_MeshElement*    PrmI,
                                                 set<const SMDS_MeshNode*>& nodesToMove)
{
  TIDSortedElemSet adjacentPyrams;
  bool mergedPyrams = false;
  for(int k=0; k<4; k++) // loop on 4 base nodes of PrmI
  {
    const SMDS_MeshNode* n = PrmI->GetNode(k);
    SMDS_ElemIteratorPtr vIt = n->GetInverseElementIterator( SMDSAbs_Volume );
    while ( vIt->more() )
    {
      const SMDS_MeshElement* PrmJ = vIt->next();
      if ( PrmJ->NbCornerNodes() != 5 || !adjacentPyrams.insert( PrmJ ).second  )
        continue;
      if ( PrmI != PrmJ && TooCloseAdjacent( PrmI, PrmJ, GetMesh()->HasShapeToMesh() ))
      {
        MergePiramids( PrmI, PrmJ, nodesToMove );
        mergedPyrams = true;
        // container of inverse elements can change
        vIt = n->GetInverseElementIterator( SMDSAbs_Volume );
      }
    }
  }
  if ( mergedPyrams )
  {
    TIDSortedElemSet::iterator prm;
    for (prm = adjacentPyrams.begin(); prm != adjacentPyrams.end(); ++prm)
      MergeAdjacent( *prm, nodesToMove );
  }
}

//================================================================================
/*!
 * \brief Constructor
 */
//================================================================================

StdMeshers_QuadToTriaAdaptor::StdMeshers_QuadToTriaAdaptor():
  myElemSearcher(0)
{
}

//================================================================================
/*!
 * \brief Destructor
 */
//================================================================================

StdMeshers_QuadToTriaAdaptor::~StdMeshers_QuadToTriaAdaptor()
{
  // temporary faces are deleted by ~SMESH_ProxyMesh()
  if ( myElemSearcher ) delete myElemSearcher;
  myElemSearcher=0;
}

//=======================================================================
//function : FindBestPoint
//purpose  : Return a point P laying on the line (PC,V) so that triangle
//           (P, P1, P2) to be equilateral as much as possible
//           V - normal to (P1,P2,PC)
//=======================================================================

static gp_Pnt FindBestPoint(const gp_Pnt& P1, const gp_Pnt& P2,
                            const gp_Pnt& PC, const gp_Vec& V)
{
  gp_Pnt Pbest = PC;
  const double a = P1.Distance(P2);
  const double b = P1.Distance(PC);
  const double c = P2.Distance(PC);
  if( a < (b+c)/2 )
    return Pbest;
  else {
    // find shift along V in order a to became equal to (b+c)/2
    const double Vsize = V.Magnitude();
    if ( fabs( Vsize ) > std::numeric_limits<double>::min() )
    {
      const double shift = sqrt( a*a + (b*b-c*c)*(b*b-c*c)/16/a/a - (b*b+c*c)/2 );
      Pbest.ChangeCoord() += shift * V.XYZ() / Vsize;
    }
  }
  return Pbest;
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
      const double preci = 1.e-10 * P.Distance(PC);
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
      if( V1.Magnitude()<preci ||
          V2.Magnitude()<preci ||
          V3.Magnitude()<preci ) {
        Pint = PIn;
        return true;
      }
      const double angularTol = 1e-6;
      gp_Vec VC1 = V1.Crossed(V2);
      gp_Vec VC2 = V2.Crossed(V3);
      gp_Vec VC3 = V3.Crossed(V1);
      if(VC1.Magnitude()<gp::Resolution()) {
        if(VC2.IsOpposite(VC3,angularTol)) {
          return false;
        }
      }
      else if(VC2.Magnitude()<gp::Resolution()) {
        if(VC1.IsOpposite(VC3,angularTol)) {
          return false;
        }
      }
      else if(VC3.Magnitude()<gp::Resolution()) {
        if(VC1.IsOpposite(VC2,angularTol)) {
          return false;
        }
      }
      else {
        if( VC1.IsOpposite(VC2,angularTol) || VC1.IsOpposite(VC3,angularTol) ||
            VC2.IsOpposite(VC3,angularTol) ) {
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

//================================================================================
/*!
 * \brief Checks if a line segment (P,PC) intersects any mesh face.
 *  \param P - first segment end
 *  \param PC - second segment end (it is a gravity center of quadrangle)
 *  \param Pint - (out) intersection point
 *  \param aMesh - mesh
 *  \param aShape - shape to check faces on
 *  \param NotCheckedFace - mesh face not to check
 *  \retval bool - true if there is an intersection
 */
//================================================================================

bool StdMeshers_QuadToTriaAdaptor::CheckIntersection (const gp_Pnt&       P,
                                                      const gp_Pnt&       PC,
                                                      gp_Pnt&             Pint,
                                                      SMESH_Mesh&         aMesh,
                                                      const TopoDS_Shape& aShape,
                                                      const SMDS_MeshElement* NotCheckedFace)
{
  if ( !myElemSearcher )
    myElemSearcher = SMESH_MeshAlgos::GetElementSearcher( *aMesh.GetMeshDS() );
  SMESH_ElementSearcher* searcher = const_cast<SMESH_ElementSearcher*>(myElemSearcher);

  //SMESHDS_Mesh * meshDS = aMesh.GetMeshDS();
  //cout<<"    CheckIntersection: meshDS->NbFaces() = "<<meshDS->NbFaces()<<endl;
  bool res = false;
  double dist = RealLast(); // find intersection closest to the segment
  gp_Pnt Pres;

  gp_Ax1 line( P, gp_Vec(P,PC));
  vector< const SMDS_MeshElement* > suspectElems;
  searcher->GetElementsNearLine( line, SMDSAbs_Face, suspectElems);

  for ( int i = 0; i < suspectElems.size(); ++i )
  {
    const SMDS_MeshElement* face = suspectElems[i];
    if ( face == NotCheckedFace ) continue;
    Handle(TColgp_HSequenceOfPnt) aContour = new TColgp_HSequenceOfPnt;
    for ( int i = 0; i < face->NbCornerNodes(); ++i )
      aContour->Append( SMESH_TNodeXYZ( face->GetNode(i) ));
    if( HasIntersection(P, PC, Pres, aContour) ) {
      res = true;
      double tmp = PC.Distance(Pres);
      if(tmp<dist) {
        Pint = Pres;
        dist = tmp;
      }
    }
  }
  return res;
}

//================================================================================
/*!
 * \brief Prepare data for the given face
 *  \param PN - coordinates of face nodes
 *  \param VN - cross products of vectors (PC-PN(i)) ^ (PC-PN(i+1))
 *  \param FNodes - face nodes
 *  \param PC - gravity center of nodes
 *  \param VNorm - face normal (sum of VN)
 *  \param volumes - two volumes sharing the given face, the first is in VNorm direction
 *  \retval int - 0 if given face is not quad,
 *                1 if given face is quad,
 *                2 if given face is degenerate quad (two nodes are coincided)
 */
//================================================================================

int StdMeshers_QuadToTriaAdaptor::Preparation(const SMDS_MeshElement*       face,
                                              Handle(TColgp_HArray1OfPnt)&  PN,
                                              Handle(TColgp_HArray1OfVec)&  VN,
                                              vector<const SMDS_MeshNode*>& FNodes,
                                              gp_Pnt&                       PC,
                                              gp_Vec&                       VNorm,
                                              const SMDS_MeshElement**      volumes)
{
  if( face->NbCornerNodes() != 4 )
  {
    return NOT_QUAD;
  }

  int i = 0;
  gp_XYZ xyzC(0., 0., 0.);
  for ( i = 0; i < 4; ++i )
  {
    gp_XYZ p = SMESH_TNodeXYZ( FNodes[i] = face->GetNode(i) );
    PN->SetValue( i+1, p );
    xyzC += p;
  }
  PC = xyzC/4;

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

    list< const SMDS_MeshNode* >::iterator itdg = myDegNodes.begin();
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
  }

  PN->SetValue(nbp+1,PN->Value(1));
  FNodes[nbp] = FNodes[0];
  // find normal direction
  gp_Vec V1(PC,PN->Value(nbp));
  gp_Vec V2(PC,PN->Value(1));
  VNorm = V1.Crossed(V2);
  VN->SetValue(nbp,VNorm);
  for(i=1; i<nbp; i++) {
    V1 = gp_Vec(PC,PN->Value(i));
    V2 = gp_Vec(PC,PN->Value(i+1));
    gp_Vec Vtmp = V1.Crossed(V2);
    VN->SetValue(i,Vtmp);
    VNorm += Vtmp;
  }

  // find volumes sharing the face
  if ( volumes )
  {
    volumes[0] = volumes[1] = 0;
    SMDS_ElemIteratorPtr vIt = FNodes[0]->GetInverseElementIterator( SMDSAbs_Volume );
    while ( vIt->more() )
    {
      const SMDS_MeshElement* vol = vIt->next();
      bool volSharesAllNodes = true;
      for ( int i = 1; i < face->NbNodes() && volSharesAllNodes; ++i )
        volSharesAllNodes = ( vol->GetNodeIndex( FNodes[i] ) >= 0 );
      if ( volSharesAllNodes )
        volumes[ volumes[0] ? 1 : 0 ] = vol;
      // we could additionally check that vol has all FNodes in its one face using SMDS_VolumeTool
    }
    // define volume position relating to the face normal
    if ( volumes[0] )
    {
      // get volume gc
      SMDS_ElemIteratorPtr nodeIt = volumes[0]->nodesIterator();
      gp_XYZ volGC(0,0,0);
      volGC = accumulate( TXyzIterator(nodeIt), TXyzIterator(), volGC ) / volumes[0]->NbNodes();

      if ( VNorm * gp_Vec( PC, volGC ) < 0 )
        swap( volumes[0], volumes[1] );
    }
  }

  //cout<<"  VNorm("<<VNorm.X()<<","<<VNorm.Y()<<","<<VNorm.Z()<<")"<<endl;
  return hasdeg ? DEGEN_QUAD : QUAD;
}


//=======================================================================
//function : Compute
//purpose  :
//=======================================================================

bool StdMeshers_QuadToTriaAdaptor::Compute(SMESH_Mesh&         aMesh,
                                           const TopoDS_Shape& aShape,
                                           SMESH_ProxyMesh*    aProxyMesh)
{
  SMESH_ProxyMesh::setMesh( aMesh );

  if ( aShape.ShapeType() != TopAbs_SOLID &&
       aShape.ShapeType() != TopAbs_SHELL )
    return false;

  myShape = aShape;

  vector<const SMDS_MeshElement*> myPyramids;

  SMESHDS_Mesh * meshDS = aMesh.GetMeshDS();
  SMESH_MesherHelper helper(aMesh);
  helper.IsQuadraticSubMesh(aShape);
  helper.SetElementsOnShape( true );

  if ( myElemSearcher ) delete myElemSearcher;
  if ( aProxyMesh )
    myElemSearcher = SMESH_MeshAlgos::GetElementSearcher( *meshDS, aProxyMesh->GetFaces(aShape));
  else
    myElemSearcher = SMESH_MeshAlgos::GetElementSearcher( *meshDS );

  const SMESHDS_SubMesh * aSubMeshDSFace;
  Handle(TColgp_HArray1OfPnt) PN = new TColgp_HArray1OfPnt(1,5);
  Handle(TColgp_HArray1OfVec) VN = new TColgp_HArray1OfVec(1,4);
  vector<const SMDS_MeshNode*> FNodes(5);
  gp_Pnt PC;
  gp_Vec VNorm;

  for (TopExp_Explorer exp(aShape,TopAbs_FACE);exp.More();exp.Next())
  {
    const TopoDS_Shape& aShapeFace = exp.Current();
    if ( aProxyMesh )
      aSubMeshDSFace = aProxyMesh->GetSubMesh( aShapeFace );
    else
      aSubMeshDSFace = meshDS->MeshElements( aShapeFace );

    vector<const SMDS_MeshElement*> trias, quads;
    bool hasNewTrias = false;

    if ( aSubMeshDSFace )
    {
      bool isRev = false;
      if ( helper.NbAncestors( aShapeFace, aMesh, aShape.ShapeType() ) > 1 )
        isRev = helper.IsReversedSubMesh( TopoDS::Face(aShapeFace) );

      SMDS_ElemIteratorPtr iteratorElem = aSubMeshDSFace->GetElements();
      while ( iteratorElem->more() ) // loop on elements on a geometrical face
      {
        const SMDS_MeshElement* face = iteratorElem->next();
        // preparation step to get face info
        int stat = Preparation(face, PN, VN, FNodes, PC, VNorm);
        switch ( stat )
        {
        case NOT_QUAD:

          trias.push_back( face );
          break;

        case DEGEN_QUAD:
          {
            // degenerate face
            // add triangles to result map
            SMDS_MeshFace* NewFace;
            if(!isRev)
              NewFace = meshDS->AddFace( FNodes[0], FNodes[1], FNodes[2] );
            else
              NewFace = meshDS->AddFace( FNodes[0], FNodes[2], FNodes[1] );
            storeTmpElement( NewFace );
            trias.push_back ( NewFace );
            quads.push_back( face );
            hasNewTrias = true;
            break;
          }

        case QUAD:
          {
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
              bool check = CheckIntersection(PCbest, PC, Pint, aMesh, aShape, face);
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
                check = CheckIntersection(PCbestTmp, PC, Pint, aMesh, aShape, face);
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
            for(i=0; i<4; i++)
            {
              trias.push_back ( meshDS->AddFace( NewNode, FNodes[i], FNodes[i+1] ));
              storeTmpElement( trias.back() );
            }
            // create a pyramid
            if ( isRev ) swap( FNodes[1], FNodes[3]);
            SMDS_MeshVolume* aPyram =
              helper.AddVolume( FNodes[0], FNodes[1], FNodes[2], FNodes[3], NewNode );
            myPyramids.push_back(aPyram);

            quads.push_back( face );
            hasNewTrias = true;
            break;

          } // case QUAD:

        } // switch ( stat )
      } // end loop on elements on a face submesh

      bool sourceSubMeshIsProxy = false;
      if ( aProxyMesh )
      {
        // move proxy sub-mesh from other proxy mesh to this
        sourceSubMeshIsProxy = takeProxySubMesh( aShapeFace, aProxyMesh );
        // move also tmp elements added in mesh
        takeTmpElemsInMesh( aProxyMesh );
      }
      if ( hasNewTrias )
      {
        SMESH_ProxyMesh::SubMesh* prxSubMesh = getProxySubMesh( aShapeFace );
        prxSubMesh->ChangeElements( trias.begin(), trias.end() );

        // delete tmp quadrangles removed from aProxyMesh
        if ( sourceSubMeshIsProxy )
        {
          for ( unsigned i = 0; i < quads.size(); ++i )
            removeTmpElement( quads[i] );

          delete myElemSearcher;
          myElemSearcher =
            SMESH_MeshAlgos::GetElementSearcher( *meshDS, aProxyMesh->GetFaces(aShape));
        }
      }
    }
  } // end for(TopExp_Explorer exp(aShape,TopAbs_FACE);exp.More();exp.Next()) {

  return Compute2ndPart(aMesh, myPyramids);
}

//================================================================================
/*!
 * \brief Computes pyramids in mesh with no shape
 */
//================================================================================

bool StdMeshers_QuadToTriaAdaptor::Compute(SMESH_Mesh& aMesh)
{
  SMESH_ProxyMesh::setMesh( aMesh );
  SMESH_ProxyMesh::_allowedTypes.push_back( SMDSEntity_Triangle );
  SMESH_ProxyMesh::_allowedTypes.push_back( SMDSEntity_Quad_Triangle );
  if ( aMesh.NbQuadrangles() < 1 )
    return false;

  // find if there is a group of faces identified as skin faces, with normal going outside the volume
  std::string groupName = "skinFaces";
  SMESHDS_GroupBase* groupDS = 0;
  SMESH_Mesh::GroupIteratorPtr groupIt = aMesh.GetGroups();
  while ( groupIt->more() )
    {
      groupDS = 0;
      SMESH_Group * group = groupIt->next();
      if ( !group ) continue;
      groupDS = group->GetGroupDS();
      if ( !groupDS || groupDS->IsEmpty() )
      {
        groupDS = 0;
        continue;
      }
      if (groupDS->GetType() != SMDSAbs_Face)
      {
        groupDS = 0;
        continue;
      }
      std::string grpName = group->GetName();
      if (grpName == groupName)
      {
        MESSAGE("group skinFaces provided");
        break;
      }
      else
        groupDS = 0;
    }

  vector<const SMDS_MeshElement*> myPyramids;
  SMESH_MesherHelper helper(aMesh);
  helper.IsQuadraticSubMesh(aMesh.GetShapeToMesh());
  helper.SetElementsOnShape( true );

  SMESHDS_Mesh * meshDS = aMesh.GetMeshDS();
  SMESH_ProxyMesh::SubMesh* prxSubMesh = getProxySubMesh();

  if ( !myElemSearcher )
    myElemSearcher = SMESH_MeshAlgos::GetElementSearcher( *meshDS );
  SMESH_ElementSearcher* searcher = const_cast<SMESH_ElementSearcher*>(myElemSearcher);

  SMDS_FaceIteratorPtr fIt = meshDS->facesIterator(/*idInceasingOrder=*/true);
  while( fIt->more())
  {
    const SMDS_MeshElement* face = fIt->next();
    if ( !face ) continue;
    // retrieve needed information about a face
    Handle(TColgp_HArray1OfPnt) PN = new TColgp_HArray1OfPnt(1,5);
    Handle(TColgp_HArray1OfVec) VN = new TColgp_HArray1OfVec(1,4);
    vector<const SMDS_MeshNode*> FNodes(5);
    gp_Pnt PC;
    gp_Vec VNorm;
    const SMDS_MeshElement* volumes[2];
    int what = Preparation(face, PN, VN, FNodes, PC, VNorm, volumes);
    if ( what == NOT_QUAD )
      continue;
    if ( volumes[0] && volumes[1] )
      continue; // face is shared by two volumes - no space for a pyramid

    if ( what == DEGEN_QUAD )
    {
      // degenerate face
      // add a triangle to the proxy mesh
      SMDS_MeshFace* NewFace;

      // check orientation
      double tmp = PN->Value(1).Distance(PN->Value(2)) + PN->Value(2).Distance(PN->Value(3));
      // far points in VNorm direction
      gp_Pnt Ptmp1 = PC.XYZ() + VNorm.XYZ() * tmp * 1.e6;
      gp_Pnt Ptmp2 = PC.XYZ() - VNorm.XYZ() * tmp * 1.e6;
      // check intersection for Ptmp1 and Ptmp2
      bool IsRev = false;
      bool IsOK1 = false;
      bool IsOK2 = false;
      double dist1 = RealLast();
      double dist2 = RealLast();
      gp_Pnt Pres1,Pres2;

      gp_Ax1 line( PC, VNorm );
      vector< const SMDS_MeshElement* > suspectElems;
      searcher->GetElementsNearLine( line, SMDSAbs_Face, suspectElems);

      for ( int iF = 0; iF < suspectElems.size(); ++iF ) {
        const SMDS_MeshElement* F = suspectElems[iF];
        if(F==face) continue;
        Handle(TColgp_HSequenceOfPnt) aContour = new TColgp_HSequenceOfPnt;
        for ( int i = 0; i < 4; ++i )
          aContour->Append( SMESH_TNodeXYZ( F->GetNode(i) ));
        gp_Pnt PPP;
        if( !volumes[0] && HasIntersection(Ptmp1, PC, PPP, aContour) ) {
          IsOK1 = true;
          double tmp = PC.Distance(PPP);
          if(tmp<dist1) {
            Pres1 = PPP;
            dist1 = tmp;
          }
        }
        if( !volumes[1] && HasIntersection(Ptmp2, PC, PPP, aContour) ) {
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
        double tmp1 = PC.Distance(Pres1);
        double tmp2 = PC.Distance(Pres2);
        if(tmp1<tmp2) {
          // using existed direction
        }
        else {
          // using opposite direction
          IsRev = true;
        }
      }
      if(!IsRev)
        NewFace = meshDS->AddFace( FNodes[0], FNodes[1], FNodes[2] );
      else
        NewFace = meshDS->AddFace( FNodes[0], FNodes[2], FNodes[1] );
      storeTmpElement( NewFace );
      prxSubMesh->AddElement( NewFace );
      continue;
    }

    // Case of non-degenerated quadrangle

    // Find pyramid peak

    gp_XYZ PCbest(0., 0., 0.); // pyramid peak
    int i = 1;
    for(; i<=4; i++) {
      gp_Pnt Pbest = FindBestPoint(PN->Value(i), PN->Value(i+1), PC, VN->Value(i));
      PCbest += Pbest.XYZ();
    }
    PCbest /= 4;

    double height = PC.Distance(PCbest); // pyramid height to precise
    if ( height < 1.e-6 ) {
      // create new PCbest using a bit shift along VNorm
      PCbest = PC.XYZ() + VNorm.XYZ() * 0.001;
      height = PC.Distance(PCbest);
      if ( height < std::numeric_limits<double>::min() )
        return false; // batterfly element
    }

    // Restrict pyramid height by intersection with other faces
    gp_Vec tmpDir(PC,PCbest); tmpDir.Normalize();
    double tmp = PN->Value(1).Distance(PN->Value(3)) + PN->Value(2).Distance(PN->Value(4));
    // far points: in (PC, PCbest) direction and vice-versa
    gp_Pnt farPnt[2] = { PC.XYZ() + tmpDir.XYZ() * tmp * 1.e6,
                         PC.XYZ() - tmpDir.XYZ() * tmp * 1.e6 };
    // check intersection for farPnt1 and farPnt2
    bool   intersected[2] = { false, false };
    double dist       [2] = { RealLast(), RealLast() };
    gp_Pnt intPnt[2];

    gp_Ax1 line( PC, tmpDir );
    vector< const SMDS_MeshElement* > suspectElems;
    searcher->GetElementsNearLine( line, SMDSAbs_Face, suspectElems);

    for ( int iF = 0; iF < suspectElems.size(); ++iF )
    {
      const SMDS_MeshElement* F = suspectElems[iF];
      if(F==face) continue;
      Handle(TColgp_HSequenceOfPnt) aContour = new TColgp_HSequenceOfPnt;
      int nbN = F->NbNodes() / ( F->IsQuadratic() ? 2 : 1 );
      for ( i = 0; i < nbN; ++i )
        aContour->Append( SMESH_TNodeXYZ( F->GetNode(i) ));
      gp_Pnt intP;
      for ( int isRev = 0; isRev < 2; ++isRev )
      {
        if( !volumes[isRev] && HasIntersection(farPnt[isRev], PC, intP, aContour) ) {
          intersected[isRev] = true;
          double d = PC.Distance( intP );
          if( d < dist[isRev] )
          {
            intPnt[isRev] = intP;
            dist  [isRev] = d;
          }
        }
      }
    }

    // if the face belong to the group of skinFaces, do not build a pyramid outside
    if (groupDS && groupDS->Contains(face))
    {
      intersected[0] = false;
    }
    else if ( intersected[0] && intersected[1] ) // check if one of pyramids is in a hole
    {
      gp_Pnt P ( PC.XYZ() + tmpDir.XYZ() * 0.5 * PC.Distance( intPnt[0] ));
      if ( searcher->GetPointState( P ) == TopAbs_OUT )
        intersected[0] = false;
      else
      {
        P = ( PC.XYZ() - tmpDir.XYZ() * 0.5 * PC.Distance( intPnt[1] ));
        if ( searcher->GetPointState( P ) == TopAbs_OUT )
          intersected[1] = false;
      }
    }

    // Create one or two pyramids

    for ( int isRev = 0; isRev < 2; ++isRev )
    {
      if( !intersected[isRev] ) continue;
      double pyramidH = Min( height, PC.Distance(intPnt[isRev])/3.);
      PCbest = PC.XYZ() + tmpDir.XYZ() * (isRev ? -pyramidH : pyramidH);

      // create node for PCbest
      SMDS_MeshNode* NewNode = helper.AddNode( PCbest.X(), PCbest.Y(), PCbest.Z() );

      // add triangles to result map
      for(i=0; i<4; i++) {
        SMDS_MeshFace* NewFace;
        if(isRev)
          NewFace = meshDS->AddFace( NewNode, FNodes[i], FNodes[i+1] );
        else
          NewFace = meshDS->AddFace( NewNode, FNodes[i+1], FNodes[i] );
        storeTmpElement( NewFace );
        prxSubMesh->AddElement( NewFace );
      }
      // create a pyramid
      SMDS_MeshVolume* aPyram;
      if(isRev)
        aPyram = helper.AddVolume( FNodes[0], FNodes[1], FNodes[2], FNodes[3], NewNode );
      else
        aPyram = helper.AddVolume( FNodes[0], FNodes[3], FNodes[2], FNodes[1], NewNode );
      myPyramids.push_back(aPyram);
    }
  } // end loop on all faces

  return Compute2ndPart(aMesh, myPyramids);
}

//================================================================================
/*!
 * \brief Update created pyramids and faces to avoid their intersection
 */
//================================================================================

bool StdMeshers_QuadToTriaAdaptor::Compute2ndPart(SMESH_Mesh&                            aMesh,
                                                  const vector<const SMDS_MeshElement*>& myPyramids)
{
  if(myPyramids.empty())
    return true;

  SMESHDS_Mesh * meshDS = aMesh.GetMeshDS();
  int i, j, k, myShapeID = myPyramids[0]->GetNode(4)->getshapeId();

  if ( myElemSearcher ) delete myElemSearcher;
  myElemSearcher = SMESH_MeshAlgos::GetElementSearcher( *meshDS );
  SMESH_ElementSearcher* searcher = const_cast<SMESH_ElementSearcher*>(myElemSearcher);

  set<const SMDS_MeshNode*> nodesToMove;

  // check adjacent pyramids

  for ( i = 0; i <  myPyramids.size(); ++i )
  {
    const SMDS_MeshElement* PrmI = myPyramids[i];
    MergeAdjacent( PrmI, nodesToMove );
  }

  // iterate on all pyramids
  for ( i = 0; i <  myPyramids.size(); ++i )
  {
    const SMDS_MeshElement* PrmI = myPyramids[i];

    // compare PrmI with all the rest pyramids

    // collect adjacent pyramids and nodes coordinates of PrmI
    set<const SMDS_MeshElement*> checkedPyrams;
    vector<gp_Pnt> PsI(5);
    for(k=0; k<5; k++) // loop on 4 base nodes of PrmI
    {
      const SMDS_MeshNode* n = PrmI->GetNode(k);
      PsI[k] = SMESH_TNodeXYZ( n );
      SMDS_ElemIteratorPtr vIt = n->GetInverseElementIterator( SMDSAbs_Volume );
      while ( vIt->more() )
      {
        const SMDS_MeshElement* PrmJ = vIt->next();
        if ( SMESH_MeshAlgos::GetCommonNodes( PrmI, PrmJ ).size() > 1 )
          checkedPyrams.insert( PrmJ );
      }
    }

    // check intersection with distant pyramids
    for(k=0; k<4; k++) // loop on 4 base nodes of PrmI
    {
      gp_Vec Vtmp(PsI[k],PsI[4]);
      gp_Ax1 line( PsI[k], Vtmp );
      vector< const SMDS_MeshElement* > suspectPyrams;
      searcher->GetElementsNearLine( line, SMDSAbs_Volume, suspectPyrams);

      for ( j = 0; j < suspectPyrams.size(); ++j )
      {
        const SMDS_MeshElement* PrmJ = suspectPyrams[j];
        if ( PrmJ == PrmI || PrmJ->NbCornerNodes() != 5 )
          continue;
        if ( myShapeID != PrmJ->GetNode(4)->getshapeId())
          continue; // pyramid from other SOLID
        if ( PrmI->GetNode(4) == PrmJ->GetNode(4) )
          continue; // pyramids PrmI and PrmJ already merged
        if ( !checkedPyrams.insert( PrmJ ).second )
          continue; // already checked

        TXyzIterator xyzIt( PrmJ->nodesIterator() );
        vector<gp_Pnt> PsJ( xyzIt, TXyzIterator() );

        gp_Pnt Pint;
        bool hasInt=false;
        for(k=0; k<4 && !hasInt; k++) {
          gp_Vec Vtmp(PsI[k],PsI[4]);
          gp_Pnt Pshift = PsI[k].XYZ() + Vtmp.XYZ() * 0.01; // base node moved a bit to apex
          hasInt =
          ( HasIntersection3( Pshift, PsI[4], Pint, PsJ[0], PsJ[1], PsJ[4]) ||
            HasIntersection3( Pshift, PsI[4], Pint, PsJ[1], PsJ[2], PsJ[4]) ||
            HasIntersection3( Pshift, PsI[4], Pint, PsJ[2], PsJ[3], PsJ[4]) ||
            HasIntersection3( Pshift, PsI[4], Pint, PsJ[3], PsJ[0], PsJ[4]) );
        }
        for(k=0; k<4 && !hasInt; k++) {
          gp_Vec Vtmp(PsJ[k],PsJ[4]);
          gp_Pnt Pshift = PsJ[k].XYZ() + Vtmp.XYZ() * 0.01;
          hasInt =
            ( HasIntersection3( Pshift, PsJ[4], Pint, PsI[0], PsI[1], PsI[4]) ||
              HasIntersection3( Pshift, PsJ[4], Pint, PsI[1], PsI[2], PsI[4]) ||
              HasIntersection3( Pshift, PsJ[4], Pint, PsI[2], PsI[3], PsI[4]) ||
              HasIntersection3( Pshift, PsJ[4], Pint, PsI[3], PsI[0], PsI[4]) );
        }

        if ( hasInt )
        {
          // count common nodes of base faces of two pyramids
          int nbc = 0;
          for (k=0; k<4; k++)
            nbc += int ( PrmI->GetNodeIndex( PrmJ->GetNode(k) ) >= 0 );

          if ( nbc == 4 )
            continue; // pyrams have a common base face

          if(nbc>0)
          {
            // Merge the two pyramids and others already merged with them
            MergePiramids( PrmI, PrmJ, nodesToMove );
          }
          else { // nbc==0

            // decrease height of pyramids
            gp_XYZ PCi(0,0,0), PCj(0,0,0);
            for(k=0; k<4; k++) {
              PCi += PsI[k].XYZ();
              PCj += PsJ[k].XYZ();
            }
            PCi /= 4; PCj /= 4;
            gp_Vec VN1(PCi,PsI[4]);
            gp_Vec VN2(PCj,PsJ[4]);
            gp_Vec VI1(PCi,Pint);
            gp_Vec VI2(PCj,Pint);
            double ang1 = fabs(VN1.Angle(VI1));
            double ang2 = fabs(VN2.Angle(VI2));
            double coef1 = 0.5 - (( ang1 < M_PI/3. ) ? cos(ang1)*0.25 : 0 );
            double coef2 = 0.5 - (( ang2 < M_PI/3. ) ? cos(ang2)*0.25 : 0 ); // cos(ang2) ?
//             double coef2 = 0.5;
//             if(ang2<PI/3)
//               coef2 -= cos(ang1)*0.25;

            VN1.Scale(coef1);
            VN2.Scale(coef2);
            SMDS_MeshNode* aNode1 = const_cast<SMDS_MeshNode*>(PrmI->GetNode(4));
            aNode1->setXYZ( PCi.X()+VN1.X(), PCi.Y()+VN1.Y(), PCi.Z()+VN1.Z() );
            SMDS_MeshNode* aNode2 = const_cast<SMDS_MeshNode*>(PrmJ->GetNode(4));
            aNode2->setXYZ( PCj.X()+VN2.X(), PCj.Y()+VN2.Y(), PCj.Z()+VN2.Z() );
            nodesToMove.insert( aNode1 );
            nodesToMove.insert( aNode2 );
          }
          // fix intersections that can appear after apex movement
          MergeAdjacent( PrmI, nodesToMove );
          MergeAdjacent( PrmJ, nodesToMove );

        } // end if(hasInt)
      } // loop on suspectPyrams
    }  // loop on 4 base nodes of PrmI

  } // loop on all pyramids

  if( !nodesToMove.empty() && !meshDS->IsEmbeddedMode() )
  {
    set<const SMDS_MeshNode*>::iterator n = nodesToMove.begin();
    for ( ; n != nodesToMove.end(); ++n )
      meshDS->MoveNode( *n, (*n)->X(), (*n)->Y(), (*n)->Z() );
  }

  // move medium nodes of merged quadratic pyramids
  if ( myPyramids[0]->IsQuadratic() )
    UpdateQuadraticPyramids( nodesToMove, GetMeshDS() );

  // erase removed triangles from the proxy mesh
  if ( !myRemovedTrias.empty() )
  {
    for ( int i = 0; i <= meshDS->MaxShapeIndex(); ++i )
      if ( SMESH_ProxyMesh::SubMesh* sm = findProxySubMesh(i))
      {
        vector<const SMDS_MeshElement *> faces;
        faces.reserve( sm->NbElements() );
        SMDS_ElemIteratorPtr fIt = sm->GetElements();
        while ( fIt->more() )
        {
          const SMDS_MeshElement* tria = fIt->next();
          set<const SMDS_MeshElement*>::iterator rmTria = myRemovedTrias.find( tria );
          if ( rmTria != myRemovedTrias.end() )
            myRemovedTrias.erase( rmTria );
          else
            faces.push_back( tria );
        }
        sm->ChangeElements( faces.begin(), faces.end() );
      }
  }

  myDegNodes.clear();

  delete myElemSearcher;
  myElemSearcher=0;

  return true;
}
