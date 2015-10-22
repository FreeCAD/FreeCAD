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
// File      : SMESH_Pattern.hxx
// Created   : Mon Aug  2 10:30:00 2004
// Author    : Edward AGAPOV (eap)
//
#include "SMESH_Block.hxx"

#include <BRepAdaptor_Curve.hxx>
#include <BRepAdaptor_Curve2d.hxx>
#include <BRepAdaptor_Surface.hxx>
#include <BRepTools.hxx>
#include <BRepTools_WireExplorer.hxx>
#include <BRep_Builder.hxx>
#include <BRep_Tool.hxx>
#include <Bnd_Box.hxx>
#include <Extrema_ExtPC.hxx>
#include <Extrema_POnCurv.hxx>
#include <Geom2d_Curve.hxx>
#include <ShapeAnalysis.hxx>
#include <TopExp_Explorer.hxx>
#include <TopTools_IndexedDataMapOfShapeListOfShape.hxx>
#include <TopTools_ListIteratorOfListOfShape.hxx>
#include <TopTools_ListOfShape.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Compound.hxx>
#include <TopoDS_Face.hxx>
#include <TopoDS_Iterator.hxx>
#include <TopoDS_Wire.hxx>
#include <gp_Trsf.hxx>
#include <gp_Vec.hxx>
#include <math_FunctionSetRoot.hxx>
#include <math_Matrix.hxx>
#include <math_Vector.hxx>

#include "SMDS_MeshNode.hxx"
#include "SMDS_MeshVolume.hxx"
#include "SMDS_VolumeTool.hxx"
#include "utilities.h"

#include <list>

using namespace std;

//#define DEBUG_PARAM_COMPUTE

//================================================================================
/*!
 * \brief Set edge data
  * \param edgeID - block subshape ID
  * \param curve - edge geometry
  * \param isForward - is curve orientation coincides with edge orientation in the block
 */
//================================================================================

void SMESH_Block::TEdge::Set( const int edgeID, Adaptor3d_Curve* curve, const bool isForward )
{
  myCoordInd = SMESH_Block::GetCoordIndOnEdge( edgeID );
  if ( myC3d ) delete myC3d;
  myC3d = curve;
  myFirst = curve->FirstParameter();
  myLast = curve->LastParameter();
  if ( !isForward )
    std::swap( myFirst, myLast );
}

//================================================================================
/*!
 * \brief Set coordinates of nodes at edge ends to work with mesh block
  * \param edgeID - block subshape ID
  * \param node1 - coordinates of node with lower ID
  * \param node2 - coordinates of node with upper ID
 */
//================================================================================

void SMESH_Block::TEdge::Set( const int edgeID, const gp_XYZ& node1, const gp_XYZ& node2 )
{
  myCoordInd = SMESH_Block::GetCoordIndOnEdge( edgeID );
  myNodes[ 0 ] = node1;
  myNodes[ 1 ] = node2;

  if ( myC3d ) delete myC3d;
  myC3d = 0;
}

//=======================================================================
//function : SMESH_Block::TEdge::GetU
//purpose  : 
//=======================================================================

double SMESH_Block::TEdge::GetU( const gp_XYZ& theParams ) const
{
  double u = theParams.Coord( myCoordInd );
  if ( !myC3d ) // if mesh block
    return u;
  return ( 1 - u ) * myFirst + u * myLast;
}

//=======================================================================
//function : SMESH_Block::TEdge::Point
//purpose  : 
//=======================================================================

gp_XYZ SMESH_Block::TEdge::Point( const gp_XYZ& theParams ) const
{
  double u = GetU( theParams );
  if ( myC3d ) return myC3d->Value( u ).XYZ();
  // mesh block
  return myNodes[0] * ( 1 - u ) + myNodes[1] * u;
}

//================================================================================
/*!
 * \brief Destructor
 */
//================================================================================

SMESH_Block::TEdge::~TEdge()
{
  if ( myC3d ) delete myC3d;
}

//================================================================================
/*!
 * \brief Set face data
  * \param faceID - block subshape ID
  * \param S - face surface geometry
  * \param c2d - 4 pcurves in the order as returned by GetFaceEdgesIDs(faceID)
  * \param isForward - orientation of pcurves comparing with block edge direction
 */
//================================================================================

void SMESH_Block::TFace::Set( const int          faceID,
                              Adaptor3d_Surface* S,
                              Adaptor2d_Curve2d* c2D[4],
                              const bool         isForward[4] )
{
  if ( myS ) delete myS;
  myS = S;
  // pcurves
  vector< int > edgeIdVec;
  GetFaceEdgesIDs( faceID, edgeIdVec );
  for ( int iE = 0; iE < edgeIdVec.size(); iE++ ) // loop on 4 edges
  {
    myCoordInd[ iE ] = GetCoordIndOnEdge( edgeIdVec[ iE ] );
    if ( myC2d[ iE ]) delete myC2d[ iE ];
    myC2d[ iE ] = c2D[ iE ];
    myFirst[ iE ] = myC2d[ iE ]->FirstParameter();
    myLast [ iE ] = myC2d[ iE ]->LastParameter();
    if ( !isForward[ iE ])
      std::swap( myFirst[ iE ], myLast[ iE ] );
  }
  // 2d corners
  myCorner[ 0 ] = myC2d[ 0 ]->Value( myFirst[0] ).XY();
  myCorner[ 1 ] = myC2d[ 0 ]->Value( myLast[0] ).XY();
  myCorner[ 2 ] = myC2d[ 1 ]->Value( myLast[1] ).XY();
  myCorner[ 3 ] = myC2d[ 1 ]->Value( myFirst[1] ).XY();
}

//================================================================================
/*!
 * \brief Set face data to work with mesh block
  * \param faceID - block subshape ID
  * \param edgeU0 - filled data of edge u0 = GetFaceEdgesIDs(faceID)[ 0 ]
  * \param edgeU1 - filled data of edge u1 = GetFaceEdgesIDs(faceID)[ 1 ]
 */
//================================================================================

void SMESH_Block::TFace::Set( const int faceID, const TEdge& edgeU0, const TEdge& edgeU1 )
{
  vector< int > edgeIdVec;
  GetFaceEdgesIDs( faceID, edgeIdVec );
  myNodes[ 0 ] = edgeU0.NodeXYZ( 1 );
  myNodes[ 1 ] = edgeU0.NodeXYZ( 0 );
  myNodes[ 2 ] = edgeU1.NodeXYZ( 0 );
  myNodes[ 3 ] = edgeU1.NodeXYZ( 1 );
  myCoordInd[ 0 ] = GetCoordIndOnEdge( edgeIdVec[ 0 ] );
  myCoordInd[ 1 ] = GetCoordIndOnEdge( edgeIdVec[ 1 ] );
  myCoordInd[ 2 ] = GetCoordIndOnEdge( edgeIdVec[ 2 ] );
  myCoordInd[ 3 ] = GetCoordIndOnEdge( edgeIdVec[ 3 ] );
  if ( myS ) delete myS;
  myS = 0;
}

//================================================================================
/*!
 * \brief Destructor
 */
//================================================================================

SMESH_Block::TFace::~TFace()
{
  if ( myS ) delete myS;
  for ( int i = 0 ; i < 4; ++i )
    if ( myC2d[ i ]) delete myC2d[ i ];
}

//=======================================================================
//function : SMESH_Block::TFace::GetCoefs
//purpose  : return coefficients for addition of [0-3]-th edge and vertex
//=======================================================================

void SMESH_Block::TFace::GetCoefs(int           iE,
                                  const gp_XYZ& theParams,
                                  double&       Ecoef,
                                  double&       Vcoef ) const
{
  double dU = theParams.Coord( GetUInd() );
  double dV = theParams.Coord( GetVInd() );
  switch ( iE ) {
  case 0:
    Ecoef = ( 1 - dV ); // u0
    Vcoef = ( 1 - dU ) * ( 1 - dV ); break; // 00
  case 1:
    Ecoef = dV; // u1
    Vcoef = dU * ( 1 - dV ); break; // 10
  case 2:
    Ecoef = ( 1 - dU ); // 0v
    Vcoef = dU * dV  ; break; // 11
  case 3:
    Ecoef = dU  ; // 1v
    Vcoef = ( 1 - dU ) * dV  ; break; // 01
  default: ASSERT(0);
  }
}

//=======================================================================
//function : SMESH_Block::TFace::GetUV
//purpose  : 
//=======================================================================

