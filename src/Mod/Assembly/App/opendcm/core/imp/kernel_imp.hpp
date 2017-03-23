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

#ifndef DCM_KERNEL_IMP_H
#define DCM_KERNEL_IMP_H

#include "../kernel.hpp"
#include <boost/math/special_functions.hpp>
#include <Eigen/QR>

namespace dcm {

template<typename Kernel>
Dogleg<Kernel>::Dogleg(Kernel* k) : m_kernel(k), tolg(1e-40), tolx(1e-20) {

#ifdef USE_LOGGING
    log.add_attribute("Tag", attrs::constant< std::string >("Dogleg"));
#endif
};

template<typename Kernel>
Dogleg<Kernel>::Dogleg() : tolg(1e-6), tolx(1e-3) {

#ifdef USE_LOGGING
    log.add_attribute("Tag", attrs::constant< std::string >("Dogleg"));
#endif
};

template<typename Kernel>
void Dogleg<Kernel>::setKernel(Kernel* k) {

    m_kernel = k;
};

template<typename Kernel>
template <typename Derived, typename Derived2, typename Derived3, typename Derived4>
void Dogleg<Kernel>::calculateStep(const Eigen::MatrixBase<Derived>& g, const Eigen::MatrixBase<Derived3>& jacobi,
                                   const Eigen::MatrixBase<Derived4>& residual, Eigen::MatrixBase<Derived2>& h_dl,
                                   const double delta) {

    // get the steepest descent stepsize and direction
    const double alpha(g.squaredNorm()/(jacobi*g).squaredNorm());
    const typename Kernel::Vector h_sd  = -g;

    // get the gauss-newton step
    const typename Kernel::Vector h_gn = jacobi.fullPivLu().solve(-residual);
    const double eigen_error = (jacobi*h_gn + residual).norm();
#ifdef USE_LOGGING

    if(!boost::math::isfinite(h_gn.norm())) {
        BOOST_LOG_SEV(log, error) << "Unnormal gauss-newton detected: "<<h_gn.norm();
    }

    if(!boost::math::isfinite(h_sd.norm())) {
        BOOST_LOG_SEV(log, error) << "Unnormal steepest descent detected: "<<h_sd.norm();
    }

    if(!boost::math::isfinite(alpha)) {
        BOOST_LOG_SEV(log, error) << "Unnormal alpha detected: "<<alpha;
    }

#endif

    // compute the dogleg step
    if(h_gn.norm() <= delta) {
        h_dl = h_gn;
    }
    else if((alpha*h_sd).norm() >= delta) {
        //h_dl = alpha*h_sd;
        h_dl = (delta/(h_sd.norm()))*h_sd;
#ifdef USE_LOGGING

        if(!boost::math::isfinite(h_dl.norm())) {
            BOOST_LOG_SEV(log, error) << "Unnormal dogleg descent detected: "<<h_dl.norm();
        }

#endif
    }
    else {
        //compute beta
        number_type beta = 0;
        typename Kernel::Vector a = alpha*h_sd;
        typename Kernel::Vector b = h_gn;
        number_type c = a.transpose()*(b-a);
        number_type bas = (b-a).squaredNorm(), as = a.squaredNorm();

        if(c<0) {
            beta = -c+std::sqrt(std::pow(c,2)+bas*(std::pow(delta,2)-as));
            beta /= bas;
        }
        else {
            beta = std::pow(delta,2)-as;
            beta /= c+std::sqrt(std::pow(c,2) + bas*(std::pow(delta,2)-as));
        };

        // and update h_dl and dL with beta
        h_dl = alpha*h_sd + beta*(b-a);

#ifdef USE_LOGGING
        if(!boost::math::isfinite(c)) {
            BOOST_LOG_SEV(log, error) << "Unnormal dogleg c detected: "<<c;
        }

        if(!boost::math::isfinite(bas)) {
            BOOST_LOG_SEV(log, error) << "Unnormal dogleg bas detected: "<<bas;
        }

        if(!boost::math::isfinite(beta)) {
            BOOST_LOG_SEV(log, error) << "Unnormal dogleg beta detected: "<<beta;
        }

#endif
    }
};

template<typename Kernel>
int Dogleg<Kernel>::solve(typename Kernel::MappedEquationSystem& sys)  {
    nothing n;
    return solve(sys, n);
};

template<typename Kernel>
template<typename Functor>
int Dogleg<Kernel>::solve(typename Kernel::MappedEquationSystem& sys, Functor& rescale) {

    clock_t start = clock();

    if(!sys.isValid())
        throw solving_error() <<  boost::errinfo_errno(5) << error_message("invalid equation system");

    F_old.resize(sys.equationCount());
    g.resize(sys.equationCount());
    J_old.resize(sys.equationCount(), sys.parameterCount());

    sys.recalculate();
#ifdef USE_LOGGING
    BOOST_LOG_SEV(log, solving) << "initial jacobi: "<<std::endl<<sys.Jacobi<<std::endl
                                << "residual: "<<sys.Residual.transpose()<<std::endl
                                << "maximal differential: "<<sys.Jacobi.template lpNorm<Eigen::Infinity>();
#endif
    sys.removeLocalGradientZeros();

#ifdef USE_LOGGING
    BOOST_LOG_SEV(log, solving) << "LGZ jacobi: "<<std::endl<<sys.Jacobi<<std::endl
                                << "maximal differential: "<<sys.Jacobi.template lpNorm<Eigen::Infinity>();
#endif

    err = sys.Residual.norm();

    F_old = sys.Residual;
    J_old = sys.Jacobi;

    g = sys.Jacobi.transpose()*(sys.Residual);

    // get the infinity norm fx_inf and g_inf
    g_inf = g.template lpNorm<E::Infinity>();
    fx_inf = sys.Residual.template lpNorm<E::Infinity>();

    delta=5;
    nu=2.;
    iter=0;
    stop=0;
    reduce=0;
    unused=0;
    counter=0;

    int maxIterNumber = m_kernel->template getProperty<iterations>();
    number_type pr = m_kernel->template getProperty<precision>()*sys.Scaling;
    number_type diverging_lim = 1e6*err + 1e12;

    do {

        // check if finished
        if(fx_inf <= pr)  // Success
            stop = 1;
        else if(g_inf <= tolg*pr)
            throw solving_error() <<  boost::errinfo_errno(2) << error_message("g infinity norm smaller below limit");
        else if(delta <= tolx*pr)
            throw solving_error() <<  boost::errinfo_errno(3) << error_message("step size below limit");
        else if(iter >= maxIterNumber)
            throw solving_error() <<  boost::errinfo_errno(4) << error_message("maximal iterations reached");
        else if(!boost::math::isfinite(err))
            throw solving_error() <<  boost::errinfo_errno(5) << error_message("error is inf or nan");
        else if(err > diverging_lim)
            throw solving_error() <<  boost::errinfo_errno(6) << error_message("error diverged");


        // see if we are already finished
        if(stop)
            break;

        number_type err_new;
        number_type dF=0, dL=0;
        number_type rho;

        //get the update step
        calculateStep(g, sys.Jacobi, sys.Residual, h_dl, delta);

#ifdef USE_LOGGING
        BOOST_LOG_SEV(log, iteration) << "Step in iter "<<iter<<std::endl
                                      << "Step: "<<h_dl.transpose()<<std::endl
                                      << "Jacobi: "<<sys.Jacobi<<std::endl
                                      << "Residual: "<<sys.Residual.transpose();
#endif

        // calculate the linear model
        dL = sys.Residual.norm() - (sys.Residual + sys.Jacobi*h_dl).norm();

        // get the new values
        sys.Parameter += h_dl;
        sys.recalculate();

#ifdef USE_LOGGING

        if(!boost::math::isfinite(sys.Residual.norm())) {
            BOOST_LOG_SEV(log, error) << "Unnormal residual detected: "<<sys.Residual.norm();
        }

        if(!boost::math::isfinite(sys.Jacobi.sum())) {
            BOOST_LOG_SEV(log, error) << "Unnormal jacobi detected: "<<sys.Jacobi.sum();
        }

#endif

        //calculate the translation update ratio
        err_new = sys.Residual.norm();
        dF = err - err_new;
        rho = dF/dL;

        if(dF<=0 || dL<=0)
            rho = -1;

        // update delta
        if(rho>0.85) {
            delta = std::max(delta,2*h_dl.norm());
            nu = 2;
        }
        else if(rho < 0.25) {
            delta = delta/nu;
            nu = 2*nu;
        }

#ifdef USE_LOGGING
        BOOST_LOG_SEV(log, iteration)<<"Result of step dF: "<<dF<<", dL: "<<dL<<std::endl
                                     << "New Residual: "<< sys.Residual.transpose()<<std::endl;
#endif

        if(dF > 0 && dL > 0) {

            //see if we got too high differentials
            if(sys.Jacobi.template lpNorm<Eigen::Infinity>() > 2) {
#ifdef USE_LOGGING
                BOOST_LOG_SEV(log, iteration)<< "High differential detected: "<<sys.Jacobi.template lpNorm<Eigen::Infinity>()<<" in iteration: "<<iter;
#endif
                rescale();
                sys.recalculate();
            }
            //it can also happen that the differentials get too small, however, we can't check for that
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
        }
        else {
#ifdef USE_LOGGING
            BOOST_LOG_SEV(log, iteration)<< "Reject step in iter "<<iter<<", dF: "<<dF<<", dL: "<<dL;
#endif
            sys.Residual = F_old;
            sys.Jacobi = J_old;
            sys.Parameter -= h_dl;
            unused++;
        }

        iter++;
        counter++;
    }
    while(!stop);


    clock_t end = clock();
    time = (double(end-start) * 1000.) / double(CLOCKS_PER_SEC);

#ifdef USE_LOGGING
    BOOST_LOG_SEV(log, solving)<<"Done solving: "<<err<<", iter: "<<iter<<", unused: "<<unused<<", reason:"<< stop;
    BOOST_LOG_SEV(log, solving)<< "final jacobi: "<<std::endl<<sys.Jacobi
                               << "residual: "<<sys.Residual.transpose()<<std::endl
                               << "maximal differential: "<<sys.Jacobi.template lpNorm<Eigen::Infinity>();
#endif

    return stop;
}


template<typename Scalar, template<class> class Nonlinear>
int Kernel<Scalar, Nonlinear>::MappedEquationSystem::parameterCount() {
    return m_params;
};

template<typename Scalar, template<class> class Nonlinear>
int Kernel<Scalar, Nonlinear>::MappedEquationSystem::equationCount() {
    return m_eqns;
};

template<typename Scalar, template<class> class Nonlinear>
AccessType Kernel<Scalar, Nonlinear>::MappedEquationSystem::access() {
    return m_access;
};

template<typename Scalar, template<class> class Nonlinear>
Kernel<Scalar, Nonlinear>::MappedEquationSystem::MappedEquationSystem(int params, int equations)
    : m_access(general), m_jacobi(equations, params),
      m_parameter(params), m_residual(equations),
      m_params(params), m_eqns(equations), Scaling(1.),
      Jacobi(&m_jacobi(0,0),equations,params,DynStride(equations,1)),
      Parameter(&m_parameter(0),params,DynStride(1,1)),
      Residual(&m_residual(0),equations,DynStride(1,1)) {

    m_param_rot_offset = 0;
    m_param_trans_offset = params;
    m_eqn_rot_offset = 0;
    m_eqn_trans_offset = equations;

    m_jacobi.setZero(); //important as some places are never written
};

template<typename Scalar, template<class> class Nonlinear>
int Kernel<Scalar, Nonlinear>::MappedEquationSystem::setParameterMap(int number, VectorMap& map, AccessType t) {

    if(t == rotation) {
        new(&map) VectorMap(&m_parameter(m_param_rot_offset), number, DynStride(1,1));
        m_param_rot_offset += number;
        return m_param_rot_offset-number;
    }
    else {
        m_param_trans_offset -= number;
        new(&map) VectorMap(&m_parameter(m_param_trans_offset), number, DynStride(1,1));
        return m_param_trans_offset;
    }
};

template<typename Scalar, template<class> class Nonlinear>
int Kernel<Scalar, Nonlinear>::MappedEquationSystem::setParameterMap(Vector3Map& map, AccessType t) {

    if(t == rotation) {
        new(&map) Vector3Map(&m_parameter(m_param_rot_offset));
        m_param_rot_offset += 3;
        return m_param_rot_offset-3;
    }
    else {
        m_param_trans_offset -= 3;
        new(&map) Vector3Map(&m_parameter(m_param_trans_offset));
        return m_param_trans_offset;
    }
};

template<typename Scalar, template<class> class Nonlinear>
int Kernel<Scalar, Nonlinear>::MappedEquationSystem::setResidualMap(VectorMap& map, AccessType t) {

    if(t == rotation) {
        new(&map) VectorMap(&m_residual(m_eqn_rot_offset), 1, DynStride(1,1));
        return m_eqn_rot_offset++;
    }
    else {
        new(&map) VectorMap(&m_residual(--m_eqn_trans_offset), 1, DynStride(1,1));
        return m_eqn_trans_offset;
    }
};

template<typename Scalar, template<class> class Nonlinear>
void Kernel<Scalar, Nonlinear>::MappedEquationSystem::setJacobiMap(int eqn, int offset, int number, CVectorMap& map) {
    new(&map) CVectorMap(&m_jacobi(eqn, offset), number, DynStride(0,m_eqns));
};

template<typename Scalar, template<class> class Nonlinear>
void Kernel<Scalar, Nonlinear>::MappedEquationSystem::setJacobiMap(int eqn, int offset, int number, VectorMap& map) {
    new(&map) VectorMap(&m_jacobi(eqn, offset), number, DynStride(0,m_eqns));
};

template<typename Scalar, template<class> class Nonlinear>
bool Kernel<Scalar, Nonlinear>::MappedEquationSystem::isValid() {
    if(!m_params || !m_eqns)
        return false;

    return true;
};

template<typename Scalar, template<class> class Nonlinear>
void Kernel<Scalar, Nonlinear>::MappedEquationSystem::setAccess(AccessType t) {

    if(t==complete) {
        new(&Jacobi) MatrixMap(&m_jacobi(0,0),m_eqns,m_params,DynStride(m_eqns,1));
        new(&Parameter) VectorMap(&m_parameter(0),m_params,DynStride(1,1));
        new(&Residual)  VectorMap(&m_residual(0), m_eqns,  DynStride(1,1));
    }
    else if(t==rotation) {
        int num = m_param_trans_offset;
        new(&Jacobi) MatrixMap(&m_jacobi(0,0),m_eqn_trans_offset,num,DynStride(m_eqns,1));
        new(&Parameter) VectorMap(&m_parameter(0),num,DynStride(1,1));
        new(&Residual)  VectorMap(&m_residual(0), m_eqn_trans_offset,  DynStride(1,1));
    }
    else if(t==general) {
        int num = m_params - m_param_trans_offset;
        int eq_num = m_eqns - m_eqn_trans_offset;
        new(&Jacobi) MatrixMap(&m_jacobi(m_eqn_trans_offset,m_param_trans_offset),eq_num,num,DynStride(m_eqns,1));
        new(&Parameter) VectorMap(&m_parameter(m_param_trans_offset),num,DynStride(1,1));
        new(&Residual)  VectorMap(&m_residual(m_eqn_trans_offset), eq_num,  DynStride(1,1));
    }

    m_access = t;
};

template<typename Scalar, template<class> class Nonlinear>
bool Kernel<Scalar, Nonlinear>::MappedEquationSystem::hasAccessType(AccessType t) {

    if(t==rotation)
        return (m_param_rot_offset>0 && m_eqn_rot_offset>0);
    else if(t==general)
        return (m_param_trans_offset<m_params && m_eqn_trans_offset<m_eqns);
    else
        return (m_params>0 && m_eqns>0);
};

template<typename Scalar, template<class> class Nonlinear>
Kernel<Scalar, Nonlinear>::Kernel() {
    //init the solver
    m_solver.setKernel(this);
}

template<typename Scalar, template<class> class Nonlinear>
SolverInfo Kernel<Scalar, Nonlinear>::getSolverInfo() {

    SolverInfo info;
    info.iterations = m_solver.iter;
    info.error = m_solver.err;
    info.time = m_solver.time;

    return info;
}

//static comparison versions
template<typename Scalar, template<class> class Nonlinear>
template <typename DerivedA,typename DerivedB>
bool Kernel<Scalar, Nonlinear>::isSame(const E::MatrixBase<DerivedA>& p1,const E::MatrixBase<DerivedB>& p2, number_type precission) {
    return ((p1-p2).squaredNorm() < precission);
}

template<typename Scalar, template<class> class Nonlinear>
bool Kernel<Scalar, Nonlinear>::isSame(number_type t1, number_type t2, number_type precission) {
    return (std::abs(t1-t2) < precission);
}

template<typename Scalar, template<class> class Nonlinear>
template <typename DerivedA,typename DerivedB>
bool Kernel<Scalar, Nonlinear>::isOpposite(const E::MatrixBase<DerivedA>& p1,const E::MatrixBase<DerivedB>& p2, number_type precission) {
    return ((p1+p2).squaredNorm() < precission);
}

template<typename Scalar, template<class> class Nonlinear>
template <typename DerivedA,typename DerivedB>
bool Kernel<Scalar, Nonlinear>::isSame(const E::MatrixBase<DerivedA>& p1,const E::MatrixBase<DerivedB>& p2) {
    return ((p1-p2).squaredNorm() < getProperty<precision>());
}

template<typename Scalar, template<class> class Nonlinear>
bool Kernel<Scalar, Nonlinear>::isSame(number_type t1, number_type t2) {
    return (std::abs(t1-t2) < getProperty<precision>());
}

template<typename Scalar, template<class> class Nonlinear>
template <typename DerivedA,typename DerivedB>
bool Kernel<Scalar, Nonlinear>::isOpposite(const E::MatrixBase<DerivedA>& p1,const E::MatrixBase<DerivedB>& p2) {
    return ((p1+p2).squaredNorm() < getProperty<precision>());
}

template<typename Scalar, template<class> class Nonlinear>
int Kernel<Scalar, Nonlinear>::solve(MappedEquationSystem& mes) {

    nothing n;
    return m_solver.solve(mes, n);
};

template<typename Scalar, template<class> class Nonlinear>
template<typename Functor>
int Kernel<Scalar, Nonlinear>::solve(MappedEquationSystem& mes, Functor& f) {

    return m_solver.solve(mes, f);
};

}

#endif //GCM_KERNEL_H






