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
#ifdef _MSC_VER
#define _USE_MATH_DEFINES
#endif // _MSC_VER
#include <cmath>

#include "SMESH_ControlsDef.hxx"

#include <set>

#include <BRepAdaptor_Surface.hxx>
#include <BRepClass_FaceClassifier.hxx>
#include <BRep_Tool.hxx>

#include <TopAbs.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Edge.hxx>
#include <TopoDS_Face.hxx>
#include <TopoDS_Shape.hxx>
#include <TopoDS_Vertex.hxx>
#include <TopoDS_Iterator.hxx>

#include <Geom_CylindricalSurface.hxx>
#include <Geom_Plane.hxx>
#include <Geom_Surface.hxx>

#include <Precision.hxx>
#include <TColStd_MapIteratorOfMapOfInteger.hxx>
#include <TColStd_MapOfInteger.hxx>
#include <TColStd_SequenceOfAsciiString.hxx>
#include <TColgp_Array1OfXYZ.hxx>

#include <gp_Ax3.hxx>
#include <gp_Cylinder.hxx>
#include <gp_Dir.hxx>
#include <gp_Pln.hxx>
#include <gp_Pnt.hxx>
#include <gp_Vec.hxx>
#include <gp_XYZ.hxx>

#include "SMDS_Mesh.hxx"
#include "SMDS_Iterator.hxx"
#include "SMDS_MeshElement.hxx"
#include "SMDS_MeshNode.hxx"
#include "SMDS_VolumeTool.hxx"
#include "SMDS_QuadraticFaceOfNodes.hxx"
#include "SMDS_QuadraticEdge.hxx"

#include "SMESHDS_Mesh.hxx"
#include "SMESHDS_GroupBase.hxx"

#ifndef PI
#define PI M_PI
#endif

/*
                            AUXILIARY METHODS
*/

namespace{
  inline double getAngle( const gp_XYZ& P1, const gp_XYZ& P2, const gp_XYZ& P3 )
  {
    gp_Vec v1( P1 - P2 ), v2( P3 - P2 );

    return v1.Magnitude() < gp::Resolution() ||
      v2.Magnitude() < gp::Resolution() ? 0 : v1.Angle( v2 );
  }

  inline double getArea( const gp_XYZ& P1, const gp_XYZ& P2, const gp_XYZ& P3 )
  {
    gp_Vec aVec1( P2 - P1 );
    gp_Vec aVec2( P3 - P1 );
    return ( aVec1 ^ aVec2 ).Magnitude() * 0.5;
  }

  inline double getArea( const gp_Pnt& P1, const gp_Pnt& P2, const gp_Pnt& P3 )
  {
    return getArea( P1.XYZ(), P2.XYZ(), P3.XYZ() );
  }



  inline double getDistance( const gp_XYZ& P1, const gp_XYZ& P2 )
  {
    double aDist = gp_Pnt( P1 ).Distance( gp_Pnt( P2 ) );
    return aDist;
  }

  int getNbMultiConnection( const SMDS_Mesh* theMesh, const int theId )
  {
    if ( theMesh == 0 )
      return 0;

    const SMDS_MeshElement* anEdge = theMesh->FindElement( theId );
    if ( anEdge == 0 || anEdge->GetType() != SMDSAbs_Edge/* || anEdge->NbNodes() != 2 */)
      return 0;

    // for each pair of nodes in anEdge (there are 2 pairs in a quadratic edge)
    // count elements containing both nodes of the pair.
    // Note that there may be such cases for a quadratic edge (a horizontal line):
    //
    //  Case 1          Case 2
    //  |     |      |        |      |
    //  |     |      |        |      |
    //  +-----+------+  +-----+------+ 
    //  |            |  |            |
    //  |            |  |            |
    // result sould be 2 in both cases
    //
    int aResult0 = 0, aResult1 = 0;
     // last node, it is a medium one in a quadratic edge
    const SMDS_MeshNode* aLastNode = anEdge->GetNode( anEdge->NbNodes() - 1 );
    const SMDS_MeshNode* aNode0 = anEdge->GetNode( 0 );
    const SMDS_MeshNode* aNode1 = anEdge->GetNode( 1 );
    if ( aNode1 == aLastNode ) aNode1 = 0;

    SMDS_ElemIteratorPtr anElemIter = aLastNode->GetInverseElementIterator();
    while( anElemIter->more() ) {
      const SMDS_MeshElement* anElem = anElemIter->next();
      if ( anElem != 0 && anElem->GetType() != SMDSAbs_Edge ) {
        SMDS_ElemIteratorPtr anIter = anElem->nodesIterator();
        while ( anIter->more() ) {
          if ( const SMDS_MeshElement* anElemNode = anIter->next() ) {
            if ( anElemNode == aNode0 ) {
              aResult0++;
              if ( !aNode1 ) break; // not a quadratic edge
            }
            else if ( anElemNode == aNode1 )
              aResult1++;
          }
        }
      }
    }
    int aResult = std::max ( aResult0, aResult1 );

//     TColStd_MapOfInteger aMap;

//     SMDS_ElemIteratorPtr anIter = anEdge->nodesIterator();
//     if ( anIter != 0 ) {
//       while( anIter->more() ) {
// 	const SMDS_MeshNode* aNode = (SMDS_MeshNode*)anIter->next();
// 	if ( aNode == 0 )
// 	  return 0;
// 	SMDS_ElemIteratorPtr anElemIter = aNode->GetInverseElementIterator();
// 	while( anElemIter->more() ) {
// 	  const SMDS_MeshElement* anElem = anElemIter->next();
// 	  if ( anElem != 0 && anElem->GetType() != SMDSAbs_Edge ) {
// 	    int anId = anElem->GetID();

// 	    if ( anIter->more() )              // i.e. first node
// 	      aMap.Add( anId );
// 	    else if ( aMap.Contains( anId ) )
// 	      aResult++;
// 	  }
// 	}
//       }
//     }

    return aResult;
  }

}



using namespace SMESH::Controls;

/*
                                FUNCTORS
*/

/*
  Class       : NumericalFunctor
  Description : Base class for numerical functors
*/
NumericalFunctor::NumericalFunctor():
  myMesh(NULL)
{
  myPrecision = -1;
}

void NumericalFunctor::SetMesh( const SMDS_Mesh* theMesh )
{
  myMesh = theMesh;
}

bool NumericalFunctor::GetPoints(const int theId,
                                 TSequenceOfXYZ& theRes ) const
{
  theRes.clear();

  if ( myMesh == 0 )
    return false;

  return GetPoints( myMesh->FindElement( theId ), theRes );
}

bool NumericalFunctor::GetPoints(const SMDS_MeshElement* anElem,
                                 TSequenceOfXYZ&         theRes )
{
  theRes.clear();

  if ( anElem == 0)
    return false;

  theRes.reserve( anElem->NbNodes() );

  // Get nodes of the element
  SMDS_ElemIteratorPtr anIter;

  if ( anElem->IsQuadratic() ) {
    switch ( anElem->GetType() ) {
    case SMDSAbs_Edge:
      anIter = static_cast<const SMDS_QuadraticEdge*>
        (anElem)->interlacedNodesElemIterator();
      break;
    case SMDSAbs_Face:
      anIter = static_cast<const SMDS_QuadraticFaceOfNodes*>
        (anElem)->interlacedNodesElemIterator();
      break;
    default:
      anIter = anElem->nodesIterator();
      //return false;
    }
  }
  else {
    anIter = anElem->nodesIterator();
  }

  if ( anIter ) {
    while( anIter->more() ) {
      if ( const SMDS_MeshNode* aNode = static_cast<const SMDS_MeshNode*>( anIter->next() ))
        theRes.push_back( gp_XYZ( aNode->X(), aNode->Y(), aNode->Z() ) );
    }
  }

  return true;
}

long  NumericalFunctor::GetPrecision() const
{
  return myPrecision;
}

void  NumericalFunctor::SetPrecision( const long thePrecision )
{
  myPrecision = thePrecision;
}

double NumericalFunctor::GetValue( long theId )
{
  myCurrElement = myMesh->FindElement( theId );
  TSequenceOfXYZ P;
  if ( GetPoints( theId, P ))
  {
    double aVal = GetValue( P );
    if ( myPrecision >= 0 )
    {
      double prec = pow( 10., (double)( myPrecision ) );
      aVal = floor( aVal * prec + 0.5 ) / prec;
    }
    return aVal;
  }

  return 0.;
}

//=======================================================================
//function : GetValue
//purpose  : 
//=======================================================================

double Volume::GetValue( long theElementId )
{
  if ( theElementId && myMesh ) {
    SMDS_VolumeTool aVolumeTool;
    if ( aVolumeTool.Set( myMesh->FindElement( theElementId )))
      return aVolumeTool.GetSize();
  }
  return 0;
}

//=======================================================================
//function : GetBadRate
//purpose  : meaningless as it is not quality control functor
//=======================================================================

double Volume::GetBadRate( double Value, int /*nbNodes*/ ) const
{
  return Value;
}

//=======================================================================
//function : GetType
//purpose  : 
//=======================================================================

SMDSAbs_ElementType Volume::GetType() const
{
  return SMDSAbs_Volume;
}


/*
  Class       : MinimumAngle
  Description : Functor for calculation of minimum angle
*/

double MinimumAngle::GetValue( const TSequenceOfXYZ& P )
{
  double aMin;

  if (P.size() <3)
    return 0.;

  aMin = getAngle(P( P.size() ), P( 1 ), P( 2 ));
  aMin = Min(aMin,getAngle(P( P.size()-1 ), P( P.size() ), P( 1 )));

  for (int i=2; i<P.size();i++){
      double A0 = getAngle( P( i-1 ), P( i ), P( i+1 ) );
    aMin = Min(aMin,A0);
  }

  return aMin * 180.0 / PI;
}

double MinimumAngle::GetBadRate( double Value, int nbNodes ) const
{
  //const double aBestAngle = PI / nbNodes;
  const double aBestAngle = 180.0 - ( 360.0 / double(nbNodes) );
  return ( fabs( aBestAngle - Value ));
}

SMDSAbs_ElementType MinimumAngle::GetType() const
{
  return SMDSAbs_Face;
}


/*
  Class       : AspectRatio
  Description : Functor for calculating aspect ratio
*/
double AspectRatio::GetValue( const TSequenceOfXYZ& P )
{
  // According to "Mesh quality control" by Nadir Bouhamau referring to
  // Pascal Jean Frey and Paul-Louis George. Maillages, applications aux elements finis.
  // Hermes Science publications, Paris 1999 ISBN 2-7462-0024-4
  // PAL10872

  int nbNodes = P.size();

  if ( nbNodes < 3 )
    return 0;

  // Compute aspect ratio

  if ( nbNodes == 3 ) {
    // Compute lengths of the sides
    std::vector< double > aLen (nbNodes);
    for ( int i = 0; i < nbNodes - 1; i++ )
      aLen[ i ] = getDistance( P( i + 1 ), P( i + 2 ) );
    aLen[ nbNodes - 1 ] = getDistance( P( 1 ), P( nbNodes ) );
    // Q = alfa * h * p / S, where
    //
    // alfa = sqrt( 3 ) / 6
    // h - length of the longest edge
    // p - half perimeter
    // S - triangle surface
    const double alfa = sqrt( 3. ) / 6.;
    double maxLen = Max( aLen[ 0 ], Max( aLen[ 1 ], aLen[ 2 ] ) );
    double half_perimeter = ( aLen[0] + aLen[1] + aLen[2] ) / 2.;
    double anArea = getArea( P( 1 ), P( 2 ), P( 3 ) );
    if ( anArea <= Precision::Confusion() )
      return 0.;
    return alfa * maxLen * half_perimeter / anArea;
  }
  else if ( nbNodes == 6 ) { // quadratic triangles
    // Compute lengths of the sides
    std::vector< double > aLen (3);
    aLen[0] = getDistance( P(1), P(3) );
    aLen[1] = getDistance( P(3), P(5) );
    aLen[2] = getDistance( P(5), P(1) );
    // Q = alfa * h * p / S, where
    //
    // alfa = sqrt( 3 ) / 6
    // h - length of the longest edge
    // p - half perimeter
    // S - triangle surface
    const double alfa = sqrt( 3. ) / 6.;
    double maxLen = Max( aLen[ 0 ], Max( aLen[ 1 ], aLen[ 2 ] ) );
    double half_perimeter = ( aLen[0] + aLen[1] + aLen[2] ) / 2.;
    double anArea = getArea( P(1), P(3), P(5) );
    if ( anArea <= Precision::Confusion() )
      return 0.;
    return alfa * maxLen * half_perimeter / anArea;
  }
  else if( nbNodes == 4 ) { // quadrangle
    // return aspect ratio of the worst triange which can be built
    // taking three nodes of the quadrangle
    TSequenceOfXYZ triaPnts(3);
    // triangle on nodes 1 3 2
    triaPnts(1) = P(1);
    triaPnts(2) = P(3);
    triaPnts(3) = P(2);
    double ar = GetValue( triaPnts );
    // triangle on nodes 1 3 4
    triaPnts(3) = P(4);
    ar = Max ( ar, GetValue( triaPnts ));
    // triangle on nodes 1 2 4
    triaPnts(2) = P(2);
    ar = Max ( ar, GetValue( triaPnts ));
    // triangle on nodes 3 2 4
    triaPnts(1) = P(3);
    ar = Max ( ar, GetValue( triaPnts ));

    return ar;
  }
  else { // nbNodes==8 - quadratic quadrangle
    // return aspect ratio of the worst triange which can be built
    // taking three nodes of the quadrangle
    TSequenceOfXYZ triaPnts(3);
    // triangle on nodes 1 3 2
    triaPnts(1) = P(1);
    triaPnts(2) = P(5);
    triaPnts(3) = P(3);
    double ar = GetValue( triaPnts );
    // triangle on nodes 1 3 4
    triaPnts(3) = P(7);
    ar = Max ( ar, GetValue( triaPnts ));
    // triangle on nodes 1 2 4
    triaPnts(2) = P(3);
    ar = Max ( ar, GetValue( triaPnts ));
    // triangle on nodes 3 2 4
    triaPnts(1) = P(5);
    ar = Max ( ar, GetValue( triaPnts ));

    return ar;
  }
}

double AspectRatio::GetBadRate( double Value, int /*nbNodes*/ ) const
{
  // the aspect ratio is in the range [1.0,infinity]
  // 1.0 = good
  // infinity = bad
  return Value / 1000.;
}

