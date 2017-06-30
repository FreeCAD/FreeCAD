// -*- C++ -*-
// $Id: Transform3D.h,v 1.3.4.1 2004/10/27 13:49:33 pfeiffer Exp $
// ---------------------------------------------------------------------------
//
// This file is a part of the CLHEP - a Class Library for High Energy Physics.
//
// Hep geometrical 3D Transformation class
//
// Author: Evgeni Chernyaev <Evgueni.Tcherniaev@cern.ch>
//
//          ******************************************
//          *                                        *
//          *               Transform                *
//          *               /  / \  \                *
//          *       --------  /   \  --------        *
//          *      /         /     \         \       *
//          *   Rotate Translate  Reflect   Scale    *
//          *    / | \    / | \    / | \    / | \    *
//          *   X  Y  Z  X  Y  Z  X  Y  Z  X  Y  Z   *
//          *                                        *
//          ******************************************
//
// Identity transformation:
//   Transform3D::Identity   - global identity transformation;
//   any constructor without parameters, e.g. Transform3D();
//   m.setIdentity()            - set "m" to identity;
//
// General transformations:
//   Transform3D(m,v)         - transformation given by Rotation "m"
//                              and CLHEP::Hep3Vector "v";
//   Transform3D(a0,a1,a2, b0,b1,b2) - transformation given by initial
//                               and transformed positions of three points;
// Rotations:
//   Rotate3D(m)              - rotation given by CLHEP::HepRotation "m";
//   Rotate3D(ang,v)          - rotation through the angle "ang" around
//                              vector "v";
//   Rotate3D(ang,p1,p2)      - rotation through the angle "ang"
//                              counterclockwise around the axis given by
//                              two points p1->p2;
//   Rotate3D(a1,a2, b1,b2)   - rotation around the origin defined by initial
//                              and transformed positions of two points;
//   RotateX3D(ang)           - rotation around X-axis;
//   RotateY3D(ang)           - rotation around Y-axis;
//   RotateZ3D(ang)           - rotation around Z-axis;
//
// Translations:
//   Translate3D(v)           - translation given by CLHEP::Hep3Vector "v";
//   Translate3D(dx,dy,dz)    - translation on vector (dx,dy,dz);
//   TraslateX3D(dx)          - translation along X-axis;
//   TraslateY3D(dy)          - translation along Y-axis;
//   TraslateZ3D(dz)          - translation along Z-axis;
//
// Reflections:
//   Reflect3D(a,b,c,d)       - reflection in the plane a*x+b*y+c*z+d=0;
//   Reflect3D(normal,p)      - reflection in the plane going through "p"
//                              and whose normal is equal to "normal";
//   ReflectX3D(a)            - reflect X in the plane x=a (default a=0);
//   ReflectY3D(a)            - reflect Y in the plane y=a (default a=0);
//   ReflectZ3D(a)            - reflect Z in the plane z=a (default a=0);
//
// Scalings:
//   Scale3D(sx,sy,sz)        - general scaling with factors "sx","sy","sz"
//                                 along X, Y and Z;
//   Scale3D(s)               - scaling with constant factor "s" along all 
//                                 directions;
//   ScaleX3D(sx)             - scale X;
//   ScaleY3D(sy)             - scale Y;
//   ScaleZ3D(sz)             - scale Z;
//
// Inverse transformation:
//   m.inverse() or           - returns inverse transformation;
//
// Compound transformation:
//   m3 = m2 * m1             - it is relatively slow in comparison with
//                              transformation of a vector. Use parenthesis
//                              to avoid this operation (see example below);
// Transformation of point:
//   p2 = m * p1
//
// Transformation of vector:
//   v2 = m * v1
//
// Transformation of normal:
//   n2 = m * n1
//
// The following table explains how different transformations affect
// point, vector and normal. "+" means affect, "-" means do not affect,
// "*" meas affect but in different way than "+" 
//
//                     Point  Vector  Normal
//      -------------+-------+-------+-------
//       Rotation    !   +   !   +   !   +
//       Translation !   +   !   -   !   -
//       Reflection  !   +   !   +   !   *
//       Scaling     !   +   !   +   !   *
//      -------------+-------+-------+-------
//
// Example of the usage:
//
//   Transform3D m1, m2, m3;
//   HepVector3D    v2, v1(0,0,0);
//
//   m1 = Rotate3D(angle, Vector3D(1,1,1));
//   m2 = Translate3D(dx,dy,dz);
//   m3 = m1.inverse();
//
//   v2 = m3*(m2*(m1*v1));
//
// History:
// 24.09.96 E.Chernyaev - initial version
//
// 26.02.97 E.Chernyaev
// - added global Identity by request of John Allison 
//   (to avoid problems with compilation on HP) 
// - added getRotation and getTranslation 
//
// 29.01.01 E.Chernyaev - added subscripting
// 11.06.01 E.Chernyaev - added getDecomposition

