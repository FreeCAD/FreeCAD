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

//  SMESH SMESH : idl implementation based on 'SMESH' unit's classes
//  File   : StdMeshers_ProjectionSource2D.hxx
//  Author : Edward AGAPOV
//  Module : SMESH
//
#ifndef _SMESH_ProjectionSource2D_HXX_
#define _SMESH_ProjectionSource2D_HXX_

#include "SMESH_StdMeshers.hxx"

#include "SMESH_Hypothesis.hxx"
#include "Utils_SALOME_Exception.hxx"

#include <TopoDS_Vertex.hxx>

class SMESH_Gen;

// =========================================================
/*!
 * This hypothesis specifies a meshed face to take a mesh pattern from
 * and optionally association of vertices between the source face and a
 * target one (where a hipothesis is assigned to)
 */
// =========================================================

class STDMESHERS_EXPORT StdMeshers_ProjectionSource2D: public SMESH_Hypothesis
{
public:
  // Constructor
  StdMeshers_ProjectionSource2D( int hypId, int studyId, SMESH_Gen * gen );
  // Destructor
  virtual ~StdMeshers_ProjectionSource2D();

  /*!
   * Sets a source <face> to take a mesh pattern from
   */
  void SetSourceFace(const TopoDS_Shape& face);

  /*!
   * Returns the source face or a group containing faces
   */
  TopoDS_Shape GetSourceFace() const;

  /*!
   * Returns true the source face is a group of faces
   */
  bool IsCompoundSource() const
  { return !_sourceFace.IsNull() && _sourceFace.ShapeType() == TopAbs_COMPOUND; }

  /*!
   * Sets source <mesh> to take a mesh pattern from
   */
  void SetSourceMesh(SMESH_Mesh* mesh);

  /*!
   * Return source mesh
   */
  SMESH_Mesh* GetSourceMesh() const { return _sourceMesh; }

  /*!
   * Sets vertex association between the source face and the target one.
   * This parameter is optional.
   * Two vertices must belong to one edge of a face
   */
  void SetVertexAssociation(const TopoDS_Shape& sourceVertex1,
                            const TopoDS_Shape& sourceVertex2,
                            const TopoDS_Shape& targetVertex1,
                            const TopoDS_Shape& targetVertex2);

  /*!
   * Returns the <i>-th source vertex associated with the <i>-th target vertex.
   * Result may be nil if association not set.
   * Valid indices are 1 and 2
   */
  TopoDS_Vertex GetSourceVertex(int i) const;

  /*!
   * Returns the <i>-th target vertex associated with the <i>-th source vertex.
   * Result may be nil if association not set.
   * Valid indices are 1 and 2
   */
  TopoDS_Vertex GetTargetVertex(int i) const;

  /*!
   * \brief Test if vertex association defined
    * \retval bool - test result
   */
  bool HasVertexAssociation() const
  { return ( !_sourceVertex1.IsNull() && !_targetVertex1.IsNull() &&
             !_sourceVertex2.IsNull() && !_targetVertex2.IsNull()); }

  /*!
   * \brief Return all parameters
   */
  void GetStoreParams(TopoDS_Shape& s1,
                      TopoDS_Shape& s2,
                      TopoDS_Shape& s3,
                      TopoDS_Shape& s4,
                      TopoDS_Shape& s5) const;

  /*!
   * \brief Set all parameters without notifying on modification
   */
  void RestoreParams(const TopoDS_Shape& s1,
                     const TopoDS_Shape& s2,
                     const TopoDS_Shape& s3,
                     const TopoDS_Shape& s4,
                     const TopoDS_Shape& s5,
                     SMESH_Mesh*         mesh);

  virtual std::ostream & SaveTo(std::ostream & save);
  virtual std::istream & LoadFrom(std::istream & load);
  friend std::ostream & operator <<(std::ostream & save, StdMeshers_ProjectionSource2D & hyp);
  friend std::istream & operator >>(std::istream & load, StdMeshers_ProjectionSource2D & hyp);

  /*!
   * \brief Initialize parameters by the mesh built on the geometry
    * \param theMesh - the built mesh
    * \param theShape - the geometry of interest
    * \retval bool - true if parameter values have been successfully defined
    *
    * Implementation does noting
   */
  virtual bool SetParametersByMesh(const SMESH_Mesh* theMesh, const TopoDS_Shape& theShape);

  /*!
   * \brief Initialize my parameter values by default parameters.
   *  \retval bool - true if parameter values have been successfully defined
   */
  virtual bool SetParametersByDefaults(const TDefaults& dflts, const SMESH_Mesh* theMesh=0);

protected:

  TopoDS_Shape  _sourceFace;
  SMESH_Mesh*   _sourceMesh;
  TopoDS_Vertex _sourceVertex1;
  TopoDS_Vertex _sourceVertex2;
  TopoDS_Vertex _targetVertex1;
  TopoDS_Vertex _targetVertex2;
};

#endif

