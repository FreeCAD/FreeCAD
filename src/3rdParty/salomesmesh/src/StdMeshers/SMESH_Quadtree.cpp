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
//  File      : SMESH_Quadtree.cxx
//  Module    : SMESH
//
#include "SMESH_Quadtree.hxx"

//===========================================================================
/*!
 * Constructor. limit must be provided at tree root construction.
 * limit will be deleted by SMESH_Quadtree.
 */
//===========================================================================

SMESH_Quadtree::SMESH_Quadtree (SMESH_TreeLimit* limit): TBaseTree( limit )
{
}

//=================================================================
/*!
 * \brief Allocate a bndbox according to childIndex. childIndex is zero based
 */
//=================================================================

Bnd_B2d* SMESH_Quadtree::newChildBox(int childIndex) const
{
  gp_XY min = getBox()->CornerMin();
  gp_XY max = getBox()->CornerMax();
  gp_XY HSize = (max - min)/2.;
  gp_XY childHsize = HSize/2.;

  gp_XY minChild( min.X() + childIndex%2     * HSize.X(),
                  min.Y() + ( childIndex<2 ) * HSize.Y());

  return new Bnd_B2d(minChild+childHsize,childHsize);
}

//===========================================================================
/*!
 * \brief Compute the bigger dimension of my box
 */
//===========================================================================

double SMESH_Quadtree::maxSize() const
{
  if ( getBox() && !getBox()->IsVoid() )
  {
    gp_XY min = getBox()->CornerMin();
    gp_XY max = getBox()->CornerMax();
    gp_XY Size = (max - min);
    double returnVal = (Size.X()>Size.Y())?Size.X():Size.Y();
    return returnVal;
  }
  return 0.;
}
