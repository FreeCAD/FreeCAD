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

//  SMESH SMESH_OctreeNode : Octree with Nodes set
//  inherites global class SMESH_Octree
//  File      : SMESH_OctreeNode.hxx
//  Created   : Tue Jan 16 16:00:00 2007
//  Author    : Nicolas Geimer & Aurelien Motteux  (OCC)
//  Module    : SMESH
//
#ifndef _SMESH_OCTREENODE_HXX_
#define _SMESH_OCTREENODE_HXX_

#include "SMDS_ElemIterator.hxx"
#include "SMDS_MeshNode.hxx"
#include "SMESH_Octree.hxx"
#include "SMESH_Utils.hxx"

#include <gp_Pnt.hxx>

#include <list>
#include <set>
#include <map>
#include <vector>

//forward declaration
class SMDS_MeshNode;
class SMESH_OctreeNode;

typedef SMDS_Iterator<SMESH_OctreeNode*>              SMESH_OctreeNodeIterator;
typedef boost::shared_ptr<SMESH_OctreeNodeIterator>   SMESH_OctreeNodeIteratorPtr;
typedef std::set< const SMDS_MeshNode*, TIDCompare >  TIDSortedNodeSet;

class SMESHUtils_EXPORT SMESH_OctreeNode : public SMESH_Octree
{
 public:

  // Constructor
  SMESH_OctreeNode (const TIDSortedNodeSet& theNodes, const int maxLevel = 8,
                    const int maxNbNodes = 5, const double minBoxSize = 0.);

  // destructor
  virtual ~SMESH_OctreeNode () {};

  // Tells us if Node is inside the current box with the precision "precision"
  virtual bool isInside(const gp_XYZ& p, const double precision = 0.);

  // Return in Result a list of Nodes potentials to be near Node
  void               NodesAround(const SMDS_MeshNode *            node,
                                 std::list<const SMDS_MeshNode*>* result,
                                 const double                     precision = 0.);

  // Return in dist2Nodes nodes mapped to their square distance from Node
  bool               NodesAround(const gp_XYZ&                           point,
                                 std::map<double, const SMDS_MeshNode*>& dist2Nodes,
                                 double                                  precision);

  // Return a list of Nodes close to a point
  void               NodesAround(const gp_XYZ&                      point,
                                 std::vector<const SMDS_MeshNode*>& nodes,
                                 double                             precision);

  // Return in theGroupsOfNodes a list of group of nodes close to each other within theTolerance
  // Search for all the nodes in nodes
  void               FindCoincidentNodes ( TIDSortedNodeSet*           nodes,
                                           const double                theTolerance,
                                           std::list< std::list< const SMDS_MeshNode*> >* theGroupsOfNodes);

  // Static method that return in theGroupsOfNodes a list of group of nodes close to each other within
  // theTolerance search for all the nodes in nodes
  static void        FindCoincidentNodes ( TIDSortedNodeSet&                              nodes,
                                           std::list< std::list< const SMDS_MeshNode*> >* theGroupsOfNodes,
                                           const double theTolerance = 0.00001,
                                           const int maxLevel = -1,
                                           const int maxNbNodes = 5);
  /*!
   * \brief Update data according to node movement
   */
  void                        UpdateByMoveNode( const SMDS_MeshNode* node, const gp_Pnt& toPnt );
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
  int                         NbNodes() const { return myNodes.size(); }

protected:

  struct Limit : public SMESH_TreeLimit
  {
    int myMaxNbNodes;
    Limit(int maxLevel, double minSize, int maxNbNodes)
      :SMESH_TreeLimit(maxLevel, minSize), myMaxNbNodes(maxNbNodes) {}
  };

  int                   getMaxNbNodes() const;

  SMESH_OctreeNode();

  // Compute the bounding box of the whole set of nodes myNodes
  virtual Bnd_B3d*      buildRootBox();

  // Shares the father's data with each of his child
  virtual void          buildChildrenData();

  // Construct an empty SMESH_OctreeNode used by SMESH_Octree::buildChildren()
  virtual SMESH_Octree* newChild() const;

  // Return in result a list of nodes closed to Node and remove it from SetOfNodes
  void                  FindCoincidentNodes( const SMDS_MeshNode *            Node,
                                             TIDSortedNodeSet*                SetOfNodes,
                                             std::list<const SMDS_MeshNode*>* Result,
                                             const double                     precision);

  // The set of nodes inside the box of the Octree (Empty if Octree is not a leaf)
  TIDSortedNodeSet   myNodes;

};

#endif
