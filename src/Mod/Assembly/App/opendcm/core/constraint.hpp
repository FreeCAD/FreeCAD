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
    GNU Lesser General Public License for more detemplate tails.

    You should have received a copy of the GNU Lesser General Public License along
    with this library; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#ifndef GCM_CONSTRAINT_H
#define GCM_CONSTRAINT_H


#include<Eigen/StdVector>

#include <assert.h>
#include <boost/variant.hpp>
#include <boost/function.hpp>
#include <boost/bind.hpp>

#include <boost/mpl/vector.hpp>
#include <boost/mpl/map.hpp>
#include <boost/mpl/less.hpp>
#include <boost/mpl/if.hpp>
#include <boost/mpl/size.hpp>
#include <boost/mpl/for_each.hpp>

#include <boost/any.hpp>

#include <boost/fusion/include/as_vector.hpp>
#include <boost/fusion/include/mpl.hpp>
#include <boost/fusion/include/at.hpp>
#include <boost/fusion/include/for_each.hpp>
#include <boost/fusion/sequence/intrinsic/size.hpp>
#include <boost/fusion/include/size.hpp>

#include "traits.hpp"
#include "object.hpp"
#include "equations.hpp"

namespace mpl = boost::mpl;
namespace fusion = boost::fusion;

namespace dcm {

namespace detail {

//type erasure container for constraints
template<typename Sys, typename Derived, typename Signals, typename MES, typename Geometry>
class Constraint : public Object<Sys, Derived, Signals > {

    typedef typename system_traits<Sys>::Kernel Kernel;
    typedef typename Kernel::number_type Scalar;
    typedef typename Kernel::DynStride DS;

    typedef boost::shared_ptr<Geometry> geom_ptr;
    typedef std::vector<typename Kernel::Vector3, Eigen::aligned_allocator<typename Kernel::Vector3> > Vec;

public:
    Constraint(Sys& system, geom_ptr f, geom_ptr s);
    ~Constraint();

    virtual boost::shared_ptr<Derived> clone(Sys& newSys);
    std::vector<boost::any> getGenericEquations();
    std::vector<boost::any> getGenericConstraints();
    std::vector<const std::type_info*> getEquationTypes();
    std::vector<const std::type_info*> getConstraintTypes();
    
    template<typename ConstraintVector>
    void initialize(ConstraintVector& obj);
    
protected:
    int equationCount();

    template< typename creator_type>
    void resetType(creator_type& c);

    void calculate(Scalar scale);

    void setMaps(MES& mes);

    void geometryReset(geom_ptr g) {
        /*    placeholder* p = content->resetConstraint(first, second);
            delete content;
            content = p;*/
    };

    void collectPseudoPoints(Vec& vec1, Vec& vec2);
    
    //Equation is the constraint with types, the EquationSet hold all needed Maps for calculation
    template<typename Equation>
    struct EquationSet {
        EquationSet() : m_diff_first(NULL,0,DS(0,0)), m_diff_second(NULL,0,DS(0,0)),
            m_residual(NULL,0,DS(0,0)) {};

        Equation m_eq;
        typename Kernel::VectorMap m_diff_first; //first geometry diff
        typename Kernel::VectorMap m_diff_second; //second geometry diff
        typename Kernel::VectorMap m_residual;

        typedef Equation eq_type;
    };

    struct placeholder  {
        virtual ~placeholder() {}
        virtual placeholder* resetConstraint(geom_ptr first, geom_ptr second) const = 0;
        virtual void calculate(geom_ptr first, geom_ptr second, Scalar scale) = 0;
        virtual int  equationCount() = 0;
        virtual void setMaps(MES& mes, geom_ptr first, geom_ptr second) = 0;
        virtual void collectPseudoPoints(geom_ptr first, geom_ptr second, Vec& vec1, Vec& vec2) = 0;
        virtual placeholder* clone() = 0;
	
