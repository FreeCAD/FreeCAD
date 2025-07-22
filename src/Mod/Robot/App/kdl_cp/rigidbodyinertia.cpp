// Copyright  (C)  2007  Ruben Smits <ruben dot smits at mech dot kuleuven dot be>

// Version: 1.0
// Author: Ruben Smits <ruben dot smits at mech dot kuleuven dot be>
// Maintainer: Ruben Smits <ruben dot smits at mech dot kuleuven dot be>
// URL: http://www.orocos.org/kdl

// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 2.1 of the License, or (at your option) any later version.

// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Lesser General Public License for more details.

// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

#include "rigidbodyinertia.hpp"

#include <Eigen/Core>

using namespace Eigen;

namespace KDL{
    
    const static bool mhi=true;

    RigidBodyInertia::RigidBodyInertia(double m_,const Vector& h_,const RotationalInertia& I_,bool /*mhi*/):
        m(m_),h(h_),I(I_)
    {
    }
    
    RigidBodyInertia::RigidBodyInertia(double m_, const Vector& c_, const RotationalInertia& Ic):
        m(m_),h(m*c_){
        //I=Ic-c x c x
        Vector3d c_eig=Map<const Vector3d>(c_.data);
        Map<Matrix3d>(I.data)=Map<const Matrix3d>(Ic.data)-m_*(c_eig*c_eig.transpose()-c_eig.dot(c_eig)*Matrix3d::Identity());
    }
    
    RigidBodyInertia operator*(double a,const RigidBodyInertia& I){
        return RigidBodyInertia(a*I.m,a*I.h,a*I.I,mhi);
    }
    
    RigidBodyInertia operator+(const RigidBodyInertia& Ia, const RigidBodyInertia& Ib){
        return RigidBodyInertia(Ia.m+Ib.m,Ia.h+Ib.h,Ia.I+Ib.I,mhi);
    }
    
    Wrench operator*(const RigidBodyInertia& I,const Twist& t){
        return Wrench(I.m*t.vel-I.h*t.rot,I.I*t.rot+I.h*t.vel);
    }

    RigidBodyInertia operator*(const Frame& T,const RigidBodyInertia& I){
        Frame X=T.Inverse();
        //mb=ma
        //hb=R*(h-m*r)
        //Ib = R(Ia+r x h x + (h-m*r) x r x)R'
        Vector hmr = (I.h-I.m*X.p);
        Vector3d r_eig = Map<Vector3d>(X.p.data);
        Vector3d h_eig = Map<const Vector3d>(I.h.data);
        Vector3d hmr_eig = Map<Vector3d>(hmr.data);
        Matrix3d rcrosshcross = h_eig *r_eig.transpose()-r_eig.dot(h_eig)*Matrix3d::Identity();
        Matrix3d hmrcrossrcross = r_eig*hmr_eig.transpose()-hmr_eig.dot(r_eig)*Matrix3d::Identity();
        Matrix3d R = Map<Matrix3d>(X.M.data);
        RotationalInertia Ib;
        Map<Matrix3d>(Ib.data) = R*((Map<const Matrix3d>(I.I.data)+rcrosshcross+hmrcrossrcross)*R.transpose());
        
        return RigidBodyInertia(I.m,T.M*hmr,Ib,mhi);
    }

    RigidBodyInertia operator*(const Rotation& M,const RigidBodyInertia& I){
        //mb=ma
        //hb=R*h
        //Ib = R(Ia)R' with r=0
        Matrix3d R = Map<const Matrix3d>(M.data);
        RotationalInertia Ib;
        Map<Matrix3d>(Ib.data) = R.transpose()*(Map<const Matrix3d>(I.I.data)*R);
        
        return RigidBodyInertia(I.m,M*I.h,Ib,mhi);
    }

    RigidBodyInertia RigidBodyInertia::RefPoint(const Vector& p){
        //mb=ma
        //hb=(h-m*r)
        //Ib = (Ia+r x h x + (h-m*r) x r x)
        Vector hmr = (this->h-this->m*p);
        Vector3d r_eig = Map<const Vector3d>(p.data);
        Vector3d h_eig = Map<Vector3d>(this->h.data);
        Vector3d hmr_eig = Map<Vector3d>(hmr.data);
        Matrix3d rcrosshcross = h_eig * r_eig.transpose()-r_eig.dot(h_eig)*Matrix3d::Identity();
        Matrix3d hmrcrossrcross = r_eig*hmr_eig.transpose()-hmr_eig.dot(r_eig)*Matrix3d::Identity();
        RotationalInertia Ib;
        Map<Matrix3d>(Ib.data) = Map<Matrix3d>(this->I.data)+rcrosshcross+hmrcrossrcross;
        
        return RigidBodyInertia(this->m,hmr,Ib,mhi);
    }
}//namespace
