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
//  SMESH SMESHDS : management of mesh data and SMESH document
//  File   : SMESH_SubMesh.cxx
//  Author : Yves FRICAUD, OCC
//  Module : SMESH
//  $Header: 
//
#include "SMESHDS_SubMesh.hxx"

#include "utilities.h"
#include "SMDS_SetIterator.hxx"

using namespace std;

//=======================================================================
//function : AddElement
//purpose  : 
//=======================================================================
void SMESHDS_SubMesh::AddElement(const SMDS_MeshElement * ME)
{
  if ( !IsComplexSubmesh() )
    myElements.insert(ME);
}

//=======================================================================
//function : RemoveElement
//purpose  : 
//=======================================================================
bool SMESHDS_SubMesh::RemoveElement(const SMDS_MeshElement * ME, bool isElemDeleted)
{
  if ( !IsComplexSubmesh() && NbElements() ) {

    if (!isElemDeleted) // alive element has valid ID and can be found
      return myElements.erase(ME);

    TElemSet::iterator e = myElements.begin(), eEnd = myElements.end();
    for ( ; e != eEnd; ++e )
      if ( ME == *e ) {
        myElements.erase( e );
        return true;
      }
  }
  
  return false;
}

//=======================================================================
//function : AddNode
//purpose  : 
//=======================================================================
void SMESHDS_SubMesh::AddNode(const SMDS_MeshNode * N)
{
  if ( !IsComplexSubmesh() )
    myNodes.insert(N);
}

//=======================================================================
//function : RemoveNode
//purpose  : 
//=======================================================================

bool SMESHDS_SubMesh::RemoveNode(const SMDS_MeshNode * N, bool isNodeDeleted)
{
  if ( !IsComplexSubmesh() && NbNodes() ) {

    if (!isNodeDeleted) // alive node has valid ID and can be found
      return myNodes.erase(N);

    TElemSet::iterator e = myNodes.begin(), eEnd = myNodes.end();
    for ( ; e != eEnd; ++e )
      if ( N == *e ) {
        myNodes.erase( e );
        return true;
      }
  }

  return false;
}

//=======================================================================
//function : NbElements
//purpose  : 
//=======================================================================
int SMESHDS_SubMesh::NbElements() const
{
  if ( !IsComplexSubmesh() )
    return myElements.size();

  int nbElems = 0;
  set<const SMESHDS_SubMesh*>::const_iterator it = mySubMeshes.begin();
  for ( ; it != mySubMeshes.end(); it++ )
    nbElems += (*it)->NbElements();

  return nbElems;
}

//=======================================================================
//function : NbNodes
//purpose  : 
//=======================================================================

int SMESHDS_SubMesh::NbNodes() const
{
 if ( !IsComplexSubmesh() )
   return myNodes.size(); 

  int nbElems = 0;
  set<const SMESHDS_SubMesh*>::const_iterator it = mySubMeshes.begin();
  for ( ; it != mySubMeshes.end(); it++ )
    nbElems += (*it)->NbNodes();

  return nbElems;
}

// =====================
// class MySetIterator
// =====================

template<class ELEM, typename TSET> class MySetIterator:
  public SMDS_SetIterator<ELEM, typename TSET::const_iterator >
{
  typedef SMDS_SetIterator<ELEM, typename TSET::const_iterator > TFather;
  public:
	MySetIterator(const TSET& s):TFather(s.begin(),s.end())
	{
	}
};

// =====================
// class MyIterator
// =====================

template<typename VALUE> class MyIterator : public SMDS_Iterator<VALUE>
{
 public:
  MyIterator (const set<const SMESHDS_SubMesh*>& theSubMeshes)
    : mySubIt( theSubMeshes.begin() ), mySubEnd( theSubMeshes.end() ), myMore(false)
    {}
  bool more()
  {
    while (( !myElemIt.get() || !myElemIt->more() ) && mySubIt != mySubEnd)
    {
      myElemIt = getElements(*mySubIt);
      mySubIt++;
    }
    myMore = myElemIt.get() && myElemIt->more();
    return myMore;
  }
  VALUE next()
  {
    VALUE elem = 0;
    if ( myMore )
      elem = myElemIt->next();
    return elem;
  }
 protected:
  virtual boost::shared_ptr< SMDS_Iterator<VALUE> >
    getElements(const SMESHDS_SubMesh*) const = 0;

 private:
  bool                                        myMore;
  set<const SMESHDS_SubMesh*>::const_iterator mySubIt, mySubEnd;
  boost::shared_ptr< SMDS_Iterator<VALUE> >   myElemIt;
};

