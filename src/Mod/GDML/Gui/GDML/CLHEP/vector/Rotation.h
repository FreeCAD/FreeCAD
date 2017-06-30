// -*- C++ -*-
// CLASSDOC OFF
// $Id: Rotation.h,v 1.3 2003/10/23 21:29:52 garren Exp $
// ---------------------------------------------------------------------------
// CLASSDOC ON
//
// This file is a part of the CLHEP - a Class Library for High Energy Physics.
//
// This is the definition of the HepRotation class for performing rotations
// on objects of the Hep3Vector (and HepLorentzVector) class.
//
// HepRotation is a concrete implementation of Hep3RotationInterface.
//
// .SS See Also
// RotationInterfaces.h
// ThreeVector.h, LorentzVector.h, LorentzRotation.h
//
// .SS Author
// Leif Lonnblad, Mark Fischler

#ifndef HEP_ROTATION_H
#define HEP_ROTATION_H

#ifdef GNUPRAGMA
#pragma interface
#endif

///#include "CLHEP/Vector/defs.h" 
///#include "CLHEP/Vector/RotationInterfaces.h"
///#include "CLHEP/Vector/RotationX.h"
///#include "CLHEP/Vector/RotationY.h"
///#include "CLHEP/Vector/RotationZ.h"
///#include "CLHEP/Vector/LorentzVector.h"
#include "../vector/defs.h" 
#include "../vector/RotationInterfaces.h"
#include "../vector/RotationX.h"
#include "../vector/RotationY.h"
#include "../vector/RotationZ.h"
#include "../vector/LorentzVector.h"

namespace CLHEP {

// Declarations of classes and global methods
class HepRotation;
inline HepRotation inverseOf ( const HepRotation & r );
inline HepRotation operator * (const HepRotationX & rx, const HepRotation & r);
inline HepRotation operator * (const HepRotationY & ry, const HepRotation & r);
inline HepRotation operator * (const HepRotationZ & rz, const HepRotation & r);

/**
 * @author
 * @ingroup vector
 */
class HepRotation {

public:

  // ----------  Constructors and Assignment:

  inline HepRotation();
  // Default constructor. Gives a unit matrix.

  inline HepRotation(const HepRotation  & m);
  // Copy constructor.

  inline HepRotation(const HepRotationX & m);
  inline HepRotation(const HepRotationY & m);
  inline HepRotation(const HepRotationZ & m);
  // Construct from specialized rotation.

  HepRotation & set( const Hep3Vector & axis, double delta );
  HepRotation      ( const Hep3Vector & axis, double delta );
  // Construct from axis and angle.

  HepRotation & set( const HepAxisAngle & ax );
  HepRotation      ( const HepAxisAngle & ax );
  // Construct from AxisAngle structure.

  HepRotation & set( double phi, double theta, double psi );
  HepRotation      ( double phi, double theta, double psi );
  // Construct from three Euler angles (in radians).

  HepRotation & set( const HepEulerAngles & e );
  HepRotation      ( const HepEulerAngles & e );
  // Construct from EulerAngles structure.

  HepRotation ( const Hep3Vector & colX,
                const Hep3Vector & colY,
                const Hep3Vector & colZ );
  // Construct from three *orthogonal* unit vector columns.
  	// NOTE:	
  	//       This constructor, and the two set methods below, 
	//	 will check that the columns (or rows) form an orthonormal 
	//	 matrix, and will adjust values so that this relation is 
	//	 as exact as possible.

  HepRotation & set( const Hep3Vector & colX,
                             const Hep3Vector & colY,
                             const Hep3Vector & colZ );
  //   supply three *orthogonal* unit vectors for the columns.

  HepRotation & setRows( const Hep3Vector & rowX,
                                 const Hep3Vector & rowY,
                                 const Hep3Vector & rowZ );
  //   supply three *orthogonal* unit vectors for the rows.