#ifndef HEP_TRANSFROM3D_H
#define HEP_TRANSFROM3D_H

///#include "CLHEP/Geometry/defs.h"
///#include "CLHEP/Vector/ThreeVector.h"
#include "defs.h"
#include "../vector/ThreeVector.h"

namespace HepGeom {

  template<class T> class Point3D;
  template<class T> class Vector3D;
  template<class T> class Normal3D;

  class Translate3D;
  class Rotate3D;
  class Scale3D;

  /**
   * Class for transformation of 3D geometrical objects.
   * It allows different translations, rotations, scalings and reflections.
   * Several specialized classes are derived from it:
   *
   * TranslateX3D, TranslateY3D, TranslateZ3D, Translate3D,<br>
   * RotateX3D,    RotateY3D,    RotateZ3D,    Rotate3D,   <br>
   * ScaleX3D,     ScaleY3D,     ScaleZ3D,     Scale3D,    <br>
   * ReflectX3D,   ReflectY3D,   ReflectZ3D,   Reflect3D.
   *
   * The idea behind these classes is to provide some additional constructors
   * for Transform3D, they normally should not be used as separate classes.
   *
   * Example:
   * @code
   *   HepGeom::Transform3D m;
   *   m = HepGeom::TranslateX3D(10.*cm);
   * @endcode
   *
   * Remark:
   * For the reason that the operator* is left associative, the notation
   * @code
   *   v2 = m3*(m2*(m1*v1));
   * @endcode
   * is much more effective then the notation
   * @code
   *   v2 = m3*m2*m1*v1;
   * @endcode
   * In the first case three operations Transform3D*Vector3D are executed,
   * in the second case two operations Transform3D*Transform3D and one
   * Transform3D*Vector3D are performed. Transform3D*Transform3D is
   * roughly three times slower than Transform3D*Vector3D.
   *
   * @author <Evgueni.Tcherniaev@cern.ch>
   * @ingroup geometry
   */
  class Transform3D {
  protected:
    double xx_, xy_, xz_, dx_,     // 4x3  Transformation Matrix
           yx_, yy_, yz_, dy_,
           zx_, zy_, zz_, dz_;

    // Protected constructor
    Transform3D(double XX, double XY, double XZ, double DX,
		double YX, double YY, double YZ, double DY,
		double ZX, double ZY, double ZZ, double DZ)
      : xx_(XX), xy_(XY), xz_(XZ), dx_(DX),
	yx_(YX), yy_(YY), yz_(YZ), dy_(DY),
	zx_(ZX), zy_(ZY), zz_(ZZ), dz_(DZ) {}

    // Set transformation matrix
    void setTransform(double XX, double XY, double XZ, double DX,
		      double YX, double YY, double YZ, double DY,
		      double ZX, double ZY, double ZZ, double DZ) {
      xx_ = XX; xy_ = XY; xz_ = XZ; dx_ = DX;
      yx_ = YX; yy_ = YY; yz_ = YZ; dy_ = DY;
      zx_ = ZX; zy_ = ZY; zz_ = ZZ; dz_ = DZ;
    }

