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
//  SMESH SMESH : implementaion of SMESH idl descriptions
//  File   : StdMeshers_QuadrangleParams.hxx
//  Author : Sergey KUUL, OCC
//  Module : SMESH
//
#ifndef _SMESH_QUADRANGLEPARAMS_HXX_
#define _SMESH_QUADRANGLEPARAMS_HXX_



#include "SMESH_StdMeshers.hxx"

#include "SMESH_Hypothesis.hxx"
#include "Utils_SALOME_Exception.hxx"

class STDMESHERS_EXPORT StdMeshers_QuadrangleParams:
  public SMESH_Hypothesis
{
public:
  StdMeshers_QuadrangleParams(int hypId, int studyId, SMESH_Gen* gen);
  virtual ~StdMeshers_QuadrangleParams();

  void SetTriaVertex(int id);

  void SetObjectEntry( const char* entry ) { _objEntry = entry; }

  const char* GetObjectEntry() { return _objEntry.c_str(); }

  int GetTriaVertex() const { return _triaVertexID; }

  virtual std::ostream & SaveTo(std::ostream & save);
  virtual std::istream & LoadFrom(std::istream & load);
  friend std::ostream& operator << (std::ostream & save,
				    StdMeshers_QuadrangleParams & hyp);
  friend std::istream& operator >> (std::istream & load,
				    StdMeshers_QuadrangleParams & hyp);

  /*!
   * \brief Initialize start and end length by the mesh built on the geometry
    * \param theMesh - the built mesh
    * \param theShape - the geometry of interest
    * \retval bool - true if parameter values have been successfully defined
   */
  virtual bool SetParametersByMesh(const SMESH_Mesh* theMesh,
				   const TopoDS_Shape& theShape);

  /*!
   * \brief Initialize my parameter values by default parameters.
   *  \retval bool - true if parameter values have been successfully defined
   */
  virtual bool SetParametersByDefaults(const TDefaults& dflts,
				       const SMESH_Mesh* theMesh=0);

protected:
  int         _triaVertexID;
  std::string _objEntry;
};

#endif
