// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2022 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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

#include <type_traits>
#include <utility>

/*!
 Using enum classes as type-safe bitmasks.
 @code
 enum class Color {
   Red   = 1 << 0,
   Green = 1 << 1,
   Blue  = 1 << 2
 };
 ENABLE_BITMASK_OPERATORS(Color)

 Color yellow = Color::Red | Color::Green;

 Flags<Color> flags(yellow);
 flags.testFlag(Color::Red);
 flags.testFlag(Color::Green);
 @endcode
*/

// NOLINTBEGIN
// clang-format off
// Based on https://stackoverflow.com/questions/1448396/how-to-use-enums-as-flags-in-c
template<class T = void> struct enum_traits {};

template<> struct enum_traits<void> {
    struct _allow_bitops {
        static constexpr bool allow_bitops = true;
    };
    using allow_bitops = _allow_bitops;

    template<class T, class R = T>
    using t = typename std::enable_if<std::is_enum<T>::value &&
        enum_traits<T>::allow_bitops, R>::type;
};

template<class T>
constexpr enum_traits<>::t<T> operator~(T a) {
    return static_cast<T>(~std::to_underlying(a));
}
template<class T>
constexpr enum_traits<>::t<T> operator|(T a, T b) {
    return static_cast<T>(std::to_underlying(a) | std::to_underlying(b));
}
template<class T>
constexpr enum_traits<>::t<T> operator&(T a, T b) {
    return static_cast<T>(std::to_underlying(a) & std::to_underlying(b));
}
template<class T>
constexpr enum_traits<>::t<T> operator^(T a, T b) {
    return static_cast<T>(std::to_underlying(a) ^ std::to_underlying(b));
}
template<class T>
constexpr enum_traits<>::t<T, T&> operator|=(T& a, T b) {
    a = a | b;
    return a;
}
template<class T>
constexpr enum_traits<>::t<T, T&> operator&=(T& a, T b) {
    a = a & b;
    return a;
}
template<class T>
constexpr enum_traits<>::t<T, T&> operator^=(T& a, T b) {
    a = a ^ b;
    return a;
}

#define ENABLE_BITMASK_OPERATORS(x)  \
template<>                           \
struct enum_traits<x> :              \
       enum_traits<>::allow_bitops {};

namespace Base {
template <typename Enum>
class Flags {
    static_assert(std::is_enum<Enum>::value, "Flags is only usable on enumeration types.");
    Enum i;

public:
    // Linter seems wrong on next line, don't want explicit here forcing downstream changes
    constexpr inline Flags(Enum f = Enum()) : i(f) {}   // NOLINT (runtime/explicit)
    constexpr bool testFlag(Enum f) const {
        return (i & f) == f && (std::to_underlying(f) != 0 || i == f);
    }
    constexpr inline void setFlag(Enum f, bool on = true) {
        on ? (i |= f) : (i &= ~f);
    }
    constexpr bool isEqual(Flags f) const {
        return std::to_underlying(i) == std::to_underlying(f.i);
    }
    constexpr Enum getFlags() const {
        return i;
    }
    constexpr Flags<Enum> &operator|=(const Flags<Enum> &other) {
        i |= other.i;
        return *this;
    }
    constexpr Flags<Enum> &operator|=(const Enum &f) {
        i |= f;
        return *this;
    }
    constexpr Flags<Enum> operator|(const Flags<Enum> &other) const {
        return i | other.i;
    }
    constexpr Flags<Enum> operator|(const Enum &f) const {
        return i | f;
    }
    constexpr Flags<Enum> &operator&=(const Flags<Enum> &other) {
        i &= other.i;
        return *this;
    }
    constexpr Flags<Enum> &operator&=(const Enum &f) {
        i &= f;
        return *this;
    }
    constexpr Flags<Enum> operator&(const Flags<Enum> &other) const {
        return i & other.i;
    }
    constexpr Flags<Enum> operator&(const Enum &f) const {
        return i & f;
    }
    constexpr Flags<Enum> operator~() const {
        return ~i;
    }

    constexpr bool operator!() const {
        return !i;
    }

    explicit operator bool() const {
        return toUnderlyingType() != 0;
    }
    std::underlying_type_t<Enum> toUnderlyingType() const {
        return std::to_underlying(i);
    }
};
}
// clang-format on
// NOLINTEND
