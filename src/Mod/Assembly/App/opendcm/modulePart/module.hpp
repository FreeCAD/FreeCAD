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

#ifndef GCM_MODULE_PART_H
#define GCM_MODULE_PART_H

#include "opendcm/core.hpp"
#include "opendcm/core/traits.hpp"
#include "opendcm/core/clustergraph.hpp"
#include "opendcm/core/property.hpp"
#include "opendcm/module3d.hpp"

#include <boost/mpl/assert.hpp>
#include <boost/utility/enable_if.hpp>

namespace mpl = boost::mpl;

namespace dcm {

enum { clusterPart = 110};

enum CoordinateFrame {Local, Global};

template<typename Typelist, typename ID = No_Identifier>
struct ModulePart {

    template<typename Sys>
    struct type {


        struct Part;
        struct PrepareCluster;
        struct EvaljuateCluster;
        typedef boost::shared_ptr<Part> Partptr;
        typedef mpl::map2< mpl::pair<remove, boost::function<void (Partptr) > >,
                mpl::pair<recalculated, boost::function<void (Partptr) > > >  PartSignal;

        typedef ID Identifier;
        typedef mpl::map1<mpl::pair<recalculated, boost::function<void (boost::shared_ptr<Sys>)> > > signals;

        class Part_base : public Object<Sys, Part, PartSignal > {
        protected:

#ifdef USE_LOGGING
            dcm_logger log;
#endif

            //check if we have module3d in this system
            typedef typename system_traits<Sys>::template getModule<details::m3d>::type module3d;
            BOOST_MPL_ASSERT((mpl::not_<boost::is_same<module3d, mpl::void_> >));

            //define what we need
            typedef typename module3d::Geometry3D Geometry3D;
            typedef boost::shared_ptr<Geometry3D> Geom;

            typedef typename boost::make_variant_over< Typelist >::type Variant;
            typedef Object<Sys, Part, PartSignal> base;
            typedef typename Sys::Kernel Kernel;
            typedef typename Sys::Cluster Cluster;
            typedef typename Kernel::number_type Scalar;
            typedef typename Kernel::Transform3D Transform;

            struct cloner : boost::static_visitor<void> {

                Variant variant;
                cloner(Variant& v) : variant(v) {};

                template<typename T>
                void operator()(T& t) {
                    variant = geometry_clone_traits<T>()(t);
                };
            };

            //visitor to write the calculated value into the variant
            struct apply_visitor : public boost::static_visitor<void> {

                apply_visitor(Transform& t) : m_transform(t) {};

                template <typename T>
                void operator()(T& t) const  {
                    (typename geometry_traits<T>::modell()).template inject<Kernel,
                    typename geometry_traits<T>::accessor >(t, m_transform);
                }
                Transform& m_transform;
            };

            //collect all clustergraph upstream cluster transforms
            void transform_traverse(Transform& t, boost::shared_ptr<Cluster> c);

        public:
            using Object<Sys, Part, PartSignal >::m_system;

            template<typename T>
            Part_base(const T& geometry, Sys& system, boost::shared_ptr<Cluster> cluster);

            template<typename Visitor>
            typename Visitor::result_type apply(Visitor& vis);

            template<typename T>
            Geom addGeometry3D(const T& geom, CoordinateFrame frame = Global);

            template<typename T>
            void set(const T& geometry);

            template<typename T>
            T& get();

            template<typename T>
            T getGlobal();

            virtual boost::shared_ptr<Part> clone(Sys& newSys);

        public:
            Variant 	m_geometry;
            Transform 	m_transform;
            boost::shared_ptr<Cluster> 	m_cluster;

            void finishCalculation();
            void fix(bool fix_value);

        public:
            //we hold a transform and need therefore a aligned new operator
            EIGEN_MAKE_ALIGNED_OPERATOR_NEW
        };

        struct Part_id : public Part_base {

            template<typename T>
            Part_id(const T& geometry, Sys& system,  boost::shared_ptr<typename Part_base::Cluster> cluster);

            template<typename T>
            typename Part_base::Geom addGeometry3D(const T& geom, Identifier id, CoordinateFrame frame = Global);

            template<typename T>
            void set(const T& geometry, Identifier id);

            bool hasGeometry3D(Identifier id);
            typename Part_base::Geom getGeometry3D(Identifier id);

            Identifier& getIdentifier();
            void setIdentifier(Identifier id);
        };

        struct Part : public mpl::if_<boost::is_same<Identifier, No_Identifier>, Part_base, Part_id>::type {

