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
//  File   : StdMeshers_Propagation.hxx
//  Module : SMESH
//
#ifndef _SMESH_PROPAGATION_HXX_
#define _SMESH_PROPAGATION_HXX_

#include "SMESH_StdMeshers.hxx"

#include "SMESH_Hypothesis.hxx"
#include "SMESH_subMeshEventListener.hxx"
#include "Utils_SALOME_Exception.hxx"

#include <TopoDS_Edge.hxx>

class SMESH_HypoFilter;

// =======================================================================
/*!
 * \brief Propagation hypothesis
 */
// =======================================================================

class STDMESHERS_EXPORT StdMeshers_Propagation : public SMESH_Hypothesis
{
 public:
  StdMeshers_Propagation(int hypId, int studyId, SMESH_Gen * gen);
  virtual ~ StdMeshers_Propagation();

  virtual std::ostream & SaveTo(std::ostream & save);
  virtual std::istream & LoadFrom(std::istream & load);

  static std::string GetName();

  /*!
   * \brief Returns a filter selecting both StdMeshers_Propagation and
   *        StdMeshers_PropagOfDistribution hypotheses
   */
  static const SMESH_HypoFilter& GetFilter();

  /*!
   * \brief Set EventListener managing propagation of hypotheses
    * \param subMesh - edge submesh to set event listener on
   * 
   * 1D algo is expected to call this method from it's SetEventListener()
   */
  static void SetPropagationMgr(SMESH_subMesh* subMesh);

  /*!
   * \brief Return an edge from which hypotheses are propagated
    * \param theMesh - mesh
    * \param theEdge - edge to which hypotheses are propagated
    * \retval TopoDS_Edge - source edge, also passing orientation
   */
  static TopoDS_Edge GetPropagationSource(SMESH_Mesh&         theMesh,
                                          const TopoDS_Shape& theEdge,
                                          bool&               isPropagOfDistribution );

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

// =======================================================================
/*!
 * \brief Propagation Of Distribution hypothesis
 */
// =======================================================================

class STDMESHERS_EXPORT StdMeshers_PropagOfDistribution: public StdMeshers_Propagation
{
 public:
  StdMeshers_PropagOfDistribution(int hypId, int studyId, SMESH_Gen * gen);

  static std::string GetName();
};

#endif
