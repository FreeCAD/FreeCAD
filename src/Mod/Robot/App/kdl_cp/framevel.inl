/*****************************************************************************
 * \file  
 *      provides inline functions of rframes.h
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
 *      $Id: rframes.inl,v 1.1.1.1 2002/08/26 14:14:21 rmoreas Exp $
 *      $Name:  $ 
 ****************************************************************************/


// Methods and operators related to FrameVelVel
// They all delegate most of the work to RotationVelVel and VectorVelVel
FrameVel& FrameVel::operator = (const FrameVel& arg) {
    M=arg.M;
    p=arg.p;
    return *this;
}

FrameVel FrameVel::Identity() {
    return FrameVel(RotationVel::Identity(),VectorVel::Zero());
}


FrameVel operator *(const FrameVel& lhs,const FrameVel& rhs)
{
    return FrameVel(lhs.M*rhs.M,lhs.M*rhs.p+lhs.p);
}
FrameVel operator *(const FrameVel& lhs,const Frame& rhs)
{
    return FrameVel(lhs.M*rhs.M,lhs.M*rhs.p+lhs.p);
}
FrameVel operator *(const Frame& lhs,const FrameVel& rhs)
{
    return FrameVel(lhs.M*rhs.M , lhs.M*rhs.p+lhs.p );
}

VectorVel FrameVel::operator *(const VectorVel & arg) const
{
    return M*arg+p;
}
VectorVel FrameVel::operator *(const Vector & arg) const
{
    return M*arg+p;
}

VectorVel FrameVel::Inverse(const VectorVel& arg) const
{
    return M.Inverse(arg-p);
}

VectorVel FrameVel::Inverse(const Vector& arg) const
{
    return M.Inverse(arg-p);
}

FrameVel FrameVel::Inverse() const
{
    return FrameVel(M.Inverse(),-M.Inverse(p));
}

FrameVel& FrameVel::operator = (const Frame& arg) {
    M = arg.M;
    p = arg.p;
    return *this;
}
bool Equal(const FrameVel& r1,const FrameVel& r2,double eps) {
    return (Equal(r1.M,r2.M,eps) && Equal(r1.p,r2.p,eps));
}
bool Equal(const Frame& r1,const FrameVel& r2,double eps) {
    return (Equal(r1.M,r2.M,eps) && Equal(r1.p,r2.p,eps));
}
bool Equal(const FrameVel& r1,const Frame& r2,double eps) {
    return (Equal(r1.M,r2.M,eps) && Equal(r1.p,r2.p,eps));
}

Frame FrameVel::GetFrame() const {
    return Frame(M.R,p.p);
}

Twist FrameVel::GetTwist() const {
    return Twist(p.v,M.w);
}


RotationVel operator* (const RotationVel& r1,const RotationVel& r2) {
    return RotationVel( r1.R*r2.R, r1.w + r1.R*r2.w );
}

RotationVel operator* (const Rotation& r1,const RotationVel& r2) {
    return RotationVel( r1*r2.R, r1*r2.w );
}

RotationVel operator* (const RotationVel& r1,const Rotation& r2) {
    return RotationVel( r1.R*r2, r1.w );
}

RotationVel& RotationVel::operator = (const RotationVel& arg) {
        R=arg.R;
        w=arg.w;
        return *this;
    }
RotationVel& RotationVel::operator = (const Rotation& arg) {
    R=arg;
    w=Vector::Zero();
    return *this;
}

VectorVel   RotationVel::UnitX() const {
	return VectorVel(R.UnitX(),w*R.UnitX()); 
}

VectorVel   RotationVel::UnitY() const {
	return VectorVel(R.UnitY(),w*R.UnitY()); 
}

VectorVel   RotationVel::UnitZ() const {
	return VectorVel(R.UnitZ(),w*R.UnitZ()); 
}



RotationVel RotationVel::Identity() {
    return RotationVel(Rotation::Identity(),Vector::Zero());
}

RotationVel RotationVel::Inverse() const {
    return RotationVel(R.Inverse(),-R.Inverse(w));
}

VectorVel RotationVel::Inverse(const VectorVel& arg) const {
    Vector tmp=R.Inverse(arg.p);
    return VectorVel(tmp,
                    R.Inverse(arg.v-w*arg.p)
                    );
}

