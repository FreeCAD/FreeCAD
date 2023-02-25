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


#include "PreCompiled.h"

#ifndef _PreComp_
#include <iomanip>
#include <sstream>
#endif

#include "Color.h"

using namespace App;


Color::Color(float R, float G, float B, float A)
  : r(R)
  , g(G)
  , b(B)
  , a(A)
{
}

Color::Color(uint32_t rgba)
{
    setPackedValue( rgba );
}

bool Color::operator==(const Color& c) const
{
    return getPackedValue() == c.getPackedValue();
}

bool Color::operator!=(const Color& c) const
{
    return !operator==(c);
}

void Color::set(float R, float G, float B, float A)
{
    r = R;
    g = G;
    b = B;
    a = A;
}

Color& Color::setPackedValue(uint32_t rgba)
{
    this->set((rgba >> 24)/255.0f,
             ((rgba >> 16)&0xff)/255.0f,
             ((rgba >> 8)&0xff)/255.0f,
             (rgba&0xff)/255.0f);
    return *this;
}

uint32_t Color::getPackedValue() const
{
    return (static_cast<uint32_t>(r*255.0f + 0.5f) << 24 |
            static_cast<uint32_t>(g*255.0f + 0.5f) << 16 |
            static_cast<uint32_t>(b*255.0f + 0.5f) << 8  |
            static_cast<uint32_t>(a*255.0f + 0.5f));
}

uint32_t Color::getPackedARGB() const
{
    return (static_cast<uint32_t>(a*255.0f + 0.5f) << 24 |
            static_cast<uint32_t>(r*255.0f + 0.5f) << 16 |
            static_cast<uint32_t>(g*255.0f + 0.5f) << 8  |
            static_cast<uint32_t>(b*255.0f + 0.5f));
}

void Color::setPackedARGB(uint32_t argb)
{
    // clang-format off
    this->set(((argb >> 16) & 0xff) / 255.0f,
              ((argb >>  8) & 0xff) / 255.0f,
               (argb        & 0xff) / 255.0f,
               (argb >> 24)         / 255.0f);
    // clang-format on
}

std::string Color::asHexString() const
{
    std::stringstream ss;
    ss << "#" << std::hex << std::uppercase << std::setfill('0')
       << std::setw(2) << int(r*255.0f)
       << std::setw(2) << int(g*255.0f)
       << std::setw(2) << int(b*255.0f);
    return ss.str();
}

bool Color::fromHexString(const std::string& hex)
{
    if (hex.size() < 7 || hex[0] != '#')
        return false;
    // #RRGGBB
    if (hex.size() == 7) {
        std::stringstream ss(hex);
        unsigned int rgb;
        char c;

        ss >> c >> std::hex >> rgb;
        int rc = (rgb >> 16) & 0xff;
        int gc = (rgb >> 8) & 0xff;
        int bc = rgb & 0xff;

        r = rc / 255.0f;
        g = gc / 255.0f;
        b = bc / 255.0f;

        return true;
    }
    // #RRGGBBAA
    if (hex.size() == 9) {
        std::stringstream ss(hex);
        unsigned int rgba;
        char c;

        ss >> c >> std::hex >> rgba;
        int rc = (rgba >> 24) & 0xff;
        int gc = (rgba >> 16) & 0xff;
        int bc = (rgba >> 8) & 0xff;
        int ac = rgba & 0xff;

        r = rc / 255.0f;
        g = gc / 255.0f;
        b = bc / 255.0f;
        a = ac / 255.0f;

        return true;
    }

    return false;
}
