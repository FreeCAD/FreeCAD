// -*- C++ -*-
// $Id: Point3D.cc,v 1.3 2003/08/13 20:00:11 garren Exp $
// ---------------------------------------------------------------------------

///#include "CLHEP/Geometry/defs.h"
///#include "CLHEP/Geometry/Point3D.h"
///#include "CLHEP/Geometry/Transform3D.h"
#include "./defs.h"
#include "./Point3D.h"
#include "./Transform3D.h"

namespace HepGeom {
  //--------------------------------------------------------------------------
  Point3D<float> &
  Point3D<float>::transform(const Transform3D & m) {
    double vx = x(), vy = y(), vz = z();
    set(m.xx()*vx + m.xy()*vy + m.xz()*vz + m.dx(),
	m.yx()*vx + m.yy()*vy + m.yz()*vz + m.dy(),
	m.zx()*vx + m.zy()*vy + m.zz()*vz + m.dz());
    return *this;
  }

  //--------------------------------------------------------------------------
  Point3D<float>
  operator*(const Transform3D & m, const Point3D<float> & v) {
    double vx = v.x(), vy = v.y(), vz = v.z();
    return Point3D<float>
      (m.xx()*vx + m.xy()*vy + m.xz()*vz + m.dx(),
       m.yx()*vx + m.yy()*vy + m.yz()*vz + m.dy(),
       m.zx()*vx + m.zy()*vy + m.zz()*vz + m.dz());
  }

  //--------------------------------------------------------------------------
  Point3D<double> &
  Point3D<double>::transform(const Transform3D & m) {
    double vx = x(), vy = y(), vz = z();
    set(m.xx()*vx + m.xy()*vy + m.xz()*vz + m.dx(),
	m.yx()*vx + m.yy()*vy + m.yz()*vz + m.dy(),
	m.zx()*vx + m.zy()*vy + m.zz()*vz + m.dz());
    return *this;
  }

  //--------------------------------------------------------------------------
  Point3D<double>
  operator*(const Transform3D & m, const Point3D<double> & v) {
    double vx = v.x(), vy = v.y(), vz = v.z();
    return Point3D<double>
      (m.xx()*vx + m.xy()*vy + m.xz()*vz + m.dx(),
       m.yx()*vx + m.yy()*vy + m.yz()*vz + m.dy(),
       m.zx()*vx + m.zy()*vy + m.zz()*vz + m.dz());
  }
} /* namespace HepGeom */
