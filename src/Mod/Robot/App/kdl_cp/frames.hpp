/***************************************************************************
                        frames.hpp `-  description
                       -------------------------
    begin                : June 2006
    copyright            : (C) 2006 Erwin Aertbelien
    email                : firstname.lastname@mech.kuleuven.be

 History (only major changes)( AUTHOR-Description ) :

 ***************************************************************************
 *   This library is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU Lesser General Public            *
 *   License as published by the Free Software Foundation; either          *
 *   version 2.1 of the License, or (at your option) any later version.    *
 *                                                                         *
 *   This library is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU     *
 *   Lesser General Public License for more details.                       *
 *                                                                         *
 *   You should have received a copy of the GNU Lesser General Public      *
 *   License along with this library; if not, write to the Free Software   *
 *   Foundation, Inc., 59 Temple Place,                                    *
 *   Suite 330, Boston, MA  02111-1307  USA                                *
 *                                                                         *
 ***************************************************************************/

/**
 * \file
 * \warning
 *       Efficienty can be improved by writing p2 = A*(B*(C*p1))) instead of
 *          p2=A*B*C*p1
 *
 * \par PROPOSED NAMING CONVENTION FOR FRAME-like OBJECTS
 *
 * \verbatim
 *      A naming convention of objects of the type defined in this file :
 *          (1) Frame : F...
 *              Rotation : R ...
 *          (2) Twist    : T ...
 *              Wrench   : W ...
 *              Vector   : V ...
 *      This prefix is followed by :
 *      for category (1) :
 *          F_A_B : w.r.t. frame A, frame B expressed
 *          ( each column of F_A_B corresponds to an axis of B,
 *            expressed w.r.t. frame A )
 *          in mathematical convention :
 *                   A
 *         F_A_B ==    F
 *                   B
 *
 *      for category (2) :
 *          V_B   : a vector expressed w.r.t. frame B
 *
 *      This can also be prepended by a name :
 *          e.g. : temporaryV_B
 *
 *      With this convention one can write :
 *
 *      F_A_B = F_B_A.Inverse();
 *      F_A_C = F_A_B * F_B_C;
 *      V_B   = F_B_C * V_C;    // both translation and rotation
 *      V_B   = R_B_C * V_C;    // only rotation
 * \endverbatim
 *
 * \par CONVENTIONS FOR WHEN USED WITH ROBOTS :
 *
 * \verbatim
 *       world : represents the frame ([1 0 0,0 1 0,0 0 1],[0 0 0]')
 *       mp    : represents mounting plate of a robot
 *               (i.e. everything before MP is constructed by robot manufacturer
 *                    everything after MP is tool )
 *       tf    : represents task frame of a robot
 *               (i.e. frame in which motion and force control is expressed)
 *       sf    : represents sensor frame of a robot
 *               (i.e. frame at which the forces measured by the force sensor
 *               are expressed )
 *
 *          Frame F_world_mp=...;
 *          Frame F_mp_sf(..)
 *          Frame F_mp_tf(,.)
 *
 *          Wrench are measured in sensor frame SF, so one could write :
 *                Wrench_tf = F_mp_tf.Inverse()* ( F_mp_sf * Wrench_sf );
 * \endverbatim
 *
 * \par CONVENTIONS REGARDING UNITS :
 *      Any consistent series of units can be used, e.g. N,mm,Nmm,..mm/sec
 *
 * \par Twist and Wrench transformations
 * 3 different types of transformations do exist for the twists
 * and wrenches.
 *
 * \verbatim
 *      1) Frame * Twist or Frame * Wrench :
 *              this transforms both the velocity/force reference point
 *             and the basis to which the twist/wrench are expressed.
 *      2) Rotation * Twist or Rotation * Wrench :
 *              this transforms the basis to which the twist/wrench are
 *              expressed, but leaves the reference point intact.
 *      3) Twist.RefPoint(v_base_AB) or Wrench.RefPoint(v_base_AB)
 *              this transforms only the reference point. v is expressed
 *              in the same base as the twist/wrench and points from the
 *              old reference point to the new reference point.
 * \endverbatim
 *
 *\par Spatial cross products
 * Let m be a 6D motion vector (Twist) and f be a 6D force vector (Wrench) 
 * attached to a rigid body moving with a certain velocity v (Twist). Then
 *\verbatim
 *     1) m_dot = v cross m or Twist=Twist*Twist
 *     2) f_dot = v cross f or Wrench=Twist*Wrench
 *\endverbatim
 *
 * \par Complexity
 *  Sometimes the amount of work is given in the documentation
 *  e.g. 6M+3A means 6 multiplications and 3 additions.
 *
 *  \author
 *      Erwin Aertbelien, Div. PMA, Dep. of Mech. Eng., K.U.Leuven
 *
 ****************************************************************************/
#ifndef KDL_FRAMES_H
#define KDL_FRAMES_H


#include "utilities/kdl-config.h"
#include "utilities/utility.h"

/////////////////////////////////////////////////////////////

namespace KDL {



class Vector;
class Rotation;
class Frame;
class Wrench;
class Twist;
class Vector2;
class Rotation2;
class Frame2;


// Equal is friend function, but default arguments for friends are forbidden (§8.3.6.4)
inline bool Equal(const Vector& a,const Vector& b,double eps=epsilon);
inline bool Equal(const Frame& a,const Frame& b,double eps=epsilon);
inline bool Equal(const Twist& a,const Twist& b,double eps=epsilon);
inline bool Equal(const Wrench& a,const Wrench& b,double eps=epsilon);
inline bool Equal(const Vector2& a,const Vector2& b,double eps=epsilon);
inline bool Equal(const Rotation2& a,const Rotation2& b,double eps=epsilon);
inline bool Equal(const Frame2& a,const Frame2& b,double eps=epsilon);


/**
 * \brief A concrete implementation of a 3 dimensional vector class
 */
class Vector
{
public:
    double data[3];
     //! Does not initialise the Vector to zero. use Vector::Zero() or SetToZero for that
     inline Vector() {data[0]=data[1]=data[2] = 0.0;}

