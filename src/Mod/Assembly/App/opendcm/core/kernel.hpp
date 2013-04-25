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

#ifndef GCM_KERNEL_H
#define GCM_KERNEL_H

#include <Eigen/Core>
#include <Eigen/Dense>
#include <Eigen/Geometry>

#include <iostream>

#include <boost/math/special_functions/fpclassify.hpp>
#include <time.h>

#include "transformation.hpp"
#include "logging.hpp"


namespace dcm {

namespace E = Eigen;

struct nothing {
    void operator()() {};
};

template<typename Kernel>
struct Dogleg {

#ifdef USE_LOGGING
    src::logger log;
#endif

    typedef typename Kernel::number_type number_type;
    number_type tolg, tolx, tolf;

    Dogleg() : tolg(1e-40), tolx(1e-20), tolf(1e-5) {

#ifdef USE_LOGGING
        log.add_attribute("Tag", attrs::constant< std::string >("Dogleg"));
#endif
    };

    template <typename Derived, typename Derived2, typename Derived3, typename Derived4>
    int calculateStep(const Eigen::MatrixBase<Derived>& g, const Eigen::MatrixBase<Derived3>& jacobi,
                      const Eigen::MatrixBase<Derived4>& residual, Eigen::MatrixBase<Derived2>& h_dl,
                      const double delta) {

        // get the steepest descent stepsize and direction
        const double alpha(g.squaredNorm()/(jacobi*g).squaredNorm());
        const typename Kernel::Vector h_sd  = -g;

        // get the gauss-newton step
        const typename Kernel::Vector h_gn = (jacobi).fullPivLu().solve(-residual);
#ifdef USE_LOGGING
        if(!boost::math::isfinite(h_gn.norm())) {
            BOOST_LOG(log)<< "Unnormal gauss-newton detected: "<<h_gn.norm();
        }
        if(!boost::math::isfinite(h_sd.norm())) {
            BOOST_LOG(log)<< "Unnormal steepest descent detected: "<<h_sd.norm();
        }
        if(!boost::math::isfinite(alpha)) {
            BOOST_LOG(log)<< "Unnormal alpha detected: "<<alpha;
        }
#endif

        // compute the dogleg step
        if(h_gn.norm() <= delta) {
            h_dl = h_gn;
        } else if((alpha*h_sd).norm() >= delta) {
            //h_dl = alpha*h_sd;
            h_dl = (delta/(h_sd.norm()))*h_sd;
#ifdef USE_LOGGING
            if(!boost::math::isfinite(h_dl.norm())) {
                BOOST_LOG(log)<< "Unnormal dogleg descent detected: "<<h_dl.norm();
            }
#endif
        } else {
            //compute beta
            number_type beta = 0;
            typename Kernel::Vector a = alpha*h_sd;
            typename Kernel::Vector b = h_gn;
            number_type c = a.transpose()*(b-a);
            number_type bas = (b-a).squaredNorm(), as = a.squaredNorm();
            if(c<0) {
                beta = -c+std::sqrt(std::pow(c,2)+bas*(std::pow(delta,2)-as));
                beta /= bas;
            } else {
                beta = std::pow(delta,2)-as;
                beta /= c+std::sqrt(std::pow(c,2) + bas*(std::pow(delta,2)-as));
            };

            // and update h_dl and dL with beta
            h_dl = alpha*h_sd + beta*(b-a);

#ifdef USE_LOGGING
            if(!boost::math::isfinite(c)) {
                BOOST_LOG(log)<< "Unnormal dogleg c detected: "<<c;
            }
            if(!boost::math::isfinite(bas)) {
                BOOST_LOG(log)<< "Unnormal dogleg bas detected: "<<bas;
            }
            if(!boost::math::isfinite(beta)) {
                BOOST_LOG(log)<< "Unnormal dogleg beta detected: "<<beta;
            }
#endif
        }

        return 0;
    };

    int solve(typename Kernel::MappedEquationSystem& sys)  {
        nothing n;
        return solve(sys, n);
    };

