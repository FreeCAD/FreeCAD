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

#include <boost/fusion/include/make_vector.hpp>

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

        typedef mpl::map2<
        mpl::pair<remove, boost::function<void (boost::shared_ptr<Shape3D>) > >,
            mpl::pair<remove, boost::function<void (boost::shared_ptr<Shape3D>) > > > ShapeSig;

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

            /*shape access functions and extractors to mimic vector<> iterators*/
            typedef std::vector<fusion::vector<boost::shared_ptr<Geometry3D>, Connection> > GeometryVector;
            typedef std::vector<fusion::vector<boost::shared_ptr<Derived>, Connection> > ShapeVector;
            typedef std::vector<fusion::vector<boost::shared_ptr<Constraint3D>, Connection> > ConstraintVector;

            struct geom_extractor  {
                typedef boost::shared_ptr<Geometry3D> result_type;
                template<typename T>
                result_type operator()(T& pair) const {
                    return fusion::at_c<0>(pair);
                };
            };
            struct shape_extractor  {
                typedef boost::shared_ptr<Shape3D> result_type;
                template<typename T>
                result_type operator()(T& pair) const {
                    return fusion::at_c<0>(pair);
                };
            };
            struct cons_extractor  {
                typedef boost::shared_ptr<Constraint3D> result_type;
                template<typename T>
                result_type operator()(T& pair) const {
                    return fusion::at_c<0>(pair);
                };
            };
            typedef boost::transform_iterator<geom_extractor, typename GeometryVector::const_iterator> geometry3d_iterator;
            typedef boost::transform_iterator<shape_extractor, typename ShapeVector::iterator >  shape3d_iterator;
            typedef boost::transform_iterator<cons_extractor, typename ConstraintVector::iterator > constraint3d_iterator;

            shape3d_iterator beginShape3D() 		{
                return boost::make_transform_iterator(m_shapes.begin(), shape_extractor());
            };
            shape3d_iterator endShape3D() 		{
                return boost::make_transform_iterator(m_shapes.end(), shape_extractor());
            };
            geometry3d_iterator beginGeometry3D()	{
                return boost::make_transform_iterator(m_geometries.begin(), geom_extractor());
            };
            geometry3d_iterator endGeometry3D()		{
                return boost::make_transform_iterator(m_geometries.end(), geom_extractor());
            };
            constraint3d_iterator beginConstraint3D()	{
                return boost::make_transform_iterator(m_constraints.begin(), cons_extractor());
            };
            constraint3d_iterator endConstraint3D()	{
                return boost::make_transform_iterator(m_constraints.end(), cons_extractor());
            };

            boost::shared_ptr<Geometry3D> geometry(purpose f);
            template<typename T>
            boost::shared_ptr<Shape3D> subshape();

            //callbacks
            void recalc(boost::shared_ptr<Geometry3D> g);
            void remove(boost::shared_ptr<Geometry3D> g);
            void remove(boost::shared_ptr<Derived> g);
            void remove(boost::shared_ptr<Constraint3D> g);

        private:

            //we store all geometries, shapes and constraint which belong to this shape.
            //Furthermore we store the remove connections, as we need to disconnect them later
            GeometryVector m_geometries;
            ShapeVector m_shapes;
            ConstraintVector m_constraints;

        protected:

#ifdef USE_LOGGING
            src::logger log;
