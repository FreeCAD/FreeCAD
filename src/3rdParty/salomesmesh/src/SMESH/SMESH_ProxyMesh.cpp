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
// File      : SMESH_ProxyMesh.cxx
// Created   : Thu Dec  2 12:32:53 2010
// Author    : Edward AGAPOV (eap)

#include "SMESH_ProxyMesh.hxx"

#include "SMDS_IteratorOnIterators.hxx"
#include "SMDS_SetIterator.hxx"
#include "SMESH_MesherHelper.hxx"

#include <TopTools_ListIteratorOfListOfShape.hxx>
#include <TopExp.hxx>
#include <TopTools_IndexedMapOfShape.hxx>

//================================================================================
/*!
 * \brief Constructor; mesh must be set by a descendant class
 */
//================================================================================

SMESH_ProxyMesh::SMESH_ProxyMesh():_mesh(0)
{
}
//================================================================================
/*!
 * \brief Make a proxy mesh from components. Components become empty
 */
//================================================================================

SMESH_ProxyMesh::SMESH_ProxyMesh(std::vector<SMESH_ProxyMesh::Ptr>& components):
  _mesh(0)
{
  if ( components.empty() ) return;

  for ( unsigned i = 0; i < components.size(); ++i )
  {
    SMESH_ProxyMesh* m = components[i].get();
    if ( !m ) continue;

    takeTmpElemsInMesh( m );

    if ( !_mesh ) _mesh = m->_mesh;
    if ( _allowedTypes.empty() ) _allowedTypes = m->_allowedTypes;

    if ( _subMeshes.size() < m->_subMeshes.size() )
      _subMeshes.resize( m->_subMeshes.size(), 0 );
    for ( unsigned j = 0; j < m->_subMeshes.size(); ++j )
    {
      if ( !m->_subMeshes[j] ) continue;
      if ( _subMeshes[j] )
      {
        // unite 2 sub-meshes
        std::set < const SMDS_MeshElement* > elems( _subMeshes[j]->_elements.begin(),
                                               _subMeshes[j]->_elements.end());
        elems.insert( m->_subMeshes[j]->_elements.begin(),
                      m->_subMeshes[j]->_elements.end());
        _subMeshes[j]->_elements.assign( elems.begin(), elems.end() );
        m->_subMeshes[j]->_elements.clear();

        if ( !_subMeshes[j]->_n2n )
          _subMeshes[j]->_n2n = m->_subMeshes[j]->_n2n, m->_subMeshes[j]->_n2n = 0;

        else if ( _subMeshes[j]->_n2n && m->_subMeshes[j]->_n2n )
          _subMeshes[j]->_n2n->insert( m->_subMeshes[j]->_n2n->begin(),
                                       m->_subMeshes[j]->_n2n->end());
      }
      else
      {
        _subMeshes[j] = m->_subMeshes[j];
        m->_subMeshes[j] = 0;
      }
    }
  }
}

//================================================================================
/*!
 * \brief Destructor deletes proxy submeshes and tmp elemens
 */
//================================================================================

SMESH_ProxyMesh::~SMESH_ProxyMesh()
{
  for ( unsigned i = 0; i < _subMeshes.size(); ++i )
    delete _subMeshes[i];
  _subMeshes.clear();

  std::set < const SMDS_MeshElement* >::iterator i = _elemsInMesh.begin();
  for ( ; i != _elemsInMesh.end(); ++i )
    GetMeshDS()->RemoveFreeElement( *i, 0 );
  _elemsInMesh.clear();
}

//================================================================================
/*!
 * \brief Returns index of a shape
 */
//================================================================================

int SMESH_ProxyMesh::shapeIndex(const TopoDS_Shape& shape) const
{
  return ( shape.IsNull() || !_mesh->HasShapeToMesh() ? 0 : GetMeshDS()->ShapeToIndex(shape));
}

//================================================================================
/*!
 * \brief Returns the submesh of a shape; it can be a proxy sub-mesh
 */
//================================================================================

const SMESHDS_SubMesh* SMESH_ProxyMesh::GetSubMesh(const TopoDS_Shape& shape) const
{
  const SMESHDS_SubMesh* sm = 0;

  int i = shapeIndex(shape);
  if ( i < _subMeshes.size() )
    sm = _subMeshes[i];
  if ( !sm )
    sm = GetMeshDS()->MeshElements( i );

  return sm;
}

//================================================================================
/*!
 * \brief Returns the proxy sub-mesh of a shape; it can be NULL
 */
//================================================================================

const SMESH_ProxyMesh::SubMesh*
SMESH_ProxyMesh::GetProxySubMesh(const TopoDS_Shape& shape) const
{
  int i = shapeIndex(shape);
  return i < _subMeshes.size() ? _subMeshes[i] : 0;
}