     //! Constructs a vector out of the three values x, y and z
     inline Vector(double x,double y, double z);

     //! Assignment operator. The normal copy by value semantics.
     inline Vector(const Vector& arg);

     //! Assignment operator. The normal copy by value semantics.
     inline Vector& operator = ( const Vector& arg);

     //! Access to elements, range checked when NDEBUG is not set, from 0..2
     inline double operator()(int index) const;

     //! Access to elements, range checked when NDEBUG is not set, from 0..2
     inline double& operator() (int index);

	 //! Equivalent to double operator()(int index) const
     double operator[] ( int index ) const
       {
	 return this->operator() ( index );
       }

	 //! Equivalent to double& operator()(int index)
     double& operator[] ( int index )
       {
	 return this->operator() ( index );
       }

     inline double x() const;
     inline double y() const;
     inline double z() const;
     inline void x(double);
     inline void y(double);
     inline void z(double);

     //! Reverses the sign of the Vector object itself
     inline void ReverseSign();


     //! subtracts a vector from the Vector object itself
     inline Vector& operator-=(const Vector& arg);


     //! Adds a vector from the Vector object itself
     inline Vector& operator +=(const Vector& arg);

     //! Scalar multiplication is defined
     inline friend Vector operator*(const Vector& lhs,double rhs);
     //! Scalar multiplication is defined
     inline friend Vector operator*(double lhs,const Vector& rhs);
     //! Scalar division is defined

     inline friend Vector operator/(const Vector& lhs,double rhs);
     inline friend Vector operator+(const Vector& lhs,const Vector& rhs);
     inline friend Vector operator-(const Vector& lhs,const Vector& rhs);
     inline friend Vector operator*(const Vector& lhs,const Vector& rhs);
     inline friend Vector operator-(const Vector& arg);
     inline friend double dot(const Vector& lhs,const Vector& rhs);

     //! To have a uniform operator to put an element to zero, for scalar values
     //! and for objects.
     inline friend void SetToZero(Vector& v);

     //! @return a zero vector
     inline static Vector Zero();

   /** Normalizes this vector and returns it norm
	* makes v a unitvector and returns the norm of v.
	* if v is smaller than eps, Vector(1,0,0) is returned with norm 0.
	* if this is not good, check the return value of this method.
	*/
     double Normalize(double eps=epsilon);

     //!    @return the norm of the vector
     double Norm() const;



     //! a 3D vector where the 2D vector v is put in the XY plane
     inline void Set2DXY(const Vector2& v);
     //! a 3D vector where the 2D vector v is put in the YZ plane
     inline void Set2DYZ(const Vector2& v);
     //! a 3D vector where the 2D vector v is put in the ZX plane
     inline void Set2DZX(const Vector2& v);
     //! a 3D vector where the 2D vector v_XY is put in the XY plane of the frame F_someframe_XY.
     inline void Set2DPlane(const Frame& F_someframe_XY,const Vector2& v_XY);


     //! do not use operator == because the definition of Equal(.,.) is slightly
     //! different.  It compares whether the 2 arguments are equal in an eps-interval
     inline friend bool Equal(const Vector& a,const Vector& b,double eps);

	 //! The literal equality operator==(), also identical.
     inline friend bool operator==(const Vector& a,const Vector& b);
	 //! The literal inequality operator!=().
     inline friend bool operator!=(const Vector& a,const Vector& b);

     friend class Rotation;
     friend class Frame;
};


/**
  \brief represents rotations in 3 dimensional space.

  This class represents a rotation matrix with the following
  conventions :
 \verbatim
     Suppose V2 = R*V,                                    (1)
     V is expressed in frame B
     V2 is expressed in frame A
     This matrix R consists of 3 collumns [ X,Y,Z ],
     X,Y, and Z contain the axes of frame B, expressed in frame A
     Because of linearity expr(1) is valid.
 \endverbatim
   This class only represents rotational_interpolation, not translation
 Two interpretations are possible for rotation angles.
 * if you rotate with angle around X frame A to have frame B,
   then the result of SetRotX is equal to frame B expressed wrt A.
     In code:
 \verbatim
      Rotation R;
      F_A_B = R.SetRotX(angle);
 \endverbatim
 * Secondly, if you take the following code :
 \verbatim
      Vector p,p2; Rotation R;
      R.SetRotX(angle);
      p2 = R*p;
 \endverbatim
   then the frame p2 is rotated around X axis with (-angle).
   Analogue reasonings can be applyd to SetRotY,SetRotZ,SetRot
 \par type
  Concrete implementation
*/
class Rotation
{
public:
    double data[9];

    inline Rotation() {
		*this = Rotation::Identity();
	}
    inline Rotation(double Xx,double Yx,double Zx,
                double Xy,double Yy,double Zy,
                double Xz,double Yz,double Zz);
    inline Rotation(const Vector& x,const Vector& y,const Vector& z);
    // default copy constructor is sufficient


     inline Rotation& operator=(const Rotation& arg);

     //!  Defines a multiplication R*V between a Rotation R and a Vector V.
     //! Complexity : 9M+6A
     inline Vector operator*(const Vector& v) const;

     //!    Access to elements 0..2,0..2, bounds are checked when NDEBUG is not set
     inline double& operator()(int i,int j);

     //!    Access to elements 0..2,0..2, bounds are checked when NDEBUG is not set
     inline double operator() (int i,int j) const;

     friend Rotation operator *(const Rotation& lhs,const Rotation& rhs);

     //! Sets the value of *this to its inverse.
     inline void SetInverse();

     //! Gives back the inverse rotation matrix of *this.
     inline Rotation Inverse() const;

     //! The same as R.Inverse()*v but more efficient.
     inline Vector Inverse(const Vector& v) const;

     //! The same as R.Inverse()*arg but more efficient.
     inline Wrench Inverse(const Wrench& arg) const;

