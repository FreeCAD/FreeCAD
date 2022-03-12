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

//  SMESH StdMeshers : implementation of SMESH idl descriptions
//  File   : StdMeshers_QuadraticMesh.hxx
//  Module : SMESH
//
#ifndef _StdMeshers_QuadraticMesh_HXX_
#define _StdMeshers_QuadraticMesh_HXX_

#include "SMESH_StdMeshers.hxx"

#include "SMESH_Hypothesis.hxx"
#include "Utils_SALOME_Exception.hxx"

/*!
 * \brief Hypothesis for StdMeshers_Regular_1D, forcing construction of quadratic edges.
 * If the 2D mesher sees that all boundary edges are quadratic ones,
 * it generates quadratic faces, else it generates linear faces using
 * medium nodes as if they were vertex ones.
 * The 3D mesher generates quadratic volumes only if all boundary faces
 * are quadratic ones, else it fails.
 */
class STDMESHERS_EXPORT StdMeshers_QuadraticMesh:public SMESH_Hypothesis
{
 public:
  StdMeshers_QuadraticMesh(int hypId, int studyId, SMESH_Gen * gen);
  virtual ~ StdMeshers_QuadraticMesh();
  
  virtual std::ostream & SaveTo(std::ostream & save);
  virtual std::istream & LoadFrom(std::istream & load);
  friend std::ostream & operator <<(std::ostream & save, StdMeshers_QuadraticMesh & hyp);
  friend std::istream & operator >>(std::istream & load, StdMeshers_QuadraticMesh & hyp);

  /*!
   * \brief Initialize my parameter values by the mesh built on the geometry
    * \param theMesh - the built mesh
    * \param theShape - the geometry of interest
    * \retval bool - true if parameter values have been successfully defined
    *
    * Just return false as this hypothesis does not have parameters values
   */
  virtual bool SetParametersByMesh(const SMESH_Mesh* theMesh, const TopoDS_Shape& theShape);

  /*!
   * \brief Initialize my parameter values by default parameters.
   *  \retval bool - true if parameter values have been successfully defined
   */
  virtual bool SetParametersByDefaults(const TDefaults& dflts, const SMESH_Mesh* theMesh=0);

};

#endif
