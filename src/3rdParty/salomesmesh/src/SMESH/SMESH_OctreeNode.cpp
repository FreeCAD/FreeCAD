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
//  SMESH SMESH_OctreeNode : Octree with Nodes set
//  inherites global class SMESH_Octree
//
//  File      : SMESH_OctreeNode.cxx
//  Created   : Tue Jan 16 16:00:00 2007
//  Author    : Nicolas Geimer & Aurélien Motteux (OCC)
//  Module    : SMESH

#include "SMESH_OctreeNode.hxx"

#include "SMDS_MeshNode.hxx"
#include "SMDS_SetIterator.hxx"
#include <gp_Pnt.hxx>

using namespace std;

//===============================================================
/*!
 * \brief Constructor : Build all the Octree using Compute()
 * \param theNodes - Set of nodes, the Octree is built from this nodes
 * \param maxLevel - Maximum level for the leaves
 * \param maxNbNodes - Maximum number of nodes, a leaf can contain
 * \param minBoxSize - Minimal size of the Octree Box
 */
//================================================================
SMESH_OctreeNode::SMESH_OctreeNode (const set<const SMDS_MeshNode*> & theNodes, const int maxLevel,
                                    const int maxNbNodes , const double minBoxSize )
  :SMESH_Octree( new SMESH_Octree::Limit( maxLevel,minBoxSize)),
  myMaxNbNodes(maxNbNodes),
  myNodes(theNodes)
{
  compute();
}

//================================================================================
/*!
 * \brief Constructor used to allocate a child
 */
//================================================================================

SMESH_OctreeNode::SMESH_OctreeNode (int maxNbNodes):
  SMESH_Octree(), myMaxNbNodes(maxNbNodes)
{
}

//==================================================================================
/*!
 * \brief Construct an empty SMESH_OctreeNode used by SMESH_Octree::buildChildren()
 */
//==================================================================================

SMESH_Octree* SMESH_OctreeNode::allocateOctreeChild() const
{
  return new SMESH_OctreeNode(myMaxNbNodes);
}

//======================================
/*!
 * \brief Compute the first bounding box
 *
 * We take the max/min coord of the nodes
 */
//======================================

Bnd_B3d* SMESH_OctreeNode::buildRootBox()
{
  Bnd_B3d* box = new Bnd_B3d;
  set<const SMDS_MeshNode*>::iterator it = myNodes.begin();
  for (; it != myNodes.end(); it++) {
    const SMDS_MeshNode* n1 = *it;
    gp_XYZ p1( n1->X(), n1->Y(), n1->Z() );
    box->Add(p1);
  }
  if ( myNodes.size() <= myMaxNbNodes )
    myIsLeaf = true;

  return box;
}

//====================================================================================
/*!
 * \brief Tells us if Node is inside the current box with the precision "precision"
 * \param Node - Node
 * \param precision - The box is enlarged with this precision
 * \retval bool - True if Node is in the box within precision
 */
//====================================================================================

const bool SMESH_OctreeNode::isInside (const SMDS_MeshNode * Node, const double precision)
{
  gp_XYZ p (Node->X(),Node->Y(),Node->Z());
  if (precision <= 0.)
    return !(getBox().IsOut(p));
  Bnd_B3d BoxWithPrecision = getBox();
  BoxWithPrecision.Enlarge(precision);
  return ! BoxWithPrecision.IsOut(p);
}

//================================================
/*!
 * \brief Set the data of the children
 * Shares the father's data with each of his child
 */
