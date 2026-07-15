// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2024 Benjamin Nauck <benjamin@nauck.se>                 *
 *                                                                         *
 *   This file is part of the FreeCAD CAx development system.              *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU Library General Public License (LGPL)   *
 *   as published by the Free Software Foundation; either version 2 of     *
 *   the License, or (at your option) any later version.                   *
 *   for detail see the LICENCE text file.                                 *
 *                                                                         *
 *   FreeCAD is distributed in the hope that it will be useful,            *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU Library General Public License for more details.                  *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with FreeCAD; if not, write to the Free Software        *
 *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
 *   USA                                                                   *
 *                                                                         *
 ***************************************************************************/

#include <array>
#include <cerrno>
#include <filesystem>
#include <map>
#include <string>
#include <system_error>
#include <vector>

#include <Base/FileInfo.h>

#include "Application.h"
#include "FCConfig.h"

#include "SafeMode.h"

#if defined(FC_OS_WIN32)
# include <objbase.h>
# include <sddl.h>
# include <windows.h>
#else
# include <unistd.h>
#endif

namespace fs = std::filesystem;

static fs::path tempDir;

static bool createSecureTemporaryBaseDir(const fs::path& base, std::error_code& error)
{
#if defined(FC_OS_WIN32)
    GUID guid {};
    if (FAILED(CoCreateGuid(&guid))) {
        error = std::make_error_code(std::errc::io_error);
        return false;
    }

    wchar_t guidText[39] {};
    if (StringFromGUID2(
            guid,
            guidText,
            static_cast<int>(sizeof(guidText) / sizeof(*guidText)))
        == 0) {
        error = std::make_error_code(std::errc::io_error);
        return false;
    }

    const fs::path candidate = base / (std::wstring(L"FreeCADSafeMode-") + guidText);

    // Protect the directory and its descendants at creation time. The
    // owner-rights ACE avoids the window where a post-creation ACL update
    // would expose Safe Mode data.
    PSECURITY_DESCRIPTOR descriptor = nullptr;
    if (!ConvertStringSecurityDescriptorToSecurityDescriptorW(
            L"D:P(A;OICI;FA;;;OW)",
            SDDL_REVISION_1,
            &descriptor,
            nullptr)) {
        error = std::error_code(static_cast<int>(GetLastError()), std::system_category());
        return false;
    }

    SECURITY_ATTRIBUTES attributes {
        sizeof(SECURITY_ATTRIBUTES),
        descriptor,
        FALSE,
    };
    const BOOL created = CreateDirectoryW(candidate.c_str(), &attributes);
    const DWORD lastError = created ? ERROR_SUCCESS : GetLastError();
    LocalFree(descriptor);

    if (!created) {
        error = lastError == ERROR_ALREADY_EXISTS
            ? std::make_error_code(std::errc::file_exists)
            : std::error_code(static_cast<int>(lastError), std::system_category());
        return false;
    }

    tempDir = candidate;
    return true;
#else
    std::string pattern = Base::FileInfo::pathToString(base / "FreeCADSafeMode-XXXXXX");
    std::vector<char> writable(pattern.begin(), pattern.end());
    writable.push_back('\0');
    if (!mkdtemp(writable.data())) {
        error = std::error_code(errno, std::generic_category());
        return false;
    }

    // mkdtemp atomically creates the directory with mode 0700.
    tempDir = fs::path(writable.data());
    return true;
#endif
}

static bool _createTemporaryBaseDir()
{
    std::error_code error;
    const fs::path base = fs::temp_directory_path(error);
    if (error) {
        return false;
    }

    for (int attempt = 0; attempt < 64; ++attempt) {
        if (createSecureTemporaryBaseDir(base, error)) {
            return true;
        }
        error.clear();
    }

    return false;
}

static bool _replaceDirs()
{
    auto& config = App::GetApplication().Config();

    constexpr std::array dirs = {
        "UserAppData",
        "UserConfigPath",
        "UserCachePath",
        "AppTempPath",
        "UserMacroPath",
        "UserHomePath",
    };

    std::map<std::string, std::string> replacements;
    for (auto const d : dirs) {
        try {
            const fs::path path = tempDir / d;
            fs::create_directories(path);
            replacements[d] = Base::FileInfo::pathToString(path) + PATHSEP;
        }
        catch (const fs::filesystem_error&) {
            return false;
        }
    }

    for (const auto& [key, path] : replacements) {
        config[key] = path;
    }
    return true;
}

void SafeMode::StartSafeMode()
{
    if (!_createTemporaryBaseDir()) {
        return;
    }

    if (!_replaceDirs()) {
        SafeMode::Destruct();
    }
}

bool SafeMode::SafeModeEnabled()
{
    return !tempDir.empty();
}

void SafeMode::Destruct()
{
    if (tempDir.empty()) {
        return;
    }
    std::error_code error;
    // Best-effort cleanup: leaving safe mode should not fail because temporary removal failed.
    fs::remove_all(tempDir, error);
    tempDir.clear();
}