  inline HepRotation & set(const HepRotationX & r);
  inline HepRotation & set(const HepRotationY & r);
  inline HepRotation & set(const HepRotationZ & r);
  // set from specialized rotation.

  inline  HepRotation & operator = (const HepRotation & r);
  // Assignment.

  inline  HepRotation & operator = (const HepRotationX & r);
  inline  HepRotation & operator = (const HepRotationY & r);
  inline  HepRotation & operator = (const HepRotationZ & r);
  // Assignment from specialized rotation.

  inline HepRotation &set( const HepRep3x3 & m );
  inline HepRotation     ( const HepRep3x3 & m );
  // WARNING - NO CHECKING IS DONE!
  // Constructon directly from from a 3x3 representation,
  // which is required to be an orthogonal matrix.

  inline ~HepRotation();
  // Trivial destructor.

  // ----------  Accessors:

  inline Hep3Vector colX() const;
  inline Hep3Vector colY() const;
  inline Hep3Vector colZ() const;
  // orthogonal unit-length column vectors

  inline Hep3Vector rowX() const;
  inline Hep3Vector rowY() const;
  inline Hep3Vector rowZ() const;
  // orthogonal unit-length row vectors
                                
  inline double xx() const;
  inline double xy() const;
  inline double xz() const;
  inline double yx() const;
  inline double yy() const;
  inline double yz() const;
  inline double zx() const;
  inline double zy() const;
  inline double zz() const;
  // Elements of the rotation matrix (Geant4).

  inline HepRep3x3 rep3x3() const;
  //   3x3 representation:

  // ------------  Subscripting:

  class HepRotation_row {
  public:
    inline HepRotation_row(const HepRotation &, int);
    inline double operator [] (int) const;
  private:
    const HepRotation & rr;
    int ii;
  };
  // Helper class for implemention of C-style subscripting r[i][j] 

  inline const HepRotation_row operator [] (int) const; 
  // Returns object of the helper class for C-style subscripting r[i][j]
  // i and j range from 0 to 2.  

  double operator () (int, int) const;
  // Fortran-style subscripting: returns (i,j) element of the rotation matrix.
  // Note:  i and j still range from 0 to 2.			[Rotation.cc]

  // ------------  Euler angles:
  inline  double getPhi  () const;
  inline  double getTheta() const;
  inline  double getPsi  () const;
  double    phi  () const;
  double    theta() const;
  double    psi  () const;
  HepEulerAngles eulerAngles() const;

  // ------------  axis & angle of rotation:
  inline  double  getDelta() const;
  inline  Hep3Vector getAxis () const;
  double     delta() const;
  Hep3Vector    axis () const;
  HepAxisAngle  axisAngle() const;
  void getAngleAxis(double & delta, Hep3Vector & axis) const;
  // Returns the rotation angle and rotation axis (Geant4). 	[Rotation.cc]

  // ------------- Angles of rotated axes
  double phiX() const;
  double phiY() const;
  double phiZ() const;
  double thetaX() const;
  double thetaY() const;
  double thetaZ() const;
  // Return angles (RADS) made by rotated axes against original axes (Geant4).
  //								[Rotation.cc]

  // ----------  Other accessors treating pure rotation as a 4-rotation

  inline HepLorentzVector col1() const;
  inline HepLorentzVector col2() const;
  inline HepLorentzVector col3() const;
  //  orthosymplectic 4-vector columns - T component will be zero

  inline HepLorentzVector col4() const;
  // Will be (0,0,0,1) for this pure Rotation.

  inline HepLorentzVector row1() const;
  inline HepLorentzVector row2() const;
  inline HepLorentzVector row3() const;
  //  orthosymplectic 4-vector rows - T component will be zero

  inline HepLorentzVector row4() const;
  // Will be (0,0,0,1) for this pure Rotation.

  inline double xt() const;
  inline double yt() const;
  inline double zt() const;
  inline double tx() const;
  inline double ty() const;
  inline double tz() const;
  // Will be zero for this pure Rotation

