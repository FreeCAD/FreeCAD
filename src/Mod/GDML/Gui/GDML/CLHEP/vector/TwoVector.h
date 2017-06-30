// -*- C++ -*-
// CLASSDOC OFF
// ---------------------------------------------------------------------------
// CLASSDOC ON
//
// This file is a part of the CLHEP - a Class Library for High Energy Physics.
//
// Hep2Vector is a general 2-vector class defining vectors in two 
// dimension using double components.   It comes from the ZOOM
// PlaneVector class (the PhysicsVectors PlaneVector.h will typedef
// PlaneVector to Hep2Vector).
//
// .SS See Also
// ThreeVector.h
//
// .SS Authors
// John Marraffino and Mark Fischler
//

#ifndef HEP_TWOVECTOR_H
#define HEP_TWOVECTOR_H

#ifdef GNUPRAGMA
#pragma interface
#endif

#include <iostream>

///#include "CLHEP/Vector/defs.h" 
///#include "CLHEP/Vector/ThreeVector.h" 
#include "defs.h" 
#include "ThreeVector.h" 

namespace CLHEP {

// Declarations of classes and global methods
class Hep2Vector;
std::ostream & operator << (std::ostream &, const Hep2Vector &);
std::istream & operator >> (std::istream &, Hep2Vector &);
inline double operator * (const Hep2Vector & a,const Hep2Vector & b);
inline Hep2Vector operator * (const Hep2Vector & p, double a);
inline Hep2Vector operator * (double a, const Hep2Vector & p);
       Hep2Vector operator / (const Hep2Vector & p, double a);
inline Hep2Vector operator + (const Hep2Vector & a, const Hep2Vector & b);
inline Hep2Vector operator - (const Hep2Vector & a, const Hep2Vector & b);

/**
 * @author
 * @ingroup vector
 */
class Hep2Vector {

public:

  enum { X=0, Y=1, NUM_COORDINATES=2, SIZE=NUM_COORDINATES };
  // Safe indexing of the coordinates when using with matrices, arrays, etc.

  inline Hep2Vector( double x = 0.0, double y = 0.0 );
  // The constructor.

  inline Hep2Vector(const Hep2Vector & p);
  // The copy constructor.

  explicit Hep2Vector( const Hep3Vector & s);
  // "demotion" constructor"
  // WARNING -- THIS IGNORES THE Z COMPONENT OF THE Hep3Vector.
  //		SO IN GENERAL, Hep2Vector(v)==v WILL NOT HOLD!

  inline ~Hep2Vector();
  // The destructor.

  inline double x() const;
  inline double y() const;
  // The components in cartesian coordinate system.

         double operator () (int i) const;
  inline double operator [] (int i) const;
  // Get components by index.  0-based.

         double & operator () (int i);
  inline double & operator [] (int i);
  // Set components by index.  0-based.

  inline void setX(double x);
  inline void setY(double y);
  inline void set (double x, double y);
  // Set the components in cartesian coordinate system.

  inline double phi() const;
  // The azimuth angle.

  inline double mag2() const;
  // The magnitude squared.

  inline double mag() const;
  // The magnitude.

  inline double r() const;
  // r in polar coordinates (r, phi):  equal to mag().

  inline void setPhi(double phi);
  // Set phi keeping mag constant.

  inline void setMag(double r);
  // Set magnitude keeping phi constant.

  inline void setR(double r);
  // Set R keeping phi constant.  Same as setMag.

  inline void setPolar(double r, double phi);
  // Set by polar coordinates.

  inline Hep2Vector & operator = (const Hep2Vector & p);
  // Assignment.

  inline bool operator == (const Hep2Vector & v) const;
  inline bool operator != (const Hep2Vector & v) const;
  // Comparisons.

  int compare (const Hep2Vector & v) const;
  bool operator > (const Hep2Vector & v) const;
  bool operator < (const Hep2Vector & v) const;
  bool operator>= (const Hep2Vector & v) const;
  bool operator<= (const Hep2Vector & v) const;
  // dictionary ordering according to y, then x component

  static inline double getTolerance();
  static double setTolerance(double tol);

  double howNear (const Hep2Vector &p) const;
  bool isNear  (const Hep2Vector & p, double epsilon=tolerance) const;

  double howParallel (const Hep2Vector &p) const;
  bool isParallel 
		(const Hep2Vector & p, double epsilon=tolerance) const;

  double howOrthogonal (const Hep2Vector &p) const;
  bool isOrthogonal
		(const Hep2Vector & p, double epsilon=tolerance) const;

  inline Hep2Vector & operator += (const Hep2Vector &p);
  // Addition.

  inline Hep2Vector & operator -= (const Hep2Vector &p);
  // Subtraction.

  inline Hep2Vector operator - () const;
  // Unary minus.

  inline Hep2Vector & operator *= (double a);
  // Scaling with real numbers.

  inline Hep2Vector unit() const;
  // Unit vector parallel to this.

  inline Hep2Vector orthogonal() const;
  // Vector orthogonal to this.

  inline double dot(const Hep2Vector &p) const;
  // Scalar product.

  inline double angle(const Hep2Vector &) const;
  // The angle w.r.t. another 2-vector.

  void rotate(double);
  // Rotates the Hep2Vector.

  operator Hep3Vector () const;
  // Cast a Hep2Vector as a Hep3Vector.

  // The remaining methods are friends, thus defined at global scope:
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  friend std::ostream & operator<< (std::ostream &, const Hep2Vector &);
  // Output to a stream.

  inline friend double operator * (const Hep2Vector & a,
				   const Hep2Vector & b);
  // Scalar product.

  inline friend Hep2Vector operator * (const Hep2Vector & p, double a);
  // v*c

  inline friend Hep2Vector operator * (double a, const Hep2Vector & p);
  // c*v

         friend Hep2Vector operator / (const Hep2Vector & p, double a);
  // v/c

  inline friend Hep2Vector operator + (const Hep2Vector & a,
				       const Hep2Vector & b);
  // v1+v2

  inline friend Hep2Vector operator - (const Hep2Vector & a,
				        const Hep2Vector & b);
  // v1-v2

  enum { ZMpvToleranceTicks = 100 };

private:

  double dx;
  double dy;
  // The components.

  static double tolerance;
  // default tolerance criterion for isNear() to return true.

};  // Hep2Vector

static const Hep2Vector X_HAT2(1.0, 0.0);
static const Hep2Vector Y_HAT2(0.0, 1.0);

}  // namespace CLHEP

///#include "CLHEP/Vector/TwoVector.icc"
#include "TwoVector.icc"

#ifdef ENABLE_BACKWARDS_COMPATIBILITY
//  backwards compatibility will be enabled ONLY in CLHEP 1.9
using namespace CLHEP;
#endif


#endif /* HEP_TWOVECTOR_H */