     //! The same as R.Inverse()*arg but more efficient.
     inline Twist Inverse(const Twist& arg) const;

     //! Gives back an identity rotaton matrix
     inline static Rotation Identity();


// = Rotations
    //! The Rot... static functions give the value of the appropriate rotation matrix back.
    inline static Rotation RotX(double angle);
    //! The Rot... static functions give the value of the appropriate rotation matrix back.
    inline static Rotation RotY(double angle);
    //! The Rot... static functions give the value of the appropriate rotation matrix back.
    inline static Rotation RotZ(double angle);
    //! The DoRot... functions apply a rotation R to *this,such that *this = *this * Rot..
    //! DoRot... functions are only defined when they can be executed more efficiently
    inline void DoRotX(double angle);
    //! The DoRot... functions apply a rotation R to *this,such that *this = *this * Rot..
    //! DoRot... functions are only defined when they can be executed more efficiently
    inline void DoRotY(double angle);
    //! The DoRot... functions apply a rotation R to *this,such that *this = *this * Rot..
    //! DoRot... functions are only defined when they can be executed more efficiently
    inline void DoRotZ(double angle);

    //! Along an arbitrary axes.  It is not necessary to normalize rotvec.
    //! returns identity rotation matrix in the case that the norm of rotvec
    //! is to small to be used.
    // @see Rot2 if you want to handle this error in another way.
    static Rotation Rot(const Vector& rotvec,double angle);

    //! Along an arbitrary axes.  rotvec should be normalized.
    static Rotation Rot2(const Vector& rotvec,double angle);

    //! Returns a vector with the direction of the equiv. axis
    //! and its norm is angle
    Vector GetRot() const;

	/** Returns the rotation angle around the equiv. axis
	 * @param axis the rotation axis is returned in this variable
	 * @param eps :  in the case of angle == 0 : rot axis is undefined and choosen
	 *                                         to be +/- Z-axis
	 *               in the case of angle == PI : 2 solutions, positive Z-component
	 *                                            of the axis is choosen.
	 * @result returns the rotation angle (between [0..PI] )
	 */
	double GetRotAngle(Vector& axis,double eps=epsilon) const;


/**     Gives back a rotation matrix specified with EulerZYZ convention :
	 *       - First rotate around Z with alfa,
	 *       - then around the new Y with beta,
	 *	     - then around new Z with gamma.
	 *  Invariants:
	 *  	- EulerZYX(alpha,beta,gamma) == EulerZYX(alpha +/- PHI, -beta, gamma +/- PI)
	 *  	- (angle + 2*k*PI)
	 **/
    static Rotation EulerZYZ(double Alfa,double Beta,double Gamma);

	/** Gives back the EulerZYZ convention description of the rotation matrix :
	 First rotate around Z with alpha,
	 then around the new Y with beta, then around
	 new Z with gamma.

	 Variables are bound by:
	 - (-PI <  alpha  <= PI),
	 - (0   <= beta  <= PI),
	 - (-PI <  gamma <= PI)

	 if beta==0 or beta==PI, then alpha and gamma are not unique, in this case gamma is chosen to be zero.
	 Invariants:
	   - EulerZYX(alpha,beta,gamma) == EulerZYX(alpha +/- PI, -beta, gamma +/- PI)
	   - angle + 2*k*PI
	 */
    void GetEulerZYZ(double& alpha,double& beta,double& gamma) const;

    //! Gives back a rotation matrix specified with Quaternion convention
    //! the norm of (x,y,z,w) should be equal to 1
    static Rotation Quaternion(double x,double y,double z, double w);
    
    //! Get the quaternion of this matrix
    //! \post the norm of (x,y,z,w) is 1
    void GetQuaternion(double& x,double& y,double& z, double& w) const;

    /**
     *
     * Gives back a rotation matrix specified with RPY convention:
     * first rotate around X with roll, then around the
     *              old Y with pitch, then around old Z with yaw
     *
     * Invariants:
     *  - RPY(roll,pitch,yaw) == RPY( roll +/- PI, PI-pitch, yaw +/- PI )
     *  - angles + 2*k*PI
     */
    static Rotation RPY(double roll,double pitch,double yaw);

/**  Gives back a vector in RPY coordinates, variables are bound by
     -  -PI <= roll <= PI
     -   -PI <= Yaw  <= PI
     -  -PI/2 <= PITCH <= PI/2

	 convention :
	 - first rotate around X with roll,
	 - then around the old Y with pitch,
	 - then around old Z with yaw

	 if pitch == PI/2 or pitch == -PI/2, multiple solutions for gamma and alpha exist.  The solution where roll==0
	 is chosen.

	 Invariants:
	 - RPY(roll,pitch,yaw) == RPY( roll +/- PI, PI-pitch, yaw +/- PI )
	 - angles + 2*k*PI

**/
    void GetRPY(double& roll,double& pitch,double& yaw) const;


    /**  EulerZYX constructs a Rotation from the Euler ZYX parameters:
     *   -  First rotate around Z with alfa,
     *   - then around the new Y with beta,
     *   - then around new X with gamma.
     *
     *  Closely related to RPY-convention.
     *
     *  Invariants:
     *  	- EulerZYX(alpha,beta,gamma) == EulerZYX(alpha +/- PI, PI-beta, gamma +/- PI)
     *  	- (angle + 2*k*PI)
     **/
    inline static Rotation EulerZYX(double Alfa,double Beta,double Gamma) {
        return RPY(Gamma,Beta,Alfa);
    }

