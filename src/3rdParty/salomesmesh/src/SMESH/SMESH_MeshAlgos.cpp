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
// File      : SMESH_MeshAlgos.hxx
// Created   : Tue Apr 30 18:00:36 2013
// Author    : Edward AGAPOV (eap)

// This file holds some low level algorithms extracted from SMESH_MeshEditor
// to make them accessible from Controls package

#include "SMESH_MeshAlgos.hxx"

#include "SMDS_FaceOfNodes.hxx"
#include "SMDS_LinearEdge.hxx"
#include "SMDS_Mesh.hxx"
#include "SMDS_PolygonalFaceOfNodes.hxx"
#include "SMDS_VolumeTool.hxx"
#include "SMESH_OctreeNode.hxx"

#include <GC_MakeSegment.hxx>
#include <GeomAPI_ExtremaCurveCurve.hxx>
#include <Geom_Line.hxx>
#include <gp_Cone.hxx>
#include <gp_Cylinder.hxx>
#include <gp_Lin.hxx>
#include <gp_Pln.hxx>
#include <gp_Sphere.hxx>
#include <IntAna_IntConicQuad.hxx>
#include <IntAna_Quadric.hxx>

#include <limits>
#include <numeric>

using namespace std;

//=======================================================================
/*!
 * \brief Implementation of search for the node closest to point
 */
//=======================================================================

struct SMESH_NodeSearcherImpl: public SMESH_NodeSearcher
{
  //---------------------------------------------------------------------
  /*!
   * \brief Constructor
   */
  SMESH_NodeSearcherImpl( const SMDS_Mesh* theMesh )
  {
    myMesh = ( SMDS_Mesh* ) theMesh;

    TIDSortedNodeSet nodes;
    if ( theMesh ) {
      SMDS_NodeIteratorPtr nIt = theMesh->nodesIterator(/*idInceasingOrder=*/true);
      while ( nIt->more() )
        nodes.insert( nodes.end(), nIt->next() );
    }
    myOctreeNode = new SMESH_OctreeNode(nodes) ;

    // get max size of a leaf box
    SMESH_OctreeNode* tree = myOctreeNode;
    while ( !tree->isLeaf() )
    {
      SMESH_OctreeNodeIteratorPtr cIt = tree->GetChildrenIterator();
      if ( cIt->more() )
        tree = cIt->next();
    }
    myHalfLeafSize = tree->maxSize() / 2.;
  }

  //---------------------------------------------------------------------
  /*!
   * \brief Move node and update myOctreeNode accordingly
   */
  void MoveNode( const SMDS_MeshNode* node, const gp_Pnt& toPnt )
  {
    myOctreeNode->UpdateByMoveNode( node, toPnt );
    myMesh->MoveNode( node, toPnt.X(), toPnt.Y(), toPnt.Z() );
  }

  //---------------------------------------------------------------------
  /*!
   * \brief Do it's job
   */
  const SMDS_MeshNode* FindClosestTo( const gp_Pnt& thePnt )
  {
    map<double, const SMDS_MeshNode*> dist2Nodes;
    myOctreeNode->NodesAround( thePnt.Coord(), dist2Nodes, myHalfLeafSize );
    if ( !dist2Nodes.empty() )
      return dist2Nodes.begin()->second;
    list<const SMDS_MeshNode*> nodes;
    //myOctreeNode->NodesAround( &tgtNode, &nodes, myHalfLeafSize );

    double minSqDist = DBL_MAX;
    if ( nodes.empty() )  // get all nodes of OctreeNode's closest to thePnt
    {
      // sort leafs by their distance from thePnt
      typedef map< double, SMESH_OctreeNode* > TDistTreeMap;
      TDistTreeMap treeMap;
      list< SMESH_OctreeNode* > treeList;
      list< SMESH_OctreeNode* >::iterator trIt;
      treeList.push_back( myOctreeNode );

      gp_XYZ pointNode( thePnt.X(), thePnt.Y(), thePnt.Z() );
      bool pointInside = myOctreeNode->isInside( pointNode, myHalfLeafSize );
      for ( trIt = treeList.begin(); trIt != treeList.end(); ++trIt)
      {
        SMESH_OctreeNode* tree = *trIt;
        if ( !tree->isLeaf() ) // put children to the queue
        {
          if ( pointInside && !tree->isInside( pointNode, myHalfLeafSize )) continue;
          SMESH_OctreeNodeIteratorPtr cIt = tree->GetChildrenIterator();
          while ( cIt->more() )
            treeList.push_back( cIt->next() );
        }
        else if ( tree->NbNodes() ) // put a tree to the treeMap
        {
          const Bnd_B3d& box = *tree->getBox();
          double sqDist = thePnt.SquareDistance( 0.5 * ( box.CornerMin() + box.CornerMax() ));
          pair<TDistTreeMap::iterator,bool> it_in = treeMap.insert( make_pair( sqDist, tree ));
          if ( !it_in.second ) // not unique distance to box center
            treeMap.insert( it_in.first, make_pair( sqDist + 1e-13*treeMap.size(), tree ));
        }
      }
      // find distance after which there is no sense to check tree's
      double sqLimit = DBL_MAX;
      TDistTreeMap::iterator sqDist_tree = treeMap.begin();
      if ( treeMap.size() > 5 ) {
        SMESH_OctreeNode* closestTree = sqDist_tree->second;
        const Bnd_B3d& box = *closestTree->getBox();
        double limit = sqrt( sqDist_tree->first ) + sqrt ( box.SquareExtent() );
        sqLimit = limit * limit;
      }
      // get all nodes from trees
      for ( ; sqDist_tree != treeMap.end(); ++sqDist_tree) {
        if ( sqDist_tree->first > sqLimit )
          break;
        SMESH_OctreeNode* tree = sqDist_tree->second;
        tree->NodesAround( tree->GetNodeIterator()->next(), &nodes );
      }
    }
    // find closest among nodes
    minSqDist = DBL_MAX;
    const SMDS_MeshNode* closestNode = 0;
    list<const SMDS_MeshNode*>::iterator nIt = nodes.begin();
    for ( ; nIt != nodes.end(); ++nIt ) {
      double sqDist = thePnt.SquareDistance( SMESH_TNodeXYZ( *nIt ) );
      if ( minSqDist > sqDist ) {
        closestNode = *nIt;
        minSqDist = sqDist;
      }
    }
    return closestNode;
  }

  //---------------------------------------------------------------------
  /*!
   * \brief Finds nodes located within a tolerance near a point 
   */
  int FindNearPoint(const gp_Pnt&                        point,
                    const double                         tolerance,
                    std::vector< const SMDS_MeshNode* >& foundNodes)
  {
    myOctreeNode->NodesAround( point.Coord(), foundNodes, tolerance );
    return foundNodes.size();
  }

  //---------------------------------------------------------------------
  /*!
   * \brief Destructor
   */
  ~SMESH_NodeSearcherImpl() { delete myOctreeNode; }

  //---------------------------------------------------------------------
  /*!
   * \brief Return the node tree
   */
  const SMESH_OctreeNode* getTree() const { return myOctreeNode; }

private:
  SMESH_OctreeNode* myOctreeNode;
  SMDS_Mesh*        myMesh;
  double            myHalfLeafSize; // max size of a leaf box
};

// ========================================================================
namespace // Utils used in SMESH_ElementSearcherImpl::FindElementsByPoint()
{
  const int MaxNbElemsInLeaf = 10; // maximal number of elements in a leaf of tree
  const int MaxLevel         = 7;  // maximal tree height -> nb terminal boxes: 8^7 = 2097152
  const double NodeRadius = 1e-9;  // to enlarge bnd box of element

  //=======================================================================
  /*!
   * \brief Octal tree of bounding boxes of elements
   */
  //=======================================================================

  class ElementBndBoxTree : public SMESH_Octree
  {
  public:

