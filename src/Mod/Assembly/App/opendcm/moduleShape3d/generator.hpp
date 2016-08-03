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

#ifndef GCM_GENERATOR_SHAPE3D_H
#define GCM_GENERATOR_SHAPE3D_H

#include <opendcm/core/defines.hpp>
#include <opendcm/core/geometry.hpp>
#include <opendcm/module3d/defines.hpp>
#include "geometry.hpp"
#include "fixed.hpp"
#include "defines.hpp"

#include <boost/exception/errinfo_errno.hpp>
#include <boost/bind.hpp>

namespace dcm {

namespace details {

template<typename Sys>
struct ShapeGeneratorBase {

    BOOST_MPL_ASSERT((typename system_traits<Sys>::template getModule<details::m3d>::has_module));
    typedef typename system_traits<Sys>::template getModule<details::m3d>::type module3d;
    typedef typename module3d::Geometry3D Geometry3D;
    typedef typename module3d::Constraint3D Constraint3D;
    typedef typename system_traits<Sys>::template getModule<details::mshape3d>::type moduleShape3d;
    typedef typename moduleShape3d::Shape3D Shape3D;
    
    typedef typename Shape3D::ShapeVector ShapeVector;
    typedef typename Shape3D::GeometryVector GeometryVector;
    typedef typename Shape3D::ConstraintVector ConstraintVector;

    typedef typename moduleShape3d::shape_purpose_prop shape_purpose_prop;
    typedef typename moduleShape3d::shape_geometry_prop shape_geometry_prop;
    typedef typename moduleShape3d::shape_constraint_prop shape_constraint_prop;

    Sys* m_system;
    boost::shared_ptr<Shape3D> m_shape;
    GeometryVector*   m_geometries;
    ShapeVector* m_shapes;
    ConstraintVector* m_constraints;

    ShapeGeneratorBase(Sys* system) : m_system(system) {};
    virtual ~ShapeGeneratorBase() {};

    void set(boost::shared_ptr<Shape3D> shape,
             GeometryVector*   geometries,
             ShapeVector* shapes,
             ConstraintVector* constraints) {

        m_shape = shape;
        m_geometries = geometries;
        m_shapes = shapes;
        m_constraints = constraints;
    };

    //check if all needed parts are supplied
    virtual bool check() = 0;
    //initialise all relations between the geometries
    virtual void init() = 0;
    //get geometry3d for optional types (e.g. midpoints)
    virtual boost::shared_ptr<Geometry3D> getOrCreateG3d(int type) = 0;
    //get hlgeometry3d for optional types
    virtual boost::shared_ptr<Shape3D> getOrCreateHLG3d(int type) = 0;
    
    //append needs to be on this base class as the shape appends are protected
    void append(boost::shared_ptr<Geometry3D> g) {
      m_shape->append(g);
    };
    void append(boost::shared_ptr<Shape3D> g) {
      m_shape->append(g);
    };
    void append(boost::shared_ptr<Constraint3D> g) {
      m_shape->append(g);
    };
};

} //details


struct dummy_generator {

    template<typename Sys>
    struct type : public details::ShapeGeneratorBase<Sys> {

        type(Sys* system) : details::ShapeGeneratorBase<Sys>(system) {};

        //check if all needed parts are supplied
        virtual bool check() {
            throw creation_error() <<  boost::errinfo_errno(210) << error_message("not all needd types for high level geometry present");
        };
        //initialise all relations between the geometries, throw on error
        virtual void init() {
            throw creation_error() <<  boost::errinfo_errno(211) << error_message("dummy generator can't create high level geometry");
        };
        //get geometry3d for optional types (e.g. midpoints)
        virtual boost::shared_ptr<typename details::ShapeGeneratorBase<Sys>::Geometry3D> getOrCreateG3d(int type) {
            throw creation_error() <<  boost::errinfo_errno(212) << error_message("dummy generator has no geometry to access");
        };
        //get hlgeometry3d for optional types
        virtual boost::shared_ptr<typename details::ShapeGeneratorBase<Sys>::Shape3D> getOrCreateHLG3d(int type) {
            throw creation_error() <<  boost::errinfo_errno(213) << error_message("dummy generator has no high level geometry to access");
        };
    };
};

//test generator
struct segment3D {

    template<typename Sys>
    struct type : public dcm::details::ShapeGeneratorBase<Sys> {

        typedef dcm::details::ShapeGeneratorBase<Sys> base;
        typedef typename Sys::Kernel Kernel;
        using typename base::Geometry3D;
        using typename base::Constraint3D;
        using typename base::Shape3D;

        type(Sys* system) : details::ShapeGeneratorBase<Sys>(system) {};

