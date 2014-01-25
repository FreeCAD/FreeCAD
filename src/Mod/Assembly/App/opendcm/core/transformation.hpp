/*
    openDCM, dimensional constraint manager
    Copyright (C) 2013  Stefan Troeger <stefantroeger@gmx.net>

    This library is free software; you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published by
    the Free Software Foundation; either version 2.1 of the License, or
    (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License along
    with this library; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/


#ifndef DCM_TRANSFORMATION_H
#define DCM_TRANSFORMATION_H

#include <cmath>
#include <Eigen/Core>
#include <Eigen/Geometry>

#include <boost/mpl/if.hpp>

namespace dcm {
namespace detail {

template<typename Scalar, int Dim>
class Transform {

public:
    typedef Eigen::Matrix<Scalar, Dim, 1> Vector;
    typedef typename boost::mpl::if_c< Dim == 3,
            Eigen::Quaternion<Scalar>,
            Eigen::Rotation2D<Scalar> >::type     Rotation;
    typedef Eigen::Translation<Scalar, Dim>	  Translation;
    typedef Eigen::UniformScaling<Scalar> 	  Scaling;
    typedef typename Rotation::RotationMatrixType RotationMatrix;

protected:
    Rotation   	m_rotation;
    Translation	m_translation;
    Scaling  	m_scale;

public:
    Transform();

    Transform(const Rotation& r);
    Transform(const Rotation& r, const Translation& t);
    Transform(const Rotation& r, const Translation& t, const Scaling& s);

    //access the single parts and manipulate them
    //***********************
    const Rotation& rotation() const;
    template<typename Derived>
    Transform& setRotation(const Eigen::RotationBase<Derived,Dim>& rotation);
    template<typename Derived>
    Transform& rotate(const Eigen::RotationBase<Derived,Dim>& rotation);

    const Translation& translation() const;
    Transform& setTranslation(const Translation& translation);
    Transform& translate(const Translation& translation);

    const Scaling& scaling() const;
    Transform& setScale(const Scaling& scaling);
    Transform& scale(const Scalar& scaling);
    Transform& scale(const Scaling& scaling);

    Transform& invert();
    Transform inverse();

    //operators for value manipulation
    //********************************

    Transform& operator=(const Translation& t);
    Transform operator*(const Translation& s) const;
    Transform& operator*=(const Translation& t);

    Transform& operator=(const Scaling& s);
    Transform operator*(const Scaling& s) const;
    Transform& operator*=(const Scaling& s);

    template<typename Derived>
    Transform& operator=(const Eigen::RotationBase<Derived,Dim>& r);
    template<typename Derived>
    Transform operator*(const Eigen::RotationBase<Derived,Dim>& r) const;
    template<typename Derived>
    Transform& operator*=(const Eigen::RotationBase<Derived,Dim>& r);

    Transform operator* (const Transform& other) const;
    Transform& operator*= (const Transform& other);

    //transform Vectors
    //*****************
    template<typename Derived>
    Derived& rotate(Eigen::MatrixBase<Derived>& vec) const;
    template<typename Derived>
    Derived& translate(Eigen::MatrixBase<Derived>& vec) const;
    template<typename Derived>
    Derived& scale(Eigen::MatrixBase<Derived>& vec) const;
    template<typename Derived>
    Derived& transform(Eigen::MatrixBase<Derived>& vec) const;
    template<typename Derived>
    Derived operator*(const Eigen::MatrixBase<Derived>& vec) const;
    template<typename Derived>
    void operator()(Eigen::MatrixBase<Derived>& vec) const;

    //Stuff
    //*****
    bool isApprox(const Transform& other, Scalar prec) const;
    void setIdentity();
    static const Transform Identity();

    Transform& normalize();

public:
	EIGEN_MAKE_ALIGNED_OPERATOR_NEW
};

template<typename Scalar, int Dim>
class DiffTransform : public Transform<Scalar, Dim> {

    typedef typename Transform<Scalar, Dim>::Rotation Rotation;
    typedef typename Transform<Scalar, Dim>::Translation Translation;
    typedef typename Transform<Scalar, Dim>::Scaling Scaling;
    typedef Eigen::Matrix<Scalar, Dim, 3*Dim> DiffMatrix;

    DiffMatrix m_diffMatrix;

public:
    DiffTransform() : Transform<Scalar, Dim>() { };
    DiffTransform(const Rotation& r) : Transform<Scalar, Dim>(r) {};
    DiffTransform(const Rotation& r, const Translation& t) : Transform<Scalar, Dim>(r,t) {};
    DiffTransform(const Rotation& r, const Translation& t, const Scaling& s) : Transform<Scalar, Dim>(r,t,s) {};
   
    DiffTransform(Transform<Scalar, Dim>& trans);

    const DiffMatrix& differential();
    Scalar& operator()(int f, int s);
    Scalar& at(int f, int s);
};

/*When you overload a binary operator as a member function of a class the overload is used
 * when the first operand is of the class type.For stream operators, the first operand
 * is the stream and not (usually) the custom class.
*/
template<typename charT, typename traits, typename Kernel, int Dim>
std::basic_ostream<charT,traits>& operator<<(std::basic_ostream<charT,traits>& os, const dcm::detail::Transform<Kernel, Dim>& t);

template<typename charT, typename traits,typename Kernel, int Dim>
std::basic_ostream<charT,traits>& operator<<(std::basic_ostream<charT,traits>& os, dcm::detail::DiffTransform<Kernel, Dim>& t);


}//detail
}//DCM

//#ifndef DCM_EXTERNAL_CORE
#include "imp/transformation_imp.hpp"
//#endif


#endif //DCM_TRANSFORMATION
