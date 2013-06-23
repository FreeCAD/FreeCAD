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
#include "clustermath.hpp"
#include "opendcm/core/sheduler.hpp"
#include "opendcm/core/traits.hpp"
#include <opendcm/core/kernel.hpp>
#include <opendcm/core/property.hpp>

#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/depth_first_search.hpp>
#include <boost/exception/errinfo_errno.hpp>

namespace dcm {
namespace details {

template<typename Sys>
struct MES  : public system_traits<Sys>::Kernel::MappedEquationSystem {

    typedef typename system_traits<Sys>::Kernel Kernel;
    typedef typename system_traits<Sys>::Cluster Cluster;
    typedef typename system_traits<Sys>::template getModule<m3d>::type module3d;
    typedef typename module3d::Geometry3D Geometry3D;
    typedef boost::shared_ptr<Geometry3D> Geom;
    typedef typename module3d::Constraint3D Constraint3D;
    typedef boost::shared_ptr<Constraint3D> Cons;
    typedef typename module3d::math_prop math_prop;
    typedef typename module3d::fix_prop fix_prop;
    typedef typename Kernel::number_type Scalar;
    typedef typename system_traits<Sys>::Kernel::MappedEquationSystem Base;

    boost::shared_ptr<Cluster> m_cluster;

    MES(boost::shared_ptr<Cluster> cl, int par, int eqn);
    virtual void recalculate();
};

template<typename Sys>
struct SystemSolver : public Job<Sys> {

    typedef typename system_traits<Sys>::Cluster Cluster;
    typedef typename system_traits<Sys>::Kernel Kernel;
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

