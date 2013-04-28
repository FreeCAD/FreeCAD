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
//  File   : StdMeshers_Regular_1D.hxx
//           Moved here from SMESH_Regular_1D.hxx
//  Author : Paul RASCLE, EDF
//  Module : SMESH

#ifndef _SMESH_REGULAR_1D_HXX_
#define _SMESH_REGULAR_1D_HXX_

#include "SMESH_StdMeshers.hxx"

#include "SMESH_1D_Algo.hxx"

class Adaptor3d_Curve;
class TopoDS_Vertex;
class StdMeshers_SegmentLengthAroundVertex;

class STDMESHERS_EXPORT StdMeshers_Regular_1D: public SMESH_1D_Algo
{
public:
  StdMeshers_Regular_1D(int hypId, int studyId, SMESH_Gen* gen);
  virtual ~StdMeshers_Regular_1D();

  virtual bool CheckHypothesis(SMESH_Mesh& aMesh,
                               const TopoDS_Shape& aShape,
                               SMESH_Hypothesis::Hypothesis_Status& aStatus);

  virtual bool Compute(SMESH_Mesh& aMesh,
		       const TopoDS_Shape& aShape);

  virtual const std::list <const SMESHDS_Hypothesis *> &
    GetUsedHypothesis(SMESH_Mesh & aMesh, const TopoDS_Shape & aShape, const bool=true);

  /*!
   * \brief Sets event listener to submeshes if necessary
    * \param subMesh - submesh where algo is set
   *
   * This method is called when a submesh gets HYP_OK algo_state.
   * After being set, event listener is notified on each event of a submesh.
   */
  virtual void SetEventListener(SMESH_subMesh* subMesh);

  /*!
   * \brief Allow algo to do something after persistent restoration
   * \param subMesh - restored submesh
   *
   * This method is called only if a submesh has HYP_OK algo_state.
   */
  void SubmeshRestored(SMESH_subMesh* subMesh);

protected:

  virtual bool computeInternalParameters (SMESH_Mesh &        theMesh,
                                          Adaptor3d_Curve &   theC3d,
                                          double              theLength,
                                          double              theFirstU,
                                          double              theLastU,
                                          std::list<double> & theParameters,
                                          const bool          theReverse,
                                          bool                theConsiderPropagation = false);

  virtual void redistributeNearVertices (SMESH_Mesh &          theMesh,
                                         Adaptor3d_Curve &     theC3d,
                                         double                theLength,
                                         std::list< double > & theParameters,
                                         const TopoDS_Vertex & theVf,
                                         const TopoDS_Vertex & theVl);

  /*!
   * \brief Return StdMeshers_SegmentLengthAroundVertex assigned to vertex
   */
  static const
  StdMeshers_SegmentLengthAroundVertex* getVertexHyp(SMESH_Mesh &          theMesh,
                                                     const TopoDS_Vertex & theV);

  enum HypothesisType { LOCAL_LENGTH, MAX_LENGTH, NB_SEGMENTS, BEG_END_LENGTH, DEFLECTION, ARITHMETIC_1D, NONE };

  enum ValueIndex {
    SCALE_FACTOR_IND = 0,
    BEG_LENGTH_IND   = 0,
    END_LENGTH_IND   = 1,
    DEFLECTION_IND   = 0,
    PRECISION_IND    = 1
  };

  enum IValueIndex {
    NB_SEGMENTS_IND  = 0,
    DISTR_TYPE_IND   = 1,
    CONV_MODE_IND    = 2
  };

  enum VValueIndex {
    TAB_FUNC_IND  = 0
  };

  enum SValueIndex {
    EXPR_FUNC_IND  = 0
  };

  HypothesisType _hypType;

  double _value[2];
  int    _ivalue[3];
  std::vector<double> _vvalue[1];
  std::string         _svalue[1];

  // a source of propagated hypothesis, is set by CheckHypothesis()
  // always called before Compute()
  TopoDS_Shape _mainEdge;
};

#endif
