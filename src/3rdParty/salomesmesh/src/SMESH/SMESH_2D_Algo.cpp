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
//  SMESH SMESH : implementaion of SMESH idl descriptions
//  File   : SMESH_2D_Algo.cxx
//  Author : Paul RASCLE, EDF
//  Module : SMESH
//  $Header: /home/server/cvs/SMESH/SMESH_SRC/src/SMESH/SMESH_2D_Algo.cxx,v 1.9.2.2 2008/11/27 12:25:15 abd Exp $
//
#include "SMESH_2D_Algo.hxx"
#include "SMESH_Gen.hxx"

#include "utilities.h"

#include <TopExp_Explorer.hxx>
#include <TopExp.hxx>
#include <TopoDS.hxx>

//=============================================================================
/*!
 *  
 */
//=============================================================================

SMESH_2D_Algo::SMESH_2D_Algo(int hypId, int studyId, SMESH_Gen* gen)
  : SMESH_Algo(hypId, studyId, gen)
{
//   _compatibleHypothesis.push_back("hypothese_2D_bidon");
  _type = ALGO_2D;
  gen->_map2D_Algo[hypId] = this;
}

//=============================================================================
/*!
 *  
 */
//=============================================================================

SMESH_2D_Algo::~SMESH_2D_Algo()
{
}

//=============================================================================
/*!
 *  
 */
//=============================================================================

int SMESH_2D_Algo::NumberOfWires(const TopoDS_Shape& S)
{
  int i = 0;
  for (TopExp_Explorer exp(S,TopAbs_WIRE); exp.More(); exp.Next())
    i++;
  return i;
}

//=============================================================================
/*!
 *  
 */
//=============================================================================

int SMESH_2D_Algo::NumberOfPoints(SMESH_Mesh& aMesh, const TopoDS_Wire& W)
{
  int nbPoints = 0;
  for (TopExp_Explorer exp(W,TopAbs_EDGE); exp.More(); exp.Next()) {
    const TopoDS_Edge& E = TopoDS::Edge(exp.Current());
    int nb = aMesh.GetSubMesh(E)->GetSubMeshDS()->NbNodes();
    if(_quadraticMesh)
      nb = nb/2;
    nbPoints += nb + 1; // internal points plus 1 vertex of 2 (last point ?)
  }
  return nbPoints;
}