    ElementBndBoxTree(const SMDS_Mesh&     mesh,
                      SMDSAbs_ElementType  elemType,
                      SMDS_ElemIteratorPtr theElemIt = SMDS_ElemIteratorPtr(),
                      double               tolerance = NodeRadius );
    void getElementsNearPoint( const gp_Pnt& point, TIDSortedElemSet& foundElems );
    void getElementsNearLine ( const gp_Ax1& line, TIDSortedElemSet& foundElems);
    void getElementsInSphere ( const gp_XYZ& center,
                               const double  radius, TIDSortedElemSet& foundElems);
    size_t getSize() { return std::max( _size, _elements.size() ); }
    virtual ~ElementBndBoxTree();

  protected:
    ElementBndBoxTree():_size(0) {}
    SMESH_Octree* newChild() const { return new ElementBndBoxTree; }
    void          buildChildrenData();
    Bnd_B3d*      buildRootBox();
  private:
    //!< Bounding box of element
    struct ElementBox : public Bnd_B3d
    {
      const SMDS_MeshElement* _element;
      int                     _refCount; // an ElementBox can be included in several tree branches
      ElementBox(const SMDS_MeshElement* elem, double tolerance);
    };
    vector< ElementBox* > _elements;
    size_t                _size;
  };

  //================================================================================
  /*!
   * \brief ElementBndBoxTree creation
   */
  //================================================================================

  ElementBndBoxTree::ElementBndBoxTree(const SMDS_Mesh& mesh, SMDSAbs_ElementType elemType, SMDS_ElemIteratorPtr theElemIt, double tolerance)
    :SMESH_Octree( new SMESH_TreeLimit( MaxLevel, /*minSize=*/0. ))
  {
    int nbElems = mesh.GetMeshInfo().NbElements( elemType );
    _elements.reserve( nbElems );

    SMDS_ElemIteratorPtr elemIt = theElemIt ? theElemIt : mesh.elementsIterator( elemType );
    while ( elemIt->more() )
      _elements.push_back( new ElementBox( elemIt->next(),tolerance  ));

    compute();
  }

  //================================================================================
  /*!
   * \brief Destructor
   */
  //================================================================================

  ElementBndBoxTree::~ElementBndBoxTree()
  {
    for ( int i = 0; i < _elements.size(); ++i )
      if ( --_elements[i]->_refCount <= 0 )
        delete _elements[i];
  }

  //================================================================================
  /*!
   * \brief Return the maximal box
   */
  //================================================================================

  Bnd_B3d* ElementBndBoxTree::buildRootBox()
  {
    Bnd_B3d* box = new Bnd_B3d;
    for ( int i = 0; i < _elements.size(); ++i )
      box->Add( *_elements[i] );
    return box;
  }

  //================================================================================
  /*!
   * \brief Redistrubute element boxes among children
   */
  //================================================================================

  void ElementBndBoxTree::buildChildrenData()
  {
    for ( int i = 0; i < _elements.size(); ++i )
    {
      for (int j = 0; j < 8; j++)
      {
        if ( !_elements[i]->IsOut( *myChildren[j]->getBox() ))
        {
          _elements[i]->_refCount++;
          ((ElementBndBoxTree*)myChildren[j])->_elements.push_back( _elements[i]);
        }
      }
      _elements[i]->_refCount--;
    }
    _size = _elements.size();
    SMESHUtils::FreeVector( _elements ); // = _elements.clear() + free memory

    for (int j = 0; j < 8; j++)
    {
      ElementBndBoxTree* child = static_cast<ElementBndBoxTree*>( myChildren[j]);
      if ( child->_elements.size() <= MaxNbElemsInLeaf )
        child->myIsLeaf = true;

      if ( child->_elements.capacity() - child->_elements.size() > 1000 )
        SMESHUtils::CompactVector( child->_elements );
    }
  }

  //================================================================================
  /*!
   * \brief Return elements which can include the point
   */
  //================================================================================

  void ElementBndBoxTree::getElementsNearPoint( const gp_Pnt&     point,
                                                TIDSortedElemSet& foundElems)
  {
    if ( getBox()->IsOut( point.XYZ() ))
      return;

    if ( isLeaf() )
    {
      for ( int i = 0; i < _elements.size(); ++i )
        if ( !_elements[i]->IsOut( point.XYZ() ))
          foundElems.insert( _elements[i]->_element );
    }
    else
    {
      for (int i = 0; i < 8; i++)
        ((ElementBndBoxTree*) myChildren[i])->getElementsNearPoint( point, foundElems );
    }
  }

  //================================================================================
  /*!
   * \brief Return elements which can be intersected by the line
   */
  //================================================================================

  void ElementBndBoxTree::getElementsNearLine( const gp_Ax1&     line,
                                               TIDSortedElemSet& foundElems)
  {
    if ( getBox()->IsOut( line ))
      return;

    if ( isLeaf() )
    {
      for ( int i = 0; i < _elements.size(); ++i )
        if ( !_elements[i]->IsOut( line ))
          foundElems.insert( _elements[i]->_element );
    }
    else
    {
      for (int i = 0; i < 8; i++)
        ((ElementBndBoxTree*) myChildren[i])->getElementsNearLine( line, foundElems );
    }
  }

  //================================================================================
  /*!
   * \brief Return elements from leaves intersecting the sphere
   */
  //================================================================================

  void ElementBndBoxTree::getElementsInSphere ( const gp_XYZ&     center,
                                                const double      radius,
                                                TIDSortedElemSet& foundElems)
  {
    if ( getBox()->IsOut( center, radius ))
      return;

    if ( isLeaf() )
    {
      for ( int i = 0; i < _elements.size(); ++i )
        if ( !_elements[i]->IsOut( center, radius ))
          foundElems.insert( _elements[i]->_element );
    }
    else
    {
      for (int i = 0; i < 8; i++)
        ((ElementBndBoxTree*) myChildren[i])->getElementsInSphere( center, radius, foundElems );
    }
  }

  //================================================================================
  /*!
   * \brief Construct the element box
   */
  //================================================================================

  ElementBndBoxTree::ElementBox::ElementBox(const SMDS_MeshElement* elem, double tolerance)
  {
    _element  = elem;
    _refCount = 1;
    SMDS_ElemIteratorPtr nIt = elem->nodesIterator();
    while ( nIt->more() )
      Add( SMESH_TNodeXYZ( nIt->next() ));
    Enlarge( tolerance );
  }

} // namespace

//=======================================================================
/*!
 * \brief Implementation of search for the elements by point and
 *        of classification of point in 2D mesh
 */
//=======================================================================

SMESH_ElementSearcher::~SMESH_ElementSearcher()
{
}

struct SMESH_ElementSearcherImpl: public SMESH_ElementSearcher
{
  SMDS_Mesh*                   _mesh;
  SMDS_ElemIteratorPtr         _meshPartIt;
  ElementBndBoxTree*           _ebbTree;
  SMESH_NodeSearcherImpl*      _nodeSearcher;
  SMDSAbs_ElementType          _elementType;
  double                       _tolerance;
  bool                         _outerFacesFound;
  set<const SMDS_MeshElement*> _outerFaces; // empty means "no internal faces at all"

  SMESH_ElementSearcherImpl( SMDS_Mesh&           mesh,
                             double               tol=-1,
                             SMDS_ElemIteratorPtr elemIt=SMDS_ElemIteratorPtr())
    : _mesh(&mesh),_meshPartIt(elemIt),_ebbTree(0),_nodeSearcher(0),_tolerance(tol),_outerFacesFound(false) {}
  virtual ~SMESH_ElementSearcherImpl()
  {
      if ( _ebbTree )
          delete _ebbTree;

      _ebbTree = 0;

      if ( _nodeSearcher )
          delete _nodeSearcher;

      _nodeSearcher = 0;
  }
  virtual int FindElementsByPoint(const gp_Pnt&                      point,
                                  SMDSAbs_ElementType                type,
                                  vector< const SMDS_MeshElement* >& foundElements);
  virtual TopAbs_State GetPointState(const gp_Pnt& point);
  virtual const SMDS_MeshElement* FindClosestTo( const gp_Pnt&       point,
                                                 SMDSAbs_ElementType type );