  inline double tt() const;
  // Will be one for this pure Rotation

  inline HepRep4x4 rep4x4() const;
  //   4x4 representation.

  // ---------   Mutators 

  void setPhi (double phi);
  // change Euler angle phi, leaving theta and psi unchanged.

  void setTheta (double theta);
  // change Euler angle theta, leaving phi and psi unchanged.

  void setPsi (double psi);
  // change Euler angle psi, leaving theta and phi unchanged.

  void setAxis (const Hep3Vector & axis);
  // change rotation axis, leaving delta unchanged.

  void setDelta (double delta);
  // change angle of rotation, leaving rotation axis unchanged.

  // ----------  Decomposition:

  void decompose (HepAxisAngle & rotation, Hep3Vector & boost) const;
  void decompose (Hep3Vector & boost, HepAxisAngle & rotation) const;
  // These are trivial, as the boost vector is 0.		[RotationP.cc]

  // ----------  Comparisons: 
  
  bool isIdentity() const;				
  // Returns true if the identity matrix (Geant4).		[Rotation.cc]	

  int compare( const HepRotation & r  ) const;
  // Dictionary-order comparison, in order zz, zy, zx, yz, ... xx
  // Used in operator<, >, <=, >= 

  inline bool operator== ( const HepRotation & r ) const;
  inline bool operator!= ( const HepRotation & r ) const;
  inline bool operator<  ( const HepRotation & r ) const;
  inline bool operator>  ( const HepRotation & r ) const;
  inline bool operator<= ( const HepRotation & r ) const;
  inline bool operator>= ( const HepRotation & r ) const;
  
  double distance2( const HepRotation &  r  ) const; 
  // 3 - Tr ( this/r ) -- This works with RotationX, Y or Z also

  double howNear( const HepRotation & r ) const;
  bool isNear( const HepRotation & r,
             double epsilon=Hep4RotationInterface::tolerance) const;

///  double distance2( const HepBoost           & lt  ) const; 
  // 3 - Tr ( this ) + |b|^2 / (1-|b|^2) 
///  double distance2( const HepLorentzRotation & lt  ) const; 
  // 3 - Tr ( this/r ) + |b|^2 / (1-|b|^2) where b is the boost vector of lt

///  double howNear( const HepBoost           & lt ) const;
  double howNear( const HepLorentzRotation & lt ) const;
///  bool isNear( const HepBoost           & lt, 
///             double epsilon=Hep4RotationInterface::tolerance) const;
  bool isNear( const HepLorentzRotation & lt,
             double epsilon=Hep4RotationInterface::tolerance) const;

  // ----------  Properties:

  double norm2() const; 
  // distance2 (IDENTITY), which is 3 - Tr ( *this )

  void rectify();
  // non-const but logically moot correction for accumulated roundoff errors
	// rectify averages the matrix with the transpose of its actual
	// inverse (absent accumulated roundoff errors, the transpose IS
	// the inverse)); this removes to first order those errors.
	// Then it formally extracts axis and delta, and forms a true
	// HepRotation with those values of axis and delta.

  // ---------- Application:

  inline Hep3Vector operator() (const Hep3Vector & p) const;
  // Rotate a Hep3Vector.					

  inline  Hep3Vector operator * (const Hep3Vector & p) const;
  // Multiplication with a Hep3Vector.

  inline HepLorentzVector operator()( const HepLorentzVector & w ) const;
  // Rotate (the space part of) a HepLorentzVector.		

  inline  HepLorentzVector operator* ( const HepLorentzVector & w ) const;
  // Multiplication with a HepLorentzVector.

  // ---------- Operations in the group of Rotations

  inline HepRotation operator * (const HepRotation & r) const;
  // Product of two rotations (this) * r - matrix multiplication  

  inline HepRotation operator * (const HepRotationX & rx) const; 
  inline HepRotation operator * (const HepRotationY & ry) const;
  inline HepRotation operator * (const HepRotationZ & rz) const;
  // Product of two rotations (this) * r - faster when specialized type 

