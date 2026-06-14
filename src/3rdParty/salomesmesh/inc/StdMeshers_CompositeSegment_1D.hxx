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

//  SMESH SMESH : implementation of SMESH idl descriptions
//  File   : StdMeshers_CompositeSegment_1D.hxx
//  Module : SMESH
//
#ifndef _SMESH_CompositeSegment_1D_HXX_
#define _SMESH_CompositeSegment_1D_HXX_

#include "StdMeshers_Regular_1D.hxx"
#include "SMESH_StdMeshers.hxx"

class SMESH_subMeshEventListener;
class SMESH_Mesh;
class StdMeshers_FaceSide;
class TopoDS_Edge;
class TopoDS_Face;

class STDMESHERS_EXPORT StdMeshers_CompositeSegment_1D: public StdMeshers_Regular_1D
{
public:
  StdMeshers_CompositeSegment_1D(int hypId, int studyId, SMESH_Gen* gen);

  virtual bool Compute(SMESH_Mesh&         aMesh,
                       const TopoDS_Shape& aShape);
  /*!
   * \brief Sets event listener to submeshes if necessary
    * \param subMesh - submesh where algo is set
   *
   * This method is called when a submesh gets HYP_OK algo_state.
   * After being set, event listener is notified on each event of a submesh.
   */
  virtual void SetEventListener(SMESH_subMesh* subMesh);

   /*!
   * \brief Return a face side the edge belongs to
   */
  static StdMeshers_FaceSide * GetFaceSide(SMESH_Mesh&        aMesh,
                                           const TopoDS_Edge& anEdge,
                                           const TopoDS_Face& aFace,
                                           const bool         ignoreMeshed);

  /*!
   * \brief Returns algo type name
   */
  static std::string AlgoName();

};

#endif