  void GetElementsNearLine( const gp_Ax1&                      line,
                            SMDSAbs_ElementType                type,
                            vector< const SMDS_MeshElement* >& foundElems);
  double getTolerance();
  bool getIntersParamOnLine(const gp_Lin& line, const SMDS_MeshElement* face,
                            const double tolerance, double & param);
  void findOuterBoundary(const SMDS_MeshElement* anyOuterFace);
  bool isOuterBoundary(const SMDS_MeshElement* face) const
  {
    return _outerFaces.empty() || _outerFaces.count(face);
  }
  struct TInters //!< data of intersection of the line and the mesh face (used in GetPointState())
  {
    const SMDS_MeshElement* _face;
    gp_Vec                  _faceNorm;
    bool                    _coincides; //!< the line lays in face plane
    TInters(const SMDS_MeshElement* face, const gp_Vec& faceNorm, bool coinc=false)
      : _face(face), _faceNorm( faceNorm ), _coincides( coinc ) {}
  };
  struct TFaceLink //!< link and faces sharing it (used in findOuterBoundary())
  {
    SMESH_TLink      _link;
    TIDSortedElemSet _faces;
    TFaceLink( const SMDS_MeshNode* n1, const SMDS_MeshNode* n2, const SMDS_MeshElement* face)
      : _link( n1, n2 ), _faces( &face, &face + 1) {}
  };
};

ostream& operator<< (ostream& out, const SMESH_ElementSearcherImpl::TInters& i)
{
  return out << "TInters(face=" << ( i._face ? i._face->GetID() : 0)
             << ", _coincides="<<i._coincides << ")";
}

//=======================================================================
/*!
 * \brief define tolerance for search
 */
//=======================================================================

double SMESH_ElementSearcherImpl::getTolerance()
{
  if ( _tolerance < 0 )
  {
    const SMDS_MeshInfo& meshInfo = _mesh->GetMeshInfo();

    _tolerance = 0;
    if ( _nodeSearcher && meshInfo.NbNodes() > 1 )
    {
      double boxSize = _nodeSearcher->getTree()->maxSize();
      _tolerance = 1e-8 * boxSize/* / meshInfo.NbNodes()*/;
    }
    else if ( _ebbTree && meshInfo.NbElements() > 0 )
    {
      double boxSize = _ebbTree->maxSize();
      _tolerance = 1e-8 * boxSize/* / meshInfo.NbElements()*/;
    }
    if ( _tolerance == 0 )
    {
      // define tolerance by size of a most complex element
      int complexType = SMDSAbs_Volume;
      while ( complexType > SMDSAbs_All &&
              meshInfo.NbElements( SMDSAbs_ElementType( complexType )) < 1 )
        --complexType;
      if ( complexType == SMDSAbs_All ) return 0; // empty mesh
      double elemSize;
      if ( complexType == int( SMDSAbs_Node ))
      {
        SMDS_NodeIteratorPtr nodeIt = _mesh->nodesIterator();
        elemSize = 1;
        if ( meshInfo.NbNodes() > 2 )
          elemSize = SMESH_TNodeXYZ( nodeIt->next() ).Distance( nodeIt->next() );
      }
      else
      {
        SMDS_ElemIteratorPtr elemIt =
            _mesh->elementsIterator( SMDSAbs_ElementType( complexType ));
        const SMDS_MeshElement* elem = elemIt->next();
        SMDS_ElemIteratorPtr nodeIt = elem->nodesIterator();
        SMESH_TNodeXYZ n1( nodeIt->next() );
        elemSize = 0;
        while ( nodeIt->more() )
        {
          double dist = n1.Distance( static_cast<const SMDS_MeshNode*>( nodeIt->next() ));
          elemSize = max( dist, elemSize );
        }
      }
      _tolerance = 1e-4 * elemSize;
    }
  }
  return _tolerance;
}

//================================================================================
/*!
 * \brief Find intersection of the line and an edge of face and return parameter on line
 */
//================================================================================

bool SMESH_ElementSearcherImpl::getIntersParamOnLine(const gp_Lin&           line,
                                                     const SMDS_MeshElement* face,
                                                     const double            tol,
                                                     double &                param)
{
  int nbInts = 0;
  param = 0;

  GeomAPI_ExtremaCurveCurve anExtCC;
  Handle(Geom_Curve) lineCurve = new Geom_Line( line );

  int nbNodes = face->IsQuadratic() ? face->NbNodes()/2 : face->NbNodes();
  for ( int i = 0; i < nbNodes && nbInts < 2; ++i )
  {
    GC_MakeSegment edge( SMESH_TNodeXYZ( face->GetNode( i )),
                         SMESH_TNodeXYZ( face->GetNode( (i+1)%nbNodes) ));
// vejmarie ISSUE THERE
//    anExtCC.Init( lineCurve, edge);
    if ( anExtCC.NbExtrema() > 0 && anExtCC.LowerDistance() <= tol)
    {
      Standard_Real pl, pe;
      anExtCC.LowerDistanceParameters( pl, pe );
      param += pl;
      if ( ++nbInts == 2 )
        break;
    }
  }
  if ( nbInts > 0 ) param /= nbInts;
  return nbInts > 0;
}
//================================================================================
/*!
 * \brief Find all faces belonging to the outer boundary of mesh
 */
//================================================================================

void SMESH_ElementSearcherImpl::findOuterBoundary(const SMDS_MeshElement* outerFace)
{
  if ( _outerFacesFound ) return;

  // Collect all outer faces by passing from one outer face to another via their links
  // and BTW find out if there are internal faces at all.

  // checked links and links where outer boundary meets internal one
  set< SMESH_TLink > visitedLinks, seamLinks;

  // links to treat with already visited faces sharing them
  list < TFaceLink > startLinks;

  // load startLinks with the first outerFace
  startLinks.push_back( TFaceLink( outerFace->GetNode(0), outerFace->GetNode(1), outerFace));
  _outerFaces.insert( outerFace );

  TIDSortedElemSet emptySet;
  while ( !startLinks.empty() )
  {
    const SMESH_TLink& link  = startLinks.front()._link;
    TIDSortedElemSet&  faces = startLinks.front()._faces;

    outerFace = *faces.begin();
    // find other faces sharing the link
    const SMDS_MeshElement* f;
    while (( f = SMESH_MeshAlgos::FindFaceInSet(link.node1(), link.node2(), emptySet, faces )))
      faces.insert( f );

    // select another outer face among the found
    const SMDS_MeshElement* outerFace2 = 0;
    if ( faces.size() == 2 )
    {
      outerFace2 = (outerFace == *faces.begin() ? *faces.rbegin() : *faces.begin());
    }
    else if ( faces.size() > 2 )
    {
      seamLinks.insert( link );

      // link direction within the outerFace
      gp_Vec n1n2( SMESH_TNodeXYZ( link.node1()),
                   SMESH_TNodeXYZ( link.node2()));
      int i1 = outerFace->GetNodeIndex( link.node1() );
      int i2 = outerFace->GetNodeIndex( link.node2() );
      bool rev = ( abs(i2-i1) == 1 ? i1 > i2 : i2 > i1 );
      if ( rev ) n1n2.Reverse();
      // outerFace normal
      gp_XYZ ofNorm, fNorm;
      if ( SMESH_MeshAlgos::FaceNormal( outerFace, ofNorm, /*normalized=*/false ))
      {
        // direction from the link inside outerFace
        gp_Vec dirInOF = gp_Vec( ofNorm ) ^ n1n2;
        // sort all other faces by angle with the dirInOF
        map< double, const SMDS_MeshElement* > angle2Face;
        set< const SMDS_MeshElement*, TIDCompare >::const_iterator face = faces.begin();
        for ( ; face != faces.end(); ++face )
        {
          if ( !SMESH_MeshAlgos::FaceNormal( *face, fNorm, /*normalized=*/false ))
            continue;
          gp_Vec dirInF = gp_Vec( fNorm ) ^ n1n2;
          double angle = dirInOF.AngleWithRef( dirInF, n1n2 );
          if ( angle < 0 ) angle += 2. * M_PI;
          angle2Face.insert( make_pair( angle, *face ));
        }
        if ( !angle2Face.empty() )
          outerFace2 = angle2Face.begin()->second;
      }
    }
    // store the found outer face and add its links to continue seaching from
    if ( outerFace2 )
    {
      _outerFaces.insert( outerFace );
      int nbNodes = outerFace2->NbNodes()/( outerFace2->IsQuadratic() ? 2 : 1 );
      for ( int i = 0; i < nbNodes; ++i )
      {
        SMESH_TLink link2( outerFace2->GetNode(i), outerFace2->GetNode((i+1)%nbNodes));
        if ( visitedLinks.insert( link2 ).second )
          startLinks.push_back( TFaceLink( link2.node1(), link2.node2(), outerFace2 ));
      }
    }
    startLinks.pop_front();
  }
  _outerFacesFound = true;

  if ( !seamLinks.empty() )
  {
    // There are internal boundaries touching the outher one,
    // find all faces of internal boundaries in order to find
    // faces of boundaries of holes, if any.

  }
  else
  {
    _outerFaces.clear();
  }
}

