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
//  File   : StdMeshers_Reversible1D.cxx
//  Module : SMESH
//

#include "StdMeshers_Reversible1D.hxx"

//=============================================================================
/*!
 *  
 */
//=============================================================================

StdMeshers_Reversible1D::StdMeshers_Reversible1D(int hypId, int studyId, SMESH_Gen * gen)
  :SMESH_Hypothesis(hypId, studyId, gen)
{
  _param_algo_dim = 1; 
}

//=============================================================================
/*!
 *  
 */
//=============================================================================

void StdMeshers_Reversible1D::SetReversedEdges( const std::vector<int>& ids )
{
  if ( ids != _edgeIDs )
  {
    _edgeIDs = ids;
    NotifySubMeshesHypothesisModification();
  }
}

//=============================================================================
/*!
 *  
 */
//=============================================================================

std::ostream & StdMeshers_Reversible1D::SaveTo(std::ostream & save)
{
  save << " " << _edgeIDs.size() << " ";

  if ( !_edgeIDs.empty() )
  {
    for ( size_t i = 0; i < _edgeIDs.size(); i++)
      save << " " << _edgeIDs[i];
    save << " " << _objEntry << " ";
  }

  return save;
}

//=============================================================================
/*!
 *  
 */
//=============================================================================

std::istream & StdMeshers_Reversible1D::LoadFrom(std::istream & load)
{
  bool isOK;
  int intVal;

  isOK = (bool)(load >> intVal);
  if (isOK && intVal > 0) {
    _edgeIDs.reserve( intVal );
    for (int i = 0; i < _edgeIDs.capacity() && isOK; i++) {
      isOK = (bool)(load >> intVal);
      if ( isOK ) _edgeIDs.push_back( intVal );
    }
    isOK = (bool)(load >> _objEntry);
  }

  return load;
}
