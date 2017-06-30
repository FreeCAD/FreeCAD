// -*- C++ -*-
// $Id: Transform3D.cc,v 1.6 2003/10/24 21:39:45 garren Exp $
// ---------------------------------------------------------------------------
//
// This file is a part of the CLHEP - a Class Library for High Energy Physics.
//
// Hep geometrical 3D Transformation library
//
// Author: Evgeni Chernyaev <Evgueni.Tcherniaev@cern.ch>
//
// History:
// 24.09.96 E.Chernyaev - initial version

#include <iostream>
#include <cmath>	// double abs()
#include <stdlib.h>	// int abs()
///#include "CLHEP/Geometry/defs.h"
///#include "CLHEP/Geometry/Transform3D.h"
#include "./defs.h"
#include "./Transform3D.h"

using std::abs;

namespace HepGeom {

  const Transform3D Transform3D::Identity = Transform3D ();

  //   T R A N S F O R M A T I O N -------------------------------------------

  double Transform3D::operator () (int i, int j) const {
    if (i == 0) {
      if (j == 0) { return xx_; }
      if (j == 1) { return xy_; }
      if (j == 2) { return xz_; } 
      if (j == 3) { return dx_; } 
    } else if (i == 1) {
      if (j == 0) { return yx_; }
      if (j == 1) { return yy_; }
      if (j == 2) { return yz_; } 
      if (j == 3) { return dy_; } 
    } else if (i == 2) {
      if (j == 0) { return zx_; }
      if (j == 1) { return zy_; }
      if (j == 2) { return zz_; } 
      if (j == 3) { return dz_; } 
    } else if (i == 3) {
      if (j == 0) { return 0.0; }
      if (j == 1) { return 0.0; }
      if (j == 2) { return 0.0; } 
      if (j == 3) { return 1.0; } 
    } 
    std::cerr << "Transform3D subscripting: bad indeces "
	      << "(" << i << "," << j << ")" << std::endl;
    return 0.0;
  }
  
  //--------------------------------------------------------------------------
  Transform3D Transform3D::operator*(const Transform3D & b) const {
    return Transform3D
      (xx_*b.xx_+xy_*b.yx_+xz_*b.zx_, xx_*b.xy_+xy_*b.yy_+xz_*b.zy_,
       xx_*b.xz_+xy_*b.yz_+xz_*b.zz_, xx_*b.dx_+xy_*b.dy_+xz_*b.dz_+dx_,
       yx_*b.xx_+yy_*b.yx_+yz_*b.zx_, yx_*b.xy_+yy_*b.yy_+yz_*b.zy_,
       yx_*b.xz_+yy_*b.yz_+yz_*b.zz_, yx_*b.dx_+yy_*b.dy_+yz_*b.dz_+dy_,
       zx_*b.xx_+zy_*b.yx_+zz_*b.zx_, zx_*b.xy_+zy_*b.yy_+zz_*b.zy_,
       zx_*b.xz_+zy_*b.yz_+zz_*b.zz_, zx_*b.dx_+zy_*b.dy_+zz_*b.dz_+dz_);
  }

