/***************************************************************************
                        frames.inl -  description
                       -------------------------
    begin                : June 2006
    copyright            : (C) 2006 Erwin Aertbelien
    email                : firstname.lastname@mech.kuleuven.ac.be

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
// clazy:excludeall=rule-of-two-soft

/**
 * \file frames.inl
 * Inlined member functions and global functions that relate to the classes in frames.cpp
 *
 */

namespace KDL {

IMETHOD Vector::Vector(const Vector & arg)
{
    data[0] = arg.data[0];
    data[1] = arg.data[1];
    data[2] = arg.data[2];
}

IMETHOD Vector::Vector(double x,double y, double z)
{
        data[0]=x;data[1]=y;data[2]=z;
}


IMETHOD Vector& Vector::operator =(const Vector & arg)
{
    data[0] = arg.data[0];
    data[1] = arg.data[1];
    data[2] = arg.data[2];
    return *this;
}

IMETHOD Vector operator +(const Vector & lhs,const Vector& rhs)
{
    Vector tmp;
    tmp.data[0] = lhs.data[0]+rhs.data[0];
    tmp.data[1] = lhs.data[1]+rhs.data[1];
    tmp.data[2] = lhs.data[2]+rhs.data[2];
    return tmp;
}

IMETHOD Vector operator -(const Vector & lhs,const Vector& rhs)
{
    Vector tmp;
    tmp.data[0] = lhs.data[0]-rhs.data[0];
    tmp.data[1] = lhs.data[1]-rhs.data[1];
    tmp.data[2] = lhs.data[2]-rhs.data[2];
    return tmp;
}

IMETHOD double Vector::x() const { return data[0]; }
IMETHOD double Vector::y() const { return data[1]; }
IMETHOD double Vector::z() const { return data[2]; }

IMETHOD void Vector::x( double _x ) { data[0] = _x; }
IMETHOD void Vector::y( double _y ) { data[1] = _y; }
IMETHOD void Vector::z( double _z ) { data[2] = _z; }

Vector operator *(const Vector& lhs,double rhs) 
{
    Vector tmp;
    tmp.data[0] = lhs.data[0]*rhs;
    tmp.data[1] = lhs.data[1]*rhs;
    tmp.data[2] = lhs.data[2]*rhs;
    return tmp;
}

Vector operator *(double lhs,const Vector& rhs) 
{
    Vector tmp;
    tmp.data[0] = lhs*rhs.data[0];
    tmp.data[1] = lhs*rhs.data[1];
    tmp.data[2] = lhs*rhs.data[2];
    return tmp;
}

Vector operator /(const Vector& lhs,double rhs) 
{
    Vector tmp;
    tmp.data[0] = lhs.data[0]/rhs;
    tmp.data[1] = lhs.data[1]/rhs;
    tmp.data[2] = lhs.data[2]/rhs;
    return tmp;
}

Vector operator *(const Vector & lhs,const Vector& rhs)
// Complexity : 6M+3A
{
    Vector tmp;
    tmp.data[0] = lhs.data[1]*rhs.data[2]-lhs.data[2]*rhs.data[1];
    tmp.data[1] = lhs.data[2]*rhs.data[0]-lhs.data[0]*rhs.data[2];
    tmp.data[2] = lhs.data[0]*rhs.data[1]-lhs.data[1]*rhs.data[0];
    return tmp;
}

Vector& Vector::operator +=(const Vector & arg)
// Complexity : 3A
{
    data[0]+=arg.data[0];
    data[1]+=arg.data[1];
    data[2]+=arg.data[2];
    return *this;
}

Vector& Vector::operator -=(const Vector & arg)
// Complexity : 3A
{
    data[0]-=arg.data[0];
    data[1]-=arg.data[1];
    data[2]-=arg.data[2];
    return *this;
}

Vector Vector::Zero()
{
    return Vector(0,0,0);
}

double Vector::operator()(int index) const {
    FRAMES_CHECKI((0<=index)&&(index<=2));
    return data[index];
}

double& Vector::operator () (int index)
{
    FRAMES_CHECKI((0<=index)&&(index<=2));
    return data[index];
}



Wrench Frame::operator * (const Wrench& arg) const
// Complexity : 24M+18A
{
    Wrench tmp;
    tmp.force  = M*arg.force;
    tmp.torque = M*arg.torque + p*tmp.force;
    return tmp;
}

Wrench Frame::Inverse(const Wrench& arg) const
{
    Wrench tmp;
    tmp.force =  M.Inverse(arg.force);
    tmp.torque = M.Inverse(arg.torque-p*arg.force);
    return tmp;
}



Wrench Rotation::Inverse(const Wrench& arg) const
{
    return Wrench(Inverse(arg.force),Inverse(arg.torque));
}

Twist Rotation::Inverse(const Twist& arg) const
{
    return Twist(Inverse(arg.vel),Inverse(arg.rot));
}

Wrench Wrench::Zero()
{
    return Wrench(Vector::Zero(),Vector::Zero());
}


void Wrench::ReverseSign()
{   
    torque.ReverseSign();
    force.ReverseSign();
}

Wrench Wrench::RefPoint(const Vector& v_base_AB) const
     // Changes the reference point of the Wrench.
     // The vector v_base_AB is expressed in the same base as the twist
     // The vector v_base_AB is a vector from the old point to
     // the new point.
{
    return Wrench(this->force,
                  this->torque+this->force*v_base_AB
                  );
}


Wrench& Wrench::operator-=(const Wrench& arg)
{
    torque-=arg.torque;
    force -=arg.force;
    return *this;
}

Wrench& Wrench::operator+=(const Wrench& arg)
{
    torque+=arg.torque;
    force +=arg.force;
    return *this;
}

double& Wrench::operator()(int i)
{
    // assert((0<=i)&&(i<6)); done by underlying routines
    if (i<3) 
        return force(i);
    else
        return torque(i-3);
}

double Wrench::operator()(int i) const
{
    // assert((0<=i)&&(i<6)); done by underlying routines
    if (i<3) 
        return force(i);
    else
        return torque(i-3);
}


Wrench operator*(const Wrench& lhs,double rhs)
{
    return Wrench(lhs.force*rhs,lhs.torque*rhs);
}

Wrench operator*(double lhs,const Wrench& rhs)
{
    return Wrench(lhs*rhs.force,lhs*rhs.torque);
}

Wrench operator/(const Wrench& lhs,double rhs)
{
    return Wrench(lhs.force/rhs,lhs.torque/rhs);
}

// addition of Wrench's
Wrench operator+(const Wrench& lhs,const Wrench& rhs)
{
    return Wrench(lhs.force+rhs.force,lhs.torque+rhs.torque);
}

Wrench operator-(const Wrench& lhs,const Wrench& rhs)
{
    return Wrench(lhs.force-rhs.force,lhs.torque-rhs.torque);
}

// unary -
Wrench operator-(const Wrench& arg) 
{
    return Wrench(-arg.force,-arg.torque);
}

Twist Frame::operator * (const Twist& arg) const
// Complexity : 24M+18A
{
    Twist tmp;
    tmp.rot = M*arg.rot;
    tmp.vel = M*arg.vel+p*tmp.rot;
    return tmp;
}
Twist Frame::Inverse(const Twist& arg) const
{
    Twist tmp;
    tmp.rot =  M.Inverse(arg.rot);
    tmp.vel = M.Inverse(arg.vel-p*arg.rot);
    return tmp;
}

Twist Twist::Zero()
{
    return Twist(Vector::Zero(),Vector::Zero());
}


void Twist::ReverseSign()
{   
    vel.ReverseSign();
    rot.ReverseSign();
}

Twist Twist::RefPoint(const Vector& v_base_AB) const
     // Changes the reference point of the twist.
     // The vector v_base_AB is expressed in the same base as the twist
     // The vector v_base_AB is a vector from the old point to
     // the new point.
     // Complexity : 6M+6A
{
    return Twist(this->vel+this->rot*v_base_AB,this->rot);
}

Twist& Twist::operator-=(const Twist& arg)
{
    vel-=arg.vel;
    rot -=arg.rot;
    return *this;
}

Twist& Twist::operator+=(const Twist& arg)
{
    vel+=arg.vel;
    rot +=arg.rot;
    return *this;
}

double& Twist::operator()(int i)
{
    // assert((0<=i)&&(i<6)); done by underlying routines
    if (i<3) 
        return vel(i);
    else
        return rot(i-3);
}

double Twist::operator()(int i) const
{
    // assert((0<=i)&&(i<6)); done by underlying routines
    if (i<3) 
        return vel(i);
    else
        return rot(i-3);
}


Twist operator*(const Twist& lhs,double rhs)
{
    return Twist(lhs.vel*rhs,lhs.rot*rhs);
}

Twist operator*(double lhs,const Twist& rhs)
{
    return Twist(lhs*rhs.vel,lhs*rhs.rot);
}

Twist operator/(const Twist& lhs,double rhs)
{
    return Twist(lhs.vel/rhs,lhs.rot/rhs);
}

// addition of Twist's
Twist operator+(const Twist& lhs,const Twist& rhs)
{
    return Twist(lhs.vel+rhs.vel,lhs.rot+rhs.rot);
}

Twist operator-(const Twist& lhs,const Twist& rhs)
{
    return Twist(lhs.vel-rhs.vel,lhs.rot-rhs.rot);
}

// unary -
Twist operator-(const Twist& arg) 
{
    return Twist(-arg.vel,-arg.rot);
}


//Spatial products for twists
Twist operator*(const Twist& lhs,const Twist& rhs)
{
    return Twist(lhs.rot*rhs.vel+lhs.vel*rhs.rot,lhs.rot*rhs.rot);
}
Wrench operator*(const Twist& lhs,const Wrench& rhs)
{
    return Wrench(lhs.rot*rhs.force,lhs.rot*rhs.torque+lhs.vel*rhs.force);
}

Frame::Frame(const Rotation & R)
{
    M=R;
    p=Vector::Zero();
}

Frame::Frame(const Vector & V)
{
    M = Rotation::Identity();
    p = V;
}

Frame::Frame(const Rotation & R, const Vector & V)
{
    M = R;
    p = V;
}

