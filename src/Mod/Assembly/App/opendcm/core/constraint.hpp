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

#include <boost/preprocessor.hpp>

#include "traits.hpp"
#include "object.hpp"
#include "equations.hpp"
#include "geometry.hpp"

namespace mpl = boost::mpl;
namespace fusion = boost::fusion;

namespace dcm {

namespace detail {

//metafunction to avoid ot-of-range access of mpl sequences
template<typename Sequence, int Value>
struct in_range_value {
	typedef typename mpl::prior<mpl::size<Sequence> >::type last_id;
	typedef typename mpl::min< mpl::int_<Value>, last_id>::type type;
};

//type erasure container for constraints
template<typename Sys, int Dim>
class Constraint {

    typedef typename Sys::Kernel Kernel;
    typedef typename Kernel::number_type Scalar;
    typedef typename Kernel::DynStride DS;
    typedef typename Kernel::MappedEquationSystem MES;

    typedef boost::shared_ptr<details::Geometry<Kernel, Dim, typename Sys::geometries> > geom_ptr;
    typedef std::vector<typename Kernel::Vector3, Eigen::aligned_allocator<typename Kernel::Vector3> > Vec;

    //metafunction to create equation from consraint and tags
    template<typename C, typename T1, typename T2>
    struct equation {
        typedef typename C::template type<Kernel, T1, T2> type;
    };

public:
    Constraint(geom_ptr f, geom_ptr s);
    ~Constraint();

    //workaround until better analysing class is created
    // TODO: remove diasable once analyser is available
    void disable() {
        content->disable();
    };

    std::vector<boost::any> getGenericEquations();
    std::vector<boost::any> getGenericConstraints();
    std::vector<const std::type_info*> getEquationTypes();
    std::vector<const std::type_info*> getConstraintTypes();

    template<typename Tag1, typename Tag2, typename ConstraintVector>
    void initializeFromTags(ConstraintVector& obj);
    template<typename ConstraintVector>
    void initialize(ConstraintVector& obj);

protected:
    //initialising from geometry functions
    template<typename WhichType, typename ConstraintVector>
    void initializeFirstGeometry(ConstraintVector& cv, boost::mpl::false_);
    template<typename WhichType, typename ConstraintVector>
    void initializeFirstGeometry(ConstraintVector& cv, boost::mpl::true_);
    template<typename WhichType, typename FirstType, typename ConstraintVector>
    void initializeSecondGeometry(ConstraintVector& cv, boost::mpl::false_);
    template<typename WhichType, typename FirstType, typename ConstraintVector>
    void initializeSecondGeometry(ConstraintVector& cv, boost::mpl::true_);
    template<typename FirstType, typename SecondType, typename ConstraintVector>
    inline void intitalizeFinalize(ConstraintVector& cv, boost::mpl::false_);
    template<typename FirstType, typename SecondType, typename ConstraintVector>
    inline void intitalizeFinalize(ConstraintVector& cv, boost::mpl::true_);


    int equationCount();

    template< typename creator_type>
    void resetType(creator_type& c);

    void calculate(Scalar scale, bool rotation_only = false);
    void treatLGZ();

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
        EquationSet() : m_diff_first(NULL,0,DS(0,0)), m_diff_first_rot(NULL,0,DS(0,0)),
            m_diff_second(NULL,0,DS(0,0)), m_diff_second_rot(NULL,0,DS(0,0)),
            m_residual(NULL,0,DS(0,0)), enabled(true) {};

        Equation m_eq;
        typename Kernel::VectorMap m_diff_first, m_diff_first_rot; //first geometry diff
        typename Kernel::VectorMap m_diff_second, m_diff_second_rot; //second geometry diff
        typename Kernel::VectorMap m_residual;

        bool pure_rotation, enabled;

        typedef Equation eq_type;
    };

    struct placeholder  {
        virtual ~placeholder() {}
        virtual placeholder* resetConstraint(geom_ptr first, geom_ptr second) const = 0;
        virtual void calculate(geom_ptr first, geom_ptr second, Scalar scale, bool rotation_only = false) = 0;
        virtual void treatLGZ(geom_ptr first, geom_ptr second) = 0;
        virtual int  equationCount() = 0;
        virtual void setMaps(MES& mes, geom_ptr first, geom_ptr second) = 0;
        virtual void collectPseudoPoints(geom_ptr first, geom_ptr second, Vec& vec1, Vec& vec2) = 0;
        virtual void disable() = 0;
        virtual placeholder* clone() = 0;

