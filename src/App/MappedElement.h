// SPDX-License-Identifier: LGPL-2.1-or-later

/****************************************************************************
 *   Copyright (c) 2022 Zheng, Lei (realthunder) <realthunder.dev@gmail.com>*
 *   Copyright (c) 2023 FreeCAD Project Association                         *
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

#ifndef APP_MAPPEDELEMENT_H
#define APP_MAPPEDELEMENT_H

#include <cstring>
#include <memory>
#include <cctype>
#include <boost/algorithm/string/predicate.hpp>
#include <QByteArray>
#include <QHash>
#include "ComplexGeoData.h"
#include "IndexedName.h"
#include "MappedName.h"

namespace App
{
class DocumentObject;
}

namespace Data
{

struct AppExport MappedElement
{
    IndexedName index;
    MappedName name;

    MappedElement() = default;

    MappedElement(const IndexedName & idx, const MappedName & n)
        : index(idx), name(n)
    {}

    MappedElement(const MappedName & n, const IndexedName & idx)
        : index(idx), name(n)
    {}

    MappedElement(const MappedElement & other)
        : index(other.index), name(other.name)
    {}

    MappedElement(MappedElement && other)
        : index(std::move(other.index)), name(std::move(other.name))
    {}

    MappedElement & operator=(MappedElement && other)
    {
        this->index = std::move(other.index);
        this->name = std::move(other.name);
        return *this;
    }

    MappedElement & operator=(const MappedElement & other)
    {
        this->index = other.index;
        this->name = other.name;
        return *this;
    }

    bool operator==(const MappedElement &other) const
    {
        return this->index == other.index && this->name == other.name;
    }

    bool operator!=(const MappedElement &other) const
    {
        return this->index != other.index || this->name != other.name;
    }

    bool operator<(const MappedElement &other) const
    {
        int res = this->index.compare(other.index);
        if (res < 0)
            return true;
        if (res > 0)
            return false;
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

struct AppExport ElementNameComp {
    /** Comparison function to make topo name more stable
     *
     * The sorting decomposes the name into either of the following two forms
     *      '#' + hex_digits + tail
     *      non_digits + digits + tail
     *
     * The non-digits part is compared lexically, while the digits part is
     * compared by its integer value.
     *
     * The reason for this is to prevent names with a larger index (which usually means that it
     * comes later in the history) being sorted to before something with the same name but a larger
     * index.
     */
    bool operator()(const MappedName &a, const MappedName &b) const;
};

typedef QVector<App::StringIDRef> ElementIDRefs;

struct AppExport MappedChildElements
{
    IndexedName indexedName;
    int count;
    int offset;
    long tag;
    ElementMapPtr elementMap;
    QByteArray postfix;
    ElementIDRefs sids;

    static const std::string & prefix();
};

} //namespace Data


#endif// FREECAD_MAPPEDELEMENT_H