  inline  HepRotation & operator *= (const HepRotation & r);
  inline  HepRotation & transform   (const HepRotation & r);
  // Matrix multiplication.
  // Note a *= b; <=> a = a * b; while a.transform(b); <=> a = b * a;

  inline  HepRotation & operator *= (const HepRotationX & r);
  inline  HepRotation & operator *= (const HepRotationY & r);
  inline  HepRotation & operator *= (const HepRotationZ & r);
  inline  HepRotation & transform   (const HepRotationX & r);
  inline  HepRotation & transform   (const HepRotationY & r);
  inline  HepRotation & transform   (const HepRotationZ & r);
  // Matrix multiplication by specialized matrices

  HepRotation & rotateX(double delta);
  // Rotation around the x-axis; equivalent to R = RotationX(delta) * R

  HepRotation & rotateY(double delta);
  // Rotation around the y-axis; equivalent to R = RotationY(delta) * R

  HepRotation & rotateZ(double delta);
  // Rotation around the z-axis; equivalent to R = RotationZ(delta) * R

         HepRotation & rotate(double delta, const Hep3Vector & axis);
  inline HepRotation & rotate(double delta, const Hep3Vector * axis);
  // Rotation around a specified vector.  
  // r.rotate(d,a) is equivalent to r = Rotation(d,a) * r

  HepRotation & rotateAxes(const Hep3Vector & newX,
                           const Hep3Vector & newY,
                           const Hep3Vector & newZ);
  // Rotation of local axes defined by 3 orthonormal vectors (Geant4).
  // Equivalent to r = Rotation (newX, newY, newZ) * r

  inline HepRotation inverse() const;
  // Returns the inverse.

  inline HepRotation & invert();
  // Inverts the Rotation matrix.

  // ---------- I/O: 

  std::ostream & print( std::ostream & os ) const;
  // Aligned six-digit-accurate output of the  rotation matrix.	[RotationIO.cc]

  // ---------- Identity Rotation:

  static const HepRotation IDENTITY;

  // ---------- Tolerance

  static inline double getTolerance();
  static inline double setTolerance(double tol); 

protected:

  inline HepRotation(double mxx, double mxy, double mxz, 
		     double myx, double myy, double myz,
		     double mzx, double mzy, double mzz);
  // Protected constructor.  
  // DOES NOT CHECK FOR VALIDITY AS A ROTATION.

  friend HepRotation operator* (const HepRotationX & rx, const HepRotation & r);
  friend HepRotation operator* (const HepRotationY & ry, const HepRotation & r);
  friend HepRotation operator* (const HepRotationZ & rz, const HepRotation & r);

  double rxx, rxy, rxz, 
	    ryx, ryy, ryz, 
	    rzx, rzy, rzz;
  // The matrix elements.

private:
  bool 
       setCols ( const Hep3Vector & u1,	// Vectors assume to be of unit length 
                 const Hep3Vector & u2,
                 const Hep3Vector & u3,
                 double u1u2,
                 Hep3Vector & v1,		// Returned vectors
                 Hep3Vector & v2,
                 Hep3Vector & v3 ) const;
  void setArbitrarily (const Hep3Vector & colX, // assumed to be of unit length
                      Hep3Vector & v1,
                      Hep3Vector & v2,
                      Hep3Vector & v3) const;
};  // HepRotation

inline   
std::ostream & operator << 
	( std::ostream & os, const HepRotation & r ) {return r.print(os);}

}  // namespace CLHEP

///#include "CLHEP/Vector/Rotation.icc"
#include "../vector/Rotation.icc"

#ifdef ENABLE_BACKWARDS_COMPATIBILITY
//  backwards compatibility will be enabled ONLY in CLHEP 1.9
using namespace CLHEP;
#endif

#endif /* HEP_ROTATION_H */