//================================================================================
/*!
 * \brief Returns the proxy node of a node; the input node is returned if no proxy exists
 */
//================================================================================

const SMDS_MeshNode* SMESH_ProxyMesh::GetProxyNode( const SMDS_MeshNode* node ) const
{
  const SMDS_MeshNode* proxy = node;
  if ( node->GetPosition()->GetTypeOfPosition() == SMDS_TOP_FACE )
  {
    if ( const SubMesh* proxySM = findProxySubMesh( node->getshapeId() ))
      proxy = proxySM->GetProxyNode( node );
  }
  else
  {
    TopoDS_Shape shape = SMESH_MesherHelper::GetSubShapeByNode( node, GetMeshDS());
    TopTools_ListIteratorOfListOfShape ancIt;
    if ( !shape.IsNull() ) ancIt.Initialize( _mesh->GetAncestors( shape ));
    for ( ; ancIt.More() && proxy == node; ancIt.Next() )
      if ( const SubMesh* proxySM = findProxySubMesh( shapeIndex(ancIt.Value())))
        proxy = proxySM->GetProxyNode( node );
  }
  return proxy;
}

//================================================================================
/*!
 * \brief Returns number of proxy sub-meshes
 */
//================================================================================

int SMESH_ProxyMesh::NbProxySubMeshes() const
{
  int nb = 0;
  for ( size_t i = 0; i < _subMeshes.size(); ++i )
    nb += bool( _subMeshes[i] );

  return nb;
}

namespace
{
  //================================================================================
  /*!
   * \brief Iterator filtering elements by type
   */
  //================================================================================

  class TFilteringIterator : public SMDS_ElemIterator
  {
    SMDS_ElemIteratorPtr        _iter;
    const SMDS_MeshElement *    _curElem;
    std::vector< SMDSAbs_EntityType> _okTypes;
  public:
    TFilteringIterator( const std::vector< SMDSAbs_EntityType>& okTypes,
                        const SMDS_ElemIteratorPtr&        elemIterator)
      :_iter(elemIterator), _curElem(0), _okTypes(okTypes)
    {
      next();
    }
    virtual bool more()
    {
      return _curElem;
    }
    virtual const SMDS_MeshElement* next()
    {
      const SMDS_MeshElement* res = _curElem;
      _curElem = 0;
      while ( _iter->more() && !_curElem )
      {
        _curElem = _iter->next();
        if ( find( _okTypes.begin(), _okTypes.end(), _curElem->GetEntityType()) == _okTypes.end())
          _curElem = 0;
      }
      return res;
    }
  };
}

//================================================================================
/*!
 * \brief Returns iterator on all faces on the shape taking into account substitutions
 */
//================================================================================

SMDS_ElemIteratorPtr SMESH_ProxyMesh::GetFaces(const TopoDS_Shape& shape) const
{
  if ( !_mesh->HasShapeToMesh() )
    return SMDS_ElemIteratorPtr();

  _subContainer.RemoveAllSubmeshes();

  TopTools_IndexedMapOfShape FF;
  TopExp::MapShapes( shape, TopAbs_FACE, FF );
  for ( int i = 1; i <= FF.Extent(); ++i )
    if ( const SMESHDS_SubMesh* sm = GetSubMesh( FF(i)))
      _subContainer.AddSubMesh( sm );

  return _subContainer.SMESHDS_SubMesh::GetElements();
}

//================================================================================
/*!
 * \brief Returns iterator on all faces of the mesh taking into account substitutions
 * To be used in case of mesh without shape
 */
//================================================================================

SMDS_ElemIteratorPtr SMESH_ProxyMesh::GetFaces() const
{
  if ( _mesh->HasShapeToMesh() )
    return SMDS_ElemIteratorPtr();

  _subContainer.RemoveAllSubmeshes();
  for ( unsigned i = 0; i < _subMeshes.size(); ++i )
    if ( _subMeshes[i] )
      _subContainer.AddSubMesh( _subMeshes[i] );

  if ( _subContainer.NbSubMeshes() == 0 ) // no elements substituted
    return GetMeshDS()->elementsIterator(SMDSAbs_Face);

  // if _allowedTypes is empty, only elements from _subMeshes are returned,...
  SMDS_ElemIteratorPtr proxyIter = _subContainer.SMESHDS_SubMesh::GetElements();
  if ( _allowedTypes.empty() || NbFaces() == _mesh->NbFaces() )
    return proxyIter;

  // ... else elements filtered using allowedTypes are additionally returned
  SMDS_ElemIteratorPtr facesIter = GetMeshDS()->elementsIterator(SMDSAbs_Face);
  SMDS_ElemIteratorPtr filterIter( new TFilteringIterator( _allowedTypes, facesIter ));
  std::vector< SMDS_ElemIteratorPtr > iters(2);
  iters[0] = proxyIter;
  iters[1] = filterIter;
    
  typedef std::vector< SMDS_ElemIteratorPtr > TElemIterVector;
  typedef SMDS_IteratorOnIterators<const SMDS_MeshElement *, TElemIterVector> TItersIter;
  return SMDS_ElemIteratorPtr( new TItersIter( iters ));
}

