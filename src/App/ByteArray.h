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

#pragma once

#include <cassert>
#include <cstring>
#include <ostream>
#include <string>
#include <vector>

#include <QByteArray>
#include <QHash>

#include "FCGlobal.h"

namespace Data
{

/// A thin wrapper around a QByteArray providing the ability to force a copy of the data at any
/// time, even if it isn't being written to. The standard assignment operator for this class *does*
/// make a copy of the data, unlike the standard assignment operator for QByteArray.
struct ByteArray
{
    explicit ByteArray(QByteArray other)
        : bytes(std::move(other))
    {}

    ByteArray(const ByteArray& other) = default;

    ByteArray(ByteArray&& other) noexcept
        : bytes(std::move(other.bytes))
    {}

    ~ByteArray() = default;

    /// Guarantee that the stored QByteArray does not share its memory with another instance.
    void ensureUnshared() const
    {
        QByteArray copy;
        copy.append(bytes.constData(), bytes.size());
        bytes = copy;
    }

    bool operator==(const ByteArray& other) const
    {
        return bytes == other.bytes;
    }

    ByteArray& operator=(const ByteArray& other)
    {
        bytes.clear();
        bytes.append(other.bytes.constData(), other.bytes.size());
        return *this;
    }

    ByteArray& operator=(ByteArray&& other) noexcept
    {
        bytes = std::move(other.bytes);
        return *this;
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

}  // namespace Data