gp_XY SMESH_Block::TFace::GetUV( const gp_XYZ& theParams ) const
{
  gp_XY uv(0.,0.);
  for ( int iE = 0; iE < 4; iE++ ) // loop on 4 edges
  {
    double Ecoef = 0, Vcoef = 0;
    GetCoefs( iE, theParams, Ecoef, Vcoef );
    // edge addition
    double u = theParams.Coord( myCoordInd[ iE ] );
    u = ( 1 - u ) * myFirst[ iE ] + u * myLast[ iE ];
    uv += Ecoef * myC2d[ iE ]->Value( u ).XY();
    // corner addition
    uv -= Vcoef * myCorner[ iE ];
  }
  return uv;
}

//=======================================================================
//function : SMESH_Block::TFace::Point
//purpose  : 
//=======================================================================

gp_XYZ SMESH_Block::TFace::Point( const gp_XYZ& theParams ) const
{
  gp_XYZ p(0.,0.,0.);
  if ( !myS ) // if mesh block
  {
    for ( int iE = 0; iE < 4; iE++ ) // loop on 4 edges
    {
      double Ecoef = 0, Vcoef = 0;
      GetCoefs( iE, theParams, Ecoef, Vcoef );
      // edge addition
      double u = theParams.Coord( myCoordInd[ iE ] );
      int i1 = 0, i2 = 1;
      switch ( iE ) {
      case 1: i1 = 3; i2 = 2; break;
      case 2: i1 = 1; i2 = 2; break;
      case 3: i1 = 0; i2 = 3; break;
      }
      p += Ecoef * ( myNodes[ i1 ] * ( 1 - u ) + myNodes[ i2 ] * u );
      // corner addition
      p -= Vcoef * myNodes[ iE ];
    }
    
  }
  else // shape block
  {
    gp_XY uv = GetUV( theParams );
    p = myS->Value( uv.X(), uv.Y() ).XYZ();
  }
  return p;
}

//=======================================================================
//function : GetShapeCoef
//purpose  : 
//=======================================================================

double* SMESH_Block::GetShapeCoef (const int theShapeID)
{
  static double shapeCoef[][3] = {
    //    V000,        V100,        V010,         V110
    { -1,-1,-1 }, {  1,-1,-1 }, { -1, 1,-1 }, {  1, 1,-1 },
    //    V001,        V101,        V011,         V111,
    { -1,-1, 1 }, {  1,-1, 1 }, { -1, 1, 1 }, {  1, 1, 1 },
    //    Ex00,        Ex10,        Ex01,         Ex11,
    {  0,-1,-1 }, {  0, 1,-1 }, {  0,-1, 1 }, {  0, 1, 1 },
    //    E0y0,        E1y0,        E0y1,         E1y1,
    { -1, 0,-1 }, {  1, 0,-1 }, { -1, 0, 1 }, {  1, 0, 1 },
    //    E00z,        E10z,        E01z,         E11z,
    { -1,-1, 0 }, {  1,-1, 0 }, { -1, 1, 0 }, {  1, 1, 0 },
    //    Fxy0,        Fxy1,        Fx0z,         Fx1z,         F0yz,           F1yz,
    {  0, 0,-1 }, {  0, 0, 1 }, {  0,-1, 0 }, {  0, 1, 0 }, { -1, 0, 0 }, {  1, 0, 0 },
    // ID_Shell
    {  0, 0, 0 }
  };
  if ( theShapeID < ID_V000 || theShapeID > ID_F1yz )
    return shapeCoef[ ID_Shell - 1 ];

  return shapeCoef[ theShapeID - 1 ];
}

//=======================================================================
//function : ShellPoint
//purpose  : return coordinates of a point in shell
//=======================================================================

bool SMESH_Block::ShellPoint( const gp_XYZ& theParams, gp_XYZ& thePoint ) const
{
  thePoint.SetCoord( 0., 0., 0. );
  for ( int shapeID = ID_V000; shapeID < ID_Shell; shapeID++ )
  {
    // coef
    double* coefs = GetShapeCoef( shapeID );
    double k = 1;
    for ( int iCoef = 0; iCoef < 3; iCoef++ ) {
      if ( coefs[ iCoef ] != 0 ) {
        if ( coefs[ iCoef ] < 0 )
          k *= ( 1. - theParams.Coord( iCoef + 1 ));
        else
          k *= theParams.Coord( iCoef + 1 );
      }
    }
    // add point on a shape
    if ( fabs( k ) > DBL_MIN )
    {
      gp_XYZ Ps;
      if ( shapeID < ID_Ex00 ) // vertex
        VertexPoint( shapeID, Ps );
      else if ( shapeID < ID_Fxy0 ) { // edge
        EdgePoint( shapeID, theParams, Ps );
        k = -k;
      } else // face
        FacePoint( shapeID, theParams, Ps );

      thePoint += k * Ps;
    }
  }
  return true;
}

//=======================================================================
//function : ShellPoint
//purpose  : computes coordinates of a point in shell by points on sub-shapes;
//           thePointOnShape[ subShapeID ] must be a point on a subShape
//=======================================================================

bool SMESH_Block::ShellPoint(const gp_XYZ&         theParams,
                             const vector<gp_XYZ>& thePointOnShape,
                             gp_XYZ&               thePoint )
{
  if ( thePointOnShape.size() < ID_F1yz )
    return false;

  double x = theParams.X(), y = theParams.Y(), z = theParams.Z();
  double x1 = 1. - x,       y1 = 1. - y,       z1 = 1. - z;
  const vector<gp_XYZ>& p = thePointOnShape;

  thePoint = 
    x1 * p[ID_F0yz] + x * p[ID_F1yz] +
    y1 * p[ID_Fx0z] + y * p[ID_Fx1z] +
    z1 * p[ID_Fxy0] + z * p[ID_Fxy1] +
    x1 * (y1 * (z1 * p[ID_V000] + z * p[ID_V001])  +
          y  * (z1 * p[ID_V010] + z * p[ID_V011])) +
    x  * (y1 * (z1 * p[ID_V100] + z * p[ID_V101])  +
          y  * (z1 * p[ID_V110] + z * p[ID_V111]));
  thePoint -=
    x1 * (y1 * p[ID_E00z] + y * p[ID_E01z]) + 
    x  * (y1 * p[ID_E10z] + y * p[ID_E11z]) + 
    y1 * (z1 * p[ID_Ex00] + z * p[ID_Ex01]) + 
    y  * (z1 * p[ID_Ex10] + z * p[ID_Ex11]) + 
    z1 * (x1 * p[ID_E0y0] + x * p[ID_E1y0]) + 
    z  * (x1 * p[ID_E0y1] + x * p[ID_E1y1]);

  return true;
}

//=======================================================================
//function : Constructor
//purpose  : 
//=======================================================================

SMESH_Block::SMESH_Block():
  myNbIterations(0),
  mySumDist(0.),
  myTolerance(-1.) // to be re-initialized
{
}


//=======================================================================
//function : NbVariables
//purpose  : 
//=======================================================================

Standard_Integer SMESH_Block::NbVariables() const
{
  return 3;
}

//=======================================================================
//function : NbEquations
//purpose  : 
//=======================================================================

Standard_Integer SMESH_Block::NbEquations() const
{
  return 1;
}

//=======================================================================
//function : Value
//purpose  : 
//=======================================================================

Standard_Boolean SMESH_Block::Value(const math_Vector& theXYZ, math_Vector& theFxyz) 
{
  gp_XYZ P, params( theXYZ(1), theXYZ(2), theXYZ(3) );
  if ( params.IsEqual( myParam, DBL_MIN )) { // same param
    theFxyz( 1 ) = funcValue( myValues[ SQUARE_DIST ]);
  }
  else {
    ShellPoint( params, P );
    gp_Vec dP( P - myPoint );
    theFxyz(1) = funcValue( dP.SquareMagnitude() );
  }
  return true;
}

//=======================================================================
//function : Derivatives
//purpose  : 
//=======================================================================

Standard_Boolean SMESH_Block::Derivatives(const math_Vector& XYZ,math_Matrix& Df) 
{
  math_Vector F(1,3);
  return Values(XYZ,F,Df);
}

//=======================================================================
//function : GetStateNumber
//purpose  : 
//=======================================================================

Standard_Integer SMESH_Block::GetStateNumber ()
{
  return 0; //myValues[0] < 1e-1;
}

//=======================================================================
//function : Values
//purpose  : 
//=======================================================================