//================================================
void SMESH_OctreeNode::buildChildrenData()
{
  gp_XYZ min = getBox().CornerMin();
  gp_XYZ max = getBox().CornerMax();
  gp_XYZ mid = (min + max)/2.;

  set<const SMDS_MeshNode*>::iterator it = myNodes.begin();
  while (it != myNodes.end())
  {
    const SMDS_MeshNode* n1 = *it;
    int ChildBoxNum = getChildIndex( n1->X(), n1->Y(), n1->Z(), mid );
    SMESH_OctreeNode* myChild = dynamic_cast<SMESH_OctreeNode*> (myChildren[ChildBoxNum]);
    myChild->myNodes.insert(myChild->myNodes.end(),n1);
    myNodes.erase( it );
    it = myNodes.begin();
  }
  for (int i = 0; i < 8; i++)
  {
    SMESH_OctreeNode* myChild = dynamic_cast<SMESH_OctreeNode*> (myChildren[i]);
    if ( myChild->myNodes.size() <= myMaxNbNodes )
      myChild->myIsLeaf = true;
  }
}

//===================================================================
/*!
 * \brief Return in Result a list of Nodes potentials to be near Node
 * \param Node - Node
 * \param precision - precision used
 * \param Result - list of Nodes potentials to be near Node
 */
//====================================================================
void SMESH_OctreeNode::NodesAround (const SMDS_MeshNode * Node,
                                    list<const SMDS_MeshNode*>* Result,
                                    const double precision)
{
  if (isInside(Node,precision))
  {
    if (isLeaf())
    {
      Result->insert(Result->end(), myNodes.begin(), myNodes.end());
    }
    else
    {
      for (int i = 0; i < 8; i++)
      {
        SMESH_OctreeNode* myChild = dynamic_cast<SMESH_OctreeNode*> (myChildren[i]);
        myChild->NodesAround(Node, Result, precision);
      }
    }
  }
}

//================================================================================
/*!
 * \brief Return in dist2Nodes nodes mapped to their square distance from Node
 *  \param node - node to find nodes closest to
 *  \param dist2Nodes - map of found nodes and their distances
 *  \param precision - radius of a sphere to check nodes inside
 *  \retval bool - true if an exact overlapping found
 */
//================================================================================

bool SMESH_OctreeNode::NodesAround(const SMDS_MeshNode *              node,
                                   map<double, const SMDS_MeshNode*>& dist2Nodes,
                                   double                             precision)
{
  if ( !dist2Nodes.empty() )
    precision = min ( precision, sqrt( dist2Nodes.begin()->first ));
  else if ( precision == 0. )
    precision = maxSize() / 2;

  if (isInside(node,precision))
  {
    if (!isLeaf())
    {
      // first check a child containing node
      gp_XYZ mid = (getBox().CornerMin() + getBox().CornerMax()) / 2.;
      int nodeChild  = getChildIndex( node->X(), node->Y(), node->Z(), mid );
      if ( ((SMESH_OctreeNode*) myChildren[nodeChild])->NodesAround(node, dist2Nodes, precision))
        return true;
      
      for (int i = 0; i < 8; i++)
        if ( i != nodeChild )
          if (((SMESH_OctreeNode*) myChildren[i])->NodesAround(node, dist2Nodes, precision))
            return true;
    }
    else if ( NbNodes() > 0 )
    {
      double minDist = precision * precision;
      gp_Pnt p1 ( node->X(), node->Y(), node->Z() );
      set<const SMDS_MeshNode*>::iterator nIt = myNodes.begin();
      for ( ; nIt != myNodes.end(); ++nIt )
      {
        gp_Pnt p2 ( (*nIt)->X(), (*nIt)->Y(), (*nIt)->Z() );
        double dist2 = p1.SquareDistance( p2 );
        if ( dist2 < minDist )
          dist2Nodes.insert( make_pair( minDist = dist2, *nIt ));
      }
//       if ( dist2Nodes.size() > 1 ) // leave only closest node in dist2Nodes
//         dist2Nodes.erase( ++dist2Nodes.begin(), dist2Nodes.end());

      return ( sqrt( minDist) <= precision * 1e-12 );
    }
  }
  return false;
}