//================================================================================
/*!
 * \brief Return total nb of faces taking into account substitutions
 */
//================================================================================

int SMESH_ProxyMesh::NbFaces() const
{
  int nb = 0;
  if ( _mesh->HasShapeToMesh() )
  {
    TopTools_IndexedMapOfShape FF;
    TopExp::MapShapes( _mesh->GetShapeToMesh(), TopAbs_FACE, FF );
    for ( int i = 1; i <= FF.Extent(); ++i )
      if ( const SMESHDS_SubMesh* sm = GetSubMesh( FF(i)))
        nb += sm->NbElements();
  }
  else
  {
    if ( _subMeshes.empty() )
      return GetMeshDS()->NbFaces();

    for ( unsigned i = 0; i < _subMeshes.size(); ++i )
      if ( _subMeshes[i] )
        nb += _subMeshes[i]->NbElements();

    // if _allowedTypes is empty, only elements from _subMeshes are returned,
    // else elements filtered using allowedTypes are additionally returned
    if ( !_allowedTypes.empty() )
    {
      for ( int t = SMDSEntity_Triangle; t <= SMDSEntity_Quad_Quadrangle; ++t )
      {
        bool allowed =
          ( find( _allowedTypes.begin(), _allowedTypes.end(), t ) != _allowedTypes.end() );
        if ( allowed )
          nb += GetMeshDS()->GetMeshInfo().NbEntities( SMDSAbs_EntityType( t ));
      }
    }
  }
  return nb;
}

//================================================================================
/*!
 * \brief Returns a proxy sub-mesh; it is created if not yet exists
 */
//================================================================================

SMESH_ProxyMesh::SubMesh* SMESH_ProxyMesh::getProxySubMesh(int index)
{
  if ( int(_subMeshes.size()) <= index )
    _subMeshes.resize( index+1, 0 );
  if ( !_subMeshes[index] )
    _subMeshes[index] = newSubmesh( index );
  return _subMeshes[index];
}

//================================================================================
/*!
 * \brief Returns a proxy sub-mesh; it is created if not yet exists
 */
//================================================================================

SMESH_ProxyMesh::SubMesh* SMESH_ProxyMesh::getProxySubMesh(const TopoDS_Shape& shape)
{
  return getProxySubMesh( shapeIndex( shape ));
}

//================================================================================
/*!
 * \brief Returns a proxy sub-mesh
 */
//================================================================================

SMESH_ProxyMesh::SubMesh* SMESH_ProxyMesh::findProxySubMesh(int shapeIndex) const
{
  return shapeIndex < int(_subMeshes.size()) ? _subMeshes[shapeIndex] : 0;
}

//================================================================================
/*!
 * \brief Returns mesh DS
 */
//================================================================================

SMESHDS_Mesh* SMESH_ProxyMesh::GetMeshDS() const
{
  return (SMESHDS_Mesh*)( _mesh ? _mesh->GetMeshDS() : 0 );
}

//================================================================================
/*!
 * \brief Move proxy sub-mesh from other proxy mesh to this, returns true if sub-mesh found
 */
//================================================================================

bool SMESH_ProxyMesh::takeProxySubMesh( const TopoDS_Shape&   shape,
                                             SMESH_ProxyMesh* proxyMesh )
{
  if ( proxyMesh && proxyMesh->_mesh == _mesh )
  {
    int iS = shapeIndex( shape );
    if ( SubMesh* sm = proxyMesh->findProxySubMesh( iS ))
    {
      if ( iS >= int(_subMeshes.size()) )
        _subMeshes.resize( iS + 1, 0 );
      _subMeshes[iS] = sm;
      proxyMesh->_subMeshes[iS] = 0;
      return true;
    }
  }
  return false;
}

//================================================================================
/*!
 * \brief Move tmp elements residing the _mesh from other proxy mesh to this
 */
//================================================================================

void SMESH_ProxyMesh::takeTmpElemsInMesh( SMESH_ProxyMesh* proxyMesh )
{
  if ( proxyMesh )
  {
    _elemsInMesh.insert( proxyMesh->_elemsInMesh.begin(),
                         proxyMesh->_elemsInMesh.end());
    proxyMesh->_elemsInMesh.clear();
  }
}

//================================================================================
/*!
 * \brief Removes tmp elements from the _mesh
 */
//================================================================================

