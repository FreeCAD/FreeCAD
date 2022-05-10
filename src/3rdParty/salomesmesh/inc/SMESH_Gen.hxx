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
//  File   : SMESH_Gen.hxx
//  Author : Paul RASCLE, EDF
//  Module : SMESH
//
#ifndef _SMESH_GEN_HXX_
#define _SMESH_GEN_HXX_

#include "SMESH_SMESH.hxx"

#include "Utils_SALOME_Exception.hxx"

#include "SMESH_Hypothesis.hxx"
#include "SMESH_ComputeError.hxx"
#include "SMESH_Algo.hxx"
#include "SMESH_Mesh.hxx"

#include <TopoDS_Shape.hxx>

#include <map>
#include <list>
#include <vector>
#include <string>

class SMESHDS_Document;

typedef SMESH_Hypothesis::Hypothesis_Status TAlgoStateErrorName;

typedef struct studyContextStruct
{
  std::map < int, SMESH_Hypothesis * >mapHypothesis;
  std::map < int, SMESH_Mesh * >mapMesh;
  SMESHDS_Document * myDocument;
} StudyContextStruct;

typedef std::set<int> TSetOfInt;

class SMESH_EXPORT  SMESH_Gen
{
public:
  SMESH_Gen();
  ~SMESH_Gen();

  SMESH_Mesh* CreateMesh(int theStudyId, bool theIsEmbeddedMode);

  /*!
   * \brief Computes aMesh on aShape 
   *  \param aShapeOnly - if true, algo->OnlyUnaryInput() feature is ignored and
   *                      only \a aShape is computed.
   *  \param anUpward - compute from vertices up to more complex shape (internal usage)
   *  \param aDim - upper level dimension of the mesh computation
   *  \param aShapesId - list of shapes with computed mesh entities (elements or nodes)
   *  \retval bool - true if none submesh failed to compute
   */
  bool Compute(::SMESH_Mesh &        aMesh,
               const TopoDS_Shape &  aShape,
               const bool            aShapeOnly=false,
               const bool            anUpward=false,
               const ::MeshDimension aDim=::MeshDim_3D,
               TSetOfInt*            aShapesId=0);

  void PrepareCompute(::SMESH_Mesh &        aMesh,
                      const TopoDS_Shape &  aShape);
  void CancelCompute(::SMESH_Mesh &        aMesh,
                     const TopoDS_Shape &  aShape);

  const SMESH_subMesh* GetCurrentSubMesh() const;

  /*!
   * \brief evaluates size of prospective mesh on a shape 
   * \param aMesh - the mesh
   * \param aShape - the shape
   * \param aResMap - map for prospective numbers of elements
   * \retval bool - is a success
   */
  bool Evaluate(::SMESH_Mesh &        aMesh,
                const TopoDS_Shape &  aShape,
                MapShapeNbElems&      aResMap,
                const bool            anUpward=false,
                TSetOfInt*            aShapesId=0);

  bool CheckAlgoState(SMESH_Mesh& aMesh, const TopoDS_Shape& aShape);
  // notify on bad state of attached algos, return false
  // if Compute() would fail because of some algo bad state

  /*!
   * \brief Sets number of segments per diagonal of boundary box of geometry by which
   *        default segment length of appropriate 1D hypotheses is defined
   */
  void SetBoundaryBoxSegmentation( int theNbSegments ) { _segmentation = theNbSegments; }
  int  GetBoundaryBoxSegmentation() const { return _segmentation; }
  /*!
   * \brief Sets default number of segments per edge
   */
  void SetDefaultNbSegments(int nb) { _nbSegments = nb; }
  int GetDefaultNbSegments() const { return _nbSegments; }

  struct TAlgoStateError
  {
    TAlgoStateErrorName _name;
    const SMESH_Algo*   _algo;
    int                 _algoDim;
    bool                _isGlobalAlgo;

    TAlgoStateError(): _name(SMESH_Hypothesis::HYP_OK), _algo(0), _algoDim(0) {}
    void Set(TAlgoStateErrorName name, const SMESH_Algo* algo, bool isGlobal)
    { _name = name; _algo = algo; _algoDim = algo->GetDim(); _isGlobalAlgo = isGlobal; }
    void Set(TAlgoStateErrorName name, const int algoDim,      bool isGlobal)
    { _name = name; _algo = 0;    _algoDim = algoDim;        _isGlobalAlgo = isGlobal; }
  };

  bool GetAlgoState(SMESH_Mesh& aMesh, const TopoDS_Shape& aShape,
                    std::list< SMESH_Gen::TAlgoStateError > & theErrors);
  // notify on bad state of attached algos, return false
  // if Compute() would fail because of some algo bad state
  // theErrors list contains problems description

  StudyContextStruct *GetStudyContext(int studyId);

  static int GetShapeDim(const TopAbs_ShapeEnum & aShapeType);
  static int GetShapeDim(const TopoDS_Shape & aShape)
  { return GetShapeDim( aShape.ShapeType() ); }

  SMESH_Algo* GetAlgo(SMESH_Mesh & aMesh, const TopoDS_Shape & aShape, TopoDS_Shape* assignedTo=0);
  SMESH_Algo* GetAlgo(SMESH_subMesh * aSubMesh, TopoDS_Shape* assignedTo=0);

  static bool IsGlobalHypothesis(const SMESH_Hypothesis* theHyp, SMESH_Mesh& aMesh);

  static std::vector< std::string > GetPluginXMLPaths();

  int GetANewId();

  // std::map < int, SMESH_Algo * >_mapAlgo;
  // std::map < int, SMESH_0D_Algo * >_map0D_Algo;
  // std::map < int, SMESH_1D_Algo * >_map1D_Algo;
  // std::map < int, SMESH_2D_Algo * >_map2D_Algo;
  // std::map < int, SMESH_3D_Algo * >_map3D_Algo;

private:

  int _localId;                         // unique Id of created objects, within SMESH_Gen entity
  std::map < int, StudyContextStruct * >_mapStudyContext;

  // hypotheses managing
  int _hypId;

  // number of segments per diagonal of boundary box of geometry by which
  // default segment length of appropriate 1D hypotheses is defined
  int _segmentation;
  // default number of segments
  int _nbSegments;

  void setCurrentSubMesh(SMESH_subMesh* sm);
  void resetCurrentSubMesh();

  volatile bool               _compute_canceled;
  std::list< SMESH_subMesh* > _sm_current;
};

#endif
