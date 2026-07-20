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

#include "PlatformPaths.h"

#include <cstdlib>
#include <string_view>
#include <system_error>
#include <vector>

#include <Base/FileInfo.h>
#include <Base/PathUtils.h>
#include <FCConfig.h>

#if defined(FC_OS_LINUX) || defined(FC_OS_CYGWIN) || defined(FC_OS_BSD) || defined(FC_OS_MACOSX)
# include <pwd.h>
# include <unistd.h>
#endif

#if defined(FC_OS_WIN32)
# include <windows.h>
# include <knownfolders.h>
# include <shlobj.h>
#endif

namespace fs = std::filesystem;

namespace Base
{

namespace
{

#if defined(FC_OS_WIN32)
std::wstring wideEnvironmentKey(const char* key)
{
    return pathFromUtf8(std::string_view(key)).native();
}

std::optional<std::string> readWindowsEnvironmentVariable(const char* key)
{
    const std::wstring wideKey = wideEnvironmentKey(key);
    DWORD bufferSize = 256;
    for (;;) {
        std::wstring value(bufferSize, L'\0');
        const DWORD length = GetEnvironmentVariableW(wideKey.c_str(), value.data(), bufferSize);
        if (length == 0) {
            return std::nullopt;
        }
        if (length < bufferSize) {
            value.resize(length);
            return FileInfo::pathToString(fs::path(value));
        }
        bufferSize = length + 1;
    }
}
#endif

static fs::path pathFromEnvOrDefault(const char* envKey, fs::path fallback)
{
    if (auto value = environmentVariableUtf8(envKey); value && !value->empty()) {
        return pathFromUtf8(*value);
    }
    return fallback;
}

static fs::path absolutePathFromEnvOrDefault(const char* envKey, fs::path fallback)
{
    if (auto value = environmentVariableUtf8(envKey); value && !value->empty()) {
        fs::path path = pathFromUtf8(*value);
        if (path.is_absolute()) {
            return path;
        }
    }
    return fallback;
}

static bool isExecutableFile(const fs::path& path)
{
    std::error_code error;
    if (!fs::is_regular_file(path, error) || error) {
        return false;
    }

#if !defined(FC_OS_WIN32)
    return access(path.c_str(), X_OK) == 0;
#else
    return true;
#endif
}

#if defined(FC_OS_WIN32)
static fs::path knownFolder(REFKNOWNFOLDERID id)
{
    PWSTR widePath = nullptr;
    const HRESULT hr = SHGetKnownFolderPath(id, KF_FLAG_DEFAULT, nullptr, &widePath);
    if (FAILED(hr) || !widePath) {
        return {};
    }
    fs::path out(widePath);
    CoTaskMemFree(widePath);
    return out;
}

static fs::path userProfileFallback()
{
    if (auto value = environmentVariableUtf8("USERPROFILE"); value && !value->empty()) {
        return pathFromUtf8(*value);
    }

    auto drive = environmentVariableUtf8("HOMEDRIVE");
    auto homepath = environmentVariableUtf8("HOMEPATH");
    if (drive && !drive->empty() && homepath && !homepath->empty()) {
        return pathFromUtf8(*drive) / pathFromUtf8(*homepath);
    }
    return fs::path("C:\\");
}
#endif

}  // namespace

std::optional<std::string> environmentVariableUtf8(const char* key)
{
    if (!key || !*key) {
        return std::nullopt;
    }
#if defined(FC_OS_WIN32)
    return readWindowsEnvironmentVariable(key);
#else
    if (const char* value = std::getenv(key); value && *value) {
        return std::string(value);
    }
    return std::nullopt;
#endif
}

StandardPaths standardPaths()
{
    StandardPaths paths;

#if defined(FC_OS_WIN32)
    paths.home = knownFolder(FOLDERID_Profile);
    if (paths.home.empty()) {
        paths.home = userProfileFallback();
    }

    paths.config = knownFolder(FOLDERID_RoamingAppData);
    if (paths.config.empty()) {
        paths.config = pathFromEnvOrDefault("APPDATA", paths.home / "AppData" / "Roaming");
    }
    paths.data = paths.config;

    paths.cache = knownFolder(FOLDERID_LocalAppData);
    if (paths.cache.empty()) {
        paths.cache = pathFromEnvOrDefault("LOCALAPPDATA", paths.home / "AppData" / "Local");
    }

    std::wstring tempBuf(MAX_PATH + 1, L'\0');
    DWORD len = GetTempPathW(static_cast<DWORD>(tempBuf.size()), tempBuf.data());
    if (len > 0 && len < tempBuf.size()) {
        tempBuf.resize(len);
        paths.temp = fs::path(tempBuf);
    }
    if (paths.temp.empty()) {
        paths.temp = paths.cache;
    }
#elif defined(FC_OS_MACOSX)
    fs::path home;
    if (auto value = environmentVariableUtf8("HOME"); value) {
        home = pathFromUtf8(*value);
    }

    if (home.empty()) {
        struct passwd pwd {};
        struct passwd* result {};
        std::vector<char> buffer(16384);
        if (getpwuid_r(getuid(), &pwd, buffer.data(), buffer.size(), &result) == 0 && result) {
            home = pathFromUtf8(pwd.pw_dir);
        }
    }

    paths.home = home;
    paths.config = home / "Library" / "Preferences";
    paths.data = home / "Library" / "Application Support";
    paths.cache = home / "Library" / "Caches";
    paths.temp = pathFromEnvOrDefault("TMPDIR", fs::path("/tmp"));
#else
    fs::path home;
    if (auto value = environmentVariableUtf8("HOME"); value) {
        home = pathFromUtf8(*value);
    }

    if (home.empty()) {
        struct passwd pwd {};
        struct passwd* result {};
        std::vector<char> buffer(16384);
        if (getpwuid_r(getuid(), &pwd, buffer.data(), buffer.size(), &result) == 0 && result) {
            home = pathFromUtf8(pwd.pw_dir);
        }
    }

    paths.home = home;
    paths.config = absolutePathFromEnvOrDefault("XDG_CONFIG_HOME", home / ".config");
    paths.data = absolutePathFromEnvOrDefault("XDG_DATA_HOME", home / ".local" / "share");
    paths.cache = absolutePathFromEnvOrDefault("XDG_CACHE_HOME", home / ".cache");
    paths.temp = pathFromEnvOrDefault("TMPDIR", fs::path("/tmp"));
    if (paths.temp.empty()) {
        paths.temp = pathFromEnvOrDefault("TMP", fs::path("/tmp"));
    }
    if (paths.temp.empty()) {
        paths.temp = pathFromEnvOrDefault("TEMP", fs::path("/tmp"));
    }
#endif

    paths.home = canonicalIfExists(paths.home);
    paths.config = canonicalIfExists(paths.config);
    paths.data = canonicalIfExists(paths.data);
    paths.cache = canonicalIfExists(paths.cache);
    paths.temp = canonicalIfExists(paths.temp);

    return paths;
}

std::filesystem::path resolveExecutablePath(const char* argv0)
{
    if (!argv0 || !*argv0) {
        return {};
    }

    const std::string argv0String(argv0);
    const bool hasSeparator = (argv0String.find('/') != std::string::npos)
        || (argv0String.find('\\') != std::string::npos);

    if (hasSeparator) {
        std::error_code error;
        fs::path p = pathFromUtf8(argv0String);
        fs::path abs = fs::absolute(p, error);
        if (error || !isExecutableFile(abs)) {
            return {};
        }
        return canonicalIfExists(abs);
    }

    const auto pathEnv = environmentVariableUtf8("PATH");
    if (!pathEnv) {
        return {};
    }

#if defined(FC_OS_WIN32)
    constexpr char separator = ';';
#else
    constexpr char separator = ':';
#endif

    std::string_view view(*pathEnv);
    while (true) {
        const std::size_t pos = view.find(separator);
        const std::string_view part = view.substr(0, pos);
        fs::path dir = part.empty() ? fs::path(".") : pathFromUtf8(part);
        std::vector<fs::path> candidates;
        candidates.emplace_back(dir / pathFromUtf8(argv0String));
#if defined(FC_OS_WIN32)
        candidates.emplace_back(dir / pathFromUtf8(argv0String + ".exe"));
        candidates.emplace_back(dir / pathFromUtf8(argv0String + ".com"));
#endif
        for (const auto& cand : candidates) {
            if (isExecutableFile(cand)) {
                return canonicalIfExists(cand);
            }
        }
        if (pos == std::string_view::npos) {
            break;
        }
        view.remove_prefix(pos + 1);
    }

    return {};
}

}  // namespace Base
