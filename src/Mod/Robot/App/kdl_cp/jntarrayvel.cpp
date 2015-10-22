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
    JntArrayVel::JntArrayVel(unsigned int size):
        q(size),qdot(size)
    {
    }
    JntArrayVel::JntArrayVel(const JntArray& qin, const JntArray& qdotin):
        q(qin),qdot(qdotin)
    {
        assert(q.rows()==qdot.rows());
    }
    JntArrayVel::JntArrayVel(const JntArray& qin):
        q(qin),qdot(q.rows())
    {
    }

    void JntArrayVel::resize(unsigned int newSize)
    {
      q.resize(newSize);
      qdot.resize(newSize);
    }

    JntArray JntArrayVel::value()const
    {
        return q;
    }

    JntArray JntArrayVel::deriv()const
    {
        return qdot;
    }

    void Add(const JntArrayVel& src1,const JntArrayVel& src2,JntArrayVel& dest)
    {
        Add(src1.q,src2.q,dest.q);
        Add(src1.qdot,src2.qdot,dest.qdot);
    }
    void Add(const JntArrayVel& src1,const JntArray& src2,JntArrayVel& dest)
    {
        Add(src1.q,src2,dest.q);
        dest.qdot=src1.qdot;
    }

    void Subtract(const JntArrayVel& src1,const JntArrayVel& src2,JntArrayVel& dest)
    {
        Subtract(src1.q,src2.q,dest.q);
        Subtract(src1.qdot,src2.qdot,dest.qdot);
    }
    void Subtract(const JntArrayVel& src1,const JntArray& src2,JntArrayVel& dest)
    {
        Subtract(src1.q,src2,dest.q);
        dest.qdot=src1.qdot;
    }

    void Multiply(const JntArrayVel& src,const double& factor,JntArrayVel& dest)
    {
        Multiply(src.q,factor,dest.q);
        Multiply(src.qdot,factor,dest.qdot);
    }
    void Multiply(const JntArrayVel& src,const doubleVel& factor,JntArrayVel& dest)
    {
        Multiply(src.q,factor.grad,dest.q);
        Multiply(src.qdot,factor.t,dest.qdot);
        Add(dest.qdot,dest.q,dest.qdot);
        Multiply(src.q,factor.t,dest.q);
    }

    void Divide(const JntArrayVel& src,const double& factor,JntArrayVel& dest)
    {
        Divide(src.q,factor,dest.q);
        Divide(src.qdot,factor,dest.qdot);
    }
    void Divide(const JntArrayVel& src,const doubleVel& factor,JntArrayVel& dest)
    {
        Multiply(src.q,(factor.grad/factor.t/factor.t),dest.q);
        Divide(src.qdot,factor.t,dest.qdot);
        Subtract(dest.qdot,dest.q,dest.qdot);
        Divide(src.q,factor.t,dest.q);
    }

    void SetToZero(JntArrayVel& array)
    {
        SetToZero(array.q);
        SetToZero(array.qdot);
    }

    bool Equal(const JntArrayVel& src1,const JntArrayVel& src2,double eps)
    {
        return Equal(src1.q,src2.q,eps)&&Equal(src1.qdot,src2.qdot,eps);
    }
}


