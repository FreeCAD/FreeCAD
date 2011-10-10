/***************************************************************************
                        frames.cxx -  description
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

#include "frames.hpp"

namespace KDL {

#ifndef KDL_INLINE
#include "frames.inl"
#endif

    void Frame::Make4x4(double * d)
    {
        int i;
        int j;
        for (i=0;i<3;i++) {
            for (j=0;j<3;j++)
                d[i*4+j]=M(i,j);
            d[i*4+3] = p(i)/1000;
        }
        for (j=0;j<3;j++)
            d[12+j] = 0.;
        d[15] = 1;
    }

    Frame Frame::DH_Craig1989(double a,double alpha,double d,double theta)
    // returns Modified Denavit-Hartenberg parameters (According to Craig)
    {
        double ct,st,ca,sa;
        ct = cos(theta);
        st = sin(theta);
        sa = sin(alpha);
        ca = cos(alpha);
        return Frame(Rotation(
                              ct,       -st,     0,
                              st*ca,  ct*ca,   -sa,
                              st*sa,  ct*sa,    ca   ),
                     Vector(
                            a,      -sa*d,  ca*d   )
                     );
    }

    Frame Frame::DH(double a,double alpha,double d,double theta)
    // returns Denavit-Hartenberg parameters (Non-Modified DH)
    {
        double ct,st,ca,sa;
        ct = cos(theta);
        st = sin(theta);
        sa = sin(alpha);
        ca = cos(alpha);
        return Frame(Rotation(
                              ct,    -st*ca,   st*sa,
                              st,     ct*ca,  -ct*sa,
                              0,        sa,      ca   ),
                     Vector(
                            a*ct,      a*st,  d   )
                     );
    }

    double Vector2::Norm() const
    {
        if (fabs(data[0]) > fabs(data[1]) ) {
            return data[0]*sqrt(1+sqr(data[1]/data[0]));
        } else {
            return data[1]*sqrt(1+sqr(data[0]/data[1]));
        }
    }
    // makes v a unitvector and returns the norm of v.
    // if v is smaller than eps, Vector(1,0,0) is returned with norm 0.
    // if this is not good, check the return value of this method.
    double Vector2::Normalize(double eps) {
        double v = this->Norm();
        if (v < eps) {
            *this = Vector2(1,0);
            return v;
        } else {
            *this = (*this)/v;
            return v;
        }
    }


    // do some effort not to lose precision
    double Vector::Norm() const
    {
        double tmp1;
        double tmp2;
        tmp1 = fabs(data[0]);
        tmp2 = fabs(data[1]);
        if (tmp1 >= tmp2) {
            tmp2=fabs(data[2]);
            if (tmp1 >= tmp2) {
                if (tmp1 == 0) {
                    // only to everything exactly zero case, all other are handled correctly
                    return 0;
                }
                return tmp1*sqrt(1+sqr(data[1]/data[0])+sqr(data[2]/data[0]));
            } else {
                return tmp2*sqrt(1+sqr(data[0]/data[2])+sqr(data[1]/data[2]));
            }
        } else {
            tmp1=fabs(data[2]);
            if (tmp2 > tmp1) {
                return tmp2*sqrt(1+sqr(data[0]/data[1])+sqr(data[2]/data[1]));
            } else {
                return tmp1*sqrt(1+sqr(data[0]/data[2])+sqr(data[1]/data[2]));
            }
        }
    }

    // makes v a unitvector and returns the norm of v.
    // if v is smaller than eps, Vector(1,0,0) is returned with norm 0.
    // if this is not good, check the return value of this method.
    double Vector::Normalize(double eps) {
        double v = this->Norm();
        if (v < eps) {
            *this = Vector(1,0,0);
            return v;
        } else {
            *this = (*this)/v;
            return v;
        }
    }


    bool Equal(const Rotation& a,const Rotation& b,double eps) {
        return (Equal(a.data[0],b.data[0],eps) &&
                Equal(a.data[1],b.data[1],eps) &&
                Equal(a.data[2],b.data[2],eps) &&
                Equal(a.data[3],b.data[3],eps) &&
                Equal(a.data[4],b.data[4],eps) &&
                Equal(a.data[5],b.data[5],eps) &&
                Equal(a.data[6],b.data[6],eps) &&
                Equal(a.data[7],b.data[7],eps) &&
                Equal(a.data[8],b.data[8],eps)    );
    }



    Rotation operator *(const Rotation& lhs,const Rotation& rhs)
    // Complexity : 27M+27A
    {
        return Rotation(
                        lhs.data[0]*rhs.data[0]+lhs.data[1]*rhs.data[3]+lhs.data[2]*rhs.data[6],
                        lhs.data[0]*rhs.data[1]+lhs.data[1]*rhs.data[4]+lhs.data[2]*rhs.data[7],
                        lhs.data[0]*rhs.data[2]+lhs.data[1]*rhs.data[5]+lhs.data[2]*rhs.data[8],
                        lhs.data[3]*rhs.data[0]+lhs.data[4]*rhs.data[3]+lhs.data[5]*rhs.data[6],
                        lhs.data[3]*rhs.data[1]+lhs.data[4]*rhs.data[4]+lhs.data[5]*rhs.data[7],
                        lhs.data[3]*rhs.data[2]+lhs.data[4]*rhs.data[5]+lhs.data[5]*rhs.data[8],
                        lhs.data[6]*rhs.data[0]+lhs.data[7]*rhs.data[3]+lhs.data[8]*rhs.data[6],
                        lhs.data[6]*rhs.data[1]+lhs.data[7]*rhs.data[4]+lhs.data[8]*rhs.data[7],
                        lhs.data[6]*rhs.data[2]+lhs.data[7]*rhs.data[5]+lhs.data[8]*rhs.data[8]
                        );

    }

    Rotation Rotation::Quaternion(double x,double y,double z, double w)
    {
        double x2, y2, z2, w2;
        x2 = x*x;  y2 = y*y; z2 = z*z;  w2 = w*w;
        return Rotation(w2+x2-y2-z2, 2*x*y-2*w*z, 2*x*z+2*w*y,
                        2*x*y+2*w*z, w2-x2+y2-z2, 2*y*z-2*w*x,
                        2*x*z-2*w*y, 2*y*z+2*w*x, w2-x2-y2+z2);
    }

    /* From the following sources:
       http://web.archive.org/web/20041029003853/http:/www.j3d.org/matrix_faq/matrfaq_latest.html
       http://www.euclideanspace.com/maths/geometry/rotations/conversions/matrixToQuaternion/index.htm
       RobOOP::quaternion.cpp
    */
    void Rotation::GetQuaternion(double& x,double& y,double& z, double& w) const
    {
        double trace = (*this)(0,0) + (*this)(1,1) + (*this)(2,2) + 1.0f;
        if( trace > epsilon ){
            double s = 0.5f / sqrt(trace);
            w = 0.25f / s;
            x = ( (*this)(2,1) - (*this)(1,2) ) * s;
            y = ( (*this)(0,2) - (*this)(2,0) ) * s;
            z = ( (*this)(1,0) - (*this)(0,1) ) * s;
        }else{
            if ( (*this)(0,0) > (*this)(1,1) && (*this)(0,0) > (*this)(2,2) ){
                float s = 2.0f * sqrtf( 1.0f + (*this)(0,0) - (*this)(1,1) - (*this)(2,2));
                w = ((*this)(2,1) - (*this)(1,2) ) / s;
                x = 0.25f * s;
                y = ((*this)(0,1) + (*this)(1,0) ) / s;
                z = ((*this)(0,2) + (*this)(2,0) ) / s;
            } else if ((*this)(1,1) > (*this)(2,2)) {
                float s = 2.0f * sqrtf( 1.0f + (*this)(1,1) - (*this)(0,0) - (*this)(2,2));
                w = ((*this)(0,2) - (*this)(2,0) ) / s;
                x = ((*this)(0,1) + (*this)(1,0) ) / s;
                y = 0.25f * s;
                z = ((*this)(1,2) + (*this)(2,1) ) / s;
            }else {
                float s = 2.0f * sqrtf( 1.0f + (*this)(2,2) - (*this)(0,0) - (*this)(1,1) );
                w = ((*this)(1,0) - (*this)(0,1) ) / s;
                x = ((*this)(0,2) + (*this)(2,0) ) / s;
                y = ((*this)(1,2) + (*this)(2,1) ) / s;
                z = 0.25f * s;
            }
        }    
    }

