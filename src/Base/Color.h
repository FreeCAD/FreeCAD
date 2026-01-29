// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2005 Jürgen Riegel <juergen.riegel@web.de>              *
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

#ifdef __GNUC__
# include <cstdint>
#endif
#include <cmath>
#include <string>

#include <FCGlobal.h>

// NOLINTBEGIN(readability-magic-numbers)
namespace Base
{

template<class color_type>
struct color_traits
{
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
    static color_type makeColor(int red, int green, int blue, int alpha = 255)
    {
        return color_type {red, green, blue, alpha};
    }

private:
    color_type ct;
};

/** Color class
 */
class BaseExport Color
{
public:
    /**
     * Defines the color as (R,G,B,A) whereas all values are in the range [0,1].
     * \a A defines the alpha value.
     */
    constexpr explicit Color(float R = 0.0, float G = 0.0, float B = 0.0, float A = 1.0)
        : r(R)
        , g(G)
        , b(B)
        , a(A)
    {}

    /**
     * Does basically the same as the constructor above unless that (R,G,B,A) is
     * encoded as an unsigned int.
     */
    explicit Color(uint32_t rgba);

    /** Copy constructor. */
    Color(const Color& c) = default;
    Color(Color&&) = default;

    /** Returns true if both colors are equal. Therefore all components must be equal. */
    bool operator==(const Color& c) const;
    bool operator!=(const Color& c) const;
    /**
     * Defines the color as (R,G,B,A) whereas all values are in the range [0,1].
     * \a A defines the alpha value, 1 means fully opaque and 0 transparent.
     */
    void set(float R, float G, float B, float A = 1.0);
    float transparency() const;
    void setTransparency(float value);
    Color& operator=(const Color& c) = default;
    Color& operator=(Color&& c) = default;
    /**
     * Sets the color value as a 32 bit combined red/green/blue/alpha value.
     * Each component is 8 bit wide (i.e. from 0x00 to 0xff), and the red
     * value should be stored leftmost, like this: 0xRRGGBBAA.
     *
     * \sa getPackedValue().
     */
    Color& setPackedValue(uint32_t rgba);
    /**
     * Returns color as a 32 bit packed unsigned int in the form 0xRRGGBBAA.
     *
     *  \sa setPackedValue().
     */
    uint32_t getPackedValue() const;
    /**
     * Returns color as a 32 bit packed unsigned int in the form 0xRRGGBB.
     */
    uint32_t getPackedRGB() const;
    /**
     * Sets color as a 32 bit packed unsigned int in the form 0xRRGGBB.
     */
    void setPackedRGB(uint32_t);
    /**
     * Returns color as a 32 bit packed unsigned int in the form 0xAARRGGBB.
     */
    uint32_t getPackedARGB() const;
    /**
     * Sets color as a 32 bit packed unsigned int in the form 0xAARRGGBB.
     */
    void setPackedARGB(uint32_t);

    template<typename T>
    static uint32_t asPackedRGBA(const T& color)
    {
        color_traits<T> ct {color};
        return (ct.red() << 24) | (ct.green() << 16) | (ct.blue() << 8) | ct.alpha();
    }

    template<typename T>
    static T fromPackedRGBA(uint32_t color)
    {
        return color_traits<T>::makeColor(
            (color >> 24) & 0xff,
            (color >> 16) & 0xff,
            (color >> 8) & 0xff,
            (color & 0xff)
        );
    }

    template<typename T>
    static uint32_t asPackedRGB(const T& color)
    {
        color_traits<T> ct {color};
        return (ct.red() << 24) | (ct.green() << 16) | (ct.blue() << 8);
    }

    template<typename T>
    static T fromPackedRGB(uint32_t color)
    {
        return color_traits<T>::makeColor((color >> 24) & 0xff, (color >> 16) & 0xff, (color >> 8) & 0xff);
    }
    /**
     * creates FC Color from template type, e.g. Qt QColor
     */
    template<typename T>
    void setValue(const T& q)
    {
        color_traits<T> ct {q};
        set(ct.redF(), ct.greenF(), ct.blueF(), ct.alphaF());
    }
    /**
     * returns a template type e.g. Qt color equivalent to FC color
     *
     */
    template<typename T>
    inline T asValue() const
    {
        // clang-format off
        return color_traits<T>::makeColor(int(std::lround(r * 255.0F)),
                                          int(std::lround(g * 255.0F)),
                                          int(std::lround(b * 255.0F)),
                                          int(std::lround(a * 255.0F)));
        // clang-format on
    }
    /**
     * creates FC Color from template type, e.g. Qt QColor
     */
    template<typename T>
    static Color fromValue(const T& q)
    {
        color_traits<T> ct {q};
        return Color(ct.redF(), ct.greenF(), ct.blueF(), ct.alphaF());
    }
    /**
     * returns color as hex color "#RRGGBB"
     *
     */
    std::string asHexString() const;

    /**
     * gets color from hex color "#RRGGBB"
     *
     */
    bool fromHexString(const std::string& hex);

    /// color values, public accessible
    float r {}, g {}, b {}, a {};
};

// Specialization for Color
template<>
struct color_traits<Base::Color>
{
    using color_type = Base::Color;
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

}  // namespace Base
// NOLINTEND(readability-magic-numbers)
