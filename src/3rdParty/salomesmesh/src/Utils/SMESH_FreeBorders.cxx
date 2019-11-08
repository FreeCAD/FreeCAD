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
// File      : SMESH_FreeBorders.cxx
// Created   : Tue Sep  8 17:08:39 2015
// Author    : Edward AGAPOV (eap)

//================================================================================
// Implementation of SMESH_MeshAlgos::FindCoincidentFreeBorders()
//================================================================================

#include "SMESH_MeshAlgos.hxx"

#include "SMDS_LinearEdge.hxx"
#include "SMDS_Mesh.hxx"
#include "SMDS_SetIterator.hxx"

#include <algorithm>
#include <limits>
#include <set>
#include <vector>

#include <NCollection_DataMap.hxx>
#include <Precision.hxx>
#include <gp_Pnt.hxx>

using namespace SMESH_MeshAlgos;

namespace
{
  struct BEdge;

  /*!
   * \brief Node on a free border
   */
  struct BNode : public SMESH_TNodeXYZ
  {
    mutable std::vector< BEdge* > myLinkedEdges;
    mutable std::vector< std::pair < BEdge*, double > > myCloseEdges; // edge & U

    BNode(const SMDS_MeshNode * node): SMESH_TNodeXYZ( node ) {}
    const SMDS_MeshNode * Node() const { return _node; }
    void   AddLinked( BEdge* e ) const;
    void   AddClose ( const BEdge* e, double u ) const;
    BEdge* GetCloseEdge( size_t i ) const { return myCloseEdges[i].first; }
    double GetCloseU( size_t i ) const { return myCloseEdges[i].second; }
    BEdge* GetCloseEdgeOfBorder( int borderID, double * u = 0 ) const;
    bool   HasCloseEdgeWithNode( const BNode* n ) const;
    bool   IsCloseEdge( const BEdge*, double * u = 0 ) const;
    bool operator<(const BNode& other) const { return Node()->GetID() < other.Node()->GetID(); }
    double SquareDistance(const BNode& e2) const { return ( e2 - *this ).SquareModulus(); }
  };
  /*!
   * \brief Edge of a free border
   */
  struct BEdge : public SMDS_LinearEdge
  {
    const BNode*            myBNode1;
    const BNode*            myBNode2;
    int                     myBorderID;
    int                     myID; // within a border
    BEdge*                  myPrev;
    BEdge*                  myNext;
    const SMDS_MeshElement* myFace;
    std::set< int >         myCloseBorders;
    int                     myInGroup;

    BEdge():SMDS_LinearEdge( 0, 0 ), myBorderID(-1), myID(-1), myPrev(0), myNext(0), myInGroup(-1) {}

    void Set( const BNode *           node1,
              const BNode *           node2,
              const SMDS_MeshElement* face,
              const int               ID)
    {
      myBNode1   = node1;
      myBNode2   = node2;
      myNodes[0] = node1->Node();
      myNodes[1] = node2->Node();
      myFace     = face;
      setId( ID ); // mesh element ID
    }
    bool IsInGroup() const
    {
      return myInGroup >= 0;
    }
    bool Contains( const BNode* n ) const
    {
      return ( n == myBNode1 || n == myBNode2 );
    }
    void AddLinked( BEdge* e )
    {
      if ( e->Contains( myBNode1 )) myPrev = e;
      else                          myNext = e;
    }
    void RemoveLinked( BEdge* e )
    {
      if ( myPrev == e ) myPrev = 0;
      if ( myNext == e ) myNext = 0;
    }
    void Reverse()
    {
      std::swap( myBNode1, myBNode2 );
      myNodes[0] = myBNode1->Node();
      myNodes[1] = myBNode2->Node();
    }
    void Orient()
    {
      if (( myPrev && !myPrev->Contains( myBNode1 )) ||
          ( myNext && !myNext->Contains( myBNode2 )))
        std::swap( myPrev, myNext );
      if ( myPrev && myPrev->myBNode2 != myBNode1 ) myPrev->Reverse();
      if ( myNext && myNext->myBNode1 != myBNode2 ) myNext->Reverse();
    }
    void SetID( int id )
    {
      if ( myID < 0 )
      {
        myID = id;
        if ( myNext )
          myNext->SetID( id + 1 );
      }
    }
    //================================================================================
    /*!
     * \brief Checks if a point is closer to this BEdge than tol
     */
    //================================================================================