#endif

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

            using Object<Sys, Derived, ShapeSig>::m_system;

            template<typename generator>
            void initShape() {
                m_generator = boost::shared_ptr<details::ShapeGeneratorBase<Sys> >(new typename generator::template type<Sys>(m_system));
                m_generator->set(ObjBase::shared_from_this(), &m_geometries, &m_shapes, &m_constraints);

                if(!m_generator->check())
                    throw creation_error() <<  boost::errinfo_errno(210) << error_message("not all needd geometry for shape present");

                m_generator->init();
            };

            //disconnect all remove signals of stored geometry/shapes/constraints
            void disconnectAll();

            //the storage is private, all things need to be added by this methods.
            //this is used to ensure the proper event connections
            boost::shared_ptr<Derived> append(boost::shared_ptr<Geometry3D> g);
            boost::shared_ptr<Derived> append(boost::shared_ptr<Derived> g);
            boost::shared_ptr<Derived> append(boost::shared_ptr<Constraint3D> g);

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
            //somehow the base class set function is not found
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
            friend struct details::ShapeGeneratorBase<Sys>;

        public:
            //the geometry class itself does not hold an aligned eigen object, but maybe the variant
            EIGEN_MAKE_ALIGNED_OPERATOR_NEW
        };

        //inheriter for own functions
        struct inheriter_base {

            inheriter_base() {
                m_this = (Sys*)this;
            };

            void system_sub(boost::shared_ptr<Sys> subsys) {};

        protected:
            Sys* m_this;

        public:
            //with no vararg templates before c++11 we need preprocessor to create the overloads of create we need
            BOOST_PP_REPEAT(5, CREATE_DEF, ~)

            void removeShape3D(boost::shared_ptr<Shape3D> g);
        };

        struct inheriter_id : public inheriter_base {
            //we don't have a createshape3d method with identifier, as identifiers can be used to
            //specifie creation geometries or shapes. Therefore a call would always be ambigious.

            void removeShape3D(ID id);
            bool hasShape3D(ID id);
            boost::shared_ptr<Shape3D> getShape3D(ID id);

            using inheriter_base::removeShape3D;

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
        typedef mpl::map0<> signals;

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

    //copy the standard stuff
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

    for(geometry3d_iterator it = beginGeometry3D(); it != endGeometry3D(); it++) {

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

    g->template setProperty<shape_geometry_prop>(true);
    Connection c = g->template connectSignal<dcm::remove>(boost::bind(static_cast<void (Shape3D_base::*)(boost::shared_ptr<Geometry3D>)>(&Shape3D_base::remove) , this, _1));
    m_geometries.push_back(fusion::make_vector(g,c));

    return ObjBase::shared_from_this();
};


template<typename Typelist, typename ID>
template<typename Sys>
template<typename Derived>
boost::shared_ptr<Derived>
ModuleShape3D<Typelist, ID>::type<Sys>::Shape3D_base<Derived>::append(boost::shared_ptr<Derived> g) {

    Connection c = g->template connectSignal<dcm::remove>(boost::bind(static_cast<void (Shape3D_base::*)(boost::shared_ptr<Derived>)>(&Shape3D_base::remove) , this, _1));
    m_shapes.push_back(fusion::make_vector(g,c));

    return ObjBase::shared_from_this();
};

template<typename Typelist, typename ID>
template<typename Sys>
template<typename Derived>
boost::shared_ptr<Derived>
ModuleShape3D<Typelist, ID>::type<Sys>::Shape3D_base<Derived>::append(boost::shared_ptr<Constraint3D> g) {

    Connection c = g->template connectSignal<dcm::remove>(boost::bind(static_cast<void (Shape3D_base::*)(boost::shared_ptr<Constraint3D>)>(&Shape3D_base::remove) , this, _1));
    m_constraints.push_back(fusion::make_vector(g,c));

    return ObjBase::shared_from_this();
};
template<typename Typelist, typename ID>
template<typename Sys>
template<typename Derived>
void ModuleShape3D<Typelist, ID>::type<Sys>::Shape3D_base<Derived>::disconnectAll() {

    typename GeometryVector::iterator git;

    for(git = m_geometries.begin(); git!=m_geometries.end(); git++)
        fusion::at_c<0>(*git)->template disconnectSignal<dcm::remove>(fusion::at_c<1>(*git));

    typename ShapeVector::iterator sit;

    for(sit = m_shapes.begin(); sit!=m_shapes.end(); sit++)
        fusion::at_c<0>(*sit)->template disconnectSignal<dcm::remove>(fusion::at_c<1>(*sit));

    typename ConstraintVector::iterator cit;

    for(cit = m_constraints.begin(); cit!=m_constraints.end(); cit++)
        fusion::at_c<0>(*cit)->template disconnectSignal<dcm::remove>(fusion::at_c<1>(*cit));
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
void ModuleShape3D<Typelist, ID>::type<Sys>::Shape3D_base<Derived>::remove(boost::shared_ptr<Geometry3D> g) {

    //before we delete this shape by calling the system remove function, we need to remove
    //this geometry as this would be deleted again by the system call and we would go into infinite recursion

    //get the vector object where the geometry is part of
    typename GeometryVector::const_iterator it;

    for(it=m_geometries.begin(); it!=m_geometries.end(); it++) {
        if(fusion::at_c<0>(*it)==g)
            break;
    };

    m_geometries.erase(std::remove(m_geometries.begin(), m_geometries.end(), *it), m_geometries.end());

    ObjBase::m_system->removeShape3D(ObjBase::shared_from_this());
};

template<typename Typelist, typename ID>
template<typename Sys>
template<typename Derived>
void ModuleShape3D<Typelist, ID>::type<Sys>::Shape3D_base<Derived>::remove(boost::shared_ptr<Derived> g) {

    //before we delete this shape by calling the system remove function, we need to remove
    //this geometry as this would be deleted again by the system call and we would go into infinite recursion

    //get the vector object where the geometry is part of
    typename ShapeVector::const_iterator it;

    for(it=m_shapes.begin(); it!=m_shapes.end(); it++) {
        if(fusion::at_c<0>(*it)==g)
            break;
    };

    m_shapes.erase(std::remove(m_shapes.begin(), m_shapes.end(), *it), m_shapes.end());

    ObjBase::m_system->removeShape3D(ObjBase::shared_from_this());
};

template<typename Typelist, typename ID>
template<typename Sys>
template<typename Derived>
void ModuleShape3D<Typelist, ID>::type<Sys>::Shape3D_base<Derived>::remove(boost::shared_ptr<Constraint3D> g) {

    //before we delete this shape by calling the system remove function, we need to remove
    //this geometry as this would be deleted again by the system call and we would go into infinite recursion

    //get the vector object where the geometry is part of
    typename ConstraintVector::const_iterator it;

    for(it=m_constraints.begin(); it!=m_constraints.end(); it++) {
        if(fusion::at_c<0>(*it)==g)
            break;
    };

    m_constraints.erase(std::remove(m_constraints.begin(), m_constraints.end(), *it), m_constraints.end());

    ObjBase::m_system->removeShape3D(ObjBase::shared_from_this());
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

    //disconnect all shapes, geometries and constraints, as otherwise we would go into infinite
    //recursion
    g->disconnectAll();

    //remove all constraints is unnessecary as they get removed together with the geometries
    //remove all geometries
    typedef typename Shape3D::geometry3d_iterator git;

    for(git it=g->beginGeometry3D(); it!=g->endGeometry3D(); it++)
        m_this->removeGeometry3D(*it);


    /* TODO: find out why it iterates over a empty vector and crashes...
        //remove all subshapes
        typedef typename Shape3D::shape3d_iterator sit;
        for(sit it=g->beginShape3D(); it!=g->endShape3D(); it++) {
            m_this->removeShape3D(*it);
        };*/

    //emit remove shape signal before actually deleting it
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