    /**   GetEulerZYX gets the euler ZYX parameters of a rotation :
     *   First rotate around Z with alfa,
     *   then around the new Y with beta, then around
     *   new X with gamma.
     *
     *  Range of the results of GetEulerZYX :
     *  -  -PI <= alfa <= PI
     *  -   -PI <= gamma <= PI
     *  -  -PI/2 <= beta <= PI/2
     *
     *  if beta == PI/2 or beta == -PI/2, multiple solutions for gamma and alpha exist.  The solution where gamma==0
     *  is chosen.
     *
     *
     *  Invariants:
     *  	- EulerZYX(alpha,beta,gamma) == EulerZYX(alpha +/- PI, PI-beta, gamma +/- PI)
     *  	- and also (angle + 2*k*PI)
     *
     *  Closely related to RPY-convention.
     **/
    inline void GetEulerZYX(double& Alfa,double& Beta,double& Gamma) const {
        GetRPY(Gamma,Beta,Alfa);
    }

     //! Transformation of the base to which the twist is expressed.
     //! Complexity : 18M+12A
     //! @see Frame*Twist for a transformation that also transforms
     //! the velocity reference point.
     inline Twist operator * (const Twist& arg) const;

     //! Transformation of the base to which the wrench is expressed.
     //! Complexity : 18M+12A
     //! @see Frame*Wrench for a transformation that also transforms
     //! the force reference point.
     inline Wrench operator * (const Wrench& arg) const;

     //! Access to the underlying unitvectors of the rotation matrix
     inline Vector UnitX() const {
         return Vector(data[0],data[3],data[6]);
     }

     //! Access to the underlying unitvectors of the rotation matrix
     inline void UnitX(const Vector& X) {
        data[0] = X(0);
        data[3] = X(1);
        data[6] = X(2);
     }

     //! Access to the underlying unitvectors of the rotation matrix
     inline Vector UnitY() const {
         return Vector(data[1],data[4],data[7]);
     }

     //! Access to the underlying unitvectors of the rotation matrix
     inline void UnitY(const Vector& X) {
        data[1] = X(0);
        data[4] = X(1);
        data[7] = X(2);
     }

     //! Access to the underlying unitvectors of the rotation matrix
     inline Vector UnitZ() const {
         return Vector(data[2],data[5],data[8]);
     }

     //! Access to the underlying unitvectors of the rotation matrix
     inline void UnitZ(const Vector& X) {
        data[2] = X(0);
        data[5] = X(1);
        data[8] = X(2);
     }

     //! do not use operator == because the definition of Equal(.,.) is slightly
     //! different.  It compares whether the 2 arguments are equal in an eps-interval
     friend bool Equal(const Rotation& a,const Rotation& b,double eps);

	 //! The literal equality operator==(), also identical.
     friend bool operator==(const Rotation& a,const Rotation& b);
	 //! The literal inequality operator!=()
     friend bool operator!=(const Rotation& a,const Rotation& b);

     friend class Frame;
};
    bool operator==(const Rotation& a,const Rotation& b);
    bool Equal(const Rotation& a,const Rotation& b,double eps=epsilon);



/**
	\brief represents a frame transformation in 3D space (rotation + translation)

    if V2 = Frame*V1 (V2 expressed in frame A, V1 expressed in frame B)
    then V2 = Frame.M*V1+Frame.p

    Frame.M contains columns that represent the axes of frame B wrt frame A
    Frame.p contains the origin of frame B expressed in frame A.
*/
class Frame {
public:
    Vector p;       //!< origine of the Frame
    Rotation M;     //!< Orientation of the Frame

public:

     inline Frame(const Rotation& R,const Vector& V);

     //! The rotation matrix defaults to identity
     explicit inline Frame(const Vector& V);
     //! The position matrix defaults to zero
     explicit inline Frame(const Rotation& R);

     inline Frame() {}
     //! The copy constructor. Normal copy by value semantics.
     inline Frame(const Frame& arg);

     //! Reads data from an double array
     //\TODO should be formulated as a constructor
     void Make4x4(double* d);

     //!  Treats a frame as a 4x4 matrix and returns element i,j
     //!  Access to elements 0..3,0..3, bounds are checked when NDEBUG is not set
     inline double operator()(int i,int j);

     //!  Treats a frame as a 4x4 matrix and returns element i,j
     //!    Access to elements 0..3,0..3, bounds are checked when NDEBUG is not set
     inline double operator() (int i,int j) const;

// = Inverse
     //! Gives back inverse transformation of a Frame
     inline Frame Inverse() const;

     //! The same as p2=R.Inverse()*p but more efficient.
     inline Vector Inverse(const Vector& arg) const;

     //! The same as p2=R.Inverse()*p but more efficient.
     inline Wrench Inverse(const Wrench& arg) const;

     //! The same as p2=R.Inverse()*p but more efficient.
     inline Twist  Inverse(const Twist& arg) const;

     //! Normal copy-by-value semantics.
     inline Frame& operator = (const Frame& arg);

     //! Transformation of the base to which the vector
     //! is expressed.
     inline Vector operator * (const Vector& arg) const;

     //! Transformation of both the force reference point
     //! and of the base to which the wrench is expressed.
     //! look at Rotation*Wrench operator for a transformation
     //! of only the base to which the twist is expressed.
     //!
     //! Complexity : 24M+18A
     inline Wrench operator * (const Wrench& arg) const;

     //! Transformation of both the velocity reference point
     //! and of the base to which the twist is expressed.
     //! look at Rotation*Twist for a transformation of only the
     //! base to which the twist is expressed.
     //!
     //! Complexity : 24M+18A
     inline Twist operator * (const Twist& arg) const;

     //! Composition of two frames.
     inline friend Frame operator *(const Frame& lhs,const Frame& rhs);

     //! @return the identity transformation Frame(Rotation::Identity(),Vector::Zero()).
     inline static Frame Identity();

     //! The twist <t_this> is expressed wrt the current
     //! frame.  This frame is integrated into an updated frame with
     //! <samplefrequency>.  Very simple first order integration rule.
     inline void Integrate(const Twist& t_this,double frequency);

