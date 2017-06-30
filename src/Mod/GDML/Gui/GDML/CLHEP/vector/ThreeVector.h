// -*- C++ -*-
// CLASSDOC OFF
// $Id: ThreeVector.h,v 1.2.4.1.2.2 2009/11/11 23:18:26 garren Exp $
// ---------------------------------------------------------------------------
// CLASSDOC ON
//
// This file is a part of the CLHEP - a Class Library for High Energy Physics.
//
// Hep3Vector is a general 3-vector class defining vectors in three
// dimension using double components. Rotations of these vectors are
// performed by multiplying with an object of the HepRotation class.
//
// .SS See Also
// LorentzVector.h, Rotation.h, LorentzRotation.h 
//
// .SS Authors
// Leif Lonnblad and Anders Nilsson; Modified by Evgueni Tcherniaev;
// ZOOM additions by Mark Fischler
//

#ifndef HEP_THREEVECTOR_H
#define HEP_THREEVECTOR_H

#ifdef GNUPRAGMA
#pragma interface
#endif

#include <iostream>
///#include "CLHEP/Vector/defs.h" 
#include "defs.h" 

namespace CLHEP {

class HepRotation;
class HepEulerAngles;
class HepAxisAngle;

/**
 * @author
 * @ingroup vector
 */
class Hep3Vector {

public:

// Basic properties and operations on 3-vectors:  

  enum { X=0, Y=1, Z=2, NUM_COORDINATES=3, SIZE=NUM_COORDINATES };
  // Safe indexing of the coordinates when using with matrices, arrays, etc.
  // (BaBar)

  inline Hep3Vector(double x = 0.0, double y = 0.0, double z = 0.0);
  // The constructor.  

  inline Hep3Vector(const Hep3Vector &);
  // The copy constructor.

  inline ~Hep3Vector();
  // The destructor.  Not virtual - inheritance from this class is dangerous.

  double operator () (int) const;
  // Get components by index -- 0-based (Geant4) 

  inline double operator [] (int) const;
  // Get components by index -- 0-based (Geant4) 

  double & operator () (int);
  // Set components by index.  0-based.

  inline double & operator [] (int);
  // Set components by index.  0-based.

  inline double x() const;
  inline double y() const;
  inline double z() const;
  // The components in cartesian coordinate system.  Same as getX() etc.

  inline void setX(double);
  inline void setY(double);
  inline void setZ(double);
  // Set the components in cartesian coordinate system.

  inline void set( double x, double y, double z); 
  // Set all three components in cartesian coordinate system.

  inline double phi() const;
  // The azimuth angle.

  inline double theta() const;
  // The polar angle.

  inline double cosTheta() const;
  // Cosine of the polar angle.

  inline double cos2Theta() const;
  // Cosine squared of the polar angle - faster than cosTheta(). (ZOOM)

  inline double mag2() const;
  // The magnitude squared (r^2 in spherical coordinate system).

  inline double mag() const;
  // The magnitude (r in spherical coordinate system).

  inline void setPhi(double);
  // Set phi keeping mag and theta constant (BaBar).

  inline void setTheta(double);
  // Set theta keeping mag and phi constant (BaBar).

         void setMag(double);
  // Set magnitude keeping theta and phi constant (BaBar).

  inline double perp2() const;
  // The transverse component squared (rho^2 in cylindrical coordinate system).

  inline double perp() const;
  // The transverse component (rho in cylindrical coordinate system).

  inline void setPerp(double);
  // Set the transverse component keeping phi and z constant.

  void setCylTheta(double);
  // Set theta while keeping transvers component and phi fixed 

  inline double perp2(const Hep3Vector &) const;
  // The transverse component w.r.t. given axis squared.

  inline double perp(const Hep3Vector &) const;
  // The transverse component w.r.t. given axis.

  inline Hep3Vector & operator = (const Hep3Vector &);
  // Assignment.

  inline bool operator == (const Hep3Vector &) const;
  inline bool operator != (const Hep3Vector &) const;
  // Comparisons (Geant4). 

  bool isNear (const Hep3Vector &, double epsilon=tolerance) const;
  // Check for equality within RELATIVE tolerance (default 2.2E-14). (ZOOM)
  // |v1 - v2|**2 <= epsilon**2 * |v1.dot(v2)| 

  double howNear(const Hep3Vector & v ) const;
  // sqrt ( |v1-v2|**2 / v1.dot(v2) ) with a maximum of 1.
  // If v1.dot(v2) is negative, will return 1.

  double deltaR(const Hep3Vector & v) const;
  // sqrt( pseudorapity_difference**2 + deltaPhi **2 )

  inline Hep3Vector & operator += (const Hep3Vector &);
  // Addition.

  inline Hep3Vector & operator -= (const Hep3Vector &);
  // Subtraction.

  inline Hep3Vector operator - () const;
  // Unary minus.

  inline Hep3Vector & operator *= (double);
  // Scaling with real numbers.

         Hep3Vector & operator /= (double);
  // Division by (non-zero) real number.

  inline Hep3Vector unit() const;
  // Vector parallel to this, but of length 1.

