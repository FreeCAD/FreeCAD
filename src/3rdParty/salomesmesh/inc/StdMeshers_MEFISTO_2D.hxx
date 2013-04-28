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
//  File   : StdMeshers_MEFISTO_2D.hxx
//           Moved here from SMESH_MEFISTO_2D.hxx
//  Author : Paul RASCLE, EDF
//  Module : SMESH
//  $Header: /home/server/cvs/SMESH/SMESH_SRC/src/StdMeshers/StdMeshers_MEFISTO_2D.hxx,v 1.9.2.2 2008/11/27 13:03:49 abd Exp $
//
#ifndef _StdMeshers_MEFISTO_2D_HXX_
#define _StdMeshers_MEFISTO_2D_HXX_

#include "SMESH_StdMeshers.hxx"

#include "SMESH_2D_Algo.hxx"

class TopoDS_Face;
class StdMeshers_MaxElementArea;
class StdMeshers_LengthFromEdges;
class SMDS_MeshNode;
class SMESH_MesherHelper;
class StdMeshers_FaceSide;

#include <vector>
#include <list>
#include "Rn.h"

class STDMESHERS_EXPORT StdMeshers_MEFISTO_2D: public SMESH_2D_Algo
{
public:
  StdMeshers_MEFISTO_2D(int hypId, int studyId, SMESH_Gen* gen);
  virtual ~StdMeshers_MEFISTO_2D();

  virtual bool CheckHypothesis(SMESH_Mesh& aMesh,
                               const TopoDS_Shape& aShape,
                               SMESH_Hypothesis::Hypothesis_Status& aStatus);

  virtual bool Compute(SMESH_Mesh& aMesh,
		       const TopoDS_Shape& aShape);

  typedef boost::shared_ptr< StdMeshers_FaceSide> StdMeshers_FaceSidePtr;
  typedef std::vector< StdMeshers_FaceSidePtr > TWireVector;

  bool LoadPoints(TWireVector &                       wires,
		  R2*                                 uvslf, 
		  std::vector< const SMDS_MeshNode*>& mefistoToDS,
                  double scalex, double               scaley);

  void ComputeScaleOnFace(SMESH_Mesh& aMesh,
			  const TopoDS_Face& aFace,
			  double& scalex,
			  double& scaley);

  void StoreResult (Z nbst, R2* uvst, Z nbt, Z* nust, 
		    std::vector< const SMDS_MeshNode*>& mefistoToDS,
                    double scalex, double scaley);
					  
protected:
  double                            _edgeLength;
  double                            _maxElementArea;
  const StdMeshers_MaxElementArea*  _hypMaxElementArea;
  const StdMeshers_LengthFromEdges* _hypLengthFromEdges;

  std::list<const SMDS_MeshNode*> myNodesOnCommonV;

  SMESH_MesherHelper* myTool; // tool for working with quadratic elements
};

#endif
