/*****************************************************************************
 * \file
 *      This file contains the definition of classes for a
 *      Rall Algebra of (subset of) the classes defined in frames,
 *      i.e. classes that contain a set (value,derivative,2nd derivative)
 *      and define operations on that set
 *      this classes are usefull for automatic differentiation ( <-> symbolic diff ,
 *      <-> numeric diff).
 *      Defines VectorAcc, RotationAcc, FrameAcc, doubleAcc.
 *      Look at the corresponding classes Vector Rotation Frame Twist and
 *      Wrench for the semantics of the methods.
 *
 *      It also contains the 2nd derivative <-> RFrames.h
 *
 *  \author
 *      Erwin Aertbelien, Div. PMA, Dep. of Mech. Eng., K.U.Leuven
 *
 *  \version
 *      ORO_Geometry V0.2
 *
 *  \par History
 *      - $log$
 *
 *  \par Release
 *      $Id: rrframes.h,v 1.1.1.1 2002/08/26 14:14:21 rmoreas Exp $
 *      $Name:  $
 ****************************************************************************/

#ifndef RRFRAMES_H
#define RRFRAMES_H


#include "utilities/rall2d.h"
#include "frames.hpp"



namespace KDL {

class TwistAcc;
typedef Rall2d<double,double,double> doubleAcc;

// Equal is friend function, but default arguments for friends are forbidden (§8.3.6.4)
class FrameAcc;
class RotationAcc;
class VectorAcc;

IMETHOD bool Equal(const FrameAcc& r1,const FrameAcc& r2,double eps=epsilon);
IMETHOD bool Equal(const Frame& r1,const FrameAcc& r2,double eps=epsilon);
IMETHOD bool Equal(const FrameAcc& r1,const Frame& r2,double eps=epsilon);
IMETHOD bool Equal(const RotationAcc& r1,const RotationAcc& r2,double eps=epsilon);
IMETHOD bool Equal(const Rotation& r1,const RotationAcc& r2,double eps=epsilon);
IMETHOD bool Equal(const RotationAcc& r1,const Rotation& r2,double eps=epsilon);
IMETHOD bool Equal(const TwistAcc& a,const TwistAcc& b,double eps=epsilon);
IMETHOD bool Equal(const Twist& a,const TwistAcc& b,double eps=epsilon);
IMETHOD bool Equal(const TwistAcc& a,const Twist& b,double eps=epsilon);
IMETHOD bool Equal(const VectorAcc& r1,const VectorAcc& r2,double eps=epsilon);
IMETHOD bool Equal(const Vector& r1,const VectorAcc& r2,double eps=epsilon);
IMETHOD bool Equal(const VectorAcc& r1,const Vector& r2,double eps=epsilon);

class VectorAcc
{
public:
    Vector p;   //!< position vector
    Vector v;   //!< velocity vector
    Vector dv;  //!< acceleration vector
public:
    VectorAcc():p(),v(),dv() {}
    explicit VectorAcc(const Vector& _p):p(_p),v(Vector::Zero()),dv(Vector::Zero()) {}
    VectorAcc(const Vector& _p,const Vector& _v):p(_p),v(_v),dv(Vector::Zero()) {}
    VectorAcc(const Vector& _p,const Vector& _v,const Vector& _dv):
        p(_p),v(_v),dv(_dv) {}
    IMETHOD VectorAcc& operator = (const VectorAcc& arg);
    IMETHOD VectorAcc& operator = (const Vector& arg);
    IMETHOD VectorAcc& operator += (const VectorAcc& arg);
    IMETHOD VectorAcc& operator -= (const VectorAcc& arg);
    IMETHOD static VectorAcc Zero();
    IMETHOD void ReverseSign();
    IMETHOD doubleAcc Norm();
    IMETHOD friend VectorAcc operator + (const VectorAcc& r1,const VectorAcc& r2);
    IMETHOD friend VectorAcc operator - (const VectorAcc& r1,const VectorAcc& r2);
    IMETHOD friend VectorAcc operator + (const Vector& r1,const VectorAcc& r2);
    IMETHOD friend VectorAcc operator - (const Vector& r1,const VectorAcc& r2);
    IMETHOD friend VectorAcc operator + (const VectorAcc& r1,const Vector& r2);
    IMETHOD friend VectorAcc operator - (const VectorAcc& r1,const Vector& r2);
    IMETHOD friend VectorAcc operator * (const VectorAcc& r1,const VectorAcc& r2);
    IMETHOD friend VectorAcc operator * (const VectorAcc& r1,const Vector& r2);
    IMETHOD friend VectorAcc operator * (const Vector& r1,const VectorAcc& r2);
    IMETHOD friend VectorAcc operator * (const VectorAcc& r1,double r2);
    IMETHOD friend VectorAcc operator * (double r1,const VectorAcc& r2);
    IMETHOD friend VectorAcc operator * (const doubleAcc& r1,const VectorAcc& r2);
    IMETHOD friend VectorAcc operator * (const VectorAcc& r2,const doubleAcc& r1);
    IMETHOD friend VectorAcc operator*(const Rotation& R,const VectorAcc& x);

