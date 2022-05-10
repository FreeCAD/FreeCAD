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
//  File   : StdMeshers_AutomaticLength.cxx
//  Author : Edward AGAPOV, OCC
//  Module : SMESH

#include "StdMeshers_AutomaticLength.hxx"

#include "SMESH_Mesh.hxx"
#include "SMESHDS_Mesh.hxx"
#include "SMESH_Algo.hxx"
#include "SMESHDS_SubMesh.hxx"

#include "utilities.h"

#include <TopTools_IndexedMapOfShape.hxx>
#include <TopExp.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Edge.hxx>

using namespace std;

//=============================================================================
/*!
 *  
 */
//=============================================================================

StdMeshers_AutomaticLength::StdMeshers_AutomaticLength(int hypId, int studyId, SMESH_Gen * gen)
  :SMESH_Hypothesis(hypId, studyId, gen)
{
  _name = "AutomaticLength";
  _param_algo_dim = 1; // is used by SMESH_Regular_1D

  _mesh = 0;
  _fineness = 0;
}

//=============================================================================
/*!
 *  
 */
//=============================================================================

StdMeshers_AutomaticLength::~StdMeshers_AutomaticLength()
{
}

//================================================================================
/*!
 * \brief Set Fineness
 * \param theFineness - The Fineness value [0.0-1.0],
 *                        0 - coarse mesh
 *                        1 - fine mesh
 * 
 * Raise if theFineness is out of range
 * The "Initial Number of Elements on the Shortest Edge" (S0)
 * is divided by (0.5 + 4.5 x theFineness)
 */
//================================================================================

const double theCoarseConst = 0.5;
const double theFineConst   = 4.5;

void StdMeshers_AutomaticLength::SetFineness(double theFineness)
{
  if ( theFineness < 0.0 || theFineness > 1.0 )
    throw SALOME_Exception(LOCALIZED("theFineness is out of range [0.0-1.0]"));

  if ( _fineness != theFineness )
  {
    NotifySubMeshesHypothesisModification();
    _fineness = theFineness;
  }
}

namespace {

  //================================================================================
  /*!
   * \brief Return pointer to TopoDS_TShape
   * \param theShape - The TopoDS_Shape
   * \retval inline const TopoDS_TShape* - result
   */
  //================================================================================

  inline const TopoDS_TShape* getTShape(const TopoDS_Shape& theShape)
  {
    return theShape.TShape().operator->();
  }

  //================================================================================
  /*!
   * \brief computes segment length by S0 and edge length
   */
  //================================================================================

  const double a14divPI = 14. / M_PI;


  inline double segLength(double S0, double edgeLen, double minLen )
  {
    // PAL10237
    // S = S0 * f(L/Lmin) where f(x) = 1 + (2/Pi * 7 * atan(x/5) )

    // =>
    // S = S0 * ( 1 + 14/PI * atan( L / ( 5 * Lmin )))
    return S0 * ( 1. + a14divPI * atan( edgeLen / ( 5 * minLen )));
  }
#if 0
  //const double a14divPI = 14. / M_PI;
  const double a2div7divPI = 2. / 7. / M_PI;

  inline double segLength(double S0, double edgeLen, double minLen )
  {
    // PAL10237
    // S = S0 * f(L/Lmin) where
    // f(x) = 1 + (7 * 2/Pi * atan(x/5))
    // =>
    // S = S0 * ( 1 + 14/PI * atan( L / ( 5 * Lmin )))
    //
    // return S0 * ( 1. + a14divPI * atan( edgeLen / ( 5 * minLen )));

    // The above formular gives too short segments when Lmax/Lmin is too high
    // because by this formular the largest segment is only 8 times longer than the
    // shortest one ( 2/Pi * atan(x/5) varies within [0,1] ). So a new formular is:
    //
    // f(x) = 1 + (x/7 * 2/Pi * atan(x/5))
    // =>
    // S = S0 * ( 1 + 2/7/PI * L/Lmin * atan( 5 * L/Lmin ))
    //
    const double Lratio = edgeLen / minLen;
    return S0 * ( 1. + a2div7divPI * Lratio * atan( 5 * Lratio ));
  }
#endif
  //================================================================================
  /*!
   * \brief Compute segment length for all edges
   * \param theMesh - The mesh
   * \param theTShapeToLengthMap - The map of edge to segment length
   */
  //================================================================================