//=============================
/*!
 * \brief  Return in theGroupsOfNodes a list of group of nodes close to each other within theTolerance
 * Search for all the nodes in theSetOfNodes
 * Static Method : no need to create an SMESH_OctreeNode
 * \param theSetOfNodes - set of nodes we look at, modified during research
 * \param theGroupsOfNodes - list of nodes closed to each other returned
 * \param theTolerance - Precision used, default value is 0.00001
 * \param maxLevel - Maximum level for SMESH_OctreeNode constructed, default value is -1 (Infinite)
 * \param maxNbNodes - maximum Nodes in a Leaf of the SMESH_OctreeNode constructed, default value is 5
 */
//=============================
void SMESH_OctreeNode::FindCoincidentNodes (set<const SMDS_MeshNode*>& theSetOfNodes,
                                            list< list< const SMDS_MeshNode*> >* theGroupsOfNodes,
                                            const double theTolerance,
                                            const int maxLevel,
                                            const int maxNbNodes)
{
  SMESH_OctreeNode theOctreeNode(theSetOfNodes, maxLevel, maxNbNodes, theTolerance);
  theOctreeNode.FindCoincidentNodes (&theSetOfNodes, theTolerance, theGroupsOfNodes);
}

//=============================
/*!
 * \brief  Return in theGroupsOfNodes a list of group of nodes close to each other within theTolerance
 * Search for all the nodes in theSetOfNodes
 * \note  The Octree itself is also modified by this method
 * \param theSetOfNodes - set of nodes we look at, modified during research
 * \param theTolerance - Precision used
 * \param theGroupsOfNodes - list of nodes closed to each other returned
 */
//=============================
void SMESH_OctreeNode::FindCoincidentNodes ( set<const SMDS_MeshNode*>* theSetOfNodes,
                                             const double               theTolerance,
                                             list< list< const SMDS_MeshNode*> >* theGroupsOfNodes)
{
  set<const SMDS_MeshNode*>::iterator it1 = theSetOfNodes->begin();
  list<const SMDS_MeshNode*>::iterator it2;

  while (it1 != theSetOfNodes->end())
  {
    const SMDS_MeshNode * n1 = *it1;

    list<const SMDS_MeshNode*> ListOfCoincidentNodes;// Initialize the lists via a declaration, it's enough

    list<const SMDS_MeshNode*> * groupPtr = 0;

    // Searching for Nodes around n1 and put them in ListofCoincidentNodes.
    // Found nodes are also erased from theSetOfNodes
    FindCoincidentNodes(n1, theSetOfNodes, &ListOfCoincidentNodes, theTolerance);

    // We build a list {n1 + his neigbours} and add this list in theGroupsOfNodes
    for (it2 = ListOfCoincidentNodes.begin(); it2 != ListOfCoincidentNodes.end(); it2++)
    {
      const SMDS_MeshNode* n2 = *it2;
      if ( !groupPtr )
      {
        theGroupsOfNodes->push_back( list<const SMDS_MeshNode*>() );
        groupPtr = & theGroupsOfNodes->back();
        groupPtr->push_back( n1 );
      }
      if (groupPtr->front() > n2)
        groupPtr->push_front( n2 );
      else
        groupPtr->push_back( n2 );
    }
    if (groupPtr != 0)
      groupPtr->sort();

    theSetOfNodes->erase(it1);
    it1 = theSetOfNodes->begin();
  }
}

//======================================================================================
/*!
 * \brief Return a list of nodes closed to Node and remove it from SetOfNodes
 * \note  The Octree itself is also modified by this method
 * \param Node - We're searching the nodes next to him.
 * \param SetOfNodes - set of nodes in which we erase the found nodes
 * \param Result - list of nodes closed to Node
 * \param precision - Precision used
 */
