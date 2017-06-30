// -*- C++ -*-
// CLASSDOC OFF
// $Id: LorentzVector.h,v 1.2 2003/10/23 21:29:52 garren Exp $
// ---------------------------------------------------------------------------
// CLASSDOC ON
//
// This file is a part of the CLHEP - a Class Library for High Energy Physics.
//
// HepLorentzVector is a Lorentz vector consisting of Hep3Vector and
// double components. Lorentz transformations (rotations and boosts)
// of these vectors are perfomed by multiplying with objects of
// the HepLorenzRotation class.
//
// .SS See Also
// ThreeVector.h, Rotation.h, LorentzRotation.h
//
// .SS Authors
// Leif Lonnblad and Anders Nilsson. Modified by Evgueni Tcherniaev, Mark Fischler
//

#ifndef HEP_LORENTZVECTOR_H
#define HEP_LORENTZVECTOR_H

#ifdef GNUPRAGMA
#pragma interface
#endif

#include <iostream>
///#include "CLHEP/Vector/defs.h" 
///#include "CLHEP/Vector/ThreeVector.h"
#include "../vector/defs.h" 
#include "../vector/ThreeVector.h"

namespace CLHEP {

// Declarations of classes and global methods
class HepLorentzVector;
class HepLorentzRotation;
class HepRotation;
class HepAxisAngle;
class HepEulerAngles;
class Tcomponent;
HepLorentzVector rotationXOf( const HepLorentzVector & vec, double delta );
HepLorentzVector rotationYOf( const HepLorentzVector & vec, double delta );
HepLorentzVector rotationZOf( const HepLorentzVector & vec, double delta );
HepLorentzVector rotationOf
    ( const HepLorentzVector & vec, const Hep3Vector & axis, double delta );
HepLorentzVector rotationOf
    ( const HepLorentzVector & vec, const HepAxisAngle & ax );
HepLorentzVector rotationOf
    ( const HepLorentzVector & vec, const HepEulerAngles & e );
HepLorentzVector rotationOf
    ( const HepLorentzVector & vec, double phi,
                                    double theta,
                                    double psi );
inline 
HepLorentzVector  boostXOf( const HepLorentzVector & vec, double beta );
inline 
HepLorentzVector  boostYOf( const HepLorentzVector & vec, double beta );
inline 
HepLorentzVector  boostZOf( const HepLorentzVector & vec, double beta );
inline HepLorentzVector  boostOf
    ( const HepLorentzVector & vec, const Hep3Vector & betaVector );
inline HepLorentzVector  boostOf
    ( const HepLorentzVector & vec, const Hep3Vector & axis,  double beta );

enum ZMpvMetric_t { TimePositive, TimeNegative };


/**
 * @author
 * @ingroup vector
 */
class HepLorentzVector {

public:

  enum { X=0, Y=1, Z=2, T=3, NUM_COORDINATES=4, SIZE=NUM_COORDINATES };
  // Safe indexing of the coordinates when using with matrices, arrays, etc.
  // (BaBar)

  inline HepLorentzVector(double x, double y,
			  double z, double t);
  // Constructor giving the components x, y, z, t.

  inline HepLorentzVector(double x, double y, double z);
  // Constructor giving the components x, y, z with t-component set to 0.0.

  inline HepLorentzVector(double t);
  // Constructor giving the t-component with x, y and z set to 0.0.

  inline HepLorentzVector();
  // Default constructor with x, y, z and t set to 0.0.

  inline HepLorentzVector(const Hep3Vector & p, double e);
  inline HepLorentzVector(double e, const Hep3Vector & p);
  // Constructor giving a 3-Vector and a time component.

  inline HepLorentzVector(const HepLorentzVector &);
  // Copy constructor.

  inline ~HepLorentzVector();
  // The destructor.

  inline operator const Hep3Vector & () const;
  inline operator Hep3Vector & ();
  // Conversion (cast) to Hep3Vector.

  inline double x() const;
  inline double y() const;
  inline double z() const;
  inline double t() const;
  // Get position and time.

  inline void setX(double);
  inline void setY(double);
  inline void setZ(double);
  inline void setT(double);
  // Set position and time.