  void computeLengths( SMESHDS_Mesh*                       aMesh,
                       map<const TopoDS_TShape*, double> & theTShapeToLengthMap,
                       double &                            theS0,
                       double &                            theMinLen)
  {
    theTShapeToLengthMap.clear();

    TopoDS_Shape aMainShape = aMesh->ShapeToMesh();

    // Find length of longest and shortest edge
    double Lmin = DBL_MAX, Lmax = -DBL_MAX;
    TopTools_IndexedMapOfShape edgeMap;
    TopExp::MapShapes( aMainShape, TopAbs_EDGE, edgeMap);
    for ( int i = 1; i <= edgeMap.Extent(); ++i )
    {
      TopoDS_Edge edge = TopoDS::Edge( edgeMap(i) );
      //if ( BRep_Tool::Degenerated( edge )) continue;

      Standard_Real L = SMESH_Algo::EdgeLength( edge );
      if ( L < DBL_MIN ) continue;

      if ( L > Lmax ) Lmax = L;
      if ( L < Lmin ) Lmin = L;

      // remember i-th edge length
      theTShapeToLengthMap.insert( make_pair( getTShape( edge ), L ));
    }

    // Compute S0 - minimal segement length, is computed by the shortest EDGE

    /* image attached to PAL10237

       NbSeg (on the shortest EDGE)
         ^
         |
       10|\
         | \
         |  \
         |   \
        5|    --------
         |
         +------------>
         1    10       Lmax/Lmin
    */
    const int NbSegMin = 5, NbSegMax = 10; //  on axis NbSeg
    const double Lrat1 = 1., Lrat2 = 10.;  //  on axis Lmax/Lmin

    double Lratio = Lmax/Lmin;
    double NbSeg = NbSegMin;
    if ( Lratio < Lrat2 )
      NbSeg += ( Lrat2 - Lratio ) / ( Lrat2 - Lrat1 )  * ( NbSegMax - NbSegMin );

    double S0 = Lmin / (int) NbSeg;
    MESSAGE( "S0 = " << S0 << ", Lmin = " << Lmin << ", Nbseg = " << (int) NbSeg);

    // Compute segments length for all edges

    map<const TopoDS_TShape*, double>::iterator tshape_length = theTShapeToLengthMap.begin();
    for ( ; tshape_length != theTShapeToLengthMap.end(); ++tshape_length )
    {
      double & L = tshape_length->second;
      L = segLength( S0, L, Lmin );
    }
    theS0 = S0;
    theMinLen = Lmin;
  }
}

//=============================================================================
/*!
 * \brief Computes segment length for an edge of given length
 */
//=============================================================================

double StdMeshers_AutomaticLength::GetLength(const SMESH_Mesh* theMesh,
                                             const double      theEdgeLength)
{
  if ( !theMesh ) throw SALOME_Exception(LOCALIZED("NULL Mesh"));

  SMESHDS_Mesh* aMeshDS = const_cast< SMESH_Mesh* > ( theMesh )->GetMeshDS();
  if ( theMesh != _mesh )
  {
    computeLengths( aMeshDS, _TShapeToLength, _S0, _minLen );
    _mesh = theMesh;
  }
  double L = segLength( _S0, theEdgeLength, _minLen );
  return L / (theCoarseConst + theFineConst * _fineness);
}

//=============================================================================
/*!
 *  
 */
//=============================================================================

double StdMeshers_AutomaticLength::GetLength(const SMESH_Mesh*   theMesh,
                                             const TopoDS_Shape& anEdge)
{
  if ( !theMesh ) throw SALOME_Exception(LOCALIZED("NULL Mesh"));

  if ( anEdge.IsNull() || anEdge.ShapeType() != TopAbs_EDGE )
    throw SALOME_Exception(LOCALIZED("Bad edge shape"));

  if ( theMesh != _mesh )
  {
    SMESHDS_Mesh* aMeshDS = const_cast< SMESH_Mesh* > ( theMesh )->GetMeshDS();
    computeLengths( aMeshDS, _TShapeToLength, _S0, _minLen );
    _mesh = theMesh;
  }

  map<const TopoDS_TShape*, double>::iterator tshape_length =
    _TShapeToLength.find( getTShape( anEdge ));

  if ( tshape_length == _TShapeToLength.end() )
    return 1; // it is a dgenerated edge

  return tshape_length->second / (theCoarseConst + theFineConst * _fineness);
}

