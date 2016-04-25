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
// File      : NETGENPlugin_SimpleHypothesis_2D_i.cxx
// Author    : Edward AGAPOV
// Project   : SALOME
//=============================================================================
//
#include "NETGENPlugin_SimpleHypothesis_2D_i.hxx"
#include "NETGENPlugin_SimpleHypothesis_2D.hxx"

#include <SMESH_Gen.hxx>
#include <SMESH_PythonDump.hxx>

#include <Utils_CorbaException.hxx>
#include <utilities.h>

#include <TCollection_AsciiString.hxx>

using namespace std;

//=============================================================================
/*!
 *  NETGENPlugin_SimpleHypothesis_2D_i::NETGENPlugin_SimpleHypothesis_2D_i
 *
 *  Constructor
 */
//=============================================================================
NETGENPlugin_SimpleHypothesis_2D_i::
NETGENPlugin_SimpleHypothesis_2D_i (PortableServer::POA_ptr thePOA,
                                    int                     theStudyId,
                                    ::SMESH_Gen*            theGenImpl)
  : SALOME::GenericObj_i( thePOA ),
    SMESH_Hypothesis_i( thePOA )
{
  MESSAGE( "NETGENPlugin_SimpleHypothesis_2D_i::NETGENPlugin_SimpleHypothesis_2D_i" );
  myBaseImpl = new ::NETGENPlugin_SimpleHypothesis_2D (theGenImpl->GetANewId(),
                                                       theStudyId,
                                                       theGenImpl);
}

//=============================================================================
/*!
 *  NETGENPlugin_SimpleHypothesis_2D_i::~NETGENPlugin_SimpleHypothesis_2D_i
 *
 *  Destructor
 */
//=============================================================================
NETGENPlugin_SimpleHypothesis_2D_i::~NETGENPlugin_SimpleHypothesis_2D_i()
{
  MESSAGE( "NETGENPlugin_SimpleHypothesis_2D_i::~NETGENPlugin_SimpleHypothesis_2D_i" );
}

//=============================================================================
/*!
 *  NETGENPlugin_SimpleHypothesis_2D_i::SetNumberOfSegments
 */
//=============================================================================
void NETGENPlugin_SimpleHypothesis_2D_i::SetNumberOfSegments(CORBA::Short nb)
  throw ( SALOME::SALOME_Exception )
{
  MESSAGE("NETGENPlugin_SimpleHypothesis_2D_i::SetNumberOfSegments");
  ASSERT(myBaseImpl);
  try {
    this->GetImpl()->SetNumberOfSegments(nb);
  }
  catch (SALOME_Exception& S_ex) {
    THROW_SALOME_CORBA_EXCEPTION( S_ex.what(), SALOME::BAD_PARAM );
  }
  SMESH::TPythonDump() << _this() << ".SetNumberOfSegments( " << SMESH::TVar(nb) << " )";
}

//=============================================================================
/*!
 *  NETGENPlugin_SimpleHypothesis_2D_i::GetNumberOfSegments()
 */
//=============================================================================
CORBA::Short NETGENPlugin_SimpleHypothesis_2D_i::GetNumberOfSegments()
{
  MESSAGE("NETGENPlugin_SimpleHypothesis_2D_i::GetNumberOfSegments");
  ASSERT(myBaseImpl);
  return this->GetImpl()->GetNumberOfSegments();
}

//================================================================================
/*!
 * LocalLength()
 */
//================================================================================

void NETGENPlugin_SimpleHypothesis_2D_i::SetLocalLength(CORBA::Double segmentLength)
{
  MESSAGE("NETGENPlugin_SimpleHypothesis_2D_i::SetLocalLength");
  ASSERT(myBaseImpl);
  try {
    this->GetImpl()->SetLocalLength(segmentLength);
  }
  catch (SALOME_Exception& S_ex) {
    THROW_SALOME_CORBA_EXCEPTION( S_ex.what(), SALOME::BAD_PARAM );
  }
  SMESH::TPythonDump() << _this() << ".SetLocalLength( " << SMESH::TVar(segmentLength) << " )";
}

//================================================================================
/*!
 * GetLocalLength()
 */
//================================================================================

CORBA::Double NETGENPlugin_SimpleHypothesis_2D_i::GetLocalLength()
{
  MESSAGE("NETGENPlugin_SimpleHypothesis_2D_i::GetLocalLength");
  ASSERT(myBaseImpl);
  return this->GetImpl()->GetLocalLength();
}