            typedef typename mpl::if_<boost::is_same<Identifier, No_Identifier>, Part_base, Part_id>::type base;

            template<typename T>
            Part(const T& geometry, Sys& system, boost::shared_ptr<typename base::Cluster> cluster);

            friend struct PrepareCluster;
            friend struct EvaljuateCluster;

        public:
            //we hold a transform and need therefore a aligned new operator
            EIGEN_MAKE_ALIGNED_OPERATOR_NEW
        };


        struct inheriter_base {

            inheriter_base();

            template<typename T>
            Partptr createPart(const T& geometry);
            void removePart(Partptr p);

            template<typename T>
            void setTransformation(const T& geom) {

                typedef typename system_traits<Sys>::template getModule<details::m3d>::type module3d;
                details::ClusterMath<Sys>& cm = ((Sys*)this)->m_cluster->template getProperty<typename module3d::math_prop>();

                (typename geometry_traits<T>::modell()).template extract<typename Sys::Kernel,
                typename geometry_traits<T>::accessor >(geom, cm.getTransform());
            };

            template<typename T>
            T getTransformation() {

                T geom;
                getTransformation(geom);
                return geom;
            };
            template<typename T>
            void getTransformation(T& geom) {

                typedef typename system_traits<Sys>::template getModule<details::m3d>::type module3d;
                details::ClusterMath<Sys>& cm = ((Sys*)this)->m_cluster->template getProperty<typename module3d::math_prop>();

                (typename geometry_traits<T>::modell()).template inject<typename Sys::Kernel,
                typename geometry_traits<T>::accessor >(geom, cm.getTransform());
            };

            //needed system functions
            void system_sub(boost::shared_ptr<Sys> subsys) {};

        protected:
            Sys* m_this;

            //function object to emit remove signal too al geometry which is deleted by part deletion
            struct remover {
                typedef typename Sys::Cluster Cluster;
                typedef typename system_traits<Sys>::template getModule<details::m3d>::type module3d;
                typedef typename module3d::Geometry3D Geometry3D;
                typedef boost::shared_ptr<Geometry3D> Geom;
                typedef typename module3d::Constraint3D Constraint3D;
                typedef boost::shared_ptr<Constraint3D> Cons;

                Sys& system;
                remover(Sys& s);
                //see if we have a geometry or a constraint and emit the remove signal
                void operator()(GlobalVertex v);
                //we delete all global edges connecting to this part
                void operator()(GlobalEdge e);
                void operator()(boost::shared_ptr<Cluster> g) {};
            };
        };

        struct inheriter_id : public inheriter_base {

            template<typename T>
            Partptr createPart(const T& geometry, Identifier id);
            bool hasPart(Identifier id);
            Partptr getPart(Identifier id);
        };

        struct inheriter : public mpl::if_<boost::is_same<Identifier, No_Identifier>, inheriter_base, inheriter_id>::type {};

        struct subsystem_property {
            typedef Sys* type;
            typedef cluster_property kind;
        };

        typedef mpl::vector1<subsystem_property>  properties;
        typedef mpl::vector1<Part>  objects;
        typedef mpl::vector0<>  geometries;

        struct PrepareCluster : public Job<Sys> {

            typedef typename Sys::Cluster Cluster;
            typedef typename Sys::Kernel Kernel;
            typedef typename system_traits<Sys>::template getModule<details::m3d>::type module3d;

            PrepareCluster();
            virtual void execute(Sys& sys);
        };

        struct EvaljuateCluster : public Job<Sys> {

            typedef typename Sys::Cluster Cluster;
            typedef typename Sys::Kernel Kernel;
            typedef typename system_traits<Sys>::template getModule<details::m3d>::type module3d;

            EvaljuateCluster();
            virtual void execute(Sys& sys);
        };