  public:
    /**
     * Global identity transformation. */
    static const Transform3D Identity;

    // Helper class for implemention of C-style subscripting r[i][j] 
    class Transform3D_row {
    public:
      inline Transform3D_row(const Transform3D &, int);
      inline double operator [] (int) const;
    private:
      const Transform3D & rr;
      int ii;
    };

    /**
     * Default constructor - sets the Identity transformation. */
    Transform3D()
      : xx_(1), xy_(0), xz_(0), dx_(0),
	yx_(0), yy_(1), yz_(0), dy_(0),
	zx_(0), zy_(0), zz_(1), dz_(0) {}
  
    /**
     * Constructor: rotation and then translation. */
    inline Transform3D(const CLHEP::HepRotation & m, const CLHEP::Hep3Vector & v);

    /**
     * Constructor: transformation of basis (assumed - no reflection). */
    Transform3D(const Point3D<double> & fr0,
		const Point3D<double> & fr1,
		const Point3D<double> & fr2,
		const Point3D<double> & to0,
		const Point3D<double> & to1,
		const Point3D<double> & to2);

    /**
     * Copy constructor. */
    Transform3D(const Transform3D & m)
      : xx_(m.xx_), xy_(m.xy_), xz_(m.xz_), dx_(m.dx_),
	yx_(m.yx_), yy_(m.yy_), yz_(m.yz_), dy_(m.dy_),
	zx_(m.zx_), zy_(m.zy_), zz_(m.zz_), dz_(m.dz_) {}

    /**
     * Destructor. 
     * Virtual for now as some persistency mechanism needs that,
     * in future releases this might go away again.
     */
    ~Transform3D() { /* nop */ }

    /**
     * Returns object of the helper class for C-style subscripting r[i][j] */
    inline const Transform3D_row operator [] (int) const; 

    /** Fortran-style subscripting: returns (i,j) element of the matrix. */
    double operator () (int, int) const;

    /**
     * Gets xx-element of the transformation matrix. */
    double xx() const { return xx_; }
    /**
     * Gets xy-element of the transformation matrix. */
    double xy() const { return xy_; }
    /**
     * Gets xz-element of the transformation matrix. */
    double xz() const { return xz_; }
    /**
     * Gets yx-element of the transformation matrix. */
    double yx() const { return yx_; }
    /**
     * Gets yy-element of the transformation matrix. */
    double yy() const { return yy_; }
    /**
     * Gets yz-element of the transformation matrix. */
    double yz() const { return yz_; }
    /** 
     * Gets zx-element of the transformation matrix. */
    double zx() const { return zx_; }
    /**
     * Gets zy-element of the transformation matrix. */
    double zy() const { return zy_; }
    /**
     * Gets zz-element of the transformation matrix. */
    double zz() const { return zz_; }
    /**
     * Gets dx-element of the transformation matrix. */
    double dx() const { return dx_; }
    /**
     * Gets dy-element of the transformation matrix. */
    double dy() const { return dy_; }
    /**
     * Gets dz-element of the transformation matrix. */
    double dz() const { return dz_; }
    
    /**
     * Assignment. */
    Transform3D & operator=(const Transform3D &m) {
      xx_= m.xx_; xy_= m.xy_; xz_= m.xz_; dx_= m.dx_;
      yx_= m.yx_; yy_= m.yy_; yz_= m.yz_; dy_= m.dy_;
      zx_= m.zx_; zy_= m.zy_; zz_= m.zz_; dz_= m.dz_;
      return *this;
    }

    /**
     * Sets the Identity transformation. */
    void setIdentity() { 
      xy_= xz_= dx_= yx_= yz_= dy_= zx_= zy_= dz_= 0; xx_= yy_= zz_= 1;
    }
    
    /**
     * Returns the inverse transformation. */
    Transform3D inverse() const;
    
    /**
     * Transformation by another Transform3D. */
    Transform3D operator*(const Transform3D & b) const;
    
