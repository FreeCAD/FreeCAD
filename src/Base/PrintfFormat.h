// SPDX-License-Identifier: LGPL-2.1-or-later
/***************************************************************************
 *   Copyright (c) 2026 FreeCAD Project Association                        *
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
 ***************************************************************************/

#ifndef BASE_PRINTFFORMAT_H
#define BASE_PRINTFFORMAT_H

#include <format>
#include <type_traits>
#include <utility>

#include <FCGlobal.h>

namespace Base
{

/// Translates a printf-style format string into a std::format one.
/// Supports the conversions used throughout FreeCAD (%s %c %d %i %u %o %x %X
/// %e %E %f %F %g %G %a %A %p %%) with flags, width and precision; length
/// modifiers are accepted and ignored because std::format derives the size
/// from the argument type.  Throws std::format_error for anything it cannot
/// map exactly (e.g. runtime '*' widths or '%n').
BaseExport std::string printfToFormatString(const char* fmt);

namespace detail
{
/// std::format only formats void pointers; printf formats any object
/// pointer, so decay them the same way fmt::sprintf used to.
template<typename T>
constexpr decltype(auto) printfArg(T&& value)
{
    using Plain = std::remove_cvref_t<T>;
    if constexpr (std::is_pointer_v<Plain> && !std::is_convertible_v<Plain, const char*>) {
        return static_cast<const void*>(value);
    }
    else {
        return std::forward<T>(value);
    }
}
}  // namespace detail

/// printf-style formatting on top of std::format: the format string is
/// translated at runtime, the arguments keep their static types.  Used by
/// the Console API, whose call sites are printf-style.  Throws
/// std::format_error on malformed format strings or argument mismatch.
template<typename... Args>
std::string sprintf(const char* fmt, const Args&... args)
{
    const std::string spec = printfToFormatString(fmt);
    // the tuple keeps the converted pointer values alive as named lvalues,
    // which std::make_format_args requires
    return std::apply(
        [&spec](const auto&... unpacked) {
            return std::vformat(spec, std::make_format_args(unpacked...));
        },
        std::forward_as_tuple(detail::printfArg(args)...)
    );
}

}  // namespace Base

#endif  // BASE_PRINTFFORMAT_H
