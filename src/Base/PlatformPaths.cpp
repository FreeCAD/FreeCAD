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

std::optional<std::string> getenvString(const char* key)
{
    if (!key || !*key) {
        return std::nullopt;
    }
    if (const char* value = std::getenv(key); value && *value) {
        return std::string(value);
    }
    return std::nullopt;
}

static fs::path pathFromEnvOrDefault(const char* envKey, fs::path fallback)
{
    if (auto value = getenvString(envKey); value) {
        return fs::path(*value);
    }
    return fallback;
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
    if (auto value = getenvString("USERPROFILE"); value) {
        return fs::path(*value);
    }

    auto drive = getenvString("HOMEDRIVE");
    auto homepath = getenvString("HOMEPATH");
    if (drive && homepath) {
        return fs::path(*drive) / fs::path(*homepath);
    }
    return fs::path("C:\\");
}
#endif

}  // namespace

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
    if (auto value = getenvString("HOME"); value) {
        home = fs::path(*value);
    }

    if (home.empty()) {
        struct passwd pwd {};
        struct passwd* result {};
        std::vector<char> buffer(16384);
        if (getpwuid_r(getuid(), &pwd, buffer.data(), buffer.size(), &result) == 0 && result) {
            home = fs::path(pwd.pw_dir);
        }
    }

    paths.home = home;
    paths.config = home / "Library" / "Preferences";
    paths.data = home / "Library" / "Application Support";
    paths.cache = home / "Library" / "Caches";
    paths.temp = pathFromEnvOrDefault("TMPDIR", fs::path("/tmp"));
#else
    fs::path home;
    if (auto value = getenvString("HOME"); value) {
        home = fs::path(*value);
    }

    if (home.empty()) {
        struct passwd pwd {};
        struct passwd* result {};
        std::vector<char> buffer(16384);
        if (getpwuid_r(getuid(), &pwd, buffer.data(), buffer.size(), &result) == 0 && result) {
            home = fs::path(pwd.pw_dir);
        }
    }

    paths.home = home;
    paths.config = pathFromEnvOrDefault("XDG_CONFIG_HOME", home / ".config");
    paths.data = pathFromEnvOrDefault("XDG_DATA_HOME", home / ".local" / "share");
    paths.cache = pathFromEnvOrDefault("XDG_CACHE_HOME", home / ".cache");
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
        fs::path p(argv0String);
        fs::path abs = fs::absolute(p, error);
        if (error) {
            return canonicalIfExists(p);
        }
        return canonicalIfExists(abs);
    }

    const auto pathEnv = getenvString("PATH");
    if (!pathEnv) {
        return {};
    }

#if defined(FC_OS_WIN32)
    constexpr char separator = ';';
#else
    constexpr char separator = ':';
#endif

    std::string_view view(*pathEnv);
    while (!view.empty()) {
        const std::size_t pos = view.find(separator);
        const std::string_view part = view.substr(0, pos);
        if (!part.empty()) {
            fs::path dir {std::string(part)};
            std::vector<fs::path> candidates;
            candidates.emplace_back(dir / argv0String);
#if defined(FC_OS_WIN32)
            candidates.emplace_back(dir / (argv0String + ".exe"));
            candidates.emplace_back(dir / (argv0String + ".com"));
#endif
            for (const auto& cand : candidates) {
                std::error_code error;
                if (fs::exists(cand, error) && !error) {
                    return canonicalIfExists(cand);
                }
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
