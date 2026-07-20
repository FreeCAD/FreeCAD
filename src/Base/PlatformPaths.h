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

#ifndef FC_GLOBAL_H
# include <FCGlobal.h>
#endif

namespace Base
{

/// Read an environment variable and return its value as UTF-8.
/// On Windows this bypasses the narrow CRT environment and preserves Unicode.
/// Empty variables are treated like undefined variables and return nullopt.
BaseExport std::optional<std::string> environmentVariableUtf8(const char* key);

struct BaseExport StandardPaths
{
    std::filesystem::path home;
    std::filesystem::path config;
    std::filesystem::path data;
    std::filesystem::path cache;
    std::filesystem::path temp;
};

/// Returns platform default locations for home/config/data/cache/temp.
///
/// These are raw platform defaults. Callers are responsible for applying
/// application-specific subdirectories, versioning, migration, and overrides.
///
/// Notes:
/// - On Linux/BSD follows XDG_*_HOME when set, otherwise defaults under HOME.
/// - On macOS uses ~/Library/{Preferences,Application Support,Caches}.
/// - On Windows uses known folders (RoamingAppData/LocalAppData/Profile) with env fallbacks.
BaseExport StandardPaths standardPaths();

/// Best-effort absolute path of the current executable given argv[0].
/// On POSIX, searches PATH when argv0 has no directory separator.
BaseExport std::filesystem::path resolveExecutablePath(const char* argv0);

}  // namespace Base
