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
//  File   : StdMeshers_Adaptive1D.hxx
//  Module : SMESH
//
#ifndef _StdMeshers_Adaptive1D_HXX_
#define _StdMeshers_Adaptive1D_HXX_

#include "SMESH_StdMeshers.hxx"

#include "StdMeshers_Regular_1D.hxx"

#include "Utils_SALOME_Exception.hxx"

/*!
 * \brief Adaptive 1D hypothesis
 */
class STDMESHERS_EXPORT StdMeshers_Adaptive1D : public SMESH_Hypothesis
{
 public:
  StdMeshers_Adaptive1D(int hypId, int studyId, SMESH_Gen * gen);
  ~StdMeshers_Adaptive1D();

  /*!
   * Sets minimal allowed segment length
   */
  void SetMinSize( double minSegLen );
  double GetMinSize() const { return myMinSize; }

  /*!
   * Sets maximal allowed segment length
   */
  void SetMaxSize( double maxSegLen );
  double GetMaxSize() const { return myMaxSize; }

  /*!
   * Sets <deflection> parameter value, 
   * i.e. a maximal allowed distance between a segment and an edge.
   */
  void SetDeflection(double value);
  double GetDeflection() const { return myDeflection; }
  
  virtual std::ostream & SaveTo(std::ostream & save);
  virtual std::istream & LoadFrom(std::istream & load);

  /*!
   * \brief Initialize deflection value by the mesh built on the geometry
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

  /*!
   * \brief Returns an algorithm that works using this hypothesis
   */
  SMESH_Algo* GetAlgo() const;

protected:

  double myMinSize, myMaxSize, myDeflection;
  SMESH_Algo* myAlgo; // StdMeshers_AdaptiveAlgo_1D implemented in cxx file
};

#endif
