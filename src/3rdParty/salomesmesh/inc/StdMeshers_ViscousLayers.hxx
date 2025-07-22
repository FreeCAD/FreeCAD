// Copyright (C) 2007-2015  CEA/DEN, EDF R&D, OPEN CASCADE
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

// File      : StdMeshers_ViscousLayers.hxx
// Created   : Wed Dec  1 15:15:34 2010
// Author    : Edward AGAPOV (eap)

#ifndef __StdMeshers_ViscousLayers_HXX__
#define __StdMeshers_ViscousLayers_HXX__

#include "SMESH_StdMeshers.hxx"

#include "SMESH_Hypothesis.hxx"
#include "SMESH_ProxyMesh.hxx"
#include "SMESH_ComputeError.hxx"

#include <vector>

/*!
 * \brief Hypothesis defining parameters of viscous layers
 */
class STDMESHERS_EXPORT StdMeshers_ViscousLayers : public SMESH_Hypothesis
{
public:
  StdMeshers_ViscousLayers(int hypId, int studyId, SMESH_Gen* gen);

  // Set boundary shapes (faces in 3D, edges in 2D) either to exclude from
  // treatment or to make the Viscous Layers on
  void   SetBndShapes(const std::vector<int>& shapeIds, bool toIgnore);
  std::vector<int> GetBndShapes() const { return _shapeIds; }
  bool   IsToIgnoreShapes() const { return _isToIgnoreShapes; }

  // Set total thickness of layers of prisms
  void   SetTotalThickness(double thickness);
  double GetTotalThickness() const { return _thickness; }

  // Set number of layers of prisms
  void   SetNumberLayers(int nb);
  int    GetNumberLayers() const { return _nbLayers; }

  // Set factor (>1.0) of growth of layer thickness towards inside of mesh
  void   SetStretchFactor(double factor);
  double GetStretchFactor() const { return _stretchFactor; }

  // Method of computing node translation 
  enum ExtrusionMethod {
    // node is translated along normal to a surface with possible further smoothing
    SURF_OFFSET_SMOOTH,
    // node is translated along the average normal of surrounding faces till
    // intersection with a neighbor face translated along its own normal 
    // by the layers thickness
    FACE_OFFSET,
    // node is translated along the average normal of surrounding faces
    // by the layers thickness
    NODE_OFFSET
  };
  void   SetMethod( ExtrusionMethod how );
  ExtrusionMethod GetMethod() const { return _method; }

  // Computes temporary 2D mesh to be used by 3D algorithm.
  // Return SMESH_ProxyMesh for each SOLID in theShape
  SMESH_ProxyMesh::Ptr Compute(SMESH_Mesh&         theMesh,
                               const TopoDS_Shape& theShape,
                               const bool          toMakeN2NMap=false) const;

  // Checks compatibility of assigned StdMeshers_ViscousLayers hypotheses
  static SMESH_ComputeErrorPtr
    CheckHypothesis(SMESH_Mesh&                          aMesh,
                    const TopoDS_Shape&                  aShape,
                    SMESH_Hypothesis::Hypothesis_Status& aStatus);

  // Checks if viscous layers should be constructed on a shape
  bool IsShapeWithLayers(int shapeIndex) const;

  virtual std::ostream & SaveTo(std::ostream & save);
  virtual std::istream & LoadFrom(std::istream & load);

  /*!
   * \brief Initialize my parameter values by the mesh built on the geometry
    * \param theMesh - the built mesh
    * \param theShape - the geometry of interest
    * \retval bool - true if parameter values have been successfully defined
   */
  virtual bool SetParametersByMesh(const SMESH_Mesh* theMesh, const TopoDS_Shape& theShape);

  /*!
   * \brief Initialize my parameter values by default parameters.
   *  \retval bool - true if parameter values have been successfully defined
   */
  virtual bool SetParametersByDefaults(const TDefaults& dflts, const SMESH_Mesh* theMesh=0)
  { return false; }

  static const char* GetHypType() { return "ViscousLayers"; }

 private:

  std::vector<int> _shapeIds;
  bool             _isToIgnoreShapes;
  int              _nbLayers;
  double           _thickness;
  double           _stretchFactor;
  ExtrusionMethod  _method;
};

class SMESH_subMesh;
namespace VISCOUS_3D
{
  // sets a sub-mesh event listener to clear sub-meshes of sub-shapes of
  // the main shape when sub-mesh of the main shape is cleared,
  // for example to clear sub-meshes of FACEs when sub-mesh of a SOLID
  // is cleared
  void ToClearSubWithMain( SMESH_subMesh* sub, const TopoDS_Shape& main);
}

#endif
