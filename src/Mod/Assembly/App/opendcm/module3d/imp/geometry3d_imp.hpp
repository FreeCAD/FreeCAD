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

#ifndef DCM_GEOMETRY_3D_IMP_H
#define DCM_GEOMETRY_3D_IMP_H

#include "../module.hpp"

#ifdef DCM_EXTERNAL_CORE
#include "opendcm/core/imp/geometry_imp.hpp"
#include "opendcm/core/imp/object_imp.hpp"
#endif

namespace dcm {

namespace details {

template<typename Variant>
struct cloner : boost::static_visitor<void> {

    Variant variant;
    cloner(Variant& v) : variant(v) {};

    template<typename T>
    void operator()(T& t) {
        variant = geometry_clone_traits<T>()(t);
    };
};

//visitor to write the calculated value into the variant
template<typename Kernel>
struct apply_visitor : public boost::static_visitor<void> {

    apply_visitor(typename Kernel::Vector& v) : value(v) {};
    template <typename T>
    void operator()(T& t) const  {
        (typename geometry_traits<T>::modell()).template inject<typename Kernel::number_type,
        typename geometry_traits<T>::accessor >(t, value);
    }
    typename Kernel::Vector& value;
};
};

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
template<typename T>
T Module3D<Typelist, ID>::type<Sys>::Geometry3D_base<Derived>::convertTo() {
    T t;
    (typename geometry_traits<T>::modell()).template inject<typename Kernel::number_type,
    typename geometry_traits<T>::accessor >(t, Base::m_global);
    return t;
};

template<typename Typelist, typename ID>
template<typename Sys>
template<typename Derived>
template<typename Visitor>
typename Visitor::result_type Module3D<Typelist, ID>::type<Sys>::Geometry3D_base<Derived>::apply(Visitor& vis) {
    return boost::apply_visitor(vis, m_geometry);
};

template<typename Typelist, typename ID>
template<typename Sys>
template<typename Derived>
boost::shared_ptr<Derived> Module3D<Typelist, ID>::type<Sys>::Geometry3D_base<Derived>::clone(Sys& newSys) {

    //copy the standart stuff
    boost::shared_ptr<Derived> np = boost::shared_ptr<Derived>(new Derived(*static_cast<Derived*>(this)));
    np->m_system = &newSys;
    //it's possible that the variant contains pointers, so we need to clone them
    details::cloner<Variant> clone_fnc(np->m_geometry);
    boost::apply_visitor(clone_fnc, m_geometry);
    return np;
};

template<typename Typelist, typename ID>
template<typename Sys>
template<typename Derived>
void Module3D<Typelist, ID>::type<Sys>::Geometry3D_base<Derived>::recalculated() {

    details::apply_visitor<Kernel> v(Base::getValue());
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

} //dcm

#endif //DCM_MODULE_3D_IMP_H
