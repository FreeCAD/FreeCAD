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
//  File   : StdMeshers_ImportSource1D.hxx
//  Module : SMESH
//
#ifndef _StdMeshers_ImportSource_HXX_
#define _StdMeshers_ImportSource_HXX_

#include "SMESH_StdMeshers.hxx"

#include "SMESH_Hypothesis.hxx"
#include "Utils_SALOME_Exception.hxx"

#include <vector>
#include <map>

class SMESH_Group;
class SMESHDS_Mesh;
class SMESH_subMesh;

//==============================================================================
/*!
 * \brief Stores groups to import elements from
 */
//==============================================================================

class STDMESHERS_EXPORT StdMeshers_ImportSource1D : public SMESH_Hypothesis
{
 public:
  StdMeshers_ImportSource1D(int hypId, int studyId, SMESH_Gen * gen);
  virtual ~ StdMeshers_ImportSource1D();

  void SetGroups(const std::vector<SMESH_Group*>& groups);
  const std::vector<SMESH_Group*>& GetGroups(bool loaded=false) const;

  void SetCopySourceMesh(bool toCopyMesh, bool toCopyGroups);
  void GetCopySourceMesh(bool& toCopyMesh, bool& toCopyGroups) const;
  
  virtual std::ostream & SaveTo(std::ostream & save);
  virtual std::istream & LoadFrom(std::istream & load);
  virtual bool SetParametersByMesh(const SMESH_Mesh* theMesh, const TopoDS_Shape& theShape);
  virtual bool SetParametersByDefaults(const TDefaults& dflts, const SMESH_Mesh* theMesh=0);
  virtual bool DataDependOnParams() const { return true; }
  void RestoreGroups(const std::vector<SMESH_Group*>& groups);

  void StoreResultGroups(const std::vector<SMESH_Group*>& groups,
                         const SMESHDS_Mesh&              srcMesh,
                         const SMESHDS_Mesh&              tgtMesh);
  std::vector<SMESH_Group*>* GetResultGroups(const SMESHDS_Mesh& srcMesh,
                                             const SMESHDS_Mesh& tgtMesh);

  std::vector<SMESH_Mesh*> GetSourceMeshes() const;
  std::vector<SMESH_subMesh*> GetSourceSubMeshes(const SMESH_Mesh* srcMesh) const;

private:

  std::vector<SMESH_Group*> _groups;
  bool _toCopyMesh, _toCopyGroups;

  // groups imported using this hypothesis
  typedef std::map< std::pair<int, int>, std::vector<SMESH_Group*> > TResGroupMap;
  TResGroupMap      _resultGroups;
  std::vector<int>  _resultGroupsStorage; // persistent representation of _resultGroups

  void resultGroupsToIntVec();
};

//==============================================================================
/*!
 * \brief Redefines name and dimension of inherited StdMeshers_ImportSource1D
 */
//==============================================================================

class STDMESHERS_EXPORT StdMeshers_ImportSource2D : public StdMeshers_ImportSource1D
{
 public:
  StdMeshers_ImportSource2D(int hypId, int studyId, SMESH_Gen * gen);
};
#endif