    template<typename Functor>
    int solve(typename Kernel::MappedEquationSystem& sys, Functor& rescale) {

        clock_t start = clock();
        clock_t inc_rec = clock();

        if(!sys.isValid()) return 5;

        bool translate = true;

        typename Kernel::Vector h_dl, F_old(sys.m_eqns), g(sys.m_eqns);
        typename Kernel::Matrix J_old(sys.m_eqns, sys.m_params);

        sys.recalculate();

#ifdef USE_LOGGING
        BOOST_LOG(log)<< "initial jacobi: "<<std::endl<<sys.Jacobi<<std::endl
                      << "residual: "<<sys.Residual.transpose()<<std::endl
                      << "maximal differential: "<<sys.Jacobi.template lpNorm<Eigen::Infinity>();
#endif

        number_type err = sys.Residual.norm();

        F_old = sys.Residual;
        J_old = sys.Jacobi;

        g = sys.Jacobi.transpose()*(sys.Residual);

        // get the infinity norm fx_inf and g_inf
        number_type g_inf = g.template lpNorm<E::Infinity>();
        number_type fx_inf = sys.Residual.template lpNorm<E::Infinity>();

        int maxIterNumber = 10000;//MaxIterations * xsize;
        number_type diverging_lim = 1e6*err + 1e12;

        number_type delta=5;
        number_type nu=2.;
        int iter=0, stop=0, reduce=0, unused=0, counter=0;


        while(!stop) {

            // check if finished
            if(fx_inf <= tolf*sys.Scaling)  // Success
                stop = 1;
            else if(g_inf <= tolg)
                stop = 2;
            else if(delta <= tolx)
                stop = 3;
            else if(iter >= maxIterNumber)
                stop = 4;
            else if(!boost::math::isfinite(err))
                stop = 5;
            else if(err > diverging_lim)
                stop = 6;


            // see if we are already finished
            if(stop)
                break;

            number_type err_new;
            number_type dF=0, dL=0;
            number_type rho;

            //get the update step
            calculateStep(g, sys.Jacobi,  sys.Residual, h_dl, delta);

            // calculate the linear model
            dL = 0.5*sys.Residual.norm() - 0.5*(sys.Residual + sys.Jacobi*h_dl).norm();

            // get the new values
            sys.Parameter += h_dl;

            clock_t start_rec = clock();
            sys.recalculate();
            clock_t end_rec = clock();
            inc_rec += end_rec-start_rec;

#ifdef USE_LOGGING
            if(!boost::math::isfinite(sys.Residual.norm())) {
                BOOST_LOG(log)<< "Unnormal residual detected: "<<sys.Residual.norm();
            }
            if(!boost::math::isfinite(sys.Jacobi.sum())) {
                BOOST_LOG(log)<< "Unnormal jacobi detected: "<<sys.Jacobi.sum();
            }
#endif

            //calculate the translation update ratio
            err_new = sys.Residual.norm();
            dF = err - err_new;
            rho = dF/dL;

            if(dF<=0 || dL<=0)  rho = -1;
            // update delta
            if(rho>0.85) {
                delta = std::max(delta,2*h_dl.norm());
                nu = 2;
            } else if(rho < 0.25) {
                delta = delta/nu;
                nu = 2*nu;
            }

            if(dF > 0 && dL > 0) {

                //see if we got too high differentials
                if(sys.Jacobi.template lpNorm<Eigen::Infinity>() > 2) {
#ifdef USE_LOGGING
                    BOOST_LOG(log)<< "High differential detected: "<<sys.Jacobi.template lpNorm<Eigen::Infinity>()<<" in iteration: "<<iter;
#endif
                    rescale();
                    sys.recalculate();
                } 
                //it can also happen that the differentials get too small, however, we cant check for that
                else if(iter>1 && (counter>50)) {
                    rescale();
                    sys.recalculate();
                    counter = 0;
                }

                F_old = sys.Residual;
                J_old = sys.Jacobi;

                err = sys.Residual.norm();
                g = sys.Jacobi.transpose()*(sys.Residual);

                // get infinity norms
                g_inf = g.template lpNorm<E::Infinity>();
                fx_inf = sys.Residual.template lpNorm<E::Infinity>();

            } else {
                sys.Residual = F_old;
                sys.Jacobi = J_old;
                sys.Parameter -= h_dl;
                unused++;
#ifdef USE_LOGGING
                BOOST_LOG(log)<< "Reject step in iter "<<iter;
#endif
            }

            iter++;
            counter++;
        }
        /*
                clock_t end = clock();
                double ms = (double(end-start) * 1000.) / double(CLOCKS_PER_SEC);
                double ms_rec = (double(inc_rec-start) * 1000.) / double(CLOCKS_PER_SEC);
        */
#ifdef USE_LOGGING
        BOOST_LOG(log) <<"Done solving: "<<err<<", iter: "<<iter<<", unused: "<<unused<<", reason:"<< stop;
        BOOST_LOG(log)<< "final jacobi: "<<std::endl<<sys.Jacobi;
#endif

        return stop;
    }
};

template<typename Scalar, template<class> class Nonlinear = Dogleg>
struct Kernel {

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

