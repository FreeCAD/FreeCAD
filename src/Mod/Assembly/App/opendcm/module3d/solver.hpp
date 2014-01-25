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

#ifndef GCM_SOLVER_3D_H
#define GCM_SOLVER_3D_H

#include "defines.hpp"
#include "opendcm/core/sheduler.hpp"

#include <boost/graph/depth_first_search.hpp>

namespace dcm {  
namespace details {

template<typename Sys>
struct MES  : public Sys::Kernel::MappedEquationSystem {

    typedef typename Sys::Kernel Kernel;
    typedef typename Sys::Cluster Cluster;
    typedef typename system_traits<Sys>::template getModule<m3d>::type module3d;
    typedef typename module3d::Geometry3D Geometry3D;
    typedef boost::shared_ptr<Geometry3D> Geom;
    typedef typename module3d::Constraint3D Constraint3D;
    typedef boost::shared_ptr<Constraint3D> Cons;
    typedef typename module3d::math_prop math_prop;
    typedef typename module3d::fix_prop fix_prop;
    typedef typename Kernel::number_type Scalar;
    typedef typename Sys::Kernel::MappedEquationSystem Base;

    boost::shared_ptr<Cluster> m_cluster;
    
#ifdef USE_LOGGING
    dcm_logger log;
#endif

    MES(boost::shared_ptr<Cluster> cl, int par, int eqn);
    virtual void recalculate();
    virtual void removeLocalGradientZeros();
};

template<typename Sys>
struct SystemSolver : public Job<Sys> {

    typedef typename Sys::Cluster Cluster;
    typedef typename Sys::Kernel Kernel;
    typedef typename Kernel::number_type Scalar;
    typedef typename system_traits<Sys>::template getModule<m3d>::type module3d;
    typedef typename module3d::Geometry3D Geometry3D;
    typedef boost::shared_ptr<Geometry3D> Geom;
    typedef typename module3d::Constraint3D Constraint3D;
    typedef boost::shared_ptr<Constraint3D> Cons;
    typedef typename module3d::math_prop math_prop;
    typedef typename module3d::fix_prop fix_prop;
    typedef typename module3d::vertex_prop vertex_prop;

    typedef MES<Sys> Mes;

#ifdef USE_LOGGING
    src::logger log;
#endif
    struct Rescaler {

        boost::shared_ptr<Cluster> cluster;
        Mes& mes;
        int rescales;

        Rescaler(boost::shared_ptr<Cluster> c, Mes& m);

        void operator()();

	Scalar calculateScale();
        Scalar scaleClusters(Scalar sc);
        void collectPseudoPoints(boost::shared_ptr<Cluster> parent,
                                 LocalVertex cluster,
                                 std::vector<typename Kernel::Vector3,
                                 Eigen::aligned_allocator<typename Kernel::Vector3> >& vec);
    };

    struct DummyScaler {
        void operator()() {};
    };

    struct cycle_dedector:public boost::default_dfs_visitor {

        bool& m_dedected;
        cycle_dedector(bool& ed) : m_dedected(ed) {
            m_dedected = false;
        };

        template <class Edge, class Graph>
        void back_edge(Edge u, const Graph& g) {
            m_dedected = true;
        }
    };

    SystemSolver();
    virtual void execute(Sys& sys);
    void solveCluster(boost::shared_ptr<Cluster> cluster, Sys& sys);
    void finish(boost::shared_ptr< Cluster > cluster, Sys& sys, Mes& mes);
};

}//details
}//dcm

#endif //DCM_SOLVER_3D_HPP
