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
//  SMESH SMESHDS : management of mesh data and SMESH document
//  File   : SMESHDS_Hypothesis.cxx
//  Author : Paul RASCLE, EDF
//  Module : SMESH
//  $Header: /home/server/cvs/SMESH/SMESH_SRC/src/SMESHDS/SMESHDS_Hypothesis.cxx,v 1.10.2.1 2008/11/27 12:31:37 abd Exp $
//
#include "SMESHDS_Hypothesis.hxx"

using namespace std;

//=============================================================================
/*!
 * 
 */
//=============================================================================

SMESHDS_Hypothesis::SMESHDS_Hypothesis(int hypId)
{
//   MESSAGE("SMESHDS_Hypothesis::SMESHDS_Hypothesis");
  _hypId = hypId;
  _name = "generic";
//   SCRUTE(_name);
//   SCRUTE(_hypId);
}

//=============================================================================
/*!
 * 
 */
//=============================================================================

SMESHDS_Hypothesis::~SMESHDS_Hypothesis()
{
//   MESSAGE("SMESHDS_Hypothesis::~SMESHDS_Hypothesis");
}

//=============================================================================
/*!
 * 
 */
//=============================================================================

const char* SMESHDS_Hypothesis::GetName() const
{
//   MESSAGE("SMESHDS_Hypothesis::GetName");
//   SCRUTE(_name);
//   SCRUTE(&_name);
  return _name.c_str();
}

//=============================================================================
/*!
 * 
 */
//=============================================================================

int SMESHDS_Hypothesis::GetID() const
{
//   MESSAGE("SMESHDS_Hypothesis::GetId");
//   SCRUTE(_hypId);
  return _hypId;
}

//=============================================================================
/*!
 * 
 */
//=============================================================================

int SMESHDS_Hypothesis::GetType() const
{
//   MESSAGE("SMESHDS_Hypothesis::GetType");
//   SCRUTE(_type);
  return _type;
}