//=============================================================================
/*!
 *  
 */
//=============================================================================

ostream & StdMeshers_AutomaticLength::SaveTo(ostream & save)
{
  save << _fineness;
  return save;
}

//=============================================================================
/*!
 *  
 */
//=============================================================================

istream & StdMeshers_AutomaticLength::LoadFrom(istream & load)
{
  if ( ! ( load >> _fineness ))
    load.clear(ios::badbit | load.rdstate());
  return load;
}

//=============================================================================
/*!
 *  
 */
//=============================================================================

ostream & operator <<(ostream & save, StdMeshers_AutomaticLength & hyp)
{
  return hyp.SaveTo( save );
}

//=============================================================================
/*!
 *  
 */
//=============================================================================

istream & operator >>(istream & load, StdMeshers_AutomaticLength & hyp)
{
  return hyp.LoadFrom( load );
}

//================================================================================
/*!
 * \brief Initialize Fineness by the mesh built on the geometry
 * \param theMesh - the built mesh
 * \param theShape - the geometry of interest
 * \retval bool - true if parameter values have been successfully defined
 */
//================================================================================

bool StdMeshers_AutomaticLength::SetParametersByMesh(const SMESH_Mesh*   theMesh,
                                                     const TopoDS_Shape& theShape)
{
  if ( !theMesh || theShape.IsNull() )
    return false;

  _fineness = 0;

  SMESHDS_Mesh* aMeshDS = const_cast< SMESH_Mesh* >( theMesh )->GetMeshDS();

  int nbEdges = 0;
  TopTools_IndexedMapOfShape edgeMap;
  TopExp::MapShapes( theShape, TopAbs_EDGE, edgeMap );
  for ( int i = 1; i <= edgeMap.Extent(); ++i )
  {
    const TopoDS_Edge& edge = TopoDS::Edge( edgeMap( i ));

    // assure the base automatic length is stored in _TShapeToLength
    if ( i == 1 ) 
      GetLength( theMesh, edge );

    // get current segment length
    double L = SMESH_Algo::EdgeLength( edge );
    if ( L <= DBL_MIN )
      continue;
    SMESHDS_SubMesh * eSubMesh = aMeshDS->MeshElements( edge );
    if ( !eSubMesh )
      return false;
    int nbSeg = eSubMesh->NbElements();
    if ( nbSeg < 1 )
      continue;
    double segLen = L / nbSeg;

    // get segment length from _TShapeToLength
    map<const TopoDS_TShape*, double>::iterator tshape_length =
      _TShapeToLength.find( getTShape( edge ));
    if ( tshape_length == _TShapeToLength.end() )
      continue;
    double autoLen = tshape_length->second;

    // segLen = autoLen / (theCoarseConst + theFineConst * _fineness) -->
    _fineness += ( autoLen / segLen - theCoarseConst ) / theFineConst;

    ++nbEdges;
  }
  if ( nbEdges )
    _fineness /= nbEdges;

  if (_fineness > 1.0)
    _fineness = 1.0;
  else if (_fineness < 0.0)
    _fineness = 0.0;

  return nbEdges;
}

//================================================================================
/*!
 * \brief Initialize my parameter values by default parameters.
 *  \retval bool - true if parameter values have been successfully defined
 */
//================================================================================

bool StdMeshers_AutomaticLength::SetParametersByDefaults(const TDefaults&  /*theDflts*/,
                                                         const SMESH_Mesh* /*theMesh*/)
{
  return false;

  // assure the base automatic length is stored in _TShapeToLength
//   GetLength( theMesh, elemLenght );

//   // find maximal edge length
//   double maxLen = 0;
//   map<const TopoDS_TShape*, double>::iterator
//     tshape_length = _TShapeToLength.begin(), slEnd = _TShapeToLength.end();
//   for ( ; tshape_length != slEnd; ++tshape_length )
//     if ( tshape_length->second > maxLen )
//       maxLen = tshape_length->second;

//   // automatic length for longest element
//   double autoLen = GetLength( theMesh, maxLen );

//   // elemLenght = autoLen / (theCoarseConst + theFineConst * _fineness) -->
//   _fineness = ( autoLen / elemLenght - theCoarseConst ) / theFineConst;

//   return true;
}