    /**
     * Decomposition of general transformation.
     * This function gets decomposition of the transformation
     * in three consequentive specific transformations: Scale3D,
     * then Rotate3D, then Translate3, i.e.
     * @code
     *   Transform3D = Translate3D * Rotate3D * Scale3D
     * @endcode
     *
     * @param scale       output: scaling transformation;
     *                    if there was a reflection, then scale factor for
     *                    z-component (scale(2,2)) will be negative.
     * @param rotation    output: rotation transformaion.
     * @param translation output: translation transformaion.
     */
    void getDecomposition(Scale3D & scale,
			  Rotate3D & rotation,
			  Translate3D & translation) const;

    /**
     * Returns true if the difference between corresponding
     * matrix elements is less than the tolerance.
     */
    bool isNear(const Transform3D & t, double tolerance = 2.2E-14 ) const;

    /**
     * Extracts the rotation matrix.
     * This functions is obsolete - use getDecomposition() instead.
     */
    inline CLHEP::HepRotation getRotation() const;
    
    /**
     * Extracts the translation vector.
     * This functions is obsolete - use getDecomposition() instead.
     */
    inline CLHEP::Hep3Vector getTranslation() const;
    
    /**
     * Test for equality. */
    bool operator == (const Transform3D & transform) const;
    
    /**
     * Test for inequality. */
    bool operator != (const Transform3D & transform) const {
      return ! operator==(transform);
    }
  };

  //   R O T A T I O N S

  /**
   * Constructs a rotation transformation.
   * This class provides additional constructors for Transform3D
   * and should not be used as a separate class.
   * 
   * Example of use:
   * @code
   *   Transform3D m;
   *   m = Rotate3D(30.*deg, HepVector3D(1.,1.,1.));
   * @endcode
   *
   * @author <Evgueni.Tcherniaev@cern.ch>
   * @ingroup geometry
   */
  class Rotate3D : public Transform3D {
  public:
    /**
     * Default constructor: sets the Identity transformation. */
    Rotate3D() : Transform3D() {}
    
    /**
     * Constructor from CLHEP::HepRotation. */
    inline Rotate3D(const CLHEP::HepRotation &m);

    /**
     * Constructor from angle and axis given by two points.
     * @param a  angle of rotation
     * @param p1 begin point of the axis
     * @param p2 end point of the axis
     */
    Rotate3D(double a,
	     const Point3D<double> & p1,
	     const Point3D<double> & p2);
    
    /**
     * Constructor from angle and axis.
     * @param a angle of rotation
     * @param v axis of rotation
     */
    inline Rotate3D(double a, const Vector3D<double> & v);

    /**
     * Constructor for rotation given by original and rotated position of
     * two points. It is assumed that there is no reflection.
     * @param fr1 original position of 1st point
     * @param fr2 original position of 2nd point
     * @param to1 rotated position of 1st point
     * @param to2 rotated position of 2nd point
     */
    inline Rotate3D(const Point3D<double> & fr1,
		    const Point3D<double> & fr2,
		    const Point3D<double> & to1,
		    const Point3D<double> & to2);
  };

  /**
   * Constructs a rotation around x-axis.
   * This class provides additional constructors for Transform3D
   * and should not be used as a separate class.
   * 
   * Example of use:
   * @code
   *   Transform3D m;
   *   m = RotateX3D(30.*deg);
   * @endcode
   *
   * @author <Evgueni.Tcherniaev@cern.ch>
   * @ingroup geometry
   */
  class RotateX3D : public Rotate3D {
  public:
    /**
     * Default constructor: sets the Identity transformation. */
    RotateX3D() : Rotate3D() {}
    
    /**
     * Constructs a rotation around x-axis by angle a. */
    RotateX3D(double a) {
      double cosa = cos(a), sina = sin(a); 
      setTransform(1,0,0,0,  0,cosa,-sina,0,  0,sina,cosa,0);
    }
  };