SMDSAbs_ElementType AspectRatio::GetType() const
{
  return SMDSAbs_Face;
}


/*
  Class       : AspectRatio3D
  Description : Functor for calculating aspect ratio
*/
namespace{

  inline double getHalfPerimeter(double theTria[3]){
    return (theTria[0] + theTria[1] + theTria[2])/2.0;
  }

  inline double getArea(double theHalfPerim, double theTria[3]){
    return sqrt(theHalfPerim*
		(theHalfPerim-theTria[0])*
		(theHalfPerim-theTria[1])*
		(theHalfPerim-theTria[2]));
  }

  inline double getVolume(double theLen[6]){
    double a2 = theLen[0]*theLen[0];
    double b2 = theLen[1]*theLen[1];
    double c2 = theLen[2]*theLen[2];
    double d2 = theLen[3]*theLen[3];
    double e2 = theLen[4]*theLen[4];
    double f2 = theLen[5]*theLen[5];
    double P = 4.0*a2*b2*d2;
    double Q = a2*(b2+d2-e2)-b2*(a2+d2-f2)-d2*(a2+b2-c2);
    double R = (b2+d2-e2)*(a2+d2-f2)*(a2+d2-f2);
    return sqrt(P-Q+R)/12.0;
  }

  inline double getVolume2(double theLen[6]){
    double a2 = theLen[0]*theLen[0];
    double b2 = theLen[1]*theLen[1];
    double c2 = theLen[2]*theLen[2];
    double d2 = theLen[3]*theLen[3];
    double e2 = theLen[4]*theLen[4];
    double f2 = theLen[5]*theLen[5];

    double P = a2*e2*(b2+c2+d2+f2-a2-e2);
    double Q = b2*f2*(a2+c2+d2+e2-b2-f2);
    double R = c2*d2*(a2+b2+e2+f2-c2-d2);
    double S = a2*b2*d2+b2*c2*e2+a2*c2*f2+d2*e2*f2;

    return sqrt(P+Q+R-S)/12.0;
  }

  inline double getVolume(const TSequenceOfXYZ& P){
    gp_Vec aVec1( P( 2 ) - P( 1 ) );
    gp_Vec aVec2( P( 3 ) - P( 1 ) );
    gp_Vec aVec3( P( 4 ) - P( 1 ) );
    gp_Vec anAreaVec( aVec1 ^ aVec2 );
    return fabs(aVec3 * anAreaVec) / 6.0;
  }

  inline double getMaxHeight(double theLen[6])
  {
    double aHeight = std::max(theLen[0],theLen[1]);
    aHeight = std::max(aHeight,theLen[2]);
    aHeight = std::max(aHeight,theLen[3]);
    aHeight = std::max(aHeight,theLen[4]);
    aHeight = std::max(aHeight,theLen[5]);
    return aHeight;
  }

}

double AspectRatio3D::GetValue( const TSequenceOfXYZ& P )
{
  double aQuality = 0.0;
  if(myCurrElement->IsPoly()) return aQuality;

  int nbNodes = P.size();

  if(myCurrElement->IsQuadratic()) {
    if(nbNodes==10) nbNodes=4; // quadratic tetrahedron
    else if(nbNodes==13) nbNodes=5; // quadratic pyramid
    else if(nbNodes==15) nbNodes=6; // quadratic pentahedron
    else if(nbNodes==20) nbNodes=8; // quadratic hexahedron
    else return aQuality;
  }

  switch(nbNodes){
  case 4:{
    double aLen[6] = {
      getDistance(P( 1 ),P( 2 )), // a
      getDistance(P( 2 ),P( 3 )), // b
      getDistance(P( 3 ),P( 1 )), // c
      getDistance(P( 2 ),P( 4 )), // d
      getDistance(P( 3 ),P( 4 )), // e
      getDistance(P( 1 ),P( 4 ))  // f
    };
    double aTria[4][3] = {
      {aLen[0],aLen[1],aLen[2]}, // abc
      {aLen[0],aLen[3],aLen[5]}, // adf
      {aLen[1],aLen[3],aLen[4]}, // bde
      {aLen[2],aLen[4],aLen[5]}  // cef
    };
    double aSumArea = 0.0;
    double aHalfPerimeter = getHalfPerimeter(aTria[0]);
    double anArea = getArea(aHalfPerimeter,aTria[0]);
    aSumArea += anArea;
    aHalfPerimeter = getHalfPerimeter(aTria[1]);
    anArea = getArea(aHalfPerimeter,aTria[1]);
    aSumArea += anArea;
    aHalfPerimeter = getHalfPerimeter(aTria[2]);
    anArea = getArea(aHalfPerimeter,aTria[2]);
    aSumArea += anArea;
    aHalfPerimeter = getHalfPerimeter(aTria[3]);
    anArea = getArea(aHalfPerimeter,aTria[3]);
    aSumArea += anArea;
    double aVolume = getVolume(P);
    //double aVolume = getVolume(aLen);
    double aHeight = getMaxHeight(aLen);
    static double aCoeff = sqrt(2.0)/12.0;
    if ( aVolume > DBL_MIN )
      aQuality = aCoeff*aHeight*aSumArea/aVolume;
    break;
  }
  case 5:{
    {
      gp_XYZ aXYZ[4] = {P( 1 ),P( 2 ),P( 3 ),P( 5 )};
      aQuality = std::max(GetValue(TSequenceOfXYZ(&aXYZ[0],&aXYZ[4])),aQuality);
    }
    {
      gp_XYZ aXYZ[4] = {P( 1 ),P( 3 ),P( 4 ),P( 5 )};
      aQuality = std::max(GetValue(TSequenceOfXYZ(&aXYZ[0],&aXYZ[4])),aQuality);
    }
    {
      gp_XYZ aXYZ[4] = {P( 1 ),P( 2 ),P( 4 ),P( 5 )};
      aQuality = std::max(GetValue(TSequenceOfXYZ(&aXYZ[0],&aXYZ[4])),aQuality);
    }
    {
      gp_XYZ aXYZ[4] = {P( 2 ),P( 3 ),P( 4 ),P( 5 )};
      aQuality = std::max(GetValue(TSequenceOfXYZ(&aXYZ[0],&aXYZ[4])),aQuality);
    }
    break;
  }
  case 6:{
    {
      gp_XYZ aXYZ[4] = {P( 1 ),P( 2 ),P( 4 ),P( 6 )};
      aQuality = std::max(GetValue(TSequenceOfXYZ(&aXYZ[0],&aXYZ[4])),aQuality);
    }
    {
      gp_XYZ aXYZ[4] = {P( 1 ),P( 2 ),P( 4 ),P( 3 )};
      aQuality = std::max(GetValue(TSequenceOfXYZ(&aXYZ[0],&aXYZ[4])),aQuality);
    }
    {
      gp_XYZ aXYZ[4] = {P( 1 ),P( 2 ),P( 5 ),P( 6 )};
      aQuality = std::max(GetValue(TSequenceOfXYZ(&aXYZ[0],&aXYZ[4])),aQuality);
    }
    {
      gp_XYZ aXYZ[4] = {P( 1 ),P( 2 ),P( 5 ),P( 3 )};
      aQuality = std::max(GetValue(TSequenceOfXYZ(&aXYZ[0],&aXYZ[4])),aQuality);
    }
    {
      gp_XYZ aXYZ[4] = {P( 2 ),P( 5 ),P( 4 ),P( 6 )};
      aQuality = std::max(GetValue(TSequenceOfXYZ(&aXYZ[0],&aXYZ[4])),aQuality);
    }
    {
      gp_XYZ aXYZ[4] = {P( 2 ),P( 5 ),P( 4 ),P( 3 )};
      aQuality = std::max(GetValue(TSequenceOfXYZ(&aXYZ[0],&aXYZ[4])),aQuality);
    }
    break;
  }
  case 8:{
    {
      gp_XYZ aXYZ[4] = {P( 1 ),P( 2 ),P( 5 ),P( 3 )};
      aQuality = std::max(GetValue(TSequenceOfXYZ(&aXYZ[0],&aXYZ[4])),aQuality);
    }
    {
      gp_XYZ aXYZ[4] = {P( 1 ),P( 2 ),P( 5 ),P( 4 )};
      aQuality = std::max(GetValue(TSequenceOfXYZ(&aXYZ[0],&aXYZ[4])),aQuality);
    }
    {
      gp_XYZ aXYZ[4] = {P( 1 ),P( 2 ),P( 5 ),P( 7 )};
      aQuality = std::max(GetValue(TSequenceOfXYZ(&aXYZ[0],&aXYZ[4])),aQuality);
    }
    {
      gp_XYZ aXYZ[4] = {P( 1 ),P( 2 ),P( 5 ),P( 8 )};
      aQuality = std::max(GetValue(TSequenceOfXYZ(&aXYZ[0],&aXYZ[4])),aQuality);
    }
    {
      gp_XYZ aXYZ[4] = {P( 1 ),P( 2 ),P( 6 ),P( 3 )};
      aQuality = std::max(GetValue(TSequenceOfXYZ(&aXYZ[0],&aXYZ[4])),aQuality);
    }
    {
      gp_XYZ aXYZ[4] = {P( 1 ),P( 2 ),P( 6 ),P( 4 )};
      aQuality = std::max(GetValue(TSequenceOfXYZ(&aXYZ[0],&aXYZ[4])),aQuality);
    }
    {
      gp_XYZ aXYZ[4] = {P( 1 ),P( 2 ),P( 6 ),P( 7 )};
      aQuality = std::max(GetValue(TSequenceOfXYZ(&aXYZ[0],&aXYZ[4])),aQuality);
    }
    {
      gp_XYZ aXYZ[4] = {P( 1 ),P( 2 ),P( 6 ),P( 8 )};
      aQuality = std::max(GetValue(TSequenceOfXYZ(&aXYZ[0],&aXYZ[4])),aQuality);
    }
    {
      gp_XYZ aXYZ[4] = {P( 2 ),P( 6 ),P( 5 ),P( 3 )};
      aQuality = std::max(GetValue(TSequenceOfXYZ(&aXYZ[0],&aXYZ[4])),aQuality);
    }
    {
      gp_XYZ aXYZ[4] = {P( 2 ),P( 6 ),P( 5 ),P( 4 )};
      aQuality = std::max(GetValue(TSequenceOfXYZ(&aXYZ[0],&aXYZ[4])),aQuality);
    }
    {
      gp_XYZ aXYZ[4] = {P( 2 ),P( 6 ),P( 5 ),P( 7 )};
      aQuality = std::max(GetValue(TSequenceOfXYZ(&aXYZ[0],&aXYZ[4])),aQuality);
    }
    {
      gp_XYZ aXYZ[4] = {P( 2 ),P( 6 ),P( 5 ),P( 8 )};
      aQuality = std::max(GetValue(TSequenceOfXYZ(&aXYZ[0],&aXYZ[4])),aQuality);
    }
    {
      gp_XYZ aXYZ[4] = {P( 3 ),P( 4 ),P( 8 ),P( 1 )};
      aQuality = std::max(GetValue(TSequenceOfXYZ(&aXYZ[0],&aXYZ[4])),aQuality);
    }
    {
      gp_XYZ aXYZ[4] = {P( 3 ),P( 4 ),P( 8 ),P( 2 )};
      aQuality = std::max(GetValue(TSequenceOfXYZ(&aXYZ[0],&aXYZ[4])),aQuality);
    }
    {
      gp_XYZ aXYZ[4] = {P( 3 ),P( 4 ),P( 8 ),P( 5 )};
      aQuality = std::max(GetValue(TSequenceOfXYZ(&aXYZ[0],&aXYZ[4])),aQuality);
    }
    {
      gp_XYZ aXYZ[4] = {P( 3 ),P( 4 ),P( 8 ),P( 6 )};
      aQuality = std::max(GetValue(TSequenceOfXYZ(&aXYZ[0],&aXYZ[4])),aQuality);
    }
    {
      gp_XYZ aXYZ[4] = {P( 3 ),P( 4 ),P( 7 ),P( 1 )};
      aQuality = std::max(GetValue(TSequenceOfXYZ(&aXYZ[0],&aXYZ[4])),aQuality);
    }
    {
      gp_XYZ aXYZ[4] = {P( 3 ),P( 4 ),P( 7 ),P( 2 )};
      aQuality = std::max(GetValue(TSequenceOfXYZ(&aXYZ[0],&aXYZ[4])),aQuality);
    }
    {
      gp_XYZ aXYZ[4] = {P( 3 ),P( 4 ),P( 7 ),P( 5 )};
      aQuality = std::max(GetValue(TSequenceOfXYZ(&aXYZ[0],&aXYZ[4])),aQuality);
    }
    {
      gp_XYZ aXYZ[4] = {P( 3 ),P( 4 ),P( 7 ),P( 6 )};
      aQuality = std::max(GetValue(TSequenceOfXYZ(&aXYZ[0],&aXYZ[4])),aQuality);
    }
    {
      gp_XYZ aXYZ[4] = {P( 4 ),P( 8 ),P( 7 ),P( 1 )};
      aQuality = std::max(GetValue(TSequenceOfXYZ(&aXYZ[0],&aXYZ[4])),aQuality);
    }
    {
      gp_XYZ aXYZ[4] = {P( 4 ),P( 8 ),P( 7 ),P( 2 )};
      aQuality = std::max(GetValue(TSequenceOfXYZ(&aXYZ[0],&aXYZ[4])),aQuality);
    }
    {
      gp_XYZ aXYZ[4] = {P( 4 ),P( 8 ),P( 7 ),P( 5 )};
      aQuality = std::max(GetValue(TSequenceOfXYZ(&aXYZ[0],&aXYZ[4])),aQuality);
    }
    {
      gp_XYZ aXYZ[4] = {P( 4 ),P( 8 ),P( 7 ),P( 6 )};
      aQuality = std::max(GetValue(TSequenceOfXYZ(&aXYZ[0],&aXYZ[4])),aQuality);
    }
    {
      gp_XYZ aXYZ[4] = {P( 4 ),P( 8 ),P( 7 ),P( 2 )};
      aQuality = std::max(GetValue(TSequenceOfXYZ(&aXYZ[0],&aXYZ[4])),aQuality);
    }
    {
      gp_XYZ aXYZ[4] = {P( 4 ),P( 5 ),P( 8 ),P( 2 )};
      aQuality = std::max(GetValue(TSequenceOfXYZ(&aXYZ[0],&aXYZ[4])),aQuality);
    }
    {
      gp_XYZ aXYZ[4] = {P( 1 ),P( 4 ),P( 5 ),P( 3 )};
      aQuality = std::max(GetValue(TSequenceOfXYZ(&aXYZ[0],&aXYZ[4])),aQuality);
    }
    {
      gp_XYZ aXYZ[4] = {P( 3 ),P( 6 ),P( 7 ),P( 1 )};
      aQuality = std::max(GetValue(TSequenceOfXYZ(&aXYZ[0],&aXYZ[4])),aQuality);
    }
    {
      gp_XYZ aXYZ[4] = {P( 2 ),P( 3 ),P( 6 ),P( 4 )};
      aQuality = std::max(GetValue(TSequenceOfXYZ(&aXYZ[0],&aXYZ[4])),aQuality);
    }
    {
      gp_XYZ aXYZ[4] = {P( 5 ),P( 6 ),P( 8 ),P( 3 )};
      aQuality = std::max(GetValue(TSequenceOfXYZ(&aXYZ[0],&aXYZ[4])),aQuality);
    }
    {
      gp_XYZ aXYZ[4] = {P( 7 ),P( 8 ),P( 6 ),P( 1 )};
      aQuality = std::max(GetValue(TSequenceOfXYZ(&aXYZ[0],&aXYZ[4])),aQuality);
    }
    {
      gp_XYZ aXYZ[4] = {P( 1 ),P( 2 ),P( 4 ),P( 7 )};
      aQuality = std::max(GetValue(TSequenceOfXYZ(&aXYZ[0],&aXYZ[4])),aQuality);
    }
    {
      gp_XYZ aXYZ[4] = {P( 3 ),P( 4 ),P( 2 ),P( 5 )};
      aQuality = std::max(GetValue(TSequenceOfXYZ(&aXYZ[0],&aXYZ[4])),aQuality);
    }
    break;
  }
  }
  if ( nbNodes > 4 ) {
    // avaluate aspect ratio of quadranle faces
    AspectRatio aspect2D;
    SMDS_VolumeTool::VolumeType type = SMDS_VolumeTool::GetType( nbNodes );
    int nbFaces = SMDS_VolumeTool::NbFaces( type );
    TSequenceOfXYZ points(4);
    for ( int i = 0; i < nbFaces; ++i ) { // loop on faces of a volume
      if ( SMDS_VolumeTool::NbFaceNodes( type, i ) != 4 )
        continue;
      const int* pInd = SMDS_VolumeTool::GetFaceNodesIndices( type, i, true );
      for ( int p = 0; p < 4; ++p ) // loop on nodes of a quadranle face
        points( p + 1 ) = P( pInd[ p ] + 1 );
      aQuality = std::max( aQuality, aspect2D.GetValue( points ));
    }
  }
  return aQuality;
}

