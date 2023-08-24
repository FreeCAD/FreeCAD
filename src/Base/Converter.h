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
 *   write to the Free Software Foundation, Inc., 51 Franklin Street,      *
 *   Fifth Floor, Boston, MA  02110-1301, USA                              *
 *                                                                         *
 ***************************************************************************/


#ifndef BASE_CONVERTER_H
#define BASE_CONVERTER_H

#include <tuple>
#include "Rotation.h"
#include "Vector3D.h"


namespace Base {

template <class vecT>
struct vec_traits { };

template <>
struct vec_traits<Vector3f> {
    using vec_type = Vector3f;
    using float_type = float;
    vec_traits(const vec_type& v) : v(v){}
    inline std::tuple<float_type,float_type,float_type> get() const {
        return std::make_tuple(v.x, v.y, v.z);
    }
private:
    const vec_type& v;
};

template <>
struct vec_traits<Vector3d> {
    using vec_type = Vector3d;
    using float_type = double;
    vec_traits(const vec_type& v) : v(v){}
    inline std::tuple<float_type,float_type,float_type> get() const {
        return std::make_tuple(v.x, v.y, v.z);
    }
private:
    const vec_type& v;
};

template <>
struct vec_traits<Rotation> {
    using vec_type = Rotation;
    using float_type = double;
    vec_traits(const vec_type& v) : v(v){}
    inline std::tuple<float_type,float_type,float_type,float_type> get() const {
        float_type q1{},q2{},q3{},q4{};
        v.getValue(q1,q2,q3,q4);
        return std::make_tuple(q1, q2, q3, q4);
    }
private:
    const vec_type& v;
};

// type with three floats
template <class _Vec, typename float_type>
_Vec make_vec(const std::tuple<float_type, float_type, float_type>&& t) {
    using traits_type = vec_traits<_Vec>;
    using float_traits_type = typename traits_type::float_type;
    return _Vec(float_traits_type(std::get<0>(t)),
                float_traits_type(std::get<1>(t)),
                float_traits_type(std::get<2>(t)));
}

// type with four floats
template <class _Vec, typename float_type>
_Vec make_vec(const std::tuple<float_type, float_type, float_type, float_type>&& t) {
    using traits_type = vec_traits<_Vec>;
    using float_traits_type = typename traits_type::float_type;
    return _Vec(float_traits_type(std::get<0>(t)),
                float_traits_type(std::get<1>(t)),
                float_traits_type(std::get<2>(t)),
                float_traits_type(std::get<3>(t)));
}

template <class _Vec1, class _Vec2>
inline _Vec1 convertTo(const _Vec2& v)
{
    using traits_type = vec_traits<_Vec2>;
    using float_type = typename traits_type::float_type;
    traits_type t(v);
    auto tuple = t.get();
    return make_vec<_Vec1, float_type>(std::move(tuple));
}

}

#endif // BASE_CONVERTER_H