  inline double px() const;
  inline double py() const;
  inline double pz() const;
  inline double e() const;
  // Get momentum and energy.

  inline void setPx(double);
  inline void setPy(double);
  inline void setPz(double);
  inline void setE(double);
  // Set momentum and energy.

  inline Hep3Vector vect() const;
  // Get spatial component. 

  inline void setVect(const Hep3Vector &);
  // Set spatial component. 

  inline double theta() const;
  inline double cosTheta() const;
  inline double phi() const;
  inline double rho() const;
  // Get spatial vector components in spherical coordinate system.

  inline void setTheta(double);
  inline void setPhi(double);
  inline void setRho(double);
  // Set spatial vector components in spherical coordinate system.

  double operator () (int) const;
  inline double operator [] (int) const;
  // Get components by index.

  double & operator () (int);
  inline double & operator [] (int);
  // Set components by index.

  inline HepLorentzVector & operator = (const HepLorentzVector &);
  // Assignment. 

  inline HepLorentzVector   operator +  (const HepLorentzVector &) const;
  inline HepLorentzVector & operator += (const HepLorentzVector &);
  // Additions.

  inline HepLorentzVector   operator -  (const HepLorentzVector &) const;
  inline HepLorentzVector & operator -= (const HepLorentzVector &);
  // Subtractions.

  inline HepLorentzVector operator - () const;
  // Unary minus.

  inline HepLorentzVector & operator *= (double);
         HepLorentzVector & operator /= (double);
  // Scaling with real numbers.

  inline bool operator == (const HepLorentzVector &) const;
  inline bool operator != (const HepLorentzVector &) const;
  // Comparisons.

  inline double perp2() const;
  // Transverse component of the spatial vector squared.

  inline double perp() const;
  // Transverse component of the spatial vector (R in cylindrical system).

  inline void setPerp(double);
  // Set the transverse component of the spatial vector.

  inline double perp2(const Hep3Vector &) const;
  // Transverse component of the spatial vector w.r.t. given axis squared.

  inline double perp(const Hep3Vector &) const;
  // Transverse component of the spatial vector w.r.t. given axis.

  inline double angle(const Hep3Vector &) const;
  // Angle wrt. another vector.

  inline double mag2() const;
  // Dot product of 4-vector with itself. 
  // By default the metric is TimePositive, and mag2() is the same as m2().

  inline double m2() const;
  // Invariant mass squared.

  inline double mag() const;
  inline double m() const;
  // Invariant mass. If m2() is negative then -sqrt(-m2()) is returned.

  inline double mt2() const;
  // Transverse mass squared.

  inline double mt() const;
  // Transverse mass.

  inline double et2() const;
  // Transverse energy squared.

  inline double et() const;
  // Transverse energy.

  inline double dot(const HepLorentzVector &) const;
  inline double operator * (const HepLorentzVector &) const;
  // Scalar product.

  inline double invariantMass2( const HepLorentzVector & w ) const;
  // Invariant mass squared of pair of 4-vectors 

  double invariantMass ( const HepLorentzVector & w ) const;
  // Invariant mass of pair of 4-vectors 

  inline void setVectMag(const Hep3Vector & spatial, double magnitude);
  inline void setVectM(const Hep3Vector & spatial, double mass);
  // Copy spatial coordinates, and set energy = sqrt(mass^2 + spatial^2)

  inline double plus() const;
  inline double minus() const;
  // Returns the positive/negative light-cone component t +/- z.

  Hep3Vector boostVector() const;
  // Boost needed from rest4Vector in rest frame to form this 4-vector
  // Returns the spatial components divided by the time component.

  HepLorentzVector & boost(double, double, double);
  inline HepLorentzVector & boost(const Hep3Vector &);
  // Lorentz boost.

  HepLorentzVector & boostX( double beta );
  HepLorentzVector & boostY( double beta );
  HepLorentzVector & boostZ( double beta );
  // Boost along an axis, by magnitue beta (fraction of speed of light)

  double rapidity() const;
  // Returns the rapidity, i.e. 0.5*ln((E+pz)/(E-pz))

