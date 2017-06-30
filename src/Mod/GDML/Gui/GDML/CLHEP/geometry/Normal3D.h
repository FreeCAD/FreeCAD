// -*- C++ -*-
// $Id: Normal3D.h,v 1.3 2003/10/23 21:29:50 garren Exp $
// ---------------------------------------------------------------------------
//
// This file is a part of the CLHEP - a Class Library for High Energy Physics.
//
// History:
// 09.09.96 E.Chernyaev - initial version
// 12.06.01 E.Chernyaev - CLHEP-1.7: introduction of BasicVector3D to decouple
//                        the functionality from CLHEP::Hep3Vector
// 01.04.03 E.Chernyaev - CLHEP-1.9: template version
//

#ifndef HEP_NORMAL3D_H
#define HEP_NORMAL3D_H

#include <iosfwd>
///#include "CLHEP/Geometry/defs.h"
///#include "CLHEP/Vector/ThreeVector.h"
///#include "CLHEP/Geometry/BasicVector3D.h"
#include "./defs.h"
#include "../vector/ThreeVector.h"
#include "./BasicVector3D.h"

namespace HepGeom {

  class Transform3D;

  /**
   * Geometrical 3D Normal.
   * This is just a declaration of the class needed to define
   * specializations Normal3D<float> and Normal3D<double>.
   *
   * @ingroup geometry
   * @author Evgeni Chernyaev <Evgueni.Tcherniaev@cern.ch>
   */
  template<class T>
  class Normal3D : public BasicVector3D<T> {};

  /**
   * Geometrical 3D Normal with components of float type.
   *
   * @author Evgeni Chernyaev <Evgueni.Tcherniaev@cern.ch>
   * @ingroup geometry
   */
  template<>
  class Normal3D<float> : public BasicVector3D<float> {
  public:
    /**
     * Default constructor. */
    Normal3D() {}

    /**
     * Constructor from three numbers. */
    Normal3D(float x, float y, float z) : BasicVector3D<float>(x,y,z) {}

    /**
     * Constructor from array of floats. */
    explicit Normal3D(const float * a)
      : BasicVector3D<float>(a[0],a[1],a[2]) {}

    /**
     * Copy constructor. */
    Normal3D(const Normal3D<float> & v) : BasicVector3D<float>(v) {}

    /**
     * Constructor from BasicVector3D<float>. */
    Normal3D(const BasicVector3D<float> & v) : BasicVector3D<float>(v) {}

    /**
     * Destructor. */
    ~Normal3D() {}

    /**
     * Assignment. */
    Normal3D<float> & operator=(const Normal3D<float> & v) {
      set(v.x(),v.y(),v.z()); return *this;
    }

    /**
     * Assignment from BasicVector3D<float>. */
    Normal3D<float> & operator=(const BasicVector3D<float> & v) {
      set(v.x(),v.y(),v.z()); return *this;
    }

    /**
     * Transformation by Transform3D. */
    Normal3D<float> & transform(const Transform3D & m);
  };

  /**
   * Transformation of Normal<float> by Transform3D.
   * @relates Normal3D
   */
  Normal3D<float>
  operator*(const Transform3D & m, const Normal3D<float> & n);

  /**
   * Geometrical 3D Normal with components of double type.
   *
   * @author Evgeni Chernyaev <Evgueni.Tcherniaev@cern.ch>
   * @ingroup geometry
   */
  template<>
  class Normal3D<double> : public BasicVector3D<double> {
  public:
    /**
     * Default constructor. */
    Normal3D() {}

    /**
     * Constructor from three numbers. */
    Normal3D(double x, double y, double z) : BasicVector3D<double>(x,y,z) {}

    /**
     * Constructor from array of floats. */
    explicit Normal3D(const float * a)
      : BasicVector3D<double>(a[0],a[1],a[2]) {}

    /**
     * Constructor from array of doubles. */
    explicit Normal3D(const double * a)
      : BasicVector3D<double>(a[0],a[1],a[2]) {}

    /**
     * Copy constructor. */
    Normal3D(const Normal3D<double> & v) : BasicVector3D<double>(v) {}

    /**
     * Constructor from BasicVector3D<float>. */
    Normal3D(const BasicVector3D<float> & v) : BasicVector3D<double>(v) {}

    /**
     * Constructor from BasicVector3D<double>. */
    Normal3D(const BasicVector3D<double> & v) : BasicVector3D<double>(v) {}

    /**
     * Destructor. */
    ~Normal3D() {}

    /**
     * Constructor from CLHEP::Hep3Vector.
     * This constructor is needed only for backward compatibility and
     * in principle should be absent.
     */
    Normal3D(const CLHEP::Hep3Vector & v)
      : BasicVector3D<double>(v.x(),v.y(),v.z()) {}

    /**
     * Conversion (cast) to CLHEP::Hep3Vector.
     * This operator is needed only for backward compatibility and
     * in principle should not exit.
     */ 
    operator CLHEP::Hep3Vector () const { return CLHEP::Hep3Vector(x(),y(),z()); }

    /**
     * Assignment. */
    Normal3D<double> & operator=(const Normal3D<double> & v) {
      set(v.x(),v.y(),v.z()); return *this;
    }

    /**
     * Assignment from BasicVector3D<float>. */
    Normal3D<double> & operator=(const BasicVector3D<float> & v) {
      set(v.x(),v.y(),v.z()); return *this;
    }

    /**
     * Assignment from BasicVector3D<double>. */
    Normal3D<double> & operator=(const BasicVector3D<double> & v) {
      set(v.x(),v.y(),v.z()); return *this;
    }

    /**
     * Transformation by Transform3D. */
    Normal3D<double> & transform(const Transform3D & m);
  };

  /**
   * Transformation of Normal<double> by Transform3D.
   * @relates Normal3D
   */
  Normal3D<double>
  operator*(const Transform3D & m, const Normal3D<double> & n);

} /* namespace HepGeom */

#ifdef ENABLE_BACKWARDS_COMPATIBILITY
//  backwards compatibility will be enabled ONLY in CLHEP 1.9
typedef HepGeom::Normal3D<double> HepNormal3D;
#endif

#endif /* HEP_NORMAL3D_H */
