//  SMESH SMESH : implementaion of SMESH idl descriptions
//
//  Copyright (C) 2003  OPEN CASCADE, EADS/CCR, LIP6, CEA/DEN,
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
// See http://www.salome-platform.org/ or email : webmaster.salome@opencascade.com

//  File   : StdMeshers_MaxLength.hxx
//  Module : SMESH

#ifndef _SMESH_MaxLength_HXX_
#define _SMESH_MaxLength_HXX_

#include "SMESH_StdMeshers.hxx"

#include "SMESH_Hypothesis.hxx"
#include "SMESH_Exception.hxx"

class STDMESHERS_EXPORT StdMeshers_MaxLength: public SMESH_Hypothesis
{
 public:
  StdMeshers_MaxLength(int hypId, int studyId, SMESH_Gen * gen);
  virtual ~ StdMeshers_MaxLength();

  void SetLength(double length) throw(SMESH_Exception);
  double GetLength() const;

  bool HavePreestimatedLength() const  { return _preestimated > 0.; }
  double GetPreestimatedLength() const { return _preestimated; }
  void SetPreestimatedLength(double length);

  void SetUsePreestimatedLength(bool toUse);
  bool GetUsePreestimatedLength() const;

  virtual std::ostream & SaveTo(std::ostream & save);
  virtual std::istream & LoadFrom(std::istream & load);

  /*!
   * \brief Initialize segment length by the mesh built on the geometry
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
  double _length, _preestimated;
  bool   _preestimation;
};

#endif
