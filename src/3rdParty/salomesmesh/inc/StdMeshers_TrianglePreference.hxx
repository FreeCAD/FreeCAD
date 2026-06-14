//  Copyright (C) 2007-2008  CEA/DEN, EDF R&D, OPEN CASCADE
//
//  Copyright (C) 2003-2007  OPEN CASCADE, EADS/CCR, LIP6, CEA/DEN,
//  CEDRAT, EDF R&D, LEG, PRINCIPIA R&D, BUREAU VERITAS
//
//  This library is free software; you can redistribute it and/or
//  modify it under the terms of the GNU Lesser General Public
//  License as published by the Free Software Foundation; either
//  version 2.1 of the License.
//
//  This library is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
//  Lesser General Public License for more details.
//
//  You should have received a copy of the GNU Lesser General Public
//  License along with this library; if not, write to the Free Software
//  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
//
//  See http://www.salome-platform.org/ or email : webmaster.salome@opencascade.com
//
//  SMESH StdMeshers : implementation of SMESH idl descriptions
//  File   : StdMeshers_TrianglePreference.hxx
//  Module : SMESH
//  $Header: /home/server/cvs/SMESH/SMESH_SRC/src/StdMeshers/Attic/StdMeshers_TrianglePreference.hxx,v 1.1.4.2 2008/11/27 13:03:50 abd Exp $
//
#ifndef _StdMeshers_TrianglePreference_HXX_
#define _StdMeshers_TrianglePreference_HXX_

#include "SMESH_StdMeshers.hxx"

#include "SMESH_Hypothesis.hxx"
#include "SMESH_Exception.hxx"

/*!
 * \brief Hypothesis for StdMeshers_Quadrangle_2D, forcing construction
 *        of triangles in the in a refinement area if the number of nodes 
 *        on opposite edges is not the same. See Issue 16186.
 */
class STDMESHERS_EXPORT StdMeshers_TrianglePreference:public SMESH_Hypothesis
{
 public:
  StdMeshers_TrianglePreference(int hypId, int studyId, SMESH_Gen * gen);
  virtual ~ StdMeshers_TrianglePreference();
  
  virtual std::ostream & SaveTo(std::ostream & save);
  virtual std::istream & LoadFrom(std::istream & load);
  friend std::ostream & operator <<(std::ostream & save, StdMeshers_TrianglePreference & hyp);
  friend std::istream & operator >>(std::istream & load, StdMeshers_TrianglePreference & hyp);

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