  // -------------------------------------------------------------------------
  Transform3D::Transform3D(const Point3D<double> & fr0,
			   const Point3D<double> & fr1,
			   const Point3D<double> & fr2,
			   const Point3D<double> & to0,
			   const Point3D<double> & to1,
			   const Point3D<double> & to2)
  /***********************************************************************
   *                                                                     *
   * Name: Transform3D::Transform3D              Date:    24.09.96 *
   * Author: E.Chernyaev (IHEP/Protvino)               Revised:          *
   *                                                                     *
   * Function: Create 3D Transformation from one coordinate system       *
   *           defined by its origin "fr0" and two axes "fr0->fr1",      *
   *           "fr0->fr2" to another coordinate system "to0", "to0->to1" *
   *           and "to0->to2"                                            *
   *                                                                     *
   ***********************************************************************/
  {
    Vector3D<double> x1,y1,z1, x2,y2,z2;
    x1 = (fr1 - fr0).unit();
    y1 = (fr2 - fr0).unit();
    x2 = (to1 - to0).unit();
    y2 = (to2 - to0).unit();
    
    //   C H E C K   A N G L E S
    
    double cos1, cos2;
    cos1 = x1*y1;
    cos2 = x2*y2;
    
    if (abs(1.0-cos1) <= 0.000001 || abs(1.0-cos2) <= 0.000001) {
      std::cerr << "Transform3D: zero angle between axes" << std::endl;
      setIdentity();
    }else{
      if (abs(cos1-cos2) > 0.000001) {
	std::cerr << "Transform3D: angles between axes are not equal"
		  << std::endl;
      }
      
      //   F I N D   R O T A T I O N   M A T R I X
      
      z1 = (x1.cross(y1)).unit();
      y1  = z1.cross(x1);
    
      z2 = (x2.cross(y2)).unit();
      y2  = z2.cross(x2);
      
      double detxx =  (y1.y()*z1.z() - z1.y()*y1.z());
      double detxy = -(y1.x()*z1.z() - z1.x()*y1.z());
      double detxz =  (y1.x()*z1.y() - z1.x()*y1.y());
      double detyx = -(x1.y()*z1.z() - z1.y()*x1.z());
      double detyy =  (x1.x()*z1.z() - z1.x()*x1.z());
      double detyz = -(x1.x()*z1.y() - z1.x()*x1.y());
      double detzx =  (x1.y()*y1.z() - y1.y()*x1.z());
      double detzy = -(x1.x()*y1.z() - y1.x()*x1.z());
      double detzz =  (x1.x()*y1.y() - y1.x()*x1.y());
 
      double txx = x2.x()*detxx + y2.x()*detyx + z2.x()*detzx; 
      double txy = x2.x()*detxy + y2.x()*detyy + z2.x()*detzy; 
      double txz = x2.x()*detxz + y2.x()*detyz + z2.x()*detzz; 
      double tyx = x2.y()*detxx + y2.y()*detyx + z2.y()*detzx; 
      double tyy = x2.y()*detxy + y2.y()*detyy + z2.y()*detzy; 
      double tyz = x2.y()*detxz + y2.y()*detyz + z2.y()*detzz; 
      double tzx = x2.z()*detxx + y2.z()*detyx + z2.z()*detzx; 
      double tzy = x2.z()*detxy + y2.z()*detyy + z2.z()*detzy; 
      double tzz = x2.z()*detxz + y2.z()*detyz + z2.z()*detzz; 

      //   S E T    T R A N S F O R M A T I O N 

      double dx1 = fr0.x(), dy1 = fr0.y(), dz1 = fr0.z();
      double dx2 = to0.x(), dy2 = to0.y(), dz2 = to0.z();

      setTransform(txx, txy, txz, dx2-txx*dx1-txy*dy1-txz*dz1,
		   tyx, tyy, tyz, dy2-tyx*dx1-tyy*dy1-tyz*dz1,
		   tzx, tzy, tzz, dz2-tzx*dx1-tzy*dy1-tzz*dz1);
    }
  }

  // -------------------------------------------------------------------------
  Transform3D Transform3D::inverse() const
  /***********************************************************************
   *                                                                     *
   * Name: Transform3D::inverse                     Date:    24.09.96 *
   * Author: E.Chernyaev (IHEP/Protvino)               Revised:          *
   *                                                                     *
   * Function: Find inverse affine transformation                        *
   *                                                                     *
   ***********************************************************************/
  {
    double detxx = yy_*zz_-yz_*zy_;
    double detxy = yx_*zz_-yz_*zx_;
    double detxz = yx_*zy_-yy_*zx_;
    double det   = xx_*detxx - xy_*detxy + xz_*detxz;
    if (det == 0) {
      std::cerr << "Transform3D::inverse error: zero determinant" << std::endl;
      return Transform3D();
    }
    det = 1./det; detxx *= det; detxy *= det; detxz *= det;
    double detyx = (xy_*zz_ - xz_*zy_)*det;
    double detyy = (xx_*zz_ - xz_*zx_)*det;
    double detyz = (xx_*zy_ - xy_*zx_)*det;
    double detzx = (xy_*yz_ - xz_*yy_)*det;
    double detzy = (xx_*yz_ - xz_*yx_)*det;
    double detzz = (xx_*yy_ - xy_*yx_)*det;
    return Transform3D
      (detxx, -detyx,  detzx, -detxx*dx_+detyx*dy_-detzx*dz_,
      -detxy,  detyy, -detzy,  detxy*dx_-detyy*dy_+detzy*dz_,
       detxz, -detyz,  detzz, -detxz*dx_+detyz*dy_-detzz*dz_);
  }

  // -------------------------------------------------------------------------
  void Transform3D::getDecomposition(Scale3D & scale,
				     Rotate3D & rotation,
				     Translate3D & translation) const
  /***********************************************************************
   *                                                           CLHEP-1.7 *
   * Name: Transform3D::getDecomposition            Date:       09.06.01 *
   * Author: E.Chernyaev (IHEP/Protvino)            Revised:             *
   *                                                                     *
   * Function: Gets decomposition of general transformation on           *
   *           three consequentive specific transformations:             *
   *           Scale, then Rotation, then Translation.                   *
   *           If there was a reflection, then ScaleZ will be negative.  *
   *                                                                     *
   ***********************************************************************/
  {
    double sx = sqrt(xx_*xx_ + yx_*yx_ + zx_*zx_);
    double sy = sqrt(xy_*xy_ + yy_*yy_ + zy_*zy_);
    double sz = sqrt(xz_*xz_ + yz_*yz_ + zz_*zz_);

    if (xx_*(yy_*zz_-yz_*zy_) -
	xy_*(yx_*zz_-yz_*zx_) +
	xz_*(yx_*zy_-yy_*zx_) < 0) sz = -sz;
    scale.setTransform(sx,0,0,0,  0,sy,0,0, 0,0,sz,0);
    rotation.setTransform(xx_/sx,xy_/sy,xz_/sz,0,
			  yx_/sx,yy_/sy,yz_/sz,0,
			  zx_/sx,zy_/sy,zz_/sz,0); 
    translation.setTransform(1,0,0,dx_, 0,1,0,dy_, 0,0,1,dz_);
  }

