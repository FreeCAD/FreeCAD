/*
    openDCM, dimensional constraint manager
    Copyright (C) 2013  Stefan Troeger <stefantroeger@gmx.net>

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

#ifndef GCM_MODULE_SHAPE3D_H
#define GCM_MODULE_SHAPE3D_H

#include <opendcm/core.hpp>
#include <opendcm/core/geometry.hpp>
#include <opendcm/module3d.hpp>

#include "defines.hpp"
#include "geometry.hpp"
#include "generator.hpp"

#include <boost/mpl/if.hpp>
#include <boost/mpl/map.hpp>
#include <boost/type_traits.hpp>

#include <boost/preprocessor.hpp>
#include <boost/preprocessor/repetition/repeat.hpp>
#include <boost/preprocessor/cat.hpp>
#include <boost/preprocessor/repetition/enum.hpp>
#include <boost/preprocessor/repetition/enum_params.hpp>
#include <boost/preprocessor/repetition/enum_trailing_params.hpp>
#include <boost/preprocessor/repetition/enum_binary_params.hpp>


#define APPEND_SINGLE(z, n, data) \
	typedef typename Sys::Identifier Identifier; \
	typedef typename system_traits<Sys>::template getModule<details::m3d>::type::geometry_types gtypes; \
	typedef typename system_traits<Sys>::template getModule<details::mshape3d>::type::geometry_types stypes; \
    g_ptr = details::converter_g<BOOST_PP_CAT(Arg,n), Geometry3D>::template apply<gtypes, Sys, Identifier>(BOOST_PP_CAT(arg,n), m_this); \
    if(!g_ptr) { \
      hlg_ptr = details::converter_hlg<BOOST_PP_CAT(Arg,n), Shape3D>::template apply<stypes, Sys, Identifier>(BOOST_PP_CAT(arg,n), data, m_this); \
      if(!hlg_ptr) \
	throw creation_error() <<  boost::errinfo_errno(216) << error_message("could not handle input"); \
      else \
	data->append(hlg_ptr);\
    } \
    else { \
      data->append(g_ptr); \
    } \
 

#define CREATE_DEF(z, n, data) \
    template < \
    typename Generator  \
    BOOST_PP_ENUM_TRAILING_PARAMS(n, typename Arg) \
    > \
    boost::shared_ptr<Shape3D> createShape3D( \
                     BOOST_PP_ENUM_BINARY_PARAMS(n, Arg, const& arg) \
                   );

#define CREATE_DEC(z, n, data) \
    template<typename TypeList, typename ID> \
    template<typename Sys> \
    template <\
    typename Generator \
    BOOST_PP_ENUM_TRAILING_PARAMS(n, typename Arg)\
    > \
    boost::shared_ptr<typename ModuleShape3D<TypeList, ID>::template type<Sys>::Shape3D> \
    ModuleShape3D<TypeList, ID>::type<Sys>::inheriter_base::createShape3D( \
            BOOST_PP_ENUM_BINARY_PARAMS(n, Arg, const& arg) \
                                              ) \
    { \
      typedef typename system_traits<Sys>::template getModule<details::m3d>::type module3d; \
      typedef typename module3d::Geometry3D Geometry3D; \
      boost::shared_ptr<Geometry3D> g_ptr; \
      boost::shared_ptr<Shape3D> hlg_ptr; \
      boost::shared_ptr<Shape3D> ptr = boost::shared_ptr<Shape3D>(new Shape3D(*m_this)); \
      BOOST_PP_REPEAT(n, APPEND_SINGLE, ptr) \
      ptr->template initShape<Generator>();\
      m_this->push_back(ptr);\
      return ptr;\
    };


namespace mpl = boost::mpl;

namespace dcm {

namespace details {

//return always a geometry3d pointer struct, no matter whats the supplied type
template<typename T, typename R>
struct converter_g {
    //check if the type T is usable from within module3d, as it could also be a shape type
    template<typename gtypes, typename Sys, typename Identifier>
    static typename boost::enable_if<
    mpl::and_<
    mpl::not_< boost::is_same<typename mpl::find<gtypes, T>::type,typename mpl::end<gtypes>::type> >,
        mpl::not_<boost::is_same<Identifier, T> >
    >, boost::shared_ptr<R>  >::type  apply(T const& t, Sys* sys) {
        return sys->createGeometry3D(t);
    };

    //seems to be a shape type, return an empty geometry
    template<typename gtypes, typename Sys, typename Identifier>
    static typename boost::enable_if<
    mpl::and_<
    boost::is_same<typename mpl::find<gtypes, T>::type, typename mpl::end<gtypes>::type>,
          mpl::not_<boost::is_same<Identifier, T> >
    >, boost::shared_ptr<R>  >::type apply(T const& t, Sys* sys) {
        return boost::shared_ptr<R>();
    };

    //seems to be an identifier type, lets check if we have such a geometry
    template<typename gtypes, typename Sys, typename Identifier>
    static typename boost::enable_if<
    boost::is_same<Identifier, T>, boost::shared_ptr<R>  >::type  apply(T const& t, Sys* sys) {
        return sys->getGeometry3D(t);
    };
};

template<typename R>
struct converter_g< boost::shared_ptr<R>, R> {
    template<typename gtypes, typename Sys, typename Identifier>
    static boost::shared_ptr<R> apply(boost::shared_ptr<R> t, Sys* sys) {
        return t;
    };
};

template<typename T, typename R>
struct converter_hlg  {
    template<typename gtypes, typename Sys, typename Identifier>
    static typename boost::enable_if<
    mpl::and_<
    boost::is_same<typename mpl::find<gtypes, T>::type, typename mpl::end<gtypes>::type>,
          mpl::not_<boost::is_same<Identifier, T> >
          >,
    boost::shared_ptr<R>  >::type apply(T const& t, boost::shared_ptr<R> self, Sys* sys) {
        return  boost::shared_ptr<R>();
    };

    template<typename gtypes, typename Sys, typename Identifier>
    static typename boost::enable_if<
    mpl::not_< boost::is_same<typename mpl::find<gtypes, T>::type, typename mpl::end<gtypes>::type> >,
    boost::shared_ptr<R>  >::type apply(T const& t, boost::shared_ptr<R> self, Sys* sys) {

        //shape can only be set one time, throw an error otherwise
        if(self->holdsType())
            throw creation_error() <<  boost::errinfo_errno(410) << error_message("Shape can only be set with one geometry");

        self->set(t);
        return  self;
    };

    //seems to be an identifier type, lets check if we have such a geometry
    template<typename gtypes, typename Sys, typename Identifier>
    static typename boost::enable_if<
    boost::is_same<Identifier, T>, boost::shared_ptr<R>  >::type  apply(T const& t, boost::shared_ptr<R> self, Sys* sys) {
        return sys->getShape3D(t);
    };
};

template<typename R>
struct converter_hlg<boost::shared_ptr<R>, R>  {
    template<typename gtypes, typename Sys, typename Identifier>
    static boost::shared_ptr<R> apply(boost::shared_ptr<R> t, boost::shared_ptr<R> self, Sys* sys) {
        return  t;
    };
};

}//details


template<typename TypeList, typename ID = No_Identifier>
struct ModuleShape3D {

    template<typename Sys>
    struct type : details::mshape3d {

        typedef TypeList geometry_types;
        //forward declare
        struct inheriter_base;
        struct Shape3D;

        typedef mpl::map0<> ShapeSig;

        template<typename Derived>
        struct Shape3D_base : public details::Geometry<typename Sys::Kernel, 3, typename Sys::geometries>, public Object<Sys, Derived, ShapeSig > {

            typedef typename Sys::Kernel Kernel;
            typedef typename Kernel::number_type Scalar;

            //traits are only accessible in subclass scope
            BOOST_MPL_ASSERT((typename system_traits<Sys>::template getModule<details::m3d>::has_module));
            typedef typename  system_traits<Sys>::template getModule<details::m3d>::type module3d;
            typedef typename module3d::Geometry3D Geometry3D;
            typedef typename module3d::Constraint3D Constraint3D;

            Shape3D_base(Sys& system);

            template<typename T>
            Shape3D_base(const T& geometry, Sys& system);

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

            /*shape access functions*/
            typedef typename std::vector<boost::shared_ptr<Geometry3D> >::const_iterator geometry3d_iterator;
            typedef typename std::vector<boost::shared_ptr<Shape3D> >::const_iterator 	 shape3d_iterator;
            typedef typename std::vector<boost::shared_ptr<Constraint3D> >::const_iterator constraint3d_iterator;
            shape3d_iterator beginShape3D() 		{
                return m_shapes.begin();
            };
            shape3d_iterator endShape3D() 		{
                return m_shapes.end();
            };
            geometry3d_iterator beginGeometry3D()	{
                return m_geometries.begin();
            };
            geometry3d_iterator endGeometry3D()		{
                return m_geometries.end();
            };
            constraint3d_iterator beginConstraint3D()	{
                return m_constraints.begin();
            };
            constraint3d_iterator endConstraint3D()		{
                return m_constraints.end();
            };

            boost::shared_ptr<Geometry3D> geometry(purpose f);
            template<typename T>
            boost::shared_ptr<Shape3D> subshape();

            void recalc(boost::shared_ptr<Geometry3D> g);
        protected:

            typedef details::Geometry<typename Sys::Kernel, 3, typename Sys::geometries> Base;
            typedef Object<Sys, Derived, ShapeSig > ObjBase;
            typedef typename mpl::push_front<TypeList, boost::blank>::type ExtTypeList;
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
            boost::shared_ptr< details::ShapeGeneratorBase<Sys> > m_generator;

            using Object<Sys, Derived, mpl::map0<> >::m_system;

            std::vector<boost::shared_ptr<Geometry3D> > m_geometries;
            std::vector<boost::shared_ptr<Derived> > m_shapes;
            std::vector<boost::shared_ptr<Constraint3D> > m_constraints;

            template<typename generator>
            void initShape() {
                m_generator = boost::shared_ptr<details::ShapeGeneratorBase<Sys> >(new typename generator::template type<Sys>(m_system));
                m_generator->set(ObjBase::shared_from_this(), &m_geometries, &m_shapes, &m_constraints);

                if(!m_generator->check())
                    throw creation_error() <<  boost::errinfo_errno(210) << error_message("not all needd geometry for shape present");

                m_generator->init();
            };


            boost::shared_ptr<Derived> append(boost::shared_ptr<Geometry3D> g);
            boost::shared_ptr<Derived> append(boost::shared_ptr<Derived> g);

            //override protected event functions to emit signals
            void reset() {};
            void recalculated() {};
            void removed() {};


            friend struct inheriter_base;
            friend struct Object<Sys, Derived, mpl::map0<> >;
        };

        template<typename Derived>
        class Shape3D_id : public Shape3D_base<Derived> {

            typedef Shape3D_base<Derived> Base;

#ifdef USE_LOGGING
            attrs::mutable_constant< std::string > log_id;
#endif
        public:
            Shape3D_id(Sys& system);

            template<typename T>
            Shape3D_id(const T& geometry, Sys& system);

            template<typename T>
            void set(const T& geometry, ID id);
            //somehow the base class set funtion is not found
            template<typename T>
            void set(const T& geometry);

            ID& getIdentifier();
            void setIdentifier(ID id);
        };

        struct Shape3D : public mpl::if_<boost::is_same<ID, No_Identifier>,
                Shape3D_base<Shape3D>, Shape3D_id<Shape3D> >::type {

            Shape3D(Sys& system);

            template<typename T>
            Shape3D(const T& geometry, Sys& system);

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

        //inheriter for own functions
        struct inheriter_base {

            inheriter_base() {
                m_this = (Sys*)this;
            };

        protected:
            Sys* m_this;

        public:
            //with no vararg templates before c++11 we need preprocessor to create the overloads of create we need
            BOOST_PP_REPEAT(5, CREATE_DEF, ~)

            void removeShape3D(boost::shared_ptr<Shape3D> g);
        };

        struct inheriter_id : public inheriter_base {
            //we don't have a createshape3d method with identifier, as identifiers can be used to
            //specifie creation geometries or shapes. therefore a call would always be ambigious.

            void removeShape3D(ID id);
            bool hasShape3D(ID id);
            boost::shared_ptr<Shape3D> getShape3D(ID id);
	    
	protected:
	    using inheriter_base::m_this;
        };

        struct inheriter : public mpl::if_<boost::is_same<ID, No_Identifier>, inheriter_base, inheriter_id>::type {};

        //add properties to geometry and constraint to evaluate their shape partipance
        struct shape_purpose_prop {
            typedef purpose type;
            typedef typename system_traits<Sys>::template getModule<details::m3d>::type::Geometry3D kind;
        };
        struct shape_geometry_prop {
            typedef bool type;
            typedef typename system_traits<Sys>::template getModule<details::m3d>::type::Geometry3D kind;
        };
        struct shape_constraint_prop {
            typedef bool type;
            typedef typename system_traits<Sys>::template getModule<details::m3d>::type::Constraint3D kind;
        };

        //needed typedefs
        typedef ID Identifier;
        typedef mpl::vector3<shape_purpose_prop, shape_constraint_prop, shape_geometry_prop> properties;
        typedef mpl::vector1<Shape3D> objects;
        typedef mpl::vector1<tag::segment3D> geometries;

        //needed static functions
        static void system_init(Sys& sys) {};
        static void system_copy(const Sys& from, Sys& into) {};

    };
};

