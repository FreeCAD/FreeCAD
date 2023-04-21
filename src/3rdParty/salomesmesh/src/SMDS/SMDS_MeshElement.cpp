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

//  SMESH SMDS : implementation of Salome mesh data structure
//
#ifdef _MSC_VER
#pragma warning(disable:4786)
#endif

#include "SMDS_MeshElement.hxx"
#include "SMDS_MeshNode.hxx"
#include "SMDS_MeshEdge.hxx"
#include "SMDS_MeshFace.hxx"
#include "SMDS_MeshVolume.hxx"
#include "utilities.h"

using namespace std;

SMDS_MeshElement::SMDS_MeshElement(int ID)
{
  init(ID);
}

SMDS_MeshElement::SMDS_MeshElement(int id, ShortType meshId, LongType shapeId)
{
  init(id, meshId, shapeId);
}

void SMDS_MeshElement::init(int id, ShortType meshId, LongType shapeId )
{
  myID = id;
  myMeshId = meshId;
  myShapeId = shapeId;
  myIdInShape = -1;
}

void SMDS_MeshElement::Print(ostream & OS) const
{
        OS << "dump of mesh element" << endl;
}

ostream & operator <<(ostream & OS, const SMDS_MeshElement * ME)
{
        ME->Print(OS);
        return OS;
}

///////////////////////////////////////////////////////////////////////////////
/// Create an iterator which iterate on nodes owned by the element.
/// This method call elementsIterator().
///////////////////////////////////////////////////////////////////////////////
SMDS_ElemIteratorPtr SMDS_MeshElement::nodesIterator() const
{
        return elementsIterator(SMDSAbs_Node);
}

///////////////////////////////////////////////////////////////////////////////
/// Create an iterator which iterate on edges linked with or owned by the element.
/// This method call elementsIterator().
///////////////////////////////////////////////////////////////////////////////
SMDS_ElemIteratorPtr SMDS_MeshElement::edgesIterator() const
{
        return elementsIterator(SMDSAbs_Edge);
}

///////////////////////////////////////////////////////////////////////////////
/// Create an iterator which iterate on faces linked with or owned by the element.
/// This method call elementsIterator().
///////////////////////////////////////////////////////////////////////////////
SMDS_ElemIteratorPtr SMDS_MeshElement::facesIterator() const
{
        return elementsIterator(SMDSAbs_Face);
}

///////////////////////////////////////////////////////////////////////////////
///Return The number of nodes owned by the current element
///////////////////////////////////////////////////////////////////////////////
int SMDS_MeshElement::NbNodes() const
{
        int nbnodes=0;
        SMDS_ElemIteratorPtr it=nodesIterator();
        while(it->more())
        {
                it->next();
                nbnodes++;
        }
        return nbnodes;
}

///////////////////////////////////////////////////////////////////////////////
///Return the number of edges owned by or linked with the current element
///////////////////////////////////////////////////////////////////////////////
int SMDS_MeshElement::NbEdges() const
{
        int nbedges=0;
        SMDS_ElemIteratorPtr it=edgesIterator();
        while(it->more())
        {
                it->next();
                nbedges++;
        }
        return nbedges;
}

///////////////////////////////////////////////////////////////////////////////
///Return the number of faces owned by or linked with the current element
///////////////////////////////////////////////////////////////////////////////
int SMDS_MeshElement::NbFaces() const
{
        int nbfaces=0;
        SMDS_ElemIteratorPtr it=facesIterator();
        while(it->more())
        {
                it->next();
                nbfaces++;
        }
        return nbfaces;
}

///////////////////////////////////////////////////////////////////////////////
///Create an iterator which iterate on elements linked with the current element.
///@param type The of elements on which you want to iterate
///@return A smart pointer to iterator, you are not to take care of freeing memory
///////////////////////////////////////////////////////////////////////////////
class SMDS_MeshElement_MyIterator:public SMDS_ElemIterator
{
  const SMDS_MeshElement * myElement;
  bool myMore;
 public:
  SMDS_MeshElement_MyIterator(const SMDS_MeshElement * element):
    myElement(element),myMore(true) {}

  bool more()
  {
    return myMore;
  }

  const SMDS_MeshElement* next()
  {
    myMore=false;
    return myElement;   
  }     
};

SMDS_ElemIteratorPtr
SMDS_MeshElement::elementsIterator(SMDSAbs_ElementType type) const
{
  /** @todo Check that iterator in the child classes return elements
      in the same order for each different implementation (i.e: SMDS_VolumeOfNodes
      and SMDS_VolumeOfFaces */
  if(type==GetType())
    return SMDS_ElemIteratorPtr(new SMDS_MeshElement_MyIterator(this));
  else
  {
    MESSAGE("Iterator not implemented");
    return SMDS_ElemIteratorPtr((SMDS_ElemIterator*)NULL);
  }
}

