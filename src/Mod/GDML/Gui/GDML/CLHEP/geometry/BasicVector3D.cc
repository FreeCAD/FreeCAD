// -*- C++ -*-
// $Id: BasicVector3D.cc,v 1.3 2003/08/13 20:00:11 garren Exp $
// ---------------------------------------------------------------------------

#include <math.h>
#include <float.h>
#include <iostream>
///#include "CLHEP/Geometry/defs.h"
///#include "CLHEP/Geometry/BasicVector3D.h"
#include "./defs.h"
#include "./BasicVector3D.h"

namespace HepGeom {
  //--------------------------------------------------------------------------
  template<>
  float BasicVector3D<float>::pseudoRapidity() const {
    float ma = mag(), dz = z();
    if (ma ==  0)  return  0;
    if (ma ==  dz) return  FLT_MAX;
    if (ma == -dz) return -FLT_MAX;
    return 0.5*log((ma+dz)/(ma-dz));
  }

  //--------------------------------------------------------------------------
  template<>
  void BasicVector3D<float>::setEta(float a) {
    double ma = mag();
    if (ma == 0) return;
    double tanHalfTheta  = exp(-a);
    double tanHalfTheta2 = tanHalfTheta * tanHalfTheta;
    double cosTheta      = (1 - tanHalfTheta2) / (1 + tanHalfTheta2);
    double rh            = ma * sqrt(1 - cosTheta*cosTheta);
    double ph            = phi();
    set(rh*cos(ph), rh*sin(ph), ma*cosTheta);
  }

  //--------------------------------------------------------------------------
  template<>
  float BasicVector3D<float>::angle(const BasicVector3D<float> & v) const {
    double cosa = 0;
    double ptot = mag()*v.mag();
    if(ptot > 0) {
      cosa = dot(v)/ptot;
      if(cosa >  1) cosa =  1;
      if(cosa < -1) cosa = -1;
    }
    return acos(cosa);
  }

  //--------------------------------------------------------------------------
  template<>
  BasicVector3D<float> & BasicVector3D<float>::rotateX(float a) {
    double sina = sin(a), cosa = cos(a), dy = y(), dz = z();
    setY(dy*cosa-dz*sina);
    setZ(dz*cosa+dy*sina);
    return *this;
  }

  //--------------------------------------------------------------------------
  template<>
  BasicVector3D<float> & BasicVector3D<float>::rotateY(float a) {
    double sina = sin(a), cosa = cos(a), dz = z(), dx = x();
    setZ(dz*cosa-dx*sina);
    setX(dx*cosa+dz*sina);
    return *this;
  }

  //--------------------------------------------------------------------------
  template<>
  BasicVector3D<float> & BasicVector3D<float>::rotateZ(float a) {
    double sina = sin(a), cosa = cos(a), dx = x(), dy = y();
    setX(dx*cosa-dy*sina);
    setY(dy*cosa+dx*sina);
    return *this;
  }

  //--------------------------------------------------------------------------
  template<>
  BasicVector3D<float> &
  BasicVector3D<float>::rotate(float a, const BasicVector3D<float> & v) {
    if (a  == 0) return *this;
    double cx = v.x(), cy = v.y(), cz = v.z();
    double ll = sqrt(cx*cx + cy*cy + cz*cz);
    if (ll == 0) {
      std::cerr << "BasicVector<float>::rotate() : zero axis" << std::endl;
      return *this;
    }
    double cosa = cos(a), sina = sin(a);
    cx /= ll; cy /= ll; cz /= ll;   

    double xx = cosa + (1-cosa)*cx*cx;
    double xy =        (1-cosa)*cx*cy - sina*cz;
    double xz =        (1-cosa)*cx*cz + sina*cy;
    
    double yx =        (1-cosa)*cy*cx + sina*cz;
    double yy = cosa + (1-cosa)*cy*cy; 
    double yz =        (1-cosa)*cy*cz - sina*cx;
    
    double zx =        (1-cosa)*cz*cx - sina*cy;
    double zy =        (1-cosa)*cz*cy + sina*cx;
    double zz = cosa + (1-cosa)*cz*cz;

    cx = x(); cy = y(); cz = z();   
    set(xx*cx+xy*cy+xz*cz, yx*cx+yy*cy+yz*cz, zx*cx+zy*cy+zz*cz);
    return *this;
  }