/*****************************************************************************************************************/
/*****************************************************************************************************************/
/*****************************************************************************************************************/
/*****************************************************************************************************************/

BOOST_PP_REPEAT(5, CREATE_DEC, ~)

template<typename Typelist, typename ID>
template<typename Sys>
template<typename Derived>
ModuleShape3D<Typelist, ID>::type<Sys>::Shape3D_base<Derived>::Shape3D_base(Sys& system)
    : Object<Sys, Derived, ShapeSig>(system) {

#ifdef USE_LOGGING
    log.add_attribute("Tag", attrs::constant< std::string >("Geometry3D"));
#endif
};

template<typename Typelist, typename ID>
template<typename Sys>
template<typename Derived>
template<typename T>
ModuleShape3D<Typelist, ID>::type<Sys>::Shape3D_base<Derived>::Shape3D_base(const T& geometry, Sys& system)
    : Object<Sys, Derived, ShapeSig>(system) {

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
void ModuleShape3D<Typelist, ID>::type<Sys>::Shape3D_base<Derived>::set(const T& geometry) {

    m_geometry = geometry;
    //first init, so that the geometry internal vector has the right size
    this->template init< typename geometry_traits<T>::tag >();
    //now write the value;
    (typename geometry_traits<T>::modell()).template extract<Scalar,
    typename geometry_traits<T>::accessor >(geometry, this->getValue());

    reset();
};

template<typename Typelist, typename ID>
template<typename Sys>
template<typename Derived>
boost::shared_ptr<Derived> ModuleShape3D<Typelist, ID>::type<Sys>::Shape3D_base<Derived>::clone(Sys& newSys) {

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
boost::shared_ptr<typename system_traits<Sys>::template getModule<details::m3d>::type::Geometry3D>
ModuleShape3D<Typelist, ID>::type<Sys>::Shape3D_base<Derived>::geometry(purpose f) {

    for(geometry3d_iterator it = m_geometries.begin(); it != m_geometries.end(); it++) {

        if((*it)->template getProperty<shape_purpose_prop>() == f)
            return *it;
    };
    return boost::shared_ptr<Geometry3D>();
};

template<typename Typelist, typename ID>
template<typename Sys>
template<typename Derived>
boost::shared_ptr<Derived>
ModuleShape3D<Typelist, ID>::type<Sys>::Shape3D_base<Derived>::append(boost::shared_ptr<Geometry3D> g) {
    m_geometries.push_back(g);
    g->template setProperty<shape_geometry_prop>(true);
    return ObjBase::shared_from_this();
};


template<typename Typelist, typename ID>
template<typename Sys>
template<typename Derived>
boost::shared_ptr<Derived>
ModuleShape3D<Typelist, ID>::type<Sys>::Shape3D_base<Derived>::append(boost::shared_ptr<Derived> g) {
    m_shapes.push_back(g);
    return ObjBase::shared_from_this();
};

template<typename Typelist, typename ID>
template<typename Sys>
template<typename Derived>
void ModuleShape3D<Typelist, ID>::type<Sys>::Shape3D_base<Derived>::recalc(boost::shared_ptr<Geometry3D> g) {

    //we recalculated thebase line, that means we have our new value. use it.
    Base::finishCalculation();
};

template<typename Typelist, typename ID>
template<typename Sys>
template<typename Derived>
ModuleShape3D<Typelist, ID>::type<Sys>::Shape3D_id<Derived>::Shape3D_id(Sys& system)
    : ModuleShape3D<Typelist, ID>::template type<Sys>::template Shape3D_base<Derived>(system)
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
ModuleShape3D<Typelist, ID>::type<Sys>::Shape3D_id<Derived>::Shape3D_id(const T& geometry, Sys& system)
    : ModuleShape3D<Typelist, ID>::template type<Sys>::template Shape3D_base<Derived>(geometry, system)
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
void ModuleShape3D<Typelist, ID>::type<Sys>::Shape3D_id<Derived>::set(const T& geometry, Identifier id) {
    this->template setProperty<id_prop<Identifier> >(id);
    Base::set(geometry);
};

template<typename Typelist, typename ID>
template<typename Sys>
template<typename Derived>
template<typename T>
void ModuleShape3D<Typelist, ID>::type<Sys>::Shape3D_id<Derived>::set(const T& geometry) {
    Base::set(geometry);
};

template<typename Typelist, typename ID>
template<typename Sys>
template<typename Derived>
typename ModuleShape3D<Typelist, ID>::template type<Sys>::Identifier&
ModuleShape3D<Typelist, ID>::type<Sys>::Shape3D_id<Derived>::getIdentifier() {
    return  this->template getProperty<id_prop<Identifier> >();
};

template<typename Typelist, typename ID>
template<typename Sys>
template<typename Derived>
void ModuleShape3D<Typelist, ID>::type<Sys>::Shape3D_id<Derived>::setIdentifier(Identifier id) {
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
ModuleShape3D<Typelist, ID>::type<Sys>::Shape3D::Shape3D(Sys& system)
    : mpl::if_<boost::is_same<Identifier, No_Identifier>,
      Shape3D_base<Shape3D>,
      Shape3D_id<Shape3D> >::type(system) {

};

template<typename Typelist, typename ID>
template<typename Sys>
template<typename T>
ModuleShape3D<Typelist, ID>::type<Sys>::Shape3D::Shape3D(const T& geometry, Sys& system)
    : mpl::if_<boost::is_same<Identifier, No_Identifier>,
      Shape3D_base<Shape3D>,
      Shape3D_id<Shape3D> >::type(geometry, system) {

};

template<typename Typelist, typename ID>
template<typename Sys>
void ModuleShape3D<Typelist, ID>::type<Sys>::inheriter_base::removeShape3D(boost::shared_ptr<Shape3D> g) {

    //remove all constraints
    typedef typename Shape3D::constraint3d_iterator cit;
    for(cit it=g->constraint3dBegin(); it!=g->constraint3dEnd(); it++)
        m_this->removeConstraint3D(*it);

    //remove all geometries
    typedef typename Shape3D::geometry3d_iterator git;
    for(git it=g->geometry3dBegin(); it!=g->geometry3dEnd(); it++)
        m_this->removeGeometry3D(*it);

    //remove all subshapes
    typedef typename Shape3D::shape3d_iterator sit;
    for(sit it=g->shape3dBegin(); it!=g->shape3dEnd(); it++)
        m_this->removeShape3D(*it);

    //emit remove shape signal bevore actually deleting it
    g->template emitSignal<remove>(g);
    m_this->erase(g);
};

template<typename Typelist, typename ID>
template<typename Sys>
bool ModuleShape3D<Typelist, ID>::type<Sys>::inheriter_id::hasShape3D(Identifier id) {
    if(getShape3D(id))
        return true;
    return false;
};

template<typename Typelist, typename ID>
template<typename Sys>
boost::shared_ptr<typename ModuleShape3D<Typelist, ID>::template type<Sys>::Shape3D>
ModuleShape3D<Typelist, ID>::type<Sys>::inheriter_id::getShape3D(Identifier id) {
    std::vector< boost::shared_ptr<Shape3D> >& vec = inheriter_base::m_this->template objectVector<Shape3D>();
    typedef typename std::vector< boost::shared_ptr<Shape3D> >::iterator iter;
    for(iter it=vec.begin(); it!=vec.end(); it++) {
        if(compare_traits<Identifier>::compare((*it)->getIdentifier(), id))
            return *it;
    };
    return  boost::shared_ptr<Shape3D>();
};

template<typename Typelist, typename ID>
template<typename Sys>
void ModuleShape3D<Typelist, ID>::type<Sys>::inheriter_id::removeShape3D(Identifier id) {

    boost::shared_ptr<Shape3D> s = getShape3D(id);
    if(s)
        removeShape3D(s);
};

}//dcm

#endif //GCM_MODULE_SHAPE3D_H
