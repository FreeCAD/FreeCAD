// Copyright (C) 2007-2015  CEA/DEN, EDF R&D, OPEN CASCADE
//
// Copyright (C) 2003-2007  OPEN CASCADE, EADS/CCR, LIP6, CEA/DEN,
// CEDRAT, EDF R&D, LEG, PRINCIPIA R&D, BUREAU VERITAS
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
// File      : SMESH_TypeDefs.hxx
// Created   : Thu Jan 27 18:38:33 2011
// Author    : Edward AGAPOV (eap)


#ifndef __SMESH_TypeDefs_HXX__
#define __SMESH_TypeDefs_HXX__

#include "SMESH_Utils.hxx"

#include "SMDS_MeshNode.hxx"

#include <gp_XYZ.hxx>
#include <gp_XY.hxx>

#include <map>
#include <list>
#include <set>
#include <cassert>

typedef std::map<const SMDS_MeshElement*,
                 std::list<const SMDS_MeshElement*>, TIDCompare > TElemOfElemListMap;
typedef std::map<const SMDS_MeshElement*,
                 std::list<const SMDS_MeshNode*>,    TIDCompare > TElemOfNodeListMap;
typedef std::map<const SMDS_MeshNode*,
                 const SMDS_MeshNode*,               TIDCompare>  TNodeNodeMap;

//!< Set of elements sorted by ID, to be used to assure predictability of edition
typedef std::set< const SMDS_MeshElement*, TIDCompare >      TIDSortedElemSet;
typedef std::set< const SMDS_MeshNode*,    TIDCompare >      TIDSortedNodeSet;

typedef std::pair< const SMDS_MeshNode*, const SMDS_MeshNode* >   NLink;

struct FaceQuadStruct; // defined in StdMeshers_Quadrangle_2D.hxx
typedef boost::shared_ptr<FaceQuadStruct> TFaceQuadStructPtr;


namespace SMESHUtils
{
  /*!
   * \brief Enforce freeing memory allocated by std::vector
   */
  template <class TVECTOR>
  void FreeVector(TVECTOR& vec)
  {
    TVECTOR v2;
    vec.swap( v2 );
  }
  template <class TVECTOR>
  void CompactVector(TVECTOR& vec)
  {
    TVECTOR v2( vec );
    vec.swap( v2 );
  }
  /*!
   * \brief Auto pointer
   */
  template <typename TOBJ>
  struct Deleter
  {
    TOBJ* _obj;
    Deleter( TOBJ* obj = (TOBJ*)NULL ): _obj( obj ) {}
    ~Deleter() { delete _obj; _obj = 0; }
  private:
    Deleter( const Deleter& );
  };
  /*!
   * \brief Auto pointer to array
   */
  template <typename TOBJ>
  struct ArrayDeleter
  {
    TOBJ* _obj;
    ArrayDeleter( TOBJ* obj ): _obj( obj ) {}
    ~ArrayDeleter() { delete [] _obj; _obj = 0; }
  private:
    ArrayDeleter( const ArrayDeleter& );
  };
}

//=======================================================================
/*!
 * \brief A sorted pair of nodes
 */
//=======================================================================

struct SMESH_TLink: public NLink
{
  SMESH_TLink(const SMDS_MeshNode* n1, const SMDS_MeshNode* n2 ):NLink( n1, n2 )
  { if ( n1->GetID() < n2->GetID() ) std::swap( first, second ); }
  SMESH_TLink(const NLink& link ):NLink( link )
  { if ( first->GetID() < second->GetID() ) std::swap( first, second ); }
  const SMDS_MeshNode* node1() const { return first; }
  const SMDS_MeshNode* node2() const { return second; }
};

//=======================================================================
/*!
 * \brief SMESH_TLink knowing its orientation
 */
//=======================================================================

struct SMESH_OrientedLink: public SMESH_TLink
{
  bool _reversed;
  SMESH_OrientedLink(const SMDS_MeshNode* n1, const SMDS_MeshNode* n2 )
    : SMESH_TLink( n1, n2 ), _reversed( n1 != node1() ) {}
};

//------------------------------------------
/*!
 * \brief SMDS_MeshNode -> gp_XYZ converter
 */
//------------------------------------------
struct SMESH_TNodeXYZ : public gp_XYZ
{
  const SMDS_MeshNode* _node;
  double               _xyz[3];
  SMESH_TNodeXYZ( const SMDS_MeshElement* e=0):gp_XYZ(0,0,0),_node(0)
  {
    Set(e);
  }
  bool Set( const SMDS_MeshElement* e=0 )
  {
    if (e) {
      assert( e->GetType() == SMDSAbs_Node );
      _node = static_cast<const SMDS_MeshNode*>(e);
      _node->GetXYZ(_xyz); // - thread safe getting coords
      SetCoord( _xyz[0], _xyz[1], _xyz[2] );
      return true;
    }
    return false;
  }
  double Distance(const SMDS_MeshNode* n)       const { return (SMESH_TNodeXYZ( n )-*this).Modulus(); }
  double SquareDistance(const SMDS_MeshNode* n) const { return (SMESH_TNodeXYZ( n )-*this).SquareModulus(); }
  bool operator==(const SMESH_TNodeXYZ& other) const { return _node == other._node; }
};

//--------------------------------------------------
/*!
 * \brief Data of a node generated on FACE boundary
 */
//--------------------------------------------------
typedef struct uvPtStruct
{
  double param;
  double normParam;
  double u, v; // original 2d parameter
  double x, y; // 2d parameter, normalized [0,1]
  const SMDS_MeshNode * node;

  uvPtStruct(): node(NULL) {}

  inline gp_XY UV() const { return gp_XY( u, v ); }

  struct NodeAccessor // accessor to iterate on nodes in UVPtStructVec
  {
    static const SMDS_MeshNode* value(std::vector< uvPtStruct >::const_iterator it)
    { return it->node; }
  };
} UVPtStruct;

typedef std::vector< UVPtStruct > UVPtStructVec;

// --------------------------------------------------------------------------------
// class SMESH_SequenceOfElemPtr
#include <NCollection_DefineSequence.hxx>

class SMDS_MeshElement;

typedef const SMDS_MeshElement* SMDS_MeshElementPtr;

DEFINE_SEQUENCE (SMESH_SequenceOfElemPtr, SMESH_BaseCollectionElemPtr, SMDS_MeshElementPtr)


// --------------------------------------------------------------------------------
// class SMESH_SequenceOfNode
typedef const SMDS_MeshNode* SMDS_MeshNodePtr;

DEFINE_SEQUENCE(SMESH_SequenceOfNode,
                SMESH_BaseCollectionNodePtr, SMDS_MeshNodePtr)

#endif
