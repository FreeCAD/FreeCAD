// ----------------------------------------------------------------------
//
// EulerAngles.cc
//
// Methods for classes, and instances of globals, declared in EulerAngles.h
//
// History:
//
// 04-Dec-1997	MF	Stub with just PI
// 12-Jan-1998  WEB	PI now found in ZMutility; used ZMutility headers
//			where available
// 16-Mar-1998  WEB	Corrected ZMpvEulerAnglesRep
// 15-Jun-1998  WEB	Added namespace support
// 26-Jul-2000  MF	CLHEP version
// 12-Apr-2001  MF  	NaN-proofing
// 19-Nov-2001  MF  	Correction to ZMpvEulerAnglesRep, which was affecting
//			.isNear().  array[3] had been incorrect.
//			Note - the correct form was used in all other places 
//			including Rotation.set(phi, theta, psi).
//
// ----------------------------------------------------------------------


///#include "CLHEP/Vector/defs.h"
///#include "CLHEP/Vector/EulerAngles.h"
///#include "CLHEP/Vector/ThreeVector.h"
#include "../vector/defs.h"
#include "../vector/EulerAngles.h"
#include "../vector/ThreeVector.h"

#include <iostream>

namespace CLHEP  {

//-*************
// static consts
//-*************

double HepEulerAngles::tolerance = Hep3Vector::ToleranceTicks * 1.0e-8;

//-*******************
// measure of distance
//-*******************


static void ZMpvEulerAnglesRep ( const HepEulerAngles & ex, double array[] ) {

  register double sinPhi   = sin( ex.phi() )  , cosPhi   = cos( ex.phi() );
  register double sinTheta = sin( ex.theta() ), cosTheta = cos( ex.theta() );
  register double sinPsi   = sin( ex.psi() )  , cosPsi   = cos( ex.psi() );

  array[0] =   cosPsi * cosPhi   - sinPsi * cosTheta * sinPhi;
  array[1] =   cosPsi * sinPhi   + sinPsi * cosTheta * cosPhi;
  array[2] =   sinPsi * sinTheta;

  array[3] = - sinPsi * cosPhi - cosPsi * cosTheta * sinPhi;
  array[4] = - sinPsi * sinPhi   + cosPsi * cosTheta * cosPhi;
  array[5] =   cosPsi * sinTheta;

  array[6] =   sinTheta * sinPhi;
  array[7] = - sinTheta * cosPhi;
  array[8] =   cosTheta;

} // ZMpvEulerAnglesRep


double HepEulerAngles::distance( const EA & ex ) const  {

  double thisRep[9];
  double exRep[9];

  ZMpvEulerAnglesRep ( *this, thisRep );
  ZMpvEulerAnglesRep ( ex,    exRep );

  double sum = 0.0;
  for (int i = 0; i < 9; i++)  {
    sum += thisRep[i] * exRep[i];
  }

  double d = 3.0 - sum;		// NaN-proofing: 
  return  (d >= 0) ? d : 0;		// sqrt(distance) is used in howNear()

}  // HepEulerAngles::distance()


bool HepEulerAngles::isNear( const EA & ex, double epsilon ) const  {

  return  distance( ex ) <= epsilon*epsilon ;

}  // HepEulerAngles::isNear()


double HepEulerAngles::howNear( const EA & ex ) const  {

  return  sqrt( distance( ex ) );

}  // HepEulerAngles::howNear()

//-**************
// Global Methods
//-**************

std::ostream & operator<<(std::ostream & os, const HepEulerAngles & ea)
{
  os << "(" << ea.phi() << ", " << ea.theta() << ", " << ea.psi() << ")";
  return  os;
}  // operator<<()

///void ZMinput3doubles ( std::istream & is, const char * type,
///                      double & x, double & y, double & z );

std::istream & operator>>(std::istream & is, HepEulerAngles & ea) {
  double thePhi;
  double theTheta;
  double thePsi;
///  ZMinput3doubles ( is, "HepEulerAngle", thePhi , theTheta , thePsi );
  ea.set ( thePhi , theTheta , thePsi );
  return  is;
}  // operator>>()

}  // namespace CLHEP


