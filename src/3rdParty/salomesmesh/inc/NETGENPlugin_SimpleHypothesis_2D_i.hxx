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
// File      : NETGENPlugin_SimpleHypothesis_2D_i.hxx
// Author    : Edward AGAPOV
// Project   : SALOME
//=============================================================================
//
#ifndef _NETGENPlugin_SimpleHypothesis_2D_i_HXX_
#define _NETGENPlugin_SimpleHypothesis_2D_i_HXX_

#include "NETGENPlugin_Defs.hxx"

#include <SALOMEconfig.h>
#include CORBA_SERVER_HEADER(NETGENPlugin_Algorithm)

#include "SMESH_Hypothesis_i.hxx"

class SMESH_Gen;
class NETGENPlugin_SimpleHypothesis_2D;

// Simplified NETGEN parameters (2D case)

class NETGENPLUGIN_EXPORT  NETGENPlugin_SimpleHypothesis_2D_i:
  public virtual POA_NETGENPlugin::NETGENPlugin_SimpleHypothesis_2D,
  public virtual SMESH_Hypothesis_i
{
 public:
  // Constructor
  NETGENPlugin_SimpleHypothesis_2D_i (PortableServer::POA_ptr thePOA,
                                      int                     theStudyId,
                                      ::SMESH_Gen*            theGenImpl);
  // Destructor
  virtual ~NETGENPlugin_SimpleHypothesis_2D_i();

  void SetNumberOfSegments(CORBA::Short nb) throw ( SALOME::SALOME_Exception );
  CORBA::Short GetNumberOfSegments();

  void SetLocalLength(CORBA::Double segmentLength);
  CORBA::Double GetLocalLength();


  void LengthFromEdges();

  void SetMaxElementArea(CORBA::Double area);
  CORBA::Double GetMaxElementArea();

  void SetAllowQuadrangles(CORBA::Boolean toAllow);
  CORBA::Boolean GetAllowQuadrangles();

  // Get implementation
  ::NETGENPlugin_SimpleHypothesis_2D* GetImpl() const;
  
  // Verify whether hypothesis supports given entity type 
  CORBA::Boolean IsDimSupported( SMESH::Dimension type );

 public:
  // method intended to remove explicit treatment of Netgen hypotheses from
  // SMESH_NoteBook to assure backward compatibility after implemeneting
  // issue 0021308: Remove hard-coded dependency of the external mesh plugins
  virtual int getParamIndex(const TCollection_AsciiString& method, int nbVars) const;

  // method used to convert variable parameters stored in an old study
  // into myMethod2VarParams. It should return a method name for an index of
  // variable parameters. Index is countered from zero
  virtual std::string getMethodOfParameter(const int paramIndex, int nbVars) const;
};

#endif
