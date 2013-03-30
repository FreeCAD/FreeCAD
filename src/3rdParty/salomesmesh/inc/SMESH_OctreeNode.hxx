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
//  File      : SMESH_OctreeNode.hxx
//  Created   : Tue Jan 16 16:00:00 2007
//  Author    : Nicolas Geimer & Aurélien Motteux  (OCC)
//  Module    : SMESH

#ifndef _SMESH_OCTREENODE_HXX_
#define _SMESH_OCTREENODE_HXX_

#include "SMESH_Octree.hxx"

#include <list>
#include <set>

#include "SMDS_ElemIterator.hxx"

//forward declaration
class SMDS_MeshNode;
class SMESH_OctreeNode;

typedef SMDS_Iterator<SMESH_OctreeNode*>            SMESH_OctreeNodeIterator;
typedef boost::shared_ptr<SMESH_OctreeNodeIterator> SMESH_OctreeNodeIteratorPtr;

class SMESH_OctreeNode : public SMESH_Octree{

public:

  // Constructor
  SMESH_OctreeNode (const std::set<const SMDS_MeshNode*>& theNodes, const int maxLevel = -1,
                    const int maxNbNodes = 5, const double minBoxSize = 0.);

//=============================
/*!
 * \brief Empty destructor
 */
//=============================
  virtual ~SMESH_OctreeNode () {};

  // Tells us if SMESH_OctreeNode is a leaf or not (-1 = not initialiazed)
  virtual const bool isLeaf();

  // Tells us if Node is inside the current box with the precision "precision"
  virtual const bool isInside(const SMDS_MeshNode * Node, const double precision = 0.);

  // Return in Result a list of Nodes potentials to be near Node
  void               NodesAround(const SMDS_MeshNode * Node,
                                 std::list<const SMDS_MeshNode*>* Result,
                                 const double precision = 0.);

  // Return in theGroupsOfNodes a list of group of nodes close to each other within theTolerance
  // Search for all the nodes in nodes
  void               FindCoincidentNodes ( std::set<const SMDS_MeshNode*>* nodes,
                                           const double                theTolerance,
                                           std::list< std::list< const SMDS_MeshNode*> >* theGroupsOfNodes);

  // Static method that return in theGroupsOfNodes a list of group of nodes close to each other within
  // theTolerance search for all the nodes in nodes
  static void        FindCoincidentNodes ( std::set<const SMDS_MeshNode*> nodes,
                                           std::list< std::list< const SMDS_MeshNode*> >* theGroupsOfNodes,
                                           const double theTolerance = 0.00001, const int maxLevel = -1,
                                           const int maxNbNodes = 5);
  /*!
   * \brief Return iterator over children
   */
  SMESH_OctreeNodeIteratorPtr GetChildrenIterator();
  /*!
   * \brief Return nodes iterator
   */
  SMDS_NodeIteratorPtr        GetNodeIterator();
  /*!
   * \brief Return nb nodes in a tree
   */
  int                         NbNodes() const { return myNbNodes; }

protected:

//=============================
/*!
 * \brief Empty constructor
 */
//=============================
  SMESH_OctreeNode (){};

  // Shares the father's data with each of his child
  virtual void          buildChildrenData();

  // Compute the bounding box of the whole set of nodes myNodes (only used for OctreeNode level 0)
  void                  computeBoxForFather();

  // Construct an empty SMESH_OctreeNode used by SMESH_Octree::buildChildren()
  virtual SMESH_Octree* allocateOctreeChild();

  // Return in result a list of nodes closed to Node and remove it from SetOfNodes
  void                  FindCoincidentNodes( const SMDS_MeshNode * Node,
                                             std::set<const SMDS_MeshNode*>* SetOfNodes,
                                             std::list<const SMDS_MeshNode*>* Result,
                                             const double precision);

  // The max number of nodes a leaf box can contain
  int                         myMaxNbNodes;

  // The set of nodes inside the box of the Octree (Empty if Octree is not a leaf)
  std::set<const SMDS_MeshNode*>   myNodes;

  // The number of nodes I have inside the box
  int                         myNbNodes;
};

#endif
