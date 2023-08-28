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


#ifndef APP_COLOR_H
#define APP_COLOR_H

#ifdef __GNUC__
# include <cstdint>
#endif
#include <string>

#include <FCGlobal.h>

namespace App
{

/** Color class
 */
class AppExport Color
{
public:
    /**
     * Defines the color as (R,G,B,A) whereas all values are in the range [0,1].
     * \a A defines the transparency.
     */
    explicit Color(float R=0.0,float G=0.0, float B=0.0, float A=0.0);

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
     * \a A defines the transparency, 0 means complete opaque and 1 invisible.
     */
    void set(float R,float G, float B, float A=0.0);
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
     * Returns color as a 32 bit packed unsigned int in the form 0xAARRGGBB.
     */
    uint32_t getPackedARGB() const;
    /**
     * Sets color as a 32 bit packed unsigned int in the form 0xAARRGGBB.
     */
    void setPackedARGB(uint32_t);

    template <typename T>
    static uint32_t asPackedRGBA(const T& color) {
        return (color.red() << 24) | (color.green() << 16) | (color.blue() << 8) | color.alpha();
    }

    template <typename T>
    static T fromPackedRGBA(uint32_t color) {
        return T((color >> 24) & 0xff, (color >> 16) & 0xff, (color >> 8) & 0xff, color & 0xff);
    }

    template <typename T>
    static uint32_t asPackedRGB(const T& color) {
        return (color.red() << 24) | (color.green() << 16) | (color.blue() << 8);
    }

    template <typename T>
    static T fromPackedRGB(uint32_t color) {
        return T((color >> 24) & 0xff, (color >> 16) & 0xff, (color >> 8) & 0xff);
    }
    /**
     * creates FC Color from template type, e.g. Qt QColor
     */
    template <typename T>
    void setValue(const T& q) {
        set(q.redF(),q.greenF(),q.blueF());
    }
    /**
     * returns a template type e.g. Qt color equivalent to FC color
     *
     */
    template <typename T>
    inline T asValue() const {
        return(T(int(r*255.0f),int(g*255.0f),int(b*255.0f)));
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
    float r,g,b,a;
};

} //namespace App

#endif // APP_COLOR_H