void SMESH_ProxyMesh::removeTmpElement( const SMDS_MeshElement* elem )
{
  if ( elem && elem->GetID() > 0 )
  {
    std::set < const SMDS_MeshElement* >::iterator i =  _elemsInMesh.find( elem );
    if ( i != _elemsInMesh.end() )
    {
      GetMeshDS()->RemoveFreeElement( elem, 0 );
      _elemsInMesh.erase( i );
    }
  }
  else
  {
    delete elem;
  }
}

//================================================================================
/*!
 * \brief Stores tmp element residing the _mesh
 */
//================================================================================

void SMESH_ProxyMesh::storeTmpElement( const SMDS_MeshElement* elem )
{
  _elemsInMesh.insert( elem );
}

//================================================================================
/*!
 * \brief Set node-node correspondence
 */
//================================================================================

void SMESH_ProxyMesh::setNode2Node(const SMDS_MeshNode* srcNode,
                                   const SMDS_MeshNode* proxyNode,
                                   const SubMesh*       subMesh)
{
  SubMesh* sm = const_cast<SubMesh*>( subMesh );
  if ( !subMesh->_n2n )
    sm->_n2n = new TN2NMap;
  sm->_n2n->insert( std::make_pair( srcNode, proxyNode ));
}

//================================================================================
/*!
 * \brief Return true if the element is a temporary one
 */
//================================================================================

bool SMESH_ProxyMesh::IsTemporary(const SMDS_MeshElement* elem ) const
{
  return ( elem->GetID() < 1 ) || _elemsInMesh.count( elem );
}

//================================================================================
/*!
 * \brief Return a proxy node or an input node
 */
//================================================================================

const SMDS_MeshNode* SMESH_ProxyMesh::SubMesh::GetProxyNode( const SMDS_MeshNode* n ) const
{
  TN2NMap::iterator n2n;
  if ( _n2n && ( n2n = _n2n->find( n )) != _n2n->end())
    return n2n->second;
  return n;
}

//================================================================================
/*!
 * \brief Deletes temporary elements
 */
//================================================================================

void SMESH_ProxyMesh::SubMesh::Clear()
{
  for ( unsigned i = 0; i < _elements.size(); ++i )
    if ( _elements[i]->GetID() < 0 )
      delete _elements[i];
  _elements.clear();
  if ( _n2n )
    delete _n2n, _n2n = 0;
}

//================================================================================
/*!
 * \brief Return number of elements in a proxy sub-mesh. The method is meaningful
 *        for a sub-mesh containing tmp faces.
 */
//================================================================================

int SMESH_ProxyMesh::SubMesh::NbElements() const
{
  return _uvPtStructVec.empty() ? _elements.size() : _uvPtStructVec.size() - 1;
}

//================================================================================
/*!
 * \brief Return elements of a proxy sub-mesh. The method is meaningful
 *        for a sub-mesh containing tmp faces.
 */
//================================================================================

SMDS_ElemIteratorPtr SMESH_ProxyMesh::SubMesh::GetElements() const
{
  return SMDS_ElemIteratorPtr
    ( new SMDS_ElementVectorIterator( _elements.begin(), _elements.end() ));
}

//================================================================================
/*!
 * \brief Return number of nodes in a proxy sub-mesh. The method is meaningful
 *        for a sub-mesh containing nodes of 2D viscous layer.
 */
//================================================================================

int SMESH_ProxyMesh::SubMesh::NbNodes() const
{
  return _uvPtStructVec.size();
}

//================================================================================
/*!
 * \brief Return nodes of a proxy sub-mesh. The method is meaningful
 *        for a sub-mesh containing nodes of 2D viscous layer.
 */
//================================================================================

SMDS_NodeIteratorPtr SMESH_ProxyMesh::SubMesh::GetNodes() const
{
  if ( !_uvPtStructVec.empty() )
    return SMDS_NodeIteratorPtr ( new SMDS_SetIterator
                                  < SMDS_pNode,
                                  UVPtStructVec::const_iterator,
                                  UVPtStruct::NodeAccessor >
                                  ( _uvPtStructVec.begin(), _uvPtStructVec.end() ));

  return SMDS_NodeIteratorPtr
    ( new SMDS_SetIterator< SMDS_pNode, std::vector< SMDS_pElement >::const_iterator>
      ( _elements.begin(), _elements.end() ));
}

//================================================================================
/*!
 * \brief Store an element
 */
//================================================================================

void SMESH_ProxyMesh::SubMesh::AddElement(const SMDS_MeshElement * e)
{
  _elements.push_back( e );
}

//================================================================================
/*!
 * \brief Check presence of element inside it-self
 */
//================================================================================

bool SMESH_ProxyMesh::SubMesh::Contains(const SMDS_MeshElement * ME) const
{
  if ( ME->GetType() != SMDSAbs_Node )
    return find( _elements.begin(), _elements.end(), ME ) != _elements.end();
  return false;
}