double AspectRatio3D::GetBadRate( double Value, int /*nbNodes*/ ) const
{
  // the aspect ratio is in the range [1.0,infinity]
  // 1.0 = good
  // infinity = bad
  return Value / 1000.;
}

SMDSAbs_ElementType AspectRatio3D::GetType() const
{
  return SMDSAbs_Volume;
}


/*
  Class       : Warping
  Description : Functor for calculating warping
*/
double Warping::GetValue( const TSequenceOfXYZ& P )
{
  if ( P.size() != 4 )
    return 0;

  gp_XYZ G = ( P( 1 ) + P( 2 ) + P( 3 ) + P( 4 ) ) / 4.;

  double A1 = ComputeA( P( 1 ), P( 2 ), P( 3 ), G );
  double A2 = ComputeA( P( 2 ), P( 3 ), P( 4 ), G );
  double A3 = ComputeA( P( 3 ), P( 4 ), P( 1 ), G );
  double A4 = ComputeA( P( 4 ), P( 1 ), P( 2 ), G );

  return Max( Max( A1, A2 ), Max( A3, A4 ) );
}

double Warping::ComputeA( const gp_XYZ& thePnt1,
                          const gp_XYZ& thePnt2,
                          const gp_XYZ& thePnt3,
                          const gp_XYZ& theG ) const
{
  double aLen1 = gp_Pnt( thePnt1 ).Distance( gp_Pnt( thePnt2 ) );
  double aLen2 = gp_Pnt( thePnt2 ).Distance( gp_Pnt( thePnt3 ) );
  double L = Min( aLen1, aLen2 ) * 0.5;
  if ( L < Precision::Confusion())
    return 0.;

  gp_XYZ GI = ( thePnt2 + thePnt1 ) / 2. - theG;
  gp_XYZ GJ = ( thePnt3 + thePnt2 ) / 2. - theG;
  gp_XYZ N  = GI.Crossed( GJ );

  if ( N.Modulus() < gp::Resolution() )
    return PI / 2;

  N.Normalize();

  double H = ( thePnt2 - theG ).Dot( N );
  return asin( fabs( H / L ) ) * 180. / PI;
}

double Warping::GetBadRate( double Value, int /*nbNodes*/ ) const
{
  // the warp is in the range [0.0,PI/2]
  // 0.0 = good (no warp)
  // PI/2 = bad  (face pliee)
  return Value;
}

SMDSAbs_ElementType Warping::GetType() const
{
  return SMDSAbs_Face;
}


/*
  Class       : Taper
  Description : Functor for calculating taper
*/
double Taper::GetValue( const TSequenceOfXYZ& P )
{
  if ( P.size() != 4 )
    return 0.;

  // Compute taper
  double J1 = getArea( P( 4 ), P( 1 ), P( 2 ) ) / 2.;
  double J2 = getArea( P( 3 ), P( 1 ), P( 2 ) ) / 2.;
  double J3 = getArea( P( 2 ), P( 3 ), P( 4 ) ) / 2.;
  double J4 = getArea( P( 3 ), P( 4 ), P( 1 ) ) / 2.;

  double JA = 0.25 * ( J1 + J2 + J3 + J4 );
  if ( JA <= Precision::Confusion() )
    return 0.;

  double T1 = fabs( ( J1 - JA ) / JA );
  double T2 = fabs( ( J2 - JA ) / JA );
  double T3 = fabs( ( J3 - JA ) / JA );
  double T4 = fabs( ( J4 - JA ) / JA );

  return Max( Max( T1, T2 ), Max( T3, T4 ) );
}

double Taper::GetBadRate( double Value, int /*nbNodes*/ ) const
{
  // the taper is in the range [0.0,1.0]
  // 0.0  = good (no taper)
  // 1.0 = bad  (les cotes opposes sont allignes)
  return Value;
}

SMDSAbs_ElementType Taper::GetType() const
{
  return SMDSAbs_Face;
}


/*
  Class       : Skew
  Description : Functor for calculating skew in degrees
*/
static inline double skewAngle( const gp_XYZ& p1, const gp_XYZ& p2, const gp_XYZ& p3 )
{
  gp_XYZ p12 = ( p2 + p1 ) / 2.;
  gp_XYZ p23 = ( p3 + p2 ) / 2.;
  gp_XYZ p31 = ( p3 + p1 ) / 2.;

  gp_Vec v1( p31 - p2 ), v2( p12 - p23 );

  return v1.Magnitude() < gp::Resolution() || v2.Magnitude() < gp::Resolution() ? 0. : v1.Angle( v2 );
}

double Skew::GetValue( const TSequenceOfXYZ& P )
{
  if ( P.size() != 3 && P.size() != 4 )
    return 0.;

  // Compute skew
  static double PI2 = PI / 2.;
  if ( P.size() == 3 )
  {
    double A0 = fabs( PI2 - skewAngle( P( 3 ), P( 1 ), P( 2 ) ) );
    double A1 = fabs( PI2 - skewAngle( P( 1 ), P( 2 ), P( 3 ) ) );
    double A2 = fabs( PI2 - skewAngle( P( 2 ), P( 3 ), P( 1 ) ) );

    return Max( A0, Max( A1, A2 ) ) * 180. / PI;
  }
  else
  {
    gp_XYZ p12 = ( P( 1 ) + P( 2 ) ) / 2.;
    gp_XYZ p23 = ( P( 2 ) + P( 3 ) ) / 2.;
    gp_XYZ p34 = ( P( 3 ) + P( 4 ) ) / 2.;
    gp_XYZ p41 = ( P( 4 ) + P( 1 ) ) / 2.;

    gp_Vec v1( p34 - p12 ), v2( p23 - p41 );
    double A = v1.Magnitude() <= gp::Resolution() || v2.Magnitude() <= gp::Resolution()
      ? 0. : fabs( PI2 - v1.Angle( v2 ) );

    //BUG SWP12743
    if ( A < Precision::Angular() )
      return 0.;

    return A * 180. / PI;
  }
}

double Skew::GetBadRate( double Value, int /*nbNodes*/ ) const
{
  // the skew is in the range [0.0,PI/2].
  // 0.0 = good
  // PI/2 = bad
  return Value;
}

SMDSAbs_ElementType Skew::GetType() const
{
  return SMDSAbs_Face;
}


/*
  Class       : Area
  Description : Functor for calculating area
*/
double Area::GetValue( const TSequenceOfXYZ& P )
{
  gp_Vec aVec1( P(2) - P(1) );
  gp_Vec aVec2( P(3) - P(1) );
  gp_Vec SumVec = aVec1 ^ aVec2;
  for (int i=4; i<=P.size(); i++) {
    gp_Vec aVec1( P(i-1) - P(1) );
    gp_Vec aVec2( P(i) - P(1) );
    gp_Vec tmp = aVec1 ^ aVec2;
    SumVec.Add(tmp);
  }
  return SumVec.Magnitude() * 0.5;
}

double Area::GetBadRate( double Value, int /*nbNodes*/ ) const
{
  // meaningless as it is not a quality control functor
  return Value;
}

SMDSAbs_ElementType Area::GetType() const
{
  return SMDSAbs_Face;
}


/*
  Class       : Length
  Description : Functor for calculating length off edge
*/
double Length::GetValue( const TSequenceOfXYZ& P )
{
  switch ( P.size() ) {
  case 2:  return getDistance( P( 1 ), P( 2 ) );
  case 3:  return getDistance( P( 1 ), P( 2 ) ) + getDistance( P( 2 ), P( 3 ) );
  default: return 0.;
  }
}

double Length::GetBadRate( double Value, int /*nbNodes*/ ) const
{
  // meaningless as it is not quality control functor
  return Value;
}

SMDSAbs_ElementType Length::GetType() const
{
  return SMDSAbs_Edge;
}

/*
  Class       : Length2D
  Description : Functor for calculating length of edge
*/

