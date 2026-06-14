// Copyright (C) 2007-2015  CEA/DEN, EDF R&D, OPEN CASCADE
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

//  SMESH SMESH : idl implementation based on 'SMESH' unit's classes
//  File   : StdMeshers_LayerDistribution2D.hxx
//  Author : Edward AGAPOV
//  Module : SMESH
//
#ifndef _SMESH_LayerDistribution2D_HXX_
#define _SMESH_LayerDistribution2D_HXX_

#include "StdMeshers_LayerDistribution.hxx"


// =========================================================
// =========================================================
/*!
 * This hypothesis is used by "Radial quadrangle" algorithm.
 * It specifies 1D hypothesis defining distribution of segments
 * between the internal and the external surfaces.
 */
// =========================================================
// =========================================================

class STDMESHERS_EXPORT StdMeshers_LayerDistribution2D
                        :public StdMeshers_LayerDistribution
{
public:
  // Constructor
  StdMeshers_LayerDistribution2D(int hypId, int studyId, SMESH_Gen* gen);
  // Destructor
  virtual ~StdMeshers_LayerDistribution2D();

};

#endif