  inline double pseudoRapidity() const;
  // Returns the pseudo-rapidity, i.e. -ln(tan(theta/2))

  inline bool isTimelike() const;
  // Test if the 4-vector is timelike

  inline bool isSpacelike() const;
  // Test if the 4-vector is spacelike

  inline bool isLightlike(double epsilon=tolerance) const;
  // Test for lightlike is within tolerance epsilon

  HepLorentzVector &  rotateX(double);
  // Rotate the spatial component around the x-axis.

  HepLorentzVector &  rotateY(double);
  // Rotate the spatial component around the y-axis.

  HepLorentzVector &  rotateZ(double);
  // Rotate the spatial component around the z-axis.

  HepLorentzVector &  rotateUz(const Hep3Vector &);
  // Rotates the reference frame from Uz to newUz (unit vector).

  HepLorentzVector & rotate(double, const Hep3Vector &);
  // Rotate the spatial component around specified axis.

  inline HepLorentzVector & operator *= (const HepRotation &);
  inline HepLorentzVector & transform(const HepRotation &);
  // Transformation with HepRotation.

  HepLorentzVector & operator *= (const HepLorentzRotation &);
  HepLorentzVector & transform(const HepLorentzRotation &);
  // Transformation with HepLorenzRotation.

// = = = = = = = = = = = = = = = = = = = = = = = =
//
// Esoteric properties and operations on 4-vectors:  
//
// 0 - Flexible metric convention and axial unit 4-vectors
// 1 - Construct and set 4-vectors in various ways 
// 2 - Synonyms for accessing coordinates and properties
// 2a - Setting space coordinates in different ways 
// 3 - Comparisions (dictionary, near-ness, and geometric)
// 4 - Intrinsic properties 
// 4a - Releativistic kinematic properties 
// 4b - Methods combining two 4-vectors
// 5 - Properties releative to z axis and to arbitrary directions
// 7 - Rotations and Boosts
//
// = = = = = = = = = = = = = = = = = = = = = = = =

// 0 - Flexible metric convention 

  static ZMpvMetric_t setMetric( ZMpvMetric_t m );
  static ZMpvMetric_t getMetric();

// 1 - Construct and set 4-vectors in various ways 

  inline void set        (double x, double y, double z, double  t);
  inline void set        (double x, double y, double z, Tcomponent t);
  inline HepLorentzVector(double x, double y, double z, Tcomponent t);
  // Form 4-vector by supplying cartesian coordinate components

  inline void set        (Tcomponent t, double x, double y, double z);
  inline HepLorentzVector(Tcomponent t, double x, double y, double z);
  // Deprecated because the 4-doubles form uses x,y,z,t, not t,x,y,z.

  inline void set                 ( double t );

  inline void set                 ( Tcomponent t );
  inline explicit HepLorentzVector( Tcomponent t );
  // Form 4-vector with zero space components, by supplying t component

  inline void set                 ( const Hep3Vector & v );
  inline explicit HepLorentzVector( const Hep3Vector & v );
  // Form 4-vector with zero time component, by supplying space 3-vector 

  inline HepLorentzVector & operator=( const Hep3Vector & v );
  // Form 4-vector with zero time component, equal to space 3-vector 

  inline void set ( const Hep3Vector & v, double t );
  inline void set ( double t, const Hep3Vector & v );
  // Set using specified space vector and time component

// 2 - Synonyms for accessing coordinates and properties

  inline double getX() const;
  inline double getY() const;
  inline double getZ() const;
  inline double getT() const;
  // Get position and time.

  inline Hep3Vector v() const;
  inline Hep3Vector getV() const;
  // Get spatial component.   Same as vect.

  inline void setV(const Hep3Vector &);
  // Set spatial component.   Same as setVect.

// 2a - Setting space coordinates in different ways 

  inline void setV( double x, double y, double z );

  inline void setRThetaPhi( double r, double theta, double phi);
  inline void setREtaPhi( double r, double eta, double phi);
  inline void setRhoPhiZ( double rho, double phi, double z );

// 3 - Comparisions (dictionary, near-ness, and geometric)

  int compare( const HepLorentzVector & w ) const;