  // -------------------------------------------------------------------------
  bool Transform3D::isNear(const Transform3D & t, double tolerance) const
  { 
    return ( (abs(xx_ - t.xx_) <= tolerance) && 
	     (abs(xy_ - t.xy_) <= tolerance) &&
	     (abs(xz_ - t.xz_) <= tolerance) &&
	     (abs(dx_ - t.dx_) <= tolerance) &&
	     (abs(yx_ - t.yx_) <= tolerance) &&
	     (abs(yy_ - t.yy_) <= tolerance) &&
	     (abs(yz_ - t.yz_) <= tolerance) &&
	     (abs(dy_ - t.dy_) <= tolerance) &&
	     (abs(zx_ - t.zx_) <= tolerance) &&
	     (abs(zy_ - t.zy_) <= tolerance) &&
	     (abs(zz_ - t.zz_) <= tolerance) &&
	     (abs(dz_ - t.dz_) <= tolerance) );
  }

  // -------------------------------------------------------------------------
  bool Transform3D::operator==(const Transform3D & t) const
  {
    return (this == &t) ? true :
      (xx_==t.xx_ && xy_==t.xy_ && xz_==t.xz_ && dx_==t.dx_ && 
       yx_==t.yx_ && yy_==t.yy_ && yz_==t.yz_ && dy_==t.dy_ &&
       zx_==t.zx_ && zy_==t.zy_ && zz_==t.zz_ && dz_==t.dz_ );
  }

  //   3 D   R O T A T I O N -------------------------------------------------

  Rotate3D::Rotate3D(double a,
		     const Point3D<double> & p1,
		     const Point3D<double> & p2) : Transform3D()
  /***********************************************************************
   *                                                                     *
   * Name: Rotate3D::Rotate3D                       Date:    24.09.96 *
   * Author: E.Chernyaev (IHEP/Protvino)               Revised:          *
   *                                                                     *
   * Function: Create 3D Rotation through angle "a" (counterclockwise)   *
   *           around the axis p1->p2                                    *
   *                                                                     *
   ***********************************************************************/
  {
    if (a == 0) return;

    double cx = p2.x()-p1.x(), cy = p2.y()-p1.y(), cz = p2.z()-p1.z();
    double ll = sqrt(cx*cx + cy*cy + cz*cz); 
    if (ll == 0) {
      std::cerr << "Rotate3D: zero axis" << std::endl;
    }else{
      double cosa = cos(a), sina = sin(a);
      cx /= ll; cy /= ll; cz /= ll;   
    
      double txx = cosa + (1-cosa)*cx*cx;
      double txy =        (1-cosa)*cx*cy - sina*cz;
      double txz =        (1-cosa)*cx*cz + sina*cy;
    
      double tyx =        (1-cosa)*cy*cx + sina*cz;
      double tyy = cosa + (1-cosa)*cy*cy; 
      double tyz =        (1-cosa)*cy*cz - sina*cx;
    
      double tzx =        (1-cosa)*cz*cx - sina*cy;
      double tzy =        (1-cosa)*cz*cy + sina*cx;
      double tzz = cosa + (1-cosa)*cz*cz;
    
      double tdx = p1.x(), tdy = p1.y(), tdz = p1.z(); 
    
      setTransform(txx, txy, txz, tdx-txx*tdx-txy*tdy-txz*tdz,
		   tyx, tyy, tyz, tdy-tyx*tdx-tyy*tdy-tyz*tdz,
		   tzx, tzy, tzz, tdz-tzx*tdx-tzy*tdy-tzz*tdz);
    }
  }

  //   3 D   R E F L E C T I O N ---------------------------------------------

  Reflect3D::Reflect3D(double a, double b, double c, double d)
  /***********************************************************************
   *                                                                     *
   * Name: Reflect3D::Reflect3D                        Date:    24.09.96 *
   * Author: E.Chernyaev (IHEP/Protvino)               Revised:          *
   *                                                                     *
   * Function: Create 3D Reflection in a plane a*x + b*y + c*z + d = 0   *
   *                                                                     *
   ***********************************************************************/
  {
    double ll = a*a+b*b+c*c;
    if (ll == 0) {
      std::cerr << "Reflect3D: zero normal" << std::endl;
      setIdentity();
    }else{
      ll = 1/ll;
      double aa = a*a*ll, ab = a*b*ll, ac = a*c*ll, ad = a*d*ll,
	     bb = b*b*ll, bc = b*c*ll, bd = b*d*ll,
	     cc = c*c*ll, cd = c*d*ll;
      setTransform(-aa+bb+cc, -ab-ab,    -ac-ac,    -ad-ad,
		   -ab-ab,     aa-bb+cc, -bc-bc,    -bd-bd,
		   -ac-ac,    -bc-bc,     aa+bb-cc, -cd-cd);
    }
  }
} /* namespace HepGeom */
