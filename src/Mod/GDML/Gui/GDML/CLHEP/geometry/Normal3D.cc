// -*- C++ -*-
// $Id: Normal3D.cc,v 1.3 2003/08/13 20:00:11 garren Exp $
// ---------------------------------------------------------------------------

///#include "CLHEP/Geometry/defs.h"
///#include "CLHEP/Geometry/Normal3D.h"
///#include "CLHEP/Geometry/Transform3D.h"
#include "./defs.h"
#include "./Normal3D.h"
#include "./Transform3D.h"

namespace HepGeom {
  //--------------------------------------------------------------------------
  Normal3D<float> &
  Normal3D<float>::transform(const Transform3D & m) {
    double vx = x(),    vy = y(),    vz = z();
    double xx = m.xx(), xy = m.xy(), xz = m.xz();
    double yx = m.yx(), yy = m.yy(), yz = m.yz();
    double zx = m.zx(), zy = m.zy(), zz = m.zz();
    set((yy*zz-yz*zy)*vx+(yz*zx-yx*zz)*vy+(yx*zy-yy*zx)*vz,
	(zy*xz-zz*xy)*vx+(zz*xx-zx*xz)*vy+(zx*xy-zy*xx)*vz,
	(xy*yz-xz*yy)*vx+(xz*yx-xx*yz)*vy+(xx*yy-xy*yx)*vz);
    return *this;
  }

  //--------------------------------------------------------------------------
  Normal3D<float>
  operator*(const Transform3D & m, const Normal3D<float> & v) {
    double vx = v.x(),  vy = v.y(),  vz = v.z();
    double xx = m.xx(), xy = m.xy(), xz = m.xz();
    double yx = m.yx(), yy = m.yy(), yz = m.yz();
    double zx = m.zx(), zy = m.zy(), zz = m.zz();
    return Normal3D<float>
      ((yy*zz-yz*zy)*vx+(yz*zx-yx*zz)*vy+(yx*zy-yy*zx)*vz,
       (zy*xz-zz*xy)*vx+(zz*xx-zx*xz)*vy+(zx*xy-zy*xx)*vz,
       (xy*yz-xz*yy)*vx+(xz*yx-xx*yz)*vy+(xx*yy-xy*yx)*vz);
  }

  //--------------------------------------------------------------------------
  Normal3D<double> &
  Normal3D<double>::transform(const Transform3D & m) {
    double vx = x(),    vy = y(),    vz = z();
    double xx = m.xx(), xy = m.xy(), xz = m.xz();
    double yx = m.yx(), yy = m.yy(), yz = m.yz();
    double zx = m.zx(), zy = m.zy(), zz = m.zz();
    set((yy*zz-yz*zy)*vx+(yz*zx-yx*zz)*vy+(yx*zy-yy*zx)*vz,
	(zy*xz-zz*xy)*vx+(zz*xx-zx*xz)*vy+(zx*xy-zy*xx)*vz,
	(xy*yz-xz*yy)*vx+(xz*yx-xx*yz)*vy+(xx*yy-xy*yx)*vz);
    return *this;
  }

  //--------------------------------------------------------------------------
  Normal3D<double>
  operator*(const Transform3D & m, const Normal3D<double> & v) {
    double vx = v.x(),  vy = v.y(),  vz = v.z();
    double xx = m.xx(), xy = m.xy(), xz = m.xz();
    double yx = m.yx(), yy = m.yy(), yz = m.yz();
    double zx = m.zx(), zy = m.zy(), zz = m.zz();
    return Normal3D<double>
      ((yy*zz-yz*zy)*vx+(yz*zx-yx*zz)*vy+(yx*zy-yy*zx)*vz,
       (zy*xz-zz*xy)*vx+(zz*xx-zx*xz)*vy+(zx*xy-zy*xx)*vz,
       (xy*yz-xz*yy)*vx+(xz*yx-xx*yz)*vy+(xx*yy-xy*yx)*vz);
  }
} /* namespace HepGeom */