    bool IsOut( const gp_XYZ& point, const double tol, double& u ) const
    {
      gp_XYZ  me = *myBNode2 - *myBNode1;
      gp_XYZ n1p = point     - *myBNode1;
      u = ( me * n1p ) / me.SquareModulus(); // param [0,1] on this
      if ( u < 0. ) return ( n1p.SquareModulus() > tol * tol );
      if ( u > 1. ) return ( ( point - *myBNode2 ).SquareModulus() > tol * tol );

      gp_XYZ  proj = ( 1. - u ) * *myBNode1 + u * *myBNode2; // projection of the point on this
      double dist2 = ( point - proj ).SquareModulus();
      return ( dist2 > tol * tol );
    }
    //================================================================================
    /*!
     * \brief Checks if two BEdges can be considered as overlapping
     */
    //================================================================================

    bool IsOverlappingProjection( const BEdge* toE, const double u, bool is1st ) const
    {
      // is1st shows which end of toE is projected on this at u
      double u2;
      const double eps = 0.1;
      if ( myBNode1->IsCloseEdge( toE, &u2 ) ||
           myBNode2->IsCloseEdge( toE, &u2 ))
        return (( 0 < u2 && u2 < 1 ) &&     // u2 is proj param of myBNode's on toE
                ( Abs( u2 - int( !is1st )) > eps ));

      const BNode* n = is1st ? toE->myBNode2 : toE->myBNode1;
      if ( this == n->GetCloseEdgeOfBorder( this->myBorderID, &u2 ))
        return Abs( u - u2 ) > eps;
      return false;
    }
    //================================================================================
    /*!
     * \brief Finds all neighbor BEdge's having the same close borders
     */
    //================================================================================

    bool GetRangeOfSameCloseBorders(BEdge* eRange[2], const std::set< int >& bordIDs)
    {
      if ( this->myCloseBorders != bordIDs )
        return false;

      if ( bordIDs.size() == 1 && bordIDs.count( myBorderID )) // border close to self
      {
        double u;
        eRange[0] = this;
        while ( eRange[0]->myBNode1->GetCloseEdgeOfBorder( myBorderID, &u ))
        {
          if ( eRange[0]->myPrev == this || u < 0 || u > 1 )
            break;
          eRange[0] = eRange[0]->myPrev;
        }
        eRange[1] = this;
        while ( eRange[1]->myBNode2->GetCloseEdgeOfBorder( myBorderID, &u ))
        {
          if ( eRange[1]->myNext == this || u < 0 || u > 1 )
            break;
          eRange[1] = eRange[1]->myNext;
        }
      }
      else
      {
        eRange[0] = this;
        while ( eRange[0]->myPrev && eRange[0]->myPrev->myCloseBorders == bordIDs )
        {
          if ( eRange[0]->myPrev == this )
            break;
          eRange[0] = eRange[0]->myPrev;
        }

        eRange[1] = this;
        if ( eRange[0]->myPrev != this ) // not closed border
          while ( eRange[1]->myNext && eRange[1]->myNext->myCloseBorders == bordIDs )
          {
            if ( eRange[1]->myNext == this )
              break;
            eRange[1] = eRange[1]->myNext;
          }
      }

      if ( eRange[0] == eRange[1] )
      {
        std::set<int>::iterator closeBord = eRange[0]->myCloseBorders.begin();
        for ( ; closeBord != eRange[0]->myCloseBorders.end(); ++closeBord )
        {
          if ( BEdge* be = eRange[0]->myBNode1->GetCloseEdgeOfBorder( *closeBord ))
            if ( be->myCloseBorders == eRange[0]->myCloseBorders )
              return true;
          if ( BEdge* be = eRange[0]->myBNode2->GetCloseEdgeOfBorder( *closeBord ))
            if ( be->myCloseBorders == eRange[0]->myCloseBorders )
              return true;
        }
        return false;
      }
      return true;
    }
  }; // class BEdge

  //================================================================================
  /*!
   * \brief Checks if all border parts include the whole closed border, and if so
   *        returns \c true and choose starting BEdge's with most coincident nodes
   */
  //================================================================================

