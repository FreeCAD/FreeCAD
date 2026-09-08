// SPDX-License-Identifier: LGPL-2.1-or-later

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

#pragma once

#include <gp_Dir.hxx>
#include <gp_Pnt.hxx>
#include <gp_Vec.hxx>
#include <gp_XYZ.hxx>

#include <Base/Converter.h>

namespace Base
{
// Specialization for gp_Pnt
template<>
struct vec_traits<gp_Pnt>
{
    using vec_type = gp_Pnt;
    using float_type = double;
    explicit vec_traits(const vec_type& v)
        : v(v)
    {}
    inline std::tuple<float_type, float_type, float_type> get() const
    {
        return std::make_tuple(v.X(), v.Y(), v.Z());
    }

private:
    const vec_type& v;
};
// Specialization for gp_Vec
template<>
struct vec_traits<gp_Vec>
{
    using vec_type = gp_Vec;
    using float_type = double;
    explicit vec_traits(const vec_type& v)
        : v(v)
    {}
    inline std::tuple<float_type, float_type, float_type> get() const
    {
        return std::make_tuple(v.X(), v.Y(), v.Z());
    }

private:
    const vec_type& v;
};
// Specialization for gp_Dir
template<>
struct vec_traits<gp_Dir>
{
    using vec_type = gp_Dir;
    using float_type = double;
    explicit vec_traits(const vec_type& v)
        : v(v)
    {}
    inline std::tuple<float_type, float_type, float_type> get() const
    {
        return std::make_tuple(v.X(), v.Y(), v.Z());
    }

private:
    const vec_type& v;
};
// Specialization for gp_XYZ
template<>
struct vec_traits<gp_XYZ>
{
    using vec_type = gp_XYZ;
    using float_type = double;
    explicit vec_traits(const vec_type& v)
        : v(v)
    {}
    inline std::tuple<float_type, float_type, float_type> get() const
    {
        return std::make_tuple(v.X(), v.Y(), v.Z());
    }

private:
    const vec_type& v;
};
}  // namespace Base
