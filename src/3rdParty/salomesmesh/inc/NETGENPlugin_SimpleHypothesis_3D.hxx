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

//  NETGENPlugin : C++ implementation
// File      : NETGENPlugin_SimpleHypothesis_3D.hxx
// Author    : Edward AGAPOV
// Project   : SALOME
//=============================================================================
//
#ifndef _NETGENPlugin_SimpleHypothesis_3D_HXX_
#define _NETGENPlugin_SimpleHypothesis_3D_HXX_

#include "NETGENPlugin_Defs.hxx"
#include "NETGENPlugin_SimpleHypothesis_2D.hxx"

#include <Utils_SALOME_Exception.hxx>

//  Simplified parameters of NETGEN
//

using namespace std;

class NETGENPLUGIN_EXPORT NETGENPlugin_SimpleHypothesis_3D: public NETGENPlugin_SimpleHypothesis_2D
{
public:

  NETGENPlugin_SimpleHypothesis_3D(int hypId, int studyId, SMESH_Gen * gen);

  void LengthFromFaces();

  void SetMaxElementVolume(double value);
  double GetMaxElementVolume() const { return _volume; }

  // Persistence
  virtual ostream & SaveTo(ostream & save);
  virtual istream & LoadFrom(istream & load);

  /*!
   * \brief Set parameters by mesh
   * \param theMesh - the built mesh
   * \param theShape - the geometry of interest
   * \retval bool - true if theShape is meshed
   */
  virtual bool SetParametersByMesh(const SMESH_Mesh* theMesh, const TopoDS_Shape& theShape);

private:
  double _volume;
};

#endif
