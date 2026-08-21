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

#pragma once

#include <string>

namespace Base
{
class InputStream;
class OutputStream;
}  // namespace Base

namespace App::PropertySerialization
{

/**
 * @brief Write a length-prefixed string to a property side-file stream.
 *
 * The payload is written as a 32-bit byte count followed by the raw string
 * bytes. This helper is intended for FreeCAD property side files, not XML
 * attribute encoding.
 */
void writeBinaryString(Base::OutputStream& stream, const std::string& value);

/**
 * @brief Read a length-prefixed string from a property side-file stream.
 *
 * The format matches `writeBinaryString()`: a 32-bit byte count followed by raw
 * string bytes.
 */
void readBinaryString(Base::InputStream& stream, std::string& value);

}  // namespace App::PropertySerialization
