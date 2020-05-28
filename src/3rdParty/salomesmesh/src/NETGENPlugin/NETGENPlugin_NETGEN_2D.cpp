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
// File   : NETGENPlugin_NETGEN_2D.cxx
// Author : Michael Sazonov (OCN)
// Date   : 20/03/2006
// Project   : SALOME
//=============================================================================
//
#include "NETGENPlugin_NETGEN_2D.hxx"
#include "NETGENPlugin_Hypothesis_2D.hxx"
#include "NETGENPlugin_SimpleHypothesis_2D.hxx"
#include "NETGENPlugin_Mesher.hxx"

#include <SMESHDS_Mesh.hxx>
#include <SMESH_ControlsDef.hxx>
#include <SMESH_Gen.hxx>
#include <SMESH_Mesh.hxx>
#include <StdMeshers_ViscousLayers2D.hxx>
#include <utilities.h>

#include <list>

namespace nglib {
#include <nglib.h>
}

// DLL_HEADER is re-defined in netgen headers
#if defined(__clang__)
# pragma clang diagnostic push
# pragma clang diagnostic ignored "-Wmacro-redefined"
#endif

#ifdef NETGEN_PYTHON
#undef NETGEN_PYTHON
#endif

#ifndef WIN32
#undef DLL_HEADER
#endif

#include <meshing.hpp>

#if defined(__clang__)
# pragma clang diagnostic pop
#endif

using namespace std;

//=============================================================================
/*!
 *  
 */
//=============================================================================

NETGENPlugin_NETGEN_2D::NETGENPlugin_NETGEN_2D(int hypId, int studyId,
                                               SMESH_Gen* gen)
  : SMESH_2D_Algo(hypId, studyId, gen)
{
  MESSAGE("NETGENPlugin_NETGEN_2D::NETGENPlugin_NETGEN_2D");
  _name = "NETGEN_2D";
  _shapeType = (1 << TopAbs_FACE); // 1 bit /shape type
  _compatibleHypothesis.push_back("NETGEN_Parameters_2D");
  _compatibleHypothesis.push_back("NETGEN_SimpleParameters_2D");
  _compatibleHypothesis.push_back( StdMeshers_ViscousLayers2D::GetHypType() );
  _requireDiscreteBoundary = false;
  _onlyUnaryInput          = false;
  _hypothesis              = NULL;
  _supportSubmeshes        = true;
}

//=============================================================================
/*!
 *  
 */
//=============================================================================

NETGENPlugin_NETGEN_2D::~NETGENPlugin_NETGEN_2D()
{
  MESSAGE("NETGENPlugin_NETGEN_2D::~NETGENPlugin_NETGEN_2D");
}

//=============================================================================
/*!
 *  
 */
//=============================================================================

bool NETGENPlugin_NETGEN_2D::CheckHypothesis (SMESH_Mesh&         aMesh,
                                              const TopoDS_Shape& aShape,
                                              Hypothesis_Status&  aStatus)
{
  _hypothesis        = NULL;
  _isViscousLayers2D = false;
  _mesher            = NULL;

  // can work with no hypothesis
  aStatus = SMESH_Hypothesis::HYP_OK;

  const list<const SMESHDS_Hypothesis*>& hyps = GetUsedHypothesis(aMesh, aShape, /*skipAux=*/false);
  list<const SMESHDS_Hypothesis*>::const_iterator h = hyps.begin();
  for ( ; h != hyps.end(); ++h )
  {
    const SMESHDS_Hypothesis* theHyp = *h;
    string hypName = theHyp->GetName();
    if ( hypName == StdMeshers_ViscousLayers2D::GetHypType() )
      _isViscousLayers2D = true;
    else if ( _hypothesis )
      aStatus = SMESH_Hypothesis::HYP_INCOMPATIBLE;
    else
      _hypothesis = theHyp;
  }

  if ( aStatus == HYP_OK && _isViscousLayers2D )
    error( StdMeshers_ViscousLayers2D::CheckHypothesis( aMesh, aShape, aStatus ));

  return aStatus == SMESH_Hypothesis::HYP_OK;
}

//=============================================================================
/*!
 *
 */
//=============================================================================

bool NETGENPlugin_NETGEN_2D::Compute(SMESH_Mesh&         aMesh,
                                     const TopoDS_Shape& aShape)
{
  netgen::multithread.terminate = 0;

  NETGENPlugin_Mesher mesher(&aMesh, aShape, /*is3D = */false);
  mesher.SetParameters(dynamic_cast<const NETGENPlugin_Hypothesis*>(_hypothesis));
  mesher.SetParameters(dynamic_cast<const NETGENPlugin_SimpleHypothesis_2D*>(_hypothesis));
  mesher.SetViscousLayers2DAssigned( _isViscousLayers2D );
  mesher.SetSelfPointer( &_mesher );
  return mesher.Compute();
}

//=============================================================================
/*!
 * Terminate Compute()
 */
//=============================================================================

void NETGENPlugin_NETGEN_2D::CancelCompute()
{
  SMESH_Algo::CancelCompute();
  netgen::multithread.terminate = 1;
}

//================================================================================
/*!
 * \brief Return progress of Compute() [0.,1]
 */
//================================================================================

double NETGENPlugin_NETGEN_2D::GetProgress() const
{
  return _mesher ? _mesher->GetProgress(this, &_progressTic, &_progress) : 0;
}

//=============================================================================
/*!
 *
 */
//=============================================================================

bool NETGENPlugin_NETGEN_2D::Evaluate(SMESH_Mesh&         aMesh,
                                      const TopoDS_Shape& aShape,
                                      MapShapeNbElems& aResMap)
{

  NETGENPlugin_Mesher mesher(&aMesh, aShape, false);
  mesher.SetParameters(dynamic_cast<const NETGENPlugin_Hypothesis*>(_hypothesis));
  mesher.SetParameters(dynamic_cast<const NETGENPlugin_SimpleHypothesis_2D*>(_hypothesis));
  return mesher.Evaluate(aResMap);
}
