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

#include <cmath>

#include <Inventor/SbColor.h>
#include <Inventor/SbColor4f.h>
#include <Inventor/SbMatrix.h>
#include <Inventor/SbRotation.h>
#include <Inventor/SbVec3d.h>
#include <Inventor/SbVec3f.h>

#include <Base/Color.h>
#include <Base/Converter.h>
#include <Base/Matrix.h>


namespace Base
{

// Specialization for SbVec3f
template<>
struct vec_traits<SbVec3f>
{
    using vec_type = SbVec3f;
    using float_type = float;
    explicit vec_traits(const vec_type& v)
        : v(v)
    {}
    inline std::tuple<float_type, float_type, float_type> get() const
    {
        return std::make_tuple(v[0], v[1], v[2]);
    }

private:
    const vec_type& v;
};
// Specialization for SbVec3d
template<>
struct vec_traits<SbVec3d>
{
    using vec_type = SbVec3d;
    using float_type = double;
    explicit vec_traits(const vec_type& v)
        : v(v)
    {}
    inline std::tuple<float_type, float_type, float_type> get() const
    {
        return std::make_tuple(v[0], v[1], v[2]);
    }

private:
    const vec_type& v;
};
// Specialization for SbRotation
template<>
struct vec_traits<SbRotation>
{
    using vec_type = SbRotation;
    using float_type = float;
    explicit vec_traits(const vec_type& v)
        : v(v)
    {}
    inline std::tuple<float_type, float_type, float_type, float_type> get() const
    {
        float_type q1, q2, q3, q4;
        v.getValue(q1, q2, q3, q4);
        return std::make_tuple(q1, q2, q3, q4);
    }

private:
    const vec_type& v;
};
// Specialization for SbColor
template<>
struct vec_traits<SbColor>
{
    using vec_type = SbColor;
    using float_type = float;
    explicit vec_traits(const vec_type& v)
        : v(v)
    {}
    inline std::tuple<float_type, float_type, float_type> get() const
    {
        return std::make_tuple(v[0], v[1], v[2]);
    }

private:
    const vec_type& v;
};
// Specialization for Color
template<>
struct vec_traits<Base::Color>
{
    using vec_type = Base::Color;
    using float_type = float;
    explicit vec_traits(const vec_type& v)
        : v(v)
    {}
    inline std::tuple<float_type, float_type, float_type> get() const
    {
        return std::make_tuple(v.r, v.g, v.b);
    }

private:
    const vec_type& v;
};
// Specialization for SbColor
template<>
struct color_traits<SbColor>
{
    using color_type = SbColor;
    color_traits() = default;
    explicit color_traits(const color_type& ct)
        : ct(ct)
    {}
    float redF() const
    {
        return ct[0];
    }
    float greenF() const
    {
        return ct[1];
    }
    float blueF() const
    {
        return ct[2];
    }
    float alphaF() const
    {
        return 1.0F;
    }
    void setRedF(float red)
    {
        ct[0] = red;
    }
    void setGreenF(float green)
    {
        ct[1] = green;
    }
    void setBlueF(float blue)
    {
        ct[2] = blue;
    }
    void setAlphaF(float alpha)
    {
        (void)alpha;
    }
    int red() const
    {
        return int(std::lround(ct[0] * 255.0F));
    }
    int green() const
    {
        return int(std::lround(ct[1] * 255.0F));
    }
    int blue() const
    {
        return int(std::lround(ct[2] * 255.0F));
    }
    int alpha() const
    {
        return 255;
    }
    void setRed(int red)
    {
        ct[0] = static_cast<float>(red) / 255.0F;
    }
    void setGreen(int green)
    {
        ct[1] = static_cast<float>(green) / 255.0F;
    }
    void setBlue(int blue)
    {
        ct[2] = static_cast<float>(blue) / 255.0F;
    }
    void setAlpha(int alpha)
    {
        (void)alpha;
    }
    static color_type makeColor(int red, int green, int blue, int alpha = 255)
    {
        (void)alpha;
        return color_type {
            static_cast<float>(red) / 255.0F,
            static_cast<float>(green) / 255.0F,
            static_cast<float>(blue) / 255.0F
        };
    }

private:
    color_type ct;
};
// Specialization for SbColor4f
template<>
struct color_traits<SbColor4f>
{
    using color_type = SbColor4f;
    color_traits() = default;
    explicit color_traits(const color_type& ct)
        : ct(ct)
    {}
    float redF() const
    {
        return ct[0];
    }
    float greenF() const
    {
        return ct[1];
    }
    float blueF() const
    {
        return ct[2];
    }
    float alphaF() const
    {
        return ct[3];
    }
    void setRedF(float red)
    {
        ct[0] = red;
    }
    void setGreenF(float green)
    {
        ct[1] = green;
    }
    void setBlueF(float blue)
    {
        ct[2] = blue;
    }
    void setAlphaF(float alpha)
    {
        ct[3] = alpha;
    }
    int red() const
    {
        return int(std::lround(ct[0] * 255.0F));
    }
    int green() const
    {
        return int(std::lround(ct[1] * 255.0F));
    }
    int blue() const
    {
        return int(std::lround(ct[2] * 255.0F));
    }
    int alpha() const
    {
        return int(std::lround(ct[3] * 255.0F));
    }
    void setRed(int red)
    {
        ct[0] = static_cast<float>(red) / 255.0F;
    }
    void setGreen(int green)
    {
        ct[1] = static_cast<float>(green) / 255.0F;
    }
    void setBlue(int blue)
    {
        ct[2] = static_cast<float>(blue) / 255.0F;
    }
    void setAlpha(int alpha)
    {
        ct[3] = static_cast<float>(alpha) / 255.0F;
    }
    static color_type makeColor(int red, int green, int blue, int alpha = 255)
    {
        return color_type {
            static_cast<float>(red) / 255.0F,
            static_cast<float>(green) / 255.0F,
            static_cast<float>(blue) / 255.0F,
            static_cast<float>(alpha) / 255.0F
        };
    }

private:
    color_type ct;
};
template<>
inline SbMatrix convertTo<SbMatrix, Base::Matrix4D>(const Base::Matrix4D& vec2)
{
    double dMtrx[16];
    vec2.getGLMatrix(dMtrx);
    return SbMatrix(
        dMtrx[0],
        dMtrx[1],
        dMtrx[2],
        dMtrx[3],  // clazy:exclude=rule-of-two-soft
        dMtrx[4],
        dMtrx[5],
        dMtrx[6],
        dMtrx[7],
        dMtrx[8],
        dMtrx[9],
        dMtrx[10],
        dMtrx[11],
        dMtrx[12],
        dMtrx[13],
        dMtrx[14],
        dMtrx[15]
    );
}
template<>
inline Base::Matrix4D convertTo<Base::Matrix4D, SbMatrix>(const SbMatrix& vec2)
{
    Base::Matrix4D mat;
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            mat[i][j] = vec2[j][i];
        }
    }
    return mat;
}

}  // namespace Base
