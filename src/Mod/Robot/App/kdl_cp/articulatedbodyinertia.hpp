// SPDX-License-Identifier: LGPL-2.1-or-later

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

#pragma once

#include "frames.hpp"

#include "rotationalinertia.hpp"
#include "rigidbodyinertia.hpp"

#include <Eigen/Core>

namespace KDL {
    
    /**
     *	\brief 6D Inertia of a articulated body
     *
     *	The inertia is defined in a certain reference point and a certain reference base.
     *	The reference point does not have to coincide with the origin of the reference frame.
     */
    class ArticulatedBodyInertia{
    public:

        /**
         * 	This constructor creates a zero articulated body inertia matrix,
         */
        ArticulatedBodyInertia(){
            *this=ArticulatedBodyInertia::Zero();
        }

        /**
         * 	This constructor creates a cartesian space articulated body inertia matrix,
         * 	the arguments is a rigid body inertia.
         */
        ArticulatedBodyInertia(const RigidBodyInertia& rbi);

        /**
         * 	This constructor creates a cartesian space inertia matrix,
         * 	the arguments are the mass, the vector from the reference point to cog and the rotational inertia in the cog.
         */
        explicit ArticulatedBodyInertia(double m, const Vector& oc=Vector::Zero(), const RotationalInertia& Ic=RotationalInertia::Zero());
        
        /**
         * Creates an inertia with zero mass, and zero RotationalInertia
         */
        static inline ArticulatedBodyInertia Zero(){
            return ArticulatedBodyInertia(Eigen::Matrix3d::Zero(),Eigen::Matrix3d::Zero(),Eigen::Matrix3d::Zero());
        };
        
        
        ~ArticulatedBodyInertia(){};
        
        friend ArticulatedBodyInertia operator*(double a,const ArticulatedBodyInertia& I);
        friend ArticulatedBodyInertia operator+(const ArticulatedBodyInertia& Ia,const ArticulatedBodyInertia& Ib);
        friend ArticulatedBodyInertia operator+(const ArticulatedBodyInertia& Ia,const RigidBodyInertia& Ib);
        friend ArticulatedBodyInertia operator-(const ArticulatedBodyInertia& Ia,const ArticulatedBodyInertia& Ib);
        friend ArticulatedBodyInertia operator-(const ArticulatedBodyInertia& Ia,const RigidBodyInertia& Ib);
        friend Wrench operator*(const ArticulatedBodyInertia& I,const Twist& t);
        friend ArticulatedBodyInertia operator*(const Frame& T,const ArticulatedBodyInertia& I);
        friend ArticulatedBodyInertia operator*(const Rotation& R,const ArticulatedBodyInertia& I);

        /**
         * Reference point change with v the vector from the old to
         * the new point expressed in the current reference frame
         */
        ArticulatedBodyInertia RefPoint(const Vector& p);

        ArticulatedBodyInertia(const Eigen::Matrix3d& M,const Eigen::Matrix3d& H,const Eigen::Matrix3d& I);

        Eigen::Matrix3d M;
        Eigen::Matrix3d H;
        Eigen::Matrix3d I;
    };

    /**
     * Scalar product: I_new = double * I_old
     */
    ArticulatedBodyInertia operator*(double a,const ArticulatedBodyInertia& I);
    /**
     * addition I: I_new = I_old1 + I_old2, make sure that I_old1
     * and I_old2 are expressed in the same reference frame/point,
     * otherwise the result is worth nothing
     */
    ArticulatedBodyInertia operator+(const ArticulatedBodyInertia& Ia,const ArticulatedBodyInertia& Ib);
    ArticulatedBodyInertia operator+(const ArticulatedBodyInertia& Ia,const RigidBodyInertia& Ib);
    ArticulatedBodyInertia operator-(const ArticulatedBodyInertia& Ia,const ArticulatedBodyInertia& Ib);
    ArticulatedBodyInertia operator-(const ArticulatedBodyInertia& Ia,const RigidBodyInertia& Ib);

    /**
     * calculate spatial momentum: h = I*v
     * make sure that the twist v and the inertia are expressed in the same reference frame/point
     */
    Wrench operator*(const ArticulatedBodyInertia& I,const Twist& t);

    /**
     * Coordinate system transform Ia = T_a_b*Ib with T_a_b the frame from a to b.
     */
    ArticulatedBodyInertia operator*(const Frame& T,const ArticulatedBodyInertia& I);
    /**
     * Reference frame orientation change Ia = R_a_b*Ib with R_a_b
     * the rotation of b expressed in a
     */
    ArticulatedBodyInertia operator*(const Rotation& R,const ArticulatedBodyInertia& I);

}