// SPDX-License-Identifier: LGPL-2.1-or-later
/****************************************************************************
 *                                                                          *
 *   Copyright (c) 2025 Pieter Hijma <info@pieterhijma.net>                 *
 *                                                                          *
 *   This file is part of FreeCAD.                                          *
 *                                                                          *
 *   FreeCAD is free software: you can redistribute it and/or modify it     *
 *   under the terms of the GNU Lesser General Public License as            *
 *   published by the Free Software Foundation, either version 2.1 of the   *
 *   License, or (at your option) any later version.                        *
 *                                                                          *
 *   FreeCAD is distributed in the hope that it will be useful, but         *
 *   WITHOUT ANY WARRANTY; without even the implied warranty of             *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU       *
 *   Lesser General Public License for more details.                        *
 *                                                                          *
 *   You should have received a copy of the GNU Lesser General Public       *
 *   License along with FreeCAD. If not, see                                *
 *   <https://www.gnu.org/licenses/>.                                       *
 *                                                                          *
 ***************************************************************************/

#pragma once

#include <FCGlobal.h>
#include <string>

namespace App
{

class DocumentObject;

/**
 * @brief Represents a dependency edge between two objects/properties.
 *
 * This class is used for fine-grained recompute and represents a dependency
 * edge from a property in one document object to another property in a
 * document object.  The dependency may also be on the object as a whole.  In
 * that case, @p fromProp or @p toProp are an empty string that represents
 * @c fromObj.HEAD or @c toObj.HEAD respectively.
 */
class AppExport DepEdge
{
public:
    /**
     * @brief Constructs a dependency edge.
     *
     * @param fromObj The source DocumentObject.
     * @param fromProp Name of the source property.  If empty, indicates the
     * dependency is on the object as a whole.
     * @param toObj The target DocumentObject.
     * @param toProp Name of the target property.  If empty, indicates the
     * dependency is on the object as a whole.
     */
    DepEdge(DocumentObject* fromObj,
            std::string fromProp,
            DocumentObject* toObj,
            std::string toProp)
        : fromObj(fromObj)
        , fromProp(std::move(fromProp))
        , toObj(toObj)
        , toProp(std::move(toProp))
    {}

    DepEdge(const DepEdge&) = default;
    DepEdge(DepEdge&&) noexcept = default;
    DepEdge& operator=(const DepEdge&) = default;
    DepEdge& operator=(DepEdge&&) noexcept = default;
    ~DepEdge() = default;

    /**
     * @brief Less-than operator for comparing two DepEdge objects.
     *
     * This operator allows for ordering DepEdge objects based on their
     * fromObj, fromProp, toObj, and toProp members.  The ordering is based on
     * what the ordering would be if DepEdge were a tuple.
     *
     * @param other The other DepEdge object to compare against.
     *
     * @return true if this DepEdge is less than the other, false otherwise.
     */
    bool operator<(const DepEdge& other) const noexcept
    {
        return std::tie(fromObj, fromProp, toObj, toProp) <
               std::tie(other.fromObj, other.fromProp, other.toObj, other.toProp);
    }

    /// Check if two DepEdge objects are equal.
    bool operator==(const DepEdge& other) const noexcept
    {
        return fromObj == other.fromObj
            && fromProp == other.fromProp
            && toObj   == other.toObj
            && toProp  == other.toProp;
    }

    /// The source DocumentObject.
    DocumentObject* fromObj;
    /// Name of the source property.
    std::string fromProp;
    /// The target DocumentObject.
    DocumentObject* toObj;
    /// Name of the target property.
    std::string toProp;
};

// Tuple-like accessors for DepEdge
template <std::size_t I>
decltype(auto) get(DepEdge& e) {
    if constexpr (I == 0) {return (e.fromObj);}
    else if constexpr (I == 1) {return (e.fromProp);}
    else if constexpr (I == 2) {return (e.toObj);}
    else /* I == 3 */ {return (e.toProp);}
}

template <std::size_t I>
decltype(auto) get(DepEdge const& e) {
    if constexpr (I == 0) {return (e.fromObj);}
    else if constexpr (I == 1) {return (e.fromProp);}
    else if constexpr (I == 2) {return (e.toObj);}
    else /* I == 3 */ {return (e.toProp);}
}

template <std::size_t I>
decltype(auto) get(DepEdge&& e) {
    if constexpr (I == 0) {return std::move(e.fromObj);}
    else if constexpr (I == 1) {return std::move(e.fromProp);}
    else if constexpr (I == 2) {return std::move(e.toObj);}
    else /* I == 3 */ {return std::move(e.toProp);}
}

}  // namespace App

namespace std
{
// Specializations of tuple_size and tuple_element for App::DepEdge for tuple-like access.
template<> struct tuple_size<App::DepEdge> : integral_constant<std::size_t, 4> {};

template<> struct tuple_element<0, App::DepEdge> { using type = App::DocumentObject*; };
template<> struct tuple_element<1, App::DepEdge> { using type = std::string; };
template<> struct tuple_element<2, App::DepEdge> { using type = App::DocumentObject*; };
template<> struct tuple_element<3, App::DepEdge> { using type = std::string; };
}
