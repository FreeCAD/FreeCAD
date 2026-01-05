// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2026 The FreeCAD project association AISBL
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

#include <filesystem>
#include <optional>
#include <string>
#include <string_view>

#ifndef FC_GLOBAL_H
# include <FCGlobal.h>
#endif

namespace Base
{

struct BaseExport NormalizePathOptions
{
    bool makeAbsolute {true};
    bool weaklyCanonical {true};
    bool createParentDirectories {false};
};

/// Convert a UTF-8 encoded string to a platform filesystem path.
/// On Windows this produces a wide path.
std::filesystem::path pathFromUtf8(std::string_view utf8);

/// Returns canonical(path) if it exists, otherwise returns the input unchanged.
std::filesystem::path canonicalIfExists(const std::filesystem::path& path);

/// Best-effort normalize a path (absolute + weakly_canonical) and optionally create parents.
/// Returns std::nullopt if normalization fails.
std::optional<std::filesystem::path> normalizePath(
    const std::filesystem::path& path,
    const NormalizePathOptions& options = {}
);

}  // namespace Base