//=============================================================================
/*!
 *  NETGENPlugin_SimpleHypothesis_2D_i::LengthFromEdges()
 */
//=============================================================================
void NETGENPlugin_SimpleHypothesis_2D_i::LengthFromEdges()
{
  MESSAGE("NETGENPlugin_SimpleHypothesis_2D_i::LengthFromEdges");
  ASSERT(myBaseImpl);
  this->GetImpl()->LengthFromEdges();
  SMESH::TPythonDump() << _this() << ".LengthFromEdges()";
}

//=============================================================================
/*!
 *  NETGENPlugin_SimpleHypothesis_2D_i::SetMaxElementArea()
 */
//=============================================================================
void NETGENPlugin_SimpleHypothesis_2D_i::SetMaxElementArea(CORBA::Double area)
{
  MESSAGE("NETGENPlugin_SimpleHypothesis_2D_i::SetMaxElementArea");
  ASSERT(myBaseImpl);
  this->GetImpl()->SetMaxElementArea(area);
  SMESH::TPythonDump() << _this() << ".SetMaxElementArea( " << SMESH::TVar(area) << " )";
}


//=============================================================================
/*!
 *  NETGENPlugin_SimpleHypothesis_2D_i::GetMaxElementArea()
 */
//=============================================================================

CORBA::Double NETGENPlugin_SimpleHypothesis_2D_i::GetMaxElementArea()
{
  MESSAGE("NETGENPlugin_SimpleHypothesis_2D_i::GetMaxElementArea");
  ASSERT(myBaseImpl);
  return this->GetImpl()->GetMaxElementArea();
}

//=============================================================================
/*!
 * Enables/disables generation of quadrangular faces
 */
//=============================================================================

void NETGENPlugin_SimpleHypothesis_2D_i::SetAllowQuadrangles(CORBA::Boolean toAllow)
{
  ASSERT(myBaseImpl);
  SMESH::TPythonDump() << _this() << ".SetAllowQuadrangles( " << toAllow << " )";
  this->GetImpl()->SetAllowQuadrangles(toAllow);
}

//=============================================================================
/*!
 * Returns true if generation of quadrangular faces is enabled
 */
//=============================================================================

CORBA::Boolean NETGENPlugin_SimpleHypothesis_2D_i::GetAllowQuadrangles()
{
  return this->GetImpl()->GetAllowQuadrangles();
}
//=============================================================================
/*!
 *  NETGENPlugin_SimpleHypothesis_2D_i::GetImpl
 */
//=============================================================================
::NETGENPlugin_SimpleHypothesis_2D* NETGENPlugin_SimpleHypothesis_2D_i::GetImpl() const
{
  MESSAGE("NETGENPlugin_SimpleHypothesis_2D_i::GetImpl");
  return (::NETGENPlugin_SimpleHypothesis_2D*)myBaseImpl;
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
CORBA::Boolean NETGENPlugin_SimpleHypothesis_2D_i::IsDimSupported( SMESH::Dimension type )
{
  return type == SMESH::DIM_2D;
}

//================================================================================
/*!
 * \brief method intended to remove explicit treatment of Netgen hypotheses from SMESH_NoteBook
 */
//================================================================================

int NETGENPlugin_SimpleHypothesis_2D_i::getParamIndex(const TCollection_AsciiString& method,
                                                      int nbVars) const
{
  if ( method == "SetLocalLength"      ) return 0;
  if ( method == "SetNumberOfSegments" ) return 0;
  if ( method == "SetMaxElementArea"   ) return 1;
  if ( method == "LengthFromEdges"     ) return 10; // just to go to the next state
  if ( method == "SetMaxElementVolume" ) return 2;
  if ( method == "LengthFromFaces"     ) return 10; // just to go to the next state

  return SMESH_Hypothesis_i::getParamIndex( method, nbVars ); // return default value
}

//================================================================================
/*!
 * \brief Method used to convert variable parameters stored in an old study
 * into myMethod2VarParams. It should return a method name for an index of
 * variable parameters. Index is countered from zero
 */
//================================================================================

std::string NETGENPlugin_SimpleHypothesis_2D_i::getMethodOfParameter(const int paramIndex,
                                                                     int nbVars) const
{
  switch ( paramIndex ) {
  case 0: return GetImpl()->GetNumberOfSegments() ? "SetNumberOfSegments" : "SetLocalLength";
  case 1: return "SetMaxElementArea";
  case 2: return "SetMaxElementVolume";
  }
  return "";
}