        static void system_init(Sys& sys) {
            sys.m_sheduler.addPreprocessJob(new PrepareCluster());
            sys.m_sheduler.addPostprocessJob(new EvaljuateCluster());
        };
        static void system_copy(const Sys& from, Sys& into) {};
    };
};

template<typename Typelist, typename ID>
template<typename Sys>
template<typename T>
ModulePart<Typelist, ID>::type<Sys>::Part_base::Part_base(const T& geometry, Sys& system, boost::shared_ptr<Cluster> cluster)
    : Object<Sys, Part, PartSignal>(system), m_geometry(geometry), m_cluster(cluster)  {

#ifdef USE_LOGGING
    log.add_attribute("Tag", attrs::constant< std::string >("Part3D"));
#endif

    (typename geometry_traits<T>::modell()).template extract<Kernel,
    typename geometry_traits<T>::accessor >(geometry, m_transform);

    cluster->template setProperty<typename module3d::fix_prop>(false);

    //the the clustermath transform
    m_cluster->template getProperty<typename module3d::math_prop>().getTransform() = m_transform;

#ifdef USE_LOGGING
    BOOST_LOG_SEV(log, information) << "Init: "<<m_transform;
#endif
};

template<typename Typelist, typename ID>
template<typename Sys>
template<typename Visitor>
typename Visitor::result_type ModulePart<Typelist, ID>::type<Sys>::Part_base::apply(Visitor& vis) {
    return boost::apply_visitor(vis, m_geometry);
};

template<typename Typelist, typename ID>
template<typename Sys>
template<typename T>
typename ModulePart<Typelist, ID>::template type<Sys>::Part_base::Geom
ModulePart<Typelist, ID>::type<Sys>::Part_base::addGeometry3D(const T& geom, CoordinateFrame frame) {
    Geom g(new Geometry3D(geom, *m_system));

    if(frame == Local) {
        //we need to collect all transforms up to this part!
        Transform t;
        transform_traverse(t, m_cluster);

        g->transform(t);
    }

    fusion::vector<LocalVertex, GlobalVertex> res = m_cluster->addVertex();
    m_cluster->template setObject<Geometry3D> (fusion::at_c<0> (res), g);
    g->template setProperty<typename module3d::vertex_prop>(fusion::at_c<1>(res));
    m_system->template objectVector<Geometry3D>().push_back(g);

    return g;
};

template<typename Typelist, typename ID>
template<typename Sys>
void ModulePart<Typelist, ID>::type<Sys>::Part_base::transform_traverse(typename ModulePart<Typelist, ID>::template type<Sys>::Part_base::Transform& t,
        boost::shared_ptr<typename ModulePart<Typelist, ID>::template type<Sys>::Part_base::Cluster> c) {

    t *= c->template getProperty<typename Part_base::module3d::math_prop>().m_transform;

    if(c->isRoot())
        return;

    transform_traverse(t, c->parent());
}

template<typename Typelist, typename ID>
template<typename Sys>
template<typename T>
void ModulePart<Typelist, ID>::type<Sys>::Part_base::set(const T& geometry) {
    Part_base::m_geometry = geometry;
    (typename geometry_traits<T>::modell()).template extract<Kernel,
    typename geometry_traits<T>::accessor >(geometry, Part_base::m_transform);

    //set the clustermath transform
    m_cluster->template getClusterProperty<typename module3d::math_prop>().getTransform() = m_transform;

};

template<typename Typelist, typename ID>
template<typename Sys>
template<typename T>
T& ModulePart<Typelist, ID>::type<Sys>::Part_base::get() {

    return get<T>(this);
};

template<typename Typelist, typename ID>
template<typename Sys>
template<typename T>
T ModulePart<Typelist, ID>::type<Sys>::Part_base::getGlobal() {

    //get the successive transform
    Transform t;
    transform_traverse(t, m_cluster);

    //put it into the user type
    T ut;
    (typename geometry_traits<T>::modell()).template inject<Kernel,
    typename geometry_traits<T>::accessor >(ut, t);

    return ut;
};

template<typename Typelist, typename ID>
template<typename Sys>
boost::shared_ptr<typename ModulePart<Typelist, ID>::template type<Sys>::Part>
ModulePart<Typelist, ID>::type<Sys>::Part_base::clone(Sys& newSys) {

    //we need to reset the cluster pointer to the new system cluster
    LocalVertex  lv = Object<Sys, Part, PartSignal>::m_system->m_cluster->getClusterVertex(m_cluster);
    GlobalVertex gv = Object<Sys, Part, PartSignal>::m_system->m_cluster->getGlobalVertex(lv);

    boost::shared_ptr<Part> np = Object<Sys, Part, PartSignal>::clone(newSys);
    //there may be  pointer inside the variant
    cloner clone_fnc(np->m_geometry);
    boost::apply_visitor(clone_fnc, m_geometry);

    fusion::vector<LocalVertex, boost::shared_ptr<Cluster>, bool> res = newSys.m_cluster->getLocalVertexGraph(gv);

    if(!fusion::at_c<2>(res)) {
        //todo: throw
        return np;
    }

    np->m_cluster = fusion::at_c<1>(res)->getVertexCluster(fusion::at_c<0>(res));

    return np;
};

template<typename Typelist, typename ID>
template<typename Sys>
void ModulePart<Typelist, ID>::type<Sys>::Part_base::finishCalculation() {

    m_transform.normalize();
    apply_visitor vis(m_transform);
    apply(vis);

#ifdef USE_LOGGING
    BOOST_LOG_SEV(log, manipulation) << "New Value: "<<m_transform;
#endif

    //emit the signal for new values
    base::template emitSignal<recalculated>(((Part*)this)->shared_from_this());
};

template<typename Typelist, typename ID>
template<typename Sys>
void ModulePart<Typelist, ID>::type<Sys>::Part_base::fix(bool fix_value) {
    m_cluster->template setProperty<typename module3d::fix_prop>(fix_value);
};

template<typename Typelist, typename ID>
template<typename Sys>
template<typename T>
ModulePart<Typelist, ID>::type<Sys>::Part_id::Part_id(const T& geometry, Sys& system,  boost::shared_ptr<typename Part_base::Cluster> cluster)
    : Part_base(geometry, system, cluster) {

};

template<typename Typelist, typename ID>
template<typename Sys>
template<typename T>
typename ModulePart<Typelist, ID>::template type<Sys>::Part_base::Geom
ModulePart<Typelist, ID>::type<Sys>::Part_id::addGeometry3D(const T& geom, Identifier id, CoordinateFrame frame) {

    typename Part_base::Geom g = Part_base::addGeometry3D(geom, frame);
    g->setIdentifier(id);
    return g;
};

template<typename Typelist, typename ID>
template<typename Sys>
template<typename T>
void ModulePart<Typelist, ID>::type<Sys>::Part_id::set(const T& geometry, Identifier id) {
    Part_base::set(geometry);
    setIdentifier(id);
};

template<typename Typelist, typename ID>
template<typename Sys>
bool ModulePart<Typelist, ID>::type<Sys>::Part_id::hasGeometry3D(Identifier id) {
    typename Part_base::Geom g = Part_base::m_system->getGeometry3D(id);

    if(!g)
        return false;

    //get the global vertex and check if it is a child of the part cluster
    GlobalVertex v = g->template getProperty<typename Part_base::module3d::vertex_prop>();
    return Part_base::m_cluster->getLocalVertex(v).second;
};

template<typename Typelist, typename ID>
template<typename Sys>
typename ModulePart<Typelist, ID>::template type<Sys>::Part_base::Geom
ModulePart<Typelist, ID>::type<Sys>::Part_id::getGeometry3D(Identifier id) {
    return Part_base::m_system->getGeometry3D(id);
};

template<typename Typelist, typename ID>
template<typename Sys>
typename ModulePart<Typelist, ID>::template type<Sys>::Identifier&
ModulePart<Typelist, ID>::type<Sys>::Part_id::getIdentifier() {
    return  this->template getProperty<id_prop<Identifier> >();
};

template<typename Typelist, typename ID>
template<typename Sys>
void ModulePart<Typelist, ID>::type<Sys>::Part_id::setIdentifier(Identifier id) {
    this->template setProperty<id_prop<Identifier> >(id);
};

template<typename Typelist, typename ID>
template<typename Sys>
template<typename T>
ModulePart<Typelist, ID>::type<Sys>::Part::Part(const T& geometry, Sys& system, boost::shared_ptr<typename base::Cluster> cluster)
    : mpl::if_<boost::is_same<Identifier, No_Identifier>,
      Part_base, Part_id>::type(geometry, system, cluster) {

};

template<typename Typelist, typename ID>
template<typename Sys>
ModulePart<Typelist, ID>::type<Sys>::inheriter_base::inheriter_base() {
    m_this = ((Sys*) this);
};

template<typename Typelist, typename ID>
template<typename Sys>
template<typename T>
typename ModulePart<Typelist, ID>::template type<Sys>::Partptr
ModulePart<Typelist, ID>::type<Sys>::inheriter_base::createPart(const T& geometry) {

    typedef typename Sys::Cluster Cluster;
    std::pair<boost::shared_ptr<Cluster>, LocalVertex>  res = m_this->m_cluster->createCluster();
    Partptr p(new Part(geometry, * ((Sys*) this), res.first));

    m_this->m_cluster->template setObject<Part> (res.second, p);
    m_this->push_back(p);

    res.first->template setProperty<type_prop>(clusterPart);
    return p;
};

template<typename Typelist, typename ID>
template<typename Sys>
void ModulePart<Typelist, ID>::type<Sys>::inheriter_base::removePart(Partptr p) {

    remover r(*m_this);
    m_this->m_cluster->removeCluster(p->m_cluster, r);
    p->template emitSignal<remove>(p);
    m_this->erase(p);
};

template<typename Typelist, typename ID>
template<typename Sys>
ModulePart<Typelist, ID>::type<Sys>::inheriter_base::remover::remover(Sys& s) : system(s) {

};

template<typename Typelist, typename ID>
template<typename Sys>
void ModulePart<Typelist, ID>::type<Sys>::inheriter_base::remover::operator()(GlobalVertex v) {
    Geom g = system.m_cluster->template getObject<Geometry3D>(v);

    if(g) {
        g->template emitSignal<remove>(g);
        system.erase(g);
    }

    Cons c = system.m_cluster->template getObject<Constraint3D>(v);

    if(c) {
        c->template emitSignal<remove>(c);
        system.erase(c);
    }
};

template<typename Typelist, typename ID>
template<typename Sys>
void ModulePart<Typelist, ID>::type<Sys>::inheriter_base::remover::operator()(GlobalEdge e) {
    Cons c = system.m_cluster->template getObject<Constraint3D>(e);

    if(c) {
        c->template emitSignal<remove>(c);
        system.erase(c);
    }
};

template<typename Typelist, typename ID>
template<typename Sys>
template<typename T>
typename ModulePart<Typelist, ID>::template type<Sys>::Partptr
ModulePart<Typelist, ID>::type<Sys>::inheriter_id::createPart(const T& geometry, Identifier id) {
    Partptr p = inheriter_base::createPart(geometry);
    p->setIdentifier(id);
    return p;
};

template<typename Typelist, typename ID>
template<typename Sys>
bool ModulePart<Typelist, ID>::type<Sys>::inheriter_id::hasPart(Identifier id) {
    if(getPart(id))
        return true;

    return false;
};

template<typename Typelist, typename ID>
template<typename Sys>
typename ModulePart<Typelist, ID>::template type<Sys>::Partptr
ModulePart<Typelist, ID>::type<Sys>::inheriter_id::getPart(Identifier id) {
    std::vector< Partptr >& vec = inheriter_base::m_this->template objectVector<Part>();
    typedef typename std::vector<Partptr>::iterator iter;

    for(iter it=vec.begin(); it!=vec.end(); it++) {
        if(compare_traits<Identifier>::compare((*it)->getIdentifier(), id))
            return *it;
    };

    return Partptr();
};

template<typename Typelist, typename ID>
template<typename Sys>
ModulePart<Typelist, ID>::type<Sys>::PrepareCluster::PrepareCluster() {
    Job<Sys>::priority = 1000;
};

template<typename Typelist, typename ID>
template<typename Sys>
void ModulePart<Typelist, ID>::type<Sys>::PrepareCluster::execute(Sys& sys) {
    //get all parts and set their values to the cluster's
    typedef typename std::vector<Partptr>::iterator iter;

    for(iter it = sys.template begin<Part>(); it != sys.template end<Part>(); it++) {

        details::ClusterMath<Sys>& cm = (*it)->m_cluster->template getProperty<typename module3d::math_prop>();
        cm.getTransform() = (*it)->m_transform;
    };
};

template<typename Typelist, typename ID>
template<typename Sys>
ModulePart<Typelist, ID>::type<Sys>::EvaljuateCluster::EvaljuateCluster() {
    Job<Sys>::priority = 1000;
};

template<typename Typelist, typename ID>
template<typename Sys>
void ModulePart<Typelist, ID>::type<Sys>::EvaljuateCluster::execute(Sys& sys) {
    //get all parts and set their values to the cluster's
    typedef typename std::vector<Partptr>::iterator iter;

    for(iter it = sys.template begin<Part>(); it != sys.template end<Part>(); it++) {

        details::ClusterMath<Sys>& cm = (*it)->m_cluster->template getProperty<typename module3d::math_prop>();
        (*it)->m_transform =  cm.getTransform();
        (*it)->finishCalculation();
    };

    //get all subsystems and report their recalculation
    typedef typename std::vector<boost::shared_ptr<Sys> >::iterator siter;

    for(siter it = sys.beginSubsystems(); it != sys.endSubsystems(); it++) {

        (*it)->template emitSignal<recalculated>(*it);
    };
};

}

#endif //GCM_MODULEPART_H




