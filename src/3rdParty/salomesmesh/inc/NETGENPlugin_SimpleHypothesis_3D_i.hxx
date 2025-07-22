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
// File      : NETGENPlugin_SimpleHypothesis_3D_i.hxx
// Author    : Edward AGAPOV
// Project   : SALOME
//=============================================================================
//
#ifndef _NETGENPlugin_SimpleHypothesis_3D_i_HXX_
#define _NETGENPlugin_SimpleHypothesis_3D_i_HXX_

#include "NETGENPlugin_Defs.hxx"
#include "NETGENPlugin_SimpleHypothesis_2D_i.hxx"

#include <SALOMEconfig.h>
#include CORBA_SERVER_HEADER(NETGENPlugin_Algorithm)

class SMESH_Gen;
class NETGENPlugin_SimpleHypothesis_3D;

// Simplified NETGEN parameters (3D case)

class NETGENPLUGIN_EXPORT  NETGENPlugin_SimpleHypothesis_3D_i:
  public virtual POA_NETGENPlugin::NETGENPlugin_SimpleHypothesis_3D,
  public virtual NETGENPlugin_SimpleHypothesis_2D_i
{
 public:
  // Constructor
  NETGENPlugin_SimpleHypothesis_3D_i (PortableServer::POA_ptr thePOA,
                                      int                     theStudyId,
                                      ::SMESH_Gen*            theGenImpl);
  // Destructor
  virtual ~NETGENPlugin_SimpleHypothesis_3D_i();

  void LengthFromFaces();

  void SetMaxElementVolume(CORBA::Double value);
  CORBA::Double GetMaxElementVolume();

  // Get implementation
  ::NETGENPlugin_SimpleHypothesis_3D* GetImpl();
  
  // Verify whether hypothesis supports given entity type 
  CORBA::Boolean IsDimSupported( SMESH::Dimension type );
};

#endif