  /**
   * Constructs a rotation around y-axis.
   * This class provides additional constructors for Transform3D
   * and should not be used as a separate class.
   * 
   * Example of use:
   * @code
   *   Transform3D m;
   *   m = RotateY3D(30.*deg);
   * @endcode
   *
   * @author <Evgueni.Tcherniaev@cern.ch>
   * @ingroup geometry
   */
  class RotateY3D : public Rotate3D {
  public:
    /**
     * Default constructor: sets the Identity transformation. */
    RotateY3D() : Rotate3D() {}
    
    /**
     * Constructs a rotation around y-axis by angle a. */
    RotateY3D(double a) {
      double cosa = cos(a), sina = sin(a); 
      setTransform(cosa,0,sina,0,  0,1,0,0,  -sina,0,cosa,0);
    }
  };

  /**
   * Constructs a rotation around z-axis.
   * This class provides additional constructors for Transform3D
   * and should not be used as a separate class.
   * 
   * Example of use:
   * @code
   *   Transform3D m;
   *   m = RotateZ3D(30.*deg);
   * @endcode
   *
   * @author <Evgueni.Tcherniaev@cern.ch>
   * @ingroup geometry
   */
  class RotateZ3D : public Rotate3D {
  public:
    /**
     * Default constructor: sets the Identity transformation. */
    RotateZ3D() : Rotate3D() {}
    
    /**
     * Constructs a rotation around z-axis by angle a. */
    RotateZ3D(double a) {
      double cosa = cos(a), sina = sin(a); 
      setTransform(cosa,-sina,0,0,  sina,cosa,0,0,  0,0,1,0);
    }
  };

  //   T R A N S L A T I O N S
  
  /**
   * Constructs a translation transformation.
   * This class provides additional constructors for Transform3D
   * and should not be used as a separate class.
   * 
   * Example of use:
   * @code
   *   Transform3D m;
   *   m = Translate3D(10.,20.,30.);
   * @endcode
   *
   * @author <Evgueni.Tcherniaev@cern.ch>
   * @ingroup geometry
   */
  class Translate3D : public Transform3D {
  public:
    /**
     * Default constructor: sets the Identity transformation. */
    Translate3D() : Transform3D() {}
    
    /**
     * Constructor from CLHEP::Hep3Vector. */
    inline Translate3D(const CLHEP::Hep3Vector &v);
    
    /**
     * Constructor from three numbers. */
    Translate3D(double x, double y, double z)
      : Transform3D(1,0,0,x, 0,1,0,y, 0,0,1,z) {}
  };

  /**
   * Constructs a translation along x-axis.
   * This class provides additional constructors for Transform3D
   * and should not be used as a separate class.
   * 
   * Example of use:
   * @code
   *   Transform3D m;
   *   m = TranslateX3D(10.);
   * @endcode
   *
   * @author <Evgueni.Tcherniaev@cern.ch>
   * @ingroup geometry
   */
  class TranslateX3D : public Translate3D {
  public:
    /**
     * Default constructor: sets the Identity transformation. */
    TranslateX3D() : Translate3D() {}
    
    /**
     * Constructor from a number. */
    TranslateX3D(double x) : Translate3D(x, 0, 0) {}
  };

  /**
   * Constructs a translation along y-axis.
   * This class provides additional constructors for Transform3D
   * and should not be used as a separate class.
   * 
   * Example of use:
   * @code
   *   Transform3D m;
   *   m = TranslateY3D(10.);
   * @endcode
   *
   * @author <Evgueni.Tcherniaev@cern.ch>
   * @ingroup geometry
   */
  class TranslateY3D : public Translate3D {
  public:
    /**
     * Default constructor: sets the Identity transformation. */
    TranslateY3D() : Translate3D() {}

    /**
     * Constructor from a number. */
    TranslateY3D(double y) : Translate3D(0, y, 0) {}
  };

  /**
   * Constructs a translation along z-axis.
   * This class provides additional constructors for Transform3D
   * and should not be used as a separate class.
   * 
   * Example of use:
   * @code
   *   Transform3D m;
   *   m = TranslateZ3D(10.);
   * @endcode
   *
   * @author <Evgueni.Tcherniaev@cern.ch>
   * @ingroup geometry
   */
  class TranslateZ3D : public Translate3D {
  public:
    /**
     * Default constructor: sets the Identity transformation. */
    TranslateZ3D() : Translate3D() {}