  //--------------------------------------------------------------------------
  std::ostream &
  operator<<(std::ostream & os, const BasicVector3D<float> & a)
  {
    return os << "(" << a.x() << "," << a.y() << "," << a.z() << ")";
  }

  //--------------------------------------------------------------------------
  std::istream &
  operator>> (std::istream & is, BasicVector3D<float> & a)
  {
    // Required format is ( a, b, c ) that is, three numbers, preceded by
    // (, followed by ), and separated by commas.  The three numbers are
    // taken as x, y, z.

    float x, y, z;
    char c;

    is >> std::ws >> c;
    // ws is defined to invoke eatwhite(istream & )
    // see (Stroustrup gray book) page 333 and 345.
    if (is.fail() || c != '(' ) {
      std::cerr
	<< "Could not find required opening parenthesis "
	<< "in input of a BasicVector3D<float>"
	<< std::endl;
      return is;
    }

    is >> x >> std::ws >> c;
    if (is.fail() || c != ',' ) {
      std::cerr
	<< "Could not find x value and required trailing comma "
	<< "in input of a BasicVector3D<float>"
	<< std::endl; 
      return is;
    }

    is >> y >> std::ws >> c;
    if (is.fail() || c != ',' ) {
      std::cerr
	<< "Could not find y value and required trailing comma "
	<<  "in input of a BasicVector3D<float>"
	<< std::endl;
      return is;
    }

    is >> z >> std::ws >> c;
    if (is.fail() || c != ')' ) {
      std::cerr
	<< "Could not find z value and required close parenthesis "
	<< "in input of a BasicVector3D<float>"
	<< std::endl;
      return is;
    }

    a.setX(x);
    a.setY(y);
    a.setZ(z);
    return is;
  }
  
  //--------------------------------------------------------------------------
  template<>
  double BasicVector3D<double>::pseudoRapidity() const {
    double ma = mag(), dz = z();
    if (ma ==  0)  return  0;
    if (ma ==  dz) return  DBL_MAX;
    if (ma == -dz) return -DBL_MAX;
    return 0.5*log((ma+dz)/(ma-dz));
  }

  //--------------------------------------------------------------------------
  template<>
  void BasicVector3D<double>::setEta(double a) {
    double ma = mag();
    if (ma == 0) return;
    double tanHalfTheta  = exp(-a);
    double tanHalfTheta2 = tanHalfTheta * tanHalfTheta;
    double cosTheta      = (1 - tanHalfTheta2) / (1 + tanHalfTheta2);
    double rh            = ma * sqrt(1 - cosTheta*cosTheta);
    double ph            = phi();
    set(rh*cos(ph), rh*sin(ph), ma*cosTheta);
  }

  //--------------------------------------------------------------------------
  template<>
  double BasicVector3D<double>::angle(const BasicVector3D<double> & v) const {
    double cosa = 0;
    double ptot = mag()*v.mag();
    if(ptot > 0) {
      cosa = dot(v)/ptot;
      if(cosa >  1) cosa =  1;
      if(cosa < -1) cosa = -1;
    }
    return acos(cosa);
  }

  //--------------------------------------------------------------------------
  template<>
  BasicVector3D<double> & BasicVector3D<double>::rotateX(double a) {
    double sina = sin(a), cosa = cos(a), dy = y(), dz = z();
    setY(dy*cosa-dz*sina);
    setZ(dz*cosa+dy*sina);
    return *this;
  }