Standard_Boolean SMESH_Block::Values(const math_Vector& theXYZ,
                                     math_Vector&       theFxyz,
                                     math_Matrix&       theDf) 
{
  gp_XYZ P, params( theXYZ(1), theXYZ(2), theXYZ(3) );
  if ( params.IsEqual( myParam, DBL_MIN )) { // same param
    theFxyz( 1 )      = funcValue( myValues[ SQUARE_DIST ] );
    theDf( 1, DRV_1 ) = myValues[ DRV_1 ];
    theDf( 1, DRV_2 ) = myValues[ DRV_2 ];
    theDf( 1, DRV_3 ) = myValues[ DRV_3 ];
    return true;
  }
#ifdef DEBUG_PARAM_COMPUTE
  MESSAGE ( "PARAM GUESS: " << params.X() << " "<< params.Y() << " "<< params.X() );
  myNbIterations++; // how many times call ShellPoint()
#endif
  ShellPoint( params, P );

  gp_Vec dP( myPoint, P );
  double sqDist = dP.SquareMagnitude();
  theFxyz(1) = funcValue( sqDist );

  if ( sqDist < myTolerance * myTolerance ) { // a solution found
    myParam = params;
    myValues[ SQUARE_DIST ] = sqDist;
    theFxyz(1)  = theDf( 1,1 ) = theDf( 1,2 ) = theDf( 1,3 ) = 0;
    return true;
  }

  if ( sqDist < myValues[ SQUARE_DIST ] ) // a better guess
  {
    // 3 partial derivatives
    gp_Vec drv[ 3 ]; // where we move with a small step in each direction
    for ( int iP = 1; iP <= 3; iP++ ) {
      if ( iP == myFaceIndex ) {
        drv[ iP - 1 ] = gp_Vec(0,0,0);
        continue;
      }
      gp_XYZ Pi;
      bool onEdge = ( theXYZ( iP ) + 0.001 > 1. );
      if ( onEdge )
        params.SetCoord( iP, theXYZ( iP ) - 0.001 );
      else
        params.SetCoord( iP, theXYZ( iP ) + 0.001 );
      ShellPoint( params, Pi );
      params.SetCoord( iP, theXYZ( iP ) ); // restore params
      gp_Vec dPi ( P, Pi );
      if ( onEdge ) dPi *= -1.;
      double mag = dPi.Magnitude();
      if ( mag > DBL_MIN )
        dPi /= mag;
      drv[ iP - 1 ] = dPi;
    }
    for ( int iP = 0; iP < 3; iP++ ) {
#if 1
      theDf( 1, iP + 1 ) = dP * drv[iP];
#else
      // Distance from P to plane passing through myPoint and defined
      // by the 2 other derivative directions:
      // like IntAna_IntConicQuad::Perform (const gp_Lin& L, const gp_Pln& P)
      // where L is (P -> myPoint), P is defined by the 2 other derivative direction
      int iPrev = ( iP ? iP - 1 : 2 );
      int iNext = ( iP == 2 ? 0 : iP + 1 );
      gp_Vec plnNorm = drv[ iPrev ].Crossed( drv [ iNext ] );
      double Direc = plnNorm * drv[ iP ];
      if ( Abs(Direc) <= DBL_MIN )
        theDf( 1, iP + 1 ) = dP * drv[ iP ];
      else {
        double Dis = plnNorm * P - plnNorm * myPoint;
        theDf( 1, iP + 1 ) = Dis/Direc;
      }
#endif
    }
#ifdef DEBUG_PARAM_COMPUTE
    MESSAGE ( "F = " << theFxyz(1) << " DRV: " << theDf(1,1) << " " << theDf(1,2) << " " << theDf(1,3) );
    myNbIterations +=3; // how many times call ShellPoint()
#endif

    // store better values
    myParam              = params;
    myValues[SQUARE_DIST]= sqDist;
    myValues[DRV_1]      = theDf(1,DRV_1);
    myValues[DRV_2]      = theDf(1,DRV_2);
    myValues[DRV_3]      = theDf(1,DRV_3);
  }

  return true;
}

//============================================================================
//function : computeParameters
//purpose  : compute point parameters in the block using math_FunctionSetRoot
//============================================================================

bool SMESH_Block::computeParameters(const gp_Pnt& thePoint,
                                    gp_XYZ&       theParams,
                                    const gp_XYZ& theParamsHint)
{
  myPoint = thePoint.XYZ();

  myParam.SetCoord( -1,-1,-1 );
  myValues[ SQUARE_DIST ] = 1e100;

  math_Vector low  ( 1, 3, 0.0 );
  math_Vector up   ( 1, 3, 1.0 );
  math_Vector tol  ( 1, 3, 1e-4 );
  math_Vector start( 1, 3, 0.0 );
  start( 1 ) = theParamsHint.X();
  start( 2 ) = theParamsHint.Y();
  start( 3 ) = theParamsHint.Z();

  math_FunctionSetRoot paramSearch( *this, tol );

  mySquareFunc = 0; // large approaching steps
  //if ( hasHint ) mySquareFunc = 1; // small approaching steps

  double loopTol = 10 * myTolerance;
  int nbLoops = 0;
  while ( distance() > loopTol && nbLoops <= 3 )
  {
    paramSearch.Perform ( *static_cast<math_FunctionSetWithDerivatives*>(this),
                          start, low, up );
    start( 1 ) = myParam.X();
    start( 2 ) = myParam.Y();
    start( 3 ) = myParam.Z();
    mySquareFunc = !mySquareFunc;
    nbLoops++;
  }
#ifdef DEBUG_PARAM_COMPUTE
  mySumDist += distance();
  MESSAGE ( " ------ SOLUTION: ( "<< myParam.X() <<" "<< myParam.Y() <<" "<< myParam.Z() <<" )"<<endl
         << " ------ DIST : " << distance() << "\t Tol=" << myTolerance << "\t Nb LOOPS=" << nbLoops << endl
         << " ------ NB IT: " << myNbIterations << ",  SUM DIST: " << mySumDist );
#endif

  theParams = myParam;

  if ( myFaceIndex > 0 )
    theParams.SetCoord( myFaceIndex, myFaceParam );

  return true;
}

//=======================================================================
//function : ComputeParameters
//purpose  : compute point parameters in the block
//=======================================================================