        //some runtime type infos are needed, as we cant access the contents with arbitrary functors
        virtual std::vector<boost::any> getGenericEquations() = 0;
        virtual std::vector<boost::any> getGenericConstraints() = 0;
        virtual std::vector<const std::type_info*> getEquationTypes() = 0;
        virtual std::vector<const std::type_info*> getConstraintTypes() = 0;
    };
    int value;

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
            bool rot_only;

            Calculater(geom_ptr f, geom_ptr s, Scalar sc, bool rotation_only = false);

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

        struct LGZ {
            geom_ptr first,second;

            LGZ(geom_ptr f, geom_ptr s);

            template< typename T >
            void operator()(T& val) const;
        };


        struct EquationCounter {
            int& count;

            EquationCounter(int& c) : count(c) {};

            template< typename T >
            void operator()(T& val) const {
                if(val.enabled)
                    count++;
            };
        };

        //workaround until we have a better analyser class
        struct disabler {
            template<typename T>
            void operator()(T& val) const {
                val.enabled = false;
            };
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

        virtual void calculate(geom_ptr first, geom_ptr second, Scalar scale, bool rotation_only = false);
        virtual void treatLGZ(geom_ptr first, geom_ptr second);
        virtual placeholder* resetConstraint(geom_ptr first, geom_ptr second) const;
        virtual void setMaps(MES& mes, geom_ptr first, geom_ptr second);
        virtual void collectPseudoPoints(geom_ptr f, geom_ptr s, Vec& vec1, Vec& vec2);
        virtual placeholder* clone();
        virtual int equationCount();
        virtual void disable() {
            fusion::for_each(m_sets, disabler());
        };

        virtual std::vector<boost::any> getGenericEquations();
        virtual std::vector<boost::any> getGenericConstraints();
        virtual std::vector<const std::type_info*> getEquationTypes();
        virtual std::vector<const std::type_info*> getConstraintTypes();

