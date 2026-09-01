// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2026 The FreeCAD Project Association AISBL
// SPDX-FileNotice: Part of the FreeCAD project.
/******************************************************************************
 *                                                                            *
 *   FreeCAD is free software: you can redistribute it and/or modify          *
 *   it under the terms of the GNU Lesser General Public License as           *
 *   published by the Free Software Foundation, either version 2.1            *
 *   of the License, or (at your option) any later version.                   *
 *                                                                            *
 *   FreeCAD is distributed in the hope that it will be useful,               *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty              *
 *   of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.                  *
 *   See the GNU Lesser General Public License for more details.              *
 *                                                                            *
 *   You should have received a copy of the GNU Lesser General Public         *
 *   License along with FreeCAD. If not, see https://www.gnu.org/licenses     *
 *                                                                            *
 ******************************************************************************/

#include "PropertySerialization.h"

#include <cstddef>
#include <cstdint>
#include <limits>
#include <vector>

#include <Base/Exception.h>
#include <Base/Stream.h>

namespace
{

int toStreamSize(std::size_t size)
{
    if (size > static_cast<std::size_t>(std::numeric_limits<uint32_t>::max())
        || size > static_cast<std::size_t>(std::numeric_limits<int>::max())) {
        throw Base::RuntimeError("String is too large to store in a property stream");
    }
    return static_cast<int>(size);
}

}  // namespace

void App::PropertySerialization::writeBinaryString(Base::OutputStream& stream,
                                                   const std::string& value)
{
    const auto streamSize = toStreamSize(value.size());
    const auto size = static_cast<uint32_t>(value.size());
    stream << size;
    stream.write(value.c_str(), streamSize);
}

void App::PropertySerialization::readBinaryString(Base::InputStream& stream,
                                                  std::string& value)
{
    uint32_t size {};
    stream >> size;
    if (size == 0) {
        value.clear();
        return;
    }

    const auto streamSize = toStreamSize(size);
    std::vector<char> buffer(static_cast<std::size_t>(size));
    stream.read(buffer.data(), streamSize);
    value.assign(buffer.data(), buffer.size());
}