bool SMESH_Block::ComputeParameters(const gp_Pnt& thePoint,
                                    gp_XYZ&       theParams,
                                    const int     theShapeID,
                                    const gp_XYZ& theParamsHint)
{
  if ( VertexParameters( theShapeID, theParams ))
    return true;

  if ( IsEdgeID( theShapeID )) {
    TEdge& e = myEdge[ theShapeID - ID_FirstE ];
    Adaptor3d_Curve* curve = e.GetCurve();
    Extrema_ExtPC anExtPC( thePoint, *curve, curve->FirstParameter(), curve->LastParameter() );
    int i, nb = anExtPC.IsDone() ? anExtPC.NbExt() : 0;
    for ( i = 1; i <= nb; i++ ) {
      if ( anExtPC.IsMin( i ))
        return EdgeParameters( theShapeID, anExtPC.Point( i ).Parameter(), theParams );
    }
    return false;
  }

  const bool isOnFace = IsFaceID( theShapeID );
  double * coef = GetShapeCoef( theShapeID );

  // Find the first guess paremeters

  gp_XYZ start(0, 0, 0);

  bool hasHint = ( 0 <= theParamsHint.X() && theParamsHint.X() <= 1 &&
                   0 <= theParamsHint.Y() && theParamsHint.Y() <= 1 &&
                   0 <= theParamsHint.Y() && theParamsHint.Y() <= 1 );
  if ( !hasHint && !myGridComputed )
  {
    // define the first guess by thePoint projection on lines
    // connecting vertices
    bool needGrid = false;
    gp_XYZ par000( 0, 0, 0 ), par111( 1, 1, 1 );
    double zero = DBL_MIN * DBL_MIN;
    for ( int iEdge = 0, iParam = 1; iParam <= 3 && !needGrid; iParam++ )
    {
      if ( isOnFace && coef[ iParam - 1 ] != 0 ) {
        iEdge += 4;
        continue;
      }
      double sumParam = 0;
      for ( int iE = 0; iE < 4; iE++, iEdge++ ) { // loop on 4 parallel edges
        gp_Pnt p0 = myEdge[ iEdge ].Point( par000 );
        gp_Pnt p1 = myEdge[ iEdge ].Point( par111 );
        gp_Vec v01( p0, p1 ), v0P( p0, thePoint );
        double len2 = v01.SquareMagnitude();
        double par = 0;
        if ( len2 > zero ) {
          par = v0P.Dot( v01 ) / len2;
          if ( par < 0 || par > 1 ) { // projection falls out of line ends => needGrid
            needGrid = true;
            break;
          }
        }
        sumParam += par;
      }
      start.SetCoord( iParam, sumParam / 4.);
    }
    if ( needGrid ) {
      // compute nodes of 3 x 3 x 3 grid
      int iNode = 0;
      Bnd_Box box;
      for ( double x = 0.25; x < 0.9; x += 0.25 )
        for ( double y = 0.25; y < 0.9; y += 0.25 )
          for ( double z = 0.25; z < 0.9; z += 0.25 ) {
            TxyzPair & prmPtn = my3x3x3GridNodes[ iNode++ ];
            prmPtn.first.SetCoord( x, y, z );
            ShellPoint( prmPtn.first, prmPtn.second );
            box.Add( gp_Pnt( prmPtn.second ));
          }
      myGridComputed = true;
      myTolerance = sqrt( box.SquareExtent() ) * 1e-5;
    }
  }

  if ( hasHint )
  {
    start = theParamsHint;
  }
  else if ( myGridComputed )
  {
    double minDist = DBL_MAX;
    gp_XYZ* bestParam = 0;
    for ( int iNode = 0; iNode < 27; iNode++ ) {
      TxyzPair & prmPtn = my3x3x3GridNodes[ iNode ];
      double dist = ( thePoint.XYZ() - prmPtn.second ).SquareModulus();
      if ( dist < minDist ) {
        minDist = dist;
        bestParam = & prmPtn.first;
      }
    }
    start = *bestParam;
  }

  myFaceIndex = -1;
  myFaceParam = 0.;
  if ( isOnFace ) {
    // put a point on the face
    for ( int iCoord = 0; iCoord < 3; iCoord++ )
      if ( coef[ iCoord ] ) {
        myFaceIndex = iCoord + 1;
        myFaceParam = ( coef[ iCoord ] < 0.5 ) ? 0.0 : 1.0;
        start.SetCoord( myFaceIndex, myFaceParam );
      }
  }

#ifdef DEBUG_PARAM_COMPUTE
  MESSAGE ( " #### POINT " <<thePoint.X()<<" "<<thePoint.Y()<<" "<<thePoint.Z()<<" ####" );
#endif

  if ( myTolerance < 0 ) myTolerance = 1e-6;

  const double parDelta = 1e-4;
  const double sqTolerance = myTolerance * myTolerance;

  gp_XYZ solution = start, params = start;
  double sqDistance = 1e100; 
  int nbLoops = 0, nbGetWorst = 0;

  while ( nbLoops <= 100 )
  {
    gp_XYZ P, Pi;
    ShellPoint( params, P );

    gp_Vec dP( thePoint, P );
    double sqDist = dP.SquareMagnitude();

    if ( sqDist > sqDistance ) { // solution get worse
      if ( ++nbGetWorst > 2 )
        return computeParameters( thePoint, theParams, solution );
    }
#ifdef DEBUG_PARAM_COMPUTE
    MESSAGE ( "PARAMS: ( " << params.X() <<" "<< params.Y() <<" "<< params.Z() <<" )" );
    MESSAGE ( "DIST: " << sqrt( sqDist ) );
#endif

    if ( sqDist < sqDistance ) { // get better
      sqDistance = sqDist;
      solution   = params;
      nbGetWorst = 0;
      if ( sqDistance < sqTolerance ) // a solution found
        break;
    }

        // look for a next better solution
    for ( int iP = 1; iP <= 3; iP++ ) {
      if ( iP == myFaceIndex )
        continue;
      // see where we move with a small (=parDelta) step in this direction
      gp_XYZ nearParams = params;
      bool onEdge = ( params.Coord( iP ) + parDelta > 1. );
      if ( onEdge )
        nearParams.SetCoord( iP, params.Coord( iP ) - parDelta );
      else
        nearParams.SetCoord( iP, params.Coord( iP ) + parDelta );
      ShellPoint( nearParams, Pi );
      gp_Vec dPi ( P, Pi );
      if ( onEdge ) dPi *= -1.;
      // modify a parameter
      double mag = dPi.Magnitude();
      if ( mag < DBL_MIN )
        continue;
      gp_Vec dir = dPi / mag; // dir we move modifying the parameter
      double dist = dir * dP; // where we should get to
      double dPar = dist / mag * parDelta; // predict parameter change
      double curPar = params.Coord( iP );
      double par = curPar - dPar; // new parameter value
      while ( par > 1 || par < 0 ) {
        dPar /= 2.;
        par = curPar - dPar;
      }
      params.SetCoord( iP, par );
    }

    nbLoops++;
  }
#ifdef DEBUG_PARAM_COMPUTE
  myNbIterations += nbLoops*4; // how many times ShellPoint called
  mySumDist += sqrt( sqDistance );
  MESSAGE ( " ------ SOLUTION: ( "<<solution.X()<<" "<<solution.Y()<<" "<<solution.Z()<<" )"<< std::endl
         << " ------ DIST : " << sqrt( sqDistance ) << "\t Tol=" << myTolerance << "\t Nb LOOPS=" << nbLoops << std::endl
         << " ------ NB IT: " << myNbIterations << ",  SUM DIST: " << mySumDist );
#endif

  theParams = solution;

  if ( myFaceIndex > 0 )
    theParams.SetCoord( myFaceIndex, myFaceParam );

  return true;
}

//=======================================================================
//function : VertexParameters
//purpose  : return parameters of a vertex given by TShapeID
//=======================================================================

bool SMESH_Block::VertexParameters(const int theVertexID, gp_XYZ& theParams)
{
  switch ( theVertexID ) {
  case ID_V000: theParams.SetCoord(0., 0., 0.); return true;
  case ID_V100: theParams.SetCoord(1., 0., 0.); return true;
  case ID_V110: theParams.SetCoord(1., 1., 0.); return true;
  case ID_V010: theParams.SetCoord(0., 1., 0.); return true;
  default:;
  }
  return false;
}

//=======================================================================
//function : EdgeParameters
//purpose  : return parameters of a point given by theU on edge
//=======================================================================

bool SMESH_Block::EdgeParameters(const int theEdgeID, const double theU, gp_XYZ& theParams)
{
  if ( IsEdgeID( theEdgeID )) {
    vector< int > vertexVec;
    GetEdgeVertexIDs( theEdgeID, vertexVec );
    VertexParameters( vertexVec[0], theParams );
    TEdge& e = myEdge[ theEdgeID - ID_Ex00 ];
    double param = ( theU - e.EndParam(0) ) / ( e.EndParam(1) - e.EndParam(0) );
    theParams.SetCoord( e.CoordInd(), param );
    return true;
  }
  return false;
}

//=======================================================================
//function : DumpShapeID
//purpose  : debug an id of a block sub-shape
//=======================================================================

#define CASEDUMP(id,strm) case id: strm << #id; break;

ostream& SMESH_Block::DumpShapeID (const int id, ostream& stream)
{
  switch ( id ) {
  CASEDUMP( ID_V000, stream );
  CASEDUMP( ID_V100, stream );
  CASEDUMP( ID_V010, stream );
  CASEDUMP( ID_V110, stream );
  CASEDUMP( ID_V001, stream );
  CASEDUMP( ID_V101, stream );
  CASEDUMP( ID_V011, stream );
  CASEDUMP( ID_V111, stream );
  CASEDUMP( ID_Ex00, stream );
  CASEDUMP( ID_Ex10, stream );
  CASEDUMP( ID_Ex01, stream );
  CASEDUMP( ID_Ex11, stream );
  CASEDUMP( ID_E0y0, stream );
  CASEDUMP( ID_E1y0, stream );
  CASEDUMP( ID_E0y1, stream );
  CASEDUMP( ID_E1y1, stream );
  CASEDUMP( ID_E00z, stream );
  CASEDUMP( ID_E10z, stream );
  CASEDUMP( ID_E01z, stream );
  CASEDUMP( ID_E11z, stream );
  CASEDUMP( ID_Fxy0, stream );
  CASEDUMP( ID_Fxy1, stream );
  CASEDUMP( ID_Fx0z, stream );
  CASEDUMP( ID_Fx1z, stream );
  CASEDUMP( ID_F0yz, stream );
  CASEDUMP( ID_F1yz, stream );
  CASEDUMP( ID_Shell, stream );
  default: stream << "ID_INVALID";
  }
  return stream;
}

//=======================================================================
//function : GetShapeIDByParams
//purpose  : define an id of the block sub-shape by normlized point coord
//=======================================================================