  bool chooseStartOfClosedBorders( std::vector< BEdge* >& ranges ) // PAL23078#c21002
  {
    bool allClosed = true;
    for ( size_t iR = 1; iR < ranges.size() && allClosed; iR += 2 )
      allClosed = ( ranges[ iR-1 ]->myPrev == ranges[ iR ] );
    if ( !allClosed )
      return allClosed;

    double u, minDiff = Precision::Infinite();
    std::vector< BEdge* > start( ranges.size() / 2 );
    BEdge* range0 = start[0] = ranges[0];
    do
    {
      double maxDiffU = 0;
      double  maxDiff = 0;
      for ( size_t iR = 3; iR < ranges.size(); iR += 2 )
      {
        int borderID = ranges[iR]->myBorderID;
        if ( BEdge* e = start[0]->myBNode1->GetCloseEdgeOfBorder( borderID, & u ))
        {
          start[ iR / 2 ] = e;
          double diffU = Min( Abs( u ), Abs( 1.-u ));
          double  diff = e->myBNode1->SquareDistance( *e->myBNode2 ) * diffU * diffU;
          maxDiffU = Max( diffU, maxDiffU );
          maxDiff  = Max( diff,  maxDiff );
        }
      }
      if ( maxDiff < minDiff )
      {
        minDiff = maxDiff;
        for ( size_t iR = 1; iR < ranges.size(); iR += 2 )
        {
          ranges[ iR-1 ] = start[ iR/2 ];
          ranges[ iR   ] = ranges[ iR-1]->myPrev;
        }
      }
      if ( maxDiffU < 1e-6 )
        break;
      start[0] = start[0]->myNext;
    }
    while ( start[0] != range0 );

    return allClosed;
  }

  //================================================================================
  /*!
   * \brief Tries to include neighbor BEdge's into a border part
   */
  //================================================================================

  void extendPart( BEdge* & e1, BEdge* & e2, const std::set< int >& bordIDs, int groupID )
  {
    if (( e1->myPrev == e2 ) ||
        ( e1 == e2 && e1->myPrev && e1->myPrev->myInGroup == groupID ))
      return; // full free border already

    double u;
    BEdge* be;
    std::set<int>::const_iterator bord;
    if ( e1->myPrev )
    {
      for ( bord = bordIDs.begin(); bord != bordIDs.end(); ++bord )
        if ((( be = e1->myBNode1->GetCloseEdgeOfBorder( *bord, &u ))) &&
            (  be->myInGroup == groupID ) &&
            (  0 < u && u < 1 ) &&
            (  be->IsOverlappingProjection( e1->myPrev, u, false )))
        {
          e1 = e1->myPrev;
          break;
        }
      if ( bord == bordIDs.end() && // not extended
           e1->myBNode1->HasCloseEdgeWithNode( e1->myPrev->myBNode1 ))
      {
        e1 = e1->myPrev;
      }
      e1->myInGroup = groupID;
    }
    if ( e2->myNext )
    {
      for ( bord = bordIDs.begin(); bord != bordIDs.end(); ++bord )
        if ((( be = e2->myBNode2->GetCloseEdgeOfBorder( *bord, &u ))) &&
            (  be->myInGroup == groupID ) &&
            (  0 < u && u < 1 ) &&
            (  be->IsOverlappingProjection( e2->myNext, u, true )))
        {
          e2 = e2->myNext;
          break;
        }
      if ( bord == bordIDs.end() && // not extended
           e2->myBNode2->HasCloseEdgeWithNode( e2->myNext->myBNode2 ))
      {
        e2 = e2->myNext;
      }
      e2->myInGroup = groupID;
    }
  }

  //================================================================================
  /*!
   * \brief Connect BEdge's incident at this node
   */
  //================================================================================

