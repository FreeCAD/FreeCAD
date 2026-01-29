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

//------- class for only the Rotational Inertia --------

namespace KDL
{
    //Forward declaration
    class RigidBodyInertia;

	class RotationalInertia{
    public:
              
        explicit RotationalInertia(double Ixx=0,double Iyy=0,double Izz=0,double Ixy=0,double Ixz=0,double Iyz=0);
        
        static inline RotationalInertia Zero(){
            return RotationalInertia(0,0,0,0,0,0);
        };

        friend RotationalInertia operator*(double a, const RotationalInertia& I);
        friend RotationalInertia operator+(const RotationalInertia& Ia, const RotationalInertia& Ib);

        /**
         * This function calculates the angular momentum resulting from a rotational velocity omega
         */
        KDL::Vector operator*(const KDL::Vector& omega) const;
        
        ~RotationalInertia();

        friend class RigidBodyInertia;
        ///Scalar product
        friend RigidBodyInertia operator*(double a,const RigidBodyInertia& I);
        ///addition
        friend RigidBodyInertia operator+(const RigidBodyInertia& Ia,const RigidBodyInertia& Ib);
        ///calculate spatial momentum
        friend Wrench operator*(const RigidBodyInertia& I,const Twist& t);
        ///coordinate system transform Ia = T_a_b*Ib with T_a_b the frame from a to b
        friend RigidBodyInertia operator*(const Frame& T,const RigidBodyInertia& I);
        ///base frame orientation change Ia = R_a_b*Ib with R_a_b the rotation for frame from a to b
        friend RigidBodyInertia operator*(const Rotation& R,const RigidBodyInertia& I);

        double data[9];
	};

    RotationalInertia operator*(double a, const RotationalInertia& I);
    RotationalInertia operator+(const RotationalInertia& Ia, const RotationalInertia& Ib);

}