// =====================
// class MyElemIterator
// =====================

class MyElemIterator: public MyIterator<const SMDS_MeshElement*>
{
 public:
  MyElemIterator (const set<const SMESHDS_SubMesh*>& theSubMeshes)
    :MyIterator<const SMDS_MeshElement*>( theSubMeshes ) {}
  SMDS_ElemIteratorPtr getElements(const SMESHDS_SubMesh* theSubMesh) const
  { return theSubMesh->GetElements(); }
};

// =====================
// class MyNodeIterator
// =====================

class MyNodeIterator: public MyIterator<const SMDS_MeshNode*>
{
 public:
  MyNodeIterator (const set<const SMESHDS_SubMesh*>& theSubMeshes)
    :MyIterator<const SMDS_MeshNode*>( theSubMeshes ) {}
  SMDS_NodeIteratorPtr getElements(const SMESHDS_SubMesh* theSubMesh) const
  { return theSubMesh->GetNodes(); }
};
  
//=======================================================================
//function : GetElements
//purpose  : 
//=======================================================================

SMDS_ElemIteratorPtr SMESHDS_SubMesh::GetElements() const
{
  if ( IsComplexSubmesh() )
    return SMDS_ElemIteratorPtr( new MyElemIterator( mySubMeshes ));

  return SMDS_ElemIteratorPtr(new MySetIterator<const SMDS_MeshElement*,TElemSet>(myElements));
}

//=======================================================================
//function : GetNodes
//purpose  : 
//=======================================================================

SMDS_NodeIteratorPtr SMESHDS_SubMesh::GetNodes() const
{
  if ( IsComplexSubmesh() )
    return SMDS_NodeIteratorPtr( new MyNodeIterator( mySubMeshes ));

  return SMDS_NodeIteratorPtr(new MySetIterator<const SMDS_MeshNode*,TElemSet>(myNodes));
}

//=======================================================================
//function : Contains
//purpose  : check if elem or node is in
//=======================================================================

bool SMESHDS_SubMesh::Contains(const SMDS_MeshElement * ME) const
{
  // DO NOT TRY TO FIND A REMOVED ELEMENT !!
  //if ( IsComplexSubmesh() || !ME )
  if (!ME )
    return false;

  if ( IsComplexSubmesh() )
  {
    set<const SMESHDS_SubMesh*>::const_iterator aSubIt = mySubMeshes.begin();
    for ( ; aSubIt != mySubMeshes.end(); aSubIt++ )
      if ( (*aSubIt)->Contains( ME ))
        return true;
    return false;
  }

  if ( ME->GetType() == SMDSAbs_Node )
    return ( myNodes.find( ME ) != myNodes.end() );

  return ( myElements.find( ME ) != myElements.end() );
}

//=======================================================================
//function : AddSubMesh
//purpose  : 
//=======================================================================

void SMESHDS_SubMesh::AddSubMesh( const SMESHDS_SubMesh* theSubMesh )
{
  ASSERT( theSubMesh );
  mySubMeshes.insert( theSubMesh );
}

//=======================================================================
//function : RemoveSubMesh
//purpose  : 
//=======================================================================

bool SMESHDS_SubMesh::RemoveSubMesh( const SMESHDS_SubMesh* theSubMesh )
{
  return mySubMeshes.erase( theSubMesh );
}

//=======================================================================
//function : ContainsSubMesh
//purpose  : 
//=======================================================================

bool SMESHDS_SubMesh::ContainsSubMesh( const SMESHDS_SubMesh* theSubMesh ) const
{
  return mySubMeshes.find( theSubMesh ) != mySubMeshes.end();
}

//=======================================================================
//function : GetSubMeshIterator
//purpose  : 
//=======================================================================

SMESHDS_SubMeshIteratorPtr SMESHDS_SubMesh::GetSubMeshIterator() const
{
  typedef set<const SMESHDS_SubMesh*>::const_iterator TIterator;
  return SMESHDS_SubMeshIteratorPtr
    ( new SMDS_SetIterator< const SMESHDS_SubMesh*, TIterator >( mySubMeshes.begin(),
                                                                 mySubMeshes.end()));
}

//=======================================================================
//function : Clear
//purpose  : remove the contents
//=======================================================================

void SMESHDS_SubMesh::Clear()
{
  myElements.clear();
  myNodes.clear();
  SMESHDS_SubMeshIteratorPtr sub = GetSubMeshIterator();
  while ( sub->more() ) {
    if ( SMESHDS_SubMesh* sm = (SMESHDS_SubMesh*) sub->next())
      sm->Clear();
  }
}