//=======================================================================
/*!
 * \brief Find elements of given type where the given point is IN or ON.
 *        Returns nb of found elements and elements them-selves.
 *
 * 'ALL' type means elements of any type excluding nodes, balls and 0D elements
 */
//=======================================================================

int SMESH_ElementSearcherImpl::
FindElementsByPoint(const gp_Pnt&                      point,
                    SMDSAbs_ElementType                type,
                    vector< const SMDS_MeshElement* >& foundElements)
{
  foundElements.clear();

  double tolerance = getTolerance();

  // =================================================================================
  if ( type == SMDSAbs_Node || type == SMDSAbs_0DElement || type == SMDSAbs_Ball)
  {
    if ( !_nodeSearcher )
      _nodeSearcher = new SMESH_NodeSearcherImpl( _mesh );

    std::vector< const SMDS_MeshNode* > foundNodes;
    _nodeSearcher->FindNearPoint( point, tolerance, foundNodes );

    if ( type == SMDSAbs_Node )
    {
      foundElements.assign( foundNodes.begin(), foundNodes.end() );
    }
    else
    {
      for ( size_t i = 0; i < foundNodes.size(); ++i )
      {
        SMDS_ElemIteratorPtr elemIt = foundNodes[i]->GetInverseElementIterator( type );
        while ( elemIt->more() )
          foundElements.push_back( elemIt->next() );
      }
    }
  }
  // =================================================================================
  else // elements more complex than 0D
  {
    if ( !_ebbTree || _elementType != type )
    {
      if ( _ebbTree ) delete _ebbTree;
      _ebbTree = new ElementBndBoxTree( *_mesh, _elementType = type, _meshPartIt, tolerance );
    }
    TIDSortedElemSet suspectElems;
    _ebbTree->getElementsNearPoint( point, suspectElems );
    TIDSortedElemSet::iterator elem = suspectElems.begin();
    for ( ; elem != suspectElems.end(); ++elem )
      if ( !SMESH_MeshAlgos::IsOut( *elem, point, tolerance ))
        foundElements.push_back( *elem );
  }
  return foundElements.size();
}

//=======================================================================
/*!
 * \brief Find an element of given type most close to the given point
 *
 * WARNING: Only face search is implemeneted so far
 */
//=======================================================================

const SMDS_MeshElement*
SMESH_ElementSearcherImpl::FindClosestTo( const gp_Pnt&       point,
                                          SMDSAbs_ElementType type )
{
  const SMDS_MeshElement* closestElem = 0;

  if ( type == SMDSAbs_Face || type == SMDSAbs_Volume )
  {
    if ( !_ebbTree || _elementType != type )
    {
      if ( _ebbTree ) delete _ebbTree;
      _ebbTree = new ElementBndBoxTree( *_mesh, _elementType = type, _meshPartIt );
    }
    TIDSortedElemSet suspectElems;
    _ebbTree->getElementsNearPoint( point, suspectElems );

    if ( suspectElems.empty() && _ebbTree->maxSize() > 0 )
    {
      gp_Pnt boxCenter = 0.5 * ( _ebbTree->getBox()->CornerMin() +
                                 _ebbTree->getBox()->CornerMax() );
      double radius = -1;
      if ( _ebbTree->getBox()->IsOut( point.XYZ() ))
        radius = point.Distance( boxCenter ) - 0.5 * _ebbTree->maxSize();
      if ( radius < 0 )
        radius = _ebbTree->maxSize() / pow( 2., _ebbTree->getHeight()) / 2;
      while ( suspectElems.empty() )
      {
        _ebbTree->getElementsInSphere( point.XYZ(), radius, suspectElems );
        radius *= 1.1;
      }
    }
    double minDist = std::numeric_limits<double>::max();
    multimap< double, const SMDS_MeshElement* > dist2face;
    TIDSortedElemSet::iterator elem = suspectElems.begin();
    for ( ; elem != suspectElems.end(); ++elem )
    {
      double dist = SMESH_MeshAlgos::GetDistance( *elem, point );
      if ( dist < minDist + 1e-10)
      {
        minDist = dist;
        dist2face.insert( dist2face.begin(), make_pair( dist, *elem ));
      }
    }
    if ( !dist2face.empty() )
    {
      multimap< double, const SMDS_MeshElement* >::iterator d2f = dist2face.begin();
      closestElem = d2f->second;
      // if there are several elements at the same distance, select one
      // with GC closest to the point
      typedef SMDS_StdIterator< SMESH_TNodeXYZ, SMDS_ElemIteratorPtr > TXyzIterator;
      double minDistToGC = 0;
      for ( ++d2f; d2f != dist2face.end() && fabs( d2f->first - minDist ) < 1e-10; ++d2f )
      {
        if ( minDistToGC == 0 )
        {
          gp_XYZ gc(0,0,0);
          gc = accumulate( TXyzIterator(closestElem->nodesIterator()),
                           TXyzIterator(), gc ) / closestElem->NbNodes();
          minDistToGC = point.SquareDistance( gc );
        }
        gp_XYZ gc(0,0,0);
        gc = accumulate( TXyzIterator( d2f->second->nodesIterator()),
                         TXyzIterator(), gc ) / d2f->second->NbNodes();
        double d = point.SquareDistance( gc );
        if ( d < minDistToGC )
        {
          minDistToGC = d;
          closestElem = d2f->second;
        }
      }
      // cout << "FindClosestTo( " <<point.X()<<", "<<point.Y()<<", "<<point.Z()<<" ) FACE "
      //      <<closestElem->GetID() << " DIST " << minDist << endl;
    }
  }
  else
  {
    // NOT IMPLEMENTED SO FAR
  }
  return closestElem;
}


//================================================================================
/*!
 * \brief Classify the given point in the closed 2D mesh
 */
//================================================================================