 Frame operator *(const Frame& lhs,const Frame& rhs)
// Complexity : 36M+36A
{
    return Frame(lhs.M*rhs.M,lhs.M*rhs.p+lhs.p);
}

Vector Frame::operator *(const Vector & arg) const
{
    return M*arg+p;
}

Vector Frame::Inverse(const Vector& arg) const
{
    return M.Inverse(arg-p);
}

Frame Frame::Inverse() const
{
    return Frame(M.Inverse(),-M.Inverse(p));
}


Frame& Frame::operator =(const Frame & arg)
{ 
    M = arg.M;
    p = arg.p;
    return *this;
}

Frame::Frame(const Frame & arg) :
    p(arg.p),M(arg.M)
{}


void Vector::ReverseSign()
{
    data[0] = -data[0];
    data[1] = -data[1];
    data[2] = -data[2];
}



Vector operator-(const Vector & arg)
{
    Vector tmp;
    tmp.data[0]=-arg.data[0];
    tmp.data[1]=-arg.data[1];
    tmp.data[2]=-arg.data[2];
    return tmp;
}

void Vector::Set2DXY(const Vector2& v)
// a 3D vector where the 2D vector v is put in the XY plane
{
    data[0]=v(0);
    data[1]=v(1);
    data[2]=0;

}
void Vector::Set2DYZ(const Vector2& v)
// a 3D vector where the 2D vector v is put in the YZ plane
{
    data[1]=v(0);
    data[2]=v(1);
    data[0]=0;

}

void Vector::Set2DZX(const Vector2& v)
// a 3D vector where the 2D vector v is put in the ZX plane
{
    data[2]=v(0);
    data[0]=v(1);
    data[1]=0;

}





double& Rotation::operator()(int i,int j) {
    FRAMES_CHECKI((0<=i)&&(i<=2)&&(0<=j)&&(j<=2));
    return data[i*3+j];
}

double Rotation::operator()(int i,int j) const {
    FRAMES_CHECKI((0<=i)&&(i<=2)&&(0<=j)&&(j<=2));
    return data[i*3+j];
}

Rotation::Rotation( double Xx,double Yx,double Zx,
                            double Xy,double Yy,double Zy,
                            double Xz,double Yz,double Zz)
{
    data[0] = Xx;data[1]=Yx;data[2]=Zx;
    data[3] = Xy;data[4]=Yy;data[5]=Zy;
    data[6] = Xz;data[7]=Yz;data[8]=Zz;
}


Rotation::Rotation(const Vector& x,const Vector& y,const Vector& z) 
{
    data[0] = x.data[0];data[3] = x.data[1];data[6] = x.data[2];
    data[1] = y.data[0];data[4] = y.data[1];data[7] = y.data[2];
    data[2] = z.data[0];data[5] = z.data[1];data[8] = z.data[2];
}

Rotation& Rotation::operator=(const Rotation& arg) {
    int count=9;
    while (count--) data[count] = arg.data[count];
    return *this;
}

Vector Rotation::operator*(const Vector& v) const {
// Complexity : 9M+6A
    return Vector(
     data[0]*v.data[0] + data[1]*v.data[1] + data[2]*v.data[2],
     data[3]*v.data[0] + data[4]*v.data[1] + data[5]*v.data[2],
     data[6]*v.data[0] + data[7]*v.data[1] + data[8]*v.data[2] 
    );
}

Twist Rotation::operator * (const Twist& arg) const
     // Transformation of the base to which the twist is expressed.
     // look at Frame*Twist for a transformation that also transforms
     // the velocity reference point.
     // Complexity : 18M+12A
{
    return Twist((*this)*arg.vel,(*this)*arg.rot);
}

Wrench Rotation::operator * (const Wrench& arg) const
     // Transformation of the base to which the wrench is expressed.
     // look at Frame*Twist for a transformation that also transforms
     // the force reference point.
{
    return Wrench((*this)*arg.force,(*this)*arg.torque);
}

Rotation Rotation::Identity() {
        return Rotation(1,0,0,0,1,0,0,0,1);
}
// *this = *this * ROT(X,angle)
void Rotation::DoRotX(double angle)
{
    double cs = cos(angle);
    double sn = sin(angle);
    double x1,x2,x3;
    x1  = cs* (*this)(0,1) + sn* (*this)(0,2);
    x2  = cs* (*this)(1,1) + sn* (*this)(1,2);
    x3  = cs* (*this)(2,1) + sn* (*this)(2,2);
    (*this)(0,2) = -sn* (*this)(0,1) + cs* (*this)(0,2);
    (*this)(1,2) = -sn* (*this)(1,1) + cs* (*this)(1,2);
    (*this)(2,2) = -sn* (*this)(2,1) + cs* (*this)(2,2);
    (*this)(0,1) = x1;
    (*this)(1,1) = x2;
    (*this)(2,1) = x3;
}

void Rotation::DoRotY(double angle)
{
    double cs = cos(angle);
    double sn = sin(angle);
    double x1,x2,x3;
    x1  = cs* (*this)(0,0) - sn* (*this)(0,2);
    x2  = cs* (*this)(1,0) - sn* (*this)(1,2);
    x3  = cs* (*this)(2,0) - sn* (*this)(2,2);
    (*this)(0,2) = sn* (*this)(0,0) + cs* (*this)(0,2);
    (*this)(1,2) = sn* (*this)(1,0) + cs* (*this)(1,2);
    (*this)(2,2) = sn* (*this)(2,0) + cs* (*this)(2,2);
    (*this)(0,0) = x1;
    (*this)(1,0) = x2;
    (*this)(2,0) = x3;
}

void Rotation::DoRotZ(double angle)
{
    double cs = cos(angle);
    double sn = sin(angle);
    double x1,x2,x3;
    x1  = cs* (*this)(0,0) + sn* (*this)(0,1);
    x2  = cs* (*this)(1,0) + sn* (*this)(1,1);
    x3  = cs* (*this)(2,0) + sn* (*this)(2,1);
    (*this)(0,1) = -sn* (*this)(0,0) + cs* (*this)(0,1);
    (*this)(1,1) = -sn* (*this)(1,0) + cs* (*this)(1,1);
    (*this)(2,1) = -sn* (*this)(2,0) + cs* (*this)(2,1);
    (*this)(0,0) = x1;
    (*this)(1,0) = x2;
    (*this)(2,0) = x3;
}


Rotation Rotation::RotX(double angle) {
    double cs=cos(angle);
    double sn=sin(angle);
    return Rotation(1,0,0,0,cs,-sn,0,sn,cs);
}
Rotation Rotation::RotY(double angle) {
    double cs=cos(angle);
    double sn=sin(angle);
    return Rotation(cs,0,sn,0,1,0,-sn,0,cs);
}
Rotation Rotation::RotZ(double angle) {
    double cs=cos(angle);
    double sn=sin(angle);
    return Rotation(cs,-sn,0,sn,cs,0,0,0,1);
}




void Frame::Integrate(const Twist& t_this,double samplefrequency)
{
    double n = t_this.rot.Norm()/samplefrequency;
    if (n<epsilon) {
        p += M*(t_this.vel/samplefrequency);
    } else {
        (*this) = (*this) * 
            Frame ( Rotation::Rot( t_this.rot, n ),
                    t_this.vel/samplefrequency
                );
    }
}

Rotation Rotation::Inverse() const
{
    Rotation tmp(*this);
    tmp.SetInverse();
    return tmp;
}

Vector Rotation::Inverse(const Vector& v) const {
    return Vector(
     data[0]*v.data[0] + data[3]*v.data[1] + data[6]*v.data[2],
     data[1]*v.data[0] + data[4]*v.data[1] + data[7]*v.data[2],
     data[2]*v.data[0] + data[5]*v.data[1] + data[8]*v.data[2] 
    );
}


void Rotation::SetInverse()
{
    double tmp;
    tmp = data[1];data[1]=data[3];data[3]=tmp;
    tmp = data[2];data[2]=data[6];data[6]=tmp;
    tmp = data[5];data[5]=data[7];data[7]=tmp;
}







double Frame::operator()(int i,int j) {
    FRAMES_CHECKI((0<=i)&&(i<=3)&&(0<=j)&&(j<=3));
    if (i==3) {
        if (j==3)
            return 1.0;
        else
            return 0.0;
    } else {
        if (j==3) 
            return p(i);
        else
            return M(i,j);

    }
}

double Frame::operator()(int i,int j) const {
    FRAMES_CHECKI((0<=i)&&(i<=3)&&(0<=j)&&(j<=3));
        if (i==3) {
        if (j==3)
            return 1;
        else
            return 0;
    } else {
        if (j==3) 
            return p(i);
        else
            return M(i,j);

    }
}


Frame Frame::Identity() {
    return Frame(Rotation::Identity(),Vector::Zero());
}




void Vector::Set2DPlane(const Frame& F_someframe_XY,const Vector2& v_XY)
// a 3D vector where the 2D vector v is put in the XY plane of the frame
// F_someframe_XY.
{
Vector tmp_XY;
tmp_XY.Set2DXY(v_XY);
tmp_XY = F_someframe_XY*(tmp_XY);
}









//============ 2 dimensional version of the frames objects =============
IMETHOD Vector2::Vector2(const Vector2 & arg)
{
    data[0] = arg.data[0];
    data[1] = arg.data[1];
}

IMETHOD Vector2::Vector2(double x,double y)
{
        data[0]=x;data[1]=y;
}


IMETHOD Vector2& Vector2::operator =(const Vector2 & arg)
{
    data[0] = arg.data[0];
    data[1] = arg.data[1];
    return *this;
}


IMETHOD Vector2 operator +(const Vector2 & lhs,const Vector2& rhs)
{
    return Vector2(lhs.data[0]+rhs.data[0],lhs.data[1]+rhs.data[1]);
}

IMETHOD Vector2 operator -(const Vector2 & lhs,const Vector2& rhs)
{
    return Vector2(lhs.data[0]-rhs.data[0],lhs.data[1]-rhs.data[1]);
}

IMETHOD Vector2 operator *(const Vector2& lhs,double rhs) 
{
    return Vector2(lhs.data[0]*rhs,lhs.data[1]*rhs);
}

IMETHOD Vector2 operator *(double lhs,const Vector2& rhs) 
{
    return Vector2(lhs*rhs.data[0],lhs*rhs.data[1]);
}

IMETHOD Vector2 operator /(const Vector2& lhs,double rhs) 
{
    return Vector2(lhs.data[0]/rhs,lhs.data[1]/rhs);
}

IMETHOD Vector2& Vector2::operator +=(const Vector2 & arg)
{
    data[0]+=arg.data[0];
    data[1]+=arg.data[1];
    return *this;
}

IMETHOD Vector2& Vector2::operator -=(const Vector2 & arg)
{
    data[0]-=arg.data[0];
    data[1]-=arg.data[1];
    return *this;
}

IMETHOD Vector2 Vector2::Zero() {
    return Vector2(0,0);
}

IMETHOD double Vector2::operator()(int index) const {
    FRAMES_CHECKI((0<=index)&&(index<=1));
    return data[index];
}

IMETHOD double& Vector2::operator () (int index)
{
    FRAMES_CHECKI((0<=index)&&(index<=1));
    return data[index];
}

IMETHOD double Vector2::x() const { return data[0]; }
IMETHOD double Vector2::y() const { return data[1]; }

IMETHOD void Vector2::x( double _x ) { data[0] = _x; }
IMETHOD void Vector2::y( double _y ) { data[1] = _y; }


IMETHOD void Vector2::ReverseSign()
{
    data[0] = -data[0];
    data[1] = -data[1];
}


IMETHOD Vector2 operator-(const Vector2 & arg)
{
    return Vector2(-arg.data[0],-arg.data[1]);
}


IMETHOD void Vector2::Set3DXY(const Vector& v)
// projects v in its XY plane, and sets *this to these values
{
    data[0]=v(0);
    data[1]=v(1);
}
IMETHOD void Vector2::Set3DYZ(const Vector& v)
// projects v in its XY plane, and sets *this to these values
{
    data[0]=v(1);
    data[1]=v(2);
}
IMETHOD void Vector2::Set3DZX(const Vector& v)
// projects v in its XY plane, and sets *this to these values
{
    data[0]=v(2);
    data[1]=v(0);
}

IMETHOD void Vector2::Set3DPlane(const Frame& F_someframe_XY,const Vector& v_someframe) 
// projects v in the XY plane of F_someframe_XY, and sets *this to these values
// expressed wrt someframe.
{
    Vector tmp = F_someframe_XY.Inverse(v_someframe);
    data[0]=tmp(0);
    data[1]=tmp(1);
}



IMETHOD Rotation2& Rotation2::operator=(const Rotation2& arg) {
    c=arg.c;s=arg.s;
    return *this;
}

IMETHOD Vector2 Rotation2::operator*(const Vector2& v) const {
    return Vector2(v.data[0]*c-v.data[1]*s,v.data[0]*s+v.data[1]*c);
}

IMETHOD double Rotation2::operator()(int i,int j) const {
    FRAMES_CHECKI((0<=i)&&(i<=1)&&(0<=j)&&(j<=1));
    if (i==j) return c;
    if (i==0) 
        return s;
    else
        return -s;
}


IMETHOD Rotation2 operator *(const Rotation2& lhs,const Rotation2& rhs) {
    return Rotation2(lhs.c*rhs.c-lhs.s*rhs.s,lhs.s*rhs.c+lhs.c*rhs.s);
}

IMETHOD void Rotation2::SetInverse() {
    s=-s;
}

IMETHOD Rotation2 Rotation2::Inverse() const {
    return Rotation2(c,-s);
}

IMETHOD Vector2 Rotation2::Inverse(const Vector2& v) const {
    return Vector2(v.data[0]*c+v.data[1]*s,-v.data[0]*s+v.data[1]*c);
}

IMETHOD Rotation2 Rotation2::Identity() {
    return Rotation2(1,0);
}
     
IMETHOD void Rotation2::SetIdentity()
{
	c = 1;
	s = 0;
}

IMETHOD void Rotation2::SetRot(double angle) {
    c=cos(angle);s=sin(angle);
}

IMETHOD Rotation2 Rotation2::Rot(double angle) {
    return Rotation2(cos(angle),sin(angle));
}

IMETHOD double Rotation2::GetRot() const {
    return atan2(s,c);
}


IMETHOD Frame2::Frame2() {
}

IMETHOD Frame2::Frame2(const Rotation2 & R)
{
    M=R;
    p=Vector2::Zero();
}

IMETHOD Frame2::Frame2(const Vector2 & V)
{
    M = Rotation2::Identity();
    p = V;
}

IMETHOD Frame2::Frame2(const Rotation2 & R, const Vector2 & V)
{
    M = R;
    p = V;
}

IMETHOD Frame2 operator *(const Frame2& lhs,const Frame2& rhs)
{
    return Frame2(lhs.M*rhs.M,lhs.M*rhs.p+lhs.p);
}

IMETHOD Vector2 Frame2::operator *(const Vector2 & arg)
{
    return M*arg+p;
}

IMETHOD Vector2 Frame2::Inverse(const Vector2& arg) const
{
    return M.Inverse(arg-p);
}

IMETHOD void Frame2::SetIdentity()
{
	M.SetIdentity();
	p = Vector2::Zero();
}

IMETHOD void Frame2::SetInverse()
{
    M.SetInverse();
    p = M*p;
    p.ReverseSign();
}


IMETHOD Frame2 Frame2::Inverse() const
{
    Frame2 tmp(*this);
    tmp.SetInverse();
    return tmp;
}

IMETHOD Frame2& Frame2::operator =(const Frame2 & arg)
{ 
    M = arg.M;
    p = arg.p;
    return *this;
}

IMETHOD Frame2::Frame2(const Frame2 & arg) :
    p(arg.p), M(arg.M)
{}


IMETHOD double Frame2::operator()(int i,int j) {
    FRAMES_CHECKI((0<=i)&&(i<=2)&&(0<=j)&&(j<=2));
    if (i==2) {
        if (j==2)
            return 1;
        else
            return 0;
    } else {
        if (j==2) 
            return p(i);
        else
            return M(i,j);

    }
}

IMETHOD double Frame2::operator()(int i,int j) const {
    FRAMES_CHECKI((0<=i)&&(i<=2)&&(0<=j)&&(j<=2));
    if (i==2) {
        if (j==2)
            return 1;
        else
            return 0;
    } else {
        if (j==2) 
            return p(i);
        else
            return M(i,j);

    }
}

// Scalar products.

IMETHOD double dot(const Vector& lhs,const Vector& rhs) {
    return rhs(0)*lhs(0)+rhs(1)*lhs(1)+rhs(2)*lhs(2);
}

IMETHOD double dot(const Twist& lhs,const Wrench& rhs) {
    return dot(lhs.vel,rhs.force)+dot(lhs.rot,rhs.torque);
}

IMETHOD double dot(const Wrench& rhs,const Twist& lhs) {
    return dot(lhs.vel,rhs.force)+dot(lhs.rot,rhs.torque);
}





// Equality operators



IMETHOD bool Equal(const Vector& a,const Vector& b,double eps) {
        return (Equal(a.data[0],b.data[0],eps)&&
                Equal(a.data[1],b.data[1],eps)&&
                Equal(a.data[2],b.data[2],eps)   );
     }
     

IMETHOD bool Equal(const Frame& a,const Frame& b,double eps) {
        return (Equal(a.p,b.p,eps)&&
                Equal(a.M,b.M,eps)   );
}

IMETHOD bool Equal(const Wrench& a,const Wrench& b,double eps) {
        return (Equal(a.force,b.force,eps)&&
                Equal(a.torque,b.torque,eps)  );
}

IMETHOD bool Equal(const Twist& a,const Twist& b,double eps) {
        return (Equal(a.rot,b.rot,eps)&&
                Equal(a.vel,b.vel,eps)  );
}

IMETHOD bool Equal(const Vector2& a,const Vector2& b,double eps) {
        return (Equal(a.data[0],b.data[0],eps)&&
                Equal(a.data[1],b.data[1],eps)   );
     }
     
IMETHOD bool Equal(const Rotation2& a,const Rotation2& b,double eps) {
    return ( Equal(a.c,b.c,eps) && Equal(a.s,b.s,eps) );
}

IMETHOD bool Equal(const Frame2& a,const Frame2& b,double eps) {
        return (Equal(a.p,b.p,eps)&&
                Equal(a.M,b.M,eps)   );
}

IMETHOD void SetToZero(Vector& v) {
    v=Vector::Zero();
}
IMETHOD void SetToZero(Twist& v) {
    SetToZero(v.rot);
    SetToZero(v.vel);
}
IMETHOD void SetToZero(Wrench& v) {
    SetToZero(v.force);
    SetToZero(v.torque);
}

IMETHOD void SetToZero(Vector2& v) {
    v = Vector2::Zero();
}


////////////////////////////////////////////////////////////////
// The following defines the operations
//   diff
//   addDelta
//   random
//   posrandom
// on all the types defined in this library.
// (mostly for uniform integration, differentiation and testing).
// Defined as functions because double is not a class and a method
// would brake uniformity when defined for a double.
////////////////////////////////////////////////////////////////






/**
 * axis_a_b is a rotation vector, its norm is a rotation angle
 * axis_a_b rotates the a frame towards the b frame.
 * This routine returns the rotation matrix R_a_b
 */
IMETHOD Rotation Rot(const Vector& axis_a_b) {
    // The formula is 
    // V.(V.tr) + st*[V x] + ct*(I-V.(V.tr))
    // can be found by multiplying it with an arbitrary vector p
    // and noting that this vector is rotated.
	Vector rotvec = axis_a_b;
	double angle = rotvec.Normalize(1E-10);
    double ct = ::cos(angle);
    double st = ::sin(angle);
    double vt = 1-ct;
    return Rotation(
        ct            +  vt*rotvec(0)*rotvec(0), 
        -rotvec(2)*st +  vt*rotvec(0)*rotvec(1), 
        rotvec(1)*st  +  vt*rotvec(0)*rotvec(2),
        rotvec(2)*st  +  vt*rotvec(1)*rotvec(0),
        ct            +  vt*rotvec(1)*rotvec(1),
        -rotvec(0)*st +  vt*rotvec(1)*rotvec(2),
        -rotvec(1)*st +  vt*rotvec(2)*rotvec(0),
        rotvec(0)*st  +  vt*rotvec(2)*rotvec(1),
        ct            +  vt*rotvec(2)*rotvec(2)
        );
    }
IMETHOD Vector diff(const Vector& a,const Vector& b,double dt) {
	return (b-a)/dt;
}

IMETHOD Vector diff(const Rotation& R_a_b1,const Rotation& R_a_b2,double dt) {
	Rotation R_b1_b2(R_a_b1.Inverse()*R_a_b2);
	return R_a_b1 * R_b1_b2.GetRot() / dt;
}

IMETHOD Twist diff(const Frame& F_a_b1,const Frame& F_a_b2,double dt) {
	return Twist(
			diff(F_a_b1.p,F_a_b2.p,dt),
			diff(F_a_b1.M,F_a_b2.M,dt)
			);
}
IMETHOD Twist diff(const Twist& a,const Twist& b,double dt) {
	return Twist(diff(a.vel,b.vel,dt),diff(a.rot,b.rot,dt));
}

IMETHOD Wrench diff(const Wrench& a,const Wrench& b,double dt) {
	return Wrench(
			diff(a.force,b.force,dt),
			diff(a.torque,b.torque,dt)
			);
}


IMETHOD Vector addDelta(const Vector& a,const Vector&da,double dt) {
	return a+da*dt;
}

IMETHOD Rotation addDelta(const Rotation& a,const Vector&da,double dt) {
	return a*Rot(a.Inverse(da)*dt);
}
IMETHOD Frame addDelta(const Frame& a,const Twist& da,double dt) {
	return Frame(
			addDelta(a.M,da.rot,dt),
			addDelta(a.p,da.vel,dt)
		   );
}
IMETHOD Twist addDelta(const Twist& a,const Twist&da,double dt) {
	return Twist(addDelta(a.vel,da.vel,dt),addDelta(a.rot,da.rot,dt));
}
IMETHOD Wrench addDelta(const Wrench& a,const Wrench&da,double dt) {
	return Wrench(addDelta(a.force,da.force,dt),addDelta(a.torque,da.torque,dt));
}


/**
 * \brief addDelta operator for displacement rotational velocity.
 *
 * The Vector arguments here represent a displacement rotational velocity.  i.e. a rotation
 * around a fixed axis for a certain angle.  For this representation you cannot use diff() but
 * have to use diff_displ().
 *
 * \param a  : displacement rotational velocity
 * \param da : rotational velocity 
 * \return   displacement rotational velocity
 *
 * \warning do not confuse displacement rotational velocities and velocities
 * \warning do not confuse displacement twist and twist.
 *
IMETHOD Vector addDelta_displ(const Vector& a,const Vector&da,double dt) {
    return getRot(addDelta(Rot(a),da,dt)); 
}*/

/**
 * \brief addDelta operator for displacement twist.
 *
 * The Vector arguments here represent a displacement rotational velocity.  i.e. a rotation
 * around a fixed axis for a certain angle.  For this representation you cannot use diff() but
 * have to use diff_displ().
 *
 * \param a  : displacement twist 
 * \param da : twist 
 * \return   displacement twist 
 *
 * \warning do not confuse displacement rotational velocities and velocities
 * \warning do not confuse displacement twist and twist.
 *
IMETHOD Twist addDelta_displ(const Twist& a,const Twist&da,double dt) {
    return Twist(addDelta(a.vel,da.vel,dt),addDelta_displ(a.rot,da.rot,dt)); 
}*/


IMETHOD void random(Vector& a) {
	random(a[0]);
	random(a[1]);
	random(a[2]);
}
IMETHOD void random(Twist& a) {
	random(a.rot);
	random(a.vel);
}
IMETHOD void random(Wrench& a) {
   	random(a.torque);
	random(a.force);
}

IMETHOD void random(Rotation& R) {
	double alfa;
	double beta;
	double gamma;
	random(alfa);
	random(beta);
	random(gamma);
	R = Rotation::EulerZYX(alfa,beta,gamma);
}

IMETHOD void random(Frame& F) {
	random(F.M);
	random(F.p);
}

IMETHOD void posrandom(Vector& a) {
	posrandom(a[0]);
	posrandom(a[1]);
	posrandom(a[2]);
}
IMETHOD void posrandom(Twist& a) {
	posrandom(a.rot);
	posrandom(a.vel);
}
IMETHOD void posrandom(Wrench& a) {
   	posrandom(a.torque);
	posrandom(a.force);
}

IMETHOD void posrandom(Rotation& R) {
	double alfa;
	double beta;
	double gamma;
	posrandom(alfa);
	posrandom(beta);
	posrandom(gamma);
	R = Rotation::EulerZYX(alfa,beta,gamma);
}

IMETHOD void posrandom(Frame& F) {
	random(F.M);
	random(F.p);
}




IMETHOD bool operator==(const Frame& a,const Frame& b ) {
#ifdef KDL_USE_EQUAL
    return Equal(a,b,epsilon);
#else
        return (a.p == b.p &&
                a.M == b.M );
#endif
}

IMETHOD bool operator!=(const Frame& a,const Frame& b) {
    return !operator==(a,b);
}

IMETHOD bool operator==(const Vector& a,const Vector& b) {
#ifdef KDL_USE_EQUAL
    return Equal(a,b,epsilon);
#else
        return (a.data[0]==b.data[0]&&
                a.data[1]==b.data[1]&&
                a.data[2]==b.data[2] );
#endif
     }

IMETHOD bool operator!=(const Vector& a,const Vector& b) {
    return !operator==(a,b);
}

IMETHOD bool operator==(const Twist& a,const Twist& b) {
#ifdef KDL_USE_EQUAL
    return Equal(a,b,epsilon);
#else
        return (a.rot==b.rot &&
                a.vel==b.vel  );
#endif
}

IMETHOD bool operator!=(const Twist& a,const Twist& b) {
    return !operator==(a,b);
}

IMETHOD bool operator==(const Wrench& a,const Wrench& b ) {
#ifdef KDL_USE_EQUAL
    return Equal(a,b,epsilon);
#else
    return (a.force==b.force &&
            a.torque==b.torque );
#endif
}

IMETHOD bool operator!=(const Wrench& a,const Wrench& b) {
    return !operator==(a,b);
}
IMETHOD bool operator!=(const Rotation& a,const Rotation& b) {
    return !operator==(a,b);
}

IMETHOD bool operator==(const Vector2& a,const Vector2& b) {
#ifdef KDL_USE_EQUAL
    return Equal(a,b,epsilon);
#else
        return (a.data[0]==b.data[0]&&
                a.data[1]==b.data[1] );
#endif
     }

IMETHOD bool operator!=(const Vector2& a,const Vector2& b) {
    return !operator==(a,b);
}

} // namespace KDL