int SMESH_Block::GetShapeIDByParams ( const gp_XYZ& theCoord )
{
  //   id ( 0 - 26 ) computation:

  //   vertex     ( 0 - 7 )  : id = 1*x + 2*y + 4*z

  //   edge || X  ( 8 - 11 ) : id = 8   + 1*y + 2*z
  //   edge || Y  ( 12 - 15 ): id = 1*x + 12  + 2*z
  //   edge || Z  ( 16 - 19 ): id = 1*x + 2*y + 16 

  //   face || XY ( 20 - 21 ): id = 8   + 12  + 1*z - 0
  //   face || XZ ( 22 - 23 ): id = 8   + 1*y + 16  - 2
  //   face || YZ ( 24 - 25 ): id = 1*x + 12  + 16  - 4

  static int iAddBnd[]    = { 1, 2, 4 };
  static int iAddNotBnd[] = { 8, 12, 16 };
  static int iFaceSubst[] = { 0, 2, 4 };

  int id = 0;
  int iOnBoundary = 0;
  for ( int iCoord = 0; iCoord < 3; iCoord++ )
  {
    double val = theCoord.Coord( iCoord + 1 );
    if ( val == 0.0 )
      iOnBoundary++;
    else if ( val == 1.0 )
      id += iAddBnd[ iOnBoundary++ ];
    else
      id += iAddNotBnd[ iCoord ];
  }
  if ( iOnBoundary == 1 ) // face
    id -= iFaceSubst[ (id - 20) / 4 ];
  else if ( iOnBoundary == 0 ) // shell
    id = 26;

  if ( id > 26 || id < 0 ) {
    MESSAGE( "GetShapeIDByParams() = " << id
            <<" "<< theCoord.X() <<" "<< theCoord.Y() <<" "<< theCoord.Z() );
  }

  return id + 1; // shape ids start at 1
}

//=======================================================================
//function : GetOrderedEdges
//purpose  : return nb wires and a list of oredered edges
//=======================================================================

int SMESH_Block::GetOrderedEdges (const TopoDS_Face&   theFace,
                                  TopoDS_Vertex        theFirstVertex,
                                  list< TopoDS_Edge >& theEdges,
                                  list< int >  &       theNbVertexInWires)
{
  // put wires in a list, so that an outer wire comes first
  list<TopoDS_Wire> aWireList;
  //TopoDS_Wire anOuterWire = BRepTools::OuterWire( theFace ); ### issue 0020184
  TopoDS_Wire anOuterWire = ShapeAnalysis::OuterWire( theFace );
  //aWireList.push_back( anOuterWire ); ### issue 0020184
  for ( TopoDS_Iterator wIt (theFace); wIt.More(); wIt.Next() )
    if ( !anOuterWire.IsSame( wIt.Value() ))
      aWireList.push_back( TopoDS::Wire( wIt.Value() ));
    else
      aWireList.push_front( TopoDS::Wire( wIt.Value() ));// ### issue 0020184

  // loop on edges of wires
  theNbVertexInWires.clear();
  list<TopoDS_Wire>::iterator wlIt = aWireList.begin();
  for ( ; wlIt != aWireList.end(); wlIt++ )
  {
    int iE;
    BRepTools_WireExplorer wExp( *wlIt, theFace );
    for ( iE = 0; wExp.More(); wExp.Next(), iE++ )
    {
      TopoDS_Edge edge = wExp.Current();
      edge = TopoDS::Edge( edge.Oriented( wExp.Orientation() ));
      theEdges.push_back( edge );
    }
    theNbVertexInWires.push_back( iE );
    iE = 0;
    if ( wlIt == aWireList.begin() && theEdges.size() > 1 ) { // the outer wire
      // orient closed edges
      list< TopoDS_Edge >::iterator eIt, eIt2;
      for ( eIt = theEdges.begin(); eIt != theEdges.end(); eIt++ )
      {
        TopoDS_Edge& edge = *eIt;
        if ( TopExp::FirstVertex( edge ).IsSame( TopExp::LastVertex( edge ) ))
        {
          eIt2 = eIt;
          bool isNext = ( eIt2 == theEdges.begin() );
          TopoDS_Edge edge2 = isNext ? *(++eIt2) : *(--eIt2);
          double f1,l1,f2,l2;
          Handle(Geom2d_Curve) c1 = BRep_Tool::CurveOnSurface( edge, theFace, f1,l1 );
          Handle(Geom2d_Curve) c2 = BRep_Tool::CurveOnSurface( edge2, theFace, f2,l2 );
          gp_Pnt2d pf = c1->Value( edge.Orientation() == TopAbs_FORWARD ? f1 : l1 );
          gp_Pnt2d pl = c1->Value( edge.Orientation() == TopAbs_FORWARD ? l1 : f1 );
          bool isFirst = ( edge2.Orientation() == TopAbs_FORWARD ? isNext : !isNext );
          gp_Pnt2d p2 = c2->Value( isFirst ? f2 : l2 );
          isFirst = ( p2.SquareDistance( pf ) < p2.SquareDistance( pl ));
          if ( isNext ? isFirst : !isFirst )
            edge.Reverse();
          // to make a seam go first
          if ( theFirstVertex.IsNull() )
            theFirstVertex = TopExp::FirstVertex( edge, true );
        }
      }
      // rotate theEdges until it begins from theFirstVertex
      if ( ! theFirstVertex.IsNull() ) {
        TopoDS_Vertex vv[2];
        TopExp::Vertices( theEdges.front(), vv[0], vv[1], true );
        // on closed face, make seam edge the first in the list
        while ( !vv[0].IsSame( theFirstVertex ) || vv[0].IsSame( vv[1] ))
        {
          theEdges.splice(theEdges.end(), theEdges,
                          theEdges.begin(), ++theEdges.begin());
          TopExp::Vertices( theEdges.front(), vv[0], vv[1], true );
          if ( iE++ > theNbVertexInWires.back() ) {
#ifdef _DEBUG_
            gp_Pnt p = BRep_Tool::Pnt( theFirstVertex );
            MESSAGE ( " : Warning : vertex "<< theFirstVertex.TShape().operator->()
                   << " ( " << p.X() << " " << p.Y() << " " << p.Z() << " )" 
                   << " not found in outer wire of face "<< theFace.TShape().operator->()
                   << " with vertices: " );
            wExp.Init( *wlIt, theFace );
            for ( int i = 0; wExp.More(); wExp.Next(), i++ )
            {
              TopoDS_Edge edge = wExp.Current();
              edge = TopoDS::Edge( edge.Oriented( wExp.Orientation() ));
              TopoDS_Vertex v = TopExp::FirstVertex( edge, true );
              gp_Pnt p = BRep_Tool::Pnt( v );
              MESSAGE_ADD ( i << " " << v.TShape().operator->() << " "
                            << p.X() << " " << p.Y() << " " << p.Z() << " " << std::endl );
            }
#endif
            break; // break infinite loop
          }
        }
      }
    } // end outer wire
  }

  return aWireList.size();
}
//================================================================================
/*!
 * \brief Call it after geometry initialisation
 */
//================================================================================

void SMESH_Block::init()
{
  myNbIterations = 0;
  mySumDist = 0;
  myGridComputed = false;
}

//=======================================================================
//function : LoadMeshBlock
//purpose  : prepare to work with theVolume
//=======================================================================

#define gpXYZ(n) gp_XYZ(n->X(),n->Y(),n->Z())

