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

#ifndef DCM_SOLVER_3D_IMP_H
#define DCM_SOLVER_3D_IMP_H

#include "../solver.hpp"
#include "../defines.hpp"

#include <boost/graph/undirected_dfs.hpp>

#include <opendcm/core/kernel.hpp>

#ifdef DCM_EXTERNAL_CORE
#include "opendcm/core/imp/kernel_imp.hpp"
#include "opendcm/core/imp/clustergraph_imp.hpp"
#endif

namespace dcm {
namespace details {


template<typename Sys>
MES<Sys>::MES(boost::shared_ptr<Cluster> cl, int par, int eqn) : Base(par, eqn), m_cluster(cl) {
#ifdef USE_LOGGING
    log.add_attribute("Tag", attrs::constant< std::string >("MES3D"));
#endif
};

template<typename Sys>
void MES<Sys>::recalculate() {

    //first calculate all clusters
    typedef typename Cluster::cluster_iterator citer;
    std::pair<citer, citer> cit = m_cluster->clusters();

    for(; cit.first != cit.second; cit.first++) {

        if(!(*cit.first).second->template getProperty<fix_prop>())
            (*cit.first).second->template getProperty<math_prop>().recalculate();

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
                (*oit.first)->calculate(Base::Scaling, Base::m_access);
        }
    }
};

template<typename Sys>
void MES<Sys>::removeLocalGradientZeros() {

#ifdef USE_LOGGING
    BOOST_LOG_SEV(log, information) << "remove local gradient zero";
#endif
    //let the constraints treat the local zeros
    typedef typename Cluster::template object_iterator<Constraint3D> oiter;
    typedef typename boost::graph_traits<Cluster>::edge_iterator eiter;
    std::pair<eiter, eiter>  eit = boost::edges(*m_cluster);

    for(; eit.first != eit.second; eit.first++) {
        //as always: every local edge can hold multiple global ones, so iterate over all constraints
        //hold by the individual edge
        std::pair< oiter, oiter > oit = m_cluster->template getObjects<Constraint3D>(*eit.first);

        for(; oit.first != oit.second; oit.first++) {
            if(*oit.first)
                (*oit.first)->treatLGZ();
        }
    }
};


template<typename Sys>
SystemSolver<Sys>::Rescaler::Rescaler(boost::shared_ptr<Cluster> c, Mes& m) : cluster(c), mes(m), rescales(0) {

};

template<typename Sys>
void SystemSolver<Sys>::Rescaler::operator()() {
    const Scalar sc = calculateScale();

    if(sc<MINFAKTOR || sc>MAXFAKTOR)
        mes.Scaling = scaleClusters(sc);

    rescales++;
};

template<typename Sys>
typename SystemSolver<Sys>::Scalar SystemSolver<Sys>::Rescaler::calculateScale() {

    typedef typename Cluster::cluster_iterator citer;
    std::pair<citer, citer> cit = cluster->clusters();
    //get the maximal scale
    Scalar sc = 0;

    for(cit = cluster->clusters(); cit.first != cit.second; cit.first++) {
        //fixed cluster are irrelevant for scaling
        if((*cit.first).second->template getProperty<fix_prop>())
            continue;

        //get the biggest scale factor
        details::ClusterMath<Sys>& math = (*cit.first).second->template getProperty<math_prop>();

        math.m_pseudo.clear();
        collectPseudoPoints(cluster, (*cit.first).first, math.m_pseudo);

        const Scalar s = math.calculateClusterScale();
        sc = (s>sc) ? s : sc;
    }

    return sc;
}

template<typename Sys>
typename SystemSolver<Sys>::Scalar SystemSolver<Sys>::Rescaler::scaleClusters(Scalar sc) {

    //if no scaling-value returned we can use 1
    sc = (Kernel::isSame(sc,0, 1e-10)) ? 1. : sc;

    typedef typename boost::graph_traits<Cluster>::vertex_iterator iter;
    std::pair<iter, iter>  it = boost::vertices(*cluster);

    for(; it.first != it.second; it.first++) {

        if(cluster->isCluster(*it.first)) {
            boost::shared_ptr<Cluster> c = cluster->getVertexCluster(*it.first);
            c->template getProperty<math_prop>().applyClusterScale(sc,
                    c->template getProperty<fix_prop>());
        }
        else {
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
    typedef typename Cluster::global_edge_iterator c_iter;
    typedef typename boost::graph_traits<Cluster>::out_edge_iterator e_iter;
    std::pair<e_iter, e_iter> it = boost::out_edges(cluster, *parent);

    for(; it.first != it.second; it.first++) {

        std::pair< c_iter, c_iter > cit = parent->getGlobalEdges(*it.first);

        for(; cit.first != cit.second; cit.first++) {
            Cons c = parent->template getObject<Constraint3D>(*cit.first);

            if(!c)
                continue;

            //get the first global vertex and see if we have it in the wanted cluster or not
            GlobalVertex v  = cit.first->source;
            std::pair<LocalVertex,bool> res = parent->getLocalVertex(v);

            if(!res.second)
                return; //means the geometry is in none of the clusters which is not allowed

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

    //set out and solve all relevant subclusters
    typedef typename Cluster::cluster_iterator citer;
    std::pair<citer, citer> cit = cluster->clusters();

    for(; cit.first != cit.second; cit.first++) {

        boost::shared_ptr<Cluster> c = (*cit.first).second;

        if(c->template getProperty<changed_prop>() &&
                ((c->template getProperty<type_prop>() == details::cluster3D)
                 || ((c->template getProperty<type_prop>() == details::subcluster) &&
                     (sys.template getOption<subsystemsolving>() == Automatic))))
            solveCluster(c, sys);
    }

    int params=0, constraints=0;
    typename Kernel::number_type scale = 1;

    //get the amount of parameters and constraint equations we need
    typedef typename boost::graph_traits<Cluster>::vertex_iterator iter;
    std::pair<iter, iter>  it = boost::vertices(*cluster);

    for(; it.first != it.second; it.first++) {

        //when cluster and not fixed it has trans and rot parameter
        if(cluster->isCluster(*it.first)) {
            if(!cluster->template getSubclusterProperty<fix_prop>(*it.first)) {
                params += 6;
            }
        }
        else {
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
        BOOST_LOG_SEV(log, error)<< "Error in system counting: params = " << params << " and constraints = "<<constraints;
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
            details::ClusterMath<Sys>& cm =  c->template getProperty<math_prop>();

            //only get maps and propagate downstream if not fixed
            if(!c->template getProperty<fix_prop>()) {
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
            }
            else
                cm.initFixMaps();

            //map all geometries within that cluster to it's rotation matrix
            //for collecting all geometries which need updates
            cm.clearGeometry();
            cm.mapClusterDownstreamGeometry(c);

        }
        else {
            Geom g = cluster->template getObject<Geometry3D>(*it.first);
            g->initMap(&mes);
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

            if(c)
                c->setMaps(mes);

            //TODO: else throw (as every global edge was counted as one equation)
        }
    }

    try {
        //if we don't have rotations we need no expensive scaling code
        if(!mes.hasAccessType(rotation)) {

#ifdef USE_LOGGING
            BOOST_LOG_SEV(log, solving)<< "No rotation parameters in system, solve without scaling";
#endif
            DummyScaler re;
            sys.kernel().solve(mes, re);

        }
        else {

            // we have rotations, so let's check our options. first search for cycles, as systems with them
            // always need the full solver power
            bool has_cycle;
            cycle_dedector cd(has_cycle);
            //create the needed property maps and fill it
            property_map<vertex_index_prop, Cluster> vi_map(cluster);
            cluster->initIndexMaps();
            typedef std::map< LocalVertex, boost::default_color_type> vcmap;
            typedef std::map< LocalEdge, boost::default_color_type> ecmap;
            vcmap v_cm;
            ecmap e_cm;
            boost::associative_property_map< vcmap > v_cpm(v_cm);
            boost::associative_property_map< ecmap > e_cpm(e_cm);

            boost::undirected_dfs(*cluster.get(), boost::visitor(cd).vertex_index_map(vi_map).vertex_color_map(v_cpm).edge_color_map(e_cpm));

            bool done = false;

            if(!has_cycle) {
#ifdef USE_LOGGING
		BOOST_LOG_SEV(log, solving)<< "non-cyclic system dedected: solve rotation only";
#endif
		//cool, lets do uncylic. first all rotational constraints with rotational parameters
		mes.setAccess(rotation);

		//rotations need to be calculated in a scaled manner. thats because the normals used for
		//rotation calculation are always 1, no matter how big the part is. This can lead to problems
		//when for example two rotated faces have a precision error on the parallel normals but a distance
		//at the outer edges is far bigger than the precision as the distance from normal origin to outer edge
		//is bigger 1. that would lead to unsolvable translation-only systems.

		//solve need to catch exceptions to reset the mes scaling on failure
		Rescaler re(cluster, mes);
		mes.Scaling = 1./(re.calculateScale()*SKALEFAKTOR);

		try {
		    DummyScaler dummy;
		    sys.kernel().solve(mes, dummy);
		    mes.Scaling = 1.;
		}
		catch(...) {
		    mes.Scaling = 1.;
		    throw;
		}

		//now let's see if we have to go on with the translations
		if(mes.hasAccessType(general)) {

		    mes.setAccess(general);
		    mes.recalculate();

		    if(sys.kernel().isSame(mes.Residual.template lpNorm<E::Infinity>(),0.))
			done = true;
		    else {
    #ifdef USE_LOGGING
			BOOST_LOG_SEV(log, solving)<< "Solve Translation after Rotations are not enough";
    #endif

			//let's try translation only
			try {
			    DummyScaler re;
			    sys.kernel().solve(mes, re);
			    done=true;
			}
			catch(boost::exception&) {
			    //not successful, so we need brute force
			    done = false;
			}
		    }
		};
            }
            else {
	      throw solving_error() <<  boost::errinfo_errno(22) << error_message("Cyclic system are not yet supported");
	    }

            //not done already? try it the hard way!
            if(!done) {
#ifdef USE_LOGGING
                BOOST_LOG_SEV(log, solving)<< "Full scale solver used";
#endif
                mes.setAccess(complete);
                mes.recalculate();

                Rescaler re(cluster, mes);
                re();
                sys.kernel().solve(mes, re);
#ifdef USE_LOGGING
                BOOST_LOG_SEV(log, solving)<< "Numbers of rescale: "<<re.rescales;
#endif
            };
        }

        //done solving, write the results back
        finish(cluster, sys, mes);
    }
    catch(boost::exception&) {

        if(sys.template getOption<solverfailure>()==ApplyResults)
            finish(cluster, sys, mes);
        else
            throw;
    }
};

template<typename Sys>
void SystemSolver<Sys>::finish(boost::shared_ptr<Cluster> cluster, Sys& sys, Mes& mes) {

    //solving is done, now go to all relevant geometries and clusters and write the values back
    //(no need to emit recalculated signal as this cluster is never recalculated in this run)
    typedef typename boost::graph_traits<Cluster>::vertex_iterator iter;
    std::pair<iter, iter>  it = boost::vertices(*cluster);

    for(; it.first != it.second; it.first++) {

        if(cluster->isCluster(*it.first)) {
            boost::shared_ptr<Cluster> c = cluster->getVertexCluster(*it.first);

            if(!cluster->template getSubclusterProperty<fix_prop>(*it.first))
                c->template getProperty<math_prop>().finishCalculation();
            else
                c->template getProperty<math_prop>().finishFixCalculation();

            std::vector<Geom>& vec = c->template getProperty<math_prop>().getGeometry();

            for(typename std::vector<Geom>::iterator vit = vec.begin(); vit != vec.end(); vit++)
                (*vit)->finishCalculation();

        }
        else {
            Geom g = cluster->template getObject<Geometry3D>(*it.first);
            g->scale(mes.Scaling);
            g->finishCalculation();
        }
    }

    //we have solved this cluster
    cluster->template setProperty<changed_prop>(false);
}

}//details
}//dcm

#endif //DCM_SOLVER_3D_HPP

