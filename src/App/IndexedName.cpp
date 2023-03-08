// SPDX-License-Identifier: LGPL-2.1-or-later

/****************************************************************************
 *   Copyright (c) 2022 Zheng, Lei (realthunder) <realthunder.dev@gmail.com>*
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

#include "PreCompiled.h"

#ifndef _PreComp_
# include <cstdlib>
# include <unordered_set>
#endif

#include <QHash>

#include "IndexedName.h"

using namespace Data;

struct ByteArray
{
    ByteArray(const QByteArray& b)
        :bytes(b)
    {}

    ByteArray(const ByteArray& other)
        :bytes(other.bytes)
    {}

    ByteArray(ByteArray&& other)
        :bytes(std::move(other.bytes))
    {}

    void mutate() const
    {
        QByteArray copy;
        copy.append(bytes.constData(), bytes.size());
        bytes = copy;
    }

    bool operator==(const ByteArray& other) const {
        return bytes == other.bytes;
    }

    mutable QByteArray bytes;
};

struct ByteArrayHasher
{
    std::size_t operator()(const ByteArray& bytes) const
    {
        return qHash(bytes.bytes);
    }

    std::size_t operator()(const QByteArray& bytes) const
    {
        return qHash(bytes);
    }
};

void IndexedName::set(
    const char* name,
    int len,
    const std::vector<const char*>& types,
    bool allowOthers)
{
    static std::unordered_set<ByteArray, ByteArrayHasher> NameSet;

    if (len < 0)
        len = static_cast<int>(std::strlen(name));
    int i;
    for (i = len - 1; i >= 0; --i) {
        if (name[i] < '0' || name[i]>'9')
            break;
    }
    ++i;
    this->index = std::atoi(name + i);

    for (int j = 0; j < i; ++j) {
        if (name[j] == '_'
            || (name[j] >= 'a' && name[j] <= 'z')
            || (name[j] >= 'A' && name[j] <= 'Z'))
            continue;
        this->type = "";
        return;
    }

    for (const char* type : types) {
        int j = 0;
        for (const char* n = name, *t = type; *n; ++n) {
            if (*n != *t || j >= i)
                break;
            ++j;
            ++t;
            if (!*t) {
                this->type = type;
                return;
            }
        }
    }

    if (allowOthers) {
        auto res = NameSet.insert(QByteArray::fromRawData(name, i));
        if (res.second)
            res.first->mutate();
        this->type = res.first->bytes.constData();
    }
    else
        this->type = "";
}