        Scalar scaleClusters();
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
};


/*****************************************************************************************************************/
/*****************************************************************************************************************/
/*****************************************************************************************************************/
/*****************************************************************************************************************/


template<typename Sys>
MES<Sys>::MES(boost::shared_ptr<Cluster> cl, int par, int eqn) : Base(par, eqn), m_cluster(cl) {

};

template<typename Sys>
void MES<Sys>::recalculate() {

    //first calculate all clusters
    typedef typename Cluster::cluster_iterator citer;
    std::pair<citer, citer> cit = m_cluster->clusters();
    for(; cit.first != cit.second; cit.first++) {

        if(!(*cit.first).second->template getClusterProperty<fix_prop>())
            (*cit.first).second->template getClusterProperty<math_prop>().recalculate();

    };

    //with everything updated just nicely we can compute the constraints
    typedef typename Cluster::template object_iterator<Constraint3D> oiter;
    typedef typename boost::graph_traits<Cluster>::edge_iterator eiter;
    std::pair<eiter, eiter>  eit = boost::edges(*m_cluster);
    for(; eit.first != eit.second; eit.first++) {
        //as always: every local edge can hold multiple global ones, so iterate over all constraints
        //hold by the individual edge
        std::pair< oiter, oiter > oit = m_cluster->template getObjects<Constraint3D>(*eit.first);
        for(; oit.first != oit.second; oit.first++) {
            if(*oit.first)
                (*oit.first)->calculate(Base::Scaling, Base::rot_only);
        }
    }
};

template<typename Sys>
SystemSolver<Sys>::Rescaler::Rescaler(boost::shared_ptr<Cluster> c, Mes& m) : cluster(c), mes(m), rescales(0) {

};

template<typename Sys>
void SystemSolver<Sys>::Rescaler::operator()() {
    mes.Scaling = scaleClusters();
    rescales++;
};

template<typename Sys>
typename SystemSolver<Sys>::Scalar SystemSolver<Sys>::Rescaler::scaleClusters() {

    typedef typename Cluster::cluster_iterator citer;
    std::pair<citer, citer> cit = cluster->clusters();
    //get the maximal scale
    Scalar sc = 0;
    for(cit = cluster->clusters(); cit.first != cit.second; cit.first++) {
        //fixed cluster are irrelevant for scaling
        if((*cit.first).second->template getClusterProperty<fix_prop>()) continue;

        //get the biggest scale factor
        details::ClusterMath<Sys>& math = (*cit.first).second->template getClusterProperty<math_prop>();

        math.m_pseudo.clear();
        collectPseudoPoints(cluster, (*cit.first).first, math.m_pseudo);

        const Scalar s = math.calculateClusterScale();
        sc = (s>sc) ? s : sc;
    }
    //if no scaling-value returned we can use 1
    sc = (Kernel::isSame(sc,0)) ? 1. : sc;

    typedef typename boost::graph_traits<Cluster>::vertex_iterator iter;
    std::pair<iter, iter>  it = boost::vertices(*cluster);
    for(; it.first != it.second; it.first++) {

        if(cluster->isCluster(*it.first)) {
            boost::shared_ptr<Cluster> c = cluster->getVertexCluster(*it.first);
            c->template getClusterProperty<math_prop>().applyClusterScale(sc,
                    c->template getClusterProperty<fix_prop>());
        } else {
            Geom g = cluster->template getObject<Geometry3D>(*it.first);
            g->scale(sc*SKALEFAKTOR);
        }
    }
    return 1./(sc*SKALEFAKTOR);
};

template<typename Sys>
void SystemSolver<Sys>::Rescaler::collectPseudoPoints(
    boost::shared_ptr<typename SystemSolver<Sys>::Cluster> parent,
    LocalVertex cluster,
    std::vector<typename SystemSolver<Sys>::Kernel::Vector3,
    Eigen::aligned_allocator<typename SystemSolver<Sys>::Kernel::Vector3> >& vec) {

    std::vector<typename Kernel::Vector3, Eigen::aligned_allocator<typename Kernel::Vector3> > vec2;
    typedef typename Cluster::template object_iterator<Constraint3D> c_iter;
    typedef typename boost::graph_traits<Cluster>::out_edge_iterator e_iter;
    std::pair<e_iter, e_iter> it = boost::out_edges(cluster, *parent);
    for(; it.first != it.second; it.first++) {

        std::pair< c_iter, c_iter > cit = parent->template getObjects<Constraint3D>(*it.first);
        for(; cit.first != cit.second; cit.first++) {
            Cons c = *(cit.first);

            if(!c)
                continue;

            //get the first global vertex and see if we have it in the wanted cluster or not
            GlobalVertex v  = c->first->template getProperty<vertex_prop>();
            std::pair<LocalVertex,bool> res = parent->getLocalVertex(v);
            if(!res.second)
                return; //means the geometry is in non of the clusters which is not allowed

            if(res.first == cluster)
                c->collectPseudoPoints(vec, vec2);
            else
                c->collectPseudoPoints(vec2, vec);
        }
    }
};

template<typename Sys>
SystemSolver<Sys>::SystemSolver() {
    Job<Sys>::priority = 1000;
#ifdef USE_LOGGING
    log.add_attribute("Tag", attrs::constant< std::string >("SystemSolver3D"));
#endif
};

template<typename Sys>
void SystemSolver<Sys>::execute(Sys& sys) {
    solveCluster(sys.m_cluster, sys);
};

template<typename Sys>
void SystemSolver<Sys>::solveCluster(boost::shared_ptr<Cluster> cluster, Sys& sys) {

    try {
        //set out and solve all relevant subclusters
        typedef typename Cluster::cluster_iterator citer;
        std::pair<citer, citer> cit = cluster->clusters();
        for(; cit.first != cit.second; cit.first++) {

            boost::shared_ptr<Cluster> c = (*cit.first).second;
            if(c->template getClusterProperty<changed_prop>() &&
                    c->template getClusterProperty<type_prop>() == details::cluster3D)
                solveCluster(c, sys);
        }

        int params=0, constraints=0;
        typename Kernel::number_type scale = 1;

        //get the ammount of parameters and constraint equations we need
        typedef typename boost::graph_traits<Cluster>::vertex_iterator iter;
        std::pair<iter, iter>  it = boost::vertices(*cluster);
        for(; it.first != it.second; it.first++) {

            //when cluster and not fixed it has trans and rot parameter
            if(cluster->isCluster(*it.first)) {
                if(!cluster->template getSubclusterProperty<fix_prop>(*it.first)) {
                    params += 6;
                }
            } else {
                params += cluster->template getObject<Geometry3D>(*it.first)->m_parameterCount;
            };
        }

        //count the equations in the constraints
        typedef typename Cluster::template object_iterator<Constraint3D> ocit;
        typedef typename boost::graph_traits<Cluster>::edge_iterator e_iter;
        std::pair<e_iter, e_iter>  e_it = boost::edges(*cluster);
        for(; e_it.first != e_it.second; e_it.first++) {
            std::pair< ocit, ocit > it = cluster->template getObjects<Constraint3D>(*e_it.first);
            for(; it.first != it.second; it.first++)
                constraints += (*it.first)->equationCount();
        };

        if(params <= 0 || constraints <= 0) {
            //TODO:throw
#ifdef USE_LOGGING
            BOOST_LOG(log)<< "Error in system counting: params = " << params << " and constraints = "<<constraints;
#endif
            return;
        }

        //initialise the system with now known size
        Mes mes(cluster, params, constraints);

        //iterate all geometrys again and set the needed maps
        it = boost::vertices(*cluster);
        for(; it.first != it.second; it.first++) {

            if(cluster->isCluster(*it.first)) {
                boost::shared_ptr<Cluster> c = cluster->getVertexCluster(*it.first);
                details::ClusterMath<Sys>& cm =  c->template getClusterProperty<math_prop>();
                //only get maps and propagate downstream if not fixed
                if(!c->template getClusterProperty<fix_prop>()) {
                    //set norm Quaternion as map to the parameter vector
                    int offset_rot = mes.setParameterMap(cm.getNormQuaternionMap(), rotation);
                    //set translation as map to the parameter vector
                    int offset = mes.setParameterMap(cm.getTranslationMap(), general);
                    //write initail values to the parameter maps
                    //remember the parameter offset as all downstream geometry must use this offset
                    cm.setParameterOffset(offset_rot, rotation);
                    cm.setParameterOffset(offset, general);
                    //wirte initial values
                    cm.initMaps();
                } else cm.initFixMaps();

                //map all geometrie within that cluster to it's rotation matrix
                //for collecting all geometries which need updates
                cm.clearGeometry();
                cm.mapClusterDownstreamGeometry(c);

            } else {
                Geom g = cluster->template getObject<Geometry3D>(*it.first);
                int offset = mes.setParameterMap(g->m_parameterCount, g->getParameterMap());
                g->m_offset = offset;
                //init the parametermap with initial values
                g->initMap();
            }
        }

        //and now the constraints to set the residual and gradient maps
        typedef typename Cluster::template object_iterator<Constraint3D> oiter;
        e_it = boost::edges(*cluster);
        for(; e_it.first != e_it.second; e_it.first++) {

            //as always: every local edge can hold multiple global ones, so iterate over all constraints
            //hold by the individual edge
            std::pair< oiter, oiter > oit = cluster->template getObjects<Constraint3D>(*e_it.first);
            for(; oit.first != oit.second; oit.first++) {

                //set the maps
                Cons c = *oit.first;
                if(c) c->setMaps(mes);
                //TODO: else throw (as every global edge was counted as one equation)
            }
        }

        //if we don't have rotations we need no expensive scaling code
        if(!mes.hasParameterType(rotation)) {

#ifdef USE_LOGGING
            BOOST_LOG(log)<< "No rotation parameters in system, solve without scaling";
#endif
            DummyScaler re;
            Kernel::solve(mes, re);

        } else {

            // we have rotations, so let's check our options. first search for cycles, as systems with them
            // always need the full solver power
            bool has_cycle;
            cycle_dedector cd(has_cycle);
            //create te needed property map, fill it and run the test
            property_map<vertex_index_prop, Cluster> vi_map(cluster);
            cluster->initIndexMaps();
            boost::depth_first_search(*cluster.get(), boost::visitor(cd).vertex_index_map(vi_map));

            bool done = false;
            if(!has_cycle) {
#ifdef USE_LOGGING
                BOOST_LOG(log)<< "non-cyclic system dedected"
#endif
                              //cool, lets do uncylic. first all rotational constraints with rotational parameters
                              mes.setAccess(rotation);
                mes.setGeneralEquationAccess(false);
                //solve can be done without catching exceptions, because this only fails if the system in
                //unsolvable
                DummyScaler re;
                Kernel::solve(mes, re);

                //now let's see if we have to go on with the translations
                mes.setAccess(general);
                mes.setGeneralEquationAccess(true);
                mes.recalculate();
                if(mes.Residual.norm()<1e-6)
                    done = true;
                else {
                    //let's try translation only
                    try {
                        DummyScaler re;
                        Kernel::solve(mes, re);
                        done=true;
                    } catch(boost::exception& ) {
                        //not successful, so we need brute force
                        done = false;
                    }
                }
            };

            //not done already? try it the hard way!
            if(!done) {
#ifdef USE_LOGGING
                BOOST_LOG(log)<< "Full scale solver used"
#endif
                              Rescaler re(cluster, mes);
                re();
                Kernel::solve(mes, re);
#ifdef USE_LOGGING
                BOOST_LOG(log)<< "Numbers of rescale: "<<re.rescales;
#endif
            };
        }

        //solving is done, now go to all relevant geometries and clusters and write the values back
        it = boost::vertices(*cluster);
        for(; it.first != it.second; it.first++) {

            if(cluster->isCluster(*it.first)) {
                boost::shared_ptr<Cluster> c = cluster->getVertexCluster(*it.first);
                if(!cluster->template getSubclusterProperty<fix_prop>(*it.first))
                    c->template getClusterProperty<math_prop>().finishCalculation();
                else
                    c->template getClusterProperty<math_prop>().finishFixCalculation();

                std::vector<Geom>& vec = c->template getClusterProperty<math_prop>().getGeometry();
                for(typename std::vector<Geom>::iterator vit = vec.begin(); vit != vec.end(); vit++)
                    (*vit)->finishCalculation();

            } else {
                Geom g = cluster->template getObject<Geometry3D>(*it.first);
                g->scale(mes.Scaling);
                g->finishCalculation();
            }
        }
        //we have solved this cluster
        cluster->template setClusterProperty<changed_prop>(false);

    } catch(boost::exception& ) {
        throw;
    }
};

}//details
}//dcm

#endif //DCM_SOLVER_3D_HPP
