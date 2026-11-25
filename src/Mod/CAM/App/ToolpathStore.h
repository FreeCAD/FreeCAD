// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2026 sliptonic <shopinthewoods@gmail.com>
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
#include <string_view>
#include <vector>

#include <Mod/CAM/PathGlobal.h>

#include "Command.h"

namespace Path::ToolpathStore
{

/** Binary form of a Command list, as stored in a FreeCAD document.
 *
 * Layout (all integers little-endian; "varint" is an unsigned LEB128 value):
 *
 *   magic            4 bytes "FCTP"
 *   version          1 byte (FormatVersion)
 *   command count    varint
 *   string table     varint n, then n x (varint length, bytes)
 *   shape table      varint n, then n x shape
 *   shape ids        one varint for each command
 *   parameter columns   varint n, then n x (varint key string id, column)
 *   annotation columns  varint n, then n x (varint key string id, column)
 *   tool axis        1 byte: 0 = no command has a Tool Axis, 1 = three columns follow
 *                    (X, Y, Z), each with one value for each command that has one
 *   string annotations  varint n, then n x (varint command index delta,
 *                                           varint key string id, varint value string id)
 *
 * A shape is the set of fields that a command has: its name, its parameter
 * letters, the keys of its numeric annotations, and if it has a Tool Axis. Commands with the same
 * shape share one shape record:
 *
 *   shape            varint name string id, 1 byte flags (bit 0: has a Tool Axis),
 *                    varint n + n x varint parameter key string id,
 *                    varint n + n x varint annotation key string id
 *
 * A column holds all values of one parameter letter (or one numeric annotation
 * key) in command order:
 *
 *   column           varint value count, 1 byte encoding, payload
 *     encoding 0     each value rounded to Resolution, then the difference from
 *                    the value before it, as a zigzag varint
 *     encoding 1     raw IEEE 754 double, 8 bytes each. Used when a value is
 *                    not finite or is too large for encoding 0.
 */

/// File name extension of a stored binary Command list.
PathExport extern const char* const FileExtension;

/// Format version that encode() writes.
constexpr unsigned char FormatVersion = 1;

/// Size of one unit of a rounded value (mm, degrees, mm/min ...).
constexpr double Resolution = 1e-6;

/// Returns true if \a data starts with the magic bytes of the binary form.
PathExport bool isBinary(std::string_view data);

/// Returns the binary form of \a commands.
PathExport std::string encode(const std::vector<Command>& commands);

/** Replaces \a commands with the content of \a data.
 * Throws Base::BadFormatError if \a data is not a valid binary form.
 */
PathExport void decode(std::string_view data, std::vector<Command>& commands);

}  // namespace Path::ToolpathStore
