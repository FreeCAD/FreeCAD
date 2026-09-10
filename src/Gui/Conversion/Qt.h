/***************************************************************************
 *   Copyright (c) 2010 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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

#include <QColor>

#include <Base/Color.h>
#include <Base/Converter.h>


namespace Base
{

template<>
struct vec_traits<QColor>
{
    using vec_type = QColor;
    using float_type = float;
    explicit vec_traits(const vec_type& v)
        : v(v)
    {}
    inline std::tuple<float_type, float_type, float_type> get() const
    {
        return std::make_tuple(v.redF(), v.greenF(), v.blueF());
    }

private:
    const vec_type& v;
};
// Specialization for QColor
template<>
struct color_traits<QColor>
{
    using color_type = QColor;
    color_traits() = default;
    explicit color_traits(const color_type& ct)
        : ct(ct)
    {}
    float redF() const
    {
        return static_cast<float>(ct.redF());
    }
    float greenF() const
    {
        return static_cast<float>(ct.greenF());
    }
    float blueF() const
    {
        return static_cast<float>(ct.blueF());
    }
    float alphaF() const
    {
        return static_cast<float>(ct.alphaF());
    }
    void setRedF(float red)
    {
        ct.setRedF(red);
    }
    void setGreenF(float green)
    {
        ct.setGreenF(green);
    }
    void setBlueF(float blue)
    {
        ct.setBlueF(blue);
    }
    void setAlphaF(float alpha)
    {
        ct.setAlphaF(alpha);
    }
    int red() const
    {
        return ct.red();
    }
    int green() const
    {
        return ct.green();
    }
    int blue() const
    {
        return ct.blue();
    }
    int alpha() const
    {
        return ct.alpha();
    }
    void setRed(int red)
    {
        ct.setRed(red);
    }
    void setGreen(int green)
    {
        ct.setGreen(green);
    }
    void setBlue(int blue)
    {
        ct.setBlue(blue);
    }
    void setAlpha(int alpha)
    {
        ct.setAlpha(alpha);
    }
    static color_type makeColor(int red, int green, int blue, int alpha = 255)
    {
        return color_type {red, green, blue, alpha};
    }

private:
    color_type ct;
};

}  // namespace Base
