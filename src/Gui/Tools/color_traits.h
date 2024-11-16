// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2024 Werner Mayer <wmayer[at]users.sourceforge.net>     *
 *                                                                         *
 *   This file is part of FreeCAD.                                         *
 *                                                                         *
 *   FreeCAD is free software: you can redistribute it and/or modify it    *
 *   under the terms of the GNU Lesser General Public License as           *
 *   published by the Free Software Foundation, either version 2.1 of the  *
 *   License, or (at your option) any later version.                       *
 *                                                                         *
 *   FreeCAD is distributed in the hope that it will be useful, but        *
 *   WITHOUT ANY WARRANTY; without even the implied warranty of            *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU      *
 *   Lesser General Public License for more details.                       *
 *                                                                         *
 *   You should have received a copy of the GNU Lesser General Public      *
 *   License along with FreeCAD. If not, see                               *
 *   <https://www.gnu.org/licenses/>.                                      *
 *                                                                         *
 **************************************************************************/

#ifndef GUI_COLORTRAITS_H
#define GUI_COLORTRAITS_H

#include <App/Color.h>
#include <Inventor/SbColor.h>
#include <Inventor/SbColor4f.h>
#include <QColor>

namespace App
{
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
        return color_type{static_cast<float>(red) / 255.0F,
                          static_cast<float>(green) / 255.0F,
                          static_cast<float>(blue) / 255.0F};
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
        return color_type{static_cast<float>(red) / 255.0F,
                          static_cast<float>(green) / 255.0F,
                          static_cast<float>(blue) / 255.0F,
                          static_cast<float>(alpha) / 255.0F};
    }

private:
    color_type ct;
};

// Specialization for Color
template<>
struct color_traits<App::Color>
{
    using color_type = App::Color;
    color_traits() = default;
    explicit color_traits(const color_type& ct)
        : ct(ct)
    {}
    float redF() const
    {
        return ct.r;
    }
    float greenF() const
    {
        return ct.g;
    }
    float blueF() const
    {
        return ct.b;
    }
    float alphaF() const
    {
        return ct.a;
    }
    void setRedF(float red)
    {
        ct.r = red;
    }
    void setGreenF(float green)
    {
        ct.g = green;
    }
    void setBlueF(float blue)
    {
        ct.b = blue;
    }
    void setAlphaF(float alpha)
    {
        ct.a = alpha;
    }
    int red() const
    {
        return int(std::lround(ct.r * 255.0F));
    }
    int green() const
    {
        return int(std::lround(ct.g * 255.0F));
    }
    int blue() const
    {
        return int(std::lround(ct.b * 255.0F));
    }
    int alpha() const
    {
        return int(std::lround(ct.a * 255.0F));
    }
    void setRed(int red)
    {
        ct.r = static_cast<float>(red) / 255.0F;
    }
    void setGreen(int green)
    {
        ct.g = static_cast<float>(green) / 255.0F;
    }
    void setBlue(int blue)
    {
        ct.b = static_cast<float>(blue) / 255.0F;
    }
    void setAlpha(int alpha)
    {
        ct.a = static_cast<float>(alpha) / 255.0F;
    }
    static color_type makeColor(int red, int green, int blue, int alpha = 255)
    {
        return color_type{static_cast<float>(red) / 255.0F,
                          static_cast<float>(green) / 255.0F,
                          static_cast<float>(blue) / 255.0F,
                          static_cast<float>(alpha) / 255.0F};
    }

private:
    color_type ct;
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
        return color_type{red, green, blue, alpha};
    }

private:
    color_type ct;
};

}  // namespace App

#endif  // GUI_COLORTRAITS_H