  bool operator >( const HepLorentzVector & w ) const;
  bool operator <( const HepLorentzVector & w ) const;
  bool operator>=( const HepLorentzVector & w ) const;
  bool operator<=( const HepLorentzVector & w ) const;

  bool   isNear ( const HepLorentzVector & w, 
					double epsilon=tolerance ) const;
  double howNear( const HepLorentzVector & w ) const;
  // Is near using Euclidean measure t**2 + v**2

  bool   isNearCM ( const HepLorentzVector & w, 
					double epsilon=tolerance ) const;
  double howNearCM( const HepLorentzVector & w ) const;
  // Is near in CM frame:  Applicable only for two timelike HepLorentzVectors

        // If w1 and w2 are already in their CM frame, then w1.isNearCM(w2)
        // is exactly equivalent to w1.isNear(w2).
        // If w1 and w2 have T components of zero, w1.isNear(w2) is exactly
        // equivalent to w1.getV().isNear(w2.v()).  

  bool isParallel( const HepLorentzVector & w, 
					double epsilon=tolerance ) const;
  // Test for isParallel is within tolerance epsilon
  double howParallel (const HepLorentzVector & w) const;

  static double getTolerance();
  static double setTolerance( double tol );
  // Set the tolerance for HepLorentzVectors to be considered near
  // The same tolerance is used for determining isLightlike, and isParallel

  double deltaR(const HepLorentzVector & v) const;
  // sqrt ( (delta eta)^2 + (delta phi)^2 ) of space part

// 4 - Intrinsic properties 

         double howLightlike() const;
  // Close to zero for almost lightlike 4-vectors; up to 1.

  inline double euclideanNorm2()  const;
  // Sum of the squares of time and space components; not Lorentz invariant. 

  inline double euclideanNorm()  const; 
  // Length considering the metric as (+ + + +); not Lorentz invariant.


// 4a - Relativistic kinematic properties 

// All Relativistic kinematic properties are independent of the sense of metric

  inline double restMass2() const;
  inline double invariantMass2() const; 
  // Rest mass squared -- same as m2()

  inline double restMass() const;
  inline double invariantMass() const; 
  // Same as m().  If m2() is negative then -sqrt(-m2()) is returned.

// The following properties are rest-frame related, 
// and are applicable only to non-spacelike 4-vectors

  HepLorentzVector rest4Vector() const;
  // This 4-vector, boosted into its own rest frame:  (0, 0, 0, m()) 
          // The following relation holds by definition:
          // w.rest4Vector().boost(w.boostVector()) == w

  // Beta and gamma of the boost vector
///  double beta() const;//hepboost
  // Relativistic beta of the boost vector

///  double gamma() const;//hepboost
  // Relativistic gamma of the boost vector

  inline double eta() const;
  // Pseudorapidity (of the space part)

  inline double eta(const Hep3Vector & ref) const;
  // Pseudorapidity (of the space part) w.r.t. specified direction

  double rapidity(const Hep3Vector & ref) const;
  // Rapidity in specified direction

  double coLinearRapidity() const;
  // Rapidity, in the relativity textbook sense:  atanh (|P|/E)

  Hep3Vector findBoostToCM() const;
  // Boost needed to get to center-of-mass  frame:
          // w.findBoostToCM() == - w.boostVector()
          // w.boost(w.findBoostToCM()) == w.rest4Vector()

  Hep3Vector findBoostToCM( const HepLorentzVector & w ) const;
  // Boost needed to get to combined center-of-mass frame:
          // w1.findBoostToCM(w2) == w2.findBoostToCM(w1)
          // w.findBoostToCM(w) == w.findBoostToCM()

  inline double et2(const Hep3Vector &) const;
  // Transverse energy w.r.t. given axis squared.

  inline double et(const Hep3Vector &) const;
  // Transverse energy w.r.t. given axis.

// 4b - Methods combining two 4-vectors

  inline double diff2( const HepLorentzVector & w ) const;
  // (this - w).dot(this-w); sign depends on metric choice

  inline double delta2Euclidean ( const HepLorentzVector & w ) const;
  // Euclidean norm of differnce:  (delta_T)^2  + (delta_V)^2

// 5 - Properties releative to z axis and to arbitrary directions

