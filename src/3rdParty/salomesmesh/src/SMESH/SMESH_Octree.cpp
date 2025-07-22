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

//  SMESH  SMESH_Octree : Octree implementation
//  File      : SMESH_Octree.cxx
//  Created   : Tue Jan 16 16:00:00 2007
//  Author    : Nicolas Geimer & Aurélien Motteux(OCC)
//  Module    : SMESH
//
#include "SMESH_Octree.hxx"

//===========================================================================
/*!
 * Constructor. limit must be provided at tree root construction.
 * limit will be deleted by SMESH_Octree.
 */
//===========================================================================

SMESH_Octree::SMESH_Octree (SMESH_TreeLimit* limit): TBaseTree( limit )
{
}

//=================================================================
/*!
 * \brief Allocate a bndbox according to childIndex. childIndex is zero based
 */
//=================================================================

Bnd_B3d* SMESH_Octree::newChildBox(int childIndex) const
{
  gp_XYZ min = getBox()->CornerMin();
  gp_XYZ max = getBox()->CornerMax();
  gp_XYZ HSize = (max - min)/2.;
  gp_XYZ childHsize = HSize/2.;

  gp_XYZ minChild( min.X() + childIndex%2     * HSize.X(),
                   min.Y() + (childIndex%4)/2 * HSize.Y(),
                   min.Z() + ( childIndex>=4 ) * HSize.Z());

  return new Bnd_B3d(minChild+childHsize,childHsize);
}

//===========================================================================
/*!
 * \brief Compute the bigger dimension of my box
 */
//===========================================================================

double SMESH_Octree::maxSize() const
{
  if ( getBox() && !getBox()->IsVoid() )
  {
    gp_XYZ min = getBox()->CornerMin();
    gp_XYZ max = getBox()->CornerMax();
    gp_XYZ Size = (max - min);
    double returnVal = (Size.X()>Size.Y())?Size.X():Size.Y();
    return (returnVal>Size.Z())?returnVal:Size.Z();
  }
  return 0.;
}