VectorVel RotationVel::Inverse(const Vector& arg) const {
    Vector tmp=R.Inverse(arg);
    return VectorVel(tmp,
                    R.Inverse(-w*arg)
                    );
}


VectorVel RotationVel::operator*(const VectorVel& arg) const {
    Vector tmp=R*arg.p;
    return VectorVel(tmp,w*tmp+R*arg.v);
}

VectorVel RotationVel::operator*(const Vector& arg) const {
    Vector tmp=R*arg;
    return VectorVel(tmp,w*tmp);
}


// = Rotations
// The Rot... static functions give the value of the appropriate rotation matrix back.
// The DoRot... functions apply a rotation R to *this,such that *this = *this * R.

void RotationVel::DoRotX(const doubleVel& angle) {
    w+=R*Vector(angle.grad,0,0);
    R.DoRotX(angle.t);
}
RotationVel RotationVel::RotX(const doubleVel& angle) {
    return RotationVel(Rotation::RotX(angle.t),Vector(angle.grad,0,0));
}

void RotationVel::DoRotY(const doubleVel& angle) {
    w+=R*Vector(0,angle.grad,0);
    R.DoRotY(angle.t);
}
RotationVel RotationVel::RotY(const doubleVel& angle) {
    return RotationVel(Rotation::RotX(angle.t),Vector(0,angle.grad,0));
}

void RotationVel::DoRotZ(const doubleVel& angle) {
    w+=R*Vector(0,0,angle.grad);
    R.DoRotZ(angle.t);
}
RotationVel RotationVel::RotZ(const doubleVel& angle) {
    return RotationVel(Rotation::RotZ(angle.t),Vector(0,0,angle.grad));
}


RotationVel RotationVel::Rot(const Vector& rotvec,const doubleVel& angle) 
// rotvec has arbitrary norm
// rotation around a constant vector !
{
    Vector v(rotvec);
	v.Normalize();
    return RotationVel(Rotation::Rot2(v,angle.t),v*angle.grad);
}

RotationVel RotationVel::Rot2(const Vector& rotvec,const doubleVel& angle) 
    // rotvec is normalized.
{
    return RotationVel(Rotation::Rot2(rotvec,angle.t),rotvec*angle.grad);
}


VectorVel operator + (const VectorVel& r1,const VectorVel& r2) {
    return VectorVel(r1.p+r2.p,r1.v+r2.v);
}

VectorVel operator - (const VectorVel& r1,const VectorVel& r2) {
    return VectorVel(r1.p-r2.p,r1.v-r2.v);
}

VectorVel operator + (const VectorVel& r1,const Vector& r2) {
    return VectorVel(r1.p+r2,r1.v);
}

VectorVel operator - (const VectorVel& r1,const Vector& r2) {
    return VectorVel(r1.p-r2,r1.v);
}

VectorVel operator + (const Vector& r1,const VectorVel& r2) {
    return VectorVel(r1+r2.p,r2.v);
}

VectorVel operator - (const Vector& r1,const VectorVel& r2) {
    return VectorVel(r1-r2.p,-r2.v);
}

// unary -
VectorVel operator - (const VectorVel& r) {
    return VectorVel(-r.p,-r.v);
}

void SetToZero(VectorVel& v){
    SetToZero(v.p);
    SetToZero(v.v);
}

// cross prod.
VectorVel operator * (const VectorVel& r1,const VectorVel& r2) {
    return VectorVel(r1.p*r2.p, r1.p*r2.v+r1.v*r2.p);
}

VectorVel operator * (const VectorVel& r1,const Vector& r2) {
    return VectorVel(r1.p*r2, r1.v*r2);
}

VectorVel operator * (const Vector& r1,const VectorVel& r2) {
    return VectorVel(r1*r2.p, r1*r2.v);
}



// scalar mult.
VectorVel operator * (double r1,const VectorVel& r2) {
    return VectorVel(r1*r2.p, r1*r2.v);
}

VectorVel operator * (const VectorVel& r1,double r2) {
    return VectorVel(r1.p*r2, r1.v*r2);
}