	//some runtime type infos are needed, as we cant access the contents with arbitrary functors
        virtual std::vector<boost::any> getGenericEquations() = 0;
	virtual std::vector<boost::any> getGenericConstraints() = 0;
	virtual std::vector<const std::type_info*> getEquationTypes() = 0;
	virtual std::vector<const std::type_info*> getConstraintTypes() = 0;
    };

public:
    template< typename ConstraintVector, typename EquationVector>
    struct holder : public placeholder  {

        //create a vector of EquationSets with some mpl trickery
        typedef typename mpl::fold< EquationVector, mpl::vector<>,
                mpl::push_back<mpl::_1, EquationSet<mpl::_2> > >::type eq_set_vector;
        typedef typename fusion::result_of::as_vector<eq_set_vector>::type EquationSets;

        typedef ConstraintVector Objects;

        template<typename T>
        struct has_option {
            //we get the index of the eqaution in the eqaution vector, and as it is the same
            //as the index of the constraint in the constraint vector we can extract the
            //option type and check if it is no_option
            typedef typename mpl::find<EquationVector, T>::type iterator;
            typedef typename mpl::distance<typename mpl::begin<EquationVector>::type, iterator>::type distance;
            BOOST_MPL_ASSERT((mpl::not_<boost::is_same<iterator, typename mpl::end<EquationVector>::type > >));
            typedef typename fusion::result_of::at<ConstraintVector, distance>::type option_type;
            typedef mpl::not_<boost::is_same<option_type, no_option> > type;
        };

        struct OptionSetter {

            Objects& objects;

            OptionSetter(Objects& val);

            //only set the value if the equation has a option
            template< typename T >
            typename boost::enable_if<typename has_option<T>::type, void>::type
            operator()(EquationSet<T>& val) const;
            //if the equation has no otpion we do nothing!
            template< typename T >
            typename boost::enable_if<mpl::not_<typename has_option<T>::type>, void>::type
            operator()(EquationSet<T>& val) const;
        };

        struct Calculater {

            geom_ptr first, second;
            Scalar scale;

            Calculater(geom_ptr f, geom_ptr s, Scalar sc);

            template< typename T >
            void operator()(T& val) const;
        };

        struct MapSetter {
            MES& mes;
            geom_ptr first, second;

            MapSetter(MES& m, geom_ptr f, geom_ptr s);

            template< typename T >
            void operator()(T& val) const;
        };

        struct PseudoCollector {
            Vec& points1;
            Vec& points2;
            geom_ptr first,second;

            PseudoCollector(geom_ptr f, geom_ptr s, Vec& vec1, Vec& vec2);

            template< typename T >
            void operator()(T& val) const;
        };

        struct GenericEquations {
            std::vector<boost::any>& vec;
            GenericEquations(std::vector<boost::any>& v);

            template<typename T>
            void operator()(T& val) const;
        };
	
	struct GenericConstraints {
            std::vector<boost::any>& vec;
            GenericConstraints(std::vector<boost::any>& v);

            template<typename T>
            void operator()(T& val) const;
        };
	
	struct Types {
            std::vector<const std::type_info*>& vec;
            Types(std::vector<const std::type_info*>& v);

            template<typename T>
            void operator()(T& val) const;
        };

	
        holder(Objects& obj);

        virtual void calculate(geom_ptr first, geom_ptr second, Scalar scale);
        virtual placeholder* resetConstraint(geom_ptr first, geom_ptr second) const;
        virtual void setMaps(MES& mes, geom_ptr first, geom_ptr second);
        virtual void collectPseudoPoints(geom_ptr f, geom_ptr s, Vec& vec1, Vec& vec2);
        virtual placeholder* clone();
        virtual int equationCount() {
            return mpl::size<EquationVector>::value;
        };
	
        virtual std::vector<boost::any> getGenericEquations();
	virtual std::vector<boost::any> getGenericConstraints();
	virtual std::vector<const std::type_info*> getEquationTypes();
	virtual std::vector<const std::type_info*> getConstraintTypes();