  inline Hep3Vector orthogonal() const;
  // Vector orthogonal to this (Geant4).

  inline double dot(const Hep3Vector &) const;
  // double product.

  inline Hep3Vector cross(const Hep3Vector &) const;
  // Cross product.

  double angle(const Hep3Vector &) const;
  // The angle w.r.t. another 3-vector.

  double pseudoRapidity() const;
  // Returns the pseudo-rapidity, i.e. -ln(tan(theta/2))

  void setEta  ( double p );
  // Set pseudo-rapidity, keeping magnitude and phi fixed.  (ZOOM)

  void setCylEta  ( double p );
  // Set pseudo-rapidity, keeping transverse component and phi fixed.  (ZOOM)

  Hep3Vector & rotateX(double);
  // Rotates the Hep3Vector around the x-axis.

  Hep3Vector & rotateY(double);
  // Rotates the Hep3Vector around the y-axis.

  Hep3Vector & rotateZ(double);
  // Rotates the Hep3Vector around the z-axis.

  Hep3Vector & rotateUz(const Hep3Vector&);
  // Rotates reference frame from Uz to newUz (unit vector) (Geant4).

    Hep3Vector & rotate(double, const Hep3Vector &);
  // Rotates around the axis specified by another Hep3Vector.
  // (Uses methods of HepRotation, forcing linking in of Rotation.cc.)

  Hep3Vector & operator *= (const HepRotation &);
  Hep3Vector & transform(const HepRotation &);
  // Transformation with a Rotation matrix.


// = = = = = = = = = = = = = = = = = = = = = = = =
//
// Esoteric properties and operations on 3-vectors:  
//
// 1 - Set vectors in various coordinate systems
// 2 - Synonyms for accessing coordinates and properties
// 3 - Comparisions (dictionary, near-ness, and geometric)
// 4 - Intrinsic properties 
// 5 - Properties releative to z axis and arbitrary directions
// 6 - Polar and azimuthal angle decomposition and deltaPhi
// 7 - Rotations 
//
// = = = = = = = = = = = = = = = = = = = = = = = =

// 1 - Set vectors in various coordinate systems

  inline void setRThetaPhi  (double r, double theta, double phi);
  // Set in spherical coordinates:  Angles are measured in RADIANS

  inline void setREtaPhi  ( double r, double eta,  double phi );
  // Set in spherical coordinates, but specify peudorapidiy to determine theta.

  inline void setRhoPhiZ   (double rho, double phi, double z);
  // Set in cylindrical coordinates:  Phi angle is measured in RADIANS

  void setRhoPhiTheta ( double rho, double phi, double theta);
  // Set in cylindrical coordinates, but specify theta to determine z.

  void setRhoPhiEta ( double rho, double phi, double eta);
  // Set in cylindrical coordinates, but specify pseudorapidity to determine z.

// 2 - Synonyms for accessing coordinates and properties

  inline double getX() const; 
  inline double getY() const;
  inline double getZ() const; 
  // x(), y(), and z()

  inline double getR    () const;
  inline double getTheta() const;
  inline double getPhi  () const;
  // mag(), theta(), and phi()

  inline double r       () const;
  // mag()

  inline double rho     () const;
  inline double getRho  () const;
  // perp()

  double eta     () const;
  double getEta  () const;
  // pseudoRapidity() 

  inline void setR ( double s );
  // setMag()

  inline void setRho ( double s );
  // setPerp()

// 3 - Comparisions (dictionary, near-ness, and geometric)

  int compare (const Hep3Vector & v) const;
  bool operator > (const Hep3Vector & v) const;
  bool operator < (const Hep3Vector & v) const;
  bool operator>= (const Hep3Vector & v) const;
  bool operator<= (const Hep3Vector & v) const;
  // dictionary ordering according to z, then y, then x component

  inline double diff2 (const Hep3Vector & v) const;
  // |v1-v2|**2

  static double setTolerance (double tol);
  static inline double getTolerance ();
  // Set the tolerance used in isNear() for Hep3Vectors 

  bool isParallel (const Hep3Vector & v, double epsilon=tolerance) const;
  // Are the vectors parallel, within the given tolerance?

  bool isOrthogonal (const Hep3Vector & v, double epsilon=tolerance) const;
  // Are the vectors orthogonal, within the given tolerance?

  double howParallel   (const Hep3Vector & v) const;
  // | v1.cross(v2) / v1.dot(v2) |, to a maximum of 1.

  double howOrthogonal (const Hep3Vector & v) const;
  // | v1.dot(v2) / v1.cross(v2) |, to a maximum of 1.

  enum { ToleranceTicks = 100 };

// 4 - Intrinsic properties 

  double beta    () const;
  // relativistic beta (considering v as a velocity vector with c=1)
  // Same as mag() but will object if >= 1

  double gamma() const;
  // relativistic gamma (considering v as a velocity vector with c=1)

  double coLinearRapidity() const;
  // inverse tanh (beta)

// 5 - Properties relative to Z axis and to an arbitrary direction

	  // Note that the non-esoteric CLHEP provides 
	  // theta(), cosTheta(), cos2Theta, and angle(const Hep3Vector&)

