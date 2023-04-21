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
//  File   : StdMeshers_AutomaticLength.hxx
//  Author : Edward AGAPOV, OCC
//  Module : SMESH
//
#ifndef _SMESH_AutomaticLength_HXX_
#define _SMESH_AutomaticLength_HXX_

#include "SMESH_StdMeshers.hxx"

#include "SMESH_Hypothesis.hxx"
#include "Utils_SALOME_Exception.hxx"

#include <map>

class SMESH_Mesh;
class TopoDS_Shape;
class TopoDS_TShape;

/*!
 * \brief 1D Hypothesis to compute segment length free of thinking
 *
 * It computes segment length basing on max shape size to shortest edge length ratio:
 * S = S0 * f(L/Lmin) where f(x) = 1 + (2/Pi * 7 * atan(x/5) )
 */

class STDMESHERS_EXPORT StdMeshers_AutomaticLength:public SMESH_Hypothesis
{
public:
  StdMeshers_AutomaticLength(int hypId, int studyId, SMESH_Gen * gen);
  virtual ~ StdMeshers_AutomaticLength();

  /*!
   * \brief Computes segment for a given edge
   */
  double GetLength(const SMESH_Mesh* aMesh, const TopoDS_Shape& anEdge);

  /*!
   * \brief Computes segment length for an edge of given length
   */
  double GetLength(const SMESH_Mesh* aMesh, const double edgeLength);

  /*!
   * \brief Set Fineness
    * \param theFineness - The Fineness value [0.0-1.0],
    *                        0 - coarse mesh
    *                        1 - fine mesh
   * 
   * Raise if theFineness is out of range
   * The "Initial Number of Elements on the Shortest Edge" (S0)
   * is divided by (0.5 + 4.5 x theFineness)
   */
  void SetFineness(double theFineness);

  /*!
   * \brief Return mesh Fineness
    * \retval double - Fineness value [0.0-1.0]
   */
  double GetFineness() const { return _fineness; }

  virtual std::ostream & SaveTo(std::ostream & save);
  virtual std::istream & LoadFrom(std::istream & load);
  friend std::ostream & operator <<(std::ostream & save, StdMeshers_AutomaticLength & hyp);
  friend std::istream & operator >>(std::istream & load, StdMeshers_AutomaticLength & hyp);

  /*!
   * \brief Initialize Fineness by the mesh built on the geometry
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
  std::map<const TopoDS_TShape*, double> _TShapeToLength;
  const SMESH_Mesh* _mesh;
  double _fineness, _S0, _minLen;
};

#endif