    IMETHOD friend VectorAcc operator / (const VectorAcc& r1,double r2);
    IMETHOD friend VectorAcc operator / (const VectorAcc& r2,const doubleAcc& r1);


    IMETHOD friend bool Equal(const VectorAcc& r1,const VectorAcc& r2,double eps);
    IMETHOD friend bool Equal(const Vector& r1,const VectorAcc& r2,double eps);
    IMETHOD friend bool Equal(const VectorAcc& r1,const Vector& r2,double eps);
    IMETHOD friend VectorAcc operator - (const VectorAcc& r);
    IMETHOD friend doubleAcc dot(const VectorAcc& lhs,const VectorAcc& rhs);
    IMETHOD friend doubleAcc dot(const VectorAcc& lhs,const Vector& rhs);
    IMETHOD friend doubleAcc dot(const Vector& lhs,const VectorAcc& rhs);
};



class RotationAcc
{
public:
    Rotation R;     //!< rotation matrix
    Vector   w;     //!< angular velocity vector
    Vector   dw;    //!< angular acceration vector
public:
    RotationAcc():R(),w() {}
    explicit RotationAcc(const Rotation& _R):R(_R),w(Vector::Zero()){}
    RotationAcc(const Rotation& _R,const Vector& _w,const Vector& _dw):
        R(_R),w(_w),dw(_dw) {}
    IMETHOD RotationAcc& operator = (const RotationAcc& arg);
    IMETHOD RotationAcc& operator = (const Rotation& arg);
    IMETHOD static RotationAcc Identity();
    IMETHOD RotationAcc Inverse() const;
    IMETHOD VectorAcc Inverse(const VectorAcc& arg) const;
    IMETHOD VectorAcc Inverse(const Vector& arg) const;
    IMETHOD VectorAcc operator*(const VectorAcc& arg) const;
    IMETHOD VectorAcc operator*(const Vector& arg) const;

    //  Rotations
    // The SetRot.. functions set the value of *this to the appropriate rotation matrix.
    // The Rot... static functions give the value of the appropriate rotation matrix back.
    // The DoRot... functions apply a rotation R to *this,such that *this = *this * R.
    // IMETHOD void DoRotX(const doubleAcc& angle);
    // IMETHOD void DoRotY(const doubleAcc& angle);
    // IMETHOD void DoRotZ(const doubleAcc& angle);
    // IMETHOD static RRotation RotX(const doubleAcc& angle);
    // IMETHOD static RRotation RotY(const doubleAcc& angle);
    // IMETHOD static RRotation RotZ(const doubleAcc& angle);

    // IMETHOD void SetRot(const Vector& rotaxis,const doubleAcc& angle);
    // Along an arbitrary axes.  The norm of rotvec is neglected.
    // IMETHOD static RotationAcc Rot(const Vector& rotvec,const doubleAcc& angle);
    // rotvec has arbitrary norm
    // rotation around a constant vector !
    // IMETHOD static RotationAcc Rot2(const Vector& rotvec,const doubleAcc& angle);
    // rotvec is normalized.
    // rotation around a constant vector !

    IMETHOD friend RotationAcc operator* (const RotationAcc& r1,const RotationAcc& r2);
    IMETHOD friend RotationAcc operator* (const Rotation& r1,const RotationAcc& r2);
    IMETHOD friend RotationAcc operator* (const RotationAcc& r1,const Rotation& r2);
    IMETHOD friend bool Equal(const RotationAcc& r1,const RotationAcc& r2,double eps);
    IMETHOD friend bool Equal(const Rotation& r1,const RotationAcc& r2,double eps);
    IMETHOD friend bool Equal(const RotationAcc& r1,const Rotation& r2,double eps);
    IMETHOD TwistAcc Inverse(const TwistAcc& arg) const;
    IMETHOD TwistAcc Inverse(const Twist& arg) const;
    IMETHOD TwistAcc operator * (const TwistAcc& arg) const;
    IMETHOD TwistAcc operator * (const Twist& arg) const;
};




class FrameAcc
{
public:
    RotationAcc M;   //!< Rotation,angular velocity, and angular acceleration of frame.
    VectorAcc   p;   //!< Translation, velocity and acceleration of origin.
public:
    FrameAcc(){}
    explicit FrameAcc(const Frame& _T):M(_T.M),p(_T.p) {}
    FrameAcc(const Frame& _T,const Twist& _t,const Twist& _dt):
        M(_T.M,_t.rot,_dt.rot),p(_T.p,_t.vel,_dt.vel) {}
    FrameAcc(const RotationAcc& _M,const VectorAcc& _p):M(_M),p(_p) {}

