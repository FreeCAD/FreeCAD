// Copyright (C) 2007-2016  CEA/DEN, EDF R&D, OPEN CASCADE
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
// File      : DriverGMF_Read.hxx
// Created   : Mon Sep 17 15:36:47 2012
// Author    : Edward AGAPOV (eap)


#ifndef __DriverGMF_Read_HXX__
#define __DriverGMF_Read_HXX__

#include "SMESH_DriverGMF.hxx"

#include "Driver_SMESHDS_Mesh.h"

#include <vector>
#include <string>

/*!
 * \brief Driver reading a mesh from the GMF file. The mesh to read is selected by 
 *  an index (counted form 0) set via SetMeshId()
 */
class MESHDriverGMF_EXPORT DriverGMF_Read : public Driver_SMESHDS_Mesh
{
public:

  DriverGMF_Read();
  ~DriverGMF_Read();

  void SetMakeRequiredGroups( bool theMakeRequiredGroups )
  {
    _makeRequiredGroups = theMakeRequiredGroups;
  }

  virtual Status Perform();

 private:

  Status storeBadNodeIds(const char* gmfKwd, int elemNb, int nb, ...);

  bool _makeRequiredGroups;

};


#endif
