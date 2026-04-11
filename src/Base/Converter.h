// SPDX-License-Identifier: LGPL-2.1-or-later

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


#pragma once

#include <tuple>
#include "Rotation.h"
#include "Vector3D.h"


namespace Base
{

template<class vecT>
struct vec_traits
{
};

template<>
struct vec_traits<Vector3f>
{
    using vec_type = Vector3f;
    using float_type = float;
    explicit vec_traits(const vec_type& vec)
        : v(vec)
    {}
    inline std::tuple<float_type, float_type, float_type> get() const
    {
        return std::make_tuple(v.x, v.y, v.z);
    }

private:
    const vec_type& v;
};

template<>
struct vec_traits<Vector3d>
{
    using vec_type = Vector3d;
    using float_type = double;
    explicit vec_traits(const vec_type& vec)
        : v(vec)
    {}
    inline std::tuple<float_type, float_type, float_type> get() const
    {
        return std::make_tuple(v.x, v.y, v.z);
    }

private:
    const vec_type& v;
};

template<>
struct vec_traits<Rotation>
{
    using vec_type = Rotation;
    using float_type = double;
    explicit vec_traits(const vec_type& vec)
        : v(vec)
    {}
    inline std::tuple<float_type, float_type, float_type, float_type> get() const
    {
        float_type q1 {};
        float_type q2 {};
        float_type q3 {};
        float_type q4 {};
        v.getValue(q1, q2, q3, q4);
        return std::make_tuple(q1, q2, q3, q4);
    }

private:
    const vec_type& v;
};

// type with three floats
template<class Vec, typename float_type>
Vec make_vec(const std::tuple<float_type, float_type, float_type>&& ft)
{
    using traits_type = vec_traits<Vec>;
    using float_traits_type = typename traits_type::float_type;
    return Vec(
        float_traits_type(std::get<0>(ft)),
        float_traits_type(std::get<1>(ft)),
        float_traits_type(std::get<2>(ft))
    );
}

// type with four floats
template<class Vec, typename float_type>
Vec make_vec(const std::tuple<float_type, float_type, float_type, float_type>&& ft)
{
    using traits_type = vec_traits<Vec>;
    using float_traits_type = typename traits_type::float_type;
    return Vec(
        float_traits_type(std::get<0>(ft)),
        float_traits_type(std::get<1>(ft)),
        float_traits_type(std::get<2>(ft)),
        float_traits_type(std::get<3>(ft))
    );
}

template<class Vec1, class Vec2>
inline Vec1 convertTo(const Vec2& vec)
{
    using traits_type = vec_traits<Vec2>;
    using float_type = typename traits_type::float_type;
    traits_type tt(vec);
    auto tuple = tt.get();
    return make_vec<Vec1, float_type>(std::move(tuple));
}

}  // namespace Base
