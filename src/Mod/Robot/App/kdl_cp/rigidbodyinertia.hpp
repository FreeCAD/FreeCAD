// Copyright  (C)  2009  Ruben Smits <ruben dot smits at mech dot kuleuven dot be>

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

#ifndef KDL_RIGIDBODYINERTIA_HPP
#define KDL_RIGIDBODYINERTIA_HPP

#include "frames.hpp"

#include "rotationalinertia.hpp"

namespace KDL {
    
    /**
     *	\brief 6D Inertia of a rigid body
     *
     *	The inertia is defined in a certain reference point and a certain reference base.
     *	The reference point does not have to coincide with the origin of the reference frame.
     */
    class RigidBodyInertia{
    public:
        
        /**
         * 	This constructor creates a cartesian space inertia matrix,
         * 	the arguments are the mass, the vector from the reference point to cog and the rotational inertia in the cog.
         */
        explicit RigidBodyInertia(double m=0, const Vector& oc=Vector::Zero(), const RotationalInertia& Ic=RotationalInertia::Zero());
        
        /**
         * Creates an inertia with zero mass, and zero RotationalInertia
         */
        static inline RigidBodyInertia Zero(){
            return RigidBodyInertia(0.0,Vector::Zero(),RotationalInertia::Zero());
        };
        
        
        ~RigidBodyInertia(){};
        
        friend RigidBodyInertia operator*(double a,const RigidBodyInertia& I);
        friend RigidBodyInertia operator+(const RigidBodyInertia& Ia,const RigidBodyInertia& Ib);
        friend Wrench operator*(const RigidBodyInertia& I,const Twist& t);
        friend RigidBodyInertia operator*(const Frame& T,const RigidBodyInertia& I);
        friend RigidBodyInertia operator*(const Rotation& R,const RigidBodyInertia& I);

        /**
         * Reference point change with v the vector from the old to
         * the new point expressed in the current reference frame
         */
        RigidBodyInertia RefPoint(const Vector& p);

        /**
         * Get the mass of the rigid body
         */
        double getMass() const{
            return m;
        };
        
        /**
         * Get the center of gravity of the rigid body
         */
        Vector getCOG() const{
            if(m==0) return Vector::Zero();
            else return h/m;
        };

        /**
         * Get the rotational inertia expressed in the reference frame (not the cog)
         */
        RotationalInertia getRotationalInertia() const{
            return I;
        };

    private:
        RigidBodyInertia(double m,const Vector& h,const RotationalInertia& I,bool mhi);
        double m;
        Vector h;
        RotationalInertia I;

        friend class ArticulatedBodyInertia;
        
    };

    /**
     * Scalar product: I_new = double * I_old
     */
    RigidBodyInertia operator*(double a,const RigidBodyInertia& I);
    /**
     * addition I: I_new = I_old1 + I_old2, make sure that I_old1
     * and I_old2 are expressed in the same reference frame/point,
     * otherwise the result is worth nothing
     */
    RigidBodyInertia operator+(const RigidBodyInertia& Ia,const RigidBodyInertia& Ib);

    /**
     * calculate spatial momentum: h = I*v
     * make sure that the twist v and the inertia are expressed in the same reference frame/point
     */
    Wrench operator*(const RigidBodyInertia& I,const Twist& t);

    /**
     * Coordinate system transform Ia = T_a_b*Ib with T_a_b the frame from a to b.
     */
    RigidBodyInertia operator*(const Frame& T,const RigidBodyInertia& I);
    /**
     * Reference frame orientation change Ia = R_a_b*Ib with R_a_b
     * the rotation of b expressed in a
     */
    RigidBodyInertia operator*(const Rotation& R,const RigidBodyInertia& I);



}//namespace


#endif
