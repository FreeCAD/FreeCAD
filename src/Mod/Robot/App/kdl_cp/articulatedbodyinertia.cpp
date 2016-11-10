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

#include "articulatedbodyinertia.hpp"

#include <Eigen/Core>

using namespace Eigen;

namespace KDL{
    
  ArticulatedBodyInertia::ArticulatedBodyInertia(const RigidBodyInertia& rbi)
    {
        this->M=Matrix3d::Identity()*rbi.m;
        this->I=Map<const Matrix3d>(rbi.I.data);
        this->H << 0,-rbi.h[2],rbi.h[1],
            rbi.h[2],0,-rbi.h[0],
            -rbi.h[1],rbi.h[0],0;
    }
    
    ArticulatedBodyInertia::ArticulatedBodyInertia(double m, const Vector& c, const RotationalInertia& Ic)
    {
        *this = RigidBodyInertia(m,c,Ic);
    }

  ArticulatedBodyInertia::ArticulatedBodyInertia(const Eigen::Matrix3d& M, const Eigen::Matrix3d& H, const Eigen::Matrix3d& I)
    {
        this->M=M;
        this->I=I;
        this->H=H;
    }
    
    ArticulatedBodyInertia operator*(double a,const ArticulatedBodyInertia& I){
        return ArticulatedBodyInertia(a*I.M,a*I.H,a*I.I);
    }
    
    ArticulatedBodyInertia operator+(const ArticulatedBodyInertia& Ia, const ArticulatedBodyInertia& Ib){
        return ArticulatedBodyInertia(Ia.M+Ib.M,Ia.H+Ib.H,Ia.I+Ib.I);
    }

    ArticulatedBodyInertia operator+(const RigidBodyInertia& Ia, const ArticulatedBodyInertia& Ib){
        return ArticulatedBodyInertia(Ia)+Ib;
    }
    ArticulatedBodyInertia operator-(const ArticulatedBodyInertia& Ia, const ArticulatedBodyInertia& Ib){
        return ArticulatedBodyInertia(Ia.M-Ib.M,Ia.H-Ib.H,Ia.I-Ib.I);
    }

    ArticulatedBodyInertia operator-(const RigidBodyInertia& Ia, const ArticulatedBodyInertia& Ib){
        return ArticulatedBodyInertia(Ia)-Ib;
    }
    
    Wrench operator*(const ArticulatedBodyInertia& I,const Twist& t){
        Wrench result;
        Vector3d::Map(result.force.data)=I.M*Vector3d::Map(t.vel.data)+I.H.transpose()*Vector3d::Map(t.rot.data);
        Vector3d::Map(result.torque.data)=I.I*Vector3d::Map(t.rot.data)+I.H*Vector3d::Map(t.vel.data);
        return result;
    }

    ArticulatedBodyInertia operator*(const Frame& T,const ArticulatedBodyInertia& I){
        Frame X=T.Inverse();
        //mb=ma
        //hb=R*(h-m*r)
        //Ib = R(Ia+r x h x + (h-m*r) x r x)R'
        Map<Matrix3d> E(X.M.data);
        Matrix3d rcross;
        rcross << 0,-X.p[2],X.p[1],
            X.p[2],0,-X.p[0],
            -X.p[1],X.p[0],0;
        
        Matrix3d HrM=I.H-rcross*I.M;
        return ArticulatedBodyInertia(E*I.M*E.transpose(),E*HrM*E.transpose(),E*(I.I-rcross*I.H.transpose()+HrM*rcross)*E.transpose());
    }

    ArticulatedBodyInertia operator*(const Rotation& M,const ArticulatedBodyInertia& I){
        Map<const Matrix3d> E(M.data);
        return ArticulatedBodyInertia(E.transpose()*I.M*E,E.transpose()*I.H*E,E.transpose()*I.I*E);
    }

    ArticulatedBodyInertia ArticulatedBodyInertia::RefPoint(const Vector& p){
        //mb=ma
        //hb=R*(h-m*r)
        //Ib = R(Ia+r x h x + (h-m*r) x r x)R'
        Matrix3d rcross;
        rcross << 0,-p[2],p[1],
            p[2],0,-p[0],
            -p[1],p[0],0;
        
        Matrix3d HrM=this->H-rcross*this->M;
        return ArticulatedBodyInertia(this->M,HrM,this->I-rcross*this->H.transpose()+HrM*rcross);
    }
}//namespace