    /*
    // DH_Craig1989 : constructs a transformationmatrix
    // T_link(i-1)_link(i) with the Denavit-Hartenberg convention as
    // described in the Craigs book: Craig, J. J.,Introduction to
    // Robotics: Mechanics and Control, Addison-Wesley,
    // isbn:0-201-10326-5, 1986.
    //
    // Note that the frame is a redundant way to express the information
    // in the DH-convention.
    // \verbatim
    // Parameters in full : a(i-1),alpha(i-1),d(i),theta(i)
    //
    //  axis i-1 is connected by link i-1 to axis i numbering axis 1
    //  to axis n link 0 (immobile base) to link n
    //
    //  link length a(i-1) length of the mutual perpendicular line
    //  (normal) between the 2 axes.  This normal runs from (i-1) to
    //  (i) axis.
    //
    //  link twist alpha(i-1): construct plane perpendicular to the
    //  normal project axis(i-1) and axis(i) into plane angle from
    //  (i-1) to (i) measured in the direction of the normal
    //
    //  link offset d(i) signed distance between normal (i-1) to (i)
    //  and normal (i) to (i+1) along axis i joint angle theta(i)
    //  signed angle between normal (i-1) to (i) and normal (i) to
    //  (i+1) along axis i
    //
    //   First and last joints : a(0)= a(n) = 0
    //   alpha(0) = alpha(n) = 0
    //
    //   PRISMATIC : theta(1) = 0 d(1) arbitrarily
    //
    //   REVOLUTE : theta(1) arbitrarily d(1) = 0
    //
    //   Not unique : if intersecting joint axis 2 choices for normal
    //   Frame assignment of the DH convention : Z(i-1) follows axis
    //   (i-1) X(i-1) is the normal between axis(i-1) and axis(i)
    //   Y(i-1) follows out of Z(i-1) and X(i-1)
    //
    //     a(i-1)     = distance from Z(i-1) to Z(i) along X(i-1)
    //     alpha(i-1) = angle between Z(i-1) to Z(i) along X(i-1)
    //     d(i)       = distance from X(i-1) to X(i) along Z(i)
    //     theta(i)   = angle between X(i-1) to X(i) along X(i)
    // \endverbatim
    */
     static Frame DH_Craig1989(double a,double alpha,double d,double theta);

    // DH : constructs a transformationmatrix T_link(i-1)_link(i) with
    // the Denavit-Hartenberg convention as described in the original
    // publictation: Denavit, J. and Hartenberg, R. S., A kinematic
    // notation for lower-pair mechanisms based on matrices, ASME
    // Journal of Applied Mechanics, 23:215-221, 1955.

     static Frame DH(double a,double alpha,double d,double theta);


     //! do not use operator == because the definition of Equal(.,.) is slightly
     //! different.  It compares whether the 2 arguments are equal in an eps-interval
     inline friend bool Equal(const Frame& a,const Frame& b,double eps);

	 //! The literal equality operator==(), also identical.
     inline friend bool operator==(const Frame& a,const Frame& b);
	 //! The literal inequality operator!=().
     inline friend bool operator!=(const Frame& a,const Frame& b);
};

/**
 * \brief represents both translational and rotational velocities.
 *
 * This class represents a twist.  A twist is the combination of translational
 * velocity and rotational velocity applied at one point.
*/
class Twist {
public:
    Vector vel; //!< The velocity of that point
    Vector rot; //!< The rotational velocity of that point.
public:

    //! The default constructor initialises to Zero via the constructor of Vector.
    Twist():vel(),rot() {};

    Twist(const Vector& _vel,const Vector& _rot):vel(_vel),rot(_rot) {};

    inline Twist& operator-=(const Twist& arg);
    inline Twist& operator+=(const Twist& arg);
    //! index-based access to components, first vel(0..2), then rot(3..5)
    inline double& operator()(int i);

    //! index-based access to components, first vel(0..2), then rot(3..5)
    //! For use with a const Twist
    inline double operator()(int i) const;

     double operator[] ( int index ) const
       {
	 return this->operator() ( index );
       }

     double& operator[] ( int index )
       {
	 return this->operator() ( index );
       }

     inline friend Twist operator*(const Twist& lhs,double rhs);
     inline friend Twist operator*(double lhs,const Twist& rhs);
     inline friend Twist operator/(const Twist& lhs,double rhs);
     inline friend Twist operator+(const Twist& lhs,const Twist& rhs);
     inline friend Twist operator-(const Twist& lhs,const Twist& rhs);
     inline friend Twist operator-(const Twist& arg);
     inline friend double dot(const Twist& lhs,const Wrench& rhs);
     inline friend double dot(const Wrench& rhs,const Twist& lhs);
     inline friend void SetToZero(Twist& v);
    /// Spatial cross product for 6d motion vectors, beware all of them have to be expressed in the same reference frame/point
    inline friend Twist operator*(const Twist& lhs,const Twist& rhs);
    /// Spatial cross product for 6d force vectors, beware all of them have to be expressed in the same reference frame/point
    inline friend Wrench operator*(const Twist& lhs,const Wrench& rhs);

     //! @return a zero Twist : Twist(Vector::Zero(),Vector::Zero())
     static inline Twist Zero();

     //! Reverses the sign of the twist
     inline void ReverseSign();

     //! Changes the reference point of the twist.
     //! The vector v_base_AB is expressed in the same base as the twist
     //! The vector v_base_AB is a vector from the old point to
     //! the new point.
     //!
     //! Complexity : 6M+6A
     inline Twist RefPoint(const Vector& v_base_AB) const;


     //! do not use operator == because the definition of Equal(.,.) is slightly
     //! different.  It compares whether the 2 arguments are equal in an eps-interval
     inline friend bool Equal(const Twist& a,const Twist& b,double eps);

