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

#ifndef KDL_JNTARRAYACC_HPP
#define KDL_JNTARRAYACC_HPP

#include "utilities/utility.h"
#include "jntarray.hpp"
#include "jntarrayvel.hpp"
#include "frameacc.hpp"

namespace KDL
{
    // Equal is friend function, but default arguments for friends are forbidden (§8.3.6.4)
    class JntArrayAcc;
    bool Equal(const JntArrayAcc& src1,const JntArrayAcc& src2,double eps=epsilon);
    void Add(const JntArrayAcc& src1,const JntArrayAcc& src2,JntArrayAcc& dest);
    void Add(const JntArrayAcc& src1,const JntArrayVel& src2,JntArrayAcc& dest);
    void Add(const JntArrayAcc& src1,const JntArray& src2,JntArrayAcc& dest);
    void Subtract(const JntArrayAcc& src1,const JntArrayAcc& src2,JntArrayAcc& dest);
    void Subtract(const JntArrayAcc& src1,const JntArrayVel& src2,JntArrayAcc& dest);
    void Subtract(const JntArrayAcc& src1,const JntArray& src2,JntArrayAcc& dest);
    void Multiply(const JntArrayAcc& src,const double& factor,JntArrayAcc& dest);
    void Multiply(const JntArrayAcc& src,const doubleVel& factor,JntArrayAcc& dest);
    void Multiply(const JntArrayAcc& src,const doubleAcc& factor,JntArrayAcc& dest);
    void Divide(const JntArrayAcc& src,const double& factor,JntArrayAcc& dest);
    void Divide(const JntArrayAcc& src,const doubleVel& factor,JntArrayAcc& dest);
    void Divide(const JntArrayAcc& src,const doubleAcc& factor,JntArrayAcc& dest);
    void SetToZero(JntArrayAcc& array);

    class JntArrayAcc
    {
    public:
        JntArray q;
        JntArray qdot;
        JntArray qdotdot;
    public:
      JntArrayAcc(){};
        explicit JntArrayAcc(unsigned int size);
        JntArrayAcc(const JntArray& q,const JntArray& qdot,const JntArray& qdotdot);
        JntArrayAcc(const JntArray& q,const JntArray& qdot);
        explicit JntArrayAcc(const JntArray& q);

        void resize(unsigned int newSize);

        JntArray value()const;
        JntArray deriv()const;
        JntArray dderiv()const;

        friend void Add(const JntArrayAcc& src1,const JntArrayAcc& src2,JntArrayAcc& dest);
        friend void Add(const JntArrayAcc& src1,const JntArrayVel& src2,JntArrayAcc& dest);
        friend void Add(const JntArrayAcc& src1,const JntArray& src2,JntArrayAcc& dest);
        friend void Subtract(const JntArrayAcc& src1,const JntArrayAcc& src2,JntArrayAcc& dest);
        friend void Subtract(const JntArrayAcc& src1,const JntArrayVel& src2,JntArrayAcc& dest);
        friend void Subtract(const JntArrayAcc& src1,const JntArray& src2,JntArrayAcc& dest);
        friend void Multiply(const JntArrayAcc& src,const double& factor,JntArrayAcc& dest);
        friend void Multiply(const JntArrayAcc& src,const doubleVel& factor,JntArrayAcc& dest);
        friend void Multiply(const JntArrayAcc& src,const doubleAcc& factor,JntArrayAcc& dest);
        friend void Divide(const JntArrayAcc& src,const double& factor,JntArrayAcc& dest);
        friend void Divide(const JntArrayAcc& src,const doubleVel& factor,JntArrayAcc& dest);
        friend void Divide(const JntArrayAcc& src,const doubleAcc& factor,JntArrayAcc& dest);
        friend void SetToZero(JntArrayAcc& array);
        friend bool Equal(const JntArrayAcc& src1,const JntArrayAcc& src2,double eps);

    };

}

#endif