  void BNode::AddLinked( BEdge* e ) const
  {
    myLinkedEdges.reserve(2);
    myLinkedEdges.push_back( e );
    if ( myLinkedEdges.size() < 2 ) return;

    if ( myLinkedEdges.size() == 2 )
    {
      myLinkedEdges[0]->AddLinked( myLinkedEdges[1] );
      myLinkedEdges[1]->AddLinked( myLinkedEdges[0] );
    }
    else
    {
      for ( size_t i = 0; i < myLinkedEdges.size(); ++i )
        for ( size_t j = 0; j < myLinkedEdges.size(); ++j )
          if ( i != j )
            myLinkedEdges[i]->RemoveLinked( myLinkedEdges[j] );
    }
  }
  void BNode::AddClose ( const BEdge* e, double u ) const
  {
    if ( ! e->Contains( this ))
      myCloseEdges.push_back( std::make_pair( const_cast< BEdge* >( e ), u ));
  }
  BEdge* BNode::GetCloseEdgeOfBorder( int borderID, double * uPtr ) const
  {
    BEdge* e = 0;
    double u = 0;
    for ( size_t i = 0; i < myCloseEdges.size(); ++i )
      if ( borderID == GetCloseEdge( i )->myBorderID )
      {
        if ( e && Abs( u - 0.5 ) < Abs( GetCloseU( i ) - 0.5 ))
          continue;
        u = GetCloseU( i );
        e = GetCloseEdge ( i );
      }
    if ( uPtr ) *uPtr = u;
    return e;
  }
  bool BNode::HasCloseEdgeWithNode( const BNode* n ) const
  {
    for ( size_t i = 0; i < myCloseEdges.size(); ++i )
      if ( GetCloseEdge( i )->Contains( n ) &&
           0 < GetCloseU( i ) && GetCloseU( i ) < 1 )
        return true;
    return false;
  }
  bool BNode::IsCloseEdge( const BEdge* e, double * uPtr ) const
  {
    for ( size_t i = 0; i < myCloseEdges.size(); ++i )
      if ( e == GetCloseEdge( i ) )
      {
        if ( uPtr ) *uPtr = GetCloseU( i );
        return true;
      }
    return false;
  }

  /// Accessor to SMDS_MeshElement* inherited by BEdge
  struct ElemAcess
  {
    static const SMDS_MeshElement* value( std::vector< BEdge >::const_iterator it)
    {
      return & (*it);
    }
  };
  /// Iterator over a vector of BEdge's
  static SMDS_ElemIteratorPtr getElemIterator( const std::vector< BEdge > & bedges )
  {
    typedef SMDS_SetIterator
      < const SMDS_MeshElement*, std::vector< BEdge >::const_iterator, ElemAcess > BEIter;
    return SMDS_ElemIteratorPtr( new BEIter( bedges.begin(), bedges.end() ));
  }

} // namespace

//================================================================================
/*
 * Returns groups of TFreeBorder's coincident within the given tolerance.
 * If the tolerance <= 0.0 then one tenth of an average size of elements adjacent
 * to free borders being compared is used.
 */
//================================================================================