Rotation Rotation::RPY(double roll,double pitch,double yaw)
    {
        double ca1,cb1,cc1,sa1,sb1,sc1;
        ca1 = cos(yaw); sa1 = sin(yaw);
        cb1 = cos(pitch);sb1 = sin(pitch);
        cc1 = cos(roll);sc1 = sin(roll);
        return Rotation(ca1*cb1,ca1*sb1*sc1 - sa1*cc1,ca1*sb1*cc1 + sa1*sc1,
                   sa1*cb1,sa1*sb1*sc1 + ca1*cc1,sa1*sb1*cc1 - ca1*sc1,
                   -sb1,cb1*sc1,cb1*cc1);
    }

// Gives back a rotation matrix specified with RPY convention
void Rotation::GetRPY(double& roll,double& pitch,double& yaw) const
    {
        if (fabs(data[6]) > 1.0 - epsilon ) {
            roll = -sign(data[6]) * atan2(data[1], data[4]);
            pitch= -sign(data[6]) * PI / 2;
            yaw  = 0.0 ;
        } else {
            roll  = atan2(data[7], data[8]);
            pitch = atan2(-data[6], sqrt( sqr(data[0]) +sqr(data[3]) )  );
            yaw   = atan2(data[3], data[0]);
        }
    }

Rotation Rotation::EulerZYZ(double Alfa,double Beta,double Gamma) {
        double sa,ca,sb,cb,sg,cg;
        sa  = sin(Alfa);ca = cos(Alfa);
        sb  = sin(Beta);cb = cos(Beta);
        sg  = sin(Gamma);cg = cos(Gamma);
        return Rotation( ca*cb*cg-sa*sg,     -ca*cb*sg-sa*cg,        ca*sb,
                 sa*cb*cg+ca*sg,     -sa*cb*sg+ca*cg,        sa*sb,
                 -sb*cg ,                sb*sg,              cb
                );

     }