//! virtual, redefined in vtkEdge, vtkFace and vtkVolume classes
SMDS_NodeIteratorPtr SMDS_MeshElement::nodesIteratorToUNV() const
{
  return nodeIterator();
}

//! virtual, redefined in vtkEdge, vtkFace and vtkVolume classes
SMDS_NodeIteratorPtr SMDS_MeshElement::interlacedNodesIterator() const
{
  return nodeIterator();
}

namespace
{
  //=======================================================================
  //class : _MyNodeIteratorFromElemIterator
  //=======================================================================
  class _MyNodeIteratorFromElemIterator : public SMDS_NodeIterator
  {
    SMDS_ElemIteratorPtr myItr;
  public:
    _MyNodeIteratorFromElemIterator(SMDS_ElemIteratorPtr elemItr):myItr( elemItr ) {}
    bool                 more() { return myItr->more(); }
    const SMDS_MeshNode* next() { return static_cast< const SMDS_MeshNode*>( myItr->next() ); }
  };
  //=======================================================================
  //class : _MyElemIteratorFromNodeIterator
  //=======================================================================
  class _MyElemIteratorFromNodeIterator : public SMDS_ElemIterator
  {
    SMDS_NodeIteratorPtr myItr;
  public:
    _MyElemIteratorFromNodeIterator(SMDS_NodeIteratorPtr nodeItr): myItr( nodeItr ) {}
    bool more()                    { return myItr->more(); }
    const SMDS_MeshElement* next() { return myItr->next(); }
  };
}

SMDS_ElemIteratorPtr SMDS_MeshElement::interlacedNodesElemIterator() const
{
  return SMDS_ElemIteratorPtr
    ( new _MyElemIteratorFromNodeIterator( interlacedNodesIterator() ));
}

SMDS_NodeIteratorPtr SMDS_MeshElement::nodeIterator() const
{
  return SMDS_NodeIteratorPtr
    ( new _MyNodeIteratorFromElemIterator( nodesIterator() ));
}

bool operator<(const SMDS_MeshElement& e1, const SMDS_MeshElement& e2)
{
        if(e1.GetType()!=e2.GetType()) return false;
        switch(e1.GetType())
        {
        case SMDSAbs_Node:
                return static_cast<const SMDS_MeshNode &>(e1) <
                        static_cast<const SMDS_MeshNode &>(e2);

        case SMDSAbs_Edge:
                return static_cast<const SMDS_MeshEdge &>(e1) <
                        static_cast<const SMDS_MeshEdge &>(e2);

        case SMDSAbs_Face:
                return static_cast<const SMDS_MeshFace &>(e1) <
                        static_cast<const SMDS_MeshFace &>(e2);

        case SMDSAbs_Volume:
                return static_cast<const SMDS_MeshVolume &>(e1) <
                        static_cast<const SMDS_MeshVolume &>(e2);

        default : MESSAGE("Internal Error");
        }
        return false;
}

bool SMDS_MeshElement::IsValidIndex(const int ind) const
{
  return ( ind>-1 && ind<NbNodes() );
}

const SMDS_MeshNode* SMDS_MeshElement::GetNode(const int ind) const
{
  if ( ind >= 0 ) {
    SMDS_ElemIteratorPtr it = nodesIterator();
    for ( int i = 0; i < ind; ++i )
      it->next();
    if ( it->more() )
      return static_cast<const SMDS_MeshNode*> (it->next());
  }
  return 0;
}

bool SMDS_MeshElement::IsQuadratic() const
{
  return false;
}

bool SMDS_MeshElement::IsMediumNode(const SMDS_MeshNode* node) const
{
  return false;
}

//================================================================================
/*!
 * \brief Return number of nodes excluding medium ones
 */
//================================================================================

int SMDS_MeshElement::NbCornerNodes() const
{
  return IsQuadratic() ? NbNodes() - NbEdges() : NbNodes();
}

//================================================================================
  /*!
   * \brief Check if a node belongs to the element
    * \param node - the node to check
    * \retval int - node index within the element, -1 if not found
   */
//================================================================================

int SMDS_MeshElement::GetNodeIndex( const SMDS_MeshNode* node ) const
{
  SMDS_ElemIteratorPtr nIt = nodesIterator();
  for ( int i = 0; nIt->more(); ++i )
    if ( nIt->next() == node )
      return i;
  return -1;
}