double Length2D::GetValue( long theElementId)
{
  TSequenceOfXYZ P;

  //cout<<"Length2D::GetValue"<<endl;
  if (GetPoints(theElementId,P)){
    //for(int jj=1; jj<=P.size(); jj++)
    //  cout<<"jj="<<jj<<" P("<<P(jj).X()<<","<<P(jj).Y()<<","<<P(jj).Z()<<")"<<endl;

    double  aVal;// = GetValue( P );
    const SMDS_MeshElement* aElem = myMesh->FindElement( theElementId );
    SMDSAbs_ElementType aType = aElem->GetType();

    int len = P.size();

    switch (aType){
    case SMDSAbs_All:
    case SMDSAbs_Node:
    case SMDSAbs_Edge:
      if (len == 2){
	aVal = getDistance( P( 1 ), P( 2 ) );
        break;
      }
      else if (len == 3){ // quadratic edge
	aVal = getDistance(P( 1 ),P( 3 )) + getDistance(P( 3 ),P( 2 ));
        break;
      }
    case SMDSAbs_Face:
      if (len == 3){ // triangles
	double L1 = getDistance(P( 1 ),P( 2 ));
	double L2 = getDistance(P( 2 ),P( 3 ));
	double L3 = getDistance(P( 3 ),P( 1 ));
	aVal = Max(L1,Max(L2,L3));
	break;
      }
      else if (len == 4){ // quadrangles
	double L1 = getDistance(P( 1 ),P( 2 ));
	double L2 = getDistance(P( 2 ),P( 3 ));
	double L3 = getDistance(P( 3 ),P( 4 ));
	double L4 = getDistance(P( 4 ),P( 1 ));
	aVal = Max(Max(L1,L2),Max(L3,L4));
	break;
      }
      if (len == 6){ // quadratic triangles
	double L1 = getDistance(P( 1 ),P( 2 )) + getDistance(P( 2 ),P( 3 ));
	double L2 = getDistance(P( 3 ),P( 4 )) + getDistance(P( 4 ),P( 5 ));
	double L3 = getDistance(P( 5 ),P( 6 )) + getDistance(P( 6 ),P( 1 ));
	aVal = Max(L1,Max(L2,L3));
        //cout<<"L1="<<L1<<" L2="<<L2<<"L3="<<L3<<" aVal="<<aVal<<endl;
	break;
      }
      else if (len == 8){ // quadratic quadrangles
	double L1 = getDistance(P( 1 ),P( 2 )) + getDistance(P( 2 ),P( 3 ));
	double L2 = getDistance(P( 3 ),P( 4 )) + getDistance(P( 4 ),P( 5 ));
	double L3 = getDistance(P( 5 ),P( 6 )) + getDistance(P( 6 ),P( 7 ));
	double L4 = getDistance(P( 7 ),P( 8 )) + getDistance(P( 8 ),P( 1 ));
	aVal = Max(Max(L1,L2),Max(L3,L4));
	break;
      }
    case SMDSAbs_Volume:
      if (len == 4){ // tetraidrs
	double L1 = getDistance(P( 1 ),P( 2 ));
	double L2 = getDistance(P( 2 ),P( 3 ));
	double L3 = getDistance(P( 3 ),P( 1 ));
	double L4 = getDistance(P( 1 ),P( 4 ));
	double L5 = getDistance(P( 2 ),P( 4 ));
	double L6 = getDistance(P( 3 ),P( 4 ));
	aVal = Max(Max(Max(L1,L2),Max(L3,L4)),Max(L5,L6));
	break;
      }
      else if (len == 5){ // piramids
	double L1 = getDistance(P( 1 ),P( 2 ));
	double L2 = getDistance(P( 2 ),P( 3 ));
	double L3 = getDistance(P( 3 ),P( 1 ));
	double L4 = getDistance(P( 4 ),P( 1 ));
	double L5 = getDistance(P( 1 ),P( 5 ));
	double L6 = getDistance(P( 2 ),P( 5 ));
	double L7 = getDistance(P( 3 ),P( 5 ));
	double L8 = getDistance(P( 4 ),P( 5 ));

	aVal = Max(Max(Max(L1,L2),Max(L3,L4)),Max(L5,L6));
	aVal = Max(aVal,Max(L7,L8));
	break;
      }
      else if (len == 6){ // pentaidres
	double L1 = getDistance(P( 1 ),P( 2 ));
	double L2 = getDistance(P( 2 ),P( 3 ));
	double L3 = getDistance(P( 3 ),P( 1 ));
	double L4 = getDistance(P( 4 ),P( 5 ));
	double L5 = getDistance(P( 5 ),P( 6 ));
	double L6 = getDistance(P( 6 ),P( 4 ));
	double L7 = getDistance(P( 1 ),P( 4 ));
	double L8 = getDistance(P( 2 ),P( 5 ));
	double L9 = getDistance(P( 3 ),P( 6 ));

	aVal = Max(Max(Max(L1,L2),Max(L3,L4)),Max(L5,L6));
	aVal = Max(aVal,Max(Max(L7,L8),L9));
	break;
      }
      else if (len == 8){ // hexaider
	double L1 = getDistance(P( 1 ),P( 2 ));
	double L2 = getDistance(P( 2 ),P( 3 ));
	double L3 = getDistance(P( 3 ),P( 4 ));
	double L4 = getDistance(P( 4 ),P( 1 ));
	double L5 = getDistance(P( 5 ),P( 6 ));
	double L6 = getDistance(P( 6 ),P( 7 ));
	double L7 = getDistance(P( 7 ),P( 8 ));
	double L8 = getDistance(P( 8 ),P( 5 ));
	double L9 = getDistance(P( 1 ),P( 5 ));
	double L10= getDistance(P( 2 ),P( 6 ));
	double L11= getDistance(P( 3 ),P( 7 ));
	double L12= getDistance(P( 4 ),P( 8 ));

	aVal = Max(Max(Max(L1,L2),Max(L3,L4)),Max(L5,L6));
	aVal = Max(aVal,Max(Max(L7,L8),Max(L9,L10)));
	aVal = Max(aVal,Max(L11,L12));
	break;

      }

      if (len == 10){ // quadratic tetraidrs
	double L1 = getDistance(P( 1 ),P( 5 )) + getDistance(P( 5 ),P( 2 ));
	double L2 = getDistance(P( 2 ),P( 6 )) + getDistance(P( 6 ),P( 3 ));
	double L3 = getDistance(P( 3 ),P( 7 )) + getDistance(P( 7 ),P( 1 ));
	double L4 = getDistance(P( 1 ),P( 8 )) + getDistance(P( 8 ),P( 4 ));
	double L5 = getDistance(P( 2 ),P( 9 )) + getDistance(P( 9 ),P( 4 ));
	double L6 = getDistance(P( 3 ),P( 10 )) + getDistance(P( 10 ),P( 4 ));
	aVal = Max(Max(Max(L1,L2),Max(L3,L4)),Max(L5,L6));
	break;
      }
      else if (len == 13){ // quadratic piramids
	double L1 = getDistance(P( 1 ),P( 6 )) + getDistance(P( 6 ),P( 2 ));
	double L2 = getDistance(P( 2 ),P( 7 )) + getDistance(P( 7 ),P( 3 ));
	double L3 = getDistance(P( 3 ),P( 8 )) + getDistance(P( 8 ),P( 1 ));
	double L4 = getDistance(P( 4 ),P( 9 )) + getDistance(P( 9 ),P( 1 ));
	double L5 = getDistance(P( 1 ),P( 10 )) + getDistance(P( 10 ),P( 5 ));
	double L6 = getDistance(P( 2 ),P( 11 )) + getDistance(P( 11 ),P( 5 ));
	double L7 = getDistance(P( 3 ),P( 12 )) + getDistance(P( 12 ),P( 5 ));
	double L8 = getDistance(P( 4 ),P( 13 )) + getDistance(P( 13 ),P( 5 ));
	aVal = Max(Max(Max(L1,L2),Max(L3,L4)),Max(L5,L6));
	aVal = Max(aVal,Max(L7,L8));
	break;
      }
      else if (len == 15){ // quadratic pentaidres
	double L1 = getDistance(P( 1 ),P( 7 )) + getDistance(P( 7 ),P( 2 ));
	double L2 = getDistance(P( 2 ),P( 8 )) + getDistance(P( 8 ),P( 3 ));
	double L3 = getDistance(P( 3 ),P( 9 )) + getDistance(P( 9 ),P( 1 ));
	double L4 = getDistance(P( 4 ),P( 10 )) + getDistance(P( 10 ),P( 5 ));
	double L5 = getDistance(P( 5 ),P( 11 )) + getDistance(P( 11 ),P( 6 ));
	double L6 = getDistance(P( 6 ),P( 12 )) + getDistance(P( 12 ),P( 4 ));
	double L7 = getDistance(P( 1 ),P( 13 )) + getDistance(P( 13 ),P( 4 ));
	double L8 = getDistance(P( 2 ),P( 14 )) + getDistance(P( 14 ),P( 5 ));
	double L9 = getDistance(P( 3 ),P( 15 )) + getDistance(P( 15 ),P( 6 ));
	aVal = Max(Max(Max(L1,L2),Max(L3,L4)),Max(L5,L6));
	aVal = Max(aVal,Max(Max(L7,L8),L9));
	break;
      }
      else if (len == 20){ // quadratic hexaider
	double L1 = getDistance(P( 1 ),P( 9 )) + getDistance(P( 9 ),P( 2 ));
	double L2 = getDistance(P( 2 ),P( 10 )) + getDistance(P( 10 ),P( 3 ));
	double L3 = getDistance(P( 3 ),P( 11 )) + getDistance(P( 11 ),P( 4 ));
	double L4 = getDistance(P( 4 ),P( 12 )) + getDistance(P( 12 ),P( 1 ));
	double L5 = getDistance(P( 5 ),P( 13 )) + getDistance(P( 13 ),P( 6 ));
	double L6 = getDistance(P( 6 ),P( 14 )) + getDistance(P( 14 ),P( 7 ));
	double L7 = getDistance(P( 7 ),P( 15 )) + getDistance(P( 15 ),P( 8 ));
	double L8 = getDistance(P( 8 ),P( 16 )) + getDistance(P( 16 ),P( 5 ));
	double L9 = getDistance(P( 1 ),P( 17 )) + getDistance(P( 17 ),P( 5 ));
	double L10= getDistance(P( 2 ),P( 18 )) + getDistance(P( 18 ),P( 6 ));
	double L11= getDistance(P( 3 ),P( 19 )) + getDistance(P( 19 ),P( 7 ));
	double L12= getDistance(P( 4 ),P( 20 )) + getDistance(P( 20 ),P( 8 ));
	aVal = Max(Max(Max(L1,L2),Max(L3,L4)),Max(L5,L6));
	aVal = Max(aVal,Max(Max(L7,L8),Max(L9,L10)));
	aVal = Max(aVal,Max(L11,L12));
	break;

      }

    default: aVal=-1;
    }

    if (aVal <0){
      return 0.;
    }

    if ( myPrecision >= 0 )
    {
      double prec = pow( 10., (double)( myPrecision ) );
      aVal = floor( aVal * prec + 0.5 ) / prec;
    }

    return aVal;

  }
  return 0.;
}

double Length2D::GetBadRate( double Value, int /*nbNodes*/ ) const
{
  // meaningless as it is not quality control functor
  return Value;
}

SMDSAbs_ElementType Length2D::GetType() const
{
  return SMDSAbs_Face;
}

Length2D::Value::Value(double theLength,long thePntId1, long thePntId2):
  myLength(theLength)
{
  myPntId[0] = thePntId1;  myPntId[1] = thePntId2;
  if(thePntId1 > thePntId2){
    myPntId[1] = thePntId1;  myPntId[0] = thePntId2;
  }
}

bool Length2D::Value::operator<(const Length2D::Value& x) const{
  if(myPntId[0] < x.myPntId[0]) return true;
  if(myPntId[0] == x.myPntId[0])
    if(myPntId[1] < x.myPntId[1]) return true;
  return false;
}

void Length2D::GetValues(TValues& theValues){
  TValues aValues;
  SMDS_FaceIteratorPtr anIter = myMesh->facesIterator();
  for(; anIter->more(); ){
    const SMDS_MeshFace* anElem = anIter->next();

    if(anElem->IsQuadratic()) {
      const SMDS_QuadraticFaceOfNodes* F =
        static_cast<const SMDS_QuadraticFaceOfNodes*>(anElem);
      // use special nodes iterator
      SMDS_NodeIteratorPtr anIter = F->interlacedNodesIterator();
      long aNodeId[4];
      gp_Pnt P[4];

      double aLength;
      const SMDS_MeshElement* aNode;
      if(anIter->more()){
        aNode = anIter->next();
        const SMDS_MeshNode* aNodes = (SMDS_MeshNode*) aNode;
        P[0] = P[1] = gp_Pnt(aNodes->X(),aNodes->Y(),aNodes->Z());
        aNodeId[0] = aNodeId[1] = aNode->GetID();
        aLength = 0;
      }
      for(; anIter->more(); ){
        const SMDS_MeshNode* N1 = static_cast<const SMDS_MeshNode*> (anIter->next());
        P[2] = gp_Pnt(N1->X(),N1->Y(),N1->Z());
        aNodeId[2] = N1->GetID();
        aLength = P[1].Distance(P[2]);
        if(!anIter->more()) break;
        const SMDS_MeshNode* N2 = static_cast<const SMDS_MeshNode*> (anIter->next());
        P[3] = gp_Pnt(N2->X(),N2->Y(),N2->Z());
        aNodeId[3] = N2->GetID();
        aLength += P[2].Distance(P[3]);
        Value aValue1(aLength,aNodeId[1],aNodeId[2]);
        Value aValue2(aLength,aNodeId[2],aNodeId[3]);
        P[1] = P[3];
        aNodeId[1] = aNodeId[3];
        theValues.insert(aValue1);
        theValues.insert(aValue2);
      }
      aLength += P[2].Distance(P[0]);
      Value aValue1(aLength,aNodeId[1],aNodeId[2]);
      Value aValue2(aLength,aNodeId[2],aNodeId[0]);
      theValues.insert(aValue1);
      theValues.insert(aValue2);
    }
    else {
      SMDS_ElemIteratorPtr aNodesIter = anElem->nodesIterator();
      long aNodeId[2];
      gp_Pnt P[3];

      double aLength;
      const SMDS_MeshElement* aNode;
      if(aNodesIter->more()){
        aNode = aNodesIter->next();
        const SMDS_MeshNode* aNodes = (SMDS_MeshNode*) aNode;
        P[0] = P[1] = gp_Pnt(aNodes->X(),aNodes->Y(),aNodes->Z());
        aNodeId[0] = aNodeId[1] = aNode->GetID();
        aLength = 0;
      }
      for(; aNodesIter->more(); ){
        aNode = aNodesIter->next();
        const SMDS_MeshNode* aNodes = (SMDS_MeshNode*) aNode;
        long anId = aNode->GetID();
        
        P[2] = gp_Pnt(aNodes->X(),aNodes->Y(),aNodes->Z());
        
        aLength = P[1].Distance(P[2]);
        
        Value aValue(aLength,aNodeId[1],anId);
        aNodeId[1] = anId;
        P[1] = P[2];
        theValues.insert(aValue);
      }

      aLength = P[0].Distance(P[1]);

      Value aValue(aLength,aNodeId[0],aNodeId[1]);
      theValues.insert(aValue);
    }
  }
}

/*
  Class       : MultiConnection
  Description : Functor for calculating number of faces conneted to the edge
*/
double MultiConnection::GetValue( const TSequenceOfXYZ& P )
{
  return 0;
}
double MultiConnection::GetValue( long theId )
{
  return getNbMultiConnection( myMesh, theId );
}

double MultiConnection::GetBadRate( double Value, int /*nbNodes*/ ) const
{
  // meaningless as it is not quality control functor
  return Value;
}

SMDSAbs_ElementType MultiConnection::GetType() const
{
  return SMDSAbs_Edge;
}

/*
  Class       : MultiConnection2D
  Description : Functor for calculating number of faces conneted to the edge
*/
double MultiConnection2D::GetValue( const TSequenceOfXYZ& P )
{
  return 0;
}

double MultiConnection2D::GetValue( long theElementId )
{
  int aResult = 0;

  const SMDS_MeshElement* aFaceElem = myMesh->FindElement(theElementId);
  SMDSAbs_ElementType aType = aFaceElem->GetType();

  switch (aType) {
  case SMDSAbs_Face:
    {
      int i = 0, len = aFaceElem->NbNodes();
      SMDS_ElemIteratorPtr anIter = aFaceElem->nodesIterator();
      if (!anIter) break;

      const SMDS_MeshNode *aNode, *aNode0;
      TColStd_MapOfInteger aMap, aMapPrev;

      for (i = 0; i <= len; i++) {
        aMapPrev = aMap;
        aMap.Clear();

        int aNb = 0;
        if (anIter->more()) {
          aNode = (SMDS_MeshNode*)anIter->next();
        } else {
          if (i == len)
            aNode = aNode0;
          else
            break;
        }
        if (!aNode) break;
        if (i == 0) aNode0 = aNode;

        SMDS_ElemIteratorPtr anElemIter = aNode->GetInverseElementIterator();
        while (anElemIter->more()) {
          const SMDS_MeshElement* anElem = anElemIter->next();
          if (anElem != 0 && anElem->GetType() == SMDSAbs_Face) {
            int anId = anElem->GetID();

            aMap.Add(anId);
            if (aMapPrev.Contains(anId)) {
              aNb++;
            }
          }
        }
        aResult = Max(aResult, aNb);
      }
    }
    break;
  default:
    aResult = 0;
  }

  return aResult;
}

double MultiConnection2D::GetBadRate( double Value, int /*nbNodes*/ ) const
{
  // meaningless as it is not quality control functor
  return Value;
}

SMDSAbs_ElementType MultiConnection2D::GetType() const
{
  return SMDSAbs_Face;
}

MultiConnection2D::Value::Value(long thePntId1, long thePntId2)
{
  myPntId[0] = thePntId1;  myPntId[1] = thePntId2;
  if(thePntId1 > thePntId2){
    myPntId[1] = thePntId1;  myPntId[0] = thePntId2;
  }
}

bool MultiConnection2D::Value::operator<(const MultiConnection2D::Value& x) const{
  if(myPntId[0] < x.myPntId[0]) return true;
  if(myPntId[0] == x.myPntId[0])
    if(myPntId[1] < x.myPntId[1]) return true;
  return false;
}