TopAbs_State SMESH_ElementSearcherImpl::GetPointState(const gp_Pnt& point)
{
  double tolerance = getTolerance();
  if ( !_ebbTree || _elementType != SMDSAbs_Face )
  {
    if ( _ebbTree ) delete _ebbTree;
    _ebbTree = new ElementBndBoxTree( *_mesh, _elementType = SMDSAbs_Face, _meshPartIt );
  }
  // Algo: analyse transition of a line starting at the point through mesh boundary;
  // try three lines parallel to axis of the coordinate system and perform rough
  // analysis. If solution is not clear perform thorough analysis.

  const int nbAxes = 3;
  gp_Dir axisDir[ nbAxes ] = { gp::DX(), gp::DY(), gp::DZ() };
  map< double, TInters >   paramOnLine2TInters[ nbAxes ];
  list< TInters > tangentInters[ nbAxes ]; // of faces whose plane includes the line
  multimap< int, int > nbInt2Axis; // to find the simplest case
  for ( int axis = 0; axis < nbAxes; ++axis )
  {
    gp_Ax1 lineAxis( point, axisDir[axis]);
    gp_Lin line    ( lineAxis );

    TIDSortedElemSet suspectFaces; // faces possibly intersecting the line
    _ebbTree->getElementsNearLine( lineAxis, suspectFaces );

    // Intersect faces with the line

    map< double, TInters > & u2inters = paramOnLine2TInters[ axis ];
    TIDSortedElemSet::iterator face = suspectFaces.begin();
    for ( ; face != suspectFaces.end(); ++face )
    {
      // get face plane
      gp_XYZ fNorm;
      if ( !SMESH_MeshAlgos::FaceNormal( *face, fNorm, /*normalized=*/false)) continue;
      gp_Pln facePlane( SMESH_TNodeXYZ( (*face)->GetNode(0)), fNorm );

      // perform intersection
      IntAna_IntConicQuad intersection( line, IntAna_Quadric( facePlane ));
      if ( !intersection.IsDone() )
        continue;
      if ( intersection.IsInQuadric() )
      {
        tangentInters[ axis ].push_back( TInters( *face, fNorm, true ));
      }
      else if ( ! intersection.IsParallel() && intersection.NbPoints() > 0 )
      {
        gp_Pnt intersectionPoint = intersection.Point(1);
        if ( !SMESH_MeshAlgos::IsOut( *face, intersectionPoint, tolerance ))
          u2inters.insert(make_pair( intersection.ParamOnConic(1), TInters( *face, fNorm )));
      }
    }
    // Analyse intersections roughly

    int nbInter = u2inters.size();
    if ( nbInter == 0 )
      return TopAbs_OUT;

    double f = u2inters.begin()->first, l = u2inters.rbegin()->first;
    if ( nbInter == 1 ) // not closed mesh
      return fabs( f ) < tolerance ? TopAbs_ON : TopAbs_UNKNOWN;

    if ( fabs( f ) < tolerance || fabs( l ) < tolerance )
      return TopAbs_ON;

    if ( (f<0) == (l<0) )
      return TopAbs_OUT;

    int nbIntBeforePoint = std::distance( u2inters.begin(), u2inters.lower_bound(0));
    int nbIntAfterPoint  = nbInter - nbIntBeforePoint;
    if ( nbIntBeforePoint == 1 || nbIntAfterPoint == 1 )
      return TopAbs_IN;

    nbInt2Axis.insert( make_pair( min( nbIntBeforePoint, nbIntAfterPoint ), axis ));

    if ( _outerFacesFound ) break; // pass to thorough analysis

  } // three attempts - loop on CS axes

  // Analyse intersections thoroughly.
  // We make two loops maximum, on the first one we only exclude touching intersections,
  // on the second, if situation is still unclear, we gather and use information on
  // position of faces (internal or outer). If faces position is already gathered,
  // we make the second loop right away.

  for ( int hasPositionInfo = _outerFacesFound; hasPositionInfo < 2; ++hasPositionInfo )
  {
    multimap< int, int >::const_iterator nb_axis = nbInt2Axis.begin();
    for ( ; nb_axis != nbInt2Axis.end(); ++nb_axis )
    {
      int axis = nb_axis->second;
      map< double, TInters > & u2inters = paramOnLine2TInters[ axis ];

      gp_Ax1 lineAxis( point, axisDir[axis]);
      gp_Lin line    ( lineAxis );

      // add tangent intersections to u2inters
      double param;
      list< TInters >::const_iterator tgtInt = tangentInters[ axis ].begin();
      for ( ; tgtInt != tangentInters[ axis ].end(); ++tgtInt )
        if ( getIntersParamOnLine( line, tgtInt->_face, tolerance, param ))
          u2inters.insert(make_pair( param, *tgtInt ));
      tangentInters[ axis ].clear();

      // Count intersections before and after the point excluding touching ones.
      // If hasPositionInfo we count intersections of outer boundary only

      int nbIntBeforePoint = 0, nbIntAfterPoint = 0;
      double f = numeric_limits<double>::max(), l = -numeric_limits<double>::max();
      map< double, TInters >::iterator u_int1 = u2inters.begin(), u_int2 = u_int1;
      bool ok = ! u_int1->second._coincides;
      while ( ok && u_int1 != u2inters.end() )
      {
        double u = u_int1->first;
        bool touchingInt = false;
        if ( ++u_int2 != u2inters.end() )
        {
          // skip intersections at the same point (if the line passes through edge or node)
          int nbSamePnt = 0;
          while ( u_int2 != u2inters.end() && fabs( u_int2->first - u ) < tolerance )
          {
            ++nbSamePnt;
            ++u_int2;
          }

          // skip tangent intersections
          int nbTgt = 0;
          const SMDS_MeshElement* prevFace = u_int1->second._face;
          while ( ok && u_int2->second._coincides )
          {
            if ( SMESH_MeshAlgos::GetCommonNodes(prevFace , u_int2->second._face).empty() )
              ok = false;
            else
            {
              nbTgt++;
              u_int2++;
              ok = ( u_int2 != u2inters.end() );
            }
          }
          if ( !ok ) break;

          // skip intersections at the same point after tangent intersections
          if ( nbTgt > 0 )
          {
            double u2 = u_int2->first;
            ++u_int2;
            while ( u_int2 != u2inters.end() && fabs( u_int2->first - u2 ) < tolerance )
            {
              ++nbSamePnt;
              ++u_int2;
            }
          }
          // decide if we skipped a touching intersection
          if ( nbSamePnt + nbTgt > 0 )
          {
            double minDot = numeric_limits<double>::max(), maxDot = -numeric_limits<double>::max();
            map< double, TInters >::iterator u_int = u_int1;
            for ( ; u_int != u_int2; ++u_int )
            {
              if ( u_int->second._coincides ) continue;
              double dot = u_int->second._faceNorm * line.Direction();
              if ( dot > maxDot ) maxDot = dot;
              if ( dot < minDot ) minDot = dot;
            }
            touchingInt = ( minDot*maxDot < 0 );
          }
        }
        if ( !touchingInt )
        {
          if ( !hasPositionInfo || isOuterBoundary( u_int1->second._face ))
          {
            if ( u < 0 )
              ++nbIntBeforePoint;
            else
              ++nbIntAfterPoint;
          }
          if ( u < f ) f = u;
          if ( u > l ) l = u;
        }

        u_int1 = u_int2; // to next intersection

      } // loop on intersections with one line

      if ( ok )
      {
        if ( fabs( f ) < tolerance || fabs( l ) < tolerance )
          return TopAbs_ON;

        if ( nbIntBeforePoint == 0  || nbIntAfterPoint == 0)
          return TopAbs_OUT;

        if ( nbIntBeforePoint + nbIntAfterPoint == 1 ) // not closed mesh
          return fabs( f ) < tolerance ? TopAbs_ON : TopAbs_UNKNOWN;

        if ( nbIntBeforePoint == 1 || nbIntAfterPoint == 1 )
          return TopAbs_IN;

        if ( (f<0) == (l<0) )
          return TopAbs_OUT;

        if ( hasPositionInfo )
          return nbIntBeforePoint % 2 ? TopAbs_IN : TopAbs_OUT;
      }
    } // loop on intersections of the tree lines - thorough analysis

    if ( !hasPositionInfo )
    {
      // gather info on faces position - is face in the outer boundary or not
      map< double, TInters > & u2inters = paramOnLine2TInters[ 0 ];
      findOuterBoundary( u2inters.begin()->second._face );
    }

  } // two attempts - with and w/o faces position info in the mesh

  return TopAbs_UNKNOWN;
}

