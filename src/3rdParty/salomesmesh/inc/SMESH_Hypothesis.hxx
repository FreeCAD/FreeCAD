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
//  File   : SMESH_Hypothesis.hxx
//  Author : Paul RASCLE, EDF
//  Module : SMESH
//
#ifndef _SMESH_HYPOTHESIS_HXX_
#define _SMESH_HYPOTHESIS_HXX_

#include "SMESH_SMESH.hxx"

#include "SMESHDS_Hypothesis.hxx"

class SMESH_Gen;
class TopoDS_Shape;
class SMESH_Mesh;

enum MeshDimension // dimension of mesh
{
  MeshDim_0D = 0,
  MeshDim_1D,
  MeshDim_2D,
  MeshDim_3D
};

class SMESH_EXPORT SMESH_Hypothesis: public SMESHDS_Hypothesis
{
public:
  enum Hypothesis_Status // in the order of severity
  {
    HYP_OK = 0,
    HYP_MISSING,      // algo misses a hypothesis
    HYP_CONCURENT,    // several applicable hypotheses assigned to father shapes
    HYP_BAD_PARAMETER,// hypothesis has a bad parameter value
    HYP_HIDDEN_ALGO,  // an algo is hidden by an upper dim algo generating all-dim elements
    HYP_HIDING_ALGO,  // an algo hides lower dim algos by generating all-dim elements
    HYP_UNKNOWN_FATAL,//  --- all statuses below should be considered as fatal
                      //      for Add/RemoveHypothesis operations
    HYP_INCOMPATIBLE, // hypothesis does not fit algo
    HYP_NOTCONFORM,   // not conform mesh is produced applying a hypothesis
    HYP_ALREADY_EXIST,// several applicable hypothesis of same priority assigned
    HYP_BAD_DIM,      // bad dimension
    HYP_BAD_SUBSHAPE, // shape is neither the main one, nor its sub-shape, nor a group
    HYP_BAD_GEOMETRY, // shape geometry mismatches algorithm's expectation
    HYP_NEED_SHAPE,   // algorithm can work on shape only
    HYP_INCOMPAT_HYPS // several additional hypotheses are incompatible one with other
  };
  static bool IsStatusFatal(Hypothesis_Status theStatus)
  { return theStatus >= HYP_UNKNOWN_FATAL; }

  SMESH_Hypothesis(int hypId, int studyId, SMESH_Gen* gen);
  virtual ~SMESH_Hypothesis();
  virtual int GetDim() const;
  int         GetStudyId() const;
  SMESH_Gen*  GetGen() const { return (SMESH_Gen*) _gen; }
  virtual int GetShapeType() const;
  virtual const char* GetLibName() const;
  virtual void NotifySubMeshesHypothesisModification();
  void  SetLibName(const char* theLibName);

  /*!
   * \brief The returned value is used by NotifySubMeshesHypothesisModification()
   *        to decide to call subMesh->AlgoStateEngine( MODIF_HYP, hyp ) or not
   *        if subMesh is ready to be computed (algo+hyp==OK)  but not yet computed.
   *        True result is reasonable for example if EventListeners depend on
   *        parameters of hypothesis.
   */
  virtual bool DataDependOnParams() const { return false; }
  void  SetParameters(const char *theParameters);
  char* GetParameters() const;

  void SetLastParameters(const char* theParameters);
  char* GetLastParameters() const;
  void ClearParameters();
  /*!
   * \brief Initialize my parameter values by the mesh built on the geometry
   *  \param theMesh - the built mesh
   *  \param theShape - the geometry of interest
   *  \retval bool - true if parameter values have been successfully defined
   */
  virtual bool SetParametersByMesh(const SMESH_Mesh* theMesh, const TopoDS_Shape& theShape)=0;

  struct TDefaults
  {
    double        _elemLength;
    int           _nbSegments;
    TopoDS_Shape* _shape; // future shape of the mesh being created
  };
  /*!
   * \brief Initialize my parameter values by default parameters.
   *  \retval bool - true if parameter values have been successfully defined
   */
  virtual bool SetParametersByDefaults(const TDefaults& dflts, const SMESH_Mesh* theMesh=0)=0;

  /*!
   * \brief Return true if me is an auxiliary hypothesis
    * \retval bool - auxiliary or not
   * 
   * An auxiliary hypothesis is optional, i.e. an algorithm
   * can work without it and another hypothesis of the same
   * dimension can be assigned to the shape
   */
  virtual bool IsAuxiliary() const
  { return GetType() == PARAM_ALGO && _param_algo_dim < 0; }

  /*!
   * \brief Find a mesh with given persistent ID
   */
  SMESH_Mesh* GetMeshByPersistentID(int id);

protected:
  SMESH_Gen* _gen;
  int        _studyId;
  int        _shapeType;
  int        _param_algo_dim; // to be set at descendant hypothesis constructor

private:
  std::string _libName; // name of library of plug-in Engine
  std::string _parameters;
  std::string _lastParameters;
};

#endif
