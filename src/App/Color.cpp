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


// NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
Color::Color(float red, float green, float blue, float alpha)
  : r(red)
  , g(green)
  , b(blue)
  , a(alpha)
{
}

Color::Color(uint32_t rgba)
  : Color{}
{
    setPackedValue(rgba);
}

bool Color::operator==(const Color& color) const
{
    return getPackedValue() == color.getPackedValue();
}

bool Color::operator!=(const Color& color) const
{
    return !operator==(color);
}

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
void Color::set(float red, float green, float blue, float alpha)
{
    r = red;
    g = green;
    b = blue;
    a = alpha;
}

// NOLINTBEGIN(cppcoreguidelines-avoid-magic-numbers,readability-magic-numbers)
Color& Color::setPackedValue(uint32_t rgba)
{
    // clang-format off
    this->set(static_cast<float> (rgba >> 24)         / 255.0F,
              static_cast<float>((rgba >> 16) & 0xff) / 255.0F,
              static_cast<float>((rgba >>  8) & 0xff) / 255.0F,
              static_cast<float> (rgba        & 0xff) / 255.0F);
    return *this;
    // clang-format on
}

uint32_t Color::getPackedValue() const
{
    // clang-format off
    return (static_cast<uint32_t>(r * 255.0F + 0.5F) << 24 |
            static_cast<uint32_t>(g * 255.0F + 0.5F) << 16 |
            static_cast<uint32_t>(b * 255.0F + 0.5F) << 8  |
            static_cast<uint32_t>(a * 255.0F + 0.5F));
    // clang-format on
}

uint32_t Color::getPackedARGB() const
{
    // clang-format off
    return (static_cast<uint32_t>(a * 255.0F + 0.5F) << 24 |
            static_cast<uint32_t>(r * 255.0F + 0.5F) << 16 |
            static_cast<uint32_t>(g * 255.0F + 0.5F) << 8  |
            static_cast<uint32_t>(b * 255.0F + 0.5F));
    // clang-format on
}

void Color::setPackedARGB(uint32_t argb)
{
    // clang-format off
    this->set(static_cast<float>((argb >> 16) & 0xff) / 255.0F,
              static_cast<float>((argb >>  8) & 0xff) / 255.0F,
              static_cast<float> (argb        & 0xff) / 255.0F,
              static_cast<float> (argb >> 24)         / 255.0F);
    // clang-format on
}

std::string Color::asHexString() const
{
    std::stringstream ss;
    ss << "#" << std::hex << std::uppercase << std::setfill('0')
       << std::setw(2) << int(r * 255.0F)
       << std::setw(2) << int(g * 255.0F)
       << std::setw(2) << int(b * 255.0F);
    return ss.str();
}

bool Color::fromHexString(const std::string& hex)
{
    if (hex.size() < 7 || hex[0] != '#') {
        return false;
    }

    // #RRGGBB
    if (hex.size() == 7) {
        std::stringstream ss(hex);
        unsigned int rgb;
        char ch{};

        ss >> ch >> std::hex >> rgb;
        int rc = (rgb >> 16) & 0xff;
        int gc = (rgb >> 8) & 0xff;
        int bc = rgb & 0xff;

        r = rc / 255.0F;
        g = gc / 255.0F;
        b = bc / 255.0F;

        return true;
    }
    // #RRGGBBAA
    if (hex.size() == 9) {
        std::stringstream ss(hex);
        unsigned int rgba;
        char ch{};

        ss >> ch >> std::hex >> rgba;
        int rc = (rgba >> 24) & 0xff;
        int gc = (rgba >> 16) & 0xff;
        int bc = (rgba >> 8) & 0xff;
        int ac =  rgba & 0xff;

        r = rc / 255.0F;
        g = gc / 255.0F;
        b = bc / 255.0F;
        a = ac / 255.0F;

        return true;
    }

    return false;
}
// NOLINTEND(cppcoreguidelines-avoid-magic-numbers,readability-magic-numbers)
