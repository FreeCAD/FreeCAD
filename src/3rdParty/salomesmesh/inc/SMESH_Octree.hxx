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
// File      : SMESH_Octree.hxx
// Created   : Tue Jan 16 16:00:00 2007
// Author    : Nicolas Geimer & Aurélien Motteux (OCC)
// Module    : SMESH
//
#ifndef _SMESH_OCTREE_HXX_
#define _SMESH_OCTREE_HXX_

#include <Bnd_B3d.hxx>

class SMESH_Octree {

public:
  // Constructor
  SMESH_Octree (const int maxLevel = -1, const double minBoxSize = 0.);

  // Destructor
  virtual ~SMESH_Octree ();

  // Tell if Octree is a leaf or not (has to be implemented in inherited classes)
  virtual const bool     isLeaf() = 0;

  // Compute the Octree
  void                   Compute();

  // Set the maximal level of the Octree
  void                   setMaxLevel(const int maxLevel);

  // Set the minimal size of the Box
  void                   setMinBoxSize(const double minBoxSize){myMinBoxSize = minBoxSize;};

  // Set the bounding box of the Octree
  void                   setBox(const Bnd_B3d* box);

  // Set box to the 3d Bounding Box of the Octree
  void                   getBox(Bnd_B3d & box);

  // Compute the bigger dimension of the box
  static double          maxSize(const Bnd_B3d* box);

  // Return its level
  int                    level() const { return myLevel; }

protected:
  // Constructor for children (has to be implemented in inherited classes)
  virtual SMESH_Octree* allocateOctreeChild() = 0;

  // Build the 8 children boxes
  void buildChildren();

  // Build the data in the 8 children (has to be implemented in inherited classes)
  virtual void buildChildrenData() = 0;

  // members

  // Box of the Octree
  Bnd_B3d*       myBox;

  // Array of 8 Octree children
  SMESH_Octree** myChildren;

  // Point the father, set to NULL for the level 0
  SMESH_Octree*  myFather;

  // Level of the Octree
  int            myLevel;

  // MaxLevel of the Octree
  int            myMaxLevel;

  // Minimal size of the Box
  double         myMinBoxSize;

  // Tell us if the Octree is a leaf or not (-1 if not initialized)
  int            myIsLeaf;
};
#endif
