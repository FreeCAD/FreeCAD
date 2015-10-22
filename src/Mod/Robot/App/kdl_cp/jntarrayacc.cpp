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

#include "jntarrayacc.hpp"

namespace KDL
{
    JntArrayAcc::JntArrayAcc(unsigned int size):
        q(size),qdot(size),qdotdot(size)
    {
    }
    JntArrayAcc::JntArrayAcc(const JntArray& qin, const JntArray& qdotin,const JntArray& qdotdotin):
        q(qin),qdot(qdotin),qdotdot(qdotdotin)
    {
        assert(q.rows()==qdot.rows()&&qdot.rows()==qdotdot.rows());
    }
    JntArrayAcc::JntArrayAcc(const JntArray& qin, const JntArray& qdotin):
        q(qin),qdot(qdotin),qdotdot(q.rows())
    {
        assert(q.rows()==qdot.rows());
    }
    JntArrayAcc::JntArrayAcc(const JntArray& qin):
        q(qin),qdot(q.rows()),qdotdot(q.rows())
    {
    }

    void JntArrayAcc::resize(unsigned int newSize)
    {
      q.resize(newSize);
      qdot.resize(newSize);
      qdotdot.resize(newSize);
    }

    JntArray JntArrayAcc::value()const
    {
        return q;
    }

    JntArray JntArrayAcc::deriv()const
    {
        return qdot;
    }
    JntArray JntArrayAcc::dderiv()const
    {
        return qdotdot;
    }

    void Add(const JntArrayAcc& src1,const JntArrayAcc& src2,JntArrayAcc& dest)
    {
        Add(src1.q,src2.q,dest.q);
        Add(src1.qdot,src2.qdot,dest.qdot);
        Add(src1.qdotdot,src2.qdotdot,dest.qdotdot);
    }
    void Add(const JntArrayAcc& src1,const JntArrayVel& src2,JntArrayAcc& dest)
    {
        Add(src1.q,src2.q,dest.q);
        Add(src1.qdot,src2.qdot,dest.qdot);
        dest.qdotdot=src1.qdotdot;
    }
    void Add(const JntArrayAcc& src1,const JntArray& src2,JntArrayAcc& dest)
    {
        Add(src1.q,src2,dest.q);
        dest.qdot=src1.qdot;
        dest.qdotdot=src1.qdotdot;
    }

    void Subtract(const JntArrayAcc& src1,const JntArrayAcc& src2,JntArrayAcc& dest)
    {
        Subtract(src1.q,src2.q,dest.q);
        Subtract(src1.qdot,src2.qdot,dest.qdot);
        Subtract(src1.qdotdot,src2.qdotdot,dest.qdotdot);
    }
    void Subtract(const JntArrayAcc& src1,const JntArrayVel& src2,JntArrayAcc& dest)
    {
        Subtract(src1.q,src2.q,dest.q);
        Subtract(src1.qdot,src2.qdot,dest.qdot);
        dest.qdotdot=src1.qdotdot;
    }
    void Subtract(const JntArrayAcc& src1,const JntArray& src2,JntArrayAcc& dest)
    {
        Subtract(src1.q,src2,dest.q);
        dest.qdot=src1.qdot;
        dest.qdotdot=src1.qdotdot;
    }

    void Multiply(const JntArrayAcc& src,const double& factor,JntArrayAcc& dest)
    {
        Multiply(src.q,factor,dest.q);
        Multiply(src.qdot,factor,dest.qdot);
        Multiply(src.qdotdot,factor,dest.qdotdot);
    }
    void Multiply(const JntArrayAcc& src,const doubleVel& factor,JntArrayAcc& dest)
    {
        Multiply(src.qdot,factor.grad*2,dest.qdot);
        Multiply(src.qdotdot,factor.t,dest.qdotdot);
        Add(dest.qdot,dest.qdotdot,dest.qdotdot);
        Multiply(src.q,factor.grad,dest.q);
        Multiply(src.qdot,factor.t,dest.qdot);
        Add(dest.qdot,dest.q,dest.qdot);
        Multiply(src.q,factor.t,dest.q);
    }
    void Multiply(const JntArrayAcc& src,const doubleAcc& factor,JntArrayAcc& dest)
    {
        Multiply(src.q,factor.dd,dest.q);
        Multiply(src.qdot,factor.d*2,dest.qdot);
        Multiply(src.qdotdot,factor.t,dest.qdotdot);
        Add(dest.qdotdot,dest.qdot,dest.qdotdot);
        Add(dest.qdotdot,dest.q,dest.qdotdot);
        Multiply(src.q,factor.d,dest.q);
        Multiply(src.qdot,factor.t,dest.qdot);
        Add(dest.qdot,dest.q,dest.qdot);
        Multiply(src.q,factor.t,dest.q);
    }

    void Divide(const JntArrayAcc& src,const double& factor,JntArrayAcc& dest)
    {
        Divide(src.q,factor,dest.q);
        Divide(src.qdot,factor,dest.qdot);
        Divide(src.qdotdot,factor,dest.qdotdot);
    }
    void Divide(const JntArrayAcc& src,const doubleVel& factor,JntArrayAcc& dest)
    {
        Multiply(src.q,(2*factor.grad*factor.grad)/(factor.t*factor.t*factor.t),dest.q);
        Multiply(src.qdot,(2*factor.grad)/(factor.t*factor.t),dest.qdot);
        Divide(src.qdotdot,factor.t,dest.qdotdot);
        Subtract(dest.qdotdot,dest.qdot,dest.qdotdot);
        Add(dest.qdotdot,dest.q,dest.qdotdot);
        Multiply(src.q,factor.grad/(factor.t*factor.t),dest.q);
        Divide(src.qdot,factor.t,dest.qdot);
        Subtract(dest.qdot,dest.q,dest.qdot);
        Divide(src.q,factor.t,dest.q);
    }
    void Divide(const JntArrayAcc& src,const doubleAcc& factor,JntArrayAcc& dest)
    {
        Multiply(src.q,(2*factor.d*factor.d)/(factor.t*factor.t*factor.t)-factor.dd/(factor.t*factor.t),dest.q);
        Multiply(src.qdot,(2*factor.d)/(factor.t*factor.t),dest.qdot);
        Divide(src.qdotdot,factor.t,dest.qdotdot);
        Subtract(dest.qdotdot,dest.qdot,dest.qdotdot);
        Add(dest.qdotdot,dest.q,dest.qdotdot);
        Multiply(src.q,factor.d/(factor.t*factor.t),dest.q);
        Divide(src.qdot,factor.t,dest.qdot);
        Subtract(dest.qdot,dest.q,dest.qdot);
        Divide(src.q,factor.t,dest.q);
    }

    void SetToZero(JntArrayAcc& array)
    {
        SetToZero(array.q);
        SetToZero(array.qdot);
        SetToZero(array.qdotdot);
    }

    bool Equal(const JntArrayAcc& src1,const JntArrayAcc& src2,double eps)
    {
        return (Equal(src1.q,src2.q,eps)&&Equal(src1.qdot,src2.qdot,eps)&&Equal(src1.qdotdot,src2.qdotdot,eps));
    }
}