	 //! The literal equality operator==(), also identical.
     inline friend bool operator==(const Twist& a,const Twist& b);
	 //! The literal inequality operator!=().
     inline friend bool operator!=(const Twist& a,const Twist& b);

// = Friends
    friend class Rotation;
    friend class Frame;

};

/**
 * 	\brief represents both translational and rotational acceleration.
 *
 * 	This class represents an acceleration twist.  A acceleration twist is
 * 	the combination of translational
 *	acceleration and rotational acceleration applied at one point.
*/
/*
class AccelerationTwist {
public:
    Vector trans; //!< The translational acceleration of that point
    Vector rot; //!< The rotational acceleration of that point.
public:

    //! The default constructor initialises to Zero via the constructor of Vector.
    AccelerationTwist():trans(),rot() {};

    AccelerationTwist(const Vector& _trans,const Vector& _rot):trans(_trans),rot(_rot) {};

    inline AccelerationTwist& operator-=(const AccelerationTwist& arg);
    inline AccelerationTwist& operator+=(const AccelerationTwist& arg);
    //! index-based access to components, first vel(0..2), then rot(3..5)
    inline double& operator()(int i);

    //! index-based access to components, first vel(0..2), then rot(3..5)
    //! For use with a const AccelerationTwist
    inline double operator()(int i) const;

    double operator[] ( int index ) const
    {
    	return this->operator() ( index );
	}

     double& operator[] ( int index )
     {
    	 return this->operator() ( index );
     }

     inline friend AccelerationTwist operator*(const AccelerationTwist& lhs,double rhs);
     inline friend AccelerationTwist operator*(double lhs,const AccelerationTwist& rhs);
     inline friend AccelerationTwist operator/(const AccelerationTwist& lhs,double rhs);
     inline friend AccelerationTwist operator+(const AccelerationTwist& lhs,const AccelerationTwist& rhs);
     inline friend AccelerationTwist operator-(const AccelerationTwist& lhs,const AccelerationTwist& rhs);
     inline friend AccelerationTwist operator-(const AccelerationTwist& arg);
     //inline friend double dot(const AccelerationTwist& lhs,const Wrench& rhs);
     //inline friend double dot(const Wrench& rhs,const AccelerationTwist& lhs);
     inline friend void SetToZero(AccelerationTwist& v);


     //! @return a zero AccelerationTwist : AccelerationTwist(Vector::Zero(),Vector::Zero())
     static inline AccelerationTwist Zero();

     //! Reverses the sign of the AccelerationTwist
     inline void ReverseSign();

     //! Changes the reference point of the AccelerationTwist.
     //! The vector v_base_AB is expressed in the same base as the AccelerationTwist
     //! The vector v_base_AB is a vector from the old point to
     //! the new point.
     //!
     //! Complexity : 6M+6A
     inline AccelerationTwist RefPoint(const Vector& v_base_AB) const;


     //! do not use operator == because the definition of Equal(.,.) is slightly
     //! different.  It compares whether the 2 arguments are equal in an eps-interval
     inline friend bool Equal(const AccelerationTwist& a,const AccelerationTwist& b,double eps=epsilon);

	 //! The literal equality operator==(), also identical.
     inline friend bool operator==(const AccelerationTwist& a,const AccelerationTwist& b);
	 //! The literal inequality operator!=().
     inline friend bool operator!=(const AccelerationTwist& a,const AccelerationTwist& b);

// = Friends
    friend class Rotation;
    friend class Frame;

};
*/
/**
 * \brief represents the combination of a force and a torque.
 *
 * This class represents a Wrench.  A Wrench is the force and torque applied at a point
 */
class Wrench
{
public:
    Vector force;       //!< Force that is applied at the origin of the current ref frame
    Vector torque;      //!< Torque that is applied at the origin of the current ref frame
public:

    //! Does  initialise force and torque to zero via the underlying constructor of Vector
    Wrench():force(),torque() {};
    Wrench(const Vector& _force,const Vector& _torque):force(_force),torque(_torque) {};

// = Operators
     inline Wrench& operator-=(const Wrench& arg);
     inline Wrench& operator+=(const Wrench& arg);

     //! index-based access to components, first force(0..2), then torque(3..5)
     inline double& operator()(int i);

     //! index-based access to components, first force(0..2), then torque(3..5)
     //! for use with a const Wrench
     inline double operator()(int i) const;

     double operator[] ( int index ) const
       {
	 return this->operator() ( index );
       }

     double& operator[] ( int index )
       {
	 return this->operator() ( index );
       }

     //! Scalar multiplication
     inline friend Wrench operator*(const Wrench& lhs,double rhs);
     //! Scalar multiplication
     inline friend Wrench operator*(double lhs,const Wrench& rhs);
     //! Scalar division
     inline friend Wrench operator/(const Wrench& lhs,double rhs);

     inline friend Wrench operator+(const Wrench& lhs,const Wrench& rhs);
     inline friend Wrench operator-(const Wrench& lhs,const Wrench& rhs);

     //! An unary - operator
     inline friend Wrench operator-(const Wrench& arg);

     //! Sets the Wrench to Zero, to have a uniform function that sets an object or
     //! double to zero.
     inline friend void SetToZero(Wrench& v);

     //! @return a zero Wrench
     static inline Wrench Zero();

     //! Reverses the sign of the current Wrench
     inline void ReverseSign();

     //! Changes the reference point of the wrench.
     //! The vector v_base_AB is expressed in the same base as the twist
     //! The vector v_base_AB is a vector from the old point to
     //! the new point.
     //!
     //! Complexity : 6M+6A
     inline Wrench RefPoint(const Vector& v_base_AB) const;


     //! do not use operator == because the definition of Equal(.,.) is slightly
     //! different.  It compares whether the 2 arguments are equal in an eps-interval
     inline friend bool Equal(const Wrench& a,const Wrench& b,double eps);

	 //! The literal equality operator==(), also identical.
     inline friend bool operator==(const Wrench& a,const Wrench& b);
	 //! The literal inequality operator!=().
     inline friend bool operator!=(const Wrench& a,const Wrench& b);

    friend class Rotation;
    friend class Frame;


};


//! 2D version of Vector
class Vector2
{
    double data[2];
public:
     //! Does not initialise to Zero().
     Vector2() {data[0]=data[1] = 0.0;}
     inline Vector2(double x,double y);
     inline Vector2(const Vector2& arg);

