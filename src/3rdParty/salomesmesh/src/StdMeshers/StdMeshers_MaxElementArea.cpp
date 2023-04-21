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
//  File   : StdMeshers_MaxElementArea.cxx
//           Moved here from SMESH_MaxElementArea.cxx
//  Author : Paul RASCLE, EDF
//  Module : SMESH
//
#include "StdMeshers_MaxElementArea.hxx"

#include "SMESH_ControlsDef.hxx"
#include "SMDS_MeshElement.hxx"
#include "SMESHDS_SubMesh.hxx"
#include "SMESH_Mesh.hxx"

#include <TopExp.hxx>
#include <TopTools_IndexedMapOfShape.hxx>

#include "utilities.h"

using namespace std;

//=============================================================================
/*!
 *  
 */
//=============================================================================

StdMeshers_MaxElementArea::StdMeshers_MaxElementArea(int hypId, int studyId, SMESH_Gen* gen)
  : SMESH_Hypothesis(hypId, studyId, gen)
{
  _maxArea =1.;
  _name = "MaxElementArea";
  _param_algo_dim = 2; 
}

//=============================================================================
/*!
 *  
 */
//=============================================================================

StdMeshers_MaxElementArea::~StdMeshers_MaxElementArea()
{
}

//=============================================================================
/*!
 *  
 */
//=============================================================================

void StdMeshers_MaxElementArea::SetMaxArea(double maxArea)
{
  double oldArea = _maxArea;
  if (maxArea <= 0) 
    throw SALOME_Exception(LOCALIZED("maxArea must be positive"));
  _maxArea = maxArea;
  if (_maxArea != oldArea)
    NotifySubMeshesHypothesisModification();
}

//=============================================================================
/*!
 *  
 */
//=============================================================================

double StdMeshers_MaxElementArea::GetMaxArea() const
{
  return _maxArea;
}

//=============================================================================
/*!
 *  
 */
//=============================================================================

ostream & StdMeshers_MaxElementArea::SaveTo(ostream & save)
{
  save << this->_maxArea;
  return save;
}

//=============================================================================
/*!
 *  
 */
//=============================================================================

istream & StdMeshers_MaxElementArea::LoadFrom(istream & load)
{
  bool isOK = true;
  double a;
  isOK = (bool)(load >> a);
  if (isOK) 
    this->_maxArea = a;
  else 
    load.clear(ios::badbit | load.rdstate());
  return load;
}

//=============================================================================
/*!
 *  
 */
//=============================================================================

ostream & operator << (ostream & save, StdMeshers_MaxElementArea & hyp)
{
  return hyp.SaveTo( save );
}

//=============================================================================
/*!
 *  
 */
//=============================================================================

istream & operator >> (istream & load, StdMeshers_MaxElementArea & hyp)
{
  return hyp.LoadFrom( load );
}

//================================================================================
/*!
 * \brief Initialize maximal area by the mesh built on the geometry
 * \param theMesh - the built mesh
 * \param theShape - the geometry of interest
 * \retval bool - true if parameter values have been successfully defined
 */
//================================================================================

bool StdMeshers_MaxElementArea::SetParametersByMesh(const SMESH_Mesh*   theMesh,
                                                    const TopoDS_Shape& theShape)
{
  if ( !theMesh || theShape.IsNull() )
    return false;

  _maxArea = 0;

  SMESH::Controls::Area areaControl;
  SMESH::Controls::TSequenceOfXYZ nodesCoords;

  SMESHDS_Mesh* aMeshDS = const_cast< SMESH_Mesh* >( theMesh )->GetMeshDS();

  TopTools_IndexedMapOfShape faceMap;
  TopExp::MapShapes( theShape, TopAbs_FACE, faceMap );
  for ( int iF = 1; iF <= faceMap.Extent(); ++iF )
  {
    SMESHDS_SubMesh * subMesh = aMeshDS->MeshElements( faceMap( iF ));
    if ( !subMesh )
      return false;
    SMDS_ElemIteratorPtr fIt = subMesh->GetElements();
    while ( fIt->more() )
    {
      const SMDS_MeshElement* elem = fIt->next();
      if ( elem->GetType() == SMDSAbs_Face ) {
        areaControl.GetPoints( elem, nodesCoords );
        _maxArea = max( _maxArea, areaControl.GetValue( nodesCoords ));
      }
    }
  }
  return _maxArea > 0;
}
//================================================================================
/*!
 * \brief Initialize my parameter values by default parameters.
 *  \retval bool - true if parameter values have been successfully defined
 */
//================================================================================

bool StdMeshers_MaxElementArea::SetParametersByDefaults(const TDefaults&  dflts,
                                                        const SMESH_Mesh* /*theMesh*/)
{
  return ( _maxArea = dflts._elemLength*dflts._elemLength );
}

