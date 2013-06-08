/***************************************************************************
 *   Copyright (c) 2013 Stefan Tröger <stefantroeger@gmx.net>              *
 *                                                                         *
 *   This file is part of the FreeCAD CAx development system.              *
 *                                                                         *
 *   This library is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU Library General Public           *
 *   License as published by the Free Software Foundation; either          *
 *   version 2 of the License, or (at your option) any later version.      *
 *                                                                         *
 *   This library  is distributed in the hope that it will be useful,      *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU Library General Public License for more details.                  *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this library; see the file COPYING.LIB. If not,    *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,         *
 *   Suite 330, Boston, MA  02111-1307, USA                                *
 *                                                                         *
 ***************************************************************************/

#ifndef SOLVER_H
#define SOLVER_H

#include <Base/Placement.h>

#include <gp_Pnt.hxx>
#include <gp_Lin.hxx>
#include <gp_Pln.hxx>
#include <gp_Cylinder.hxx>

#include "opendcm/core.hpp"
#include "opendcm/module3d.hpp"
#include "opendcm/modulepart.hpp"

struct gp_pnt_accessor {

    template<typename Scalar, int ID, typename T>
    Scalar get(T& t) {
        switch(ID) {
            case 0:
                return t.X();
            case 1:
                return t.Y();
            case 2:
                return t.Z();
            default:
                return 0;
        };
    };
    template<typename Scalar, int ID, typename T>
    void set(Scalar value, T& t) {
        switch(ID) {
            case 0:
                t.SetX(value);
                break;
            case 1:
                t.SetY(value);
                break;
            case 2:
                t.SetZ(value);
                break;
        };
    };
};

struct gp_lin_accessor {

    template<typename Scalar, int ID, typename T>
    Scalar get(T& t) {
        switch(ID) {
            case 0:
                return t.Location().X();
            case 1:
                return t.Location().Y();
            case 2:
                return t.Location().Z();
            case 3:
                return t.Direction().X();
            case 4:
                return t.Direction().Y();
            case 5:
                return t.Direction().Z();
            default:
                return 0;
        };
    };
    template<typename Scalar, int ID, typename T>
    void set(Scalar value, T& t) {
        gp_Pnt p = t.Location();
        gp_Dir d = t.Direction();
        switch(ID) {
            case 0:
                p.SetX(value);
                break;
            case 1:
                p.SetY(value);
                break;
            case 2:
                p.SetZ(value);
		break;
            case 3:
                d.SetX(value);
                break;
            case 4:
                d.SetY(value);
                break;
            case 5:
                d.SetZ(value);
                break;
        };
        t.SetLocation(p);
        t.SetDirection(d);
    };
};

struct gp_pln_accessor {

    template<typename Scalar, int ID, typename T>
    Scalar get(T& t) {
        switch(ID) {
            case 0:
                return t.Axis().Location().X();
            case 1:
                return t.Axis().Location().Y();
            case 2:
                return t.Axis().Location().Z();
            case 3:
                return t.Axis().Direction().X();
            case 4:
                return t.Axis().Direction().Y();
            case 5:
                return t.Axis().Direction().Z();
            default:
                return 0;
        };
    };
    template<typename Scalar, int ID, typename T>
    void set(Scalar value, T& t) {
        gp_Pnt p = t.Axis().Location();
        gp_Dir d = t.Axis().Direction();
        switch(ID) {
            case 0:
                p.SetX(value);
                break;
            case 1:
                p.SetY(value);
                break;
            case 2:
                p.SetZ(value);
		break;
            case 3:
                d.SetX(value);
                break;
            case 4:
                d.SetY(value);
                break;
            case 5:
                d.SetZ(value);
                break;
        };
        t.SetAxis(gp_Ax1(p,d));
    };
};

struct gp_cylinder_accessor {

