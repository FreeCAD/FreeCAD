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
// File      : NETGENPlugin_NETGEN_2D.hxx
// Author    : Michael Sazonov (OCN)
// Date      : 20/03/2006
// Project   : SALOME
//=============================================================================
//
#ifndef _NETGENPlugin_NETGEN_2D_HXX_
#define _NETGENPlugin_NETGEN_2D_HXX_

#include "NETGENPlugin_Defs.hxx"

#include "SMESH_Algo.hxx"
#include "SMESH_Mesh.hxx"

class NETGENPlugin_Mesher;

class NETGENPLUGIN_EXPORT NETGENPlugin_NETGEN_2D: public SMESH_2D_Algo
{
public:
  NETGENPlugin_NETGEN_2D(int hypId, int studyId, SMESH_Gen* gen);
  virtual ~NETGENPlugin_NETGEN_2D();

  virtual bool CheckHypothesis(SMESH_Mesh& aMesh,
                               const TopoDS_Shape& aShape,
                               SMESH_Hypothesis::Hypothesis_Status& aStatus);

  virtual bool Compute(SMESH_Mesh& aMesh,
                       const TopoDS_Shape& aShape);

  virtual void CancelCompute();

  virtual double GetProgress() const;


  virtual bool Evaluate(SMESH_Mesh& aMesh, const TopoDS_Shape& aShape,
                        MapShapeNbElems& aResMap);

protected:
  const SMESHDS_Hypothesis* _hypothesis;
  bool                      _isViscousLayers2D;
  NETGENPlugin_Mesher *     _mesher;
};

#endif
