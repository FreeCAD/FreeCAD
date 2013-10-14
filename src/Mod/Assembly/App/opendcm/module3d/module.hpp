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

#ifndef GCM_MODULE_3D_H
#define GCM_MODULE_3D_H

#include <boost/mpl/vector.hpp>
#include <boost/mpl/map.hpp>
#include <boost/mpl/less.hpp>
#include <boost/mpl/if.hpp>
#include <boost/mpl/size.hpp>

#include <boost/fusion/include/as_vector.hpp>
#include <boost/fusion/include/mpl.hpp>
#include <boost/fusion/include/at.hpp>

#include <boost/static_assert.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/preprocessor/iteration/local.hpp>
#include <boost/variant.hpp>

#include <boost/function.hpp>
#include <boost/bind.hpp>

#include "opendcm/core.hpp"
#include "opendcm/core/object.hpp"
#include "opendcm/core/geometry.hpp"
#include "opendcm/core/clustergraph.hpp"
#include "opendcm/core/sheduler.hpp"
#include "opendcm/core/traits.hpp"
#include "geometry.hpp"
#include "distance.hpp"
#include "parallel.hpp"
#include "angle.hpp"
#include "solver.hpp"
#include "defines.hpp"
#include "clustermath.hpp"


namespace mpl = boost::mpl;

namespace dcm {

namespace details {

template<typename seq, typename t>
struct distance {
    typedef typename mpl::find<seq, t>::type iterator;
    typedef typename mpl::distance<typename mpl::begin<seq>::type, iterator>::type type;
    BOOST_MPL_ASSERT((mpl::not_< boost::is_same<iterator, typename mpl::end<seq>::type > >));
};
}
}//dcm

namespace dcm {

struct reset {}; 	//signal name

template<typename Typelist, typename ID = No_Identifier>
struct Module3D {

    template<typename Sys>
    struct type : details::m3d {
        struct Constraint3D;
        struct Geometry3D;
        struct vertex_prop;
        struct inheriter_base;

        typedef boost::shared_ptr<Geometry3D> Geom;
        typedef boost::shared_ptr<Constraint3D> Cons;

        typedef mpl::map3<  mpl::pair<reset, boost::function<void (Geom) > >,
                mpl::pair<remove, boost::function<void (Geom) > > ,
                mpl::pair<recalculated, boost::function<void (Geom)> > > GeomSig;
        typedef mpl::map1< mpl::pair<remove, boost::function<void (Cons) > > >  ConsSignal;

        typedef ID Identifier;
        typedef Typelist geometry_types;

        typedef details::MES<Sys> MES;
        typedef details::SystemSolver<Sys> SystemSolver;

        template<typename Derived>
        class Geometry3D_base : public details::Geometry<typename Sys::Kernel, 3, typename Sys::geometries>,
            public Object<Sys, Derived, GeomSig> {

            typedef details::Geometry<typename Sys::Kernel, 3, typename Sys::geometries> Base;
            typedef Object<Sys, Derived, GeomSig> ObjBase;
            typedef typename Sys::Kernel Kernel;
            typedef typename Kernel::number_type Scalar;

        public:
            Geometry3D_base(Sys& system);

            template<typename T>
            Geometry3D_base(const T& geometry, Sys& system);

            template<typename T>
            void set(const T& geometry);

            bool holdsType() {
                return m_geometry.which()!=0;
            };
            int whichType() {
                return m_geometry.which()-1;
            };

            template<typename Visitor>
            typename Visitor::result_type apply(Visitor& vis) {
                return boost::apply_visitor(vis, m_geometry);
            };

            template<typename T>
            T convertTo() {
                T t;
                (typename geometry_traits<T>::modell()).template inject<typename Kernel::number_type,
                typename geometry_traits<T>::accessor >(t, Base::m_global);
                return t;
            };

            virtual boost::shared_ptr<Derived> clone(Sys& newSys);

        protected:
            typedef typename mpl::push_front<Typelist, boost::blank>::type ExtTypeList;
            typedef typename boost::make_variant_over< ExtTypeList >::type Variant;

            struct cloner : boost::static_visitor<void> {
                typedef typename boost::make_variant_over< ExtTypeList >::type Variant;

                Variant variant;
                cloner(Variant& v) : variant(v) {};

                template<typename T>
                void operator()(T& t) {
                    variant = geometry_clone_traits<T>()(t);
                };
            };

            //visitor to write the calculated value into the variant
            struct apply_visitor : public boost::static_visitor<void> {

                apply_visitor(typename Kernel::Vector& v) : value(v) {};
                template <typename T>
                void operator()(T& t) const  {
                    (typename geometry_traits<T>::modell()).template inject<typename Kernel::number_type,
                    typename geometry_traits<T>::accessor >(t, value);
                }
                typename Kernel::Vector& value;
            };

            Variant m_geometry; //Variant holding the real geometry type

            //override protected event functions to emit signals
            void reset();
            void recalculated();
            void removed();

            friend struct Constraint3D;
        };

