/*
    openDCM, dimensional constraint manager
    Copyright (C) 2012  Stefan Troeger <stefantroeger@gmx.net>

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

#ifndef DCM_KERNEL_H
#define DCM_KERNEL_H

#include <Eigen/Core>
#include <Eigen/Dense>
#include <Eigen/Geometry>

#include "transformation.hpp"
#include "logging.hpp"
#include "defines.hpp"
#include "property.hpp"

namespace E = Eigen;
namespace mpl= boost::mpl;

namespace dcm {

struct nothing {
    void operator()() {};
};

//information about solving
struct SolverInfo {
    int iterations;
    double error;
    double time;
};

//the parameter types
enum AccessType {
    general,  //every non-rotation parameter, therefore every translation and non transformed parameter
    rotation, //all rotation parameters
    complete  //all parameter
};

//solver settings
struct precision {

    typedef double type;
    typedef setting_property kind;
    struct default_value {
        double operator()() {
            return 1e-6;
        };
    };
};

struct iterations {

    typedef int type;
    typedef setting_property kind;
    struct default_value {
        int operator()() {
            return int(5e3);
        };
    };
};

//and the solver itself
template<typename Kernel>
struct Dogleg {

#ifdef USE_LOGGING
    dcm_logger log;
#endif

    typedef typename Kernel::number_type number_type;
    number_type tolg, tolx, delta, nu, g_inf, fx_inf, err, time;
    Kernel* m_kernel;
    int iter, stop, reduce, unused, counter;
    typename Kernel::Vector h_dl, F_old, g;
    typename Kernel::Matrix J_old;

    Dogleg(Kernel* k);
    Dogleg();

    void setKernel(Kernel* k);

    template <typename Derived, typename Derived2, typename Derived3, typename Derived4>
    void calculateStep(const Eigen::MatrixBase<Derived>& g, const Eigen::MatrixBase<Derived3>& jacobi,
                       const Eigen::MatrixBase<Derived4>& residual, Eigen::MatrixBase<Derived2>& h_dl,
                       const double delta);

    int solve(typename Kernel::MappedEquationSystem& sys);

    template<typename Functor>
    int solve(typename Kernel::MappedEquationSystem& sys, Functor& rescale);
};

template<typename Scalar, template<class> class Nonlinear = Dogleg>
struct Kernel : public PropertyOwner< mpl::vector2<precision, iterations> > {

    //basics
    typedef Scalar number_type;

    //linear algebra types 2D
    typedef E::Matrix<Scalar, 2, 1> Vector2;

    //Linear algebra types 3D
    typedef E::Matrix<Scalar, 3, 1> Vector3;
    typedef E::Matrix<Scalar, 1, 3> CVector3;
    typedef E::Matrix<Scalar, 3, 3> Matrix3;
    typedef E::Matrix<Scalar, E::Dynamic, 1> Vector;
    typedef E::Matrix<Scalar, 1, E::Dynamic> CVector;
    typedef E::Matrix<Scalar, E::Dynamic, E::Dynamic> Matrix;

    //mapped types
    typedef E::Stride<E::Dynamic, E::Dynamic> DynStride;
    typedef E::Map< Vector3 > Vector3Map;
    typedef E::Map< CVector3> CVector3Map;
    typedef E::Map< Matrix3 > Matrix3Map;
    typedef E::Map< Vector, 0, DynStride > VectorMap;
    typedef E::Map< CVector, 0, DynStride > CVectorMap;
    typedef E::Map< Matrix, 0, DynStride > MatrixMap;

    //Special types
    typedef E::Quaternion<Scalar>   Quaternion;
    typedef E::Matrix<Scalar, 3, 9> Matrix39;
    typedef E::Map< Matrix39 >      Matrix39Map;
    typedef E::Block<Matrix>	    MatrixBlock;

    typedef detail::Transform<Scalar, 3> 	Transform3D;
    typedef detail::DiffTransform<Scalar, 3> 	DiffTransform3D;

    typedef detail::Transform<Scalar, 2> 	Transform2D;
    typedef detail::DiffTransform<Scalar, 2> 	DiffTransform2D;

    typedef Nonlinear< Kernel<Scalar, Nonlinear> > NonlinearSolver;

    template<int Dim>
    struct transform_type {
        typedef typename boost::mpl::if_c<Dim==2, Transform2D, Transform3D>::type type;
        typedef typename boost::mpl::if_c<Dim==2, DiffTransform2D, DiffTransform3D>::type diff_type;
    };

    template<int Dim>
    struct vector_type {
        typedef E::Matrix<Scalar, Dim, 1> type;
    };

    struct MappedEquationSystem {

    //protected:
        Matrix m_jacobi;
        Vector m_parameter, m_residual;

        AccessType m_access; //which parameters/equation shall be calculated?
        int m_params, m_eqns; //total amount
        int m_param_rot_offset, m_param_trans_offset, m_eqn_rot_offset, m_eqn_trans_offset;   //current positions while creation

    public:
        MatrixMap Jacobi;
        VectorMap Parameter;
        VectorMap Residual;

        number_type Scaling;

        int parameterCount();
        int equationCount();
        AccessType access();

        MappedEquationSystem(int params, int equations);

        int setParameterMap(int number, VectorMap& map, AccessType t = general);
        int setParameterMap(Vector3Map& map, AccessType t = general);
        int setResidualMap(VectorMap& map, AccessType t = general);
        void setJacobiMap(int eqn, int offset, int number, CVectorMap& map);
        void setJacobiMap(int eqn, int offset, int number, VectorMap& map);

        bool isValid();

        void setAccess(AccessType t);
        bool hasAccessType(AccessType t);

        virtual void recalculate() = 0;
        virtual void removeLocalGradientZeros() = 0;
    };


    Kernel();

    //static comparison versions
    template <typename DerivedA,typename DerivedB>
    static bool isSame(const E::MatrixBase<DerivedA>& p1,const E::MatrixBase<DerivedB>& p2, number_type precission);
    static bool isSame(number_type t1, number_type t2, number_type precission);
    template <typename DerivedA,typename DerivedB>
    static bool isOpposite(const E::MatrixBase<DerivedA>& p1,const E::MatrixBase<DerivedB>& p2, number_type precission);

    //runtime comparison versions (which use user settings for precission)
    template <typename DerivedA,typename DerivedB>
    bool isSame(const E::MatrixBase<DerivedA>& p1,const E::MatrixBase<DerivedB>& p2);
    bool isSame(number_type t1, number_type t2);
    template <typename DerivedA,typename DerivedB>
    bool isOpposite(const E::MatrixBase<DerivedA>& p1,const E::MatrixBase<DerivedB>& p2);

    int solve(MappedEquationSystem& mes);

    template<typename Functor>
    int solve(MappedEquationSystem& mes, Functor& f);

    SolverInfo getSolverInfo();

private:
    NonlinearSolver m_solver;

};


}//dcm

#ifndef DCM_EXTERNAL_CORE
#include "imp/kernel_imp.hpp"
#endif

#endif //GCM_KERNEL_H