    IMETHOD FrameAcc& operator = (const FrameAcc& arg);
    IMETHOD FrameAcc& operator = (const Frame& arg);
    IMETHOD static FrameAcc Identity();
    IMETHOD FrameAcc Inverse() const;
    IMETHOD VectorAcc Inverse(const VectorAcc& arg) const;
    IMETHOD VectorAcc operator*(const VectorAcc& arg) const;
    IMETHOD VectorAcc operator*(const Vector& arg) const;
    IMETHOD VectorAcc Inverse(const Vector& arg) const;
    IMETHOD Frame GetFrame() const;
    IMETHOD Twist GetTwist() const;
    IMETHOD Twist GetAccTwist() const;
    IMETHOD friend FrameAcc operator * (const FrameAcc& f1,const FrameAcc& f2);
    IMETHOD friend FrameAcc operator * (const Frame& f1,const FrameAcc& f2);
    IMETHOD friend FrameAcc operator * (const FrameAcc& f1,const Frame& f2);
    IMETHOD friend bool Equal(const FrameAcc& r1,const FrameAcc& r2,double eps);
    IMETHOD friend bool Equal(const Frame& r1,const FrameAcc& r2,double eps);
    IMETHOD friend bool Equal(const FrameAcc& r1,const Frame& r2,double eps);

    IMETHOD TwistAcc  Inverse(const TwistAcc& arg) const;
    IMETHOD TwistAcc  Inverse(const Twist& arg) const;
    IMETHOD TwistAcc operator * (const TwistAcc& arg) const;
    IMETHOD TwistAcc operator * (const Twist& arg) const;
};








//very similar to Wrench class.
class TwistAcc
{
public:
    VectorAcc vel;       //!< translational velocity and its 1st and 2nd derivative
    VectorAcc rot;       //!< rotational velocity and its 1st and 2nd derivative
public:

     TwistAcc():vel(),rot() {};
     TwistAcc(const VectorAcc& _vel,const VectorAcc& _rot):vel(_vel),rot(_rot) {};

     IMETHOD TwistAcc& operator-=(const TwistAcc& arg);
     IMETHOD TwistAcc& operator+=(const TwistAcc& arg);

     IMETHOD friend TwistAcc operator*(const TwistAcc& lhs,double rhs);
     IMETHOD friend TwistAcc operator*(double lhs,const TwistAcc& rhs);
     IMETHOD friend TwistAcc operator/(const TwistAcc& lhs,double rhs);

     IMETHOD friend TwistAcc operator*(const TwistAcc& lhs,const doubleAcc& rhs);
     IMETHOD friend TwistAcc operator*(const doubleAcc& lhs,const TwistAcc& rhs);
     IMETHOD friend TwistAcc operator/(const TwistAcc& lhs,const doubleAcc& rhs);

     IMETHOD friend TwistAcc operator+(const TwistAcc& lhs,const TwistAcc& rhs);
     IMETHOD friend TwistAcc operator-(const TwistAcc& lhs,const TwistAcc& rhs);
     IMETHOD friend TwistAcc operator-(const TwistAcc& arg);

     IMETHOD friend void SetToZero(TwistAcc& v);

     static IMETHOD TwistAcc Zero();

     IMETHOD void ReverseSign();

     IMETHOD TwistAcc RefPoint(const VectorAcc& v_base_AB);
     // Changes the reference point of the RTwist.
     // The RVector v_base_AB is expressed in the same base as the RTwist
     // The RVector v_base_AB is a RVector from the old point to
     // the new point.
     // Complexity : 6M+6A

     IMETHOD friend bool Equal(const TwistAcc& a,const TwistAcc& b,double eps);
     IMETHOD friend bool Equal(const Twist& a,const TwistAcc& b,double eps);
     IMETHOD friend bool Equal(const TwistAcc& a,const Twist& b,double eps);


     IMETHOD Twist GetTwist() const;
     IMETHOD Twist GetTwistDot() const;

    friend class RotationAcc;
    friend class FrameAcc;

};







#ifdef KDL_INLINE
#include "frameacc.inl"
#endif

}





#endif