VectorVel operator * (const doubleVel& r1,const VectorVel& r2) {
    return VectorVel(r1.t*r2.p, r1.t*r2.v + r1.grad*r2.p);
}

VectorVel operator * (const VectorVel& r2,const doubleVel& r1) {
    return VectorVel(r1.t*r2.p, r1.t*r2.v + r1.grad*r2.p);
}

VectorVel operator / (const VectorVel& r1,double r2) {
    return VectorVel(r1.p/r2, r1.v/r2);
}

VectorVel operator / (const VectorVel& r2,const doubleVel& r1) {
    return VectorVel(r2.p/r1.t, r2.v/r1.t - r2.p*r1.grad/r1.t/r1.t);
}

VectorVel operator*(const Rotation& R,const VectorVel& x) {
    return VectorVel(R*x.p,R*x.v);
}

VectorVel& VectorVel::operator = (const VectorVel& arg) {
    p=arg.p;
    v=arg.v;
    return *this;
}
VectorVel& VectorVel::operator = (const Vector& arg) {
    p=arg;
    v=Vector::Zero();
    return *this;
}
VectorVel& VectorVel::operator += (const VectorVel& arg) {
    p+=arg.p;
    v+=arg.v;
    return *this;
}
VectorVel& VectorVel::operator -= (const VectorVel& arg) {
    p-=arg.p;
    v-=arg.v;
    return *this;
}

VectorVel VectorVel::Zero() {
    return VectorVel(Vector::Zero(),Vector::Zero());
}
void VectorVel::ReverseSign() {
    p.ReverseSign();
    v.ReverseSign();
}
doubleVel VectorVel::Norm() const {
    double n = p.Norm();
    return doubleVel(n,dot(p,v)/n);
}

bool Equal(const VectorVel& r1,const VectorVel& r2,double eps) {
    return (Equal(r1.p,r2.p,eps) && Equal(r1.v,r2.v,eps));
}
bool Equal(const Vector& r1,const VectorVel& r2,double eps) {
    return (Equal(r1,r2.p,eps) && Equal(Vector::Zero(),r2.v,eps));
}
bool Equal(const VectorVel& r1,const Vector& r2,double eps) {
    return (Equal(r1.p,r2,eps) && Equal(r1.v,Vector::Zero(),eps));
}

bool Equal(const RotationVel& r1,const RotationVel& r2,double eps) {
    return (Equal(r1.w,r2.w,eps) && Equal(r1.R,r2.R,eps));
}
bool Equal(const Rotation& r1,const RotationVel& r2,double eps) {
    return (Equal(Vector::Zero(),r2.w,eps) && Equal(r1,r2.R,eps));
}
bool Equal(const RotationVel& r1,const Rotation& r2,double eps) {
    return (Equal(r1.w,Vector::Zero(),eps) && Equal(r1.R,r2,eps));
}
bool Equal(const TwistVel& a,const TwistVel& b,double eps) {
        return (Equal(a.rot,b.rot,eps)&&
                Equal(a.vel,b.vel,eps)  );
}
bool Equal(const Twist& a,const TwistVel& b,double eps) {
        return (Equal(a.rot,b.rot,eps)&&
                Equal(a.vel,b.vel,eps)  );
}
bool Equal(const TwistVel& a,const Twist& b,double eps) {
        return (Equal(a.rot,b.rot,eps)&&
                Equal(a.vel,b.vel,eps)  );
}



IMETHOD doubleVel dot(const VectorVel& lhs,const VectorVel& rhs) {
    return doubleVel(dot(lhs.p,rhs.p),dot(lhs.p,rhs.v)+dot(lhs.v,rhs.p));
}
IMETHOD doubleVel dot(const VectorVel& lhs,const Vector& rhs) {
    return doubleVel(dot(lhs.p,rhs),dot(lhs.v,rhs));
}
IMETHOD doubleVel dot(const Vector& lhs,const VectorVel& rhs) {
    return doubleVel(dot(lhs,rhs.p),dot(lhs,rhs.v));
}












TwistVel TwistVel::Zero()
{
    return TwistVel(VectorVel::Zero(),VectorVel::Zero());
}


