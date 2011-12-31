/***************************************************************************
 *   Copyright (c) 2011 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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


#ifndef PART_TOOLS_H
#define PART_TOOLS_H

#include <gp_Pnt.hxx>
#include <gp_Vec.hxx>
#include <gp_Dir.hxx>
#include <gp_XYZ.hxx>

namespace Base {
// Specialization for gp_Pnt
template <>
struct vec_traits<gp_Pnt> {
    typedef gp_Pnt vec_type;
    typedef double float_type;
    vec_traits(const vec_type& v) : v(v){}
    inline float_type x() { return v.X(); }
    inline float_type y() { return v.Y(); }
    inline float_type z() { return v.Z(); }
private:
    const vec_type& v;
};
// Specialization for gp_Vec
template <>
struct vec_traits<gp_Vec> {
    typedef gp_Vec vec_type;
    typedef double float_type;
    vec_traits(const vec_type& v) : v(v){}
    inline float_type x() { return v.X(); }
    inline float_type y() { return v.Y(); }
    inline float_type z() { return v.Z(); }
private:
    const vec_type& v;
};
// Specialization for gp_Dir
template <>
struct vec_traits<gp_Dir> {
    typedef gp_Dir vec_type;
    typedef double float_type;
    vec_traits(const vec_type& v) : v(v){}
    inline float_type x() { return v.X(); }
    inline float_type y() { return v.Y(); }
    inline float_type z() { return v.Z(); }
private:
    const vec_type& v;
};
// Specialization for gp_XYZ
template <>
struct vec_traits<gp_XYZ> {
    typedef gp_XYZ vec_type;
    typedef double float_type;
    vec_traits(const vec_type& v) : v(v){}
    inline float_type x() { return v.X(); }
    inline float_type y() { return v.Y(); }
    inline float_type z() { return v.Z(); }
private:
    const vec_type& v;
};
}

namespace Part
{

} //namespace Part


#endif // PART_TOOLS_H
