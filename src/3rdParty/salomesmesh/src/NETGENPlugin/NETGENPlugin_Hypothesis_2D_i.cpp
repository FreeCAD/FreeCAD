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

//  NETGENPlugin : C++ implementation
// File      : NETGENPlugin_Hypothesis_2D_i.cxx
// Author    : Michael Sazonov (OCN)
// Date      : 03/04/2006
// Project   : SALOME
//=============================================================================
//
#include "NETGENPlugin_Hypothesis_2D_i.hxx"
#include "SMESH_Gen.hxx"
#include "SMESH_PythonDump.hxx"

#include "Utils_CorbaException.hxx"
#include "utilities.h"

using namespace std;

//=============================================================================
/*!
 *  NETGENPlugin_Hypothesis_2D_i::NETGENPlugin_Hypothesis_2D_i
 *
 *  Constructor
 */
//=============================================================================
NETGENPlugin_Hypothesis_2D_i::
NETGENPlugin_Hypothesis_2D_i (PortableServer::POA_ptr thePOA,
                              int                     theStudyId,
                              ::SMESH_Gen*            theGenImpl)
  : SALOME::GenericObj_i( thePOA ),
    SMESH_Hypothesis_i( thePOA ),
    NETGENPlugin_Hypothesis_i( thePOA, theStudyId, theGenImpl )
{
  MESSAGE( "NETGENPlugin_Hypothesis_2D_i::NETGENPlugin_Hypothesis_2D_i" );
  if (myBaseImpl)
    delete (::NETGENPlugin_Hypothesis*)myBaseImpl;
  myBaseImpl = new ::NETGENPlugin_Hypothesis_2D (theGenImpl->GetANewId(),
                                                 theStudyId,
                                                 theGenImpl);
}

//=============================================================================
/*!
 *  NETGENPlugin_Hypothesis_2D_i::~NETGENPlugin_Hypothesis_2D_i
 *
 *  Destructor
 */
//=============================================================================
NETGENPlugin_Hypothesis_2D_i::~NETGENPlugin_Hypothesis_2D_i()
{
  MESSAGE( "NETGENPlugin_Hypothesis_2D_i::~NETGENPlugin_Hypothesis_2D_i" );
}

//=============================================================================
/*!
 *  NETGENPlugin_Hypothesis_2D_i::SetQuadAllowed
 *
 *  Set QuadAllowed flag
 */
//=============================================================================
// void NETGENPlugin_Hypothesis_2D_i::SetQuadAllowed (CORBA::Boolean theValue)
// {
//   if ( NETGENPlugin_Hypothesis_i::isToSetParameter( GetQuadAllowed(),
//                                                     theValue,
//                                                     METH_SetQuadAllowed ))
//   {
//     this->GetImpl()->SetQuadAllowed(theValue);
//     SMESH::TPythonDump() << _this() << ".SetQuadAllowed( " << theValue << " )";
//   }
// }

//=============================================================================
/*!
 *  NETGENPlugin_Hypothesis_2D_i::GetQuadAllowed
 *
 *  Get QuadAllowed flag
 */
//=============================================================================
// CORBA::Boolean NETGENPlugin_Hypothesis_2D_i::GetQuadAllowed()
// {
//   return this->GetImpl()->GetQuadAllowed();
// }

//=============================================================================
/*!
 *  NETGENPlugin_Hypothesis_2D_i::GetImpl
 *
 *  Get implementation
 */
//=============================================================================
::NETGENPlugin_Hypothesis_2D* NETGENPlugin_Hypothesis_2D_i::GetImpl()
{
  return (::NETGENPlugin_Hypothesis_2D*)myBaseImpl;
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
CORBA::Boolean NETGENPlugin_Hypothesis_2D_i::IsDimSupported( SMESH::Dimension type )
{
  return type == SMESH::DIM_2D;
}