        //check if all needed parts are supplied, a segment needs 2 points
        virtual bool check() {

            //even we have a real geometry segment
            if(base::m_shape->getGeometryType() == tag::weight::segment::value)
                return true;

            //or two point geometries
            if(base::m_geometries->size() == 2)
                return true;

            return false;
        };
        //initialise all relations between the geometries
        virtual void init() {

            if(base::m_shape->getGeometryType() == dcm::tag::weight::segment::value) {

                //link the line geometrie to our shape
                boost::shared_ptr<Geometry3D> g1 = base::m_system->createGeometry3D();
                base::append(g1);
                g1->template linkTo<tag::segment3D>(base::m_shape,0);
                g1->template setProperty<typename base::shape_purpose_prop>(line);
                g1->template connectSignal<recalculated>(boost::bind(&base::Shape3D::recalc, base::m_shape, _1));

                //we have a segment, lets link the two points to it
                boost::shared_ptr<Geometry3D> g2 = base::m_system->createGeometry3D();
                base::append(g2);
                g2->template setProperty<typename base::shape_purpose_prop>(startpoint);
                boost::shared_ptr<Geometry3D> g3 = base::m_system->createGeometry3D();
                base::append(g3);
                g3->template setProperty<typename base::shape_purpose_prop>(endpoint);

                //link the points to our new segment
                g2->template linkTo<tag::point3D>(base::m_shape, 0);
                g3->template linkTo<tag::point3D>(base::m_shape, 3);

                //add the fix constraints
                boost::shared_ptr<Constraint3D> c1 = base::m_system->createConstraint3D(g1,g2, details::fixed);
                boost::shared_ptr<Constraint3D> c2 = base::m_system->createConstraint3D(g1,g3, details::fixed);
                c1->disable(); //required by fixed constraint
                base::append(c1);
                c2->disable(); //requiered by fixed constraint
                base::append(c2);
            }
            else if(base::m_geometries->size() == 2) {
                //we have two points, lets get them
                boost::shared_ptr<Geometry3D> g1 = fusion::at_c<0>(base::m_geometries->operator[](0));
                boost::shared_ptr<Geometry3D> g2 = fusion::at_c<0>(base::m_geometries->operator[](1));

                //possibility 1: two points. we add a segment line an link the point in
                if(g1->getGeometryType() == tag::weight::point::value || g2->getGeometryType() == tag::weight::point::value) {

                    g1->template setProperty<typename base::shape_purpose_prop>(startpoint);
                    g2->template setProperty<typename base::shape_purpose_prop>(endpoint);

                    //construct our segment value
                    typename Kernel::Vector val(6);
                    val.head(3) = g1->getValue();
                    val.tail(3) = g2->getValue();

                    //the shape is a segment
                    base::m_shape->template setValue<tag::segment3D>(val);

                    //and create a segment geometry we use as line
                    boost::shared_ptr<Geometry3D> g3 = base::m_system->createGeometry3D();
                    base::append(g3);
                    g3->template linkTo<tag::segment3D>(base::m_shape,0);
                    g3->template setProperty<typename base::shape_purpose_prop>(line);
                    g3->template connectSignal<recalculated>(boost::bind(&base::Shape3D::recalc, base::m_shape, _1));

                    //link the points to our new segment
                    g1->template linkTo<tag::point3D>(base::m_shape, 0);
                    g2->template linkTo<tag::point3D>(base::m_shape, 3);

                    //add the fix constraints to show our relation
                    boost::shared_ptr<Constraint3D> c1 = base::m_system->createConstraint3D(g1,g3, details::fixed);
                    boost::shared_ptr<Constraint3D> c2 = base::m_system->createConstraint3D(g1,g3, details::fixed);
                    c1->disable(); //required by fixed constraint
                    base::append(c1);
                    c2->disable(); //requiered by fixed constraint
                    base::append(c2);

                }
                else
                    throw creation_error() <<  boost::errinfo_errno(501) << error_message("Wrong geometries for segment construction");
            };
        };
        //get geometry3d for optional types (e.g. midpoints)
        virtual boost::shared_ptr<typename dcm::details::ShapeGeneratorBase<Sys>::Geometry3D> getOrCreateG3d(int type) {
            return boost::shared_ptr<typename dcm::details::ShapeGeneratorBase<Sys>::Geometry3D>();
        };
        //get hlgeometry3d for optional types
        virtual boost::shared_ptr<typename dcm::details::ShapeGeneratorBase<Sys>::Shape3D> getOrCreateHLG3d(int type) {
            return boost::shared_ptr<typename dcm::details::ShapeGeneratorBase<Sys>::Shape3D>();
        };
    };
};

} //dcm


#endif //GCM_GENERATOR_SHAPE3D_H
