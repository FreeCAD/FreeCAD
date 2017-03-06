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

#ifndef DCM_MODULE_3D_H
#define DCM_MODULE_3D_H

#include <boost/mpl/vector.hpp>
#include <boost/mpl/map.hpp>
#include <boost/mpl/if.hpp>

#include <boost/shared_ptr.hpp>
#include <boost/variant.hpp>
#include <boost/function.hpp>

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

struct reset {}; 	//signal name
struct module3d_error : virtual boost::exception {}; //exception for all module3d special errors

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
            typename Visitor::result_type apply(Visitor& vis);

            template<typename T>
            T convertTo();

            virtual boost::shared_ptr<Derived> clone(Sys& newSys);

        protected:
            typedef typename mpl::push_front<Typelist, boost::blank>::type ExtTypeList;
            typedef typename boost::make_variant_over< ExtTypeList >::type Variant;

#ifdef USE_LOGGING
            src::logger log;
#endif
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
            //somehow the base class set function is not found
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

            inheriter_base();

            template<typename T>
            Geom createGeometry3D(T geom);
            Geom createGeometry3D();
            void removeGeometry3D(Geom g);

            template<typename T1>
            Cons createConstraint3D(Geom first, Geom second, T1 constraint1);
            void removeConstraint3D(Cons c);

            void system_sub(boost::shared_ptr<Sys> subsys) {};

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

        typedef mpl::vector6<vertex_prop, edge_prop, math_prop,
                fix_prop, solverfailure, subsystemsolving>  properties;
        typedef mpl::vector2<Geometry3D, Constraint3D> objects;
        typedef mpl::vector5<tag::point3D, tag::direction3D, tag::line3D, tag::plane3D, tag::cylinder3D> geometries;
        typedef mpl::map0<> signals;

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

}//dcm

#include "imp/module_imp.hpp"

#endif //DCM_GEOMETRY3D_H