     inline Vector2& operator = ( const Vector2& arg);

     //! Access to elements, range checked when NDEBUG is not set, from 0..1
     inline double operator()(int index) const;

     //! Access to elements, range checked when NDEBUG is not set, from 0..1
     inline double& operator() (int index);

    //! Equivalent to double operator()(int index) const
	double operator[] ( int index ) const
	{
		return this->operator() ( index );
	}

	//! Equivalent to double& operator()(int index)
	double& operator[] ( int index )
	{
		return this->operator() ( index );
	}

	inline double x() const;
	inline double y() const;
	inline void x(double);
	inline void y(double);

     inline void ReverseSign();
     inline Vector2& operator-=(const Vector2& arg);
     inline Vector2& operator +=(const Vector2& arg);


     inline friend Vector2 operator*(const Vector2& lhs,double rhs);
     inline friend Vector2 operator*(double lhs,const Vector2& rhs);
     inline friend Vector2 operator/(const Vector2& lhs,double rhs);
     inline friend Vector2 operator+(const Vector2& lhs,const Vector2& rhs);
     inline friend Vector2 operator-(const Vector2& lhs,const Vector2& rhs);
     inline friend Vector2 operator*(const Vector2& lhs,const Vector2& rhs);
     inline friend Vector2 operator-(const Vector2& arg);
     inline friend void SetToZero(Vector2& v);

     //! @return a zero 2D vector.
     inline static Vector2 Zero();

   /** Normalizes this vector and returns it norm
	* makes v a unitvector and returns the norm of v.
	* if v is smaller than eps, Vector(1,0,0) is returned with norm 0.
	* if this is not good, check the return value of this method.
	*/
     double Normalize(double eps=epsilon);

     //!  @return the norm of the vector
     double Norm() const;

     //! projects v in its XY plane, and sets *this to these values
     inline void Set3DXY(const Vector& v);

     //! projects v in its YZ plane, and sets *this to these values
     inline void Set3DYZ(const Vector& v);

     //! projects v in its ZX plane, and sets *this to these values
     inline void Set3DZX(const Vector& v);

     //! projects v_someframe in the XY plane of F_someframe_XY,
     //! and sets *this to these values
     //! expressed wrt someframe.
     inline void Set3DPlane(const Frame& F_someframe_XY,const Vector& v_someframe);


     //! do not use operator == because the definition of Equal(.,.) is slightly
     //! different.  It compares whether the 2 arguments are equal in an eps-interval
     inline friend bool Equal(const Vector2& a,const Vector2& b,double eps);

	//! The literal equality operator==(), also identical.
	inline friend bool operator==(const Vector2& a,const Vector2& b);
	//! The literal inequality operator!=().
	inline friend bool operator!=(const Vector2& a,const Vector2& b);

    friend class Rotation2;
};


//! A 2D Rotation class, for conventions see Rotation. For further documentation
//! of the methods see Rotation class.
class Rotation2
{
    double s,c;
    //! c,s represent  cos(angle), sin(angle), this also represents first col. of rot matrix
    //! from outside, this class behaves as if it would store the complete 2x2 matrix.
public:
    //! Default constructor does NOT initialise to Zero().
    Rotation2() {c=1.0;s=0.0;}

    explicit Rotation2(double angle_rad):s(sin(angle_rad)),c(cos(angle_rad)) {}

    Rotation2(double ca,double sa):s(sa),c(ca){}

     inline Rotation2& operator=(const Rotation2& arg);
     inline Vector2 operator*(const Vector2& v) const;
     //!    Access to elements 0..1,0..1, bounds are checked when NDEBUG is not set
     inline double operator() (int i,int j) const;

     inline friend Rotation2 operator *(const Rotation2& lhs,const Rotation2& rhs);

     inline void SetInverse();
     inline Rotation2 Inverse() const;
     inline Vector2 Inverse(const Vector2& v) const;

     inline void SetIdentity();
     inline static Rotation2 Identity();


     //! The SetRot.. functions set the value of *this to the appropriate rotation matrix.
     inline void SetRot(double angle);

     //! The Rot... static functions give the value of the appropriate rotation matrix bac
     inline static Rotation2 Rot(double angle);

     //! Gets the angle (in radians)
     inline double GetRot() const;

     //! do not use operator == because the definition of Equal(.,.) is slightly
     //! different.  It compares whether the 2 arguments are equal in an eps-interval
     inline friend bool Equal(const Rotation2& a,const Rotation2& b,double eps);
};

//! A 2D frame class, for further documentation see the Frames class
//! for methods with unchanged semantics.
class Frame2
 {
public:
    Vector2 p;          //!< origine of the Frame
    Rotation2 M;        //!< Orientation of the Frame

public:

     inline Frame2(const Rotation2& R,const Vector2& V);
     explicit inline Frame2(const Vector2& V);
     explicit inline Frame2(const Rotation2& R);
     inline Frame2(void);
     inline Frame2(const Frame2& arg);
     inline void Make4x4(double* d);

     //!  Treats a frame as a 3x3 matrix and returns element i,j
     //!    Access to elements 0..2,0..2, bounds are checked when NDEBUG is not set
     inline double operator()(int i,int j);

     //!  Treats a frame as a 4x4 matrix and returns element i,j
     //!  Access to elements 0..3,0..3, bounds are checked when NDEBUG is not set
     inline double operator() (int i,int j) const;

