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

#include "rotationalinertia.hpp"
#include <Eigen/Core>
using namespace Eigen;

namespace KDL
{
	RotationalInertia::RotationalInertia(double Ixx,double Iyy,double Izz,double Ixy,double Ixz,double Iyz)
	{
        data[0]=Ixx;
        data[1]=data[3]=Ixy;
        data[2]=data[6]=Ixz;
        data[4]=Iyy;
        data[5]=data[7]=Iyz;
        data[8]=Izz;
        
	}

	RotationalInertia::~RotationalInertia()
	{
	}

	Vector RotationalInertia::operator*(const Vector& omega) const {
		// Complexity : 9M+6A
        Vector result;
        Map<Vector3d>(result.data)=Map<const Matrix3d>(this->data)*Map<const Vector3d>(omega.data);
        return result;
 	}

    RotationalInertia operator*(double a, const RotationalInertia& I){
        RotationalInertia result;
        Map<Matrix3d>(result.data)=a*Map<const Matrix3d>(I.data);
        return result;
    }
    
    RotationalInertia operator+(const RotationalInertia& Ia, const RotationalInertia& Ib){
        RotationalInertia result;
        Map<Matrix3d>(result.data)=Map<const Matrix3d>(Ia.data)+Map<const Matrix3d>(Ib.data);
        return result;
    }
}