//=======================================================================
/*!
 * \brief Return elements possibly intersecting the line
 */
//=======================================================================

void SMESH_ElementSearcherImpl::GetElementsNearLine( const gp_Ax1&                      line,
                                                     SMDSAbs_ElementType                type,
                                                     vector< const SMDS_MeshElement* >& foundElems)
{
  if ( !_ebbTree || _elementType != type )
  {
    if ( _ebbTree ) delete _ebbTree;
    _ebbTree = new ElementBndBoxTree( *_mesh, _elementType = type, _meshPartIt );
  }
  TIDSortedElemSet suspectFaces; // elements possibly intersecting the line
  _ebbTree->getElementsNearLine( line, suspectFaces );
  foundElems.assign( suspectFaces.begin(), suspectFaces.end());
}

//=======================================================================
/*!
 * \brief Return true if the point is IN or ON of the element
 */
//=======================================================================

bool SMESH_MeshAlgos::IsOut( const SMDS_MeshElement* element, const gp_Pnt& point, double tol )
{
  if ( element->GetType() == SMDSAbs_Volume)
  {
    return SMDS_VolumeTool( element ).IsOut( point.X(), point.Y(), point.Z(), tol );
  }

  // get ordered nodes

  vector< SMESH_TNodeXYZ > xyz;

  SMDS_ElemIteratorPtr nodeIt = element->interlacedNodesElemIterator();
  while ( nodeIt->more() )
  {
    SMESH_TNodeXYZ node = nodeIt->next();
    xyz.push_back( node );
  }

  int i, nbNodes = (int) xyz.size(); // central node of biquadratic is missing

  if ( element->GetType() == SMDSAbs_Face ) // --------------------------------------------------
  {
    // compute face normal
    gp_Vec faceNorm(0,0,0);
    xyz.push_back( xyz.front() );
    for ( i = 0; i < nbNodes; ++i )
    {
      gp_Vec edge1( xyz[i+1], xyz[i]);
      gp_Vec edge2( xyz[i+1], xyz[(i+2)%nbNodes] );
      faceNorm += edge1 ^ edge2;
    }
    double normSize = faceNorm.Magnitude();
    if ( normSize <= tol )
    {
      // degenerated face: point is out if it is out of all face edges
      for ( i = 0; i < nbNodes; ++i )
      {
        SMDS_LinearEdge edge( xyz[i]._node, xyz[i+1]._node );
        if ( !IsOut( &edge, point, tol ))
          return false;
      }
      return true;
    }
    faceNorm /= normSize;

    // check if the point lays on face plane
    gp_Vec n2p( xyz[0], point );
    if ( fabs( n2p * faceNorm ) > tol )
      return true; // not on face plane

    // check if point is out of face boundary:
    // define it by closest transition of a ray point->infinity through face boundary
    // on the face plane.
    // First, find normal of a plane perpendicular to face plane, to be used as a cutting tool
    // to find intersections of the ray with the boundary.
    gp_Vec ray = n2p;
    gp_Vec plnNorm = ray ^ faceNorm;
    normSize = plnNorm.Magnitude();
    if ( normSize <= tol ) return false; // point coincides with the first node
    plnNorm /= normSize;
    // for each node of the face, compute its signed distance to the plane
    vector<double> dist( nbNodes + 1);
    for ( i = 0; i < nbNodes; ++i )
    {
      gp_Vec n2p( xyz[i], point );
      dist[i] = n2p * plnNorm;
    }
    dist.back() = dist.front();
    // find the closest intersection
    int    iClosest = -1;
    double rClosest, distClosest = 1e100;;
    gp_Pnt pClosest;
    for ( i = 0; i < nbNodes; ++i )
    {
      double r;
      if ( fabs( dist[i]) < tol )
        r = 0.;
      else if ( fabs( dist[i+1]) < tol )
        r = 1.;
      else if ( dist[i] * dist[i+1] < 0 )
        r = dist[i] / ( dist[i] - dist[i+1] );
      else
        continue; // no intersection
      gp_Pnt pInt = xyz[i] * (1.-r) + xyz[i+1] * r;
      gp_Vec p2int ( point, pInt);
      if ( p2int * ray > -tol ) // right half-space
      {
        double intDist = p2int.SquareMagnitude();
        if ( intDist < distClosest )
        {
          iClosest = i;
          rClosest = r;
          pClosest = pInt;
          distClosest = intDist;
        }
      }
    }
    if ( iClosest < 0 )
      return true; // no intesections - out

    // analyse transition
    gp_Vec edge( xyz[iClosest], xyz[iClosest+1] );
    gp_Vec edgeNorm = -( edge ^ faceNorm ); // normal to intersected edge pointing out of face
    gp_Vec p2int ( point, pClosest );
    bool out = (edgeNorm * p2int) < -tol;
    if ( rClosest > 0. && rClosest < 1. ) // not node intersection
      return out;

    // ray pass through a face node; analyze transition through an adjacent edge
    gp_Pnt p1 = xyz[ (rClosest == 0.) ? ((iClosest+nbNodes-1) % nbNodes) : (iClosest+1) ];
    gp_Pnt p2 = xyz[ (rClosest == 0.) ? iClosest : ((iClosest+2) % nbNodes) ];
    gp_Vec edgeAdjacent( p1, p2 );
    gp_Vec edgeNorm2 = -( edgeAdjacent ^ faceNorm );
    bool out2 = (edgeNorm2 * p2int) < -tol;

    bool covexCorner = ( edgeNorm * edgeAdjacent * (rClosest==1. ? 1. : -1.)) < 0;
    return covexCorner ? (out || out2) : (out && out2);
  }
  if ( element->GetType() == SMDSAbs_Edge ) // --------------------------------------------------
  {
    // point is out of edge if it is NOT ON any straight part of edge
    // (we consider quadratic edge as being composed of two straight parts)
    for ( i = 1; i < nbNodes; ++i )
    {
      gp_Vec edge( xyz[i-1], xyz[i] );
      gp_Vec n1p ( xyz[i-1], point  );
      double u = ( edge * n1p ) / edge.SquareMagnitude(); // param [0,1] on the edge
      if ( u <= 0. ) {
        if ( n1p.SquareMagnitude() < tol * tol )
          return false;
        continue;
      }
      if ( u >= 1. ) {
        if ( point.SquareDistance( xyz[i] ) < tol * tol )
          return false;
        continue;
      }
      gp_XYZ proj = ( 1. - u ) * xyz[i-1] + u * xyz[i]; // projection of the point on the edge
      double dist2 = point.SquareDistance( proj );
      if ( dist2 > tol * tol )
        continue;
      return false; // point is ON this part
    }
    return true;
  }
  // Node or 0D element -------------------------------------------------------------------------
  {
    gp_Vec n2p ( xyz[0], point );
    return n2p.SquareMagnitude() <= tol * tol;
  }
  return true;
}

//=======================================================================
namespace
{
  // Position of a point relative to a segment
  //            .           .
  //            .  LEFT     .
  //            .           .
  //  VERTEX 1  o----ON----->  VERTEX 2
  //            .           .
  //            .  RIGHT    .
  //            .           .
  enum PositionName { POS_LEFT = 1, POS_VERTEX = 2, POS_RIGHT = 4, //POS_ON = 8,
                      POS_ALL = POS_LEFT | POS_RIGHT | POS_VERTEX };
  struct PointPos
  {
    PositionName _name;
    int          _index; // index of vertex or segment