     inline void SetInverse();
     inline Frame2 Inverse() const;
     inline Vector2 Inverse(const Vector2& arg) const;
     inline Frame2& operator = (const Frame2& arg);
     inline Vector2 operator * (const Vector2& arg);
     inline friend Frame2 operator *(const Frame2& lhs,const Frame2& rhs);
     inline void SetIdentity();
     inline void Integrate(const Twist& t_this,double frequency);
     inline static Frame2 Identity() {
        Frame2 tmp;
        tmp.SetIdentity();
        return tmp;
     }
     inline friend bool Equal(const Frame2& a,const Frame2& b,double eps);
};

/**
 * determines the difference of vector b with vector a.
 *
 * see diff for Rotation matrices for further background information.
 *
 * \param p_w_a start vector a expressed to some frame w
 * \param p_w_b end vector   b expressed to some frame w .
 * \param dt [optional][obsolete] time interval over which the numerical differentiation takes place.
 * \return the difference (b-a) expressed to the frame w.
 */
IMETHOD Vector diff(const Vector& p_w_a,const Vector& p_w_b,double dt=1);


/**
 * determines the (scaled) rotation axis necessary to rotate from b1 to b2.  
 *
 * This rotation axis is expressed w.r.t. frame a.  The rotation axis is scaled
 * by the necessary rotation angle. The rotation angle is always in the 
 * (inclusive) interval \f$ [0 , \pi] \f$.
 *
 * This definition is chosen in this way to facilitate numerical differentiation.
 * With this definition diff(a,b) == -diff(b,a).
 *
 * The diff() function is overloaded for all classes in frames.hpp and framesvel.hpp, such that 
 * numerical differentiation, equality checks with tolerances, etc.  can be performed 
 * without caring exactly on which type the operation is performed.  
 *  
 * \param R_a_b1: The rotation matrix \f$ _a^{b1} R  \f$ of b1 with respect to frame a. 
 * \param R_a_b2: The Rotation matrix \f$ _a^{b2} R \f$ of b2 with respect to frame a. 
 * \param dt [optional][obsolete] time interval over which the numerical differentiation takes place. By default this is set to 1.0.
 * \return rotation axis to rotate from b1 to b2, scaled by the rotation angle, expressed in frame a.
 * \warning - The result is not a rotational vector, i.e. it is not a mathematical vector.
 *          (no communitative addition). 
 *
 * \warning - When used in the context of numerical differentiation, with the frames b1 and b2 very
 *           close to each other, the semantics correspond to the twist, scaled by the time. 
 *
 * \warning - For angles equal to \f$ \pi \f$, The negative of the
 *          return value is equally valid. 
 */
IMETHOD Vector diff(const Rotation& R_a_b1,const Rotation& R_a_b2,double dt=1);

/**
 * determines the rotation axis necessary to rotate the frame b1 to the same orientation as frame b2 and the vector
 * necessary to translate the origin of b1 to the origin of b2, and stores the result in a Twist datastructure.   
 * \param F_a_b1 frame b1 expressed with respect to some frame a. 
 * \param F_a_b2 frame b2 expressed with respect to some frame a. 
 * \warning The result is not a Twist!  
 * see diff() for Rotation and Vector arguments for further detail on the semantics.
 */
IMETHOD Twist diff(const Frame& F_a_b1,const Frame& F_a_b2,double dt=1);

/**
 * determines the difference between two twists i.e. the difference between
 * the underlying velocity vectors and rotational velocity vectors. 
 */
IMETHOD Twist diff(const Twist& a,const Twist& b,double dt=1);

/**
 * determines the difference between two wrenches i.e. the difference between
 * the underlying torque vectors and force vectors. 
 */
IMETHOD Wrench diff(const Wrench& W_a_p1,const Wrench& W_a_p2,double dt=1);

/**
 * \brief adds vector da to vector a.
 * see also the corresponding diff() routine.
 * \param p_w_a vector a expressed to some frame w.
 * \param p_w_da vector da expressed to some frame w.
 * \returns the vector resulting from the displacement of vector a by vector da, expressed in the frame w.
 */
IMETHOD Vector addDelta(const Vector& p_w_a,const Vector& p_w_da,double dt=1);

/**
 * returns the rotation matrix resulting from the rotation of frame a by the axis and angle
 * specified with da_w.  
 *
 * see also the corresponding diff() routine.
 *
 * \param R_w_a Rotation matrix of frame a expressed to some frame w.
 * \param da_w  axis and angle of the rotation expressed to some frame w.
 * \returns the rotation matrix resulting from the rotation of frame a by the axis and angle
 *          specified with da.   The resulting rotation matrix is expressed with respect to
 *          frame w.
 */
IMETHOD Rotation addDelta(const Rotation& R_w_a,const Vector& da_w,double dt=1);

/**
 * returns the frame resulting from the rotation of frame a by the axis and angle
 * specified in da_w and the translation of the origin (also specified in da_w). 
 *
 * see also the corresponding diff() routine.
 * \param R_w_a Rotation matrix of frame a expressed to some frame w.
 * \param da_w  axis and angle of the rotation (da_w.rot), together with a displacement vector for the origin (da_w.vel),  expressed to some frame w.
 * \returns the frame resulting from the rotation of frame a by the axis and angle
 *          specified with da.rot, and the translation of the origin da_w.vel .  The resulting frame is expressed with respect to frame w.
 */
IMETHOD Frame addDelta(const Frame& F_w_a,const Twist& da_w,double dt=1);

/**
 * \brief adds the twist da to the twist a.
 * see also the corresponding diff() routine.
 * \param a a twist wrt some frame
 * \param da a twist difference wrt some frame
 * \returns The twist (a+da) wrt the corresponding frame.
 */
IMETHOD Twist addDelta(const Twist& a,const Twist&da,double dt=1);


/**
 * \brief adds the wrench da to the wrench w.
 * see also the corresponding diff() routine.
 * see also the corresponding diff() routine.
 * \param a a wrench wrt some frame
 * \param da a wrench difference wrt some frame
 * \returns the wrench (a+da) wrt the corresponding frame.
 */
IMETHOD Wrench addDelta(const Wrench& a,const Wrench&da,double dt=1);

#ifdef KDL_INLINE
//    #include "vector.inl"
//   #include "wrench.inl"
    //#include "rotation.inl"
    //#include "frame.inl"
    //#include "twist.inl"
    //#include "vector2.inl"
    //#include "rotation2.inl"
    //#include "frame2.inl"
#include "frames.inl"
#endif



}


#endif
