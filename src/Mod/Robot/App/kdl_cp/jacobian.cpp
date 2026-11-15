// Copyright  (C)  2007  Ruben Smits <ruben dot smits at intermodalics dot eu>

// Version: 1.0
// Author: Ruben Smits <ruben dot smits at intermodalics dot eu>
// Maintainer: Ruben Smits <ruben dot smits at intermodalics dot eu>
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

#include "jacobian.hpp"

namespace KDL
{
    Jacobian::Jacobian()
    {
    }


    Jacobian::Jacobian(unsigned int nr_of_columns):
        data(6,nr_of_columns)
    {
        data.setZero();
    }

    Jacobian::Jacobian(const Jacobian& arg):
        data(arg.data)
    {
    }

    Jacobian& Jacobian::operator = (const Jacobian& arg)
    {
        this->data=arg.data;
        return *this;
    }


    Jacobian::~Jacobian()
    {

    }

    void Jacobian::resize(unsigned int new_nr_of_columns)
    {
        data.conservativeResize(Eigen::NoChange,new_nr_of_columns);
    }

    double Jacobian::operator()(unsigned int i,unsigned int j)const
    {
        return data(i,j);
    }

    double& Jacobian::operator()(unsigned int i,unsigned int j)
    {
        return data(i,j);
    }

    unsigned int Jacobian::rows()const
    {
        return static_cast<unsigned int>(data.rows());
    }

    unsigned int Jacobian::columns()const
    {
        return static_cast<unsigned int>(data.cols());
    }

    void SetToZero(Jacobian& jac)
    {
        jac.data.setZero();
    }

    void Jacobian::changeRefPoint(const Vector& base_AB){
        for(unsigned int i=0;i<data.cols();i++)
            this->setColumn(i,this->getColumn(i).RefPoint(base_AB));
    }

    bool changeRefPoint(const Jacobian& src1, const Vector& base_AB, Jacobian& dest)
    {
        if(src1.columns()!=dest.columns())
            return false;
        for(unsigned int i=0;i<src1.columns();i++)
            dest.setColumn(i,src1.getColumn(i).RefPoint(base_AB));
        return true;
    }

    void Jacobian::changeBase(const Rotation& rot){
        for(unsigned int i=0;i<data.cols();i++)
            this->setColumn(i,rot*this->getColumn(i));;
    }

    bool changeBase(const Jacobian& src1, const Rotation& rot, Jacobian& dest)
    {
        if(src1.columns()!=dest.columns())
            return false;
        for(unsigned int i=0;i<src1.columns();i++)
            dest.setColumn(i,rot*src1.getColumn(i));;
        return true;
    }

    void Jacobian::changeRefFrame(const Frame& frame){
        for(unsigned int i=0;i<data.cols();i++)
            this->setColumn(i,frame*this->getColumn(i));
    }

    bool changeRefFrame(const Jacobian& src1,const Frame& frame, Jacobian& dest)
    {
        if(src1.columns()!=dest.columns())
            return false;
        for(unsigned int i=0;i<src1.columns();i++)
            dest.setColumn(i,frame*src1.getColumn(i));
        return true;
    }

    bool Jacobian::operator ==(const Jacobian& arg)const
    {
        return Equal((*this),arg);
    }

    bool Jacobian::operator!=(const Jacobian& arg)const
    {
        return !Equal((*this),arg);
    }

    bool Equal(const Jacobian& a,const Jacobian& b,double eps)
    {
        if(a.rows()==b.rows()&&a.columns()==b.columns()){
            return (a.data-b.data).isZero(eps);
        }else
            return false;
    }

    Twist Jacobian::getColumn(unsigned int i) const{
        return Twist(Vector(data(0,i),data(1,i),data(2,i)),Vector(data(3,i),data(4,i),data(5,i)));
    }

    void Jacobian::setColumn(unsigned int i,const Twist& t){
        data.col(i).head<3>()=Eigen::Map<const Eigen::Vector3d>(t.vel.data);
        data.col(i).tail<3>()=Eigen::Map<const Eigen::Vector3d>(t.rot.data);
    }

}