//======================================================================================
void SMESH_OctreeNode::FindCoincidentNodes (const SMDS_MeshNode * Node,
                                            set<const SMDS_MeshNode*>* SetOfNodes,
                                            list<const SMDS_MeshNode*>* Result,
                                            const double precision)
{
  bool isInsideBool = isInside(Node,precision);

  if (isInsideBool)
  {
    // I'm only looking in the leaves, since all the nodes are stored there.
    if (isLeaf())
    {
      gp_Pnt p1 (Node->X(), Node->Y(), Node->Z());

      set<const SMDS_MeshNode*> myNodesCopy = myNodes;
      set<const SMDS_MeshNode*>::iterator it = myNodesCopy.begin();
      double tol2 = precision * precision;
      bool squareBool;

      while (it != myNodesCopy.end())
      {
        const SMDS_MeshNode* n2 = *it;
        // We're only looking at nodes with a superior Id.
        // JFA: Why?
        //if (Node->GetID() < n2->GetID())
        if (Node->GetID() != n2->GetID()) // JFA: for bug 0020185
        {
          gp_Pnt p2 (n2->X(), n2->Y(), n2->Z());
          // Distance optimized computation
          squareBool = (p1.SquareDistance( p2 ) <= tol2);

          // If n2 inside the SquareDistance, we add it in Result and remove it from SetOfNodes and myNodes
          if (squareBool)
          {
            Result->insert(Result->begin(), n2);
            SetOfNodes->erase( n2 );
            myNodes.erase( n2 );
          }
        }
        //myNodesCopy.erase( it );
        //it = myNodesCopy.begin();
        it++;
      }
      if (Result->size() > 0)
        myNodes.erase(Node); // JFA: for bug 0020185
    }
    else
    {
      // If I'm not a leaf, I'm going to see my children !
      for (int i = 0; i < 8; i++)
      {
        SMESH_OctreeNode* myChild = dynamic_cast<SMESH_OctreeNode*> (myChildren[i]);
        myChild->FindCoincidentNodes(Node, SetOfNodes, Result, precision);
      }
    }
  }
}

//================================================================================
/*!
 * \brief Update data according to node movement
 */
//================================================================================

void SMESH_OctreeNode::UpdateByMoveNode( const SMDS_MeshNode* node, const gp_Pnt& toPnt )
{
  if ( isLeaf() )
  {
    set<const SMDS_MeshNode*>::iterator pNode = myNodes.find( node );
    bool nodeInMe = ( pNode != myNodes.end() );

    SMDS_MeshNode pointNode( toPnt.X(), toPnt.Y(), toPnt.Z() );
    bool pointInMe = isInside( &pointNode, 1e-10 );

    if ( pointInMe != nodeInMe )
    {
      if ( pointInMe )
        myNodes.insert( node );
      else
        myNodes.erase( node );
    }
  }
  else if ( myChildren )
  {
    gp_XYZ mid = (getBox().CornerMin() + getBox().CornerMax()) / 2.;
    int nodeChild  = getChildIndex( node->X(), node->Y(), node->Z(), mid );
    int pointChild = getChildIndex( toPnt.X(), toPnt.Y(), toPnt.Z(), mid );
    if ( nodeChild != pointChild )
    {
      ((SMESH_OctreeNode*) myChildren[ nodeChild  ])->UpdateByMoveNode( node, toPnt );
      ((SMESH_OctreeNode*) myChildren[ pointChild ])->UpdateByMoveNode( node, toPnt );
    }
  }
}

//================================================================================
/*!
 * \brief Return iterator over children
 */
//================================================================================
SMESH_OctreeNodeIteratorPtr SMESH_OctreeNode::GetChildrenIterator()
{
  return SMESH_OctreeNodeIteratorPtr
    ( new SMDS_SetIterator< SMESH_OctreeNode*, SMESH_Octree** >
      ( myChildren, (( isLeaf() || !myChildren ) ? myChildren : &myChildren[ 8 ] )));
}

//================================================================================
/*!
 * \brief Return nodes iterator
 */
//================================================================================
SMDS_NodeIteratorPtr SMESH_OctreeNode::GetNodeIterator()
{
  return SMDS_NodeIteratorPtr
    ( new SMDS_SetIterator< SMDS_pNode, set< SMDS_pNode >::const_iterator >
      ( myNodes.begin(), myNodes.size() ? myNodes.end() : myNodes.begin()));
}
