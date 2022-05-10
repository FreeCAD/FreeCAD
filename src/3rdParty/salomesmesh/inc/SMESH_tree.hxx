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

//  SMESH SMESH_Tree : tree implementation
//  File      : SMESH_Tree.hxx
//  Created   : Tue Jan 16 16:00:00 2007
//  Author    : Nicolas Geimer & Aurélien Motteux (OCC)
//  Module    : SMESH
//
#ifndef _SMESH_Tree_HXX_
#define _SMESH_Tree_HXX_

#include "SMESH_Utils.hxx"

//================================================================================
// Data limiting the tree height
struct SMESH_TreeLimit {
  // MaxLevel of the Tree
  int    myMaxLevel;
  // Minimal size of the Box
  double myMinBoxSize;

  // Default:
  // maxLevel-> 8^8 = 16777216 terminal trees at most
  // minSize -> box size not checked
  SMESH_TreeLimit(int maxLevel=8, double minSize=0.):myMaxLevel(maxLevel),myMinBoxSize(minSize) {}
  virtual ~SMESH_TreeLimit() {} // it can be inherited
};

//================================================================================
/*!
 * \brief Base class for 2D and 3D trees
 */
//================================================================================

template< class BND_BOX,
          int   NB_CHILDREN>
class SMESH_Tree
{
 public:

  typedef BND_BOX box_type;

  // Constructor. limit must be provided at tree root construction.
  // limit will be deleted by SMESH_Tree
  SMESH_Tree (SMESH_TreeLimit* limit=0);

  // Destructor
  virtual ~SMESH_Tree ();

  // Compute the Tree. Must be called by constructor of inheriting class
  void                   compute();

  // Tell if Tree is a leaf or not.
  // An inheriting class can influence it via myIsLeaf protected field
  bool                   isLeaf() const;

  // Return its level
  int                    level() const { return myLevel; }

  // Return Bounding Box of the Tree
  const box_type*        getBox() const { return myBox; }

  // Return height of the tree, full or from this level to topest leaf
  int                    getHeight(const bool full=true) const;

  static int             nbChildren() { return NB_CHILDREN; }

  // Compute the biggest dimension of my box
  virtual double         maxSize() const = 0;

protected:
  // Return box of the whole tree
  virtual box_type*      buildRootBox() = 0;

  // Allocate a child
  virtual SMESH_Tree*    newChild() const = 0;

  // Allocate a bndbox according to childIndex. childIndex is zero based
  virtual box_type*      newChildBox(int childIndex) const = 0;

  // Fill in data of the children
  virtual void           buildChildrenData() = 0;

  // members

  // Array of children
  SMESH_Tree**   myChildren;

  // Point the father, NULL for the level 0
  SMESH_Tree*    myFather;

  // Tell us if the Tree is a leaf or not
  bool           myIsLeaf;

  // Tree limit
  const SMESH_TreeLimit* myLimit;

  // Bounding box of a tree
  box_type*      myBox;

  // Level of the Tree
  int            myLevel;

  // Build the children recursively
  void                   buildChildren();
};

//===========================================================================
/*!
 * Constructor. limit must be provided at tree root construction.
 * limit will be deleted by SMESH_Tree.
 */
//===========================================================================

template< class BND_BOX, int NB_CHILDREN>
SMESH_Tree<BND_BOX,NB_CHILDREN>::SMESH_Tree (SMESH_TreeLimit* limit):
  myChildren(0),
  myFather(0),
  myIsLeaf( false ),
  myLimit( limit ),
  myLevel(0),
  myBox(0)
{
  //if ( !myLimit ) myLimit = new SMESH_TreeLimit();
}

//================================================================================
/*!
 * \brief Compute the Tree
 */
//================================================================================

template< class BND_BOX, int NB_CHILDREN>
void SMESH_Tree<BND_BOX,NB_CHILDREN>::compute()
{
  if ( myLevel==0 )
  {
    if ( !myLimit ) myLimit = new SMESH_TreeLimit();
    myBox = buildRootBox();
    if ( myLimit->myMinBoxSize > 0. && maxSize() <= myLimit->myMinBoxSize )
      myIsLeaf = true;
    else
      buildChildren();
  }
}

//======================================
/*!
 * \brief SMESH_Tree Destructor
 */
//======================================

template< class BND_BOX, int NB_CHILDREN>
SMESH_Tree<BND_BOX,NB_CHILDREN>::~SMESH_Tree ()
{
  if ( myChildren )
  {
    if ( !isLeaf() )
    {
      for(int i = 0; i<NB_CHILDREN; i++)
        delete myChildren[i];
      delete[] myChildren;
      myChildren = 0;
    }
  }
  if ( myBox )
    delete myBox;
  myBox = 0;
  if ( level() == 0 )
    delete myLimit;
  myLimit = 0;
}

//=================================================================
/*!
 * \brief Build the children boxes and call buildChildrenData()
 */
//=================================================================

template< class BND_BOX, int NB_CHILDREN>
void SMESH_Tree<BND_BOX,NB_CHILDREN>::buildChildren()
{
  if ( isLeaf() ) return;

  myChildren = new SMESH_Tree*[NB_CHILDREN];

  // get the whole model size
  double rootSize = 0;
  {
    SMESH_Tree* root = this;
    while ( root->myLevel > 0 )
      root = root->myFather;
    rootSize = root->maxSize();
  }
  for (int i = 0; i < NB_CHILDREN; i++)
  {
    // The child is of the same type than its father (For instance, a SMESH_OctreeNode)
    // We allocate the memory we need for the child
    myChildren[i] = newChild();
    // and we assign to him its box.
    myChildren[i]->myFather = this;
    if (myChildren[i]->myLimit)
      delete myChildren[i]->myLimit;
    myChildren[i]->myLimit = myLimit;
    myChildren[i]->myLevel = myLevel + 1;
    myChildren[i]->myBox = newChildBox( i );
    myChildren[i]->myBox->Enlarge( rootSize * 1e-10 );
    if ( myLimit->myMinBoxSize > 0. && myChildren[i]->maxSize() <= myLimit->myMinBoxSize )
      myChildren[i]->myIsLeaf = true;
  }

  // After building the NB_CHILDREN boxes, we put the data into the children.
  buildChildrenData();

  //After we pass to the next level of the Tree
  for (int i = 0; i<NB_CHILDREN; i++)
    myChildren[i]->buildChildren();
}

//================================================================================
/*!
 * \brief Tell if Tree is a leaf or not
 *        An inheriting class can influence it via myIsLeaf protected field
 */
//================================================================================

template< class BND_BOX, int NB_CHILDREN>
bool SMESH_Tree<BND_BOX,NB_CHILDREN>::isLeaf() const
{
  return myIsLeaf || ((myLimit->myMaxLevel > 0) ? (level() >= myLimit->myMaxLevel) : false );
}

//================================================================================
/*!
 * \brief Return height of the tree, full or from this level to topest leaf
 */
//================================================================================

template< class BND_BOX, int NB_CHILDREN>
int SMESH_Tree<BND_BOX,NB_CHILDREN>::getHeight(const bool full) const
{
  if ( full && myFather )
    return myFather->getHeight( true );

  if ( isLeaf() )
    return 1;

  int height = 0;
  for (int i = 0; i<NB_CHILDREN; i++)
  {
    int h = myChildren[i]->getHeight( false );
    if ( h > height)
        height = h;
  }
  return height + 1;
}

#endif
