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

#include "PathUtils.h"

#include <algorithm>
#include <system_error>

#include <Base/FileInfo.h>
#include <FCConfig.h>

#if defined(FC_OS_WIN32)
# include <windows.h>
#endif

namespace fs = std::filesystem;

namespace Base
{

fs::path pathFromUtf8(std::string_view utf8)
{
    if (utf8.empty()) {
        return {};
    }

#if defined(FC_OS_WIN32)
    // Prefer Win32 conversion to avoid deprecated codecvt usage on newer toolchains.
    const int required
        = MultiByteToWideChar(CP_UTF8, 0, utf8.data(), static_cast<int>(utf8.size()), nullptr, 0);
    if (required > 0) {
        std::wstring wide(static_cast<std::size_t>(required), L'\0');
        const int written = MultiByteToWideChar(
            CP_UTF8,
            0,
            utf8.data(),
            static_cast<int>(utf8.size()),
            wide.data(),
            required
        );
        if (written > 0) {
            return fs::path(std::move(wide));
        }
    }

    // Fallback to existing conversion logic.
    return FileInfo::stringToPath(std::string(utf8));
#else
    return fs::path(std::string(utf8));
#endif
}

std::string pathToPortableUtf8(const fs::path& path)
{
#if defined(FC_OS_WIN32)
    auto result = FileInfo::pathToString(path);
    std::replace(result.begin(), result.end(), '\\', '/');
    return result;
#else
    return path.generic_string();
#endif
}

fs::path canonicalIfExists(const fs::path& path)
{
    std::error_code error;
    if (!fs::exists(path, error) || error) {
        return path;
    }

    fs::path canon = fs::canonical(path, error);
    if (error) {
        return path;
    }
    return canon;
}

std::optional<fs::path> normalizePath(const fs::path& path, const NormalizePathOptions& options)
{
    std::error_code error;
    fs::path out = path;

    if (options.makeAbsolute) {
        out = fs::absolute(out, error);
        if (error) {
            return std::nullopt;
        }
    }

    if (options.weaklyCanonical) {
        out = fs::weakly_canonical(out, error);
        if (error) {
            return std::nullopt;
        }
    }
    else {
        out = out.lexically_normal();
    }

    if (options.createParentDirectories) {
        fs::create_directories(out.parent_path(), error);
        if (error) {
            return std::nullopt;
        }
    }

    return out;
}

}  // namespace Base