void MultiConnection2D::GetValues(MValues& theValues){
  SMDS_FaceIteratorPtr anIter = myMesh->facesIterator();
  for(; anIter->more(); ){
    const SMDS_MeshFace* anElem = anIter->next();
    SMDS_ElemIteratorPtr aNodesIter;
    if ( anElem->IsQuadratic() )
      aNodesIter = static_cast<const SMDS_QuadraticFaceOfNodes*>
        (anElem)->interlacedNodesElemIterator();
    else
      aNodesIter = anElem->nodesIterator();
    long aNodeId[3];

    //int aNbConnects=0;
    const SMDS_MeshNode* aNode0;
    const SMDS_MeshNode* aNode1;
    const SMDS_MeshNode* aNode2;
    if(aNodesIter->more()){
      aNode0 = (SMDS_MeshNode*) aNodesIter->next();
      aNode1 = aNode0;
      const SMDS_MeshNode* aNodes = (SMDS_MeshNode*) aNode1;
      aNodeId[0] = aNodeId[1] = aNodes->GetID();
    }
    for(; aNodesIter->more(); ) {
      aNode2 = (SMDS_MeshNode*) aNodesIter->next();
      long anId = aNode2->GetID();
      aNodeId[2] = anId;

      Value aValue(aNodeId[1],aNodeId[2]);
      MValues::iterator aItr = theValues.find(aValue);
      if (aItr != theValues.end()){
	aItr->second += 1;
	//aNbConnects = nb;
      }
      else {
	theValues[aValue] = 1;
	//aNbConnects = 1;
      }
      //cout << "NodeIds: "<<aNodeId[1]<<","<<aNodeId[2]<<" nbconn="<<aNbConnects<<endl;
      aNodeId[1] = aNodeId[2];
      aNode1 = aNode2;
    }
    Value aValue(aNodeId[0],aNodeId[2]);
    MValues::iterator aItr = theValues.find(aValue);
    if (aItr != theValues.end()) {
      aItr->second += 1;
      //aNbConnects = nb;
    }
    else {
      theValues[aValue] = 1;
      //aNbConnects = 1;
    }
    //cout << "NodeIds: "<<aNodeId[0]<<","<<aNodeId[2]<<" nbconn="<<aNbConnects<<endl;
  }

}

/*
                            PREDICATES
*/

/*
  Class       : BadOrientedVolume
  Description : Predicate bad oriented volumes
*/

BadOrientedVolume::BadOrientedVolume()
{
  myMesh = 0;
}

void BadOrientedVolume::SetMesh( const SMDS_Mesh* theMesh )
{
  myMesh = theMesh;
}

bool BadOrientedVolume::IsSatisfy( long theId )
{
  if ( myMesh == 0 )
    return false;

  SMDS_VolumeTool vTool( myMesh->FindElement( theId ));
  return !vTool.IsForward();
}

SMDSAbs_ElementType BadOrientedVolume::GetType() const
{
  return SMDSAbs_Volume;
}



/*
  Class       : FreeBorders
  Description : Predicate for free borders
*/

FreeBorders::FreeBorders()
{
  myMesh = 0;
}

void FreeBorders::SetMesh( const SMDS_Mesh* theMesh )
{
  myMesh = theMesh;
}

bool FreeBorders::IsSatisfy( long theId )
{
  return getNbMultiConnection( myMesh, theId ) == 1;
}

SMDSAbs_ElementType FreeBorders::GetType() const
{
  return SMDSAbs_Edge;
}


/*
  Class       : FreeEdges
  Description : Predicate for free Edges
*/
FreeEdges::FreeEdges()
{
  myMesh = 0;
}

void FreeEdges::SetMesh( const SMDS_Mesh* theMesh )
{
  myMesh = theMesh;
}

bool FreeEdges::IsFreeEdge( const SMDS_MeshNode** theNodes, const int theFaceId  )
{
  TColStd_MapOfInteger aMap;
  for ( int i = 0; i < 2; i++ )
  {
    SMDS_ElemIteratorPtr anElemIter = theNodes[ i ]->GetInverseElementIterator();
    while( anElemIter->more() )
    {
      const SMDS_MeshElement* anElem = anElemIter->next();
      if ( anElem != 0 && anElem->GetType() == SMDSAbs_Face )
      {
        int anId = anElem->GetID();

        if ( i == 0 )
          aMap.Add( anId );
        else if ( aMap.Contains( anId ) && anId != theFaceId )
          return false;
      }
    }
  }
  return true;
}

bool FreeEdges::IsSatisfy( long theId )
{
  if ( myMesh == 0 )
    return false;

  const SMDS_MeshElement* aFace = myMesh->FindElement( theId );
  if ( aFace == 0 || aFace->GetType() != SMDSAbs_Face || aFace->NbNodes() < 3 )
    return false;

  SMDS_ElemIteratorPtr anIter;
  if ( aFace->IsQuadratic() ) {
    anIter = static_cast<const SMDS_QuadraticFaceOfNodes*>
      (aFace)->interlacedNodesElemIterator();
  }
  else {
    anIter = aFace->nodesIterator();
  }
  if ( anIter == 0 )
    return false;

  int i = 0, nbNodes = aFace->NbNodes();
  std::vector <const SMDS_MeshNode*> aNodes( nbNodes+1 );
  while( anIter->more() )
  {
    const SMDS_MeshNode* aNode = (SMDS_MeshNode*)anIter->next();
    if ( aNode == 0 )
      return false;
    aNodes[ i++ ] = aNode;
  }
  aNodes[ nbNodes ] = aNodes[ 0 ];

  for ( i = 0; i < nbNodes; i++ )
    if ( IsFreeEdge( &aNodes[ i ], theId ) )
      return true;

  return false;
}

SMDSAbs_ElementType FreeEdges::GetType() const
{
  return SMDSAbs_Face;
}

FreeEdges::Border::Border(long theElemId, long thePntId1, long thePntId2):
  myElemId(theElemId)
{
  myPntId[0] = thePntId1;  myPntId[1] = thePntId2;
  if(thePntId1 > thePntId2){
    myPntId[1] = thePntId1;  myPntId[0] = thePntId2;
  }
}

bool FreeEdges::Border::operator<(const FreeEdges::Border& x) const{
  if(myPntId[0] < x.myPntId[0]) return true;
  if(myPntId[0] == x.myPntId[0])
    if(myPntId[1] < x.myPntId[1]) return true;
  return false;
}

inline void UpdateBorders(const FreeEdges::Border& theBorder,
			  FreeEdges::TBorders& theRegistry,
			  FreeEdges::TBorders& theContainer)
{
  if(theRegistry.find(theBorder) == theRegistry.end()){
    theRegistry.insert(theBorder);
    theContainer.insert(theBorder);
  }else{
    theContainer.erase(theBorder);
  }
}

void FreeEdges::GetBoreders(TBorders& theBorders)
{
  TBorders aRegistry;
  SMDS_FaceIteratorPtr anIter = myMesh->facesIterator();
  for(; anIter->more(); ){
    const SMDS_MeshFace* anElem = anIter->next();
    long anElemId = anElem->GetID();
    SMDS_ElemIteratorPtr aNodesIter;
    if ( anElem->IsQuadratic() )
      aNodesIter = static_cast<const SMDS_QuadraticFaceOfNodes*>(anElem)->
        interlacedNodesElemIterator();
    else
      aNodesIter = anElem->nodesIterator();
    long aNodeId[2];
    const SMDS_MeshElement* aNode;
    if(aNodesIter->more()){
      aNode = aNodesIter->next();
      aNodeId[0] = aNodeId[1] = aNode->GetID();
    }
    for(; aNodesIter->more(); ){
      aNode = aNodesIter->next();
      long anId = aNode->GetID();
      Border aBorder(anElemId,aNodeId[1],anId);
      aNodeId[1] = anId;
      //std::cout<<aBorder.myPntId[0]<<"; "<<aBorder.myPntId[1]<<"; "<<aBorder.myElemId<<endl;
      UpdateBorders(aBorder,aRegistry,theBorders);
    }
    Border aBorder(anElemId,aNodeId[0],aNodeId[1]);
    //std::cout<<aBorder.myPntId[0]<<"; "<<aBorder.myPntId[1]<<"; "<<aBorder.myElemId<<endl;
    UpdateBorders(aBorder,aRegistry,theBorders);
  }
  //std::cout<<"theBorders.size() = "<<theBorders.size()<<endl;
}


/*
  Class       : FreeNodes
  Description : Predicate for free nodes
*/

FreeNodes::FreeNodes()
{
  myMesh = 0;
}

void FreeNodes::SetMesh( const SMDS_Mesh* theMesh )
{
  myMesh = theMesh;
}

bool FreeNodes::IsSatisfy( long theNodeId )
{
  const SMDS_MeshNode* aNode = myMesh->FindNode( theNodeId );
  if (!aNode)
    return false;

  return (aNode->NbInverseElements() < 1);
}

SMDSAbs_ElementType FreeNodes::GetType() const
{
  return SMDSAbs_Node;
}


/*
  Class       : FreeFaces
  Description : Predicate for free faces
*/

FreeFaces::FreeFaces()
{
  myMesh = 0;
}

void FreeFaces::SetMesh( const SMDS_Mesh* theMesh )
{
  myMesh = theMesh;
}

bool FreeFaces::IsSatisfy( long theId )
{
  if (!myMesh) return false;
  // check that faces nodes refers to less than two common volumes
  const SMDS_MeshElement* aFace = myMesh->FindElement( theId );
  if ( !aFace || aFace->GetType() != SMDSAbs_Face )
    return false;

  int nbNode = aFace->NbNodes();

  // collect volumes check that number of volumss with count equal nbNode not less than 2
  typedef map< SMDS_MeshElement*, int > TMapOfVolume; // map of volume counters
  typedef map< SMDS_MeshElement*, int >::iterator TItrMapOfVolume; // iterator
  TMapOfVolume mapOfVol;

  SMDS_ElemIteratorPtr nodeItr = aFace->nodesIterator();
  while ( nodeItr->more() ) {
    const SMDS_MeshNode* aNode = static_cast<const SMDS_MeshNode*>(nodeItr->next());
    if ( !aNode ) continue;
    SMDS_ElemIteratorPtr volItr = aNode->GetInverseElementIterator(SMDSAbs_Volume);
    while ( volItr->more() ) {
      SMDS_MeshElement* aVol = (SMDS_MeshElement*)volItr->next();
      TItrMapOfVolume itr = mapOfVol.insert(make_pair(aVol, 0)).first;
      (*itr).second++;
    } 
  }
  int nbVol = 0;
  TItrMapOfVolume volItr = mapOfVol.begin();
  TItrMapOfVolume volEnd = mapOfVol.end();
  for ( ; volItr != volEnd; ++volItr )
    if ( (*volItr).second >= nbNode )
       nbVol++;
  // face is not free if number of volumes constructed on thier nodes more than one
  return (nbVol < 2);
}

SMDSAbs_ElementType FreeFaces::GetType() const
{
  return SMDSAbs_Face;
}

/*
  Class       : LinearOrQuadratic
  Description : Predicate to verify whether a mesh element is linear
*/

LinearOrQuadratic::LinearOrQuadratic()
{
  myMesh = 0;
}

void LinearOrQuadratic::SetMesh( const SMDS_Mesh* theMesh )
{
  myMesh = theMesh;
}

bool LinearOrQuadratic::IsSatisfy( long theId )
{
  if (!myMesh) return false;
  const SMDS_MeshElement* anElem = myMesh->FindElement( theId );
  if ( !anElem || (myType != SMDSAbs_All && anElem->GetType() != myType) )
    return false;
  return (!anElem->IsQuadratic());
}

void LinearOrQuadratic::SetType( SMDSAbs_ElementType theType )
{
  myType = theType;
}

SMDSAbs_ElementType LinearOrQuadratic::GetType() const
{
  return myType;
}

/*
  Class       : GroupColor
  Description : Functor for check color of group to whic mesh element belongs to
*/

GroupColor::GroupColor()
{
}

bool GroupColor::IsSatisfy( long theId )
{
  return (myIDs.find( theId ) != myIDs.end());
}

void GroupColor::SetType( SMDSAbs_ElementType theType )
{
  myType = theType;
}

SMDSAbs_ElementType GroupColor::GetType() const
{
  return myType;
}

static bool isEqual( const Quantity_Color& theColor1,
                     const Quantity_Color& theColor2 )
{
  // tolerance to compare colors
  const double tol = 5*1e-3;
  return ( fabs( theColor1.Red() - theColor2.Red() ) < tol &&
           fabs( theColor1.Green() - theColor2.Green() ) < tol &&
           fabs( theColor1.Blue() - theColor2.Blue() ) < tol );
}


void GroupColor::SetMesh( const SMDS_Mesh* theMesh )
{
  myIDs.clear();
  
  const SMESHDS_Mesh* aMesh = dynamic_cast<const SMESHDS_Mesh*>(theMesh);
  if ( !aMesh )
    return;

  int nbGrp = aMesh->GetNbGroups();
  if ( !nbGrp )
    return;
  
  // iterates on groups and find necessary elements ids
  const std::set<SMESHDS_GroupBase*>& aGroups = aMesh->GetGroups();
  set<SMESHDS_GroupBase*>::const_iterator GrIt = aGroups.begin();
  for (; GrIt != aGroups.end(); GrIt++) {
    SMESHDS_GroupBase* aGrp = (*GrIt);
    if ( !aGrp )
      continue;
    // check type and color of group
    if ( !isEqual( myColor, aGrp->GetColor() ) )
      continue;
    if ( myType != SMDSAbs_All && myType != (SMDSAbs_ElementType)aGrp->GetType() )
      continue;

    SMDSAbs_ElementType aGrpElType = (SMDSAbs_ElementType)aGrp->GetType();
    if ( myType == aGrpElType || (myType == SMDSAbs_All && aGrpElType != SMDSAbs_Node) ) {
      // add elements IDS into control
      int aSize = aGrp->Extent();
      for (int i = 0; i < aSize; i++)
        myIDs.insert( aGrp->GetID(i+1) );
    }
  }
}