bool SMESH_Block::LoadMeshBlock(const SMDS_MeshVolume*        theVolume,
                                const int                     theNode000Index,
                                const int                     theNode001Index,
                                vector<const SMDS_MeshNode*>& theOrderedNodes)
{
  MESSAGE(" ::LoadMeshBlock()");
  init();

  SMDS_VolumeTool vTool;
  if (!vTool.Set( theVolume ) || vTool.NbNodes() != 8 ||
      !vTool.IsLinked( theNode000Index, theNode001Index )) {
    MESSAGE(" Bad arguments ");
    return false;
  }
  vTool.SetExternalNormal();
  // In terms of indices used for access to nodes and faces in SMDS_VolumeTool:
  int V000, V100, V010, V110, V001, V101, V011, V111; // 8 vertices
  int Fxy0, Fxy1; // bottom and top faces
  // vertices of faces
  vector<int> vFxy0, vFxy1;

  V000 = theNode000Index;
  V001 = theNode001Index;

  // get faces sharing V000 and V001
  list<int> fV000, fV001;
  int i, iF, iE, iN;
  for ( iF = 0; iF < vTool.NbFaces(); ++iF ) {
    const int* nid = vTool.GetFaceNodesIndices( iF );
    for ( iN = 0; iN < 4; ++iN )
      if ( nid[ iN ] == V000 ) {
        fV000.push_back( iF );
      } else if ( nid[ iN ] == V001 ) {
        fV001.push_back( iF );
      }
  }

  // find the bottom (Fxy0), the top (Fxy1) faces
  list<int>::iterator fIt1, fIt2, Fxy0Pos;
  for ( fIt1 = fV000.begin(); fIt1 != fV000.end(); fIt1++) {
    fIt2 = std::find( fV001.begin(), fV001.end(), *fIt1 );
    if ( fIt2 != fV001.end() ) { // *fIt1 is in the both lists
      fV001.erase( fIt2 ); // erase Fx0z or F0yz from fV001
    } else { // *fIt1 is in fV000 only
      Fxy0Pos = fIt1; // points to Fxy0
    }
  }
  Fxy0 = *Fxy0Pos;
  Fxy1 = fV001.front();
  const SMDS_MeshNode** nn = vTool.GetNodes();

  // find bottom veritices, their order is that a face normal is external
  vFxy0.resize(4);
  const int* nid = vTool.GetFaceNodesIndices( Fxy0 );
  for ( i = 0; i < 4; ++i )
    if ( nid[ i ] == V000 )
      break;
  for ( iN = 0; iN < 4; ++iN, ++i ) {
    if ( i == 4 ) i = 0;
    vFxy0[ iN ] = nid[ i ];
  }
  // find top veritices, their order is that a face normal is external
  vFxy1.resize(4);
  nid = vTool.GetFaceNodesIndices( Fxy1 );
  for ( i = 0; i < 4; ++i )
    if ( nid[ i ] == V001 )
      break;
  for ( iN = 0; iN < 4; ++iN, ++i ) {
    if ( i == 4 ) i = 0;
    vFxy1[ iN ] = nid[ i ];
  }
  // find indices of the rest veritices 
  V100 = vFxy0[3];
  V010 = vFxy0[1];
  V110 = vFxy0[2];
  V101 = vFxy1[1];
  V011 = vFxy1[3];
  V111 = vFxy1[2];

  // set points coordinates
  myPnt[ ID_V000 - 1 ] = gpXYZ( nn[ V000 ] );
  myPnt[ ID_V100 - 1 ] = gpXYZ( nn[ V100 ] );
  myPnt[ ID_V010 - 1 ] = gpXYZ( nn[ V010 ] );
  myPnt[ ID_V110 - 1 ] = gpXYZ( nn[ V110 ] );
  myPnt[ ID_V001 - 1 ] = gpXYZ( nn[ V001 ] );
  myPnt[ ID_V101 - 1 ] = gpXYZ( nn[ V101 ] );
  myPnt[ ID_V011 - 1 ] = gpXYZ( nn[ V011 ] );
  myPnt[ ID_V111 - 1 ] = gpXYZ( nn[ V111 ] );

  // fill theOrderedNodes
  theOrderedNodes.resize( 8 );
  theOrderedNodes[ 0 ] = nn[ V000 ];
  theOrderedNodes[ 1 ] = nn[ V100 ];
  theOrderedNodes[ 2 ] = nn[ V010 ];
  theOrderedNodes[ 3 ] = nn[ V110 ];
  theOrderedNodes[ 4 ] = nn[ V001 ];
  theOrderedNodes[ 5 ] = nn[ V101 ];
  theOrderedNodes[ 6 ] = nn[ V011 ];
  theOrderedNodes[ 7 ] = nn[ V111 ];
  
  // fill edges
  vector< int > vertexVec;
  for ( iE = 0; iE < NbEdges(); ++iE ) {
    GetEdgeVertexIDs(( iE + ID_FirstE ), vertexVec );
    myEdge[ iE ].Set(( iE + ID_FirstE ),
                     myPnt[ vertexVec[0] - 1 ],
                     myPnt[ vertexVec[1] - 1 ]);
  }

  // fill faces' corners
  for ( iF = ID_Fxy0; iF < ID_Shell; ++iF )
  {
    TFace& tFace = myFace[ iF - ID_FirstF ];
    vector< int > edgeIdVec(4, -1);
    GetFaceEdgesIDs( iF, edgeIdVec );
    tFace.Set( iF, myEdge[ edgeIdVec [ 0 ] - ID_Ex00], myEdge[ edgeIdVec [ 1 ] - ID_Ex00]);
  }

  return true;
}

//=======================================================================
//function : LoadBlockShapes
//purpose  : Initialize block geometry with theShell,
//           add sub-shapes of theBlock to theShapeIDMap so that they get
//           IDs acoording to enum TShapeID
//=======================================================================

bool SMESH_Block::LoadBlockShapes(const TopoDS_Shell&         theShell,
                                  const TopoDS_Vertex&        theVertex000,
                                  const TopoDS_Vertex&        theVertex001,
                                  TopTools_IndexedMapOfOrientedShape& theShapeIDMap )
{
  MESSAGE(" ::LoadBlockShapes()");
  return ( FindBlockShapes( theShell, theVertex000, theVertex001, theShapeIDMap ) &&
           LoadBlockShapes( theShapeIDMap ));
}

//=======================================================================
//function : LoadBlockShapes
//purpose  : add sub-shapes of theBlock to theShapeIDMap so that they get
//           IDs acoording to enum TShapeID
//=======================================================================

