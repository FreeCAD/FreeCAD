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
//  SMESH SMESH_Octree : global Octree implementation
//
//  File      : SMESH_Octree.hxx
//  Created   : Tue Jan 16 16:00:00 2007
//  Author    : Nicolas Geimer & Aurélien Motteux (OCC)
//  Module    : SMESH

#ifndef _SMESH_OCTREE_HXX_
#define _SMESH_OCTREE_HXX_

#include <Bnd_B3d.hxx>

class SMESH_Octree {

public:

  // Data limiting the tree height
  struct Limit {
    // MaxLevel of the Octree
    int    myMaxLevel;
    // Minimal size of the Box
    double myMinBoxSize;

    // Default:
    // maxLevel-> 8^8 = 16777216 terminal trees
    // minSize -> box size not checked
    Limit(int maxLevel=8, double minSize=0.):myMaxLevel(maxLevel),myMinBoxSize(minSize) {}
    virtual ~Limit() {} // it can be inherited
  };

  // Constructor. limit must be provided at tree root construction.
  // limit will be deleted by SMESH_Octree
  SMESH_Octree (Limit* limit=0);

  // Destructor
  virtual ~SMESH_Octree ();

  // Compute the Octree. Must be called by constructor of inheriting class
  void                   compute();

  // Tell if Octree is a leaf or not.
  // An inheriting class can influence it via myIsLeaf protected field
  bool                   isLeaf() const;

  // Return its level
  int                    level() const { return myLevel; }

  // Get box to the 3d Bounding Box of the Octree
  const Bnd_B3d&         getBox() const { return *myBox; }

  // Compute the bigger dimension of my box
  double                 maxSize() const;

  // Return index of a child the given point is in
  inline int             getChildIndex(double x, double y, double z, const gp_XYZ& boxMiddle)const;

protected:
  // Return box of the whole tree
  virtual Bnd_B3d*       buildRootBox() = 0;

  // Constructor for children
  virtual SMESH_Octree*  allocateOctreeChild() const = 0;

  // Build the data in the 8 children
  virtual void           buildChildrenData() = 0;

  // members

  // Array of 8 Octree children
  SMESH_Octree** myChildren;

  // Point the father, set to NULL for the level 0
  SMESH_Octree*  myFather;

  // Tell us if the Octree is a leaf or not
  bool           myIsLeaf;

  // Tree limit
  const Limit*   myLimit;

private:
  // Build the 8 children boxes recursively
  void                   buildChildren();

  // Level of the Octree
  int            myLevel;

  Bnd_B3d*       myBox;
};

//================================================================================
/*!
 * \brief Return index of a child the given point is in
 */
//================================================================================

inline int SMESH_Octree::getChildIndex(double x, double y, double z, const gp_XYZ& mid) const
{
  return (x > mid.X()) + ( y > mid.Y())*2 + (z > mid.Z())*4;
}

#endif
