// -*- C++ -*-
// $Id: Plane3D.h,v 1.3.4.1 2004/11/30 20:08:38 garren Exp $
// ---------------------------------------------------------------------------
//
// This file is a part of the CLHEP - a Class Library for High Energy Physics.
//
// History:
// 22.09.96 E.Chernyaev - initial version
// 19.10.96 J.Allison - added == and <<.
// 15.04.03 E.Chernyaev - CLHEP-1.9: template version

#ifndef HEP_PLANE3D_H
#define HEP_PLANE3D_H

#include <iosfwd>
/*#include "CLHEP/Geometry/defs.h"
#include "CLHEP/Geometry/Point3D.h"
#include "CLHEP/Geometry/Normal3D.h"
#include "CLHEP/Geometry/Transform3D.h"
*/
#include "../units/defs.h"
#include "Point3D.h"
#include "Normal3D.h"
#include "Transform3D.h"

namespace HepGeom {

  /**
   * Template class for geometrical plane in 3D.
   *
   * @author Evgeni Chernyaev <Evgueni.Tcherniaev@cern.ch>
   * @ingroup geometry
   */
  template<class T>
  class Plane3D {
  protected:
    T a_, b_, c_, d_;
 
  public:
    /**
     *  Default constructor - creates plane z=0. */
    Plane3D() : a_(0.), b_(0.), c_(1.), d_(0.) {}

    /**
     * Constructor from four numbers - creates plane a*x+b*y+c*z+d=0. */
    Plane3D(T a, T b, T c, T d) : a_(a), b_(b), c_(c), d_(d) {}

    /**
     * Constructor from normal and point. */
    Plane3D(const Normal3D<T> & n, const Point3D<T> & p)
      : a_(n.x()), b_(n.y()), c_(n.z()), d_(-n*p) {}

    /**
     * Constructor from three points. */
    Plane3D(const Point3D<T> & p1,
	    const Point3D<T> & p2,
	    const Point3D<T> & p3) {
      Normal3D<T> n = (p2-p1).cross(p3-p1);
      a_ = n.x(); b_ = n.y(); c_ = n.z(); d_ = -n*p1;
    }

    /** Copy constructor.
     * Plane3D<double> has two constructors:
     * from Plane3D<double> (provided by compiler) and
     * from Plane3D<float> (defined in this file).
     * Plane3D<float> has only the last one.
     */
    Plane3D(const Plane3D<float> & p)
      : a_(p.a_), b_(p.b_), c_(p.c_), d_(p.d_) {}

    /**
     * Destructor. */
    ~Plane3D() {};

    /**
     * Assignment. */
    Plane3D<T> & operator=(const Plane3D<T> & p) {
      a_ = p.a_; b_ = p.b_; c_ = p.c_; d_ = p.d_; return *this;
    }

    /**
     * Returns the a-coefficient in the plane equation: a*x+b*y+c*z+d=0. */
    T a() const { return a_; }
    /**
     * Returns the b-coefficient in the plane equation: a*x+b*y+c*z+d=0. */
    T b() const { return b_; }
    /**
     * Returns the c-coefficient in the plane equation: a*x+b*y+c*z+d=0. */
    T c() const { return c_; }
    /**
     * Returns the free member of the plane equation: a*x+b*y+c*z+d=0. */
    T d() const { return d_; }

    /**
     * Returns normal. */
    Normal3D<T> normal() const { return Normal3D<T>(a_,b_,c_); }

    /**
     * Normalization. */
    Plane3D<T> & normalize() {
      double ll = sqrt(a_*a_ + b_*b_ + c_*c_);
      if (ll > 0.) { a_ /= ll; b_ /= ll; c_ /= ll, d_ /= ll; }
      return *this;
    }

    /**
     * Returns distance to the point. */
    T distance(const Point3D<T> & p) const {
      return a()*p.x() + b()*p.y() + c()*p.z() + d();
    }

    /**
     * Returns projection of the point to the plane. */
    Point3D<T> point(const Point3D<T> & p) const {
      T k = distance(p)/(a()*a()+b()*b()+c()*c());
      return Point3D<T>(p.x()-a()*k, p.y()-b()*k, p.z()-c()*k);
    }

    /**
     * Returns projection of the origin to the plane. */
    Point3D<T> point() const {
      T k = -d()/(a()*a()+b()*b()+c()*c());
      return Point3D<T>(a()*k, b()*k, c()*k);
    }

    /**
     * Test for equality. */
    bool operator == (const Plane3D<T> & p) const {
      return a() == p.a() && b() == p.b() && c() == p.c() && d() == p.d();
    }

    /**
     * Test for inequality. */
    bool operator != (const Plane3D<T> & p) const {
      return a() != p.a() || b() != p.b() || c() != p.c() || d() != p.d();
    }

    /**
     * Transformation by Transform3D. */
    Plane3D<T> & transform(const Transform3D & m) {
      Normal3D<T> n = normal();
      n.transform(m);
      d_ = -n*point().transform(m); a_ = n.x(); b_ = n.y(); c_ = n.z();
      return *this;
    }
  };

  /**
   * Output to the stream.
   * @relates Plane3D
   */
  std::ostream & operator<<(std::ostream & os, const Plane3D<float> & p);

  /**
   * Output to the stream.
   * @relates Plane3D
   */
  std::ostream & operator<<(std::ostream & os, const Plane3D<double> & p);

} /* namespace HepGeom */

#ifdef ENABLE_BACKWARDS_COMPATIBILITY
//  backwards compatibility will be enabled ONLY in CLHEP 1.9
typedef HepGeom::Plane3D<double> HepPlane3D;
#endif

#endif /* HEP_PLANE3D_H */