    template<typename Scalar, int ID, typename T>
    Scalar get(T& t) {
        switch(ID) {
            case 0:
                return t.Axis().Location().X();
            case 1:
                return t.Axis().Location().Y();
            case 2:
                return t.Axis().Location().Z();
            case 3:
                return t.Axis().Direction().X();
            case 4:
                return t.Axis().Direction().Y();
            case 5:
                return t.Axis().Direction().Z();
            case 6:
                return t.Radius();
            default:
                return 0;
        };
    };
    template<typename Scalar, int ID, typename T>
    void set(Scalar value, T& t) {
        gp_Pnt p = t.Axis().Location();
        gp_Dir d = t.Axis().Direction();
        switch(ID) {
            case 0:
                p.SetX(value);
                break;
            case 1:
                p.SetY(value);
                break;
            case 2:
                p.SetZ(value);
		break;
            case 3:
                d.SetX(value);
                break;
            case 4:
                d.SetY(value);
                break;
            case 5:
                d.SetZ(value);
                break;
            case 6:
                t.SetRadius(value);
		break;
        };
        t.SetAxis(gp_Ax1(p,d));
    };
};

struct placement_accessor {

    double q0, q1, q2, q3;
    Base::Vector3d vec;

    template<typename Scalar, int ID, typename T>
    Scalar get(T& t) {
        t.getRotation().getValue(q0,q1,q2,q3);
        switch(ID) {
            case 0:
                return q3;
            case 1:
                return q0;
            case 2:
                return q1;
            case 3:
                return q2;
            case 4:
                return t.getPosition()[0];
            case 5:
                return t.getPosition()[1];
            case 6:
                return t.getPosition()[2];
            default:
                return 0;
        };
    };
    template<typename Scalar, int ID, typename T>
    void set(Scalar value, T& t) {
        switch(ID) {
            case 0:
                q3 = value;
                break;
            case 1:
                q0 = value;
                break;
            case 2:
                q1 = value;
                break;
            case 3:
                q2 = value;
                break;
            case 4:
                vec[0] = value;
                break;
            case 5:
                vec[1] = value;
                break;
            case 6:
                vec[2] = value;
                break;
        };
    };

    template<typename T>
    void finalize(T& t) {
        //need to do it at once as setting every value step by step would always normalize the rotation and
        //therefor give a false value
        Base::Rotation rot(q0,q1,q2,q3);
        t.setRotation(rot);
        t.setPosition(vec);
    };
};

//geometry_traits for opencascade
namespace dcm {
template<>
struct geometry_traits<gp_Pnt> {
    typedef tag::point3D  tag;
    typedef modell::XYZ modell;
    typedef gp_pnt_accessor accessor;
};
template<>
struct geometry_traits<gp_Lin> {
    typedef tag::line3D  tag;
    typedef modell::XYZ2 modell;
    typedef gp_lin_accessor accessor;
};
template<>
struct geometry_traits<gp_Pln> {
    typedef tag::plane3D  tag;
    typedef modell::XYZ2 modell;
    typedef gp_pln_accessor accessor;
};
template<>
struct geometry_traits<gp_Cylinder> {
    typedef tag::cylinder3D  tag;
    typedef modell::XYZ2P modell;
    typedef gp_cylinder_accessor accessor;
};
template<>
struct geometry_traits<Base::Placement> {
    typedef tag::part  tag;
    typedef modell::quaternion_wxyz_vec3 modell;
    typedef placement_accessor accessor;
};
}

//our constraint solving system
typedef dcm::Kernel<double> Kernel;
typedef dcm::Module3D< mpl::vector4< gp_Pnt, gp_Lin, gp_Pln, gp_Cylinder>, std::string > Module3D;
typedef dcm::ModulePart< mpl::vector1< Base::Placement >, std::string > ModulePart;
typedef dcm::System<Kernel, Module3D, ModulePart> Solver;

typedef ModulePart::type<Solver>::Part Part3D;
typedef Module3D::type<Solver>::Geometry3D Geometry3D;
typedef Module3D::type<Solver>::Constraint3D Constraint3D;


#endif //SOLVER_H