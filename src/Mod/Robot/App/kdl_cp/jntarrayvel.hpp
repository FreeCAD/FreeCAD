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

#ifndef KDL_JNTARRAYVEL_HPP
#define KDL_JNTARRAYVEL_HPP

#include "utilities/utility.h"
#include "jntarray.hpp"
#include "framevel.hpp"

namespace KDL
{
    // Equal is friend function, but default arguments for friends are forbidden (§8.3.6.4)
    class JntArrayVel;
    bool Equal(const JntArrayVel& src1,const JntArrayVel& src2,double eps=epsilon);
    void Add(const JntArrayVel& src1,const JntArrayVel& src2,JntArrayVel& dest);
    void Add(const JntArrayVel& src1,const JntArray& src2,JntArrayVel& dest);
    void Subtract(const JntArrayVel& src1,const JntArrayVel& src2,JntArrayVel& dest);
    void Subtract(const JntArrayVel& src1,const JntArray& src2,JntArrayVel& dest);
    void Multiply(const JntArrayVel& src,const double& factor,JntArrayVel& dest);
    void Multiply(const JntArrayVel& src,const doubleVel& factor,JntArrayVel& dest);
    void Divide(const JntArrayVel& src,const double& factor,JntArrayVel& dest);
    void Divide(const JntArrayVel& src,const doubleVel& factor,JntArrayVel& dest);
    void SetToZero(JntArrayVel& array);


    class JntArrayVel
    {
    public:
        JntArray q;
        JntArray qdot;
    public:
        JntArrayVel(){};
        explicit JntArrayVel(unsigned int size);
        JntArrayVel(const JntArray& q,const JntArray& qdot);
        explicit JntArrayVel(const JntArray& q);

        void resize(unsigned int newSize);

        JntArray value()const;
        JntArray deriv()const;

        friend void Add(const JntArrayVel& src1,const JntArrayVel& src2,JntArrayVel& dest);
        friend void Add(const JntArrayVel& src1,const JntArray& src2,JntArrayVel& dest);
        friend void Subtract(const JntArrayVel& src1,const JntArrayVel& src2,JntArrayVel& dest);
        friend void Subtract(const JntArrayVel& src1,const JntArray& src2,JntArrayVel& dest);
        friend void Multiply(const JntArrayVel& src,const double& factor,JntArrayVel& dest);
        friend void Multiply(const JntArrayVel& src,const doubleVel& factor,JntArrayVel& dest);
        friend void Divide(const JntArrayVel& src,const double& factor,JntArrayVel& dest);
        friend void Divide(const JntArrayVel& src,const doubleVel& factor,JntArrayVel& dest);
        friend void SetToZero(JntArrayVel& array);
        friend bool Equal(const JntArrayVel& src1,const JntArrayVel& src2,double eps);

    };

}

#endif
