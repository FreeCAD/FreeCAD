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

#ifndef KDL_JACOBIAN_HPP
#define KDL_JACOBIAN_HPP

#include "frames.hpp"
#include <Eigen/Core>

namespace KDL
{
    // Equal is friend function, but default arguments for friends are forbidden (§8.3.6.4)
    class Jacobian;
    bool Equal(const Jacobian& a,const Jacobian& b,double eps=epsilon);


    class Jacobian
    {
    public:

        EIGEN_MAKE_ALIGNED_OPERATOR_NEW
        Eigen::Matrix<double,6,Eigen::Dynamic> data;
        Jacobian();
        explicit Jacobian(unsigned int nr_of_columns);
        Jacobian(const Jacobian& arg);

        ///Allocates memory for new size (can break realtime behavior)
        void resize(unsigned int newNrOfColumns);

        ///Allocates memory if size of this and argument is different
        Jacobian& operator=(const Jacobian& arg);

        bool operator ==(const Jacobian& arg)const;
        bool operator !=(const Jacobian& arg)const;
        
        friend bool Equal(const Jacobian& a,const Jacobian& b,double eps);
        

        ~Jacobian();

        double operator()(unsigned int i,unsigned int j)const;
        double& operator()(unsigned int i,unsigned int j);
        unsigned int rows()const;
        unsigned int columns()const;

        friend void SetToZero(Jacobian& jac);

        friend bool changeRefPoint(const Jacobian& src1, const Vector& base_AB, Jacobian& dest);
        friend bool changeBase(const Jacobian& src1, const Rotation& rot, Jacobian& dest);
        friend bool changeRefFrame(const Jacobian& src1,const Frame& frame, Jacobian& dest);

        Twist getColumn(unsigned int i) const;
        void setColumn(unsigned int i,const Twist& t);

        void changeRefPoint(const Vector& base_AB);
        void changeBase(const Rotation& rot);
        void changeRefFrame(const Frame& frame);


    };

    bool changeRefPoint(const Jacobian& src1, const Vector& base_AB, Jacobian& dest);
    bool changeBase(const Jacobian& src1, const Rotation& rot, Jacobian& dest);
    bool changeRefFrame(const Jacobian& src1,const Frame& frame, Jacobian& dest);


}

#endif