void GroupColor::SetColorStr( const TCollection_AsciiString& theStr )
{
  TCollection_AsciiString aStr = theStr;
  aStr.RemoveAll( ' ' );
  aStr.RemoveAll( '\t' );
  for ( int aPos = aStr.Search( ";;" ); aPos != -1; aPos = aStr.Search( ";;" ) )
    aStr.Remove( aPos, 2 );
  Standard_Real clr[3];
  clr[0] = clr[1] = clr[2] = 0.;
  for ( int i = 0; i < 3; i++ ) {
    TCollection_AsciiString tmpStr = aStr.Token( ";", i+1 );
    if ( !tmpStr.IsEmpty() && tmpStr.IsRealValue() )
      clr[i] = tmpStr.RealValue();
  }
  myColor = Quantity_Color( clr[0], clr[1], clr[2], Quantity_TOC_RGB );
}

//=======================================================================
// name    : GetRangeStr
// Purpose : Get range as a string.
//           Example: "1,2,3,50-60,63,67,70-"
//=======================================================================
void GroupColor::GetColorStr( TCollection_AsciiString& theResStr ) const
{
  theResStr.Clear();
  theResStr += TCollection_AsciiString( myColor.Red() );
  theResStr += TCollection_AsciiString( ";" ) + TCollection_AsciiString( myColor.Green() );
  theResStr += TCollection_AsciiString( ";" ) + TCollection_AsciiString( myColor.Blue() );
}

/*
  Class       : ElemGeomType
  Description : Predicate to check element geometry type
*/

ElemGeomType::ElemGeomType()
{
  myMesh = 0;
  myType = SMDSAbs_All;
  myGeomType = SMDSGeom_TRIANGLE;
}

void ElemGeomType::SetMesh( const SMDS_Mesh* theMesh )
{
  myMesh = theMesh;
}

bool ElemGeomType::IsSatisfy( long theId )
{
  if (!myMesh) return false;
  const SMDS_MeshElement* anElem = myMesh->FindElement( theId );
  const SMDSAbs_ElementType anElemType = anElem->GetType();
  if ( !anElem || (myType != SMDSAbs_All && anElemType != myType) )
    return false;
  const int aNbNode = anElem->NbNodes();
  bool isOk = false;
  switch( anElemType )
  {
  case SMDSAbs_Node:
    isOk = (myGeomType == SMDSGeom_POINT);
    break;

  case SMDSAbs_Edge:
    isOk = (myGeomType == SMDSGeom_EDGE);
    break;
	
  case SMDSAbs_Face:
    if ( myGeomType == SMDSGeom_TRIANGLE )
      isOk = (!anElem->IsPoly() && (anElem->IsQuadratic() ? aNbNode == 6 : aNbNode == 3));
    else if ( myGeomType == SMDSGeom_QUADRANGLE )
      isOk = (!anElem->IsPoly() && (anElem->IsQuadratic() ? aNbNode == 8 : aNbNode == 4));
    else if ( myGeomType == SMDSGeom_POLYGON )
      isOk = anElem->IsPoly();
    break;

  case SMDSAbs_Volume:
    if ( myGeomType == SMDSGeom_TETRA )
      isOk = (!anElem->IsPoly() && (anElem->IsQuadratic() ? aNbNode == 10 : aNbNode == 4));
    else if ( myGeomType == SMDSGeom_PYRAMID )
      isOk = (!anElem->IsPoly() && (anElem->IsQuadratic() ? aNbNode == 13 : aNbNode == 5));
    else if ( myGeomType == SMDSGeom_PENTA )
      isOk = (!anElem->IsPoly() && (anElem->IsQuadratic() ? aNbNode == 15 : aNbNode == 6));
    else if ( myGeomType == SMDSGeom_HEXA )
      isOk = (!anElem->IsPoly() && (anElem->IsQuadratic() ? aNbNode == 20 : aNbNode == 8));
     else if ( myGeomType == SMDSGeom_POLYHEDRA )
      isOk = anElem->IsPoly();
    break;
	
    default: break;
  }
  return isOk;
}

void ElemGeomType::SetType( SMDSAbs_ElementType theType )
{
  myType = theType;
}

SMDSAbs_ElementType ElemGeomType::GetType() const
{
  return myType;
}

void ElemGeomType::SetGeomType( SMDSAbs_GeometryType theType )
{
  myGeomType = theType;
}

SMDSAbs_GeometryType ElemGeomType::GetGeomType() const
{
  return myGeomType;
}

/*
  Class       : RangeOfIds
  Description : Predicate for Range of Ids.
                Range may be specified with two ways.
                1. Using AddToRange method
                2. With SetRangeStr method. Parameter of this method is a string
                   like as "1,2,3,50-60,63,67,70-"
*/

//=======================================================================
// name    : RangeOfIds
// Purpose : Constructor
//=======================================================================
RangeOfIds::RangeOfIds()
{
  myMesh = 0;
  myType = SMDSAbs_All;
}

//=======================================================================
// name    : SetMesh
// Purpose : Set mesh
//=======================================================================
void RangeOfIds::SetMesh( const SMDS_Mesh* theMesh )
{
  myMesh = theMesh;
}

//=======================================================================
// name    : AddToRange
// Purpose : Add ID to the range
//=======================================================================
bool RangeOfIds::AddToRange( long theEntityId )
{
  myIds.Add( theEntityId );
  return true;
}

//=======================================================================
// name    : GetRangeStr
// Purpose : Get range as a string.
//           Example: "1,2,3,50-60,63,67,70-"
//=======================================================================
void RangeOfIds::GetRangeStr( TCollection_AsciiString& theResStr )
{
  theResStr.Clear();

  TColStd_SequenceOfInteger     anIntSeq;
  TColStd_SequenceOfAsciiString aStrSeq;

  TColStd_MapIteratorOfMapOfInteger anIter( myIds );
  for ( ; anIter.More(); anIter.Next() )
  {
    int anId = anIter.Key();
    TCollection_AsciiString aStr( anId );
    anIntSeq.Append( anId );
    aStrSeq.Append( aStr );
  }

  for ( int i = 1, n = myMin.Length(); i <= n; i++ )
  {
    int aMinId = myMin( i );
    int aMaxId = myMax( i );

    TCollection_AsciiString aStr;
    if ( aMinId != IntegerFirst() )
      aStr += aMinId;

    aStr += "-";

    if ( aMaxId != IntegerLast() )
      aStr += aMaxId;

    // find position of the string in result sequence and insert string in it
    if ( anIntSeq.Length() == 0 )
    {
      anIntSeq.Append( aMinId );
      aStrSeq.Append( aStr );
    }
    else
    {
      if ( aMinId < anIntSeq.First() )
      {
        anIntSeq.Prepend( aMinId );
        aStrSeq.Prepend( aStr );
      }
      else if ( aMinId > anIntSeq.Last() )
      {
        anIntSeq.Append( aMinId );
        aStrSeq.Append( aStr );
      }
      else
        for ( int j = 1, k = anIntSeq.Length(); j <= k; j++ )
          if ( aMinId < anIntSeq( j ) )
          {
            anIntSeq.InsertBefore( j, aMinId );
            aStrSeq.InsertBefore( j, aStr );
            break;
          }
    }
  }

  if ( aStrSeq.Length() == 0 )
    return;

  theResStr = aStrSeq( 1 );
  for ( int j = 2, k = aStrSeq.Length(); j <= k; j++  )
  {
    theResStr += ",";
    theResStr += aStrSeq( j );
  }
}

//=======================================================================
// name    : SetRangeStr
// Purpose : Define range with string
//           Example of entry string: "1,2,3,50-60,63,67,70-"
//=======================================================================
bool RangeOfIds::SetRangeStr( const TCollection_AsciiString& theStr )
{
  myMin.Clear();
  myMax.Clear();
  myIds.Clear();

  TCollection_AsciiString aStr = theStr;
  aStr.RemoveAll( ' ' );
  aStr.RemoveAll( '\t' );

  for ( int aPos = aStr.Search( ",," ); aPos != -1; aPos = aStr.Search( ",," ) )
    aStr.Remove( aPos, 2 );

  TCollection_AsciiString tmpStr = aStr.Token( ",", 1 );
  int i = 1;
  while ( tmpStr != "" )
  {
    tmpStr = aStr.Token( ",", i++ );
    int aPos = tmpStr.Search( '-' );

    if ( aPos == -1 )
    {
      if ( tmpStr.IsIntegerValue() )
        myIds.Add( tmpStr.IntegerValue() );
      else
        return false;
    }
    else
    {
      TCollection_AsciiString aMaxStr = tmpStr.Split( aPos );
      TCollection_AsciiString aMinStr = tmpStr;

      while ( aMinStr.Search( "-" ) != -1 ) aMinStr.RemoveAll( '-' );
      while ( aMaxStr.Search( "-" ) != -1 ) aMaxStr.RemoveAll( '-' );

      if ( !aMinStr.IsEmpty() && !aMinStr.IsIntegerValue() ||
           !aMaxStr.IsEmpty() && !aMaxStr.IsIntegerValue() )
        return false;

      myMin.Append( aMinStr.IsEmpty() ? IntegerFirst() : aMinStr.IntegerValue() );
      myMax.Append( aMaxStr.IsEmpty() ? IntegerLast()  : aMaxStr.IntegerValue() );
    }
  }

  return true;
}

//=======================================================================
// name    : GetType
// Purpose : Get type of supported entities
//=======================================================================
SMDSAbs_ElementType RangeOfIds::GetType() const
{
  return myType;
}

//=======================================================================
// name    : SetType
// Purpose : Set type of supported entities
//=======================================================================
void RangeOfIds::SetType( SMDSAbs_ElementType theType )
{
  myType = theType;
}

//=======================================================================
// name    : IsSatisfy
// Purpose : Verify whether entity satisfies to this rpedicate
//=======================================================================
bool RangeOfIds::IsSatisfy( long theId )
{
  if ( !myMesh )
    return false;

  if ( myType == SMDSAbs_Node )
  {
    if ( myMesh->FindNode( theId ) == 0 )
      return false;
  }
  else
  {
    const SMDS_MeshElement* anElem = myMesh->FindElement( theId );
    if ( anElem == 0 || myType != anElem->GetType() && myType != SMDSAbs_All )
      return false;
  }

  if ( myIds.Contains( theId ) )
    return true;

  for ( int i = 1, n = myMin.Length(); i <= n; i++ )
    if ( theId >= myMin( i ) && theId <= myMax( i ) )
      return true;

  return false;
}

/*
  Class       : Comparator
  Description : Base class for comparators
*/
Comparator::Comparator():
  myMargin(0)
{}

Comparator::~Comparator()
{}

void Comparator::SetMesh( const SMDS_Mesh* theMesh )
{
  if ( myFunctor )
    myFunctor->SetMesh( theMesh );
}

void Comparator::SetMargin( double theValue )
{
  myMargin = theValue;
}

void Comparator::SetNumFunctor( NumericalFunctorPtr theFunct )
{
  myFunctor = theFunct;
}

SMDSAbs_ElementType Comparator::GetType() const
{
  return myFunctor ? myFunctor->GetType() : SMDSAbs_All;
}

double Comparator::GetMargin()
{
  return myMargin;
}


/*
  Class       : LessThan
  Description : Comparator "<"
*/
bool LessThan::IsSatisfy( long theId )
{
  return (myFunctor!=NULL) && myFunctor->GetValue( theId ) < myMargin;
}


/*
  Class       : MoreThan
  Description : Comparator ">"
*/
bool MoreThan::IsSatisfy( long theId )
{
  return (myFunctor!=NULL) && myFunctor->GetValue( theId ) > myMargin;
}


/*
  Class       : EqualTo
  Description : Comparator "="
*/
EqualTo::EqualTo():
  myToler(Precision::Confusion())
{}

bool EqualTo::IsSatisfy( long theId )
{
  return (myFunctor!=NULL) && fabs( myFunctor->GetValue( theId ) - myMargin ) < myToler;
}

void EqualTo::SetTolerance( double theToler )
{
  myToler = theToler;
}

double EqualTo::GetTolerance()
{
  return myToler;
}

/*
  Class       : LogicalNOT
  Description : Logical NOT predicate
*/
LogicalNOT::LogicalNOT()
{}

LogicalNOT::~LogicalNOT()
{}

bool LogicalNOT::IsSatisfy( long theId )
{
  return (myPredicate!=NULL) && !myPredicate->IsSatisfy( theId );
}

void LogicalNOT::SetMesh( const SMDS_Mesh* theMesh )
{
  if ( myPredicate )
    myPredicate->SetMesh( theMesh );
}

void LogicalNOT::SetPredicate( PredicatePtr thePred )
{
  myPredicate = thePred;
}

SMDSAbs_ElementType LogicalNOT::GetType() const
{
  return myPredicate ? myPredicate->GetType() : SMDSAbs_All;
}


/*
  Class       : LogicalBinary
  Description : Base class for binary logical predicate
*/
LogicalBinary::LogicalBinary()
{}

LogicalBinary::~LogicalBinary()
{}

void LogicalBinary::SetMesh( const SMDS_Mesh* theMesh )
{
  if ( myPredicate1 )
    myPredicate1->SetMesh( theMesh );

  if ( myPredicate2 )
    myPredicate2->SetMesh( theMesh );
}

void LogicalBinary::SetPredicate1( PredicatePtr thePredicate )
{
  myPredicate1 = thePredicate;
}

void LogicalBinary::SetPredicate2( PredicatePtr thePredicate )
{
  myPredicate2 = thePredicate;
}

SMDSAbs_ElementType LogicalBinary::GetType() const
{
  if ( !myPredicate1 || !myPredicate2 )
    return SMDSAbs_All;

  SMDSAbs_ElementType aType1 = myPredicate1->GetType();
  SMDSAbs_ElementType aType2 = myPredicate2->GetType();

  return aType1 == aType2 ? aType1 : SMDSAbs_All;
}


/*
  Class       : LogicalAND
  Description : Logical AND
*/
bool LogicalAND::IsSatisfy( long theId )
{
  return
    (myPredicate1!=NULL) &&
    (myPredicate2!=NULL) &&
    myPredicate1->IsSatisfy( theId ) &&
    myPredicate2->IsSatisfy( theId );
}


/*
  Class       : LogicalOR
  Description : Logical OR
*/
bool LogicalOR::IsSatisfy( long theId )
{
  return
    (myPredicate1!=NULL) &&
    (myPredicate2!=NULL) &&
    myPredicate1->IsSatisfy( theId ) ||
    myPredicate2->IsSatisfy( theId );
}


/*
                              FILTER
*/

Filter::Filter()
{}

Filter::~Filter()
{}

void Filter::SetPredicate( PredicatePtr thePredicate )
{
  myPredicate = thePredicate;
}