void TwistVel::ReverseSign()
{   
    vel.ReverseSign();
    rot.ReverseSign();
}

TwistVel TwistVel::RefPoint(const VectorVel& v_base_AB)
     // Changes the reference point of the TwistVel.
     // The VectorVel v_base_AB is expressed in the same base as the TwistVel
     // The VectorVel v_base_AB is a VectorVel from the old point to
     // the new point.
     // Complexity : 6M+6A
{
    return TwistVel(this->vel+this->rot*v_base_AB,this->rot);
}

TwistVel& TwistVel::operator-=(const TwistVel& arg)
{
    vel-=arg.vel;
    rot -=arg.rot;
    return *this;
}

TwistVel& TwistVel::operator+=(const TwistVel& arg)
{
    vel+=arg.vel;
    rot +=arg.rot;
    return *this;
}


TwistVel operator*(const TwistVel& lhs,double rhs)
{
    return TwistVel(lhs.vel*rhs,lhs.rot*rhs);
}

TwistVel operator*(double lhs,const TwistVel& rhs)
{
    return TwistVel(lhs*rhs.vel,lhs*rhs.rot);
}

TwistVel operator/(const TwistVel& lhs,double rhs)
{
    return TwistVel(lhs.vel/rhs,lhs.rot/rhs);
}


TwistVel operator*(const TwistVel& lhs,const doubleVel& rhs)
{
    return TwistVel(lhs.vel*rhs,lhs.rot*rhs);
}

TwistVel operator*(const doubleVel& lhs,const TwistVel& rhs)
{
    return TwistVel(lhs*rhs.vel,lhs*rhs.rot);
}

TwistVel operator/(const TwistVel& lhs,const doubleVel& rhs)
{
    return TwistVel(lhs.vel/rhs,lhs.rot/rhs);
}



// addition of TwistVel's
TwistVel operator+(const TwistVel& lhs,const TwistVel& rhs)
{
    return TwistVel(lhs.vel+rhs.vel,lhs.rot+rhs.rot);
}

TwistVel operator-(const TwistVel& lhs,const TwistVel& rhs)
{
    return TwistVel(lhs.vel-rhs.vel,lhs.rot-rhs.rot);
}

// unary -
TwistVel operator-(const TwistVel& arg) 
{
    return TwistVel(-arg.vel,-arg.rot);
}

void SetToZero(TwistVel& v)
{
   SetToZero(v.vel);
   SetToZero(v.rot);
}





TwistVel RotationVel::Inverse(const TwistVel& arg) const
{
    return TwistVel(Inverse(arg.vel),Inverse(arg.rot));
}

TwistVel RotationVel::operator * (const TwistVel& arg) const
{
    return TwistVel((*this)*arg.vel,(*this)*arg.rot);
}

TwistVel RotationVel::Inverse(const Twist& arg) const
{
    return TwistVel(Inverse(arg.vel),Inverse(arg.rot));
}

TwistVel RotationVel::operator * (const Twist& arg) const
{
    return TwistVel((*this)*arg.vel,(*this)*arg.rot);
}


TwistVel FrameVel::operator * (const TwistVel& arg) const
{
    TwistVel tmp;
    tmp.rot = M*arg.rot;
    tmp.vel = M*arg.vel+p*tmp.rot;
    return tmp;
}

TwistVel FrameVel::operator * (const Twist& arg) const
{
    TwistVel tmp;
    tmp.rot = M*arg.rot;
    tmp.vel = M*arg.vel+p*tmp.rot;
    return tmp;
}

TwistVel FrameVel::Inverse(const TwistVel& arg) const
{
    TwistVel tmp;
    tmp.rot =  M.Inverse(arg.rot);
    tmp.vel = M.Inverse(arg.vel-p*arg.rot);
    return tmp;
}

TwistVel FrameVel::Inverse(const Twist& arg) const
{
    TwistVel tmp;
    tmp.rot =  M.Inverse(arg.rot);
    tmp.vel = M.Inverse(arg.vel-p*arg.rot);
    return tmp;
}

Twist TwistVel::GetTwist() const {
    return Twist(vel.p,rot.p);
}

Twist TwistVel::GetTwistDot() const {
    return Twist(vel.v,rot.v);
}