        Matrix Jacobi;
        Vector Parameter;
        Vector Residual;
        number_type Scaling;
        int m_params, m_eqns; //total amount
        int m_param_offset, m_eqn_offset;   //current positions while creation

        MappedEquationSystem(int params, int equations)
            : Jacobi(equations, params),
              Parameter(params), Residual(equations),
              m_params(params), m_eqns(equations), Scaling(1.) {

            m_param_offset = 0;
            m_eqn_offset = 0;

            Jacobi.setZero(); //important as some places are never written
        };

        int setParameterMap(int number, VectorMap& map) {

            new(&map) VectorMap(&Parameter(m_param_offset), number, DynStride(1,1));
            m_param_offset += number;
            return m_param_offset-number;
        };
        int setParameterMap(Vector3Map& map) {

            new(&map) Vector3Map(&Parameter(m_param_offset));
            m_param_offset += 3;
            return m_param_offset-3;
        };
        int setResidualMap(VectorMap& map) {
            new(&map) VectorMap(&Residual(m_eqn_offset), 1, DynStride(1,1));
            return m_eqn_offset++;
        };
        void setJacobiMap(int eqn, int offset, int number, CVectorMap& map) {
            new(&map) CVectorMap(&Jacobi(eqn, offset), number, DynStride(0,m_eqns));
        };
        void setJacobiMap(int eqn, int offset, int number, VectorMap& map) {
            new(&map) VectorMap(&Jacobi(eqn, offset), number, DynStride(0,m_eqns));
        };

        bool isValid() {
            if(!m_params || !m_eqns) return false;
            return true;
        };

        virtual void recalculate() = 0;

    };

    Kernel()  {};

    template <typename DerivedA,typename DerivedB>
    static bool isSame(const E::MatrixBase<DerivedA>& p1,const E::MatrixBase<DerivedB>& p2) {
        return ((p1-p2).squaredNorm() < 0.00001);
    }
    static bool isSame(number_type t1, number_type t2) {
        return (std::abs(t1-t2) < 0.00001);
    }
    template <typename DerivedA,typename DerivedB>
    static bool isOpposite(const E::MatrixBase<DerivedA>& p1,const E::MatrixBase<DerivedB>& p2) {
        return ((p1+p2).squaredNorm() < 0.00001);
    }

    static int solve(MappedEquationSystem& mes) {
        nothing n;
        return Nonlinear< Kernel<Scalar, Nonlinear> >().solve(mes, n);
    };

    template<typename Functor>
    static int solve(MappedEquationSystem& mes, Functor& f) {
        return Nonlinear< Kernel<Scalar, Nonlinear> >().solve(mes, f);
    };

};

}

#endif //GCM_KERNEL_H