template<class TElement, class TIterator, class TPredicate>
inline void FillSequence(const TIterator& theIterator,
			 TPredicate& thePredicate,
			 Filter::TIdSequence& theSequence)
{
  if ( theIterator ) {
    while( theIterator->more() ) {
      TElement anElem = theIterator->next();
      long anId = anElem->GetID();
      if ( thePredicate->IsSatisfy( anId ) )
	theSequence.push_back( anId );
    }
  }
}

void
Filter::
GetElementsId( const SMDS_Mesh* theMesh,
	       PredicatePtr thePredicate,
	       TIdSequence& theSequence )
{
  theSequence.clear();

  if ( !theMesh || !thePredicate )
    return;

  thePredicate->SetMesh( theMesh );

  SMDSAbs_ElementType aType = thePredicate->GetType();
  switch(aType){
  case SMDSAbs_Node:
    FillSequence<const SMDS_MeshNode*>(theMesh->nodesIterator(),thePredicate,theSequence);
    break;
  case SMDSAbs_Edge:
    FillSequence<const SMDS_MeshElement*>(theMesh->edgesIterator(),thePredicate,theSequence);
    break;
  case SMDSAbs_Face:
    FillSequence<const SMDS_MeshElement*>(theMesh->facesIterator(),thePredicate,theSequence);
    break;
  case SMDSAbs_Volume:
    FillSequence<const SMDS_MeshElement*>(theMesh->volumesIterator(),thePredicate,theSequence);
    break;
  case SMDSAbs_All:
    FillSequence<const SMDS_MeshElement*>(theMesh->edgesIterator(),thePredicate,theSequence);
    FillSequence<const SMDS_MeshElement*>(theMesh->facesIterator(),thePredicate,theSequence);
    FillSequence<const SMDS_MeshElement*>(theMesh->volumesIterator(),thePredicate,theSequence);
    break;
  }
}

void
Filter::GetElementsId( const SMDS_Mesh* theMesh,
		       Filter::TIdSequence& theSequence )
{
  GetElementsId(theMesh,myPredicate,theSequence);
}

/*
                              ManifoldPart
*/

typedef std::set<SMDS_MeshFace*>                    TMapOfFacePtr;

/*
   Internal class Link
*/

ManifoldPart::Link::Link( SMDS_MeshNode* theNode1,
                          SMDS_MeshNode* theNode2 )
{
  myNode1 = theNode1;
  myNode2 = theNode2;
}

ManifoldPart::Link::~Link()
{
  myNode1 = 0;
  myNode2 = 0;
}

bool ManifoldPart::Link::IsEqual( const ManifoldPart::Link& theLink ) const
{
  if ( myNode1 == theLink.myNode1 &&
       myNode2 == theLink.myNode2 )
    return true;
  else if ( myNode1 == theLink.myNode2 &&
            myNode2 == theLink.myNode1 )
    return true;
  else
    return false;
}

bool ManifoldPart::Link::operator<( const ManifoldPart::Link& x ) const
{
  if(myNode1 < x.myNode1) return true;
  if(myNode1 == x.myNode1)
    if(myNode2 < x.myNode2) return true;
  return false;
}

bool ManifoldPart::IsEqual( const ManifoldPart::Link& theLink1,
                            const ManifoldPart::Link& theLink2 )
{
  return theLink1.IsEqual( theLink2 );
}

ManifoldPart::ManifoldPart()
{
  myMesh = 0;
  myAngToler = Precision::Angular();
  myIsOnlyManifold = true;
}

ManifoldPart::~ManifoldPart()
{
  myMesh = 0;
}

void ManifoldPart::SetMesh( const SMDS_Mesh* theMesh )
{
  myMesh = theMesh;
  process();
}

SMDSAbs_ElementType ManifoldPart::GetType() const
{ return SMDSAbs_Face; }

bool ManifoldPart::IsSatisfy( long theElementId )
{
  return myMapIds.Contains( theElementId );
}

void ManifoldPart::SetAngleTolerance( const double theAngToler )
{ myAngToler = theAngToler; }

double ManifoldPart::GetAngleTolerance() const
{ return myAngToler; }

void ManifoldPart::SetIsOnlyManifold( const bool theIsOnly )
{ myIsOnlyManifold = theIsOnly; }

void ManifoldPart::SetStartElem( const long  theStartId )
{ myStartElemId = theStartId; }

bool ManifoldPart::process()
{
  myMapIds.Clear();
  myMapBadGeomIds.Clear();

  myAllFacePtr.clear();
  myAllFacePtrIntDMap.clear();
  if ( !myMesh )
    return false;

  // collect all faces into own map
  SMDS_FaceIteratorPtr anFaceItr = myMesh->facesIterator();
  for (; anFaceItr->more(); )
  {
    SMDS_MeshFace* aFacePtr = (SMDS_MeshFace*)anFaceItr->next();
    myAllFacePtr.push_back( aFacePtr );
    myAllFacePtrIntDMap[aFacePtr] = myAllFacePtr.size()-1;
  }

  SMDS_MeshFace* aStartFace = (SMDS_MeshFace*)myMesh->FindElement( myStartElemId );
  if ( !aStartFace )
    return false;

  // the map of non manifold links and bad geometry
  TMapOfLink aMapOfNonManifold;
  TColStd_MapOfInteger aMapOfTreated;

  // begin cycle on faces from start index and run on vector till the end
  //  and from begin to start index to cover whole vector
  const int aStartIndx = myAllFacePtrIntDMap[aStartFace];
  bool isStartTreat = false;
  for ( int fi = aStartIndx; !isStartTreat || fi != aStartIndx ; fi++ )
  {
    if ( fi == aStartIndx )
      isStartTreat = true;
    // as result next time when fi will be equal to aStartIndx

    SMDS_MeshFace* aFacePtr = myAllFacePtr[ fi ];
    if ( aMapOfTreated.Contains( aFacePtr->GetID() ) )
      continue;

    aMapOfTreated.Add( aFacePtr->GetID() );
    TColStd_MapOfInteger aResFaces;
    if ( !findConnected( myAllFacePtrIntDMap, aFacePtr,
                         aMapOfNonManifold, aResFaces ) )
      continue;
    TColStd_MapIteratorOfMapOfInteger anItr( aResFaces );
    for ( ; anItr.More(); anItr.Next() )
    {
      int aFaceId = anItr.Key();
      aMapOfTreated.Add( aFaceId );
      myMapIds.Add( aFaceId );
    }

    if ( fi == ( myAllFacePtr.size() - 1 ) )
      fi = 0;
  } // end run on vector of faces
  return !myMapIds.IsEmpty();
}

static void getLinks( const SMDS_MeshFace* theFace,
                      ManifoldPart::TVectorOfLink& theLinks )
{
  int aNbNode = theFace->NbNodes();
  SMDS_ElemIteratorPtr aNodeItr = theFace->nodesIterator();
  int i = 1;
  SMDS_MeshNode* aNode = 0;
  for ( ; aNodeItr->more() && i <= aNbNode; )
  {

    SMDS_MeshNode* aN1 = (SMDS_MeshNode*)aNodeItr->next();
    if ( i == 1 )
      aNode = aN1;
    i++;
    SMDS_MeshNode* aN2 = ( i >= aNbNode ) ? aNode : (SMDS_MeshNode*)aNodeItr->next();
    i++;
    ManifoldPart::Link aLink( aN1, aN2 );
    theLinks.push_back( aLink );
  }
}

static gp_XYZ getNormale( const SMDS_MeshFace* theFace )
{
  gp_XYZ n;
  int aNbNode = theFace->NbNodes();
  TColgp_Array1OfXYZ anArrOfXYZ(1,4);
  SMDS_ElemIteratorPtr aNodeItr = theFace->nodesIterator();
  int i = 1;
  for ( ; aNodeItr->more() && i <= 4; i++ ) {
    SMDS_MeshNode* aNode = (SMDS_MeshNode*)aNodeItr->next();
    anArrOfXYZ.SetValue(i, gp_XYZ( aNode->X(), aNode->Y(), aNode->Z() ) );
  }

  gp_XYZ q1 = anArrOfXYZ.Value(2) - anArrOfXYZ.Value(1);
  gp_XYZ q2 = anArrOfXYZ.Value(3) - anArrOfXYZ.Value(1);
  n  = q1 ^ q2;
  if ( aNbNode > 3 ) {
    gp_XYZ q3 = anArrOfXYZ.Value(4) - anArrOfXYZ.Value(1);
    n += q2 ^ q3;
  }
  double len = n.Modulus();
  if ( len > 0 )
    n /= len;

  return n;
}

bool ManifoldPart::findConnected
                 ( const ManifoldPart::TDataMapFacePtrInt& theAllFacePtrInt,
                  SMDS_MeshFace*                           theStartFace,
                  ManifoldPart::TMapOfLink&                theNonManifold,
                  TColStd_MapOfInteger&                    theResFaces )
{
  theResFaces.Clear();
  if ( !theAllFacePtrInt.size() )
    return false;

  if ( getNormale( theStartFace ).SquareModulus() <= gp::Resolution() )
  {
    myMapBadGeomIds.Add( theStartFace->GetID() );
    return false;
  }

  ManifoldPart::TMapOfLink aMapOfBoundary, aMapToSkip;
  ManifoldPart::TVectorOfLink aSeqOfBoundary;
  theResFaces.Add( theStartFace->GetID() );
  ManifoldPart::TDataMapOfLinkFacePtr aDMapLinkFace;

  expandBoundary( aMapOfBoundary, aSeqOfBoundary,
                 aDMapLinkFace, theNonManifold, theStartFace );

  bool isDone = false;
  while ( !isDone && aMapOfBoundary.size() != 0 )
  {
    bool isToReset = false;
    ManifoldPart::TVectorOfLink::iterator pLink = aSeqOfBoundary.begin();
    for ( ; !isToReset && pLink != aSeqOfBoundary.end(); ++pLink )
    {
      ManifoldPart::Link aLink = *pLink;
      if ( aMapToSkip.find( aLink ) != aMapToSkip.end() )
        continue;
      // each link could be treated only once
      aMapToSkip.insert( aLink );

      ManifoldPart::TVectorOfFacePtr aFaces;
      // find next
      if ( myIsOnlyManifold &&
           (theNonManifold.find( aLink ) != theNonManifold.end()) )
        continue;
      else
      {
        getFacesByLink( aLink, aFaces );
        // filter the element to keep only indicated elements
        ManifoldPart::TVectorOfFacePtr aFiltered;
        ManifoldPart::TVectorOfFacePtr::iterator pFace = aFaces.begin();
        for ( ; pFace != aFaces.end(); ++pFace )
        {
          SMDS_MeshFace* aFace = *pFace;
          if ( myAllFacePtrIntDMap.find( aFace ) != myAllFacePtrIntDMap.end() )
            aFiltered.push_back( aFace );
        }
        aFaces = aFiltered;
        if ( aFaces.size() < 2 )  // no neihgbour faces
          continue;
        else if ( myIsOnlyManifold && aFaces.size() > 2 ) // non manifold case
        {
          theNonManifold.insert( aLink );
          continue;
        }
      }

      // compare normal with normals of neighbor element
      SMDS_MeshFace* aPrevFace = aDMapLinkFace[ aLink ];
      ManifoldPart::TVectorOfFacePtr::iterator pFace = aFaces.begin();
      for ( ; pFace != aFaces.end(); ++pFace )
      {
        SMDS_MeshFace* aNextFace = *pFace;
        if ( aPrevFace == aNextFace )
          continue;
        int anNextFaceID = aNextFace->GetID();
        if ( myIsOnlyManifold && theResFaces.Contains( anNextFaceID ) )
         // should not be with non manifold restriction. probably bad topology
          continue;
        // check if face was treated and skipped
        if ( myMapBadGeomIds.Contains( anNextFaceID ) ||
             !isInPlane( aPrevFace, aNextFace ) )
          continue;
        // add new element to connected and extend the boundaries.
        theResFaces.Add( anNextFaceID );
        expandBoundary( aMapOfBoundary, aSeqOfBoundary,
                        aDMapLinkFace, theNonManifold, aNextFace );
        isToReset = true;
      }
    }
    isDone = !isToReset;
  }

  return !theResFaces.IsEmpty();
}

bool ManifoldPart::isInPlane( const SMDS_MeshFace* theFace1,
                              const SMDS_MeshFace* theFace2 )
{
  gp_Dir aNorm1 = gp_Dir( getNormale( theFace1 ) );
  gp_XYZ aNorm2XYZ = getNormale( theFace2 );
  if ( aNorm2XYZ.SquareModulus() <= gp::Resolution() )
  {
    myMapBadGeomIds.Add( theFace2->GetID() );
    return false;
  }
  if ( aNorm1.IsParallel( gp_Dir( aNorm2XYZ ), myAngToler ) )
    return true;

  return false;
}

void ManifoldPart::expandBoundary
                   ( ManifoldPart::TMapOfLink&            theMapOfBoundary,
                     ManifoldPart::TVectorOfLink&         theSeqOfBoundary,
                     ManifoldPart::TDataMapOfLinkFacePtr& theDMapLinkFacePtr,
                     ManifoldPart::TMapOfLink&            theNonManifold,
                     SMDS_MeshFace*                       theNextFace ) const
{
  ManifoldPart::TVectorOfLink aLinks;
  getLinks( theNextFace, aLinks );
  int aNbLink = (int)aLinks.size();
  for ( int i = 0; i < aNbLink; i++ )
  {
    ManifoldPart::Link aLink = aLinks[ i ];
    if ( myIsOnlyManifold && (theNonManifold.find( aLink ) != theNonManifold.end()) )
      continue;
    if ( theMapOfBoundary.find( aLink ) != theMapOfBoundary.end() )
    {
      if ( myIsOnlyManifold )
      {
        // remove from boundary
        theMapOfBoundary.erase( aLink );
        ManifoldPart::TVectorOfLink::iterator pLink = theSeqOfBoundary.begin();
        for ( ; pLink != theSeqOfBoundary.end(); ++pLink )
        {
          ManifoldPart::Link aBoundLink = *pLink;
          if ( aBoundLink.IsEqual( aLink ) )
          {
            theSeqOfBoundary.erase( pLink );
            break;
          }
        }
      }
    }
    else
    {
      theMapOfBoundary.insert( aLink );
      theSeqOfBoundary.push_back( aLink );
      theDMapLinkFacePtr[ aLink ] = theNextFace;
    }
  }
}