void SMESH_MeshAlgos::FindCoincidentFreeBorders(SMDS_Mesh&              mesh,
                                                double                  tolerance,
                                                CoincidentFreeBorders & foundFreeBordes)
{
  // find free links
  typedef NCollection_DataMap<SMESH_TLink, const SMDS_MeshElement*, SMESH_TLink > TLink2FaceMap;
  TLink2FaceMap linkMap;
  int nbSharedLinks = 0;
  SMDS_FaceIteratorPtr faceIt = mesh.facesIterator();
  while ( faceIt->more() )
  {
    const SMDS_MeshElement* face = faceIt->next();
    if ( !face ) continue;

    const SMDS_MeshNode*     n0 = face->GetNode( face->NbNodes() - 1 );
    SMDS_NodeIteratorPtr nodeIt = face->interlacedNodesIterator();
    while ( nodeIt->more() )
    {
      const SMDS_MeshNode* n1 = nodeIt->next();
      SMESH_TLink link( n0, n1 );
      if ( const SMDS_MeshElement** faceInMap = linkMap.ChangeSeek( link ))
      {
        nbSharedLinks += bool( *faceInMap );
        *faceInMap = 0;
      }
      else
      {
        linkMap.Bind( link, face );
      }
      n0 = n1;
    }
  }
  if ( linkMap.Extent() == nbSharedLinks )
    return;

  // form free borders
  std::set   < BNode > bNodes;
  std::vector< BEdge > bEdges( linkMap.Extent() - nbSharedLinks );

  TLink2FaceMap::Iterator linkIt( linkMap );
  for ( int iEdge = 0; linkIt.More(); linkIt.Next() )
  {
    if ( !linkIt.Value() ) continue;
    const SMESH_TLink & link = linkIt.Key();
    std::set< BNode >::iterator n1 = bNodes.insert( BNode( link.node1() )).first;
    std::set< BNode >::iterator n2 = bNodes.insert( BNode( link.node2() )).first;
    bEdges[ iEdge ].Set( &*n1, &*n2, linkIt.Value(), iEdge+1 );
    n1->AddLinked( & bEdges[ iEdge ] );
    n2->AddLinked( & bEdges[ iEdge ] );
    ++iEdge;
  }
  linkMap.Clear();

  // assign IDs to borders
  std::vector< BEdge* > borders; // 1st of connected (via myPrev and myNext) edges
  std::set< BNode >::iterator bn = bNodes.begin();
  for ( ; bn != bNodes.end(); ++bn )
  {
    for ( size_t i = 0; i < bn->myLinkedEdges.size(); ++i )
    {
      if ( bn->myLinkedEdges[i]->myBorderID < 0 )
      {
        BEdge* be = bn->myLinkedEdges[i];
        int borderID = borders.size();
        borders.push_back( be );
        for ( ; be && be->myBorderID < 0; be = be->myNext )
        {
          be->myBorderID = borderID;
          be->Orient();
        }
        bool isClosed = ( be == bn->myLinkedEdges[i] );
        be = bn->myLinkedEdges[i]->myPrev;
        for ( ; be && be->myBorderID < 0; be = be->myPrev )
        {
          be->myBorderID = borderID;
          be->Orient();
        }
        if ( !isClosed )
          while ( borders.back()->myPrev )
            borders.back() = borders.back()->myPrev;

        borders.back()->SetID( 0 ); // set IDs to all edges of the border
      }
    }
  }

  // compute tolerance of each border
  double maxTolerance = tolerance;
  std::vector< double > bordToler( borders.size(), tolerance );
  if ( maxTolerance < std::numeric_limits< double >::min() )
  {
    // no tolerance provided by the user; compute tolerance of each border
    // as one tenth of an average size of faces adjacent to a border
    for ( size_t i = 0; i < borders.size(); ++i )
    {
      double avgFaceSize = 0;
      int    nbFaces     = 0;
      BEdge* be = borders[ i ];
      do {
        double facePerimeter = 0;
        gp_Pnt p0 = SMESH_TNodeXYZ( be->myFace->GetNode( be->myFace->NbNodes() - 1 ));
        SMDS_NodeIteratorPtr nodeIt = be->myFace->interlacedNodesIterator();
        while ( nodeIt->more() )
        {
          gp_Pnt p1 = SMESH_TNodeXYZ( nodeIt->next() );
          facePerimeter += p0.Distance( p1 );
          p0 = p1;
        }
        avgFaceSize += ( facePerimeter / be->myFace->NbCornerNodes() );
        nbFaces++;

        be = be->myNext;
      }
      while ( be && be != borders[i] );

      bordToler[ i ] = 0.1 * avgFaceSize / nbFaces;
      maxTolerance = Max( maxTolerance, bordToler[ i ]);
    }
  }

  // for every border node find close border edges
  SMESH_ElementSearcher* searcher =
    GetElementSearcher( mesh, getElemIterator( bEdges ), maxTolerance );
  SMESHUtils::Deleter< SMESH_ElementSearcher > searcherDeleter( searcher );
  std::vector< const SMDS_MeshElement* > candidateEdges;
  for ( bn = bNodes.begin(); bn != bNodes.end(); ++bn )
  {
    searcher->FindElementsByPoint( *bn, SMDSAbs_Edge, candidateEdges );
    if ( candidateEdges.size() <= bn->myLinkedEdges.size() )
      continue;

    double nodeTol = 0, u;
    for ( size_t i = 0; i < bn->myLinkedEdges.size(); ++i )
      nodeTol = Max( nodeTol, bordToler[ bn->myLinkedEdges[ i ]->myBorderID ]);

    for ( size_t i = 0; i < candidateEdges.size(); ++i )
    {
      const BEdge* be = static_cast< const BEdge* >( candidateEdges[ i ]);
      double      tol = Max( nodeTol, bordToler[ be->myBorderID ]);
      if ( !be->IsOut( *bn, tol, u ))
        bn->AddClose( be, u );
    }
  }

  // for every border edge find close borders

  std::vector< BEdge* > closeEdges;
  for ( size_t i = 0; i < bEdges.size(); ++i )
  {
    BEdge& be = bEdges[i];
    if ( be.myBNode1->myCloseEdges.empty() ||
         be.myBNode2->myCloseEdges.empty() )
      continue;

    closeEdges.clear();
    for ( size_t iE1 = 0; iE1 < be.myBNode1->myCloseEdges.size(); ++iE1 )
    {
      // find edges of the same border close to both nodes of the edge
      BEdge* closeE1 = be.myBNode1->GetCloseEdge( iE1 );
      BEdge* closeE2 = be.myBNode2->GetCloseEdgeOfBorder( closeE1->myBorderID );
      if ( !closeE2 )
        continue;
      // check that edges connecting closeE1 and closeE2 (if any) are also close to 'be'
      if ( closeE1 != closeE2 )
      {
        bool coincide;
        for ( int j = 0; j < 2; ++j ) // move closeE1 -> closeE2 or inversely
        {
          BEdge* ce = closeE1;
          do {
            coincide = ( ce->myBNode2->GetCloseEdgeOfBorder( be.myBorderID ));
            ce       = ce->myNext;
          } while ( coincide && ce && ce != closeE2 );

          if ( coincide && ce == closeE2 )
            break;
          if ( j == 0 )
            std::swap( closeE1, closeE2 );
          coincide = false;
        }
        if ( !coincide )
          continue;
        closeEdges.push_back( closeE1 );
        closeEdges.push_back( closeE2 );
      }
      else
      {
        closeEdges.push_back( closeE1 );
      }
      be.myCloseBorders.insert( closeE1->myBorderID );
    }
    if ( !closeEdges.empty() )
    {
      be.myCloseBorders.insert( be.myBorderID );
      // for ( size_t iB = 0; iB < closeEdges.size(); ++iB )
      //   closeEdges[ iB ]->myCloseBorders.insert( be.myCloseBorders.begin(),
      //                                            be.myCloseBorders.end() );
    }
  }

  // Fill in CoincidentFreeBorders

  // save nodes of free borders
  foundFreeBordes._borders.resize( borders.size() );
  for ( size_t i = 0; i < borders.size(); ++i )
  {
    BEdge* be = borders[i];
    foundFreeBordes._borders[i].push_back( be->myBNode1->Node() );
    do {
      foundFreeBordes._borders[i].push_back( be->myBNode2->Node() );
      be = be->myNext;
    }
    while ( be && be != borders[i] );
  }

  // form groups of coincident parts of free borders

  TFreeBorderPart       part;
  TCoincidentGroup      group;
  std::vector< BEdge* > ranges; // couples of edges delimiting parts
  BEdge* be = 0; // a current edge
  int skipGroup = bEdges.size(); // a group ID used to avoid repeating treatment of edges

  for ( int i = 0, nbBords = borders.size(); i < nbBords; i += bool(!be) )
  {
    if ( !be )
      be = borders[i];

    // look for an edge close to other borders
    do {
      if ( !be->IsInGroup() && !be->myCloseBorders.empty() )
        break;
      be = be->myNext;
    } while ( be && be != borders[i] );

    if ( !be || be->IsInGroup() || be->myCloseBorders.empty() )
    {
      be = 0;
      continue; // all edges of a border are treated or non-coincident
    }
    group.clear();
    ranges.clear();

    // look for the 1st and last edge of a coincident group
    BEdge* beRange[2];
    if ( !be->GetRangeOfSameCloseBorders( beRange, be->myCloseBorders ))
    {
      be->myInGroup = skipGroup;
      be = be->myNext;
      continue;
    }

    ranges.push_back( beRange[0] );
    ranges.push_back( beRange[1] );

    int groupID = foundFreeBordes._coincidentGroups.size();
    be = beRange[0];
    be->myInGroup = groupID;
    while ( be != beRange[1] )
    {
      be->myInGroup = groupID;
      be = be->myNext;
    }
    beRange[1]->myInGroup = groupID;

    // get starting edge of each close border
    closeEdges.clear();
    be = beRange[0];
    if ( be->myCloseBorders.empty() )
      be = beRange[0]->myNext;
    std::set<int>::iterator closeBord = be->myCloseBorders.begin();
    for ( ; closeBord != be->myCloseBorders.end(); ++closeBord )
      if ( BEdge* e = be->myBNode2->GetCloseEdgeOfBorder( *closeBord ))
        closeEdges.push_back( e );

    for ( size_t iE = 0; iE < closeEdges.size(); ++iE )
      if ( be->myCloseBorders != closeEdges[iE]->myCloseBorders )
      {
        closeBord = closeEdges[iE]->myCloseBorders.begin();
        for ( ; closeBord != closeEdges[iE]->myCloseBorders.end(); ++closeBord )
          if ( !be->myCloseBorders.count( *closeBord ))
            if ( BEdge* e = closeEdges[iE]->myBNode2->GetCloseEdgeOfBorder( *closeBord ))
              if ( std::find( closeEdges.begin(), closeEdges.end(), e ) == closeEdges.end() )
                closeEdges.push_back( e );
      }

    // add parts of other borders

    BEdge* be1st = beRange[0];
    for ( size_t iE = 0; iE < closeEdges.size(); ++iE )
    {
      be = closeEdges[ iE ];
      if ( !be ) continue;

      bool ok = be->GetRangeOfSameCloseBorders( beRange, be->myCloseBorders );
      // if ( !ok && be->myPrev )
      //   ok = be->myPrev->GetRangeOfSameCloseBorders( beRange, be1st->myCloseBorders );
      // if ( !ok && be->myNext )
      //   ok = be->myNext->GetRangeOfSameCloseBorders( beRange, be1st->myCloseBorders );
      if ( !ok )
        continue;

      be = beRange[0];

      ranges.push_back( beRange[0] );
      ranges.push_back( beRange[1] );

      be->myInGroup = groupID;
      while ( be != beRange[1] )
      {
        be->myInGroup = groupID;
        be = be->myNext;
      }
      beRange[1]->myInGroup = groupID;
    }

    if ( ranges.size() > 2 )
    {
      if ( !chooseStartOfClosedBorders( ranges ))
        for ( size_t iR = 1; iR < ranges.size(); iR += 2 )
          extendPart( ranges[ iR-1 ], ranges[ iR ], be1st->myCloseBorders, groupID );

      // fill in a group
      beRange[0] = ranges[0];
      beRange[1] = ranges[1];

      part._border   = i;
      part._node1    = beRange[0]->myID;
      part._node2    = beRange[0]->myID + 1;
      part._nodeLast = beRange[1]->myID + 1;
      group.push_back( part );

      be1st = beRange[0];
      for ( size_t iR = 3; iR < ranges.size(); iR += 2 )
      {
        beRange[0] = ranges[iR-1];
        beRange[1] = ranges[iR-0];

        // find out mutual orientation of borders
        double u1, u2;
        be1st       ->IsOut( *beRange[ 0 ]->myBNode1, maxTolerance, u1 );
        beRange[ 0 ]->IsOut( *be1st->myBNode1,        maxTolerance, u2 );
        bool reverse = (( u1 < 0 || u1 > 1 ) && ( u2 < 0 || u2 > 1 ));

        // fill in a group
        part._border   = beRange[0]->myBorderID;
        if ( reverse ) {
          part._node1    = beRange[1]->myID + 1;
          part._node2    = beRange[1]->myID;
          part._nodeLast = beRange[0]->myID;
        }
        else  {
          part._node1    = beRange[0]->myID;
          part._node2    = beRange[0]->myID + 1;
          part._nodeLast = beRange[1]->myID + 1;
        }
        // if ( group[0]._node2 != part._node2 )
        group.push_back( part );
      }
      //if ( group.size() > 1 )
        foundFreeBordes._coincidentGroups.push_back( group );
    }
    else
    {
      beRange[0] = ranges[0];
      beRange[1] = ranges[1];

      be = beRange[0];
      be->myInGroup = skipGroup;
      while ( be != beRange[1] )
      {
        be->myInGroup = skipGroup;
        be = be->myNext;
      }
      beRange[1]->myInGroup = skipGroup;
    }

    be = ranges[1];

  } // loop on free borders

  return;

} // SMESH_MeshAlgos::FindCoincidentFreeBorders()