  inline double angle() const;
  // angle against the Z axis -- synonym for theta()

  inline double theta(const Hep3Vector & v2) const;  
  // synonym for angle(v2)

  double cosTheta (const Hep3Vector & v2) const;
  double cos2Theta(const Hep3Vector & v2) const;
  // cos and cos^2 of the angle between two vectors

  inline Hep3Vector project () const;
         Hep3Vector project (const Hep3Vector & v2) const;
  // projection of a vector along a direction.  

  inline Hep3Vector perpPart() const;
  inline Hep3Vector perpPart (const Hep3Vector & v2) const;
  // vector minus its projection along a direction.

  double rapidity () const;
  // inverse tanh(v.z())

  double rapidity (const Hep3Vector & v2) const;
  // rapidity with respect to specified direction:  
  // inverse tanh (v.dot(u)) where u is a unit in the direction of v2

  double eta(const Hep3Vector & v2) const;
  // - ln tan of the angle beween the vector and the ref direction.

// 6 - Polar and azimuthal angle decomposition and deltaPhi

  // Decomposition of an angle within reference defined by a direction:

  double polarAngle (const Hep3Vector & v2) const;
  // The reference direction is Z: the polarAngle is abs(v.theta()-v2.theta()).

  double deltaPhi (const Hep3Vector & v2) const;
  // v.phi()-v2.phi(), brought into the range (-PI,PI]

  double azimAngle  (const Hep3Vector & v2) const;
  // The reference direction is Z: the azimAngle is the same as deltaPhi

  double polarAngle (const Hep3Vector & v2, 
					const Hep3Vector & ref) const;
  // For arbitrary reference direction, 
  // 	polarAngle is abs(v.angle(ref) - v2.angle(ref)).

  double azimAngle  (const Hep3Vector & v2, 
					const Hep3Vector & ref) const;
  // To compute azimangle, project v and v2 into the plane normal to
  // the reference direction.  Then in that plane take the angle going
  // clockwise around the direction from projection of v to that of v2.

// 7 - Rotations 

// These mehtods **DO NOT** use anything in the HepRotation class.
// Thus, use of v.rotate(axis,delta) does not force linking in Rotation.cc.

  Hep3Vector & rotate  (const Hep3Vector & axis, double delta);
  // Synonym for rotate (delta, axis)

  Hep3Vector & rotate  (const HepAxisAngle & ax);
  // HepAxisAngle is a struct holding an axis direction and an angle.

  Hep3Vector & rotate (const HepEulerAngles & e);
  Hep3Vector & rotate (double phi,
                        double theta,
                        double psi);
  // Rotate via Euler Angles. Our Euler Angles conventions are 
  // those of Goldstein Classical Mechanics page 107.

protected:
  void setSpherical (double r, double theta, double phi);
  void setCylindrical (double r, double phi, double z);
  double negativeInfinity() const;

protected:

  double dx;
  double dy;
  double dz;
  // The components.

  static double tolerance;
  // default tolerance criterion for isNear() to return true.
};  // Hep3Vector

// Global Methods

Hep3Vector rotationXOf (const Hep3Vector & vec, double delta);
Hep3Vector rotationYOf (const Hep3Vector & vec, double delta);
Hep3Vector rotationZOf (const Hep3Vector & vec, double delta);

Hep3Vector rotationOf (const Hep3Vector & vec, 
				const Hep3Vector & axis, double delta);
Hep3Vector rotationOf (const Hep3Vector & vec, const HepAxisAngle & ax);

Hep3Vector rotationOf (const Hep3Vector & vec, 
				double phi, double theta, double psi);
Hep3Vector rotationOf (const Hep3Vector & vec, const HepEulerAngles & e);
// Return a new vector based on a rotation of the supplied vector

std::ostream & operator << (std::ostream &, const Hep3Vector &);
// Output to a stream.

std::istream & operator >> (std::istream &, Hep3Vector &);
// Input from a stream.

extern const Hep3Vector HepXHat, HepYHat, HepZHat;

typedef Hep3Vector HepThreeVectorD;
typedef Hep3Vector HepThreeVectorF;

Hep3Vector operator / (const Hep3Vector &, double a);
// Division of 3-vectors by non-zero real number

inline Hep3Vector operator + (const Hep3Vector &, const Hep3Vector &);
// Addition of 3-vectors.

inline Hep3Vector operator - (const Hep3Vector &, const Hep3Vector &);
// Subtraction of 3-vectors.

inline double operator * (const Hep3Vector &, const Hep3Vector &);
// double product of 3-vectors.

inline Hep3Vector operator * (const Hep3Vector &, double a);
inline Hep3Vector operator * (double a, const Hep3Vector &);
// Scaling of 3-vectors with a real number

}  // namespace CLHEP

///#include "CLHEP/Vector/ThreeVector.icc"
#include "ThreeVector.icc"

#ifdef ENABLE_BACKWARDS_COMPATIBILITY
//  backwards compatibility will be enabled ONLY in CLHEP 1.9
using namespace CLHEP;
#endif

#endif /* HEP_THREEVECTOR_H */
