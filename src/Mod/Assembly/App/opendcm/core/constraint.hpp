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

#include <boost/mpl/vector.hpp>
#include <boost/mpl/map.hpp>
#include <boost/mpl/less.hpp>
#include <boost/mpl/if.hpp>
#include <boost/mpl/size.hpp>
#include <boost/mpl/for_each.hpp>

#include <boost/any.hpp>
#include <boost/fusion/include/as_vector.hpp>

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


    int  equationCount();
    void calculate(Scalar scale, AccessType access = general);
    void treatLGZ();
    void setMaps(MES& mes);
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

        bool enabled;
	AccessType access;

        typedef Equation eq_type;
    };

    struct placeholder  {
        virtual ~placeholder() {}
        virtual placeholder* resetConstraint(geom_ptr first, geom_ptr second) const = 0;
        virtual void calculate(geom_ptr first, geom_ptr second, Scalar scale, AccessType access = general) = 0;
        virtual void treatLGZ(geom_ptr first, geom_ptr second) = 0;
        virtual int  equationCount() = 0;
        virtual void setMaps(MES& mes, geom_ptr first, geom_ptr second) = 0;
        virtual void collectPseudoPoints(geom_ptr first, geom_ptr second, Vec& vec1, Vec& vec2) = 0;
        virtual void disable() = 0;
        virtual placeholder* clone() = 0;

        //some runtime type infos are needed, as we can't access the contents with arbitrary functors
        virtual std::vector<boost::any> getGenericEquations() = 0;
        virtual std::vector<boost::any> getGenericConstraints() = 0;
        virtual std::vector<const std::type_info*> getEquationTypes() = 0;
        virtual std::vector<const std::type_info*> getConstraintTypes() = 0;
    };
    int value;

public:
    template< typename ConstraintVector, typename tag1, typename tag2>
    struct holder : public placeholder  {

        //transform the constraints into eqautions with the now known types
        typedef typename mpl::fold< ConstraintVector, mpl::vector<>,
                mpl::push_back<mpl::_1, equation<mpl::_2, tag1, tag2> > >::type EquationVector;

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
            AccessType access;

            Calculater(geom_ptr f, geom_ptr s, Scalar sc, AccessType a = general);

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

        virtual void calculate(geom_ptr first, geom_ptr second, Scalar scale, AccessType a = general);
        virtual void treatLGZ(geom_ptr first, geom_ptr second);
        virtual placeholder* resetConstraint(geom_ptr first, geom_ptr second) const;
        virtual void setMaps(MES& mes, geom_ptr first, geom_ptr second);
        virtual void collectPseudoPoints(geom_ptr f, geom_ptr s, Vec& vec1, Vec& vec2);
        virtual placeholder* clone();
        virtual int equationCount();
        virtual void disable();

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

};//detail
};//dcm

#ifndef DCM_EXTERNAL_CORE
#include "imp/constraint_imp.hpp"
#include "imp/constraint_holder_imp.hpp"
#endif

#endif //GCM_CONSTRAINT_H



