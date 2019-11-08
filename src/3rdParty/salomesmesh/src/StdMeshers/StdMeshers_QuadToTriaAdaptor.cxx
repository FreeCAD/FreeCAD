// Copyright (C) 2007-2016  CEA/DEN, EDF R&D, OPEN CASCADE
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

#include "SMDS_IteratorOnIterators.hxx"
#include "SMDS_SetIterator.hxx"
#include "SMESHDS_GroupBase.hxx"
#include "SMESHDS_Mesh.hxx"
#include "SMESH_Algo.hxx"
#include "SMESH_Group.hxx"
#include "SMESH_Mesh.hxx"
#include "SMESH_MeshAlgos.hxx"
#include "SMESH_MesherHelper.hxx"
#include "SMESH_subMesh.hxx"

#include <IntAna_IntConicQuad.hxx>
#include <IntAna_Quadric.hxx>
#include <TColgp_Array1OfPnt.hxx>
#include <TColgp_Array1OfVec.hxx>
#include <TColgp_SequenceOfPnt.hxx>
#include <TopExp_Explorer.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Iterator.hxx>
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
    double  angle = nI.Angle( nJ );
    bool tooClose = ( angle < 15. * M_PI / 180. );

    // Check if pyramids collide
    if ( !tooClose && ( baI * baJ > 0 ) && ( nI * nJ > 0 ))
    {
      // find out if nI points outside of PrmI or inside
      int    dInd = baseNodesIndI[1] - baseNodesIndI[0];
      bool isOutI = ( abs(dInd)==1 ) ? dInd < 0 : dInd > 0;

      // find out sign of projection of baI to nJ
      double proj = baI * nJ;

      tooClose = ( isOutI ? proj > 0 : proj < 0 );
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
        gp_Vec baOFN( base2, SMESH_TNodeXYZ( otherFaceNode ));
        if ( nI * baOFN > 0 && nJ * baOFN > 0 &&
             baI* baOFN > 0 && baJ* baOFN > 0 ) // issue 0023212
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

  //================================================================================
  /*!
   * \brief Store an error about overlapping faces
   */
  //================================================================================

  bool overlapError( SMESH_Mesh&             mesh,
                     const SMDS_MeshElement* face1,
                     const SMDS_MeshElement* face2,
                     const TopoDS_Shape&     shape = TopoDS_Shape())
  {
    if ( !face1 || !face2 ) return false;

    SMESH_Comment msg;
    msg << "face " << face1->GetID() << " overlaps face " << face2->GetID();

    SMESH_subMesh * sm = 0;
    if ( shape.IsNull() )
    {
      sm = mesh.GetSubMesh( mesh.GetShapeToMesh() );
    }
    else if ( shape.ShapeType() >= TopAbs_SOLID )
    {
      sm = mesh.GetSubMesh( shape );
    }
    else
    {
      TopoDS_Iterator it ( shape );
      if ( it.More() )
        sm = mesh.GetSubMesh( it.Value() );
    }
    if ( sm )
    {
      SMESH_ComputeErrorPtr& err = sm->GetComputeError();
      if ( !err || err->IsOK() )
      {
        err = SMESH_ComputeError::New( COMPERR_BAD_INPUT_MESH, msg, sm->GetAlgo() );
        err->myBadElements.push_back( face1 );
        err->myBadElements.push_back( face2 );
      }
    }
    //throw SALOME_Exception( msg.c_str() );

    return false; // == "algo fails"
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
  // cout << endl << "Merge " << PrmI->GetID() << " " << PrmJ->GetID() << " "
  //      << PrmI->GetNode(4) << PrmJ->GetNode(4) << endl;
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
  for ( size_t i = 0; i < inverseElems.size(); ++i )
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
  vector< const SMDS_MeshNode* > nodes;
  inverseElems.assign( TStdElemIterator( Nrem->GetInverseElementIterator()), itEnd );
  for ( size_t i = 0; i < inverseElems.size(); ++i )
  {
    const SMDS_MeshElement* elem = inverseElems[i];
    nodes.assign( elem->begin_nodes(), elem->end_nodes() );
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
                                                 set<const SMDS_MeshNode*>& nodesToMove,
                                                 const bool                 isRecursion)
{
  TIDSortedElemSet adjacentPyrams;
  bool mergedPyrams = false;
  for ( int k=0; k<4; k++ ) // loop on 4 base nodes of PrmI
  {
    const SMDS_MeshNode*   n = PrmI->GetNode(k);
    SMDS_ElemIteratorPtr vIt = n->GetInverseElementIterator( SMDSAbs_Volume );
    while ( vIt->more() )
    {
      const SMDS_MeshElement* PrmJ = vIt->next();
      if ( PrmJ == PrmI || PrmJ->NbCornerNodes() != 5 || !adjacentPyrams.insert( PrmJ ).second  )
        continue;
      if ( TooCloseAdjacent( PrmI, PrmJ, GetMesh()->HasShapeToMesh() ))
      {
        MergePiramids( PrmI, PrmJ, nodesToMove );
        mergedPyrams = true;
        // container of inverse elements can change
        // vIt = n->GetInverseElementIterator( SMDSAbs_Volume ); -- iterator re-implemented
      }
    }
  }
  if ( mergedPyrams && !isRecursion )
  {
    TIDSortedElemSet::iterator prm;
    for (prm = adjacentPyrams.begin(); prm != adjacentPyrams.end(); ++prm)
      MergeAdjacent( *prm, nodesToMove, true );
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
  const double a2 = P1.SquareDistance(P2);
  const double b2 = P1.SquareDistance(PC);
  const double c2 = P2.SquareDistance(PC);
  if ( a2 < ( b2 + Sqrt( 4 * b2 * c2 ) + c2 ) / 4 ) // ( a < (b+c)/2 )
    return Pbest;
  else {
    // find shift along V in order a to became equal to (b+c)/2
    const double Vsize = V.Magnitude();
    if ( fabs( Vsize ) > std::numeric_limits<double>::min() )
    {
      const double shift = sqrt( a2 + (b2-c2)*(b2-c2)/16/a2 - (b2+c2)/2 );
      Pbest.ChangeCoord() += shift * V.XYZ() / Vsize;
    }
  }
  return Pbest;
}

//=======================================================================
//function : HasIntersection3
//purpose  : Find intersection point between a triangle (P1,P2,P3)
//           and a segment [PC,P]
//=======================================================================

static bool HasIntersection3(const gp_Pnt& P, const gp_Pnt& PC, gp_Pnt& Pint,
                             const gp_Pnt& P1, const gp_Pnt& P2, const gp_Pnt& P3)
{
  const double EPSILON = 1e-6;
  double segLen = P.Distance( PC );

  gp_XYZ  orig = PC.XYZ();
  gp_XYZ   dir = ( P.XYZ() - PC.XYZ() ) / segLen;
  gp_XYZ vert0 = P1.XYZ();
  gp_XYZ vert1 = P2.XYZ();
  gp_XYZ vert2 = P3.XYZ();

  gp_XYZ edge1 = vert1 - vert0;
  gp_XYZ edge2 = vert2 - vert0;

  /* begin calculating determinant - also used to calculate U parameter */
  gp_XYZ pvec = dir ^ edge2;

  /* if determinant is near zero, ray lies in plane of triangle */
  double det = edge1 * pvec;

  const double ANGL_EPSILON = 1e-12;
  if ( det > -ANGL_EPSILON && det < ANGL_EPSILON )
    return false;

  /* calculate distance from vert0 to ray origin */
  gp_XYZ  tvec = orig - vert0;

  /* calculate U parameter and test bounds */
  double u = ( tvec * pvec ) / det;
  //if (u < 0.0 || u > 1.0)
  if (u < -EPSILON || u > 1.0 + EPSILON)
    return false;

  /* prepare to test V parameter */
  gp_XYZ qvec = tvec ^ edge1;

  /* calculate V parameter and test bounds */
  double v = (dir * qvec) / det;
  //if ( v < 0.0 || u + v > 1.0 )
  if ( v < -EPSILON || u + v > 1.0 + EPSILON)
    return false;

  /* calculate t, ray intersects triangle */
  double t = (edge2 * qvec) / det;

  Pint = orig + dir * t;

  return ( t > 0.  &&  t < segLen );
}

//=======================================================================
//function : HasIntersection
//purpose  : Auxilare for CheckIntersection()
//=======================================================================

static bool HasIntersection(const gp_Pnt& P, const gp_Pnt& PC, gp_Pnt& Pint,
                            TColgp_SequenceOfPnt& aContour)
{
  if ( aContour.Length() == 3 ) {
    return HasIntersection3( P, PC, Pint, aContour(1), aContour(2), aContour(3) );
  }
  else {
    bool check = false;
    if( (aContour(1).SquareDistance(aContour(2)) > 1.e-12) &&
        (aContour(1).SquareDistance(aContour(3)) > 1.e-12) &&
        (aContour(2).SquareDistance(aContour(3)) > 1.e-12) ) {
      check = HasIntersection3( P, PC, Pint, aContour(1), aContour(2), aContour(3) );
    }
    if(check) return true;
    if( (aContour(1).SquareDistance(aContour(4)) > 1.e-12) &&
        (aContour(1).SquareDistance(aContour(3)) > 1.e-12) &&
        (aContour(4).SquareDistance(aContour(3)) > 1.e-12) ) {
      check = HasIntersection3( P, PC, Pint, aContour(1), aContour(3), aContour(4) );
    }
    if(check) return true;
  }

  return false;
}

//================================================================================
/*!
 * \brief Return allowed height of a pyramid
 *  \param Papex - optimal pyramid apex
 *  \param PC - gravity center of a quadrangle
 *  \param PN - four nodes of the quadrangle
 *  \param aMesh - mesh
 *  \param NotCheckedFace - the quadrangle face
 *  \param Shape - the shape being meshed
 *  \retval false if mesh invalidity detected
 */
//================================================================================

bool StdMeshers_QuadToTriaAdaptor::LimitHeight (gp_Pnt&                             Papex,
                                                const gp_Pnt&                       PC,
                                                const TColgp_Array1OfPnt&           PN,
                                                const vector<const SMDS_MeshNode*>& FNodes,
                                                SMESH_Mesh&                         aMesh,
                                                const SMDS_MeshElement*             NotCheckedFace,
                                                const bool                          UseApexRay,
                                                const TopoDS_Shape&                 Shape)
{
  if ( !myElemSearcher )
    myElemSearcher = SMESH_MeshAlgos::GetElementSearcher( *aMesh.GetMeshDS() );
  SMESH_ElementSearcher* searcher = const_cast<SMESH_ElementSearcher*>(myElemSearcher);

  // Find intersection of faces with (P,PC) segment elongated 3 times

  double height = Papex.Distance( PC );
  gp_Ax1 line( PC, gp_Vec( PC, Papex ));
  gp_Pnt Pint, Ptest;
  vector< const SMDS_MeshElement* > suspectFaces;
  TColgp_SequenceOfPnt aContour;

  if ( UseApexRay )
  {
    double idealHeight = height;
    const SMDS_MeshElement* intFace = 0;

    // find intersection closest to PC
    Ptest = PC.XYZ() + line.Direction().XYZ() * height * 3;

    searcher->GetElementsNearLine( line, SMDSAbs_Face, suspectFaces );
    for ( size_t iF = 0; iF < suspectFaces.size(); ++iF )
    {
      const SMDS_MeshElement* face = suspectFaces[iF];
      if ( face == NotCheckedFace ) continue;

      aContour.Clear();
      for ( int i = 0, nb = face->NbCornerNodes(); i < nb; ++i )
        aContour.Append( SMESH_TNodeXYZ( face->GetNode(i) ));

      if ( HasIntersection( Ptest, PC, Pint, aContour ))
      {
        double dInt = PC.Distance( Pint ) / 3.;
        if ( dInt < height )
        {
          height = dInt;
          intFace = face;
        }
      }
    }
    if ( height < 1e-2 * idealHeight && intFace )
      return overlapError( aMesh, NotCheckedFace, intFace, Shape );
  }

  // Find faces intersecting triangular facets of the pyramid (issue 23212)

  gp_XYZ center   = PC.XYZ() + line.Direction().XYZ() * height * 0.5;
  double diameter = Max( PN(1).Distance(PN(3)), PN(2).Distance(PN(4)));
  suspectFaces.clear();
  searcher->GetElementsInSphere( center, diameter * 0.6, SMDSAbs_Face, suspectFaces);

  const double upShift = 1.5;
  Ptest = PC.XYZ() + line.Direction().XYZ() * height * upShift; // tmp apex

  for ( size_t iF = 0; iF < suspectFaces.size(); ++iF )
  {
    const SMDS_MeshElement* face = suspectFaces[iF];
    if ( face == NotCheckedFace ) continue;
    if ( face->GetNodeIndex( FNodes[0] ) >= 0 ||
         face->GetNodeIndex( FNodes[1] ) >= 0 ||
         face->GetNodeIndex( FNodes[2] ) >= 0 ||
         face->GetNodeIndex( FNodes[3] ) >= 0 )
      continue; // neighbor face of the quadrangle

    // limit height using points of intersection of face links with pyramid facets
    int   nbN = face->NbCornerNodes();
    gp_Pnt P1 = SMESH_TNodeXYZ( face->GetNode( nbN-1 )); // 1st link end
    for ( int i = 0; i < nbN; ++i )
    {
      gp_Pnt P2 = SMESH_TNodeXYZ( face->GetNode(i) );    // 2nd link end

      for ( int iN = 1; iN <= 4; ++iN ) // loop on pyramid facets
      {
        if ( HasIntersection3( P1, P2, Pint, PN(iN), PN(iN+1), Ptest ))
        {
          height = Min( height, gp_Vec( PC, Pint ) * line.Direction() );
          //Ptest = PC.XYZ() + line.Direction().XYZ() * height * upShift; // new tmp apex
        }
      }
      P1 = P2;
    }
  }

  Papex  = PC.XYZ() + line.Direction().XYZ() * height;

  return true;
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
                                              TColgp_Array1OfPnt&           PN,
                                              TColgp_Array1OfVec&           VN,
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
    PN.SetValue( i+1, p );
    xyzC += p;
  }
  PC = xyzC/4;

  int nbp = 4;

  int j = 0;
  for(i=1; i<4; i++) {
    j = i+1;
    for(; j<=4; j++) {
      if( PN(i).Distance(PN(j)) < 1.e-6 )
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
    gp_Pnt Pdeg = PN(i);

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
      PN.SetValue(i,PN.Value(i+1));
      FNodes[i-1] = FNodes[i];
    }
    nbp = 3;
  }

  PN.SetValue(nbp+1,PN(1));
  FNodes[nbp] = FNodes[0];
  // find normal direction
  gp_Vec V1(PC,PN(nbp));
  gp_Vec V2(PC,PN(1));
  VNorm = V1.Crossed(V2);
  VN.SetValue(nbp,VNorm);
  for(i=1; i<nbp; i++) {
    V1 = gp_Vec(PC,PN(i));
    V2 = gp_Vec(PC,PN(i+1));
    gp_Vec Vtmp = V1.Crossed(V2);
    VN.SetValue(i,Vtmp);
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

  const SMESHDS_SubMesh * aSubMeshDSFace;
  SMESHDS_Mesh * meshDS = aMesh.GetMeshDS();
  SMESH_MesherHelper helper(aMesh);
  helper.IsQuadraticSubMesh(aShape);
  helper.SetElementsOnShape( true );

  if ( myElemSearcher ) delete myElemSearcher;
  vector< SMDS_ElemIteratorPtr > itVec;
  if ( aProxyMesh )
  {
    itVec.push_back( aProxyMesh->GetFaces( aShape ));
  }
  else
  {
    for ( TopExp_Explorer exp(aShape,TopAbs_FACE); exp.More(); exp.Next() )
      if (( aSubMeshDSFace = meshDS->MeshElements( exp.Current() )))
        itVec.push_back( aSubMeshDSFace->GetElements() );
  }
  typedef
    SMDS_IteratorOnIterators< const SMDS_MeshElement*, vector< SMDS_ElemIteratorPtr > > TIter;
  SMDS_ElemIteratorPtr faceIt( new TIter( itVec ));
  myElemSearcher = SMESH_MeshAlgos::GetElementSearcher( *meshDS, faceIt );

  TColgp_Array1OfPnt PN(1,5);
  TColgp_Array1OfVec VN(1,4);
  vector<const SMDS_MeshNode*> FNodes(5);
  gp_Pnt PC;
  gp_Vec VNorm;

  for ( TopExp_Explorer exp(aShape,TopAbs_FACE); exp.More(); exp.Next() )
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
                Pbest = FindBestPoint(PN(i), PN(i+1), PC, VN(i).Reversed());
              else
                Pbest = FindBestPoint(PN(i), PN(i+1), PC, VN(i));
              xc += Pbest.X();
              yc += Pbest.Y();
              zc += Pbest.Z();
            }
            gp_Pnt PCbest(xc/4., yc/4., zc/4.);

            // check PCbest
            double height = PCbest.Distance(PC);
            if ( height < 1.e-6 ) {
              // create new PCbest using a bit shift along VNorm
              PCbest = PC.XYZ() + VNorm.XYZ() * 0.001;
            }
            else {
              // check possible intersection with other faces
              if ( !LimitHeight( PCbest, PC, PN, FNodes, aMesh, face, /*UseApexRay=*/true, aShape ))
                return false;
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
      break;
    }
    else
      groupDS = 0;
  }

  const bool toFindVolumes = aMesh.NbVolumes() > 0;

  vector<const SMDS_MeshElement*> myPyramids;
  SMESH_MesherHelper helper(aMesh);
  helper.IsQuadraticSubMesh(aMesh.GetShapeToMesh());
  helper.SetElementsOnShape( true );

  SMESHDS_Mesh * meshDS = aMesh.GetMeshDS();
  SMESH_ProxyMesh::SubMesh* prxSubMesh = getProxySubMesh();

  if ( !myElemSearcher )
    myElemSearcher = SMESH_MeshAlgos::GetElementSearcher( *meshDS );
  SMESH_ElementSearcher* searcher = const_cast<SMESH_ElementSearcher*>(myElemSearcher);
  SMESHUtils::Deleter<SMESH_ElementSearcher>
    volSearcher( SMESH_MeshAlgos::GetElementSearcher( *meshDS ));
  vector< const SMDS_MeshElement* > suspectFaces, foundVolumes;

  TColgp_Array1OfPnt PN(1,5);
  TColgp_Array1OfVec VN(1,4);
  vector<const SMDS_MeshNode*> FNodes(5);
  TColgp_SequenceOfPnt aContour;

  SMDS_FaceIteratorPtr fIt = meshDS->facesIterator(/*idInceasingOrder=*/true);
  while( fIt->more())
  {
    const SMDS_MeshElement* face = fIt->next();
    if ( !face ) continue;
    // retrieve needed information about a face
    gp_Pnt PC;
    gp_Vec VNorm;
    const SMDS_MeshElement* volumes[2];
    int what = Preparation(face, PN, VN, FNodes, PC, VNorm, volumes);
    if ( what == NOT_QUAD )
      continue;
    if ( volumes[0] && volumes[1] )
      continue; // face is shared by two volumes - no room for a pyramid

    if ( what == DEGEN_QUAD )
    {
      // degenerate face
      // add a triangle to the proxy mesh
      SMDS_MeshFace* NewFace;

      // check orientation
      double tmp = PN(1).Distance(PN(2)) + PN(2).Distance(PN(3));
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
      vector< const SMDS_MeshElement* > suspectFaces;
      searcher->GetElementsNearLine( line, SMDSAbs_Face, suspectFaces);

      for ( size_t iF = 0; iF < suspectFaces.size(); ++iF ) {
        const SMDS_MeshElement* F = suspectFaces[iF];
        if ( F == face ) continue;
        aContour.Clear();
        for ( int i = 0; i < 4; ++i )
          aContour.Append( SMESH_TNodeXYZ( F->GetNode(i) ));
        gp_Pnt PPP;
        if ( !volumes[0] && HasIntersection( Ptmp1, PC, PPP, aContour )) {
          IsOK1 = true;
          double tmp = PC.Distance(PPP);
          if ( tmp < dist1 ) {
            Pres1 = PPP;
            dist1 = tmp;
          }
        }
        if ( !volumes[1] && HasIntersection( Ptmp2, PC, PPP, aContour )) {
          IsOK2 = true;
          double tmp = PC.Distance(PPP);
          if ( tmp < dist2 ) {
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

    // -----------------------------------
    // Case of non-degenerated quadrangle
    // -----------------------------------

    // Find pyramid peak

    gp_XYZ PCbest(0., 0., 0.); // pyramid peak
    int i = 1;
    for ( ; i <= 4; i++ ) {
      gp_Pnt Pbest = FindBestPoint(PN(i), PN(i+1), PC, VN(i));
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
    double tmp = PN(1).Distance(PN(3)) + PN(2).Distance(PN(4));
    // far points: in (PC, PCbest) direction and vice-versa
    gp_Pnt farPnt[2] = { PC.XYZ() + tmpDir.XYZ() * tmp * 1.e6,
                         PC.XYZ() - tmpDir.XYZ() * tmp * 1.e6 };
    // check intersection for farPnt1 and farPnt2
    bool   intersected[2] = { false, false };
    double dist2int   [2] = { RealLast(), RealLast() };
    gp_Pnt intPnt     [2];
    int    intFaceInd [2] = { 0, 0 };

    if ( toFindVolumes && 0 ) // non-conformal mesh is not suitable for any mesher so far
    {
      // there are volumes in the mesh, in a non-conformal mesh an neighbor
      // volume can be not found yet
      for ( int isRev = 0; isRev < 2; ++isRev )
      {
        if ( volumes[isRev] ) continue;
        gp_Pnt testPnt = PC.XYZ() + tmpDir.XYZ() * height * ( isRev ? -0.1: 0.1 );
        foundVolumes.clear();
        if ( volSearcher->FindElementsByPoint( testPnt, SMDSAbs_Volume, foundVolumes ))
          volumes[isRev] = foundVolumes[0];
      }
      if ( volumes[0] && volumes[1] )
        continue; // no room for a pyramid
    }

    gp_Ax1 line( PC, tmpDir );
    suspectFaces.clear();
    searcher->GetElementsNearLine( line, SMDSAbs_Face, suspectFaces);

    for ( size_t iF = 0; iF < suspectFaces.size(); ++iF )
    {
      const SMDS_MeshElement* F = suspectFaces[iF];
      if ( F == face ) continue;
      aContour.Clear();
      int nbN = F->NbCornerNodes();
      for ( i = 0; i < nbN; ++i )
        aContour.Append( SMESH_TNodeXYZ( F->GetNode(i) ));
      gp_Pnt intP;
      for ( int isRev = 0; isRev < 2; ++isRev )
      {
        if( !volumes[isRev] && HasIntersection(farPnt[isRev], PC, intP, aContour) )
        {
          double d = PC.Distance( intP );
          if ( d < dist2int[isRev] )
          {
            intersected[isRev] = true;
            intPnt     [isRev] = intP;
            dist2int   [isRev] = d;
            intFaceInd [isRev] = iF;
          }
        }
      }
    }

    // if the face belong to the group of skinFaces, do not build a pyramid outside
    if ( groupDS && groupDS->Contains(face) )
    {
      intersected[0] = false;
    }
    else if ( intersected[0] && intersected[1] ) // check if one of pyramids is in a hole
    {
      gp_Pnt P ( PC.XYZ() + tmpDir.XYZ() * 0.5 * dist2int[0] );
      if ( searcher->GetPointState( P ) == TopAbs_OUT )
        intersected[0] = false;
      else
      {
        P = ( PC.XYZ() - tmpDir.XYZ() * 0.5 * dist2int[1] );
        if ( searcher->GetPointState( P ) == TopAbs_OUT )
          intersected[1] = false;
      }
    }

    // Create one or two pyramids

    for ( int isRev = 0; isRev < 2; ++isRev )
    {
      if ( !intersected[isRev] ) continue;
      double pyramidH = Min( height, dist2int[isRev]/3. );
      gp_Pnt    Papex = PC.XYZ() + tmpDir.XYZ() * (isRev ? -pyramidH : pyramidH);
      if ( pyramidH < 1e-2 * height )
        return overlapError( aMesh, face, suspectFaces[ intFaceInd[isRev] ] );

      if ( !LimitHeight( Papex, PC, PN, FNodes, aMesh, face, /*UseApexRay=*/false ))
        return false;

      // create node for Papex
      SMDS_MeshNode* NewNode = helper.AddNode( Papex.X(), Papex.Y(), Papex.Z() );

      // add triangles to result map
      for ( i = 0; i < 4; i++) {
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
  if ( myPyramids.empty() )
    return true;

  SMESHDS_Mesh * meshDS = aMesh.GetMeshDS();
  size_t i, j, k;
  //int myShapeID = myPyramids[0]->GetNode(4)->getshapeId();
  {
    SMDS_ElemIteratorPtr
      pyramIt( new SMDS_ElementVectorIterator( myPyramids.begin(), myPyramids.end() ));
    if ( myElemSearcher ) delete myElemSearcher;
    myElemSearcher = SMESH_MeshAlgos::GetElementSearcher( *meshDS, pyramIt );
  }
  SMESH_ElementSearcher* searcher = const_cast<SMESH_ElementSearcher*>( myElemSearcher );

  set<const SMDS_MeshNode*> nodesToMove;

  // check adjacent pyramids

  for ( i = 0; i <  myPyramids.size(); ++i )
  {
    const SMDS_MeshElement* PrmI = myPyramids[i];
    MergeAdjacent( PrmI, nodesToMove );
  }

  // iterate on all new pyramids
  vector< const SMDS_MeshElement* > suspectPyrams;
  for ( i = 0; i <  myPyramids.size(); ++i )
  {
    const SMDS_MeshElement*  PrmI = myPyramids[i];
    const SMDS_MeshNode*    apexI = PrmI->GetNode( PYRAM_APEX );

    // compare PrmI with all the rest pyramids

    // collect adjacent pyramids and nodes coordinates of PrmI
    set<const SMDS_MeshElement*> checkedPyrams;
    gp_Pnt PsI[5];
    for ( k = 0; k < 5; k++ )
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

    // get pyramids to check
    gp_XYZ       PC = ( PsI[0].XYZ() + PsI[1].XYZ() + PsI[2].XYZ() + PsI[3].XYZ() ) / 4.;
    gp_XYZ      ray = PsI[4].XYZ() - PC;
    gp_XYZ   center = PC + 0.5 * ray;
    double diameter = Max( PsI[0].Distance(PsI[2]), PsI[1].Distance(PsI[3]));
    suspectPyrams.clear();
    searcher->GetElementsInSphere( center, diameter * 0.6, SMDSAbs_Volume, suspectPyrams);

    // check intersection with distant pyramids
    for ( j = 0; j < suspectPyrams.size(); ++j )
    {
      const SMDS_MeshElement* PrmJ = suspectPyrams[j];
      if ( PrmJ == PrmI )
        continue;
      if ( apexI == PrmJ->GetNode( PYRAM_APEX ))
        continue; // pyramids PrmI and PrmJ already merged
      if ( !checkedPyrams.insert( PrmJ ).second )
        continue; // already checked

      gp_Pnt PsJ[5];
      for ( k = 0; k < 5; k++ )
        PsJ[k] = SMESH_TNodeXYZ( PrmJ->GetNode(k) );

      if ( ray * ( PsJ[4].XYZ() - PC ) < 0. )
        continue; // PrmJ is below PrmI

      for ( k = 0; k < 4; k++ ) // loop on 4 base nodes of PrmI
      {
        gp_Pnt Pint;
        bool hasInt=false;
        for ( k = 0; k < 4  &&  !hasInt; k++ )
        {
          gp_Vec Vtmp( PsI[k], PsI[ PYRAM_APEX ]);
          gp_Pnt Pshift = PsI[k].XYZ() + Vtmp.XYZ() * 0.01; // base node moved a bit to apex
          hasInt =
          ( HasIntersection3( Pshift, PsI[4], Pint, PsJ[0], PsJ[1], PsJ[PYRAM_APEX]) ||
            HasIntersection3( Pshift, PsI[4], Pint, PsJ[1], PsJ[2], PsJ[PYRAM_APEX]) ||
            HasIntersection3( Pshift, PsI[4], Pint, PsJ[2], PsJ[3], PsJ[PYRAM_APEX]) ||
            HasIntersection3( Pshift, PsI[4], Pint, PsJ[3], PsJ[0], PsJ[PYRAM_APEX]) );
        }
        for ( k = 0; k < 4  &&  !hasInt; k++ )
        {
          gp_Vec Vtmp( PsJ[k], PsJ[ PYRAM_APEX ]);
          gp_Pnt Pshift = PsJ[k].XYZ() + Vtmp.XYZ() * 0.01;
          hasInt =
            ( HasIntersection3( Pshift, PsJ[4], Pint, PsI[0], PsI[1], PsI[PYRAM_APEX]) ||
              HasIntersection3( Pshift, PsJ[4], Pint, PsI[1], PsI[2], PsI[PYRAM_APEX]) ||
              HasIntersection3( Pshift, PsJ[4], Pint, PsI[2], PsI[3], PsI[PYRAM_APEX]) ||
              HasIntersection3( Pshift, PsJ[4], Pint, PsI[3], PsI[0], PsI[PYRAM_APEX]) );
        }

        if ( hasInt )
        {
          // count common nodes of base faces of two pyramids
          int nbc = 0;
          for ( k = 0; k < 4; k++ )
            nbc += int ( PrmI->GetNodeIndex( PrmJ->GetNode(k) ) >= 0 );

          if ( nbc == 4 )
            continue; // pyrams have a common base face

          if ( nbc > 0 )
          {
            // Merge the two pyramids and others already merged with them
            MergePiramids( PrmI, PrmJ, nodesToMove );
          }
          else  // nbc==0
          {
            // decrease height of pyramids
            gp_XYZ PCi(0,0,0), PCj(0,0,0);
            for ( k = 0; k < 4; k++ ) {
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
            SMDS_MeshNode* aNode1 = const_cast<SMDS_MeshNode*>( apexI );
            aNode1->setXYZ( PCi.X()+VN1.X(), PCi.Y()+VN1.Y(), PCi.Z()+VN1.Z() );
            SMDS_MeshNode* aNode2 = const_cast<SMDS_MeshNode*>(PrmJ->GetNode( PYRAM_APEX ));
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
