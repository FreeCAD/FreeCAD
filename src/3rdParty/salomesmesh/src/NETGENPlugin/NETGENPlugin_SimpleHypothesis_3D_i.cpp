// Copyright (C) 2007-2015  CEA/DEN, EDF R&D, OPEN CASCADE
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

//  NETGENPlugin : C++ implementation
// File      : NETGENPlugin_SimpleHypothesis_3D_i.cxx
// Author    : Edward AGAPOV
// Project   : SALOME
//=============================================================================
//
#include "NETGENPlugin_SimpleHypothesis_3D_i.hxx"
#include "NETGENPlugin_SimpleHypothesis_3D.hxx"

#include <SMESH_Gen.hxx>
#include <SMESH_PythonDump.hxx>

#include <Utils_CorbaException.hxx>
#include <utilities.h>

using namespace std;

//=============================================================================
/*!
 *  NETGENPlugin_SimpleHypothesis_3D_i::NETGENPlugin_SimpleHypothesis_3D_i
 *
 *  Constructor
 */
//=============================================================================
NETGENPlugin_SimpleHypothesis_3D_i::
NETGENPlugin_SimpleHypothesis_3D_i (PortableServer::POA_ptr thePOA,
                                    int                     theStudyId,
                                    ::SMESH_Gen*            theGenImpl)
  : SALOME::GenericObj_i( thePOA ),
    SMESH_Hypothesis_i( thePOA ),
    NETGENPlugin_SimpleHypothesis_2D_i( thePOA,theStudyId,theGenImpl )
{
  MESSAGE( "NETGENPlugin_SimpleHypothesis_3D_i::NETGENPlugin_SimpleHypothesis_3D_i" );
  if ( myBaseImpl )
    delete myBaseImpl;
  myBaseImpl = new ::NETGENPlugin_SimpleHypothesis_3D (theGenImpl->GetANewId(),
                                                       theStudyId,
                                                       theGenImpl);
}

//=============================================================================
/*!
 *  NETGENPlugin_SimpleHypothesis_3D_i::~NETGENPlugin_SimpleHypothesis_3D_i
 *
 *  Destructor
 */
//=============================================================================
NETGENPlugin_SimpleHypothesis_3D_i::~NETGENPlugin_SimpleHypothesis_3D_i()
{
  MESSAGE( "NETGENPlugin_SimpleHypothesis_3D_i::~NETGENPlugin_SimpleHypothesis_3D_i" );
}

//=============================================================================
/*!
 *  NETGENPlugin_SimpleHypothesis_3D_i::LengthFromFaces()
 */
//=============================================================================
void NETGENPlugin_SimpleHypothesis_3D_i::LengthFromFaces()
{
  MESSAGE("NETGENPlugin_SimpleHypothesis_3D_i::LengthFromFaces");
  ASSERT(myBaseImpl);
  this->GetImpl()->LengthFromFaces();
  SMESH::TPythonDump() << _this() << ".LengthFromFaces()";
}

//=============================================================================
/*!
 *  NETGENPlugin_SimpleHypothesis_3D_i::SetMaxElementVolume()
 */
//=============================================================================
void NETGENPlugin_SimpleHypothesis_3D_i::SetMaxElementVolume(CORBA::Double value)
{
  MESSAGE("NETGENPlugin_SimpleHypothesis_3D_i::SetMaxElementVolume");
  ASSERT(myBaseImpl);
  this->GetImpl()->SetMaxElementVolume(value);
  SMESH::TPythonDump() << _this() << ".SetMaxElementVolume( " << SMESH::TVar(value) << " )";
}


//=============================================================================
/*!
 *  NETGENPlugin_SimpleHypothesis_3D_i::GetMaxElementVolume()
 */
//=============================================================================
CORBA::Double NETGENPlugin_SimpleHypothesis_3D_i::GetMaxElementVolume()
{
  MESSAGE("NETGENPlugin_SimpleHypothesis_3D_i::GetMaxElementVolume");
  ASSERT(myBaseImpl);
  return this->GetImpl()->GetMaxElementVolume();
}
//=============================================================================
/*!
 *  NETGENPlugin_SimpleHypothesis_3D_i::GetImpl
 */
//=============================================================================
::NETGENPlugin_SimpleHypothesis_3D* NETGENPlugin_SimpleHypothesis_3D_i::GetImpl()
{
  MESSAGE("NETGENPlugin_SimpleHypothesis_3D_i::GetImpl");
  return (::NETGENPlugin_SimpleHypothesis_3D*)myBaseImpl;
}

//================================================================================
/*!
 * \brief Verify whether hypothesis supports given entity type 
  * \param type - dimension (see SMESH::Dimension enumeration)
  * \retval CORBA::Boolean - TRUE if dimension is supported, FALSE otherwise
 * 
 * Verify whether hypothesis supports given entity type (see SMESH::Dimension enumeration)
 */
//================================================================================  
CORBA::Boolean NETGENPlugin_SimpleHypothesis_3D_i::IsDimSupported( SMESH::Dimension type )
{
  return type == SMESH::DIM_3D;
}
