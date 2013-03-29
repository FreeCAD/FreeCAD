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
//  NETGENPlugin : C++ implementation
// File   : NETGENPlugin_NETGEN_2D3D.cxx
// Author : Michael Sazonov (OCN)
// Date   : 20/03/2006
// Project   : SALOME
// $Header: /home/server/cvs/NETGENPLUGIN/NETGENPLUGIN_SRC/src/NETGENPlugin/NETGENPlugin_NETGEN_2D3D.cxx,v 1.4.2.2 2008/11/27 14:29:44 abd Exp $
//=============================================================================
//
#include "NETGENPlugin_NETGEN_2D3D.hxx"
#include "NETGENPlugin_Hypothesis.hxx"
#include "NETGENPlugin_SimpleHypothesis_2D.hxx"
#include "NETGENPlugin_Mesher.hxx"

#include <SMESH_Gen.hxx>
#include <SMESH_Mesh.hxx>
#include <SMESH_ControlsDef.hxx>
#include <SMESHDS_Mesh.hxx>
#include <utilities.h>

#include <list>

using namespace std;

//=============================================================================
/*!
 *  
 */
//=============================================================================

NETGENPlugin_NETGEN_2D3D::NETGENPlugin_NETGEN_2D3D(int hypId, int studyId,
                                                   SMESH_Gen* gen)
  : SMESH_3D_Algo(hypId, studyId, gen)
{
  MESSAGE("NETGENPlugin_NETGEN_2D3D::NETGENPlugin_NETGEN_2D3D");
  _name = "NETGEN_2D3D";
  _shapeType = (1 << TopAbs_SHELL) | (1 << TopAbs_SOLID);// 1 bit /shape type
  _compatibleHypothesis.push_back("NETGEN_Parameters");
  _compatibleHypothesis.push_back("NETGEN_SimpleParameters_3D");
  _requireDescretBoundary = false;
  _onlyUnaryInput = false;
  _hypothesis = NULL;
  _supportSubmeshes = true;
}

//=============================================================================
/*!
 *  
 */
//=============================================================================

NETGENPlugin_NETGEN_2D3D::~NETGENPlugin_NETGEN_2D3D()
{
  MESSAGE("NETGENPlugin_NETGEN_2D3D::~NETGENPlugin_NETGEN_2D3D");
}

//=============================================================================
/*!
 *  
 */
//=============================================================================

bool NETGENPlugin_NETGEN_2D3D::CheckHypothesis
                         (SMESH_Mesh& aMesh,
                          const TopoDS_Shape& aShape,
                          SMESH_Hypothesis::Hypothesis_Status& aStatus)
{
  MESSAGE("NETGENPlugin_NETGEN_2D3D::CheckHypothesis");

  _hypothesis = NULL;

  const list<const SMESHDS_Hypothesis*>& hyps = GetUsedHypothesis(aMesh, aShape);
  int nbHyp = hyps.size();
  if (!nbHyp)
  {
    aStatus = SMESH_Hypothesis::HYP_OK;
    return true;  // can work with no hypothesis
  }

  const SMESHDS_Hypothesis* theHyp = hyps.front(); // use only the first hypothesis

  string hypName = theHyp->GetName();

  if ( find( _compatibleHypothesis.begin(), _compatibleHypothesis.end(),
             hypName ) != _compatibleHypothesis.end() )
  {
    _hypothesis = theHyp;
    aStatus = SMESH_Hypothesis::HYP_OK;
  }
  else
  {
    aStatus = SMESH_Hypothesis::HYP_INCOMPATIBLE;
  }

  return aStatus == SMESH_Hypothesis::HYP_OK;
}

//=============================================================================
/*!
 *Here we are going to use the NETGEN mesher
 */
//=============================================================================

bool NETGENPlugin_NETGEN_2D3D::Compute(SMESH_Mesh&         aMesh,
                                       const TopoDS_Shape& aShape)
{
//   SMESHDS_Mesh* meshDS = aMesh.GetMeshDS();

  NETGENPlugin_Mesher mesher(&aMesh, aShape, true);
//   NETGENPlugin_Mesher mesher(meshDS, aShape, true);
  mesher.SetParameters(dynamic_cast<const NETGENPlugin_Hypothesis*>(_hypothesis));
  mesher.SetParameters(dynamic_cast<const NETGENPlugin_SimpleHypothesis_2D*>(_hypothesis));
  return mesher.Compute();
}