void Rotation::GetEulerZYZ(double& alfa,double& beta,double& gamma) const {
        if (fabs(data[6]) < epsilon ) {
            alfa=0.0;
            if (data[8]>0) {
                beta = 0.0;
                gamma= atan2(-data[1],data[0]);
            } else {
                beta = PI;
                gamma= atan2(data[1],-data[0]);
            }
        } else {
            alfa=atan2(data[5], data[2]);
            beta=atan2(sqrt( sqr(data[6]) +sqr(data[7]) ),data[8]);
            gamma=atan2(data[7], -data[6]);
        }
 }

Rotation Rotation::Rot(const Vector& rotaxis,double angle) {
    // The formula is
    // V.(V.tr) + st*[V x] + ct*(I-V.(V.tr))
    // can be found by multiplying it with an arbitrary vector p
    // and noting that this vector is rotated.
    Vector rotvec = rotaxis;
	rotvec.Normalize();
	return Rotation::Rot2(rotvec,angle);
}

Rotation Rotation::Rot2(const Vector& rotvec,double angle) {
    // rotvec should be normalized !
    // The formula is
    // V.(V.tr) + st*[V x] + ct*(I-V.(V.tr))
    // can be found by multiplying it with an arbitrary vector p
    // and noting that this vector is rotated.
    double ct = cos(angle);
    double st = sin(angle);
    double vt = 1-ct;
    double m_vt_0=vt*rotvec(0);
    double m_vt_1=vt*rotvec(1);
    double m_vt_2=vt*rotvec(2);
    double m_st_0=rotvec(0)*st;
    double m_st_1=rotvec(1)*st;
    double m_st_2=rotvec(2)*st;
    double m_vt_0_1=m_vt_0*rotvec(1);
    double m_vt_0_2=m_vt_0*rotvec(2);
    double m_vt_1_2=m_vt_1*rotvec(2);
    return Rotation(
        ct      +  m_vt_0*rotvec(0),
        -m_st_2 +  m_vt_0_1,
        m_st_1  +  m_vt_0_2,
        m_st_2  +  m_vt_0_1,
        ct      +  m_vt_1*rotvec(1),
        -m_st_0 +  m_vt_1_2,
        -m_st_1 +  m_vt_0_2,
        m_st_0  +  m_vt_1_2,
        ct      +  m_vt_2*rotvec(2)
        );
}



Vector Rotation::GetRot() const
         // Returns a vector with the direction of the equiv. axis
         // and its norm is angle
     {
       Vector axis  = Vector((data[7]-data[5]),
			     (data[2]-data[6]),
			     (data[3]-data[1]) )/2;

       double sa    = axis.Norm();
       double ca    = (data[0]+data[4]+data[8]-1)/2.0;
       double alfa;
       if (sa > epsilon)
           alfa = ::atan2(sa,ca)/sa;
       else
           alfa = 1;
       return axis * alfa;
     }



/** Returns the rotation angle around the equiv. axis
 * @param axis the rotation axis is returned in this variable
 * @param eps :  in the case of angle == 0 : rot axis is undefined and choosen
 *                                         to be +/- Z-axis
 *               in the case of angle == PI : 2 solutions, positive Z-component
 *                                            of the axis is choosen.
 * @result returns the rotation angle (between [0..PI] )
 * /todo :
 *   Check corresponding routines in rframes and rrframes
 */
double Rotation::GetRotAngle(Vector& axis,double eps) const {
	double ca    = (data[0]+data[4]+data[8]-1)/2.0;
	if (ca>1-eps) {
		// undefined choose the Z-axis, and angle 0
		axis = Vector(0,0,1);
		return 0;
	}
	if (ca < -1+eps) {
		// two solutions, choose a positive Z-component of the axis
		double z = sqrt( (data[8]+1)/2 );
		double x = (data[2])/2/z;
		double y = (data[5])/2/z;
		axis = Vector( x,y,z  );
		return PI;
	}
	double angle = acos(ca);
	double sa    = sin(angle);
	axis  = Vector((data[7]-data[5])/2/sa,
                       (data[2]-data[6])/2/sa,
                       (data[3]-data[1])/2/sa  );
	return angle;
}

bool operator==(const Rotation& a,const Rotation& b) {
#ifdef KDL_USE_EQUAL
    return Equal(a,b);
#else
    return ( a.data[0]==b.data[0] &&
             a.data[1]==b.data[1] &&
             a.data[2]==b.data[2] &&
             a.data[3]==b.data[3] &&
             a.data[4]==b.data[4] &&
             a.data[5]==b.data[5] &&
             a.data[6]==b.data[6] &&
             a.data[7]==b.data[7] &&
             a.data[8]==b.data[8]  );
#endif
}
}