  double  plus(  const Hep3Vector & ref ) const;
  // t + projection in reference direction

  double  minus( const Hep3Vector & ref ) const;
  // t - projection in reference direction

// 7 - Rotations and boosts

  HepLorentzVector & rotate ( const Hep3Vector & axis, double delta );
  // Same as rotate (delta, axis)

  HepLorentzVector & rotate ( const HepAxisAngle & ax );
  HepLorentzVector & rotate ( const HepEulerAngles & e );
  HepLorentzVector & rotate ( double phi,
                              double theta,
                              double psi );
  // Rotate using these HepEuler angles - see Goldstein page 107 for conventions

  HepLorentzVector & boost ( const Hep3Vector & axis,  double beta );
  // Normalizes the Hep3Vector to define a direction, and uses beta to
  // define the magnitude of the boost.

  friend HepLorentzVector rotationXOf
    ( const HepLorentzVector & vec, double delta );
  friend HepLorentzVector rotationYOf
    ( const HepLorentzVector & vec, double delta );
  friend HepLorentzVector rotationZOf
    ( const HepLorentzVector & vec, double delta );
  friend HepLorentzVector rotationOf
    ( const HepLorentzVector & vec, const Hep3Vector & axis, double delta );
  friend HepLorentzVector rotationOf
    ( const HepLorentzVector & vec, const HepAxisAngle & ax );
  friend HepLorentzVector rotationOf
    ( const HepLorentzVector & vec, const HepEulerAngles & e );
  friend HepLorentzVector rotationOf
    ( const HepLorentzVector & vec, double phi,
                                    double theta,
                                    double psi );

  inline friend HepLorentzVector  boostXOf
    ( const HepLorentzVector & vec, double beta );
  inline friend HepLorentzVector  boostYOf
    ( const HepLorentzVector & vec, double beta );
  inline friend HepLorentzVector  boostZOf
    ( const HepLorentzVector & vec, double beta );
  inline friend HepLorentzVector  boostOf
    ( const HepLorentzVector & vec, const Hep3Vector & betaVector );
  inline friend HepLorentzVector  boostOf
    ( const HepLorentzVector & vec, const Hep3Vector & axis,  double beta );
 
private:

  Hep3Vector pp;
  double  ee;

  static double tolerance;
  static double metric;

};  // HepLorentzVector

// 8 - Axial Unit 4-vectors

static const HepLorentzVector X_HAT4 = HepLorentzVector( 1, 0, 0, 0 );
static const HepLorentzVector Y_HAT4 = HepLorentzVector( 0, 1, 0, 0 );
static const HepLorentzVector Z_HAT4 = HepLorentzVector( 0, 0, 1, 0 );
static const HepLorentzVector T_HAT4 = HepLorentzVector( 0, 0, 0, 1 );

// Global methods

std::ostream & operator << (std::ostream &, const HepLorentzVector &);
// Output to a stream.

std::istream & operator >> (std::istream &, HepLorentzVector &);
// Input from a stream.

typedef HepLorentzVector HepLorentzVectorD;
typedef HepLorentzVector HepLorentzVectorF;

inline HepLorentzVector operator * (const HepLorentzVector &, double a);
inline HepLorentzVector operator * (double a, const HepLorentzVector &);
// Scaling LorentzVector with a real number

       HepLorentzVector operator / (const HepLorentzVector &, double a);
// Dividing LorentzVector by a real number

// Tcomponent definition:

// Signature protection for 4-vector constructors taking 4 components
class Tcomponent {
private:
  double t_;
public:
  explicit Tcomponent(double t) : t_(t) {}
  operator double() const { return t_; }
};  // Tcomponent

}  // namespace CLHEP

//#include "CLHEP/Vector/LorentzVector.icc"
#include "../vector/LorentzVector.icc"

#ifdef ENABLE_BACKWARDS_COMPATIBILITY
//  backwards compatibility will be enabled ONLY in CLHEP 1.9
using namespace CLHEP;
#endif

#endif /* HEP_LORENTZVECTOR_H */