    PointPos( PositionName n, int i=-1 ): _name(n), _index(i) {}
    bool operator < (const PointPos& other ) const
    {
      if ( _name == other._name )
        return  ( _index < 0 || other._index < 0 ) ? false : _index < other._index;
      return _name < other._name;
    }
  };

  //================================================================================
  /*!
   * \brief Return of a point relative to a segment
   *  \param point2D      - the point to analyze position of
   *  \param xyVec        - end points of segments
   *  \param index0       - 0-based index of the first point of segment
   *  \param posToFindOut - flags of positions to detect
   *  \retval PointPos - point position
   */
  //================================================================================

  PointPos getPointPosition( const gp_XY& point2D,
                             const gp_XY* segEnds,
                             const int    index0 = 0,
                             const int    posToFindOut = POS_ALL)
  {
    const gp_XY& p1 = segEnds[ index0   ];
    const gp_XY& p2 = segEnds[ index0+1 ];
    const gp_XY grad = p2 - p1;

    if ( posToFindOut & POS_VERTEX )
    {
      // check if the point2D is at "vertex 1" zone
      gp_XY pp1[2] = { p1, gp_XY( p1.X() - grad.Y(),
                                  p1.Y() + grad.X() ) };
      if ( getPointPosition( point2D, pp1, 0, POS_LEFT|POS_RIGHT )._name == POS_LEFT )
        return PointPos( POS_VERTEX, index0 );

      // check if the point2D is at "vertex 2" zone
      gp_XY pp2[2] = { p2, gp_XY( p2.X() - grad.Y(),
                                  p2.Y() + grad.X() ) };
      if ( getPointPosition( point2D, pp2, 0, POS_LEFT|POS_RIGHT )._name == POS_RIGHT )
        return PointPos( POS_VERTEX, index0 + 1);
    }
    double edgeEquation =
      ( point2D.X() - p1.X() ) * grad.Y() - ( point2D.Y() - p1.Y() ) * grad.X();
    return PointPos( edgeEquation < 0 ? POS_LEFT : POS_RIGHT, index0 );
  }
}

//=======================================================================
/*!
 * \brief Return minimal distance from a point to an element
 *
 * Currently we ignore non-planarity and 2nd order of face
 */
//=======================================================================

double SMESH_MeshAlgos::GetDistance( const SMDS_MeshElement* elem,
                                     const gp_Pnt&           point )
{
  switch ( elem->GetType() )
  {
  case SMDSAbs_Volume:
    return GetDistance( dynamic_cast<const SMDS_MeshVolume*>( elem ), point);
  case SMDSAbs_Face:
    return GetDistance( dynamic_cast<const SMDS_MeshFace*>( elem ), point);
  case SMDSAbs_Edge:
    return GetDistance( dynamic_cast<const SMDS_MeshEdge*>( elem ), point);
  case SMDSAbs_Node:
    return point.Distance( SMESH_TNodeXYZ( elem ));
  }
  return -1;
}

//=======================================================================
/*!
 * \brief Return minimal distance from a point to a face
 *
 * Currently we ignore non-planarity and 2nd order of face
 */
//=======================================================================

double SMESH_MeshAlgos::GetDistance( const SMDS_MeshFace* face,
                                     const gp_Pnt&        point )
{
  double badDistance = -1;
  if ( !face ) return badDistance;

  // coordinates of nodes (medium nodes, if any, ignored)
  typedef SMDS_StdIterator< SMESH_TNodeXYZ, SMDS_ElemIteratorPtr > TXyzIterator;
  vector<gp_XYZ> xyz( TXyzIterator( face->nodesIterator()), TXyzIterator() );
  xyz.resize( face->NbCornerNodes()+1 );

  // transformation to get xyz[0] lies on the origin, xyz[1] lies on the Z axis,
  // and xyz[2] lies in the XZ plane. This is to pass to 2D space on XZ plane.
  gp_Trsf trsf;
  gp_Vec OZ ( xyz[0], xyz[1] );
  gp_Vec OX ( xyz[0], xyz[2] );
  if ( OZ.Magnitude() < std::numeric_limits<double>::min() )
  {
    if ( xyz.size() < 4 ) return badDistance;
    OZ = gp_Vec ( xyz[0], xyz[2] );
    OX = gp_Vec ( xyz[0], xyz[3] );
  }
  gp_Ax3 tgtCS;
  try {
    tgtCS = gp_Ax3( xyz[0], OZ, OX );
  }
  catch ( Standard_Failure &) {
    return badDistance;
  }
  trsf.SetTransformation( tgtCS );

  // move all the nodes to 2D
  vector<gp_XY> xy( xyz.size() );
  for ( size_t i = 0;i < xyz.size()-1; ++i )
  {
    gp_XYZ p3d = xyz[i];
    trsf.Transforms( p3d );
    xy[i].SetCoord( p3d.X(), p3d.Z() );
  }
  xyz.back() = xyz.front();
  xy.back() = xy.front();

  // // move the point in 2D
  gp_XYZ tmpPnt = point.XYZ();
  trsf.Transforms( tmpPnt );
  gp_XY point2D( tmpPnt.X(), tmpPnt.Z() );

  // loop on segments of the face to analyze point position ralative to the face
  set< PointPos > pntPosSet;
  for ( size_t i = 1; i < xy.size(); ++i )
  {
    PointPos pos = getPointPosition( point2D, &xy[0], i-1 );
    pntPosSet.insert( pos );
  }

  // compute distance
  PointPos pos = *pntPosSet.begin();
  // cout << "Face " << face->GetID() << " DIST: ";
  switch ( pos._name )
  {
  case POS_LEFT: {
    // point is most close to a segment
    gp_Vec p0p1( point, xyz[ pos._index ] );
    gp_Vec p1p2( xyz[ pos._index ], xyz[ pos._index+1 ]); // segment vector
    p1p2.Normalize();
    double projDist = p0p1 * p1p2; // distance projected to the segment
    gp_Vec projVec = p1p2 * projDist;
    gp_Vec distVec = p0p1 - projVec;
    // cout << distVec.Magnitude()  << ", SEG " << face->GetNode(pos._index)->GetID()
    //      << " - " << face->GetNodeWrap(pos._index+1)->GetID() << endl;
    return distVec.Magnitude();
  }
  case POS_RIGHT: {
    // point is inside the face
    double distToFacePlane = tmpPnt.Y();
    // cout << distToFacePlane << ", INSIDE " << endl;
    return Abs( distToFacePlane );
  }
  case POS_VERTEX: {
    // point is most close to a node
    gp_Vec distVec( point, xyz[ pos._index ]);
    // cout << distVec.Magnitude()  << " VERTEX " << face->GetNode(pos._index)->GetID() << endl;
    return distVec.Magnitude();
  }
  }
  return badDistance;
}

//=======================================================================
/*!
 * \brief Return minimal distance from a point to an edge
 */
//=======================================================================

double SMESH_MeshAlgos::GetDistance( const SMDS_MeshEdge* edge, const gp_Pnt& point )
{
  throw SALOME_Exception(LOCALIZED("not implemented so far"));
}

//=======================================================================
/*!
 * \brief Return minimal distance from a point to a volume
 *
 * Currently we ignore non-planarity and 2nd order
 */
//=======================================================================

