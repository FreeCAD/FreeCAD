// -*- C++ -*-
// ---------------------------------------------------------------------------
//
// This file is a part of the CLHEP - a Class Library for High Energy Physics.
//
// This is the implementation of the Hep2Vector class.
//
//-------------------------------------------------------------

/*#include "CLHEP/Vector/defs.h"
#include "CLHEP/Vector/TwoVector.h"
#include "CLHEP/Vector/ZMxpv.h"
#include "CLHEP/Vector/ThreeVector.h"
*/
#include "defs.h"
#include "TwoVector.h"
#include "ZMxpv.h"
#include "ThreeVector.h"

#include <cmath>
#include <iostream>

namespace CLHEP  {

double Hep2Vector::tolerance = Hep2Vector::ZMpvToleranceTicks * 2.22045e-16;

double Hep2Vector::setTolerance (double tol) {
// Set the tolerance for Hep2Vectors to be considered near one another
  double oldTolerance (tolerance);
  tolerance = tol;
  return oldTolerance;
}

double Hep2Vector::operator () (int i) const {
  if (i == 0) {
    return x();
  }else if (i == 1) {
    return y();
  }else{
    ZMthrowA(ZMxpvIndexRange(
	"Hep2Vector::operator(): bad index"));
    return 0.0;
  }
}

double & Hep2Vector::operator () (int i) {
  static double dummy;
  switch(i) {
  case X:
    return dx;
  case Y:
    return dy;
  default:
    ZMthrowA (ZMxpvIndexRange(
	"Hep2Vector::operator() : bad index"));
    return dummy;
  }
}

void Hep2Vector::rotate(double angle) {
  double s = sin(angle);
  double c = cos(angle);
  double xx = dx;
  dx = c*xx - s*dy;
  dy = s*xx + c*dy;
}

Hep2Vector operator/ (const Hep2Vector & p, double a) {
  if (a==0) {
    ZMthrowA(ZMxpvInfiniteVector( "Division of Hep2Vector by zero"));
  }
  return Hep2Vector(p.x()/a, p.y()/a);
}

std::ostream & operator << (std::ostream & os, const Hep2Vector & q) {
  os << "(" << q.x() << ", " << q.y() << ")";
  return os;
}

///void ZMinput2doubles ( std::istream & is, const char * type,
///                       double & x, double & y );

std::istream & operator>>(std::istream & is, Hep2Vector & p) {
  double x, y;
///  ZMinput2doubles ( is, "Hep2Vector", x, y );
  p.set(x, y);
  return  is;
}  // operator>>()

Hep2Vector::operator Hep3Vector () const {
  return Hep3Vector ( dx, dy, 0.0 );
}

int Hep2Vector::compare (const Hep2Vector & v) const {
  if       ( dy > v.dy ) {
    return 1;
  } else if ( dy < v.dy ) {
    return -1;
  } else if ( dx > v.dx ) {
    return 1;
  } else if ( dx < v.dx ) {
    return -1;
  } else {
    return 0;
  }
} /* Compare */


bool Hep2Vector::operator > (const Hep2Vector & v) const {
	return (compare(v)  > 0);
}
bool Hep2Vector::operator < (const Hep2Vector & v) const {
	return (compare(v)  < 0);
}
bool Hep2Vector::operator>= (const Hep2Vector & v) const {
	return (compare(v) >= 0);
}
bool Hep2Vector::operator<= (const Hep2Vector & v) const {
	return (compare(v) <= 0);
}

bool Hep2Vector::isNear(const Hep2Vector & p, double epsilon) const {
  double limit = dot(p)*epsilon*epsilon;
  return ( (*this - p).mag2() <= limit );
} /* isNear() */

double Hep2Vector::howNear(const Hep2Vector & p ) const {
  double d   = (*this - p).mag2();
  double pdp = dot(p);
  if ( (pdp > 0) && (d < pdp)  ) {
    return sqrt (d/pdp);
  } else if ( (pdp == 0) && (d == 0) ) {
    return 0;
  } else {
    return 1;
  }
} /* howNear */

double Hep2Vector::howParallel (const Hep2Vector & v) const {
  // | V1 x V2 | / | V1 dot V2 |
  // Of course, the "cross product" is fictitious but the math is valid
  double v1v2 = fabs(dot(v));
  if ( v1v2 == 0 ) {
    // Zero is parallel to no other vector except for zero.
    return ( (mag2() == 0) && (v.mag2() == 0) ) ? 0 : 1;
  }
  double abscross = fabs ( dx * v.y() - dy - v.x() );
  if ( abscross >= v1v2 ) {
    return 1;
  } else {
    return abscross/v1v2;
  }
} /* howParallel() */

bool Hep2Vector::isParallel (const Hep2Vector & v,
			     double epsilon) const {
  // | V1 x V2 | <= epsilon * | V1 dot V2 | 
  // Of course, the "cross product" is fictitious but the math is valid
  double v1v2 = fabs(dot(v));
  if ( v1v2 == 0 ) {
    // Zero is parallel to no other vector except for zero.
    return ( (mag2() == 0) && (v.mag2() == 0) );
  }
  double abscross = fabs ( dx * v.y() - dy - v.x() );
  return ( abscross <= epsilon * v1v2 );
} /* isParallel() */

double Hep2Vector::howOrthogonal (const Hep2Vector & v) const {
  // | V1 dot V2 | / | V1 x V2 | 
  // Of course, the "cross product" is fictitious but the math is valid
  double v1v2 = fabs(dot(v));
  if ( v1v2 == 0 ) {
    return 0;	// Even if one or both are 0, they are considered orthogonal
  }
  double abscross = fabs ( dx * v.y() - dy - v.x() );
  if ( v1v2 >= abscross ) {
    return 1;
  } else {
    return v1v2/abscross;
  }
} /* howOrthogonal() */

bool Hep2Vector::isOrthogonal (const Hep2Vector & v,
			     double epsilon) const {
  // | V1 dot V2 | <= epsilon * | V1 x V2 | 
  // Of course, the "cross product" is fictitious but the math is valid
  double v1v2 = fabs(dot(v));
  double abscross = fabs ( dx * v.y() - dy - v.x() );
  return ( v1v2 <= epsilon * abscross );
} /* isOrthogonal() */

}  // namespace CLHEP
