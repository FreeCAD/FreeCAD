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

//  SMESH StdMeshers_Deflection1D : implementation of SMESH idl descriptions
//  File   : StdMeshers_Deflection1D.cxx
//  Module : SMESH
//
#include "StdMeshers_Deflection1D.hxx"
#include "utilities.h"

#include "SMESH_Mesh.hxx"
#include "SMESH_Algo.hxx"

#include <BRep_Tool.hxx>
#include <GeomAdaptor_Curve.hxx>
#include <Geom_Curve.hxx>
#include <TopExp.hxx>
#include <TopLoc_Location.hxx>
#include <TopTools_IndexedMapOfShape.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Edge.hxx>
#include <gp_Lin.hxx>
#include <gp_Pnt.hxx>

using namespace std;

//=============================================================================
/*!
 *  
 */
//=============================================================================

StdMeshers_Deflection1D::StdMeshers_Deflection1D(int         hypId,
                                                 int         studyId,
                                                 SMESH_Gen * gen)
     :SMESH_Hypothesis(hypId, studyId, gen)
{
  _value = 1.;
  _name = "Deflection1D";
  _param_algo_dim = 1; // is used by SMESH_Regular_1D
}

//=============================================================================
/*!
 *  
 */
//=============================================================================

StdMeshers_Deflection1D::~StdMeshers_Deflection1D()
{
}

//=============================================================================
/*!
 *  
 */
//=============================================================================

void StdMeshers_Deflection1D::SetDeflection(double value)
{
  if (_value != value) {
    if (value <= 0.)
      throw SALOME_Exception(LOCALIZED("Value must be positive"));

    NotifySubMeshesHypothesisModification();

    _value = value;
  }
}

//=============================================================================
/*!
 *  
 */
//=============================================================================

double StdMeshers_Deflection1D::GetDeflection() const
{
  return _value;
}

//=============================================================================
/*!
 *  
 */
//=============================================================================

ostream & StdMeshers_Deflection1D::SaveTo(ostream & save)
{
  save << _value;
  return save;
}

//=============================================================================
/*!
 *  
 */
//=============================================================================

istream & StdMeshers_Deflection1D::LoadFrom(istream & load)
{
  bool isOK = (bool)(load >> _value);
  if (!isOK)
    load.clear(ios::badbit | load.rdstate());
  return load;
}

//=============================================================================
/*!
 *  
 */
//=============================================================================

ostream & operator <<(ostream & save, StdMeshers_Deflection1D & hyp)
{
  return hyp.SaveTo( save );
}

//=============================================================================
/*!
 *  
 */
//=============================================================================

istream & operator >>(istream & load, StdMeshers_Deflection1D & hyp)
{
  return hyp.LoadFrom( load );
}
//================================================================================
/*!
 * \brief Evaluate curve deflection between two points
  * \param theCurve - the curve
  * \param theU1 - the parameter of the first point
  * \param theU2 - the parameter of the second point
  * \retval double - deflection value
 */
//================================================================================

static double deflection(const GeomAdaptor_Curve & theCurve,
                         double                    theU1,
                         double                    theU2)
{
  if ( theCurve.GetType() == GeomAbs_Line )
    return 0;
  // line between theU1 and theU2
  gp_Pnt p1 = theCurve.Value( theU1 ), p2 = theCurve.Value( theU2 );
  gp_Lin segment( p1, gp_Vec( p1, p2 ));

  // evaluate square distance of theCurve from the segment
  Standard_Real dist2 = 0;
  const int nbPnt = 7;
  const double step = ( theU2 - theU1 ) / nbPnt;
  while (( theU1 += step ) < theU2 )
    dist2 = Max( dist2, segment.SquareDistance( theCurve.Value( theU1 )));

  return sqrt( dist2 );
}

//================================================================================
/*!
 * \brief Initialize deflection value by the mesh built on the geometry
 * \param theMesh - the built mesh
 * \param theShape - the geometry of interest
 * \retval bool - true if parameter values have been successfully defined
 */
//================================================================================

bool StdMeshers_Deflection1D::SetParametersByMesh(const SMESH_Mesh*   theMesh,
                                                  const TopoDS_Shape& theShape)
{
  if ( !theMesh || theShape.IsNull() )
    return false;

  _value = 0.;

  Standard_Real UMin, UMax;
  TopLoc_Location L;

  int nbEdges = 0;
  TopTools_IndexedMapOfShape edgeMap;
  TopExp::MapShapes( theShape, TopAbs_EDGE, edgeMap );

  for ( int iE = 1; iE <= edgeMap.Extent(); ++iE )
  {
    const TopoDS_Edge& edge = TopoDS::Edge( edgeMap( iE ));
    Handle(Geom_Curve) C = BRep_Tool::Curve( edge, L, UMin, UMax );
    GeomAdaptor_Curve AdaptCurve(C, UMin, UMax);
    if ( AdaptCurve.GetType() != GeomAbs_Line )
    {
      vector< double > params;
      SMESHDS_Mesh* aMeshDS = const_cast< SMESH_Mesh* >( theMesh )->GetMeshDS();
      if ( SMESH_Algo::GetNodeParamOnEdge( aMeshDS, edge, params ))
      {
        nbEdges++;
        for ( int i = 1; i < params.size(); ++i )
          _value = Max( _value, deflection( AdaptCurve, params[ i-1 ], params[ i ]));
      }
    }
    else
      nbEdges++;
  }
  return nbEdges;
}

//================================================================================
/*!
 * \brief Initialize my parameter values by default parameters.
 *  \retval bool - true if parameter values have been successfully defined
 */
//================================================================================

bool StdMeshers_Deflection1D::SetParametersByDefaults(const TDefaults&  /*dflts*/,
                                                      const SMESH_Mesh* /*theMesh*/)
{
  return false;
}