    /**
     * Constructor from a number. */
    TranslateZ3D(double z) : Translate3D(0, 0, z) {}
  };

  //   R E F L E C T I O N S

  /**
   * Constructs a reflection transformation.
   * This class provides additional constructors for Transform3D
   * and should not be used as a separate class.
   * 
   * Example of use:
   * @code
   *   Transform3D m;
   *   m = Reflect3D(1.,1.,1.,0.);
   * @endcode
   *
   * @author <Evgueni.Tcherniaev@cern.ch>
   * @ingroup geometry
   */
  class Reflect3D : public Transform3D {
  protected:
    Reflect3D(double XX, double XY, double XZ, double DX,
		 double YX, double YY, double YZ, double DY,
		 double ZX, double ZY, double ZZ, double DZ)
      : Transform3D(XX,XY,XZ,DX, YX,YY,YZ,DY, ZX,ZY,ZZ,DZ) {}

  public:
    /**
     * Default constructor: sets the Identity transformation. */
    Reflect3D() : Transform3D() {}

    /**
     * Constructor from four numbers.
     * Sets reflection in a plane a*x+b*y+c*z+d=0
     */
    Reflect3D(double a, double b, double c, double d);

    /**
     * Constructor from a plane given by its normal and origin. */
    inline Reflect3D(const Normal3D<double> & normal,
			const Point3D<double> & point);
  };

  /**
   * Constructs reflection in a plane x=const.
   * This class provides additional constructors for Transform3D
   * and should not be used as a separate class.
   * 
   * Example of use:
   * @code
   *   Transform3D m;
   *   m = ReflectX3D(1.);
   * @endcode
   *
   * @author <Evgueni.Tcherniaev@cern.ch>
   * @ingroup geometry
   */
  class ReflectX3D : public Reflect3D {
  public:
    /**
     * Constructor from a number. */
    ReflectX3D(double x=0) : Reflect3D(-1,0,0,x+x, 0,1,0,0, 0,0,1,0) {}
  };
 
  /**
   * Constructs reflection in a plane y=const.
   * This class provides additional constructors for Transform3D
   * and should not be used as a separate class.
   * 
   * Example of use:
   * @code
   *   Transform3D m;
   *   m = ReflectY3D(1.);
   * @endcode
   *
   * @author <Evgueni.Tcherniaev@cern.ch>
   * @ingroup geometry
   */
  class ReflectY3D : public Reflect3D {
  public:
    /**
     * Constructor from a number. */
    ReflectY3D(double y=0) : Reflect3D(1,0,0,0, 0,-1,0,y+y, 0,0,1,0) {}
  };
 
  /**
   * Constructs reflection in a plane z=const.
   * This class provides additional constructors for Transform3D
   * and should not be used as a separate class.
   * 
   * Example of use:
   * @code
   *   Transform3D m;
   *   m = ReflectZ3D(1.);
   * @endcode
   *
   * @author <Evgueni.Tcherniaev@cern.ch>
   * @ingroup geometry
   */
  class ReflectZ3D : public Reflect3D {
  public:
    /**
     *  Constructor from a number. */
    ReflectZ3D(double z=0) : Reflect3D(1,0,0,0, 0,1,0,0, 0,0,-1,z+z) {}
  };
 
  //   S C A L I N G S

  /**
   * Constructs a scaling transformation.
   * This class provides additional constructors for Transform3D
   * and should not be used as a separate class.
   * 
   * Example of use:
   * @code
   *   Transform3D m;
   *   m = Scale3D(2.);
   * @endcode
   *
   * @author <Evgueni.Tcherniaev@cern.ch>
   * @ingroup geometry
   */
  class Scale3D : public Transform3D {
  public:
    /**
     * Default constructor: sets the Identity transformation. */
    Scale3D() : Transform3D() {}