        EquationSets m_sets;
	Objects m_objects;
    };

protected:
    template< typename  ConstraintVector >
    struct creator : public boost::static_visitor<void> {

        typedef ConstraintVector Objects;
        Objects& objects;

        creator(Objects& obj);

        template<typename C, typename T1, typename T2>
        struct equation {
            typedef typename C::template type<Kernel, T1, T2> type;
        };

        template<typename T1, typename T2>
        void operator()(const T1&, const T2&);

        placeholder* p;
        bool need_swap;
    };

    placeholder* content;
    Connection cf, cs;
public:
    geom_ptr first, second;
};


/*****************************************************************************************************************/
/*****************************************************************************************************************/
/*****************************************************************************************************************/
/*****************************************************************************************************************/


template<typename Sys, typename Derived, typename Signals, typename MES, typename Geometry>
Constraint<Sys, Derived, Signals, MES, Geometry>::Constraint(Sys& system, geom_ptr f, geom_ptr s)
    : first(f), second(s), content(0)	{

    this->m_system = &system;
    cf = first->template connectSignal<reset> (boost::bind(&Constraint::geometryReset, this, _1));
    cs = second->template connectSignal<reset> (boost::bind(&Constraint::geometryReset, this, _1));
};

template<typename Sys, typename Derived, typename Signals, typename MES, typename Geometry>
Constraint<Sys, Derived, Signals, MES, Geometry>::~Constraint()  {
    delete content;
    first->template disconnectSignal<reset>(cf);
    second->template disconnectSignal<reset>(cs);
};

template<typename Sys, typename Derived, typename Signals, typename MES, typename Geometry>
boost::shared_ptr<Derived> Constraint<Sys, Derived, Signals, MES, Geometry>::clone(Sys& newSys) {

    //copy the standart stuff
    boost::shared_ptr<Derived> np = boost::shared_ptr<Derived>(new Derived(*static_cast<Derived*>(this)));
    np->m_system = &newSys;
    //copy the internals
    np->content = content->clone();
    //and get the geometry pointers right
    if(first) {
        GlobalVertex v = first->template getProperty<typename Geometry::vertex_propertie>();
        np->first = newSys.m_cluster->template getObject<Geometry>(v);
    }
    if(second) {
        GlobalVertex v = second->template getProperty<typename Geometry::vertex_propertie>();
        np->second = newSys.m_cluster->template getObject<Geometry>(v);
    }
    return np;
};

template<typename Sys, typename Derived, typename Signals, typename MES, typename Geometry>
template<typename ConstraintVector>
void Constraint<Sys, Derived, Signals, MES, Geometry>::initialize(ConstraintVector& obj) {

    //first create the new placeholder
    creator<ConstraintVector> c(obj);
    boost::apply_visitor(c, first->m_geometry, second->m_geometry);

    //and now store it
    content = c.p;
    //geometry order needs to be the one needed by equations
    if(c.need_swap) first.swap(second);

};

template<typename Sys, typename Derived, typename Signals, typename MES, typename Geometry>
int Constraint<Sys, Derived, Signals, MES, Geometry>::equationCount() {
    return content->equationCount();
};

template<typename Sys, typename Derived, typename Signals, typename MES, typename Geometry>
template< typename creator_type>
void Constraint<Sys, Derived, Signals, MES, Geometry>::resetType(creator_type& c) {
    boost::apply_visitor(c, first->m_geometry, second->m_geometry);
    content = c.p;
    if(c.need_swap) first.swap(second);
};

template<typename Sys, typename Derived, typename Signals, typename MES, typename Geometry>
void Constraint<Sys, Derived, Signals, MES, Geometry>::calculate(Scalar scale) {
    content->calculate(first, second, scale);
};

template<typename Sys, typename Derived, typename Signals, typename MES, typename Geometry>
void Constraint<Sys, Derived, Signals, MES, Geometry>::setMaps(MES& mes) {
    content->setMaps(mes, first, second);
};

template<typename Sys, typename Derived, typename Signals, typename MES, typename Geometry>
void Constraint<Sys, Derived, Signals, MES, Geometry>::collectPseudoPoints(Vec& vec1, Vec& vec2) {
    content->collectPseudoPoints(first, second, vec1, vec2);
};

template<typename Sys, typename Derived, typename Signals, typename MES, typename Geometry>
std::vector<boost::any> Constraint<Sys, Derived, Signals, MES, Geometry>::getGenericEquations() {
    return content->getGenericEquations();
};

template<typename Sys, typename Derived, typename Signals, typename MES, typename Geometry>
std::vector<boost::any> Constraint<Sys, Derived, Signals, MES, Geometry>::getGenericConstraints() {
    return content->getGenericConstraints();
};

template<typename Sys, typename Derived, typename Signals, typename MES, typename Geometry>
std::vector<const std::type_info*> Constraint<Sys, Derived, Signals, MES, Geometry>::getEquationTypes() {
    return content->getEquationTypes();
};

template<typename Sys, typename Derived, typename Signals, typename MES, typename Geometry>
std::vector<const std::type_info*> Constraint<Sys, Derived, Signals, MES, Geometry>::getConstraintTypes() {
    return content->getConstraintTypes();
};

template<typename Sys, typename Derived, typename Signals, typename MES, typename Geometry>
template<typename ConstraintVector, typename EquationVector>
Constraint<Sys, Derived, Signals, MES, Geometry>::holder<ConstraintVector, EquationVector>::OptionSetter::OptionSetter(Objects& val) : objects(val) {};

template<typename Sys, typename Derived, typename Signals, typename MES, typename Geometry>
template<typename ConstraintVector, typename EquationVector>
template<typename T>
typename boost::enable_if<typename Constraint<Sys, Derived, Signals, MES, Geometry>::template holder<ConstraintVector, EquationVector>::template has_option<T>::type, void>::type
Constraint<Sys, Derived, Signals, MES, Geometry>::holder<ConstraintVector, EquationVector>::OptionSetter::operator()(EquationSet<T>& val) const {

    //get the index of the corresbonding equation
    typedef typename mpl::find<EquationVector, T>::type iterator;
    typedef typename mpl::distance<typename mpl::begin<EquationVector>::type, iterator>::type distance;
    BOOST_MPL_ASSERT((mpl::not_<boost::is_same<iterator, typename mpl::end<EquationVector>::type > >));
    val.m_eq.value = fusion::at<distance>(objects).value;
};

template<typename Sys, typename Derived, typename Signals, typename MES, typename Geometry>
template<typename ConstraintVector, typename EquationVector>
template<typename T>
typename boost::enable_if<mpl::not_<typename Constraint<Sys, Derived, Signals, MES, Geometry>::template holder<ConstraintVector, EquationVector>::template has_option<T>::type>, void>::type
Constraint<Sys, Derived, Signals, MES, Geometry>::holder<ConstraintVector, EquationVector>::OptionSetter::operator()(EquationSet<T>& val) const {

};

template<typename Sys, typename Derived, typename Signals, typename MES, typename Geometry>
template<typename ConstraintVector, typename EquationVector>
Constraint<Sys, Derived, Signals, MES, Geometry>::holder<ConstraintVector, EquationVector>::Calculater::Calculater(geom_ptr f, geom_ptr s, Scalar sc) : first(f), second(s), scale(sc) {

};

template<typename Sys, typename Derived, typename Signals, typename MES, typename Geometry>
template<typename ConstraintVector, typename EquationVector>
template< typename T >
void Constraint<Sys, Derived, Signals, MES, Geometry>::holder<ConstraintVector, EquationVector>::Calculater::operator()(T& val) const {

    val.m_eq.setScale(scale);

    val.m_residual(0) = val.m_eq.calculate(first->m_parameter, second->m_parameter);

    //now see which way we should calculate the gradient (may be diffrent for both geometries)
    if(first->m_parameterCount) {
        if(first->getClusterMode()) {
            //when the cluster is fixed no maps are set as no parameters exist.
            if(!first->isClusterFixed()) {

                //cluster mode, so we do a full calculation with all 3 rotation diffparam vectors
                for(int i=0; i<6; i++) {
                    typename Kernel::VectorMap block(&first->m_diffparam(0,i),first->m_parameterCount,1, DS(1,1));
                    val.m_diff_first(i) = val.m_eq.calculateGradientFirst(first->m_parameter,
                                          second->m_parameter, block);
                }
            }
        } else {
            //not in cluster, so allow the constraint to optimize the gradient calculation
            val.m_eq.calculateGradientFirstComplete(first->m_parameter, second->m_parameter, val.m_diff_first);
        }
    }
    if(second->m_parameterCount) {
        if(second->getClusterMode()) {
            if(!second->isClusterFixed()) {

                //cluster mode, so we do a full calculation with all 3 rotation diffparam vectors
                for(int i=0; i<6; i++) {
                    typename Kernel::VectorMap block(&second->m_diffparam(0,i),second->m_parameterCount,1, DS(1,1));
                    val.m_diff_second(i) = val.m_eq.calculateGradientSecond(first->m_parameter,
                                           second->m_parameter, block);
                }
            }
        } else {
            //not in cluster, so allow the constraint to optimize the gradient calculation
            val.m_eq.calculateGradientSecondComplete(first->m_parameter, second->m_parameter, val.m_diff_second);
        }
    }
};

template<typename Sys, typename Derived, typename Signals, typename MES, typename Geometry>
template<typename ConstraintVector, typename EquationVector>
Constraint<Sys, Derived, Signals, MES, Geometry>::holder<ConstraintVector, EquationVector>::MapSetter::MapSetter(MES& m, geom_ptr f, geom_ptr s)
    : mes(m), first(f), second(s) {

};

template<typename Sys, typename Derived, typename Signals, typename MES, typename Geometry>
template<typename ConstraintVector, typename EquationVector>
template< typename T >
void Constraint<Sys, Derived, Signals, MES, Geometry>::holder<ConstraintVector, EquationVector>::MapSetter::operator()(T& val) const {

    //when in cluster, there are 6 clusterparameter we differentiat for, if not we differentiat
    //for every parameter in the geometry;
    int equation = mes.setResidualMap(val.m_residual);
    if(first->getClusterMode()) {
        if(!first->isClusterFixed()) {
            mes.setJacobiMap(equation, first->m_offset, 6, val.m_diff_first);
        }
    } else mes.setJacobiMap(equation, first->m_offset, first->m_parameterCount, val.m_diff_first);


    if(second->getClusterMode()) {
        if(!second->isClusterFixed()) {
            mes.setJacobiMap(equation, second->m_offset, 6, val.m_diff_second);
        }
    } else mes.setJacobiMap(equation, second->m_offset, second->m_parameterCount, val.m_diff_second);
};

template<typename Sys, typename Derived, typename Signals, typename MES, typename Geometry>
template<typename ConstraintVector, typename EquationVector>
Constraint<Sys, Derived, Signals, MES, Geometry>::holder<ConstraintVector, EquationVector>::PseudoCollector::PseudoCollector(geom_ptr f, geom_ptr s, Vec& vec1, Vec& vec2)
    : first(f), second(s), points1(vec1), points2(vec2) {

};

template<typename Sys, typename Derived, typename Signals, typename MES, typename Geometry>
template<typename ConstraintVector, typename EquationVector>
template< typename T >
void Constraint<Sys, Derived, Signals, MES, Geometry>::holder<ConstraintVector, EquationVector>::PseudoCollector::operator()(T& val) const {
    if(first->m_isInCluster && second->m_isInCluster) {
        val.m_eq.calculatePseudo(first->m_rotated, points1, second->m_rotated, points2);
    } else if(first->m_isInCluster) {
        typename Kernel::Vector sec = second->m_parameter;
        val.m_eq.calculatePseudo(first->m_rotated, points1, sec, points2);
    } else if(second->m_isInCluster) {
        typename Kernel::Vector fir = first->m_parameter;
        val.m_eq.calculatePseudo(fir, points1, second->m_rotated, points2);
    }
};


template<typename Sys, typename Derived, typename Signals, typename MES, typename Geometry>
template<typename ConstraintVector, typename EquationVector>
Constraint<Sys, Derived, Signals, MES, Geometry>::holder<ConstraintVector, EquationVector>::GenericEquations::GenericEquations(std::vector<boost::any>& v)
    : vec(v) {

};

template<typename Sys, typename Derived, typename Signals, typename MES, typename Geometry>
template<typename ConstraintVector, typename EquationVector>
template< typename T >
void Constraint<Sys, Derived, Signals, MES, Geometry>::holder<ConstraintVector, EquationVector>::GenericEquations::operator()(T& val) const {
    vec.push_back(val.m_eq);
};

template<typename Sys, typename Derived, typename Signals, typename MES, typename Geometry>
template<typename ConstraintVector, typename EquationVector>
Constraint<Sys, Derived, Signals, MES, Geometry>::holder<ConstraintVector, EquationVector>::GenericConstraints::GenericConstraints(std::vector<boost::any>& v)
    : vec(v) {

};

template<typename Sys, typename Derived, typename Signals, typename MES, typename Geometry>
template<typename ConstraintVector, typename EquationVector>
template< typename T >
void Constraint<Sys, Derived, Signals, MES, Geometry>::holder<ConstraintVector, EquationVector>::GenericConstraints::operator()(T& val) const {
    vec.push_back(val);
};

template<typename Sys, typename Derived, typename Signals, typename MES, typename Geometry>
template<typename ConstraintVector, typename EquationVector>
Constraint<Sys, Derived, Signals, MES, Geometry>::holder<ConstraintVector, EquationVector>::Types::Types(std::vector<const std::type_info*>& v)
    : vec(v) {

};

template<typename Sys, typename Derived, typename Signals, typename MES, typename Geometry>
template<typename ConstraintVector, typename EquationVector>
template< typename T >
void Constraint<Sys, Derived, Signals, MES, Geometry>::holder<ConstraintVector, EquationVector>::Types::operator()(T& val) const {
    vec.push_back(&typeid(T));
};		

template<typename Sys, typename Derived, typename Signals, typename MES, typename Geometry>
template<typename ConstraintVector, typename EquationVector>
Constraint<Sys, Derived, Signals, MES, Geometry>::holder<ConstraintVector, EquationVector>::holder(Objects& obj) : m_objects(obj)  {
    //set the initial values in the equations
    fusion::for_each(m_sets, OptionSetter(obj));
};

template<typename Sys, typename Derived, typename Signals, typename MES, typename Geometry>
template<typename ConstraintVector, typename EquationVector>
void Constraint<Sys, Derived, Signals, MES, Geometry>::holder<ConstraintVector, EquationVector>::calculate(geom_ptr first, geom_ptr second, Scalar scale) {
    fusion::for_each(m_sets, Calculater(first, second, scale));
};

template<typename Sys, typename Derived, typename Signals, typename MES, typename Geometry>
template<typename ConstraintVector, typename EquationVector>
typename Constraint<Sys, Derived, Signals, MES, Geometry>::placeholder*
Constraint<Sys, Derived, Signals, MES, Geometry>::holder<ConstraintVector, EquationVector>::resetConstraint(geom_ptr first, geom_ptr second) const {
    //boost::apply_visitor(creator, first->m_geometry, second->m_geometry);
    //if(creator.need_swap) first.swap(second);
    return NULL;
};

template<typename Sys, typename Derived, typename Signals, typename MES, typename Geometry>
template<typename ConstraintVector, typename EquationVector>
void Constraint<Sys, Derived, Signals, MES, Geometry>::holder<ConstraintVector, EquationVector>::setMaps(MES& mes, geom_ptr first, geom_ptr second) {
    fusion::for_each(m_sets, MapSetter(mes, first, second));
};

template<typename Sys, typename Derived, typename Signals, typename MES, typename Geometry>
template<typename ConstraintVector, typename EquationVector>
void Constraint<Sys, Derived, Signals, MES, Geometry>::holder<ConstraintVector, EquationVector>::collectPseudoPoints(geom_ptr f, geom_ptr s, Vec& vec1, Vec& vec2) {
    fusion::for_each(m_sets, PseudoCollector(f, s, vec1, vec2));
};

template<typename Sys, typename Derived, typename Signals, typename MES, typename Geometry>
template<typename ConstraintVector, typename EquationVector>
typename Constraint<Sys, Derived, Signals, MES, Geometry>::placeholder*
Constraint<Sys, Derived, Signals, MES, Geometry>::holder<ConstraintVector, EquationVector>::clone() {
    return new holder(*this);
};

template<typename Sys, typename Derived, typename Signals, typename MES, typename Geometry>
template<typename ConstraintVector, typename EquationVector>
std::vector<boost::any> 
Constraint<Sys, Derived, Signals, MES, Geometry>::holder<ConstraintVector, EquationVector>::getGenericEquations() {
    std::vector<boost::any> vec;
    fusion::for_each( m_sets, GenericEquations(vec) );
    return vec;
};

template<typename Sys, typename Derived, typename Signals, typename MES, typename Geometry>
template<typename ConstraintVector, typename EquationVector>
std::vector<boost::any> 
Constraint<Sys, Derived, Signals, MES, Geometry>::holder<ConstraintVector, EquationVector>::getGenericConstraints() {
    std::vector<boost::any> vec;
    fusion::for_each( m_objects, GenericConstraints(vec) );
    return vec;
};

template<typename Sys, typename Derived, typename Signals, typename MES, typename Geometry>
template<typename ConstraintVector, typename EquationVector>
std::vector<const std::type_info*>
Constraint<Sys, Derived, Signals, MES, Geometry>::holder<ConstraintVector, EquationVector>::getEquationTypes() {
    std::vector<const std::type_info*> vec;
    mpl::for_each< EquationVector >( Types(vec) );
    return vec;
};

template<typename Sys, typename Derived, typename Signals, typename MES, typename Geometry>
template<typename ConstraintVector, typename EquationVector>
std::vector<const std::type_info*>
Constraint<Sys, Derived, Signals, MES, Geometry>::holder<ConstraintVector, EquationVector>::getConstraintTypes() {
    std::vector<const std::type_info*> vec;
    mpl::for_each< ConstraintVector >( Types(vec) );
    return vec;
};

template<typename Sys, typename Derived, typename Signals, typename MES, typename Geometry>
template< typename  ConstraintVector >
Constraint<Sys, Derived, Signals, MES, Geometry>::creator<ConstraintVector>::creator(Objects& obj) : objects(obj) {

};

template<typename Sys, typename Derived, typename Signals, typename MES, typename Geometry>
template< typename  ConstraintVector >
template<typename T1, typename T2>
void Constraint<Sys, Derived, Signals, MES, Geometry>::creator<ConstraintVector>::operator()(const T1&, const T2&) {

    typedef tag_order< typename geometry_traits<T1>::tag, typename geometry_traits<T2>::tag > order;

    //transform the constraints into eqautions with the now known types
    typedef typename mpl::fold< ConstraintVector, mpl::vector<>,
            mpl::push_back<mpl::_1, equation<mpl::_2, typename order::first_tag,
            typename order::second_tag> > >::type EquationVector;

    //and build the placeholder
    p = new holder<ConstraintVector, EquationVector>(objects);
    need_swap = order::swapt::value;
};


};//detail

};//dcm

#endif //GCM_CONSTRAINT_H

