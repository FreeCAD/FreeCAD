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

//  SMESH NETGENPlugin : implementaion of SMESH idl descriptions
//  File   : NETGENPlugin.cxx
//  Author : Julia DOROVSKIKH
//  Module : SMESH
//  $Header$
//
#include "utilities.h"

#include "NETGENPlugin_Hypothesis_2D_ONLY_i.hxx"
#include "NETGENPlugin_Hypothesis_2D_i.hxx"
#include "NETGENPlugin_Hypothesis_3D_i.hxx"
#include "NETGENPlugin_Hypothesis_i.hxx"
#include "NETGENPlugin_NETGEN_2D3D_i.hxx"
#include "NETGENPlugin_NETGEN_2D_ONLY_i.hxx"
#include "NETGENPlugin_NETGEN_2D_i.hxx"
#include "NETGENPlugin_NETGEN_3D_i.hxx"
#include "NETGENPlugin_SimpleHypothesis_2D_i.hxx"
#include "NETGENPlugin_SimpleHypothesis_3D_i.hxx"

template <class T> class NETGENPlugin_Creator_i:public HypothesisCreator_i<T>
{
  // as we have 'module NETGENPlugin' in NETGENPlugin_Algorithm.idl
  virtual std::string GetModuleName() { return "NETGENPlugin"; }
};

//=============================================================================
/*!
 *
 */
//=============================================================================

extern "C"
{
  NETGENPLUGIN_EXPORT
  GenericHypothesisCreator_i* GetHypothesisCreator (const char* aHypName)
  {
    MESSAGE("GetHypothesisCreator " << aHypName);

    GenericHypothesisCreator_i* aCreator = 0;

    // Hypotheses

    // Algorithms
    if (strcmp(aHypName, "NETGEN_3D") == 0)
      aCreator = new NETGENPlugin_Creator_i<NETGENPlugin_NETGEN_3D_i>;
    else if (strcmp(aHypName, "NETGEN_2D") == 0)
      aCreator = new NETGENPlugin_Creator_i<NETGENPlugin_NETGEN_2D_i>;
    else if (strcmp(aHypName, "NETGEN_2D_ONLY") == 0)
      aCreator = new NETGENPlugin_Creator_i<NETGENPlugin_NETGEN_2D_ONLY_i>;
    else if (strcmp(aHypName, "NETGEN_2D3D") == 0)
      aCreator = new NETGENPlugin_Creator_i<NETGENPlugin_NETGEN_2D3D_i>;
    // Hypotheses
    else if (strcmp(aHypName, "NETGEN_Parameters") == 0)
      aCreator = new NETGENPlugin_Creator_i<NETGENPlugin_Hypothesis_i>;
    else if (strcmp(aHypName, "NETGEN_Parameters_2D") == 0)
      aCreator = new NETGENPlugin_Creator_i<NETGENPlugin_Hypothesis_2D_i>;
    else if (strcmp(aHypName, "NETGEN_Parameters_3D") == 0)
      aCreator = new NETGENPlugin_Creator_i<NETGENPlugin_Hypothesis_3D_i>;
    else if (strcmp(aHypName, "NETGEN_Parameters_2D_ONLY") == 0)
      aCreator = new NETGENPlugin_Creator_i<NETGENPlugin_Hypothesis_2D_ONLY_i>;
    else if (strcmp(aHypName, "NETGEN_SimpleParameters_2D") == 0)
      aCreator = new NETGENPlugin_Creator_i<NETGENPlugin_SimpleHypothesis_2D_i>;
    else if (strcmp(aHypName, "NETGEN_SimpleParameters_3D") == 0)
      aCreator = new NETGENPlugin_Creator_i<NETGENPlugin_SimpleHypothesis_3D_i>;
    else ;

    return aCreator;
  }
}