        template<typename Derived>
        class Geometry3D_id : public Geometry3D_base<Derived> {

            typedef Geometry3D_base<Derived> Base;

#ifdef USE_LOGGING
            attrs::mutable_constant< std::string > log_id;
#endif
        public:
            Geometry3D_id(Sys& system);

            template<typename T>
            Geometry3D_id(const T& geometry, Sys& system);

            template<typename T>
            void set(const T& geometry, Identifier id);
            //somehow the base class set funtion is not found
            template<typename T>
            void set(const T& geometry);

            Identifier& getIdentifier();
            void setIdentifier(Identifier id);
        };

        struct Geometry3D : public mpl::if_<boost::is_same<Identifier, No_Identifier>,
                Geometry3D_base<Geometry3D>, Geometry3D_id<Geometry3D> >::type {

            typedef vertex_prop vertex_propertie;

            Geometry3D(Sys& system);

            template<typename T>
            Geometry3D(const T& geometry, Sys& system);

            //allow accessing the internals by module3d classes but not by users
            friend struct details::ClusterMath<Sys>;
            friend struct details::ClusterMath<Sys>::map_downstream;
            friend struct details::SystemSolver<Sys>;
            friend struct details::SystemSolver<Sys>::Rescaler;
            friend struct inheriter_base;

        public:
            //the geometry class itself does not hold an aligned eigen object, but maybe the variant
            EIGEN_MAKE_ALIGNED_OPERATOR_NEW
        };

        template<typename Derived>
        class Constraint3D_base : public detail::Constraint<Sys, 3>, public Object<Sys, Derived, ConsSignal> {

            typedef detail::Constraint<Sys, 3> CBase;
        public:
            Constraint3D_base(Sys& system, Geom f, Geom s) : detail::Constraint<Sys, 3>(f,s),
                Object<Sys, Derived, ConsSignal>(system) {};

            virtual boost::shared_ptr<Derived> clone(Sys& newSys);
        };

        template<typename Derived>
        class Constraint3D_id : public Constraint3D_base<Derived> {

            typedef Constraint3D_base<Derived> base;
        public:
            Constraint3D_id(Sys& system, Geom f, Geom s);

            Identifier& getIdentifier();
            void setIdentifier(Identifier id);
        };

        struct Constraint3D : public mpl::if_<boost::is_same<Identifier, No_Identifier>,
                Constraint3D_base<Constraint3D>,
                Constraint3D_id<Constraint3D> >::type {

            Constraint3D(Sys& system, Geom first, Geom second);

            friend struct details::SystemSolver<Sys>;
            friend struct details::SystemSolver<Sys>::Rescaler;
            friend struct details::MES<Sys>;
            friend struct inheriter_base;
        };

        struct inheriter_base {

            //build a constraint vector
            template<typename T>
            struct fusion_vec {
                typedef typename mpl::if_< mpl::is_sequence<T>,
                        T, fusion::vector<T> >::type type;
            };

            template<typename covec>
            struct initalizer : public boost::static_visitor<void> {

                Cons constraint;
                covec& cov;
                initalizer(Cons c, covec& co) : constraint(c), cov(co) {};
                template<typename T1, typename T2>
                void operator()(const T1& t1, const T2& t2) {
                    constraint->template initialize< typename geometry_traits<T1>::tag,
                               typename geometry_traits<T2>::tag, covec>(cov);
                };
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

            inheriter_base();

            template<typename T>
            Geom createGeometry3D(T geom);
            Geom createGeometry3D();
            void removeGeometry3D(Geom g);

