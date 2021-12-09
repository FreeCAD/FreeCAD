// Copyright  (C)  2009  Dominick Vanthienen <dominick dot vanthienen at mech dot kuleuven dot be>

// Version: 1.0
// Author: Dominick Vanthienen <dominick dot vanthienen at mech dot kuleuven dot be>
// Maintainer: Dominick Vanthienen <ruben dot smits at mech dot kuleuven dot be>
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

#include "jntspaceinertiamatrix.hpp"

namespace KDL
{
    using namespace Eigen;

    JntSpaceInertiaMatrix::JntSpaceInertiaMatrix()
    {
    }

    JntSpaceInertiaMatrix::JntSpaceInertiaMatrix(int _size):
        data(_size, _size)
    {
        data.setZero();
    }


    JntSpaceInertiaMatrix::JntSpaceInertiaMatrix(const JntSpaceInertiaMatrix& arg):
        data(arg.data)
    {
    }

    JntSpaceInertiaMatrix& JntSpaceInertiaMatrix::operator = (const JntSpaceInertiaMatrix& arg)
    {
        data=arg.data;
        return *this;
    }


    JntSpaceInertiaMatrix::~JntSpaceInertiaMatrix()
    {
    }

    void JntSpaceInertiaMatrix::resize(unsigned int newSize)
    {
        data.resize(newSize,newSize);
    }

    double JntSpaceInertiaMatrix::operator()(unsigned int i,unsigned int j)const
    {
        return data(i, j);
    }

    double& JntSpaceInertiaMatrix::operator()(unsigned int i,unsigned int j)
    {
        return data(i, j);
    }

    unsigned int JntSpaceInertiaMatrix::rows()const
    {
        return data.rows();
    }

    unsigned int JntSpaceInertiaMatrix::columns()const
    {
        return data.cols();
    }
    

    void Add(const JntSpaceInertiaMatrix& src1,const JntSpaceInertiaMatrix& src2,JntSpaceInertiaMatrix& dest)
    {
        dest.data=src1.data+src2.data;
    }

    void Subtract(const JntSpaceInertiaMatrix& src1,const JntSpaceInertiaMatrix& src2,JntSpaceInertiaMatrix& dest)
    {
        dest.data=src1.data-src2.data;
    }

    void Multiply(const JntSpaceInertiaMatrix& src,const double& factor,JntSpaceInertiaMatrix& dest)
    {
        dest.data=factor*src.data;
    }

    void Divide(const JntSpaceInertiaMatrix& src,const double& factor,JntSpaceInertiaMatrix& dest)
    {
        dest.data=src.data/factor;
    }

    void Multiply(const JntSpaceInertiaMatrix& src, const JntArray& vec, JntArray& dest)
    {
        dest.data=src.data.lazyProduct(vec.data);
    }
    
    void SetToZero(JntSpaceInertiaMatrix& mat)
    {
        mat.data.setZero();
    }

    bool Equal(const JntSpaceInertiaMatrix& src1, const JntSpaceInertiaMatrix& src2,double eps)
    {
        if(src1.rows()!=src2.rows()||src1.columns()!=src2.columns())
            return false;
        return src1.data.isApprox(src2.data,eps);
    }

    bool operator==(const JntSpaceInertiaMatrix& src1,const JntSpaceInertiaMatrix& src2){return Equal(src1,src2,epsilon);}
    //bool operator!=(const JntSpaceInertiaMatrix& src1,const JntSpaceInertiaMatrix& src2){return Equal(src1,src2);}

}


