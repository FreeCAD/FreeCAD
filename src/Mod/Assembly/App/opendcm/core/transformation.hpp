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
    Transform() : m_rotation(Rotation::Identity()),
        m_translation(Translation::Identity()),
        m_scale(Scaling(1.)) { };

    Transform(const Rotation& r) : m_rotation(r),
        m_translation(Translation::Identity()),
        m_scale(Scaling(1.)) {
        m_rotation.normalize();
    };

    Transform(const Rotation& r, const Translation& t) : m_rotation(r),
        m_translation(t),
        m_scale(Scaling(1.)) {
        m_rotation.normalize();
    };

    Transform(const Rotation& r, const Translation& t, const Scaling& s) : m_rotation(r),
        m_translation(t),
        m_scale(s) {
        m_rotation.normalize();
    };

    //access the single parts and manipulate them
    //***********************
    const Rotation& rotation() const {
        return m_rotation;
    }
    template<typename Derived>
    Transform& rotate(const Eigen::RotationBase<Derived,Dim>& rotation) {
        m_rotation = rotation.derived().normalized()*m_rotation;
        return *this;
    }

    const Translation& translation() const {
        return m_translation;
    }
    Transform& translate(const Translation& translation) {
        m_translation = m_translation*translation;
        return *this;
    }

    const Scaling& scaling() const {
        return m_scale;
    }
    Transform& scale(const Scalar& scaling) {
        m_scale *= Scaling(scaling);
        return *this;
    }
    Transform& scale(const Scaling& scaling) {
        m_scale.factor() *= scaling.factor();
        return *this;
    }

    Transform& invert() {
        m_rotation = m_rotation.inverse();
        m_translation.vector() = (m_rotation*m_translation.vector()) * (-m_scale.factor());
        m_scale = Scaling(1./m_scale.factor());
        return *this;
    };
    Transform inverse() {
        Transform res(*this);
        res.invert();
        return res;
    };

    //operators for value manipulation
    //********************************

    inline Transform& operator=(const Translation& t) {
        m_translation = t;
        m_rotation = Rotation::Identity();
        m_scale = Scaling(1.);
        return *this;
    }
    inline Transform operator*(const Translation& t) const {
        Transform res = *this;
        res.translate(t);
        return res;
    }
    inline Transform& operator*=(const Translation& t) {
        return translate(t);
    }

    inline Transform& operator=(const Scaling& s) {
        m_scale = s;
        m_translation = Translation::Identity();
        m_rotation = Rotation::Identity();
        return *this;
    }
    inline Transform operator*(const Scaling& s) const {
        Transform res = *this;
        res.scale(s);
        return res;
    }
    inline Transform& operator*=(const Scaling& s) {
        return scale(s);
    }

    template<typename Derived>
    inline Transform& operator=(const Eigen::RotationBase<Derived,Dim>& r) {
        m_rotation = r.derived();
        m_rotation.normalize();
        m_translation = Translation::Identity();
        m_scale = Scaling(1);
        return *this;
    }
    template<typename Derived>
    inline Transform operator*(const Eigen::RotationBase<Derived,Dim>& r) const {
        Transform res = *this;
        res.rotate(r.derived());
        return res;
    }
    template<typename Derived>
    inline Transform& operator*=(const Eigen::RotationBase<Derived,Dim>& r) {
        return rotate(r.derived());
    }

    inline Transform operator* (const Transform& other) const  {
        Transform res(*this);
        res*= other;
        return res;
    }
    inline Transform& operator*= (const Transform& other) {
        rotate(other.rotation());
        other.rotate(m_translation.vector());
        m_translation.vector() += other.translation().vector()/m_scale.factor();
        m_scale.factor() *= other.scaling().factor();
        return *this;
    }

    //transform Vectors
    //*****************
    template<typename Derived>
    inline Derived& rotate(Eigen::MatrixBase<Derived>& vec) const {
        vec = m_rotation*vec;
        return vec.derived();
    }
    template<typename Derived>
    inline Derived& translate(Eigen::MatrixBase<Derived>& vec) const {
        vec = m_translation*vec;
        return vec.derived();
    }
    template<typename Derived>
    inline Derived& scale(Eigen::MatrixBase<Derived>& vec) const {
        vec*=m_scale.factor();
        return vec.derived();
    }
    template<typename Derived>
    inline Derived& transform(Eigen::MatrixBase<Derived>& vec) const {
        vec = (m_rotation*vec + m_translation.vector())*m_scale.factor();
        return vec.derived();
    }
    template<typename Derived>
    inline Derived operator*(const Eigen::MatrixBase<Derived>& vec) const {
        return (m_rotation*vec + m_translation.vector())*m_scale.factor();
    }
    template<typename Derived>
    inline void operator()(Eigen::MatrixBase<Derived>& vec) const {
        transform(vec);
    }

    //Stuff
    //*****
    bool isApprox(const Transform& other, Scalar prec) const {
        return m_rotation.isApprox(other.rotation(), prec)
               && ((m_translation.vector()- other.translation().vector()).norm() < prec)
               && (std::abs(m_scale.factor()-other.scaling().factor()) < prec);
    };
    void setIdentity() {
        m_rotation.setIdentity();
        m_translation = Translation::Identity();
        m_scale = Scaling(1.);
    }
    static const Transform Identity() {
        return Transform(Rotation::Identity(), Translation::Identity(), Scaling(1));
    }

    Transform& normalize() {
        m_rotation.normalize();
        return *this;
    }

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
   
    DiffTransform(Transform<Scalar, Dim>& trans)
        : Transform<Scalar, Dim>(trans.rotation(), trans.translation(), trans.scaling()) {

        m_diffMatrix.setZero();
    };

    const DiffMatrix& differential() {
        return m_diffMatrix;
    };
    inline Scalar& operator()(int f, int s) {
        return m_diffMatrix(f,s);
    };
    inline Scalar& at(int f, int s) {
        return m_diffMatrix(f,s);
    };
};

/*When you overload a binary operator as a member function of a class the overload is used
 * when the first operand is of the class type.For stream operators, the first operand
 * is the stream and not (usually) the custom class.
*/
template<typename charT, typename traits, typename Kernel, int Dim>
std::basic_ostream<charT,traits>& operator<<(std::basic_ostream<charT,traits>& os, const dcm::detail::Transform<Kernel, Dim>& t) {
    os << "Rotation:    " << t.rotation().coeffs().transpose() << std::endl
       << "Translation: " << t.translation().vector().transpose() <<std::endl
       << "Scale:       " << t.scaling().factor();
    return os;
}

template<typename charT, typename traits,typename Kernel, int Dim>
std::basic_ostream<charT,traits>& operator<<(std::basic_ostream<charT,traits>& os, dcm::detail::DiffTransform<Kernel, Dim>& t) {
    os << "Rotation:    " << t.rotation().coeffs().transpose() << std::endl
       << "Translation: " << t.translation().vector().transpose() <<std::endl
       << "Scale:       " << t.scaling().factor() << std::endl
       << "Differential:" << std::endl<<t.differential();
    return os;
}

}//detail
}//DCM




#endif //DCM_TRANSFORMATION
