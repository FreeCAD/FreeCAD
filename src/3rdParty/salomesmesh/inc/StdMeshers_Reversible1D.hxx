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

//  SMESH SMESH : implementaion of SMESH idl descriptions
//  File   : StdMeshers_Reversible1D.hxx
//  Module : SMESH
//
#ifndef _SMESH_Reversible1D_HXX_
#define _SMESH_Reversible1D_HXX_

#include "SMESH_StdMeshers.hxx"
#include "SMESH_Hypothesis.hxx"

#include <vector>

/*!
 * \brief A base of reversible 1D hypotheses
 */
class STDMESHERS_EXPORT StdMeshers_Reversible1D : public SMESH_Hypothesis
{
public:
  StdMeshers_Reversible1D(int hypId, int studyId, SMESH_Gen* gen);

  void SetReversedEdges( const std::vector<int>& ids);

  void SetObjectEntry( const char* entry ) { _objEntry = entry; }

  const char* GetObjectEntry() { return _objEntry.c_str(); }

  const std::vector<int>& GetReversedEdges() const { return _edgeIDs; }

  virtual std::ostream & SaveTo(std::ostream & save);
  virtual std::istream & LoadFrom(std::istream & load);

protected:
  std::vector<int> _edgeIDs;
  std::string      _objEntry;
};

#endif
