// SPDX-License-Identifier: LGPL-2.1-or-later

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

#pragma once
#include "frames.hpp"
 

namespace KDL {
/**
 * Preliminary class to implement Stiffness, only diagonal stiffness is implemented
 * no transformations provided...
 *
 * Implements a diagonal stiffness matrix.
 * first 3 elements are stiffness for translations
 * last  3 elements are stiffness for rotations.
 */
class Stiffness {
    double data[6];
public:
	Stiffness() {
		data[0]=0;
		data[1]=0;
		data[2]=0;
		data[3]=0;
		data[4]=0;
		data[5]=0;
	}
    Stiffness(double* d) {
        data[0]=d[0];
        data[1]=d[1];
        data[2]=d[2];
        data[3]=d[3];
        data[4]=d[4];
        data[5]=d[5];
    }
    Stiffness(double x,double y,double z,double rx,double ry,double rz) {
        data[0]=x;
        data[1]=y;
        data[2]=z;
        data[3]=rx;
        data[4]=ry;
        data[5]=rz;
    }
    double& operator[](int i) {
        return data[i];
    }
    double operator[](int i) const {
        return data[i];
    }
	Twist Inverse(const Wrench& w) const{
		Twist t;
		t[0]=w[0]/data[0];
		t[1]=w[1]/data[1];
		t[2]=w[2]/data[2];
		t[3]=w[3]/data[3];
		t[4]=w[4]/data[4];
		t[5]=w[5]/data[5];
		return t;
	}
};

inline Wrench operator * (const Stiffness& s, const Twist& t) {
    Wrench w;
    w[0]=s[0]*t[0];
    w[1]=s[1]*t[1];
    w[2]=s[2]*t[2];
    w[3]=s[3]*t[3];
    w[4]=s[4]*t[4];
    w[5]=s[5]*t[5];
    return w;
}

inline Stiffness operator+(const Stiffness& s1, const Stiffness& s2) {
	Stiffness s;
	s[0]=s1[0]+s2[0];
	s[1]=s1[1]+s2[1];
	s[2]=s1[2]+s2[2];
	s[3]=s1[3]+s2[3];
	s[4]=s1[4]+s2[4];
	s[5]=s1[5]+s2[5];
	return s;
}
inline void posrandom(Stiffness& F) {
	posrandom(F[0]);
	posrandom(F[1]);
	posrandom(F[2]);
	posrandom(F[3]);
	posrandom(F[4]);
	posrandom(F[5]);
}

inline void random(Stiffness& F) {
	posrandom(F);
}


} 