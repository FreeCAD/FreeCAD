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

//  SMESH_Quadtree : Quartree implementation
//  File      : SMESH_Quadtree.hxx
//  Module    : SMESH
//
#ifndef _SMESH_Quadtree_HXX_
#define _SMESH_Quadtree_HXX_

#include "SMESH_Utils.hxx"
#include "SMESH_tree.hxx"
#include <Bnd_B2d.hxx>

/*!
 * \brief 2D tree of anything.
 * Methods to implement in a descendant are:
 * - Bnd_B2d*       buildRootBox(); // box of the whole tree
 * - descendant*    newChild() const; // a new child instance
 * - void           buildChildrenData(); // Fill in data of the children
 */
class SMESHUtils_EXPORT SMESH_Quadtree : public SMESH_Tree< Bnd_B2d, 4 >
{
public:
  typedef SMESH_Tree< Bnd_B2d, 4> TBaseTree;

  // Constructor. limit must be provided at tree root construction.
  // limit will be deleted by SMESH_Quadtree
  SMESH_Quadtree (SMESH_TreeLimit* limit=0);

  // Compute the bigger dimension of my box
  double                 maxSize() const;

  // Return index of a child the given point is in
  //inline int             getChildIndex(double x, double y, const gp_XY& boxMiddle)const;

 protected:

  // Allocate a bndbox according to childIndex. childIndex is zero based
  virtual Bnd_B2d*       newChildBox(int childIndex) const;
};

//================================================================================
/*!
 * \brief Return index of a child the given point is in
 */
//================================================================================

// inline int SMESH_Quadtree::getChildIndex(double x, double y, const gp_XY& mid) const
// {
//   return (x > mid.X()) + ( y > mid.Y())*2 + (z > mid.Z())*4;
// }

#endif
