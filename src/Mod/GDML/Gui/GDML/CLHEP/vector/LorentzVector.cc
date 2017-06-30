// -*- C++ -*-
// $Id: LorentzVector.cc,v 1.2 2003/08/13 20:00:14 garren Exp $
// ---------------------------------------------------------------------------
//
// This file is a part of the CLHEP - a Class Library for High Energy Physics.
//
// This is the implementation of that portion of the HepLorentzVector class
// which was in the original CLHEP and which does not force loading of either
// Rotation.cc or LorentzRotation.cc
//

#ifdef GNUPRAGMA
#pragma implementation
#endif

///#include "CLHEP/Vector/defs.h"
///#include "CLHEP/Vector/LorentzVector.h"
///#include "CLHEP/Vector/ZMxpv.h"
#include "../vector/defs.h"
#include "../vector/LorentzVector.h"
#include "../vector/ZMxpv.h"

#include <iostream>

namespace CLHEP  {

double HepLorentzVector::operator () (int i) const {
  switch(i) {
  case X:
  case Y:
  case Z:
    return pp(i);
  case T:
    return e();
  default:
    std::cerr << "HepLorentzVector subscripting: bad index (" << i << ")"
		 << std::endl;
  }
  return 0.;
}  

double & HepLorentzVector::operator () (int i) {
  static double dummy;
  switch(i) {
  case X:
  case Y:
  case Z:
    return pp(i);
  case T:
    return ee;
  default:
    std::cerr
      << "HepLorentzVector subscripting: bad index (" << i << ")"
      << std::endl;
    return dummy;
  }
}

HepLorentzVector & HepLorentzVector::boost
				(double bx, double by, double bz){
  double b2 = bx*bx + by*by + bz*bz;
  register double gamma = 1.0 / sqrt(1.0 - b2);
  register double bp = bx*x() + by*y() + bz*z();
  register double gamma2 = b2 > 0 ? (gamma - 1.0)/b2 : 0.0;

  setX(x() + gamma2*bp*bx + gamma*bx*t());
  setY(y() + gamma2*bp*by + gamma*by*t());
  setZ(z() + gamma2*bp*bz + gamma*bz*t());
  setT(gamma*(t() + bp));
  return *this;
}

HepLorentzVector & HepLorentzVector::rotateX(double a) {
  pp.rotateX(a); 
  return *this; 
}
HepLorentzVector & HepLorentzVector::rotateY(double a) { 
  pp.rotateY(a); 
  return *this; 
}
HepLorentzVector & HepLorentzVector::rotateZ(double a) { 
  pp.rotateZ(a); 
  return *this; 
}

HepLorentzVector & HepLorentzVector::rotateUz(const Hep3Vector &v) {
  pp.rotateUz(v);
  return *this;
}

std::ostream & operator<< (std::ostream & os, const HepLorentzVector & v)
{
  return os << "(" << v.x() << "," << v.y() << "," << v.z()
	    << ";" << v.t() << ")";
}

std::istream & operator>> (std::istream & is, HepLorentzVector & v) {

// Required format is ( a, b, c; d ) that is, four numbers, preceded by
// (, followed by ), components of the spatial vector separated by commas,
// time component separated by semicolon. The four numbers are taken
// as x, y, z, t.

  double x, y, z, t;
  char c;

  is >> std::ws >> c;
    // ws is defined to invoke eatwhite(istream & )
    // see (Stroustrup gray book) page 333 and 345.
  if (is.fail() || c != '(' ) {
    std::cerr << "Could not find required opening parenthesis "
	      << "in input of a HepLorentzVector" << std::endl;
    return is;
  }

  is >> x >> std::ws >> c;
  if (is.fail() || c != ',' ) {
    std::cerr << "Could not find x value and required trailing comma "
	      << "in input of a HepLorentzVector" << std::endl; 
    return is;
  }

  is >> y >> std::ws >> c;
  if (is.fail() || c != ',' ) {
    std::cerr << "Could not find y value and required trailing comma "
              <<  "in input of a HepLorentzVector" << std::endl;
    return is;
  }

  is >> z >> std::ws >> c;
  if (is.fail() || c != ';' ) {
    std::cerr << "Could not find z value and required trailing semicolon "
		 <<  "in input of a HepLorentzVector" << std::endl;
    return is;
  }

  is >> t >> std::ws >> c;
  if (is.fail() || c != ')' ) {
    std::cerr << "Could not find t value and required close parenthesis "
		 << "in input of a HepLorentzVector" << std::endl;
    return is;
  }

  v.setX(x);
  v.setY(y);
  v.setZ(z);
  v.setT(t);
  return is;
}

// The following were added when ZOOM classes were merged in:

HepLorentzVector & HepLorentzVector::operator /= (double c) {
  if (c == 0) {
    ZMthrowA (ZMxpvInfiniteVector(
      "Attempt to do LorentzVector /= 0 -- \n"
      "division by zero would produce infinite or NAN components"));
  }
  double oneOverC = 1.0/c;
  pp *= oneOverC;
  ee *= oneOverC;
  return *this;
} /* w /= c */

HepLorentzVector operator / (const HepLorentzVector & w, double c) {
if (c == 0) {
    ZMthrowA (ZMxpvInfiniteVector(
      "Attempt to do LorentzVector / 0 -- \n"
      "division by zero would produce infinite or NAN components"));
  }
  double oneOverC = 1.0/c;
  return HepLorentzVector (w.getV() * oneOverC,
                        w.getT() * oneOverC);
} /* LV = w / c */

Hep3Vector HepLorentzVector::boostVector() const {
  if (ee == 0) {
    if (pp.mag2() == 0) {
      return Hep3Vector(0,0,0);
    } else {
    ZMthrowA (ZMxpvInfiniteVector(
      "boostVector computed for LorentzVector with t=0 -- infinite result"));
    return pp/ee;
    }
  }
  if (restMass2() <= 0) {
    ZMthrowC (ZMxpvTachyonic(
      "boostVector computed for a non-timelike LorentzVector "));
        // result will make analytic sense but is physically meaningless
  }
  return pp * (1./ee);
} /* boostVector */


HepLorentzVector & HepLorentzVector::boostX (double beta){
  register double b2 = beta*beta;
  if (b2 >= 1) {
    ZMthrowA (ZMxpvTachyonic(
      "boost along X with beta >= 1 (speed of light) -- no boost done"));
  } else {
    register double gamma = sqrt(1./(1-b2));
    register double tt = ee;
    ee = gamma*(ee + beta*pp.getX());
    pp.setX(gamma*(pp.getX() + beta*tt));
  }
  return *this;
} /* boostX */

HepLorentzVector & HepLorentzVector::boostY (double beta){
  register double b2 = beta*beta;
  if (b2 >= 1) {
    ZMthrowA (ZMxpvTachyonic(
      "boost along Y with beta >= 1 (speed of light) -- \nno boost done"));
  } else {
    register double gamma = sqrt(1./(1-b2));
    register double tt = ee;
    ee = gamma*(ee + beta*pp.getY());
    pp.setY(gamma*(pp.getY() + beta*tt));
  }
  return *this;
} /* boostY */

HepLorentzVector & HepLorentzVector::boostZ (double beta){
  register double b2 = beta*beta;
  if (b2 >= 1) {
    ZMthrowA (ZMxpvTachyonic(
      "boost along Z with beta >= 1 (speed of light) -- \nno boost done"));
  } else {
    register double gamma = sqrt(1./(1-b2));
    register double tt = ee;
    ee = gamma*(ee + beta*pp.getZ());
    pp.setZ(gamma*(pp.getZ() + beta*tt));
  }
  return *this;
} /* boostZ */

double HepLorentzVector::setTolerance ( double tol ) {
// Set the tolerance for two LorentzVectors to be considered near each other
  double oldTolerance (tolerance);
  tolerance = tol;
  return oldTolerance;
}

double HepLorentzVector::getTolerance ( ) {
// Get the tolerance for two LorentzVectors to be considered near each other
  return tolerance;
}

double HepLorentzVector::tolerance = 
				Hep3Vector::ToleranceTicks * 2.22045e-16;
double HepLorentzVector::metric = 1.0;


}  // namespace CLHEP