bool SMESH_Block::FindBlockShapes(const TopoDS_Shell&         theShell,
                                  const TopoDS_Vertex&        theVertex000,
                                  const TopoDS_Vertex&        theVertex001,
                                  TopTools_IndexedMapOfOrientedShape& theShapeIDMap )
{
  MESSAGE(" ::FindBlockShapes()");

  // 8 vertices
  TopoDS_Shape V000, V100, V010, V110, V001, V101, V011, V111;
  // 12 edges
  TopoDS_Shape Ex00, Ex10, Ex01, Ex11;
  TopoDS_Shape E0y0, E1y0, E0y1, E1y1;
  TopoDS_Shape E00z, E10z, E01z, E11z;
  // 6 faces
  TopoDS_Shape Fxy0, Fx0z, F0yz, Fxy1, Fx1z, F1yz;

  // nb of faces bound to a vertex in TopTools_IndexedDataMapOfShapeListOfShape
  // filled by TopExp::MapShapesAndAncestors()
  const int NB_FACES_BY_VERTEX = 6;

  TopTools_IndexedDataMapOfShapeListOfShape vfMap;
  TopExp::MapShapesAndAncestors( theShell, TopAbs_VERTEX, TopAbs_FACE, vfMap );
  if ( vfMap.Extent() != 8 ) {
    MESSAGE(" Wrong nb of vertices in the block: " << vfMap.Extent() );
    return false;
  }

  V000 = theVertex000;
  V001 = theVertex001;

  if ( V000.IsNull() ) {
    // find vertex 000 - the one with smallest coordinates
    double minVal = DBL_MAX, minX, val;
    for ( int i = 1; i <= 8; i++ ) {
      const TopoDS_Vertex& v = TopoDS::Vertex( vfMap.FindKey( i ));
      gp_Pnt P = BRep_Tool::Pnt( v );
      val = P.X() + P.Y() + P.Z();
      if ( val < minVal || ( val == minVal && P.X() < minX )) {
        V000 = v;
        minVal = val;
        minX = P.X();
      }
    }
    // find vertex 001 - the one on the most vertical edge passing through V000
    TopTools_IndexedDataMapOfShapeListOfShape veMap;
    TopExp::MapShapesAndAncestors( theShell, TopAbs_VERTEX, TopAbs_EDGE, veMap );
    gp_Vec dir001 = gp::DZ();
    gp_Pnt p000 = BRep_Tool::Pnt( TopoDS::Vertex( V000 ));
    double maxVal = -DBL_MAX;
    TopTools_ListIteratorOfListOfShape eIt ( veMap.FindFromKey( V000 ));
    for (  ; eIt.More(); eIt.Next() ) {
      const TopoDS_Edge& e = TopoDS::Edge( eIt.Value() );
      TopoDS_Vertex v = TopExp::FirstVertex( e );
      if ( v.IsSame( V000 ))
        v = TopExp::LastVertex( e );
      val = dir001 * gp_Vec( p000, BRep_Tool::Pnt( v )).Normalized();
      if ( val > maxVal ) {
        V001 = v;
        maxVal = val;
      }
    }
  }

  // find the bottom (Fxy0), Fx0z and F0yz faces

  const TopTools_ListOfShape& f000List = vfMap.FindFromKey( V000 );
  const TopTools_ListOfShape& f001List = vfMap.FindFromKey( V001 );
  if (f000List.Extent() != NB_FACES_BY_VERTEX ||
      f001List.Extent() != NB_FACES_BY_VERTEX ) {
    MESSAGE(" LoadBlockShapes() " << f000List.Extent() << " " << f001List.Extent());
    return false;
  }
  TopTools_ListIteratorOfListOfShape f001It, f000It ( f000List );
  int i, j, iFound1, iFound2;
  for ( j = 0; f000It.More(); f000It.Next(), j++ )
  {
    if ( NB_FACES_BY_VERTEX == 6 && j % 2 ) continue; // each face encounters twice
    const TopoDS_Shape& F = f000It.Value();
    for ( i = 0, f001It.Initialize( f001List ); f001It.More(); f001It.Next(), i++ ) {
      if ( NB_FACES_BY_VERTEX == 6 && i % 2 ) continue; // each face encounters twice
      if ( F.IsSame( f001It.Value() ))
        break;
    }
    if ( f001It.More() ) // Fx0z or F0yz found
      if ( Fx0z.IsNull() ) {
        Fx0z = F;
        iFound1 = i;
      } else {
        F0yz = F;
        iFound2 = i;
      }
    else // F is the bottom face
      Fxy0 = F;
  }
  if ( Fxy0.IsNull() || Fx0z.IsNull() || F0yz.IsNull() ) {
    MESSAGE( Fxy0.IsNull() <<" "<< Fx0z.IsNull() <<" "<< F0yz.IsNull() );
    return false;
  }

  // choose the top face (Fxy1)
  for ( i = 0, f001It.Initialize( f001List ); f001It.More(); f001It.Next(), i++ ) {
    if ( NB_FACES_BY_VERTEX == 6 && i % 2 ) continue; // each face encounters twice
    if ( i != iFound1 && i != iFound2 )
      break;
  }
  Fxy1 = f001It.Value();
  if ( Fxy1.IsNull() ) {
    MESSAGE(" LoadBlockShapes() error ");
    return false;
  }

  // find bottom edges and veritices
  list< TopoDS_Edge > eList;
  list< int >         nbVertexInWires;
  GetOrderedEdges( TopoDS::Face( Fxy0 ), TopoDS::Vertex( V000 ), eList, nbVertexInWires );
  if ( nbVertexInWires.size() != 1 || nbVertexInWires.front() != 4 ) {
    MESSAGE(" LoadBlockShapes() error ");
    return false;
  }
  list< TopoDS_Edge >::iterator elIt = eList.begin();
  for ( i = 0; elIt != eList.end(); elIt++, i++ )
    switch ( i ) {
    case 0: E0y0 = *elIt; V010 = TopExp::LastVertex( *elIt, true ); break;
    case 1: Ex10 = *elIt; V110 = TopExp::LastVertex( *elIt, true ); break;
    case 2: E1y0 = *elIt; V100 = TopExp::LastVertex( *elIt, true ); break;
    case 3: Ex00 = *elIt; break;
    default:;
    }
  if ( i != 4 || E0y0.IsNull() || Ex10.IsNull() || E1y0.IsNull() || Ex00.IsNull() ) {
    MESSAGE(" LoadBlockShapes() error, eList.size()=" << eList.size());
    return false;
  }


  // find top edges and veritices
  eList.clear();
  GetOrderedEdges( TopoDS::Face( Fxy1 ), TopoDS::Vertex( V001 ), eList, nbVertexInWires );
  if ( nbVertexInWires.size() != 1 || nbVertexInWires.front() != 4 ) {
    MESSAGE(" LoadBlockShapes() error ");
    return false;
  }
  for ( i = 0, elIt = eList.begin(); elIt != eList.end(); elIt++, i++ )
    switch ( i ) {
    case 0: Ex01 = *elIt; V101 = TopExp::LastVertex( *elIt, true ); break;
    case 1: E1y1 = *elIt; V111 = TopExp::LastVertex( *elIt, true ); break;
    case 2: Ex11 = *elIt; V011 = TopExp::LastVertex( *elIt, true ); break;
    case 3: E0y1 = *elIt; break;
    default:;
    }
  if ( i != 4 || Ex01.IsNull() || E1y1.IsNull() || Ex11.IsNull() || E0y1.IsNull() ) {
    MESSAGE(" LoadBlockShapes() error, eList.size()=" << eList.size());
    return false;
  }

  // swap Fx0z and F0yz if necessary
  TopExp_Explorer exp( Fx0z, TopAbs_VERTEX );
  for ( ; exp.More(); exp.Next() ) // Fx0z shares V101 and V100
    if ( V101.IsSame( exp.Current() ) || V100.IsSame( exp.Current() ))
      break; // V101 or V100 found
  if ( !exp.More() ) { // not found
    std::swap( Fx0z, F0yz);
  }

  // find Fx1z and F1yz faces
  const TopTools_ListOfShape& f111List = vfMap.FindFromKey( V111 );
  const TopTools_ListOfShape& f110List = vfMap.FindFromKey( V110 );
  if (f111List.Extent() != NB_FACES_BY_VERTEX ||
      f110List.Extent() != NB_FACES_BY_VERTEX ) {
    MESSAGE(" LoadBlockShapes() " << f111List.Extent() << " " << f110List.Extent());
    return false;
  }
  TopTools_ListIteratorOfListOfShape f111It, f110It ( f110List);
  for ( j = 0 ; f110It.More(); f110It.Next(), j++ ) {
    if ( NB_FACES_BY_VERTEX == 6 && j % 2 ) continue; // each face encounters twice
    const TopoDS_Shape& F = f110It.Value();
    for ( i = 0, f111It.Initialize( f111List ); f111It.More(); f111It.Next(), i++ ) {
      if ( NB_FACES_BY_VERTEX == 6 && i % 2 ) continue; // each face encounters twice
      if ( F.IsSame( f111It.Value() )) { // Fx1z or F1yz found
        if ( Fx1z.IsNull() )
          Fx1z = F;
        else
          F1yz = F;
      }
    }
  }
  if ( Fx1z.IsNull() || F1yz.IsNull() ) {
    MESSAGE(" LoadBlockShapes() error ");
    return false;
  }

  // swap Fx1z and F1yz if necessary
  for ( exp.Init( Fx1z, TopAbs_VERTEX ); exp.More(); exp.Next() )
    if ( V010.IsSame( exp.Current() ) || V011.IsSame( exp.Current() ))
      break;
  if ( !exp.More() ) {
    std::swap( Fx1z, F1yz);
  }

  // find vertical edges
  for ( exp.Init( Fx0z, TopAbs_EDGE ); exp.More(); exp.Next() ) {
    const TopoDS_Edge& edge = TopoDS::Edge( exp.Current() );
    const TopoDS_Shape& vFirst = TopExp::FirstVertex( edge, true );
    if ( vFirst.IsSame( V001 ))
      E00z = edge;
    else if ( vFirst.IsSame( V100 ))
      E10z = edge;
  }
  if ( E00z.IsNull() || E10z.IsNull() ) {
    MESSAGE(" LoadBlockShapes() error ");
    return false;
  }
  for ( exp.Init( Fx1z, TopAbs_EDGE ); exp.More(); exp.Next() ) {
    const TopoDS_Edge& edge = TopoDS::Edge( exp.Current() );
    const TopoDS_Shape& vFirst = TopExp::FirstVertex( edge, true );
    if ( vFirst.IsSame( V111 ))
      E11z = edge;
    else if ( vFirst.IsSame( V010 ))
      E01z = edge;
  }
  if ( E01z.IsNull() || E11z.IsNull() ) {
    MESSAGE(" LoadBlockShapes() error ");
    return false;
  }

  // load shapes in theShapeIDMap

  theShapeIDMap.Clear();
  
  theShapeIDMap.Add(V000.Oriented( TopAbs_FORWARD ));
  theShapeIDMap.Add(V100.Oriented( TopAbs_FORWARD ));
  theShapeIDMap.Add(V010.Oriented( TopAbs_FORWARD ));
  theShapeIDMap.Add(V110.Oriented( TopAbs_FORWARD ));
  theShapeIDMap.Add(V001.Oriented( TopAbs_FORWARD ));
  theShapeIDMap.Add(V101.Oriented( TopAbs_FORWARD ));
  theShapeIDMap.Add(V011.Oriented( TopAbs_FORWARD ));
  theShapeIDMap.Add(V111.Oriented( TopAbs_FORWARD ));

  theShapeIDMap.Add(Ex00);
  theShapeIDMap.Add(Ex10);
  theShapeIDMap.Add(Ex01);
  theShapeIDMap.Add(Ex11);

  theShapeIDMap.Add(E0y0);
  theShapeIDMap.Add(E1y0);
  theShapeIDMap.Add(E0y1);
  theShapeIDMap.Add(E1y1);

  theShapeIDMap.Add(E00z);
  theShapeIDMap.Add(E10z);
  theShapeIDMap.Add(E01z);
  theShapeIDMap.Add(E11z);

  theShapeIDMap.Add(Fxy0);
  theShapeIDMap.Add(Fxy1);
  theShapeIDMap.Add(Fx0z);
  theShapeIDMap.Add(Fx1z);
  theShapeIDMap.Add(F0yz);
  theShapeIDMap.Add(F1yz);
  
  theShapeIDMap.Add(theShell);

  return true;
}