            template<typename T1>
            Cons createConstraint3D(Geom first, Geom second, T1 constraint1);
            void removeConstraint3D(Cons c);

        protected:
            Sys* m_this;
            void apply_edge_remove(GlobalEdge e);
        };

        struct inheriter_id : public inheriter_base {

        protected:
            using inheriter_base::m_this;

        public:
	    using inheriter_base::createGeometry3D;
	    using inheriter_base::createConstraint3D;
	    
            template<typename T>
            Geom createGeometry3D(T geom, Identifier id);
            Geom createGeometry3D(Identifier id);
            template<typename T>
            Cons createConstraint3D(Identifier id, Geom first, Geom second, T constraint1);

            void removeGeometry3D(Identifier id);
            void removeConstraint3D(Identifier id);
            using inheriter_base::removeGeometry3D;
            using inheriter_base::removeConstraint3D;

            bool hasGeometry3D(Identifier id);
            Geom getGeometry3D(Identifier id);
            bool hasConstraint3D(Identifier id);
            Cons getConstraint3D(Identifier id);
        };

        struct inheriter : public mpl::if_<boost::is_same<Identifier, No_Identifier>, inheriter_base, inheriter_id>::type {};

        struct math_prop {
            typedef cluster_property kind;
            typedef details::ClusterMath<Sys> type;
        };
        struct fix_prop {
            typedef cluster_property kind;
            typedef bool type;
        };
        struct vertex_prop {
            typedef Geometry3D kind;
            typedef GlobalVertex type;
        };
        struct edge_prop {
            typedef Constraint3D kind;
            typedef GlobalEdge type;
        };

        typedef mpl::vector4<vertex_prop, edge_prop, math_prop, fix_prop>  properties;
        typedef mpl::vector2<Geometry3D, Constraint3D> objects;
        typedef mpl::vector5<tag::point3D, tag::direction3D, tag::line3D, tag::plane3D, tag::cylinder3D> geometries;

        static void system_init(Sys& sys) {
            sys.m_sheduler.addProcessJob(new SystemSolver());
        };
        static void system_copy(const Sys& from, Sys& into) {
            //nothing to to as all objects and properties are copyed with the clustergraph
        };
    };
};

namespace details {
//allow direct access to the stored geometry in a Geometry3D, copyed from boost variant get
template <typename T>
struct get_visitor {
private:

    typedef typename boost::add_pointer<T>::type pointer;
    typedef typename boost::add_reference<T>::type reference;

public:

    typedef pointer result_type;

public:
    pointer operator()(reference operand) const   {
        return boost::addressof(operand);
    }