void ManifoldPart::getFacesByLink( const ManifoldPart::Link& theLink,
                                   ManifoldPart::TVectorOfFacePtr& theFaces ) const
{
  SMDS_Mesh::SetOfFaces aSetOfFaces;
  // take all faces that shared first node
  SMDS_ElemIteratorPtr anItr = theLink.myNode1->facesIterator();
  for ( ; anItr->more(); )
  {
    SMDS_MeshFace* aFace = (SMDS_MeshFace*)anItr->next();
    if ( !aFace )
      continue;
    aSetOfFaces.Add( aFace );
  }
  // take all faces that shared second node
  anItr = theLink.myNode2->facesIterator();
  // find the common part of two sets
  for ( ; anItr->more(); )
  {
    SMDS_MeshFace* aFace = (SMDS_MeshFace*)anItr->next();
    if ( aSetOfFaces.Contains( aFace ) )
      theFaces.push_back( aFace );
  }
}


/*
   ElementsOnSurface
*/

ElementsOnSurface::ElementsOnSurface()
{
  myMesh = 0;
  myIds.Clear();
  myType = SMDSAbs_All;
  mySurf.Nullify();
  myToler = Precision::Confusion();
  myUseBoundaries = false;
}

ElementsOnSurface::~ElementsOnSurface()
{
  myMesh = 0;
}

void ElementsOnSurface::SetMesh( const SMDS_Mesh* theMesh )
{
  if ( myMesh == theMesh )
    return;
  myMesh = theMesh;
  process();
}

bool ElementsOnSurface::IsSatisfy( long theElementId )
{
  return myIds.Contains( theElementId );
}

SMDSAbs_ElementType ElementsOnSurface::GetType() const
{ return myType; }

void ElementsOnSurface::SetTolerance( const double theToler )
{
  if ( myToler != theToler )
    myIds.Clear();
  myToler = theToler;
}

double ElementsOnSurface::GetTolerance() const
{ return myToler; }

void ElementsOnSurface::SetUseBoundaries( bool theUse )
{
  if ( myUseBoundaries != theUse ) {
    myUseBoundaries = theUse;
    SetSurface( mySurf, myType );
  }
}

void ElementsOnSurface::SetSurface( const TopoDS_Shape& theShape,
                                    const SMDSAbs_ElementType theType )
{
  myIds.Clear();
  myType = theType;
  mySurf.Nullify();
  if ( theShape.IsNull() || theShape.ShapeType() != TopAbs_FACE )
    return;
  mySurf = TopoDS::Face( theShape );
  BRepAdaptor_Surface SA( mySurf, myUseBoundaries );
  Standard_Real
    u1 = SA.FirstUParameter(),
    u2 = SA.LastUParameter(),
    v1 = SA.FirstVParameter(),
    v2 = SA.LastVParameter();
  Handle(Geom_Surface) surf = BRep_Tool::Surface( mySurf );
  myProjector.Init( surf, u1,u2, v1,v2 );
  process();
}

void ElementsOnSurface::process()
{
  myIds.Clear();
  if ( mySurf.IsNull() )
    return;

  if ( myMesh == 0 )
    return;

  if ( myType == SMDSAbs_Face || myType == SMDSAbs_All )
  {
    myIds.ReSize( myMesh->NbFaces() );
    SMDS_FaceIteratorPtr anIter = myMesh->facesIterator();
    for(; anIter->more(); )
      process( anIter->next() );
  }

  if ( myType == SMDSAbs_Edge || myType == SMDSAbs_All )
  {
    myIds.ReSize( myIds.Extent() + myMesh->NbEdges() );
    SMDS_EdgeIteratorPtr anIter = myMesh->edgesIterator();
    for(; anIter->more(); )
      process( anIter->next() );
  }

  if ( myType == SMDSAbs_Node )
  {
    myIds.ReSize( myMesh->NbNodes() );
    SMDS_NodeIteratorPtr anIter = myMesh->nodesIterator();
    for(; anIter->more(); )
      process( anIter->next() );
  }
}

void ElementsOnSurface::process( const SMDS_MeshElement* theElemPtr )
{
  SMDS_ElemIteratorPtr aNodeItr = theElemPtr->nodesIterator();
  bool isSatisfy = true;
  for ( ; aNodeItr->more(); )
  {
    SMDS_MeshNode* aNode = (SMDS_MeshNode*)aNodeItr->next();
    if ( !isOnSurface( aNode ) )
    {
      isSatisfy = false;
      break;
    }
  }
  if ( isSatisfy )
    myIds.Add( theElemPtr->GetID() );
}

bool ElementsOnSurface::isOnSurface( const SMDS_MeshNode* theNode )
{
  if ( mySurf.IsNull() )
    return false;

  gp_Pnt aPnt( theNode->X(), theNode->Y(), theNode->Z() );
  //  double aToler2 = myToler * myToler;
//   if ( mySurf->IsKind(STANDARD_TYPE(Geom_Plane)))
//   {
//     gp_Pln aPln = Handle(Geom_Plane)::DownCast(mySurf)->Pln();
//     if ( aPln.SquareDistance( aPnt ) > aToler2 )
//       return false;
//   }
//   else if ( mySurf->IsKind(STANDARD_TYPE(Geom_CylindricalSurface)))
//   {
//     gp_Cylinder aCyl = Handle(Geom_CylindricalSurface)::DownCast(mySurf)->Cylinder();
//     double aRad = aCyl.Radius();
//     gp_Ax3 anAxis = aCyl.Position();
//     gp_XYZ aLoc = aCyl.Location().XYZ();
//     double aXDist = anAxis.XDirection().XYZ() * ( aPnt.XYZ() - aLoc );
//     double aYDist = anAxis.YDirection().XYZ() * ( aPnt.XYZ() - aLoc );
//     if ( fabs(aXDist*aXDist + aYDist*aYDist - aRad*aRad) > aToler2 )
//       return false;
//   }
//   else
//     return false;
  myProjector.Perform( aPnt );
  bool isOn = ( myProjector.IsDone() && myProjector.LowerDistance() <= myToler );

  return isOn;
}


/*
  ElementsOnShape
*/

ElementsOnShape::ElementsOnShape()
  : myMesh(0),
    myType(SMDSAbs_All),
    myToler(Precision::Confusion()),
    myAllNodesFlag(false)
{
  myCurShapeType = TopAbs_SHAPE;
}

ElementsOnShape::~ElementsOnShape()
{
}

void ElementsOnShape::SetMesh (const SMDS_Mesh* theMesh)
{
  if (myMesh != theMesh) {
    myMesh = theMesh;
    SetShape(myShape, myType);
  }
}

bool ElementsOnShape::IsSatisfy (long theElementId)
{
  return myIds.Contains(theElementId);
}

SMDSAbs_ElementType ElementsOnShape::GetType() const
{
  return myType;
}

void ElementsOnShape::SetTolerance (const double theToler)
{
  if (myToler != theToler) {
    myToler = theToler;
    SetShape(myShape, myType);
  }
}

double ElementsOnShape::GetTolerance() const
{
  return myToler;
}

void ElementsOnShape::SetAllNodes (bool theAllNodes)
{
  if (myAllNodesFlag != theAllNodes) {
    myAllNodesFlag = theAllNodes;
    SetShape(myShape, myType);
  }
}

void ElementsOnShape::SetShape (const TopoDS_Shape&       theShape,
                                const SMDSAbs_ElementType theType)
{
  myType = theType;
  myShape = theShape;
  myIds.Clear();

  if (myMesh == 0) return;

  switch (myType)
  {
  case SMDSAbs_All:
    myIds.ReSize(myMesh->NbEdges() + myMesh->NbFaces() + myMesh->NbVolumes());
    break;
  case SMDSAbs_Node:
    myIds.ReSize(myMesh->NbNodes());
    break;
  case SMDSAbs_Edge:
    myIds.ReSize(myMesh->NbEdges());
    break;
  case SMDSAbs_Face:
    myIds.ReSize(myMesh->NbFaces());
    break;
  case SMDSAbs_Volume:
    myIds.ReSize(myMesh->NbVolumes());
    break;
  default:
    break;
  }

  myShapesMap.Clear();
  addShape(myShape);
}

void ElementsOnShape::addShape (const TopoDS_Shape& theShape)
{
  if (theShape.IsNull() || myMesh == 0)
    return;

  if (!myShapesMap.Add(theShape)) return;

  myCurShapeType = theShape.ShapeType();
  switch (myCurShapeType)
  {
  case TopAbs_COMPOUND:
  case TopAbs_COMPSOLID:
  case TopAbs_SHELL:
  case TopAbs_WIRE:
    {
      TopoDS_Iterator anIt (theShape, Standard_True, Standard_True);
      for (; anIt.More(); anIt.Next()) addShape(anIt.Value());
    }
    break;
  case TopAbs_SOLID:
    {
      myCurSC.Load(theShape);
      process();
    }
    break;
  case TopAbs_FACE:
    {
      TopoDS_Face aFace = TopoDS::Face(theShape);
      BRepAdaptor_Surface SA (aFace, true);
      Standard_Real
        u1 = SA.FirstUParameter(),
        u2 = SA.LastUParameter(),
        v1 = SA.FirstVParameter(),
        v2 = SA.LastVParameter();
      Handle(Geom_Surface) surf = BRep_Tool::Surface(aFace);
      myCurProjFace.Init(surf, u1,u2, v1,v2);
      myCurFace = aFace;
      process();
    }
    break;
  case TopAbs_EDGE:
    {
      TopoDS_Edge anEdge = TopoDS::Edge(theShape);
      Standard_Real u1, u2;
      Handle(Geom_Curve) curve = BRep_Tool::Curve(anEdge, u1, u2);
      myCurProjEdge.Init(curve, u1, u2);
      process();
    }
    break;
  case TopAbs_VERTEX:
    {
      TopoDS_Vertex aV = TopoDS::Vertex(theShape);
      myCurPnt = BRep_Tool::Pnt(aV);
      process();
    }
    break;
  default:
    break;
  }
}

void ElementsOnShape::process()
{
  if (myShape.IsNull() || myMesh == 0)
    return;

  if (myType == SMDSAbs_Node)
  {
    SMDS_NodeIteratorPtr anIter = myMesh->nodesIterator();
    while (anIter->more())
      process(anIter->next());
  }
  else
  {
    if (myType == SMDSAbs_Edge || myType == SMDSAbs_All)
    {
      SMDS_EdgeIteratorPtr anIter = myMesh->edgesIterator();
      while (anIter->more())
        process(anIter->next());
    }

    if (myType == SMDSAbs_Face || myType == SMDSAbs_All)
    {
      SMDS_FaceIteratorPtr anIter = myMesh->facesIterator();
      while (anIter->more()) {
        process(anIter->next());
      }
    }

    if (myType == SMDSAbs_Volume || myType == SMDSAbs_All)
    {
      SMDS_VolumeIteratorPtr anIter = myMesh->volumesIterator();
      while (anIter->more())
        process(anIter->next());
    }
  }
}

void ElementsOnShape::process (const SMDS_MeshElement* theElemPtr)
{
  if (myShape.IsNull())
    return;

  SMDS_ElemIteratorPtr aNodeItr = theElemPtr->nodesIterator();
  bool isSatisfy = myAllNodesFlag;

  gp_XYZ centerXYZ (0, 0, 0);

  while (aNodeItr->more() && (isSatisfy == myAllNodesFlag))
  {
    SMDS_MeshNode* aNode = (SMDS_MeshNode*)aNodeItr->next();
    gp_Pnt aPnt (aNode->X(), aNode->Y(), aNode->Z());
    centerXYZ += aPnt.XYZ();

    switch (myCurShapeType)
    {
    case TopAbs_SOLID:
      {
        myCurSC.Perform(aPnt, myToler);
        isSatisfy = (myCurSC.State() == TopAbs_IN || myCurSC.State() == TopAbs_ON);
      }
      break;
    case TopAbs_FACE:
      {
        myCurProjFace.Perform(aPnt);
        isSatisfy = (myCurProjFace.IsDone() && myCurProjFace.LowerDistance() <= myToler);
        if (isSatisfy)
        {
          // check relatively the face
          Quantity_Parameter u, v;
          myCurProjFace.LowerDistanceParameters(u, v);
          gp_Pnt2d aProjPnt (u, v);
          BRepClass_FaceClassifier aClsf (myCurFace, aProjPnt, myToler);
          isSatisfy = (aClsf.State() == TopAbs_IN || aClsf.State() == TopAbs_ON);
        }
      }
      break;
    case TopAbs_EDGE:
      {
        myCurProjEdge.Perform(aPnt);
        isSatisfy = (myCurProjEdge.NbPoints() > 0 && myCurProjEdge.LowerDistance() <= myToler);
      }
      break;
    case TopAbs_VERTEX:
      {
        isSatisfy = (aPnt.Distance(myCurPnt) <= myToler);
      }
      break;
    default:
      {
        isSatisfy = false;
      }
    }
  }

  if (isSatisfy && myCurShapeType == TopAbs_SOLID) { // Check the center point for volumes MantisBug 0020168
    centerXYZ /= theElemPtr->NbNodes();
    gp_Pnt aCenterPnt (centerXYZ);
    myCurSC.Perform(aCenterPnt, myToler);
    if ( !(myCurSC.State() == TopAbs_IN || myCurSC.State() == TopAbs_ON))
      isSatisfy = false;
  }

  if (isSatisfy)
    myIds.Add(theElemPtr->GetID());
}

TSequenceOfXYZ::TSequenceOfXYZ()
{}

TSequenceOfXYZ::TSequenceOfXYZ(size_type n) : std::vector<gp_XYZ>(n)
{}

TSequenceOfXYZ::TSequenceOfXYZ(size_type n, const value_type& t) : std::vector<gp_XYZ>(n,t)
{}

TSequenceOfXYZ::TSequenceOfXYZ(const TSequenceOfXYZ& theSequenceOfXYZ) : std::vector<gp_XYZ>(theSequenceOfXYZ)
{}

template <class InputIterator>
TSequenceOfXYZ::TSequenceOfXYZ(InputIterator theBegin, InputIterator theEnd): std::vector<gp_XYZ>(theBegin,theEnd)
{}

TSequenceOfXYZ& TSequenceOfXYZ::operator=(const TSequenceOfXYZ& theSequenceOfXYZ)
{
  std::vector<gp_XYZ>::operator=(theSequenceOfXYZ);
  return *this;
}

std::vector<gp_XYZ>::reference TSequenceOfXYZ::operator()(size_type n)
{
  return std::vector<gp_XYZ>::operator[](n-1);
}

std::vector<gp_XYZ>::const_reference TSequenceOfXYZ::operator()(size_type n) const
{
  return std::vector<gp_XYZ>::operator[](n-1);
}