double SMESH_MeshAlgos::GetDistance( const SMDS_MeshVolume* volume, const gp_Pnt& point )
{
  SMDS_VolumeTool vTool( volume );
  vTool.SetExternalNormal();
  const int iQ = volume->IsQuadratic() ? 2 : 1;

  double n[3], bc[3];
  double minDist = 1e100, dist;
  for ( int iF = 0; iF < vTool.NbFaces(); ++iF )
  {
    // skip a facet with normal not "looking at" the point
    if ( !vTool.GetFaceNormal( iF, n[0], n[1], n[2] ) ||
         !vTool.GetFaceBaryCenter( iF, bc[0], bc[1], bc[2] ))
      continue;
    gp_XYZ bcp = point.XYZ() - gp_XYZ( bc[0], bc[1], bc[2] );
    if ( gp_XYZ( n[0], n[1], n[2] ) * bcp < 1e-6 )
      continue;

    // find distance to a facet
    const SMDS_MeshNode** nodes = vTool.GetFaceNodes( iF );
    switch ( vTool.NbFaceNodes( iF ) / iQ ) {
    case 3:
    {
      SMDS_FaceOfNodes tmpFace( nodes[0], nodes[ 1*iQ ], nodes[ 2*iQ ] );
      dist = GetDistance( &tmpFace, point );
      break;
    }
    case 4:
    {
      SMDS_FaceOfNodes tmpFace( nodes[0], nodes[ 1*iQ ], nodes[ 2*iQ ], nodes[ 3*iQ ]);
      dist = GetDistance( &tmpFace, point );
      break;
    }
    default:
      vector<const SMDS_MeshNode *> nvec( nodes, nodes + vTool.NbFaceNodes( iF ));
      SMDS_PolygonalFaceOfNodes tmpFace( nvec );
      dist = GetDistance( &tmpFace, point );
    }
    minDist = Min( minDist, dist );
  }
  return minDist;
}

//================================================================================
/*!
 * \brief Returns barycentric coordinates of a point within a triangle.
 *        A not returned bc2 = 1. - bc0 - bc1.
 *        The point lies within the triangle if ( bc0 >= 0 && bc1 >= 0 && bc0+bc1 <= 1 )
 */
//================================================================================

void SMESH_MeshAlgos::GetBarycentricCoords( const gp_XY& p,
                                            const gp_XY& t0,
                                            const gp_XY& t1,
                                            const gp_XY& t2,
                                            double &     bc0,
                                            double &     bc1)
{
  const double // matrix 2x2
    T11 = t0.X()-t2.X(), T12 = t1.X()-t2.X(),
    T21 = t0.Y()-t2.Y(), T22 = t1.Y()-t2.Y();
  const double Tdet = T11*T22 - T12*T21; // matrix determinant
  if ( Abs( Tdet ) < std::numeric_limits<double>::min() )
  {
    bc0 = bc1 = 2.;
    return;
  }
  // matrix inverse
  const double t11 = T22, t12 = -T12, t21 = -T21, t22 = T11;
  // vector
  const double r11 = p.X()-t2.X(), r12 = p.Y()-t2.Y();
  // barycentric coordinates: mutiply matrix by vector
  bc0 = (t11 * r11 + t12 * r12)/Tdet;
  bc1 = (t21 * r11 + t22 * r12)/Tdet;
}

//=======================================================================
//function : FindFaceInSet
//purpose  : Return a face having linked nodes n1 and n2 and which is
//           - not in avoidSet,
//           - in elemSet provided that !elemSet.empty()
//           i1 and i2 optionally returns indices of n1 and n2
//=======================================================================

const SMDS_MeshElement*
SMESH_MeshAlgos::FindFaceInSet(const SMDS_MeshNode*    n1,
                               const SMDS_MeshNode*    n2,
                               const TIDSortedElemSet& elemSet,
                               const TIDSortedElemSet& avoidSet,
                               int*                    n1ind,
                               int*                    n2ind)

{
  int i1, i2;
  const SMDS_MeshElement* face = 0;

  SMDS_ElemIteratorPtr invElemIt = n1->GetInverseElementIterator(SMDSAbs_Face);
  while ( invElemIt->more() && !face ) // loop on inverse faces of n1
  {
    const SMDS_MeshElement* elem = invElemIt->next();
    if (avoidSet.count( elem ))
      continue;
    if ( !elemSet.empty() && !elemSet.count( elem ))
      continue;
    // index of n1
    i1 = elem->GetNodeIndex( n1 );
    // find a n2 linked to n1
    int nbN = elem->IsQuadratic() ? elem->NbNodes()/2 : elem->NbNodes();
    for ( int di = -1; di < 2 && !face; di += 2 )
    {
      i2 = (i1+di+nbN) % nbN;
      if ( elem->GetNode( i2 ) == n2 )
        face = elem;
    }
    if ( !face && elem->IsQuadratic())
    {
      // analysis for quadratic elements using all nodes
      SMDS_ElemIteratorPtr anIter = elem->interlacedNodesElemIterator();
      const SMDS_MeshNode* prevN = static_cast<const SMDS_MeshNode*>( anIter->next() );
      for ( i1 = -1, i2 = 0; anIter->more() && !face; i1++, i2++ )
      {
        const SMDS_MeshNode* n = static_cast<const SMDS_MeshNode*>( anIter->next() );
        if ( n1 == prevN && n2 == n )
        {
          face = elem;
        }
        else if ( n2 == prevN && n1 == n )
        {
          face = elem; swap( i1, i2 );
        }
        prevN = n;
      }
    }
  }
  if ( n1ind ) *n1ind = i1;
  if ( n2ind ) *n2ind = i2;
  return face;
}

//================================================================================
/*!
 * \brief Calculate normal of a mesh face
 */
//================================================================================

bool SMESH_MeshAlgos::FaceNormal(const SMDS_MeshElement* F, gp_XYZ& normal, bool normalized)
{
  if ( !F || F->GetType() != SMDSAbs_Face )
    return false;

  normal.SetCoord(0,0,0);
  int nbNodes = F->NbCornerNodes();
  for ( int i = 0; i < nbNodes-2; ++i )
  {
    gp_XYZ p[3];
    for ( int n = 0; n < 3; ++n )
    {
      const SMDS_MeshNode* node = F->GetNode( i + n );
      p[n].SetCoord( node->X(), node->Y(), node->Z() );
    }
    normal += ( p[2] - p[1] ) ^ ( p[0] - p[1] );
  }
  double size2 = normal.SquareModulus();
  bool ok = ( size2 > numeric_limits<double>::min() * numeric_limits<double>::min());
  if ( normalized && ok )
    normal /= sqrt( size2 );

  return ok;
}

//=======================================================================
//function : GetCommonNodes
//purpose  : Return nodes common to two elements
//=======================================================================

vector< const SMDS_MeshNode*> SMESH_MeshAlgos::GetCommonNodes(const SMDS_MeshElement* e1,
                                                              const SMDS_MeshElement* e2)
{
  vector< const SMDS_MeshNode*> common;
  for ( int i = 0 ; i < e1->NbNodes(); ++i )
    if ( e2->GetNodeIndex( e1->GetNode( i )) >= 0 )
      common.push_back( e1->GetNode( i ));
  return common;
}

//=======================================================================
/*!
 * \brief Return SMESH_NodeSearcher
 */
//=======================================================================

SMESH_NodeSearcher* SMESH_MeshAlgos::GetNodeSearcher(SMDS_Mesh& mesh)
{
  return new SMESH_NodeSearcherImpl( &mesh );
}

//=======================================================================
/*!
 * \brief Return SMESH_ElementSearcher
 */
//=======================================================================

SMESH_ElementSearcher* SMESH_MeshAlgos::GetElementSearcher(SMDS_Mesh& mesh,
                                                           double     tolerance)
{
  return new SMESH_ElementSearcherImpl( mesh, tolerance );
}

//=======================================================================
/*!
 * \brief Return SMESH_ElementSearcher acting on a sub-set of elements
 */
//=======================================================================

SMESH_ElementSearcher* SMESH_MeshAlgos::GetElementSearcher(SMDS_Mesh&           mesh,
                                                           SMDS_ElemIteratorPtr elemIt,
                                                           double               tolerance)
{
  return new SMESH_ElementSearcherImpl( mesh, tolerance, elemIt );
}
