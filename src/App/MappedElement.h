// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************************************
 *                                                                                                 *
 *   Copyright (c) 2022 Zheng, Lei (realthunder) <realthunder.dev@gmail.com>                       *
 *   Copyright (c) 2023 FreeCAD Project Association                                                *
 *                                                                                                 *
 *   This file is part of FreeCAD.                                                                 *
 *                                                                                                 *
 *   FreeCAD is free software: you can redistribute it and/or modify it under the terms of the     *
 *   GNU Lesser General Public License as published by the Free Software Foundation, either        *
 *   version 2.1 of the License, or (at your option) any later version.                            *
 *                                                                                                 *
 *   FreeCAD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;          *
 *   without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.     *
 *   See the GNU Lesser General Public License for more details.                                   *
 *                                                                                                 *
 *   You should have received a copy of the GNU Lesser General Public License along with           *
 *   FreeCAD. If not, see <https://www.gnu.org/licenses/>.                                         *
 *                                                                                                 *
 **************************************************************************************************/

#ifndef APP_MAPPED_ELEMENT_H
#define APP_MAPPED_ELEMENT_H

#include "IndexedName.h"
#include "MappedName.h"


namespace App
{
class DocumentObject;
}

namespace Data
{

/// A MappedElement combines a MappedName and and IndexedName into a single entity and provides
/// simple comparison operators for the combination (including operator< so that the entity can
/// be sorted, or used in sorted containers).
struct AppExport MappedElement
{
    IndexedName index;
    MappedName name;

    MappedElement() = default;

    MappedElement(const IndexedName& idx, MappedName n)
        : index(idx),
          name(std::move(n))
    {}

    MappedElement(MappedName n, const IndexedName& idx)
        : index(idx),
          name(std::move(n))
    {}

    ~MappedElement() = default;

    MappedElement(const MappedElement& other) = default;

    MappedElement(MappedElement&& other) noexcept
        : index(other.index),
          name(std::move(other.name))
    {}

    MappedElement& operator=(MappedElement&& other) noexcept
    {
        this->index = other.index;
        this->name = std::move(other.name);
        return *this;
    }

    MappedElement& operator=(const MappedElement& other) = default;

    bool operator==(const MappedElement& other) const
    {
        return this->index == other.index && this->name == other.name;
    }

    bool operator!=(const MappedElement& other) const
    {
        return this->index != other.index || this->name != other.name;
    }

    /// For sorting purposes, one MappedElement is considered "less" than another if its index
    /// compares less (which is first alphabetical, and then by numeric index). If the index of this
    /// MappedElement is the same, then the names are compared lexicographically.
    bool operator<(const MappedElement& other) const
    {
        int res = this->index.compare(other.index);
        if (res < 0) {
            return true;
        }
        if (res > 0) {
            return false;
        }
        return this->name < other.name;
    }
};

struct AppExport HistoryItem {
    App::DocumentObject *obj;
    long tag;
    Data::MappedName element;
    Data::IndexedName index;
    std::vector<Data::MappedName> intermediates;
    HistoryItem(App::DocumentObject *obj, const Data::MappedName &name);
};

struct AppExport ElementNameComparator {
    /** Comparison function to make topo name more stable
     *
     * The sorting decomposes the name into either of the following two forms
     *      '#' + hex_digits + tail
     *      non_digits + digits + tail
     *
     * The non-digits part is compared lexically, while the digits part is
     * compared by its integer value.
     *
     * The reason for this is to prevent names with bigger digits (which usually means
     * they come later in history) from coming earlier when sorting.
     */
    bool operator()(const MappedName & leftName, const MappedName & rightName) const;
};

}// namespace Data


#endif// APP_MAPPED_ELEMENT_H
