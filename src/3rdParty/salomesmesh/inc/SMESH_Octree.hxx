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

//  SMESH SMESH_Octree : Octree implementation
//  File      : SMESH_Octree.hxx
//  Created   : Tue Jan 16 16:00:00 2007
//  Author    : Nicolas Geimer & Aurélien Motteux (OCC)
//  Module    : SMESH
//
#ifndef _SMESH_OCTREE_HXX_
#define _SMESH_OCTREE_HXX_

#include "SMESH_Utils.hxx"
#include "SMESH_tree.hxx"
#include <Bnd_B3d.hxx>

//================================================================================
/*!
 * \brief 3D tree of anything.
 * Methods to implement in a descendant are:
 * - Bnd_B3d*       buildRootBox(); // box of the whole tree
 * - descendant*    newChild() const; // a new child instance
 * - void           buildChildrenData(); // Fill in data of the children
 */
class SMESHUtils_EXPORT SMESH_Octree : public SMESH_Tree< Bnd_B3d, 8 >
{
public:
  typedef SMESH_Tree< Bnd_B3d, 8> TBaseTree;

  // Constructor. limit must be provided at tree root construction.
  // limit will be deleted by SMESH_Octree
  SMESH_Octree (SMESH_TreeLimit* limit=0);
  virtual ~SMESH_Octree() {};

  // Compute the bigger dimension of my box
  double                 maxSize() const;

  // Return index of a child the given point is in
  inline static int      getChildIndex(double x, double y, double z, const gp_XYZ& boxMiddle);

 protected:

  // Allocate a bndbox according to childIndex. childIndex is zero based
  virtual Bnd_B3d*       newChildBox(int childIndex) const;
};

//================================================================================
/*!
 * \brief Return index of a child the given point is in
 */
inline int SMESH_Octree::getChildIndex(double x, double y, double z, const gp_XYZ& mid)
{
  return (x > mid.X()) + ( y > mid.Y())*2 + (z > mid.Z())*4;
}

#endif
