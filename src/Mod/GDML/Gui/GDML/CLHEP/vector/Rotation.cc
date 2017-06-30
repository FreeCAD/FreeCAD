// -*- C++ -*-
// $Id: Rotation.cc,v 1.4 2003/08/13 20:00:14 garren Exp $
// ---------------------------------------------------------------------------
//
// This file is a part of the CLHEP - a Class Library for High Energy Physics.
//
// This is the implementation of the parts of the the HepRotation class which
// were present in the original CLHEP before the merge with ZOOM PhysicsVectors.
//

#ifdef GNUPRAGMA
#pragma implementation
#endif

///#include "CLHEP/Vector/defs.h"
///#include "CLHEP/Vector/Rotation.h"
///#include "CLHEP/Units/PhysicalConstants.h"
#include "../vector/defs.h"
#include "../vector/Rotation.h"
#include "../units/PhysicalConstants.h"

#include <iostream>
#include <cmath>

using std::abs;

namespace CLHEP  {

static inline double safe_acos (double x) {
  if (abs(x) <= 1.0) return acos(x);
  return ( (x>0) ? 0 : CLHEP::pi );
}

double HepRotation::operator() (int i, int j) const {
  if (i == 0) {
    if (j == 0) { return xx(); }
    if (j == 1) { return xy(); }
    if (j == 2) { return xz(); } 
  } else if (i == 1) {
    if (j == 0) { return yx(); }
    if (j == 1) { return yy(); }
    if (j == 2) { return yz(); } 
  } else if (i == 2) {
    if (j == 0) { return zx(); }
    if (j == 1) { return zy(); }
    if (j == 2) { return zz(); } 
  } 
  std::cerr << "HepRotation subscripting: bad indices "
       << "(" << i << "," << j << ")" << std::endl;
  return 0.0;
} 

HepRotation & HepRotation::rotate(double a, const Hep3Vector& axis) {
  if (a != 0.0) {
    double ll = axis.mag();
    if (ll == 0.0) {
      ZMthrowC (ZMxpvZeroVector("HepRotation: zero axis"));
    }else{
      double sa = sin(a), ca = cos(a);
      double dx = axis.x()/ll, dy = axis.y()/ll, dz = axis.z()/ll;   
      HepRotation m(
	ca+(1-ca)*dx*dx,          (1-ca)*dx*dy-sa*dz,    (1-ca)*dx*dz+sa*dy,
	   (1-ca)*dy*dx+sa*dz, ca+(1-ca)*dy*dy,          (1-ca)*dy*dz-sa*dx,
	   (1-ca)*dz*dx-sa*dy,    (1-ca)*dz*dy+sa*dx, ca+(1-ca)*dz*dz );
      transform(m);
    }
  }
  return *this;
}

HepRotation & HepRotation::rotateX(double a) {
  double c = cos(a);
  double s = sin(a);
  double x = ryx, y = ryy, z = ryz; 
  ryx = c*x - s*rzx;
  ryy = c*y - s*rzy;
  ryz = c*z - s*rzz;
  rzx = s*x + c*rzx;
  rzy = s*y + c*rzy;
  rzz = s*z + c*rzz;
  return *this;
}

HepRotation & HepRotation::rotateY(double a){
  double c = cos(a);
  double s = sin(a);
  double x = rzx, y = rzy, z = rzz; 
  rzx = c*x - s*rxx;
  rzy = c*y - s*rxy;
  rzz = c*z - s*rxz;
  rxx = s*x + c*rxx;
  rxy = s*y + c*rxy;
  rxz = s*z + c*rxz;
  return *this;
}

HepRotation & HepRotation::rotateZ(double a) {
  double c = cos(a);
  double s = sin(a);
  double x = rxx, y = rxy, z = rxz; 
  rxx = c*x - s*ryx;
  rxy = c*y - s*ryy;
  rxz = c*z - s*ryz;
  ryx = s*x + c*ryx;
  ryy = s*y + c*ryy;
  ryz = s*z + c*ryz;
  return *this;
}

HepRotation & HepRotation::rotateAxes(const Hep3Vector &newX,
				      const Hep3Vector &newY,
				      const Hep3Vector &newZ) {
  double del = 0.001;
  Hep3Vector w = newX.cross(newY);

  if (abs(newZ.x()-w.x()) > del ||
      abs(newZ.y()-w.y()) > del ||
      abs(newZ.z()-w.z()) > del ||
      abs(newX.mag2()-1.) > del ||
      abs(newY.mag2()-1.) > del || 
      abs(newZ.mag2()-1.) > del ||
      abs(newX.dot(newY)) > del ||
      abs(newY.dot(newZ)) > del ||
      abs(newZ.dot(newX)) > del) {
    std::cerr << "HepRotation::rotateAxes: bad axis vectors" << std::endl;
    return *this;
  }else{
    return transform(HepRotation(newX.x(), newY.x(), newZ.x(),
                                 newX.y(), newY.y(), newZ.y(),
                                 newX.z(), newY.z(), newZ.z()));
  }
}

double HepRotation::phiX() const {
  return (yx() == 0.0 && xx() == 0.0) ? 0.0 : std::atan2(yx(),xx());
}

double HepRotation::phiY() const {
  return (yy() == 0.0 && xy() == 0.0) ? 0.0 : std::atan2(yy(),xy());
}

double HepRotation::phiZ() const {
  return (yz() == 0.0 && xz() == 0.0) ? 0.0 : std::atan2(yz(),xz());
}

double HepRotation::thetaX() const {
  return safe_acos(zx());
}

double HepRotation::thetaY() const {
  return safe_acos(zy());
}

double HepRotation::thetaZ() const {
  return safe_acos(zz());
}

void HepRotation::getAngleAxis(double &angle, Hep3Vector &axis) const {
  double cosa  = 0.5*(xx()+yy()+zz()-1);
  double cosa1 = 1-cosa;
  if (cosa1 <= 0) {
    angle = 0;
    axis  = Hep3Vector(0,0,1);
  }else{
    double x=0, y=0, z=0;
    if (xx() > cosa) x = sqrt((xx()-cosa)/cosa1);
    if (yy() > cosa) y = sqrt((yy()-cosa)/cosa1);
    if (zz() > cosa) z = sqrt((zz()-cosa)/cosa1);
    if (zy() < yz()) x = -x;
    if (xz() < zx()) y = -y;
    if (yx() < xy()) z = -z;
    angle = (cosa < -1.) ? acos(-1.) : acos(cosa);
    axis  = Hep3Vector(x,y,z);
  }
}

bool HepRotation::isIdentity() const {
  return  (rxx == 1.0 && rxy == 0.0 && rxz == 0.0 &&
           ryx == 0.0 && ryy == 1.0 && ryz == 0.0 &&
           rzx == 0.0 && rzy == 0.0 && rzz == 1.0) ? true : false;
}

int HepRotation::compare ( const HepRotation & r ) const {
       if (rzz<r.rzz) return -1; else if (rzz>r.rzz) return 1;
  else if (rzy<r.rzy) return -1; else if (rzy>r.rzy) return 1;
  else if (rzx<r.rzx) return -1; else if (rzx>r.rzx) return 1;
  else if (ryz<r.ryz) return -1; else if (ryz>r.ryz) return 1;
  else if (ryy<r.ryy) return -1; else if (ryy>r.ryy) return 1;
  else if (ryx<r.ryx) return -1; else if (ryx>r.ryx) return 1;
  else if (rxz<r.rxz) return -1; else if (rxz>r.rxz) return 1;
  else if (rxy<r.rxy) return -1; else if (rxy>r.rxy) return 1;
  else if (rxx<r.rxx) return -1; else if (rxx>r.rxx) return 1;
  else return 0;
}


const HepRotation HepRotation::IDENTITY;

}  // namespace CLHEP


