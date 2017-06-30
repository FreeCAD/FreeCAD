// -*- C++ -*-
// ---------------------------------------------------------------------------
//
// This file is a part of the CLHEP - a Class Library for High Energy Physics.
//
// This is the implementation of methods of the HepRotationZ class which
// were introduced when ZOOM PhysicsVectors was merged in.
//

#ifdef GNUPRAGMA
#pragma implementation
#endif

///#include "CLHEP/Vector/defs.h"
///#include "CLHEP/Vector/RotationZ.h"
///#include "CLHEP/Vector/AxisAngle.h"
///#include "CLHEP/Vector/EulerAngles.h"
///#include "CLHEP/Vector/LorentzRotation.h"
///#include "CLHEP/Units/PhysicalConstants.h"
#include "../vector/defs.h"
#include "../vector/RotationZ.h"
#include "../vector/AxisAngle.h"
#include "../vector/EulerAngles.h"
#include "../vector/LorentzRotation.h"
#include "../units/PhysicalConstants.h"

#include <cmath>
#include <stdlib.h>
#include <iostream>

using std::abs;

namespace CLHEP  {

static inline double safe_acos (double x) {
  if (abs(x) <= 1.0) return acos(x);
  return ( (x>0) ? 0 : CLHEP::pi );
}

HepRotationZ::HepRotationZ(double delta) : 
		d(proper(delta)), s(sin(delta)), c(cos(delta))
{}

HepRotationZ & HepRotationZ::set ( double delta ) {
  d = proper(delta);
  s = sin(d);
  c = cos(d);
  return *this;
}

double  HepRotationZ::phi() const {
  return  - d/2.0;
}  // HepRotationZ::phi()

double  HepRotationZ::theta() const {
  return  0.0 ;
}  // HepRotationZ::theta()

double  HepRotationZ::psi() const {
  return  - d/2.0;
}  // HepRotationZ::psi()

HepEulerAngles HepRotationZ::eulerAngles() const {
  return HepEulerAngles(  phi(),  theta(),  psi() );
}  // HepRotationZ::eulerAngles()


// From the defining code in the implementation of CLHEP (in Rotation.cc)
// it is clear that thetaX, phiX form the polar angles in the original
// coordinate system of the new X axis (and similarly for phiY and phiZ).
//
// This code is take directly from CLHEP original.  However, there are as
// shown opportunities for significant speed improvement.

double HepRotationZ::phiX() const {
  return (yx() == 0.0 && xx() == 0.0) ? 0.0 : atan2(yx(),xx());
  		// or ---- return d;
}

double HepRotationZ::phiY() const {
  return (yy() == 0.0 && xy() == 0.0) ? 0.0 : atan2(yy(),xy());
}

double HepRotationZ::phiZ() const {
  return (yz() == 0.0 && xz() == 0.0) ? 0.0 : atan2(yz(),xz());
		// or ---- return 0.0;
}

double HepRotationZ::thetaX() const {
  return safe_acos(zx());
		// or ----  return CLHEP::halfpi;
}

double HepRotationZ::thetaY() const {
  return safe_acos(zy());
		// or ----  return CLHEP::halfpi;
}

double HepRotationZ::thetaZ() const {
  return safe_acos(zz());  
		// or ---- return 0.0;
}

void HepRotationZ::setDelta ( double delta ) {
  set(delta);
}

void HepRotationZ::decompose
	(HepAxisAngle & rotation, Hep3Vector & boost) const {
  boost.set(0,0,0);
  rotation = axisAngle();
}

void HepRotationZ::decompose
	(Hep3Vector & boost, HepAxisAngle & rotation) const {
  boost.set(0,0,0);
  rotation = axisAngle();
}
 /*
void HepRotationZ::decompose
        (HepRotation & rotation, HepBoost & boost) const {
  boost.set(0,0,0);
  rotation = HepRotation(*this);
}
                                                                              
void HepRotationZ::decompose
        (HepBoost & boost, HepRotation & rotation) const {
  boost.set(0,0,0);
  rotation = HepRotation(*this);
}
 */ 
double HepRotationZ::distance2( const HepRotationZ & r  ) const {
  double answer = 2.0 * ( 1.0 - ( s * r.s + c * r.c ) ) ;
  return (answer >= 0) ? answer : 0;
}

double HepRotationZ::distance2( const HepRotation & r  ) const {
  double sum =    xx() * r.xx() + xy() * r.xy()
                   + yx() * r.yx() + yy() * r.yy()
						   + r.zz();
  double answer = 3.0 - sum;
  return (answer >= 0 ) ? answer : 0;
}
/*
double HepRotationZ::distance2( const HepLorentzRotation & lt  ) const {
  HepAxisAngle a; 
  Hep3Vector   b;
  lt.decompose(b, a);
  double bet = b.beta();
  double bet2 = bet*bet;
  HepRotation r(a);
  return bet2/(1-bet2) + distance2(r);
}

double HepRotationZ::distance2( const HepBoost & lt ) const {
  return distance2( HepLorentzRotation(lt));
}
*/
double HepRotationZ::howNear( const HepRotationZ & r ) const {
  return sqrt(distance2(r));
}
double HepRotationZ::howNear( const HepRotation & r ) const {
  return sqrt(distance2(r));
}
/*double HepRotationZ::howNear( const HepBoost & lt ) const {
  return sqrt(distance2(lt));
}
double HepRotationZ::howNear( const HepLorentzRotation & lt ) const {
  return sqrt(distance2(lt));
}*/
bool HepRotationZ::isNear(const HepRotationZ & r,double epsilon)const {
  return (distance2(r) <= epsilon*epsilon);
}
bool HepRotationZ::isNear(const HepRotation & r,double epsilon)const {
  return (distance2(r) <= epsilon*epsilon);
}
/*bool HepRotationZ::isNear( const HepBoost & lt,double epsilon) const {
  return (distance2(lt) <= epsilon*epsilon);
}
bool HepRotationZ::isNear( const HepLorentzRotation & lt,
                                     double epsilon) const {
  return (distance2(lt) <= epsilon*epsilon);
}*/

double HepRotationZ::norm2() const {
  return 2.0 - 2.0 * c;
}

std::ostream & HepRotationZ::print( std::ostream & os ) const {
  os << "\nRotation about Z (" << d <<
                ") [cos d = " << c << " sin d = " << s << "]\n";
  return os;
}

}  // namespace CLHEP

