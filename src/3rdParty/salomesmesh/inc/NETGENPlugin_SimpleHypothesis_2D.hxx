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
//  NETGENPlugin : C++ implementation
// File      : NETGENPlugin_SimpleHypothesis_2D.hxx
// Author    : Edward AGAPOV
// Project   : SALOME
//=============================================================================
//
#ifndef _NETGENPlugin_SimpleHypothesis_2D_HXX_
#define _NETGENPlugin_SimpleHypothesis_2D_HXX_

#include "NETGENPlugin_Defs.hxx"

#include "SMESH_Hypothesis.hxx"
#include "SMESH_Exception.hxx"

//  Simplified parameters of NETGEN
//

class NETGENPLUGIN_EXPORT NETGENPlugin_SimpleHypothesis_2D: public SMESH_Hypothesis
{
public:

  NETGENPlugin_SimpleHypothesis_2D(int hypId, int studyId, SMESH_Gen * gen);

  /*!
   * Sets <number of segments> value
   */
  void SetNumberOfSegments(int nb) throw (SMESH_Exception);
  /*!
   * Returns <number of segments> value.
   * Can be zero in case if LocalLength() has been set
   */
  int GetNumberOfSegments() const { return _nbSegments; }

  /*!
   * Sets <segment length> value
   */
  void SetLocalLength(double segmentLength) throw (SMESH_Exception);
  /*!
   * Returns <segment length> value.
   * Can be zero in case if NumberOfSegments() has been set
   */
  double GetLocalLength() const { return _segmentLength; }

  /*!
   * Sets <maximum element area> to be dependent on 1D discretization
   */
  void LengthFromEdges();

  /*!
   * Sets <maximum element area> value.
   * Zero or negative value means same as LengthFromEdges().
   */
  void SetMaxElementArea(double area);
  /*!
   * Returns <maximum element area> value.
   * Can be zero in case of LengthFromEdges()
   */
  double GetMaxElementArea() const { return _area; }

  // Persistence
  virtual std::ostream & SaveTo(std::ostream & save);
  virtual std::istream & LoadFrom(std::istream & load);

  /*!
   * \brief Set parameters by mesh
   * \param theMesh - the built mesh
   * \param theShape - the geometry of interest
   * \retval bool - true if theShape is meshed
   */
  virtual bool SetParametersByMesh(const SMESH_Mesh* theMesh, const TopoDS_Shape& theShape);
  
   /*!
   * \brief Initialize my parameter values by default parameters.
   *  \retval bool - true if parameter values have been successfully defined
   */
  virtual bool SetParametersByDefaults(const TDefaults& dflts, const SMESH_Mesh* theMesh=0);
  
private:
  int    _nbSegments;
  double _segmentLength, _area;
};

#endif
