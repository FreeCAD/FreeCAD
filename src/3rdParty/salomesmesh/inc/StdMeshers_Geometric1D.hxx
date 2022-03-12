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

//  SMESH SMESH : implementation of SMESH idl descriptions
//  File   : StdMeshers_Geometric1D.hxx
//  Module : SMESH
//
#ifndef _SMESH_Geometric1D_HXX_
#define _SMESH_Geometric1D_HXX_

#include "SMESH_StdMeshers.hxx"

#include "StdMeshers_Reversible1D.hxx"
#include "Utils_SALOME_Exception.hxx"

class STDMESHERS_EXPORT StdMeshers_Geometric1D: public StdMeshers_Reversible1D
{
public:
  StdMeshers_Geometric1D(int hypId, int studyId, SMESH_Gen* gen);

  void SetStartLength(double length);
  void SetCommonRatio(double factor);

  double GetStartLength() const;
  double GetCommonRatio() const;

  virtual std::ostream & SaveTo(std::ostream & save);
  virtual std::istream & LoadFrom(std::istream & load);

  /*!
   * \brief Initialize start and end length by the mesh built on the geometry
    * \param theMesh - the built mesh
    * \param theShape - the geometry of interest
    * \retval bool - true if parameter values have been successfully defined
   */
  virtual bool SetParametersByMesh(const SMESH_Mesh* theMesh, const TopoDS_Shape& theShape);

  /*!
   * \brief Initialize my parameter values by default parameters.
   *  \retval bool - true if parameter values have been successfully defined
   */
  virtual bool SetParametersByDefaults(const TDefaults& dflts, const SMESH_Mesh* theMesh=0);

protected:
  double _begLength, _ratio;
};

#endif
