// ----------------------------------------------------------------------
//
// AxisAngle.cc
//
// History:
//   23-Jan-1998  WEB  Initial draft
//   13-Mar-1998  WEB  Corrected ZMpvAxisAngleRep
//   15-Jun-1998  WEB  Added namespace support
//   26-Jul-2000  MF  CLHEP version
//   12-Apr-2001  MF  NaN-proofing
//
// ----------------------------------------------------------------------

///#include "CLHEP/Vector/defs.h"
///#include "CLHEP/Vector/AxisAngle.h"
#include "../vector/defs.h"
#include "../vector/AxisAngle.h"

namespace CLHEP  {

double HepAxisAngle::tolerance = Hep3Vector::ToleranceTicks * 1.0e-08;

static void ZMpvAxisAngleRep( const HepAxisAngle & aa, double array[] ) {

  register double sinDelta = sin( aa.delta() );
  register double cosDelta = cos( aa.delta() );
  register double oneMinusCosDelta = 1.0 - cosDelta;

  register double uX = aa.getAxis().getX();
  register double uY = aa.getAxis().getY();
  register double uZ = aa.getAxis().getZ();

  array[0] = oneMinusCosDelta * uX * uX  +  cosDelta;
  array[1] = oneMinusCosDelta * uX * uY  -  sinDelta * uZ;
  array[2] = oneMinusCosDelta * uX * uZ  +  sinDelta * uY;

  array[3] = oneMinusCosDelta * uY * uX  +  sinDelta * uZ;
  array[4] = oneMinusCosDelta * uY * uY  +  cosDelta;
  array[5] = oneMinusCosDelta * uY * uZ  -  sinDelta * uX;

  array[6] = oneMinusCosDelta * uZ * uX  -  sinDelta * uY;
  array[7] = oneMinusCosDelta * uZ * uY  +  sinDelta * uX;
  array[8] = oneMinusCosDelta * uZ * uZ  +  cosDelta;

} // ZMpvAxisAngleRep


double HepAxisAngle::distance( const AA & aa ) const  {

  double thisRep[9];
  double aaRep[9];

  ZMpvAxisAngleRep( *this, thisRep );
  ZMpvAxisAngleRep( aa,    aaRep );

  double sum = 0.0;
  for ( int i = 0; i < 9; i++ )  {
    sum += thisRep[i] * aaRep[i];
  }

  double d = 3.0 - sum;		// NaN-proofing: 
  return  (d >= 0) ? d : 0;             // sqrt(distance) is used in howNear()

}  // HepAxisAngle::distance()


bool HepAxisAngle::isNear( const AA & aa, Scalar epsilon ) const  {

  return  distance( aa ) <= epsilon * epsilon;

}  // HepAxisAngle::isNear()


double HepAxisAngle::howNear( const AA & aa ) const  {

  return  sqrt( distance( aa ) );

}  // HepAxisAngle::howNear()


//-********************
//
// Global methods
//
//-********************


std::ostream & operator<<(std::ostream & os, const HepAxisAngle & aa) {
  os << '(' << aa.axis() << ", " << aa.delta() << ')';
  return  os;
}  // operator<<()


///void ZMinputAxisAngle ( std::istream & is, 
///			double & x, double & y, double & z, 
 ///                      	double & delta );

std::istream & operator>>(std::istream & is, HepAxisAngle & aa) {
  Hep3Vector axis;
  double delta;
  double x,y,z;
///  ZMinputAxisAngle ( is, x, y, z, delta );
  axis.set(x,y,z);
  aa.set ( axis, delta );
  return  is;
}  // operator>>()

}  // namespace CLHEP