//================================================================================
/*!
 * \brief Initialize block geometry with shapes from theShapeIDMap
  * \param theShapeIDMap - map of block subshapes
  * \retval bool - is a success
 */
//================================================================================

bool SMESH_Block::LoadBlockShapes(const TopTools_IndexedMapOfOrientedShape& theShapeIDMap)
{
  init();

  // store shapes geometry
  for ( int shapeID = 1; shapeID < theShapeIDMap.Extent(); shapeID++ )
  {
    const TopoDS_Shape& S = theShapeIDMap( shapeID );
    switch ( S.ShapeType() )
    {
    case TopAbs_VERTEX: {

      if ( !IsVertexID( ID_V111 )) return false;
      myPnt[ shapeID - ID_V000 ] = BRep_Tool::Pnt( TopoDS::Vertex( S )).XYZ();
      break;
    }
    case TopAbs_EDGE: {

      if ( !IsEdgeID( shapeID )) return false;
      const TopoDS_Edge& edge = TopoDS::Edge( S );
      TEdge& tEdge = myEdge[ shapeID - ID_FirstE ];
      tEdge.Set( shapeID,
                 new BRepAdaptor_Curve( edge ),
                 IsForwardEdge( edge, theShapeIDMap ));
      break;
    }
    case TopAbs_FACE: {

      if ( !LoadFace( TopoDS::Face( S ), shapeID, theShapeIDMap ))
        return false;
      break;
    }
    default: break;
    }
  } // loop on shapes in theShapeIDMap

  return true;
}

//================================================================================
/*!
 * \brief Load face geometry
  * \param theFace - face
  * \param theFaceID - face in-block ID
  * \param theShapeIDMap - map of block subshapes
  * \retval bool - is a success
 * 
 * It is enough to compute params or coordinates on the face.
 * Face subshapes must be loaded into theShapeIDMap before
 */
//================================================================================

bool SMESH_Block::LoadFace(const TopoDS_Face& theFace,
                           const int          theFaceID,
                           const TopTools_IndexedMapOfOrientedShape& theShapeIDMap)
{
  if ( !IsFaceID( theFaceID ) ) return false;
  // pcurves
  Adaptor2d_Curve2d* c2d[4];
  bool isForward[4];
  vector< int > edgeIdVec;
  GetFaceEdgesIDs( theFaceID, edgeIdVec );
  for ( int iE = 0; iE < edgeIdVec.size(); iE++ ) // loop on 4 edges
  {
    if ( edgeIdVec[ iE ] > theShapeIDMap.Extent() )
      return false;
    const TopoDS_Edge& edge = TopoDS::Edge( theShapeIDMap( edgeIdVec[ iE ]));
    c2d[ iE ] = new BRepAdaptor_Curve2d( edge, theFace );
    isForward[ iE ] = IsForwardEdge( edge, theShapeIDMap );
  }
  TFace& tFace = myFace[ theFaceID - ID_FirstF ];
  tFace.Set( theFaceID, new BRepAdaptor_Surface( theFace ), c2d, isForward );
  return true;
}

//================================================================================
/*!
 * \brief/ Insert theShape into theShapeIDMap with theShapeID
  * \param theShape - shape to insert
  * \param theShapeID - shape in-block ID
  * \param theShapeIDMap - map of block subshapes
 */
//================================================================================

bool SMESH_Block::Insert(const TopoDS_Shape& theShape,
                         const int           theShapeID,
                         TopTools_IndexedMapOfOrientedShape& theShapeIDMap)
{
  if ( !theShape.IsNull() && theShapeID > 0 )
  {
    if ( theShapeIDMap.Contains( theShape ))
      return ( theShapeIDMap.FindIndex( theShape ) == theShapeID );

    if ( theShapeID <= theShapeIDMap.Extent() ) {
        theShapeIDMap.Substitute( theShapeID, theShape );
    }
    else {
      while ( theShapeIDMap.Extent() < theShapeID - 1 ) {
        TopoDS_Compound comp;
        BRep_Builder().MakeCompound( comp );
        theShapeIDMap.Add( comp );
      }
      theShapeIDMap.Add( theShape );
    }
    return true;
  }
  return false;
}

//=======================================================================
//function : GetFaceEdgesIDs
//purpose  : return edges IDs in the order u0, u1, 0v, 1v
//           u0 means "|| u, v == 0"
//=======================================================================

void SMESH_Block::GetFaceEdgesIDs (const int faceID, vector< int >& edgeVec )
{
  edgeVec.resize( 4 );
  switch ( faceID ) {
  case ID_Fxy0:
    edgeVec[ 0 ] = ID_Ex00;
    edgeVec[ 1 ] = ID_Ex10;
    edgeVec[ 2 ] = ID_E0y0;
    edgeVec[ 3 ] = ID_E1y0;
    break;
  case ID_Fxy1:
    edgeVec[ 0 ] = ID_Ex01;
    edgeVec[ 1 ] = ID_Ex11;
    edgeVec[ 2 ] = ID_E0y1;
    edgeVec[ 3 ] = ID_E1y1;
    break;
  case ID_Fx0z:
    edgeVec[ 0 ] = ID_Ex00;
    edgeVec[ 1 ] = ID_Ex01;
    edgeVec[ 2 ] = ID_E00z;
    edgeVec[ 3 ] = ID_E10z;
    break;
  case ID_Fx1z:
    edgeVec[ 0 ] = ID_Ex10;
    edgeVec[ 1 ] = ID_Ex11;
    edgeVec[ 2 ] = ID_E01z;
    edgeVec[ 3 ] = ID_E11z;
    break;
  case ID_F0yz:
    edgeVec[ 0 ] = ID_E0y0;
    edgeVec[ 1 ] = ID_E0y1;
    edgeVec[ 2 ] = ID_E00z;
    edgeVec[ 3 ] = ID_E01z;
    break;
  case ID_F1yz:
    edgeVec[ 0 ] = ID_E1y0;
    edgeVec[ 1 ] = ID_E1y1;
    edgeVec[ 2 ] = ID_E10z;
    edgeVec[ 3 ] = ID_E11z;
    break;
  default:
    MESSAGE(" GetFaceEdgesIDs(), wrong face ID: " << faceID );
  }
}

//=======================================================================
//function : GetEdgeVertexIDs
//purpose  : return vertex IDs of an edge
//=======================================================================

void SMESH_Block::GetEdgeVertexIDs (const int edgeID, vector< int >& vertexVec )
{
  vertexVec.resize( 2 );
  switch ( edgeID ) {

  case ID_Ex00:
    vertexVec[ 0 ] = ID_V000;
    vertexVec[ 1 ] = ID_V100;
    break;
  case ID_Ex10:
    vertexVec[ 0 ] = ID_V010;
    vertexVec[ 1 ] = ID_V110;
    break;
  case ID_Ex01:
    vertexVec[ 0 ] = ID_V001;
    vertexVec[ 1 ] = ID_V101;
    break;
  case ID_Ex11:
    vertexVec[ 0 ] = ID_V011;
    vertexVec[ 1 ] = ID_V111;
    break;

  case ID_E0y0:
    vertexVec[ 0 ] = ID_V000;
    vertexVec[ 1 ] = ID_V010;
    break;
  case ID_E1y0:
    vertexVec[ 0 ] = ID_V100;
    vertexVec[ 1 ] = ID_V110;
    break;
  case ID_E0y1:
    vertexVec[ 0 ] = ID_V001;
    vertexVec[ 1 ] = ID_V011;
    break;
  case ID_E1y1:
    vertexVec[ 0 ] = ID_V101;
    vertexVec[ 1 ] = ID_V111;
    break;

  case ID_E00z:
    vertexVec[ 0 ] = ID_V000;
    vertexVec[ 1 ] = ID_V001;
    break;
  case ID_E10z:
    vertexVec[ 0 ] = ID_V100;
    vertexVec[ 1 ] = ID_V101;
    break;
  case ID_E01z:
    vertexVec[ 0 ] = ID_V010;
    vertexVec[ 1 ] = ID_V011;
    break;
  case ID_E11z:
    vertexVec[ 0 ] = ID_V110;
    vertexVec[ 1 ] = ID_V111;
    break;
  default:
    vertexVec.resize(0);
    MESSAGE(" GetEdgeVertexIDs(), wrong edge ID: " << edgeID );
  }
}