    /**
     * Constructor from three numbers - scale factors in different directions.
     */
    Scale3D(double x, double y, double z)
      : Transform3D(x,0,0,0, 0,y,0,0, 0,0,z,0) {}

    /**
     * Constructor from a number: sets uniform scaling in all directions. */
    Scale3D(double s)
      : Transform3D(s,0,0,0, 0,s,0,0, 0,0,s,0) {}
  };

  /**
   * Constructs a scaling transformation in x-direction.
   * This class provides additional constructors for Transform3D
   * and should not be used as a separate class.
   * 
   * Example of use:
   * @code
   *   Transform3D m;
   *   m = ScaleX3D(2.);
   * @endcode
   *
   * @author <Evgueni.Tcherniaev@cern.ch>
   * @ingroup geometry
   */
  class ScaleX3D : public Scale3D {
  public:
    /**
     * Default constructor: sets the Identity transformation. */
    ScaleX3D() : Scale3D() {}

    /**
     * Constructor from a number (scale factor in x-direction). */
    ScaleX3D(double x) : Scale3D(x, 1, 1) {}
  };

  /**
   * Constructs a scaling transformation in y-direction.
   * This class provides additional constructors for Transform3D
   * and should not be used as a separate class.
   * 
   * Example of use:
   * @code
   *   Transform3D m;
   *   m = ScaleY3D(2.);
   * @endcode
   *
   * @author <Evgueni.Tcherniaev@cern.ch>
   * @ingroup geometry
   */
  class ScaleY3D : public Scale3D {
  public:
    /**
     * Default constructor: sets the Identity transformation. */
    ScaleY3D() : Scale3D() {}

    /**
     * Constructor from a number (scale factor in y-direction). */
    ScaleY3D(double y) : Scale3D(1, y, 1) {}
  };

  /**
   * Constructs a scaling transformation in z-direction.
   * This class provides additional constructors for Transform3D
   * and should not be used as a separate class.
   *
   * Example of use:
   * @code
   *   Transform3D m;
   *   m = ScaleZ3D(2.);
   * @endcode
   *
   * @author <Evgueni.Tcherniaev@cern.ch>
   * @ingroup geometry
   */
  class ScaleZ3D : public Scale3D {
  public:
    /**
     * Default constructor: sets the Identity transformation. */
    ScaleZ3D() : Scale3D() {}
    /**
     * Constructor from a number (scale factor in z-direction). */
    ScaleZ3D(double z) : Scale3D(1, 1, z) {}
  };
} /* namespace HepGeom */

#ifdef ENABLE_BACKWARDS_COMPATIBILITY
//  backwards compatibility will be enabled ONLY in CLHEP 1.9
typedef HepGeom::Transform3D  HepTransform3D; 
typedef HepGeom::Rotate3D     HepRotate3D;
typedef HepGeom::RotateX3D    HepRotateX3D;
typedef HepGeom::RotateY3D    HepRotateY3D;
typedef HepGeom::RotateZ3D    HepRotateZ3D;
typedef HepGeom::Translate3D  HepTranslate3D;
typedef HepGeom::TranslateX3D HepTranslateX3D;
typedef HepGeom::TranslateY3D HepTranslateY3D;
typedef HepGeom::TranslateZ3D HepTranslateZ3D;
typedef HepGeom::Reflect3D    HepReflect3D; 
typedef HepGeom::ReflectX3D   HepReflectX3D;
typedef HepGeom::ReflectY3D   HepReflectY3D;
typedef HepGeom::ReflectZ3D   HepReflectZ3D;
typedef HepGeom::Scale3D      HepScale3D;
typedef HepGeom::ScaleX3D     HepScaleX3D;
typedef HepGeom::ScaleY3D     HepScaleY3D;
typedef HepGeom::ScaleZ3D     HepScaleZ3D;
#endif

///#include "CLHEP/Geometry/Transform3D.icc"
#include "./Transform3D.icc"

#endif /* HEP_TRANSFROM3D_H */
