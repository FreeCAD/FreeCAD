// -*- C++ -*-
// $Id: Vector3D.cc,v 1.3 2003/08/13 20:00:11 garren Exp $
// ---------------------------------------------------------------------------

///#include "CLHEP/Geometry/defs.h"
///#include "CLHEP/Geometry/Vector3D.h"
///#include "CLHEP/Geometry/Transform3D.h"
#include "./defs.h"
#include "./Vector3D.h"
#include "./Transform3D.h"

namespace HepGeom {
  //--------------------------------------------------------------------------
  Vector3D<float> &
  Vector3D<float>::transform(const Transform3D & m) {
    double vx = x(), vy = y(), vz = z();
    set(m.xx()*vx + m.xy()*vy + m.xz()*vz,
	m.yx()*vx + m.yy()*vy + m.yz()*vz,
	m.zx()*vx + m.zy()*vy + m.zz()*vz);
    return *this;
  }

  //--------------------------------------------------------------------------
  Vector3D<float>
  operator*(const Transform3D & m, const Vector3D<float> & v) {
    double vx = v.x(), vy = v.y(), vz = v.z();
    return Vector3D<float>
      (m.xx()*vx + m.xy()*vy + m.xz()*vz,
       m.yx()*vx + m.yy()*vy + m.yz()*vz,
       m.zx()*vx + m.zy()*vy + m.zz()*vz);
  }

  //--------------------------------------------------------------------------
  Vector3D<double> &
  Vector3D<double>::transform(const Transform3D & m) {
    double vx = x(), vy = y(), vz = z();
    set(m.xx()*vx + m.xy()*vy + m.xz()*vz,
	m.yx()*vx + m.yy()*vy + m.yz()*vz,
	m.zx()*vx + m.zy()*vy + m.zz()*vz);
    return *this;
  }

  //--------------------------------------------------------------------------
  Vector3D<double>
  operator*(const Transform3D & m, const Vector3D<double> & v) {
    double vx = v.x(), vy = v.y(), vz = v.z();
    return Vector3D<double>
      (m.xx()*vx + m.xy()*vy + m.xz()*vz,
       m.yx()*vx + m.yy()*vy + m.yz()*vz,
       m.zx()*vx + m.zy()*vy + m.zz()*vz);
  }
} /* namespace HepGeom */