        EquationSets m_sets;
        Objects m_objects;
    protected:
        void for_each(EquationSets m_sets, Calculater Calculater);
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


template<typename Sys, int Dim>
Constraint<Sys, Dim>::Constraint(geom_ptr f, geom_ptr s)
    : first(f), second(s), content(0)	{

    //cf = first->template connectSignal<reset> (boost::bind(&Constraint::geometryReset, this, _1));
    //cs = second->template connectSignal<reset> (boost::bind(&Constraint::geometryReset, this, _1));
};

template<typename Sys, int Dim>
Constraint<Sys, Dim>::~Constraint()  {
    delete content;
    //first->template disconnectSignal<reset>(cf);
    //second->template disconnectSignal<reset>(cs);
};

template<typename Sys, int Dim>
template<typename tag1, typename tag2, typename ConstraintVector>
void Constraint<Sys, Dim>::initializeFromTags(ConstraintVector& v) {

    typedef tag_order< tag1, tag2 > order;

    //transform the constraints into eqautions with the now known types
    typedef typename mpl::fold< ConstraintVector, mpl::vector<>,
            mpl::push_back<mpl::_1, equation<mpl::_2, typename order::first_tag,
            typename order::second_tag> > >::type EquationVector;

    //and build the placeholder
    content = new holder<ConstraintVector, EquationVector>(v);

    //geometry order needs to be the one needed by equations
    if(order::swapt::value)
        first.swap(second);
};

template<typename Sys, int Dim>
template<typename ConstraintVector>
void Constraint<Sys, Dim>::initialize(ConstraintVector& cv) {

    //use the compile time unrolling to retrieve the geometry tags
    initializeFirstGeometry<mpl::int_<0> >(cv, mpl::true_());
};

template<typename Sys, int Dim>
int Constraint<Sys, Dim>::equationCount() {
    return content->equationCount();
};

template<typename Sys, int Dim>
template< typename creator_type>
void Constraint<Sys, Dim>::resetType(creator_type& c) {
    boost::apply_visitor(c, first->m_geometry, second->m_geometry);
    content = c.p;
    if(c.need_swap)
        first.swap(second);
};

template<typename Sys, int Dim>
void Constraint<Sys, Dim>::calculate(Scalar scale, bool rotation_only) {
    content->calculate(first, second, scale, rotation_only);
};

template<typename Sys, int Dim>
void Constraint<Sys, Dim>::treatLGZ() {
    content->treatLGZ(first, second);
};

template<typename Sys, int Dim>
void Constraint<Sys, Dim>::setMaps(MES& mes) {
    content->setMaps(mes, first, second);
};

template<typename Sys, int Dim>
void Constraint<Sys, Dim>::collectPseudoPoints(Vec& vec1, Vec& vec2) {
    content->collectPseudoPoints(first, second, vec1, vec2);
};

template<typename Sys, int Dim>
std::vector<boost::any> Constraint<Sys, Dim>::getGenericEquations() {
    return content->getGenericEquations();
};

template<typename Sys, int Dim>
std::vector<boost::any> Constraint<Sys, Dim>::getGenericConstraints() {
    return content->getGenericConstraints();
};

template<typename Sys, int Dim>
std::vector<const std::type_info*> Constraint<Sys, Dim>::getEquationTypes() {
    return content->getEquationTypes();
};

template<typename Sys, int Dim>
std::vector<const std::type_info*> Constraint<Sys, Dim>::getConstraintTypes() {
    return content->getConstraintTypes();
};

template<typename Sys, int Dim>
template<typename ConstraintVector, typename EquationVector>
Constraint<Sys, Dim>::holder<ConstraintVector, EquationVector>::OptionSetter::OptionSetter(Objects& val) : objects(val) {};

template<typename Sys, int Dim>
template<typename ConstraintVector, typename EquationVector>
template<typename T>
typename boost::enable_if<typename Constraint<Sys, Dim>::template holder<ConstraintVector, EquationVector>::template has_option<T>::type, void>::type
Constraint<Sys, Dim>::holder<ConstraintVector, EquationVector>::OptionSetter::operator()(EquationSet<T>& val) const {

    //get the index of the corresbonding equation
    typedef typename mpl::find<EquationVector, T>::type iterator;
    typedef typename mpl::distance<typename mpl::begin<EquationVector>::type, iterator>::type distance;
    BOOST_MPL_ASSERT((mpl::not_<boost::is_same<iterator, typename mpl::end<EquationVector>::type > >));
    fusion::copy(fusion::at<distance>(objects).values, val.m_eq.values);
    val.pure_rotation = fusion::at<distance>(objects).pure_rotation;
};

template<typename Sys, int Dim>
template<typename ConstraintVector, typename EquationVector>
template<typename T>
typename boost::enable_if<mpl::not_<typename Constraint<Sys, Dim>::template holder<ConstraintVector, EquationVector>::template has_option<T>::type>, void>::type
Constraint<Sys, Dim>::holder<ConstraintVector, EquationVector>::OptionSetter::operator()(EquationSet<T>& val) const {

};

template<typename Sys, int Dim>
template<typename ConstraintVector, typename EquationVector>
Constraint<Sys, Dim>::holder<ConstraintVector, EquationVector>::Calculater::Calculater(geom_ptr f, geom_ptr s, Scalar sc, bool rotation_only)
    : first(f), second(s), scale(sc), rot_only(rotation_only) {

};

template<typename Sys, int Dim>
template<typename ConstraintVector, typename EquationVector>
template< typename T >
void Constraint<Sys, Dim>::holder<ConstraintVector, EquationVector>::Calculater::operator()(T& val) const {

    //if the equation is disabled we don't have anything mapped so avoid accessing it
    if(!val.enabled)
        return;

    //if we only need pure rotational functions and we are not such a nice thing, everything becomes 0
    if(rot_only && !val.pure_rotation) {

        val.m_residual(0) = 0;
        if(first->getClusterMode()) {
            if(!first->isClusterFixed()) {
                val.m_diff_first_rot.setZero();
                val.m_diff_first.setZero();
            }
        }
        else
            val.m_diff_first.setZero();

        if(second->getClusterMode()) {
            if(!second->isClusterFixed()) {
                val.m_diff_second_rot.setZero();
                val.m_diff_second.setZero();
            }
        }
        else
            val.m_diff_second.setZero();

    }
    //we need to calculate, so lets go for it!
    else {

        val.m_eq.setScale(scale);

        val.m_residual(0) = val.m_eq.calculate(first->m_parameter, second->m_parameter);

        //now see which way we should calculate the gradient (may be diffrent for both geometries)
        if(first->m_parameterCount) {
            if(first->getClusterMode()) {
                //when the cluster is fixed no maps are set as no parameters exist.
                if(!first->isClusterFixed()) {

                    //cluster mode, so we do a full calculation with all 3 rotation diffparam vectors
                    for(int i=0; i<3; i++) {
                        val.m_diff_first_rot(i) = val.m_eq.calculateGradientFirst(first->m_parameter,
                                                  second->m_parameter, first->m_diffparam.col(i));
                    }
                    //and now with the translations
                    for(int i=0; i<3; i++) {
                        val.m_diff_first(i) = val.m_eq.calculateGradientFirst(first->m_parameter,
                                              second->m_parameter, first->m_diffparam.col(i+3));
                    }
                }
            }
            else {
                //not in cluster, so allow the constraint to optimize the gradient calculation
                val.m_eq.calculateGradientFirstComplete(first->m_parameter, second->m_parameter, val.m_diff_first);
            }
        }
        if(second->m_parameterCount) {
            if(second->getClusterMode()) {
                if(!second->isClusterFixed()) {

                    //cluster mode, so we do a full calculation with all 3 rotation diffparam vectors
                    for(int i=0; i<3; i++) {
                        val.m_diff_second_rot(i) = val.m_eq.calculateGradientSecond(first->m_parameter,
                                                   second->m_parameter, second->m_diffparam.col(i));
                    }
                    //and the translation seperated
                    for(int i=0; i<3; i++) {
                        val.m_diff_second(i) = val.m_eq.calculateGradientSecond(first->m_parameter,
                                               second->m_parameter, second->m_diffparam.col(i+3));
                    }
                }
            }
            else {
                //not in cluster, so allow the constraint to optimize the gradient calculation
                val.m_eq.calculateGradientSecondComplete(first->m_parameter, second->m_parameter, val.m_diff_second);
            }
        }
    }
};

template<typename Sys, int Dim>
template<typename ConstraintVector, typename EquationVector>
Constraint<Sys, Dim>::holder<ConstraintVector, EquationVector>::MapSetter::MapSetter(MES& m, geom_ptr f, geom_ptr s)
    : mes(m), first(f), second(s) {

};

template<typename Sys, int Dim>
template<typename ConstraintVector, typename EquationVector>
template< typename T >
void Constraint<Sys, Dim>::holder<ConstraintVector, EquationVector>::MapSetter::operator()(T& val) const {

    if(!val.enabled)
        return;

    //when in cluster, there are 6 clusterparameter we differentiat for, if not we differentiat
    //for every parameter in the geometry;
    int equation = mes.setResidualMap(val.m_residual);
    if(first->getClusterMode()) {
        if(!first->isClusterFixed()) {
            mes.setJacobiMap(equation, first->m_offset_rot, 3, val.m_diff_first_rot);
            mes.setJacobiMap(equation, first->m_offset, 3, val.m_diff_first);
        }
    }
    else
        mes.setJacobiMap(equation, first->m_offset, first->m_parameterCount, val.m_diff_first);


    if(second->getClusterMode()) {
        if(!second->isClusterFixed()) {
            mes.setJacobiMap(equation, second->m_offset_rot, 3, val.m_diff_second_rot);
            mes.setJacobiMap(equation, second->m_offset, 3, val.m_diff_second);
        }
    }
    else
        mes.setJacobiMap(equation, second->m_offset, second->m_parameterCount, val.m_diff_second);
};

template<typename Sys, int Dim>
template<typename ConstraintVector, typename EquationVector>
Constraint<Sys, Dim>::holder<ConstraintVector, EquationVector>::PseudoCollector::PseudoCollector(geom_ptr f, geom_ptr s, Vec& vec1, Vec& vec2)
    : first(f), second(s), points1(vec1), points2(vec2) {

};

template<typename Sys, int Dim>
template<typename ConstraintVector, typename EquationVector>
template< typename T >
void Constraint<Sys, Dim>::holder<ConstraintVector, EquationVector>::PseudoCollector::operator()(T& val) const {

    if(!val.enabled)
        return;

    if(first->m_isInCluster && second->m_isInCluster) {
        val.m_eq.calculatePseudo(first->m_rotated, points1, second->m_rotated, points2);
    }
    else
        if(first->m_isInCluster) {
            typename Kernel::Vector sec = second->m_parameter;
            val.m_eq.calculatePseudo(first->m_rotated, points1, sec, points2);
        }
        else
            if(second->m_isInCluster) {
                typename Kernel::Vector fir = first->m_parameter;
                val.m_eq.calculatePseudo(fir, points1, second->m_rotated, points2);
            }
};

template<typename Sys, int Dim>
template<typename ConstraintVector, typename EquationVector>
Constraint<Sys, Dim>::holder<ConstraintVector, EquationVector>::LGZ::LGZ(geom_ptr f, geom_ptr s)
    : first(f), second(s) {

};

template<typename Sys, int Dim>
template<typename ConstraintVector, typename EquationVector>
template< typename T >
void Constraint<Sys, Dim>::holder<ConstraintVector, EquationVector>::LGZ::operator()(T& val) const {

    if(!val.enabled)
        return;

    //to treat local gradient zeros we calculate a approximate second derivative of the equations
    //only do that if neseccary: residual is not zero
    if(val.m_residual(0) > 1e-7) { //TODO: use exact precission and scale value

        //rotations exist only in cluster
        if(first->getClusterMode() && !first->isClusterFixed()) {
            //LGZ exists for rotations only
            for(int i=0; i<3; i++) {

                //only treat if the gradient realy is zero
                if(std::abs(val.m_diff_first_rot(i)) < 1e-7) {

                    //to get the approximated second derivative we need the slightly moved geometrie
                    const typename Kernel::Vector  p_old =  first->m_parameter;
                    first->m_parameter += first->m_diffparam.col(i)*1e-3;
                    first->normalize();
                    //with this changed geometrie we test if a gradient exist now
                    typename Kernel::VectorMap block(&first->m_diffparam(0,i),first->m_parameterCount,1, DS(1,1));
                    typename Kernel::number_type res = val.m_eq.calculateGradientFirst(first->m_parameter,
                                                       second->m_parameter, block);
                    first->m_parameter = p_old;

                    //let's see if the initial LGZ was a real one
                    if(std::abs(res) > 1e-7) {

                        //is a fake zero, let's correct it
                        val.m_diff_first_rot(i) = res;
                    };
                };
            };
        }
        //and the same for the second one too
        if(second->getClusterMode() && !second->isClusterFixed()) {

            for(int i=0; i<3; i++) {

                //only treat if the gradient realy is zero
                if(std::abs(val.m_diff_second_rot(i)) < 1e-7) {

                    //to get the approximated second derivative we need the slightly moved geometrie
                    const typename Kernel::Vector  p_old =  second->m_parameter;
                    second->m_parameter += second->m_diffparam.col(i)*1e-3;
                    second->normalize();
                    //with this changed geometrie we test if a gradient exist now
                    typename Kernel::VectorMap block(&second->m_diffparam(0,i),second->m_parameterCount,1, DS(1,1));
                    typename Kernel::number_type res = val.m_eq.calculateGradientFirst(first->m_parameter,
                                                       second->m_parameter, block);
                    second->m_parameter = p_old;

                    //let's see if the initial LGZ was a real one
                    if(std::abs(res) > 1e-7) {

                        //is a fake zero, let's correct it
                        val.m_diff_second_rot(i) = res;
                    };
                };
            };
        };
    };
};

template<typename Sys, int Dim>
template<typename ConstraintVector, typename EquationVector>
Constraint<Sys, Dim>::holder<ConstraintVector, EquationVector>::GenericEquations::GenericEquations(std::vector<boost::any>& v)
    : vec(v) {

};

template<typename Sys, int Dim>
template<typename ConstraintVector, typename EquationVector>
template< typename T >
void Constraint<Sys, Dim>::holder<ConstraintVector, EquationVector>::GenericEquations::operator()(T& val) const {
    vec.push_back(val.m_eq);
};

template<typename Sys, int Dim>
template<typename ConstraintVector, typename EquationVector>
Constraint<Sys, Dim>::holder<ConstraintVector, EquationVector>::GenericConstraints::GenericConstraints(std::vector<boost::any>& v)
    : vec(v) {

};

template<typename Sys, int Dim>
template<typename ConstraintVector, typename EquationVector>
template< typename T >
void Constraint<Sys, Dim>::holder<ConstraintVector, EquationVector>::GenericConstraints::operator()(T& val) const {
    vec.push_back(val);
};

template<typename Sys, int Dim>
template<typename ConstraintVector, typename EquationVector>
Constraint<Sys, Dim>::holder<ConstraintVector, EquationVector>::Types::Types(std::vector<const std::type_info*>& v)
    : vec(v) {

};

template<typename Sys, int Dim>
template<typename ConstraintVector, typename EquationVector>
template< typename T >
void Constraint<Sys, Dim>::holder<ConstraintVector, EquationVector>::Types::operator()(T& val) const {
    vec.push_back(&typeid(T));
};

template<typename Sys, int Dim>
template<typename ConstraintVector, typename EquationVector>
Constraint<Sys, Dim>::holder<ConstraintVector, EquationVector>::holder(Objects& obj) : m_objects(obj)  {
    //set the initial values in the equations
    fusion::for_each(m_sets, OptionSetter(obj));
};

template<typename Sys, int Dim>
template<typename ConstraintVector, typename EquationVector>
void Constraint<Sys, Dim>::holder<ConstraintVector, EquationVector>::calculate(geom_ptr first, geom_ptr second,
        Scalar scale, bool rotation_only) {
    fusion::for_each(m_sets, Calculater(first, second, scale, rotation_only));
};

template<typename Sys, int Dim>
template<typename ConstraintVector, typename EquationVector>
void Constraint<Sys, Dim>::holder<ConstraintVector, EquationVector>::treatLGZ(geom_ptr first, geom_ptr second) {
    fusion::for_each(m_sets, LGZ(first, second));
};

template<typename Sys, int Dim>
template<typename ConstraintVector, typename EquationVector>
typename Constraint<Sys, Dim>::placeholder*
Constraint<Sys, Dim>::holder<ConstraintVector, EquationVector>::resetConstraint(geom_ptr first, geom_ptr second) const {
    //boost::apply_visitor(creator, first->m_geometry, second->m_geometry);
    //if(creator.need_swap) first.swap(second);
    return NULL;
};

template<typename Sys, int Dim>
template<typename ConstraintVector, typename EquationVector>
void Constraint<Sys, Dim>::holder<ConstraintVector, EquationVector>::setMaps(MES& mes, geom_ptr first, geom_ptr second) {
    fusion::for_each(m_sets, MapSetter(mes, first, second));
};

template<typename Sys, int Dim>
template<typename ConstraintVector, typename EquationVector>
void Constraint<Sys, Dim>::holder<ConstraintVector, EquationVector>::collectPseudoPoints(geom_ptr f, geom_ptr s, Vec& vec1, Vec& vec2) {
    fusion::for_each(m_sets, PseudoCollector(f, s, vec1, vec2));
};

template<typename Sys, int Dim>
template<typename ConstraintVector, typename EquationVector>
typename Constraint<Sys, Dim>::placeholder*
Constraint<Sys, Dim>::holder<ConstraintVector, EquationVector>::clone() {
    return new holder(*this);
};

template<typename Sys, int Dim>
template<typename ConstraintVector, typename EquationVector>
int Constraint<Sys, Dim>::holder<ConstraintVector, EquationVector>::equationCount() {
    int count = 0;
    EquationCounter counter(count);
    fusion::for_each(m_sets, counter);
    return count;
};

template<typename Sys, int Dim>
template<typename ConstraintVector, typename EquationVector>
std::vector<boost::any>
Constraint<Sys, Dim>::holder<ConstraintVector, EquationVector>::getGenericEquations() {
    std::vector<boost::any> vec;
    fusion::for_each(m_sets, GenericEquations(vec));
    return vec;
};

template<typename Sys, int Dim>
template<typename ConstraintVector, typename EquationVector>
std::vector<boost::any>
Constraint<Sys, Dim>::holder<ConstraintVector, EquationVector>::getGenericConstraints() {
    std::vector<boost::any> vec;
    fusion::for_each(m_objects, GenericConstraints(vec));
    return vec;
};

template<typename Sys, int Dim>
template<typename ConstraintVector, typename EquationVector>
std::vector<const std::type_info*>
Constraint<Sys, Dim>::holder<ConstraintVector, EquationVector>::getEquationTypes() {
    std::vector<const std::type_info*> vec;
    mpl::for_each< EquationVector >(Types(vec));
    return vec;
};

template<typename Sys, int Dim>
template<typename ConstraintVector, typename EquationVector>
std::vector<const std::type_info*>
Constraint<Sys, Dim>::holder<ConstraintVector, EquationVector>::getConstraintTypes() {
    std::vector<const std::type_info*> vec;
    mpl::for_each< ConstraintVector >(Types(vec));
    return vec;
};

/****************************************************************/
/**	compiletime unrolled geometry initialising		*/
/****************************************************************/

template<typename Sys, int Dim>
template<typename WhichType, typename ConstraintVector>
void Constraint<Sys, Dim>::initializeFirstGeometry(ConstraintVector& cv, boost::mpl::false_ /*unrolled*/) {
    //this function is only for breaking the compilation loop, it should never be called
    BOOST_ASSERT(false); //Should never assert here; only meant to stop recursion at the end of the typelist
};

template<typename Sys, int Dim>
template<typename WhichType, typename ConstraintVector>
void Constraint<Sys, Dim>::initializeFirstGeometry(ConstraintVector& cv, boost::mpl::true_ /*unrolled*/) {

    typedef typename Sys::geometries geometries;

    switch(first->getExactType()) {

#ifdef BOOST_PP_LOCAL_ITERATE
#define BOOST_PP_LOCAL_MACRO(n) \
      case (WhichType::value + n): \
        return initializeSecondGeometry<boost::mpl::int_<0>,\
		typename mpl::at<geometries, typename in_range_value<geometries, WhichType::value + n>::type >::type,\
					ConstraintVector>(cv, typename boost::mpl::less<boost::mpl::int_<WhichType::value + n>, boost::mpl::size<geometries> >::type()); \
        break;
#define BOOST_PP_LOCAL_LIMITS (0, 10)
#include BOOST_PP_LOCAL_ITERATE()
#endif //BOOST_PP_LOCAL_ITERATE
    default:
        typedef typename mpl::int_<WhichType::value + 10> next_which_t;
        return initializeFirstGeometry<next_which_t, ConstraintVector> (cv,
                typename mpl::less< next_which_t, typename mpl::size<geometries>::type >::type());
    }
};

template<typename Sys, int Dim>
template<typename WhichType, typename FirstType, typename ConstraintVector>
void Constraint<Sys, Dim>::initializeSecondGeometry(ConstraintVector& cv, boost::mpl::false_ /*unrolled*/) {
    //this function is only for breaking the compilation loop, it should never be called
    BOOST_ASSERT(false); //Should never assert here; only meant to stop recursion at the end of the typelist
};

template<typename Sys, int Dim>
template<typename WhichType, typename FirstType, typename ConstraintVector>
void Constraint<Sys, Dim>::initializeSecondGeometry(ConstraintVector& cv, boost::mpl::true_ /*unrolled*/) {

    typedef typename Sys::geometries geometries;
    switch(second->getExactType()) {

#ifdef BOOST_PP_LOCAL_ITERATE
#define BOOST_PP_LOCAL_MACRO(n) \
      case (WhichType::value + n): \
        return intitalizeFinalize<FirstType, \
		typename mpl::at<geometries, typename in_range_value<geometries, WhichType::value + n>::type >::type,\
				  ConstraintVector>(cv, typename boost::mpl::less<boost::mpl::int_<WhichType::value + n>, boost::mpl::size<geometries> >::type()); \
        break;
#define BOOST_PP_LOCAL_LIMITS (0, 10)
#include BOOST_PP_LOCAL_ITERATE()
#endif //BOOST_PP_LOCAL_ITERATE
    default:
        typedef typename mpl::int_<WhichType::value + 10> next_which_t;
        return initializeSecondGeometry<next_which_t, FirstType, ConstraintVector>
               (cv, typename mpl::less
                < next_which_t
                , typename mpl::size<geometries>::type>::type()
               );
    }
};

template<typename Sys, int Dim>
template<typename FirstType, typename SecondType, typename ConstraintVector>
inline void Constraint<Sys, Dim>::intitalizeFinalize(ConstraintVector& cv, boost::mpl::true_ /*is_unrolled_t*/) {

    initializeFromTags<FirstType, SecondType>(cv);
};

template<typename Sys, int Dim>
template<typename FirstType, typename SecondType, typename ConstraintVector>
inline void Constraint<Sys, Dim>::intitalizeFinalize(ConstraintVector& cv, boost::mpl::false_ /*is_unrolled_t*/) {
    //Should never be here at runtime; only required to block code generation that deref's the sequence out of bounds
    BOOST_ASSERT(false);
}



};//detail

};//dcm

#endif //GCM_CONSTRAINT_H