  //--------------------------------------------------------------------------
  template<>
  BasicVector3D<double> & BasicVector3D<double>::rotateY(double a) {
    double sina = sin(a), cosa = cos(a), dz = z(), dx = x();
    setZ(dz*cosa-dx*sina);
    setX(dx*cosa+dz*sina);
    return *this;
  }

  //--------------------------------------------------------------------------
  template<>
  BasicVector3D<double> & BasicVector3D<double>::rotateZ(double a) {
    double sina = sin(a), cosa = cos(a), dx = x(), dy = y();
    setX(dx*cosa-dy*sina);
    setY(dy*cosa+dx*sina);
    return *this;
  }

  //--------------------------------------------------------------------------
  template<>
  BasicVector3D<double> &
  BasicVector3D<double>::rotate(double a, const BasicVector3D<double> & v) {
    if (a  == 0) return *this;
    double cx = v.x(), cy = v.y(), cz = v.z();
    double ll = sqrt(cx*cx + cy*cy + cz*cz);
    if (ll == 0) {
      std::cerr << "BasicVector<double>::rotate() : zero axis" << std::endl;
      return *this;
    }
    double cosa = cos(a), sina = sin(a);
    cx /= ll; cy /= ll; cz /= ll;   

    double xx = cosa + (1-cosa)*cx*cx;
    double xy =        (1-cosa)*cx*cy - sina*cz;
    double xz =        (1-cosa)*cx*cz + sina*cy;
    
    double yx =        (1-cosa)*cy*cx + sina*cz;
    double yy = cosa + (1-cosa)*cy*cy; 
    double yz =        (1-cosa)*cy*cz - sina*cx;
    
    double zx =        (1-cosa)*cz*cx - sina*cy;
    double zy =        (1-cosa)*cz*cy + sina*cx;
    double zz = cosa + (1-cosa)*cz*cz;

    cx = x(); cy = y(); cz = z();   
    set(xx*cx+xy*cy+xz*cz, yx*cx+yy*cy+yz*cz, zx*cx+zy*cy+zz*cz);
    return *this;
  }

  //--------------------------------------------------------------------------
  std::ostream &
  operator<< (std::ostream & os, const BasicVector3D<double> & a)
  {
    return os << "(" << a.x() << "," << a.y() << "," << a.z() << ")";
  }
  
  //--------------------------------------------------------------------------
  std::istream &
  operator>> (std::istream & is, BasicVector3D<double> & a)
  {
    // Required format is ( a, b, c ) that is, three numbers, preceded by
    // (, followed by ), and separated by commas.  The three numbers are
    // taken as x, y, z.
    
    double x, y, z;
    char c;
    
    is >> std::ws >> c;
    // ws is defined to invoke eatwhite(istream & )
    // see (Stroustrup gray book) page 333 and 345.
    if (is.fail() || c != '(' ) {
      std::cerr
	<< "Could not find required opening parenthesis "
	<< "in input of a BasicVector3D<double>"
	<< std::endl;
      return is;
    }

    is >> x >> std::ws >> c;
    if (is.fail() || c != ',' ) {
      std::cerr
	<< "Could not find x value and required trailing comma "
	<< "in input of a BasicVector3D<double>"
	<< std::endl; 
      return is;
    }

    is >> y >> std::ws >> c;
    if (is.fail() || c != ',' ) {
      std::cerr
	<< "Could not find y value and required trailing comma "
	<<  "in input of a BasicVector3D<double>"
	<< std::endl;
      return is;
    }

    is >> z >> std::ws >> c;
    if (is.fail() || c != ')' ) {
      std::cerr
	<< "Could not find z value and required close parenthesis "
	<< "in input of a BasicVector3D<double>"
	<< std::endl;
      return is;
    }

    a.setX(x);
    a.setY(y);
    a.setZ(z);
    return is;
  }
} /* namespace HepGeom */
