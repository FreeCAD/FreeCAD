/*****************************************************************************
 * \file  
 *      provides inline functions of rrframes.h
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
 *      $Id: rrframes.inl,v 1.1.1.1 2002/08/26 14:14:21 rmoreas Exp $
 *      $Name:  $ 
 ****************************************************************************/


namespace KDL {

/////////////////// VectorAcc /////////////////////////////////////

VectorAcc operator + (const VectorAcc& r1,const VectorAcc& r2) {
    return VectorAcc(r1.p+r2.p,r1.v+r2.v,r1.dv+r2.dv);
}

VectorAcc operator - (const VectorAcc& r1,const VectorAcc& r2) {
    return VectorAcc(r1.p-r2.p, r1.v-r2.v, r1.dv-r2.dv);
}
VectorAcc operator + (const Vector& r1,const VectorAcc& r2) {
    return VectorAcc(r1+r2.p,r2.v,r2.dv);
}

VectorAcc operator - (const Vector& r1,const VectorAcc& r2) {
    return VectorAcc(r1-r2.p, -r2.v, -r2.dv);
}
VectorAcc operator + (const VectorAcc& r1,const Vector& r2) {
    return VectorAcc(r1.p+r2,r1.v,r1.dv);
}

VectorAcc operator - (const VectorAcc& r1,const Vector& r2) {
    return VectorAcc(r1.p-r2, r1.v, r1.dv);
}

// unary -
VectorAcc operator - (const VectorAcc& r) {
    return VectorAcc(-r.p,-r.v,-r.dv);
}

// cross prod.
VectorAcc operator * (const VectorAcc& r1,const VectorAcc& r2) {
    return VectorAcc(r1.p*r2.p, 
                    r1.p*r2.v+r1.v*r2.p,
                    r1.dv*r2.p+2*r1.v*r2.v+r1.p*r2.dv
                    );
}

VectorAcc operator * (const VectorAcc& r1,const Vector& r2) {
    return VectorAcc(r1.p*r2, r1.v*r2, r1.dv*r2 );
}

VectorAcc operator * (const Vector& r1,const VectorAcc& r2) {
    return VectorAcc(r1*r2.p, r1*r2.v, r1*r2.dv );
}



// scalar mult.
VectorAcc operator * (double r1,const VectorAcc& r2) {
    return VectorAcc(r1*r2.p, r1*r2.v, r1*r2.dv );
}

VectorAcc operator * (const VectorAcc& r1,double r2) {
    return VectorAcc(r1.p*r2, r1.v*r2, r1.dv*r2 );
}

VectorAcc operator * (const doubleAcc& r1,const VectorAcc& r2) {
    return VectorAcc(r1.t*r2.p, 
                   r1.t*r2.v  + r1.d*r2.p,
                   r1.t*r2.dv + 2*r1.d*r2.v + r1.dd*r2.p
                   );
}

VectorAcc operator * (const VectorAcc& r2,const doubleAcc& r1) {
    return VectorAcc(r1.t*r2.p, 
               r1.t*r2.v  + r1.d*r2.p,
               r1.t*r2.dv + 2*r1.d*r2.v + r1.dd*r2.p
               );
}

VectorAcc& VectorAcc::operator = (const VectorAcc& arg) {
    p=arg.p;
    v=arg.v;
    dv=arg.dv;
    return *this;
}

VectorAcc& VectorAcc::operator = (const Vector& arg) {
    p=arg;
    v=Vector::Zero();
    dv=Vector::Zero();
    return *this;
}

VectorAcc& VectorAcc::operator += (const VectorAcc& arg) {
    p+=arg.p;
    v+=arg.v;
    dv+= arg.dv;
    return *this;
}
VectorAcc& VectorAcc::operator -= (const VectorAcc& arg) {
    p-=arg.p;
    v-=arg.v;
    dv-=arg.dv;
    return *this;
}

VectorAcc VectorAcc::Zero() {
    return VectorAcc(Vector::Zero(),Vector::Zero(),Vector::Zero());
}

void VectorAcc::ReverseSign() {
    p.ReverseSign();
    v.ReverseSign();
    dv.ReverseSign();
}

doubleAcc VectorAcc::Norm() {
    doubleAcc res;
    res.t  = p.Norm();
    res.d  = dot(p,v)/res.t;
    res.dd = (dot(p,dv)+dot(v,v)-res.d*res.d)/res.t;
    return res;
}

doubleAcc dot(const VectorAcc& lhs,const VectorAcc& rhs) {
    return doubleAcc( dot(lhs.p,rhs.p),
                    dot(lhs.p,rhs.v)+dot(lhs.v,rhs.p),
                    dot(lhs.p,rhs.dv)+2*dot(lhs.v,rhs.v)+dot(lhs.dv,rhs.p)
                    );
}

doubleAcc dot(const VectorAcc& lhs,const Vector& rhs) {
    return doubleAcc( dot(lhs.p,rhs),
                     dot(lhs.v,rhs),
                     dot(lhs.dv,rhs)
                    );
}

doubleAcc dot(const Vector& lhs,const VectorAcc& rhs) {
    return doubleAcc( dot(lhs,rhs.p),
                    dot(lhs,rhs.v),
                    dot(lhs,rhs.dv)
                    );
}


bool Equal(const VectorAcc& r1,const VectorAcc& r2,double eps) {
    return (Equal(r1.p,r2.p,eps) 
         && Equal(r1.v,r2.v,eps)
         && Equal(r1.dv,r2.dv,eps)
         );
}

bool Equal(const Vector& r1,const VectorAcc& r2,double eps) {
    return (Equal(r1,r2.p,eps) 
         && Equal(Vector::Zero(),r2.v,eps)
         && Equal(Vector::Zero(),r2.dv,eps)
         );
}

bool Equal(const VectorAcc& r1,const Vector& r2,double eps) {
    return (Equal(r1.p,r2,eps) 
         && Equal(r1.v,Vector::Zero(),eps)
         && Equal(r1.dv,Vector::Zero(),eps)
         );
}

VectorAcc operator / (const VectorAcc& r1,double r2) {
    return r1*(1.0/r2);
}

VectorAcc operator / (const VectorAcc& r2,const doubleAcc& r1) {
    return r2*(1.0/r1);
}



/////////////////// RotationAcc /////////////////////////////////////

RotationAcc operator* (const RotationAcc& r1,const RotationAcc& r2) {
    return RotationAcc( r1.R  * r2.R, 
                      r1.w  + r1.R*r2.w,  
                      r1.dw + r1.w*(r1.R*r2.w) + r1.R*r2.dw 
                      );
}

RotationAcc operator* (const Rotation& r1,const RotationAcc& r2) {
    return RotationAcc( r1*r2.R, r1*r2.w, r1*r2.dw);
}

RotationAcc operator* (const RotationAcc& r1,const Rotation& r2) {
    return RotationAcc( r1.R*r2, r1.w, r1.dw );
}

RotationAcc& RotationAcc::operator = (const RotationAcc& arg) {
    R=arg.R;
    w=arg.w;
    dw=arg.dw;
    return *this;
}
RotationAcc& RotationAcc::operator = (const Rotation& arg) {
    R = arg;
    w = Vector::Zero();
    dw = Vector::Zero();
    return *this;
}

RotationAcc RotationAcc::Identity() {
    return RotationAcc(Rotation::Identity(),Vector::Zero(),Vector::Zero());
}

RotationAcc RotationAcc::Inverse() const {
    return RotationAcc(R.Inverse(),-R.Inverse(w),-R.Inverse(dw));
}

VectorAcc RotationAcc::Inverse(const VectorAcc& arg) const {
    VectorAcc tmp;
    tmp.p  = R.Inverse(arg.p);
    tmp.v  = R.Inverse(arg.v - w * arg.p);
    tmp.dv = R.Inverse(arg.dv - dw*arg.p - w*(arg.v+R*tmp.v));
    return tmp;
}

VectorAcc RotationAcc::Inverse(const Vector& arg) const {
    VectorAcc tmp;
    tmp.p  = R.Inverse(arg);
    tmp.v  = R.Inverse(-w*arg);
    tmp.dv = R.Inverse(-dw*arg - w*(R*tmp.v));
    return tmp;
}


VectorAcc RotationAcc::operator*(const VectorAcc& arg) const {
    VectorAcc tmp;
    tmp.p = R*arg.p;
    tmp.dv = R*arg.v;
    tmp.v = w*tmp.p + tmp.dv;
    tmp.dv = dw*tmp.p + w*(tmp.v + tmp.dv) + R*arg.dv;
    return tmp;
}

VectorAcc operator*(const Rotation& R,const VectorAcc& x) {
    return VectorAcc(R*x.p,R*x.v,R*x.dv);
}

VectorAcc RotationAcc::operator*(const Vector& arg) const {
    VectorAcc tmp;
    tmp.p = R*arg;
    tmp.v = w*tmp.p;
    tmp.dv = dw*tmp.p + w*tmp.v;
    return tmp;
}

/*
        // = Rotations
        // The Rot... static functions give the value of the appropriate rotation matrix back.
        // The DoRot... functions apply a rotation R to *this,such that *this = *this * R.

        void RRotation::DoRotX(const RDouble& angle) {
            w+=R*Vector(angle.grad,0,0);
            R.DoRotX(angle.t);
        }
RotationAcc RotationAcc::RotX(const doubleAcc& angle) {
    return RotationAcc(Rotation::RotX(angle.t),
                      Vector(angle.d,0,0),
                      Vector(angle.dd,0,0)
                      );
}

        void RRotation::DoRotY(const RDouble& angle) {
            w+=R*Vector(0,angle.grad,0);
            R.DoRotY(angle.t);
        }
RotationAcc RotationAcc::RotY(const doubleAcc& angle) {
    return RotationAcc(
              Rotation::RotX(angle.t),
              Vector(0,angle.d,0),
              Vector(0,angle.dd,0)
            );
}

        void RRotation::DoRotZ(const RDouble& angle) {
            w+=R*Vector(0,0,angle.grad);
            R.DoRotZ(angle.t);
        }
RotationAcc RotationAcc::RotZ(const doubleAcc& angle) {
    return RotationAcc(
                Rotation::RotZ(angle.t),
                Vector(0,0,angle.d),
                Vector(0,0,angle.dd)
            );
}


        RRotation RRotation::Rot(const Vector& rotvec,const RDouble& angle) 
        // rotvec has arbitrary norm
        // rotation around a constant vector !
        {
            Vector v = rotvec.Normalize();
            return RRotation(Rotation::Rot2(v,angle.t),v*angle.grad);
        }

        RRotation RRotation::Rot2(const Vector& rotvec,const RDouble& angle) 
            // rotvec is normalized.
        {
            return RRotation(Rotation::Rot2(rotvec,angle.t),rotvec*angle.grad);
        }

*/

bool Equal(const RotationAcc& r1,const RotationAcc& r2,double eps) {
    return (Equal(r1.w,r2.w,eps) && Equal(r1.R,r2.R,eps) && Equal(r1.dw,r2.dw,eps) );
}
bool Equal(const Rotation& r1,const RotationAcc& r2,double eps) {
    return (Equal(Vector::Zero(),r2.w,eps) && Equal(r1,r2.R,eps) && 
            Equal(Vector::Zero(),r2.dw,eps) );
}
bool Equal(const RotationAcc& r1,const Rotation& r2,double eps) {
    return (Equal(r1.w,Vector::Zero(),eps) && Equal(r1.R,r2,eps) && 
            Equal(r1.dw,Vector::Zero(),eps) );
}


// Methods and operators related to FrameAcc
// They all delegate most of the work to RotationAcc and VectorAcc
FrameAcc& FrameAcc::operator = (const FrameAcc& arg) {
    M=arg.M;
    p=arg.p;
    return *this;
}

FrameAcc FrameAcc::Identity() {
    return FrameAcc(RotationAcc::Identity(),VectorAcc::Zero());
}


FrameAcc operator *(const FrameAcc& lhs,const FrameAcc& rhs)
{
    return FrameAcc(lhs.M*rhs.M,lhs.M*rhs.p+lhs.p);
}
FrameAcc operator *(const FrameAcc& lhs,const Frame& rhs)
{
    return FrameAcc(lhs.M*rhs.M,lhs.M*rhs.p+lhs.p);
}
FrameAcc operator *(const Frame& lhs,const FrameAcc& rhs)
{
    return FrameAcc(lhs.M*rhs.M,lhs.M*rhs.p+lhs.p);
}

VectorAcc FrameAcc::operator *(const VectorAcc & arg) const
{
    return M*arg+p;
}
VectorAcc FrameAcc::operator *(const Vector & arg) const
{
    return M*arg+p;
}

VectorAcc FrameAcc::Inverse(const VectorAcc& arg) const
{
    return M.Inverse(arg-p);
}

VectorAcc FrameAcc::Inverse(const Vector& arg) const
{
    return M.Inverse(arg-p);
}

FrameAcc FrameAcc::Inverse() const
{
    return FrameAcc(M.Inverse(),-M.Inverse(p));
}

FrameAcc& FrameAcc::operator =(const Frame & arg)
{ 
    M = arg.M;
    p = arg.p;
    return *this;
}

bool Equal(const FrameAcc& r1,const FrameAcc& r2,double eps) {
    return (Equal(r1.M,r2.M,eps) && Equal(r1.p,r2.p,eps));
}
bool Equal(const Frame& r1,const FrameAcc& r2,double eps) {
    return (Equal(r1.M,r2.M,eps) && Equal(r1.p,r2.p,eps));
}
bool Equal(const FrameAcc& r1,const Frame& r2,double eps) {
    return (Equal(r1.M,r2.M,eps) && Equal(r1.p,r2.p,eps));
}


Frame FrameAcc::GetFrame() const {
    return Frame(M.R,p.p);
}


Twist FrameAcc::GetTwist() const {
    return Twist(p.v,M.w);
}


Twist FrameAcc::GetAccTwist() const {
    return Twist(p.dv,M.dw);
}

















TwistAcc TwistAcc::Zero()
{
    return TwistAcc(VectorAcc::Zero(),VectorAcc::Zero());
}


void TwistAcc::ReverseSign()
{   
    vel.ReverseSign();
    rot.ReverseSign();
}

TwistAcc TwistAcc::RefPoint(const VectorAcc& v_base_AB)
     // Changes the reference point of the TwistAcc.
     // The RVector v_base_AB is expressed in the same base as the TwistAcc
     // The RVector v_base_AB is a RVector from the old point to
     // the new point.
     // Complexity : 6M+6A
{
    return TwistAcc(this->vel+this->rot*v_base_AB,this->rot);
}

TwistAcc& TwistAcc::operator-=(const TwistAcc& arg)
{
    vel-=arg.vel;
    rot -=arg.rot;
    return *this;
}

TwistAcc& TwistAcc::operator+=(const TwistAcc& arg)
{
    vel+=arg.vel;
    rot +=arg.rot;
    return *this;
}


TwistAcc operator*(const TwistAcc& lhs,double rhs)
{
    return TwistAcc(lhs.vel*rhs,lhs.rot*rhs);
}

TwistAcc operator*(double lhs,const TwistAcc& rhs)
{
    return TwistAcc(lhs*rhs.vel,lhs*rhs.rot);
}

TwistAcc operator/(const TwistAcc& lhs,double rhs)
{
    return TwistAcc(lhs.vel/rhs,lhs.rot/rhs);
}


TwistAcc operator*(const TwistAcc& lhs,const doubleAcc& rhs)
{
    return TwistAcc(lhs.vel*rhs,lhs.rot*rhs);
}

TwistAcc operator*(const doubleAcc& lhs,const TwistAcc& rhs)
{
    return TwistAcc(lhs*rhs.vel,lhs*rhs.rot);
}

TwistAcc operator/(const TwistAcc& lhs,const doubleAcc& rhs)
{
    return TwistAcc(lhs.vel/rhs,lhs.rot/rhs);
}



// addition of TwistAcc's
TwistAcc operator+(const TwistAcc& lhs,const TwistAcc& rhs)
{
    return TwistAcc(lhs.vel+rhs.vel,lhs.rot+rhs.rot);
}

TwistAcc operator-(const TwistAcc& lhs,const TwistAcc& rhs)
{
    return TwistAcc(lhs.vel-rhs.vel,lhs.rot-rhs.rot);
}

// unary -
TwistAcc operator-(const TwistAcc& arg) 
{
    return TwistAcc(-arg.vel,-arg.rot);
}





TwistAcc RotationAcc::Inverse(const TwistAcc& arg) const
{
    return TwistAcc(Inverse(arg.vel),Inverse(arg.rot));
}

TwistAcc RotationAcc::operator * (const TwistAcc& arg) const
{
    return TwistAcc((*this)*arg.vel,(*this)*arg.rot);
}

TwistAcc RotationAcc::Inverse(const Twist& arg) const
{
    return TwistAcc(Inverse(arg.vel),Inverse(arg.rot));
}

TwistAcc RotationAcc::operator * (const Twist& arg) const
{
    return TwistAcc((*this)*arg.vel,(*this)*arg.rot);
}


TwistAcc FrameAcc::operator * (const TwistAcc& arg) const
{
    TwistAcc tmp;
    tmp.rot = M*arg.rot;
    tmp.vel = M*arg.vel+p*tmp.rot;
    return tmp;
}

TwistAcc FrameAcc::operator * (const Twist& arg) const
{
    TwistAcc tmp;
    tmp.rot = M*arg.rot;
    tmp.vel = M*arg.vel+p*tmp.rot;
    return tmp;
}

TwistAcc FrameAcc::Inverse(const TwistAcc& arg) const
{
    TwistAcc tmp;
    tmp.rot =  M.Inverse(arg.rot);
    tmp.vel = M.Inverse(arg.vel-p*arg.rot);
    return tmp;
}

TwistAcc FrameAcc::Inverse(const Twist& arg) const
{
    TwistAcc tmp;
    tmp.rot =  M.Inverse(arg.rot);
    tmp.vel = M.Inverse(arg.vel-p*arg.rot);
    return tmp;
}

Twist TwistAcc::GetTwist() const {
    return Twist(vel.p,rot.p);
}

Twist TwistAcc::GetTwistDot() const {
    return Twist(vel.v,rot.v);
}

bool Equal(const TwistAcc& a,const TwistAcc& b,double eps) {
        return (Equal(a.rot,b.rot,eps)&&
                Equal(a.vel,b.vel,eps)  );
}
bool Equal(const Twist& a,const TwistAcc& b,double eps) {
        return (Equal(a.rot,b.rot,eps)&&
                Equal(a.vel,b.vel,eps)  );
}
bool Equal(const TwistAcc& a,const Twist& b,double eps) {
        return (Equal(a.rot,b.rot,eps)&&
                Equal(a.vel,b.vel,eps)  );
}

}
