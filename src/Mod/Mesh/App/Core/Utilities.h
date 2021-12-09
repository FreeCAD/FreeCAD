/***************************************************************************
 *   Copyright (c) 2019 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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


#ifndef MESH_UTILITIES_H
#define MESH_UTILITIES_H

#include <Base/Converter.h>
#include <Mod/Mesh/App/WildMagic4/Wm4Vector3.h>

namespace Base {
// Specialization for Wm4::Vector3d
template <>
struct vec_traits<Wm4::Vector3d> {
    typedef Wm4::Vector3d vec_type;
    typedef double float_type;
    vec_traits(const vec_type& v) : v(v){}
    inline std::tuple<float_type,float_type,float_type> get() const {
        return std::make_tuple(v.X(), v.Y(), v.Z());
    }
private:
    const vec_type& v;
};
// Specialization for Wm4::Vector3f
template <>
struct vec_traits<Wm4::Vector3f> {
    typedef Wm4::Vector3f vec_type;
    typedef float float_type;
    vec_traits(const vec_type& v) : v(v){}
    inline std::tuple<float_type,float_type,float_type> get() const {
        return std::make_tuple(v.X(), v.Y(), v.Z());
    }
private:
    const vec_type& v;
};
}


#endif // MESH_UTILITIES_H
