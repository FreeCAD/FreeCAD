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

#ifndef DCM_MODULE_3D_IMP_H
#define DCM_MODULE_3D_IMP_H

#include "../module.hpp"

#include <boost_bind_bind.hpp>

#include "constraint3d_imp.hpp"
#include "geometry3d_imp.hpp"
namespace bp = boost::placeholders;

namespace dcm {

namespace details {

// template<typename seq, typename t>
// struct distance {
//     typedef typename mpl::find<seq, t>::type iterator;
//     typedef typename mpl::distance<typename mpl::begin<seq>::type, iterator>::type type;
//     BOOST_MPL_ASSERT((mpl::not_< boost::is_same<iterator, typename mpl::end<seq>::type > >));
// };

//build a constraint vector
template<typename T>
struct fusion_vec {
    typedef typename mpl::if_< mpl::is_sequence<T>,
            T, fusion::vector1<T> >::type type;
};

struct set_constraint_option {

    template<typename T>
    typename boost::enable_if<mpl::is_sequence<T>, typename fusion_vec<T>::type >::type
    operator()(T& val) {
        return val;
    };
    template<typename T>
    typename boost::disable_if<mpl::is_sequence<T>, typename fusion_vec<T>::type >::type
    operator()(T& val) {
        typename fusion_vec<T>::type vec;
        fusion::at_c<0>(vec) = val;
        return vec;
    };
};

}

template<typename Typelist, typename ID>
template<typename Sys>
Module3D<Typelist, ID>::type<Sys>::inheriter_base::inheriter_base() {
    m_this = ((Sys*) this);
};

template<typename Typelist, typename ID>
template<typename Sys>
typename Module3D<Typelist, ID>::template type<Sys>::Geom
Module3D<Typelist, ID>::type<Sys>::inheriter_base::createGeometry3D() {

    Geom g(new Geometry3D(* ((Sys*) this)));
    fusion::vector<LocalVertex, GlobalVertex> res = m_this->m_cluster->addVertex();
    m_this->m_cluster->template setObject<Geometry3D> (fusion::at_c<0> (res), g);
    g->template setProperty<vertex_prop>(fusion::at_c<1>(res));
    m_this->push_back(g);
    return g;
};

template<typename Typelist, typename ID>
template<typename Sys>
template<typename T>
typename Module3D<Typelist, ID>::template type<Sys>::Geom
Module3D<Typelist, ID>::type<Sys>::inheriter_base::createGeometry3D(T geom) {

    Geom g(new Geometry3D(geom, * ((Sys*) this)));
    fusion::vector<LocalVertex, GlobalVertex> res = m_this->m_cluster->addVertex();
    m_this->m_cluster->template setObject<Geometry3D> (fusion::at_c<0> (res), g);
    g->template setProperty<vertex_prop>(fusion::at_c<1>(res));
    m_this->push_back(g);
    return g;
};

template<typename Typelist, typename ID>
template<typename Sys>
void Module3D<Typelist, ID>::type<Sys>::inheriter_base::removeGeometry3D(Geom g) {

    GlobalVertex v = g->template getProperty<vertex_prop>();

    //check if this vertex holds a constraint
    Cons c = m_this->m_cluster->template getObject<Constraint3D>(v);
    if(c)
        c->template emitSignal<remove>(c);

    //emit remove geometry signal bevore actually deleting it, in case anyone want to access the
    //graph before
    g->template emitSignal<remove>(g);

    //remove the vertex from graph and emit all edges that get removed with the functor
    boost::function<void(GlobalEdge)> functor = boost::bind(&inheriter_base::apply_edge_remove, this, bp::_1);
    m_this->m_cluster->removeVertex(v, functor);
    m_this->erase(g);
};

template<typename Typelist, typename ID>
template<typename Sys>
template<typename T1>
typename Module3D<Typelist, ID>::template type<Sys>::Cons
Module3D<Typelist, ID>::type<Sys>::inheriter_base::createConstraint3D(Geom first, Geom second, T1 constraint1) {

    //get the fusion vector type
    typedef typename details::fusion_vec<T1>::type covec;

    //set the objects
    covec cv = details::set_constraint_option()(constraint1);

    //now create the constraint
    Cons c(new Constraint3D(*m_this, first, second));
    //initialize constraint
    c->initialize(cv);

    //add it to the clustergraph
    fusion::vector<LocalEdge, GlobalEdge, bool, bool> res;
    res = m_this->m_cluster->addEdge(first->template getProperty<vertex_prop>(),
                                     second->template getProperty<vertex_prop>());
    if(!fusion::at_c<2>(res))  {
        Cons rc;
        return rc; //TODO: throw
    };
    m_this->m_cluster->template setObject<Constraint3D> (fusion::at_c<1> (res), c);
    //add the coresbondig edge to the constraint
    c->template setProperty<edge_prop>(fusion::at_c<1>(res));
    //store the constraint in general object vector of main system
    m_this->push_back(c);

    return c;
};

template<typename Typelist, typename ID>
template<typename Sys>
void Module3D<Typelist, ID>::type<Sys>::inheriter_base::removeConstraint3D(Cons c) {

    GlobalEdge e = c->template getProperty<edge_prop>();
    c->template emitSignal<remove>(c);
    m_this->m_cluster->removeEdge(e);
    m_this->erase(c);
};

template<typename Typelist, typename ID>
template<typename Sys>
void Module3D<Typelist, ID>::type<Sys>::inheriter_base::apply_edge_remove(GlobalEdge e) {
    Cons c = m_this->m_cluster->template getObject<Constraint3D>(e);
    c->template emitSignal<remove>(c);
    m_this->erase(c);
};

template<typename Typelist, typename ID>
template<typename Sys>
template<typename T>
typename Module3D<Typelist, ID>::template type<Sys>::Geom
Module3D<Typelist, ID>::type<Sys>::inheriter_id::createGeometry3D(T geom, Identifier id) {
    Geom g = inheriter_base::createGeometry3D(geom);
    g->setIdentifier(id);
    return g;
};

template<typename Typelist, typename ID>
template<typename Sys>
typename Module3D<Typelist, ID>::template type<Sys>::Geom
Module3D<Typelist, ID>::type<Sys>::inheriter_id::createGeometry3D(Identifier id) {
    Geom g = inheriter_base::createGeometry3D();
    g->setIdentifier(id);
    return g;
};

template<typename Typelist, typename ID>
template<typename Sys>
void Module3D<Typelist, ID>::type<Sys>::inheriter_id::removeGeometry3D(Identifier id) {

    if(hasGeometry3D(id))
        inheriter_base::removeGeometry3D(getGeometry3D(id));
    else
        throw module3d_error() <<  boost::errinfo_errno(410) << error_message("no geometry with this ID in this system");

};

template<typename Typelist, typename ID>
template<typename Sys>
template<typename T>
typename Module3D<Typelist, ID>::template type<Sys>::Cons
Module3D<Typelist, ID>::type<Sys>::inheriter_id::createConstraint3D(Identifier id, Geom first, Geom second, T constraint1) {

    Cons c = inheriter_base::createConstraint3D(first, second, constraint1);
    c->setIdentifier(id);
    return c;
};

template<typename Typelist, typename ID>
template<typename Sys>
void Module3D<Typelist, ID>::type<Sys>::inheriter_id::removeConstraint3D(Identifier id) {

    if(hasConstraint3D(id))
        removeConstraint3D(getConstraint3D(id));
    else
        throw module3d_error() <<  boost::errinfo_errno(411) << error_message("no constraint with this ID in this system");
};


template<typename Typelist, typename ID>
template<typename Sys>
bool Module3D<Typelist, ID>::type<Sys>::inheriter_id::hasGeometry3D(Identifier id) {
    if(getGeometry3D(id))
        return true;
    return false;
};

template<typename Typelist, typename ID>
template<typename Sys>
typename Module3D<Typelist, ID>::template type<Sys>::Geom
Module3D<Typelist, ID>::type<Sys>::inheriter_id::getGeometry3D(Identifier id) {
    std::vector< Geom >& vec = inheriter_base::m_this->template objectVector<Geometry3D>();
    typedef typename std::vector<Geom>::iterator iter;
    for(iter it=vec.begin(); it!=vec.end(); it++) {
        if(compare_traits<Identifier>::compare((*it)->getIdentifier(), id))
            return *it;
    };
    return Geom();
};

template<typename Typelist, typename ID>
template<typename Sys>
bool Module3D<Typelist, ID>::type<Sys>::inheriter_id::hasConstraint3D(Identifier id) {
    if(getConstraint3D(id))
        return true;
    return false;
};

template<typename Typelist, typename ID>
template<typename Sys>
typename Module3D<Typelist, ID>::template type<Sys>::Cons
Module3D<Typelist, ID>::type<Sys>::inheriter_id::getConstraint3D(Identifier id) {
    std::vector< Cons >& vec = inheriter_base::m_this->template objectVector<Constraint3D>();
    typedef typename std::vector<Cons>::iterator iter;
    for(iter it=vec.begin(); it!=vec.end(); it++) {
        if(compare_traits<Identifier>::compare((*it)->getIdentifier(), id))
            return *it;
    };
    return Cons();
};

} //dcm

#endif //DCM_MODULE_3D_IMP_H
