// Copyright (C) 2007-2016  CEA/DEN, EDF R&D, OPEN CASCADE
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

//  SMESH SMESHDS : management of mesh data and SMESH document
//  File   : SMESHDS_Hypothesis.cxx
//  Author : Paul RASCLE, EDF
//  Module : SMESH
//
#include "SMESHDS_Hypothesis.hxx"

#include <sstream>

using namespace std;

//=============================================================================
/*!
 * 
 */
//=============================================================================

SMESHDS_Hypothesis::SMESHDS_Hypothesis(int hypId)
{
  _hypId = hypId;
  _name = "generic";
}

//=============================================================================
/*!
 * 
 */
//=============================================================================

SMESHDS_Hypothesis::~SMESHDS_Hypothesis()
{
}

//=============================================================================
/*!
 * 
 */
//=============================================================================

const char* SMESHDS_Hypothesis::GetName() const
{
  return _name.c_str();
}

//=============================================================================
/*!
 * 
 */
//=============================================================================

int SMESHDS_Hypothesis::GetID() const
{
  return _hypId;
}

//=============================================================================
/*!
 * 
 */
//=============================================================================

int SMESHDS_Hypothesis::GetType() const
{
  return _type;
}

//=============================================================================
/*!
 * Equality
 */
//=============================================================================

bool SMESHDS_Hypothesis::operator==(const SMESHDS_Hypothesis& other) const
{
  if ( this == &other )
    return true;
  if ( _name != other._name )
    return false;
  ostringstream mySave, otherSave;
  ((SMESHDS_Hypothesis*)this  )->SaveTo(mySave);
  ((SMESHDS_Hypothesis*)&other)->SaveTo(otherSave);
  return mySave.str() == otherSave.str();
}

//================================================================================
/*!
 * \brief Compare types of hypotheses
 */
//================================================================================

bool SMESHDS_Hypothesis::IsSameName( const SMESHDS_Hypothesis& other) const
{
  return _name == other._name;
}