    template <typename U>
    pointer operator()(const U&) const  {
        return static_cast<pointer>(0);
    }
};
}

template<typename T, typename G>
typename boost::add_reference<T>::type get(G geom) {

    typedef typename boost::add_pointer<T>::type T_ptr;
    details::get_visitor<T> v;
    T_ptr result = geom->apply(v);

    //if (!result)
    //TODO:throw bad_get();
    return *result;
};


/*****************************************************************************************************************/
/*****************************************************************************************************************/
/*****************************************************************************************************************/
/*****************************************************************************************************************/

template<typename Typelist, typename ID>
template<typename Sys>
template<typename Derived>
Module3D<Typelist, ID>::type<Sys>::Geometry3D_base<Derived>::Geometry3D_base(Sys& system)
    : Object<Sys, Derived, GeomSig>(system) {

#ifdef USE_LOGGING
    log.add_attribute("Tag", attrs::constant< std::string >("Geometry3D"));
#endif
};

template<typename Typelist, typename ID>
template<typename Sys>
template<typename Derived>
template<typename T>
Module3D<Typelist, ID>::type<Sys>::Geometry3D_base<Derived>::Geometry3D_base(const T& geometry, Sys& system)
    : Object<Sys, Derived, GeomSig>(system) {

#ifdef USE_LOGGING
    log.add_attribute("Tag", attrs::constant< std::string >("Geometry3D"));
#endif

    m_geometry = geometry;
    //first init, so that the geometry internal vector has the right size
    Base::template init< typename geometry_traits<T>::tag >();
    //now write the value;
    (typename geometry_traits<T>::modell()).template extract<Scalar,
    typename geometry_traits<T>::accessor >(geometry, Base::getValue());
};

template<typename Typelist, typename ID>
template<typename Sys>
template<typename Derived>
template<typename T>
void Module3D<Typelist, ID>::type<Sys>::Geometry3D_base<Derived>::set(const T& geometry) {

    m_geometry = geometry;
    //first init, so that the geometry internal vector has the right size
    Base::template init< typename geometry_traits<T>::tag >();
    //now write the value;
    (typename geometry_traits<T>::modell()).template extract<Scalar,
    typename geometry_traits<T>::accessor >(geometry, Base::getValue());

    reset();
};

template<typename Typelist, typename ID>
template<typename Sys>
template<typename Derived>
boost::shared_ptr<Derived> Module3D<Typelist, ID>::type<Sys>::Geometry3D_base<Derived>::clone(Sys& newSys) {

    //copy the standart stuff
    boost::shared_ptr<Derived> np = boost::shared_ptr<Derived>(new Derived(*static_cast<Derived*>(this)));
    np->m_system = &newSys;
    //it's possible that the variant contains pointers, so we need to clone them
    cloner clone_fnc(np->m_geometry);
    boost::apply_visitor(clone_fnc, m_geometry);
    return np;
};

template<typename Typelist, typename ID>
template<typename Sys>
template<typename Derived>
void Module3D<Typelist, ID>::type<Sys>::Geometry3D_base<Derived>::recalculated() {

    apply_visitor v(Base::getValue());
    apply(v);

    ObjBase::template emitSignal<dcm::recalculated>(((Derived*)this)->shared_from_this());
};

template<typename Typelist, typename ID>
template<typename Sys>
template<typename Derived>
void Module3D<Typelist, ID>::type<Sys>::Geometry3D_base<Derived>::removed() {

    ObjBase::template emitSignal<dcm::remove>(((Derived*)this)->shared_from_this());
};

template<typename Typelist, typename ID>
template<typename Sys>
template<typename Derived>
void Module3D<Typelist, ID>::type<Sys>::Geometry3D_base<Derived>::reset() {

    ObjBase::template emitSignal<dcm::reset>(((Derived*)this)->shared_from_this());
};

template<typename Typelist, typename ID>
template<typename Sys>
template<typename Derived>
Module3D<Typelist, ID>::type<Sys>::Geometry3D_id<Derived>::Geometry3D_id(Sys& system)
    : Module3D<Typelist, ID>::template type<Sys>::template Geometry3D_base<Derived>(system)
#ifdef USE_LOGGING
, log_id("No ID")
#endif
{

#ifdef USE_LOGGING
    Base::log.add_attribute("ID", log_id);
#endif
};

template<typename Typelist, typename ID>
template<typename Sys>
template<typename Derived>
template<typename T>
Module3D<Typelist, ID>::type<Sys>::Geometry3D_id<Derived>::Geometry3D_id(const T& geometry, Sys& system)
    : Module3D<Typelist, ID>::template type<Sys>::template Geometry3D_base<Derived>(geometry, system)
#ifdef USE_LOGGING
, log_id("No ID")
#endif
{

#ifdef USE_LOGGING
    Base::log.add_attribute("ID", log_id);
#endif
};

template<typename Typelist, typename ID>
template<typename Sys>
template<typename Derived>
template<typename T>
void Module3D<Typelist, ID>::type<Sys>::Geometry3D_id<Derived>::set(const T& geometry, Identifier id) {
    this->template setProperty<id_prop<Identifier> >(id);
    Base::set(geometry);
};

template<typename Typelist, typename ID>
template<typename Sys>
template<typename Derived>
template<typename T>
void Module3D<Typelist, ID>::type<Sys>::Geometry3D_id<Derived>::set(const T& geometry) {
    Base::set(geometry);
};

template<typename Typelist, typename ID>
template<typename Sys>
template<typename Derived>
typename Module3D<Typelist, ID>::template type<Sys>::Identifier&
Module3D<Typelist, ID>::type<Sys>::Geometry3D_id<Derived>::getIdentifier() {
    return  this->template getProperty<id_prop<Identifier> >();
};

template<typename Typelist, typename ID>
template<typename Sys>
template<typename Derived>
void Module3D<Typelist, ID>::type<Sys>::Geometry3D_id<Derived>::setIdentifier(Identifier id) {
    this->template setProperty<id_prop<Identifier> >(id);
#ifdef USE_LOGGING
    std::stringstream str;
    str<<this->template getProperty<id_prop<Identifier> >();
    log_id.set(str.str());
    BOOST_LOG(Base::log)<<"Identifyer set: "<<id;
#endif
};

template<typename Typelist, typename ID>
template<typename Sys>
Module3D<Typelist, ID>::type<Sys>::Geometry3D::Geometry3D(Sys& system)
    : mpl::if_<boost::is_same<Identifier, No_Identifier>,
      Geometry3D_base<Geometry3D>,
      Geometry3D_id<Geometry3D> >::type(system) {

};

template<typename Typelist, typename ID>
template<typename Sys>
template<typename T>
Module3D<Typelist, ID>::type<Sys>::Geometry3D::Geometry3D(const T& geometry, Sys& system)
    : mpl::if_<boost::is_same<Identifier, No_Identifier>,
      Geometry3D_base<Geometry3D>,
      Geometry3D_id<Geometry3D> >::type(geometry, system) {

};

template<typename Typelist, typename ID>
template<typename Sys>
template<typename Derived>
boost::shared_ptr<Derived> Module3D<Typelist, ID>::type<Sys>::Constraint3D_base<Derived>::clone(Sys& newSys) {

    //copy the standart stuff
    boost::shared_ptr<Derived> np = boost::shared_ptr<Derived>(new Derived(*static_cast<Derived*>(this)));
    np->m_system = &newSys;
    //copy the internals
    np->content = CBase::content->clone();
    //and get the geometry pointers right
    if(CBase::first) {
        GlobalVertex v = boost::static_pointer_cast<Geometry3D>(CBase::first)->template getProperty<vertex_prop>();
        np->first = newSys.m_cluster->template getObject<Geometry3D>(v);
    }
    if(CBase::second) {
        GlobalVertex v = boost::static_pointer_cast<Geometry3D>(CBase::second)->template getProperty<vertex_prop>();
        np->second = newSys.m_cluster->template getObject<Geometry3D>(v);
    }
    return np;
};

template<typename Typelist, typename ID>
template<typename Sys>
template<typename Derived>
Module3D<Typelist, ID>::type<Sys>::Constraint3D_id<Derived>::Constraint3D_id(Sys& system, Geom f, Geom s)
    : Constraint3D_base<Derived>(system, f, s) {

};

template<typename Typelist, typename ID>
template<typename Sys>
template<typename Derived>
typename Module3D<Typelist, ID>::template type<Sys>::Identifier&
Module3D<Typelist, ID>::type<Sys>::Constraint3D_id<Derived>::getIdentifier() {
    return  this->template getProperty<id_prop<Identifier> >();
};

template<typename Typelist, typename ID>
template<typename Sys>
template<typename Derived>
void Module3D<Typelist, ID>::type<Sys>::Constraint3D_id<Derived>::setIdentifier(Identifier id) {
    this->template setProperty<id_prop<Identifier> >(id);
};

template<typename Typelist, typename ID>
template<typename Sys>
Module3D<Typelist, ID>::type<Sys>::Constraint3D::Constraint3D(Sys& system, Geom first, Geom second)
    : mpl::if_<boost::is_same<Identifier, No_Identifier>,
      Constraint3D_base<Constraint3D>,
      Constraint3D_id<Constraint3D> >::type(system, first, second) {

};

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
    boost::function<void(GlobalEdge)> functor = boost::bind(&inheriter_base::apply_edge_remove, this, _1);
    m_this->m_cluster->removeVertex(v, functor);
    m_this->erase(g);
};

template<typename Typelist, typename ID>
template<typename Sys>
template<typename T1>
typename Module3D<Typelist, ID>::template type<Sys>::Cons
Module3D<Typelist, ID>::type<Sys>::inheriter_base::createConstraint3D(Geom first, Geom second, T1 constraint1) {

    //get the fusion vector type
    typedef typename fusion_vec<T1>::type covec;

    //set the objects
    covec cv = set_constraint_option()(constraint1);

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

}//dcm

#endif //GCM_GEOMETRY3D_H







