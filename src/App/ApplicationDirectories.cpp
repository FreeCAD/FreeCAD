// SPDX-License-Identifier: LGPL-2.1-or-later
/***************************************************************************************************
 *                                                                                                 *
 *   Copyright (c) 2002 Jürgen Riegel <juergen.riegel@web.de>                                      *
 *   Copyright (c) 2025 The FreeCAD project association AISBL                                      *
 *                                                                                                 *
 *   This file is part of FreeCAD.                                                                 *
 *                                                                                                 *
 *   FreeCAD is free software: you can redistribute it and/or modify it under the terms of the     *
 *   GNU Lesser General Public License as published by the Free Software Foundation, either        *
 *   version 2.1 of the License, or (at your option) any later version.                            *
 *                                                                                                 *
 *   FreeCAD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without  *
 *   even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the     *
 *   GNU Lesser General Public License for more details.                                           *
 *                                                                                                 *
 *   You should have received a copy of the GNU Lesser General Public License along with FreeCAD.  *
 *   If not, see <https://www.gnu.org/licenses/>.                                                  *
 *                                                                                                 *
 **************************************************************************************************/

#include <fmt/format.h>
#include <utility>
#include <QDir>
#include <QProcessEnvironment>
#include <QStandardPaths>
#include <QCoreApplication>

#include "ApplicationDirectories.h"

#include <FCConfig.h>

#if defined(FC_OS_LINUX) || defined(FC_OS_MACOSX) || defined(FC_OS_BSD)
#include <pwd.h>
#endif

#include <Base/FileInfo.h>
#include <Base/Exception.h>
#include "SafeMode.h"

#include <Python.h>
#include <QString>

#include "Base/Console.h"


using namespace App;
namespace fs = std::filesystem;

fs::path qstringToPath(const QString& path)
{
#if defined(FC_OS_WIN32)
    return {path.toStdWString()};
#else
    return {path.toStdString()};
#endif
}

ApplicationDirectories::ApplicationDirectories(std::map<std::string,std::string> &config)
{
    _currentVersion = extractVersionFromConfigMap(config);
    configurePaths(config);
    configureResourceDirectory(config);
    configureLibraryDirectory(config);
    configureHelpDirectory(config);
}

const fs::path& ApplicationDirectories::getHomePath() const
{
    return this->_home;
}

const fs::path& ApplicationDirectories::getUserHomePath() const
{
    return this->_userHome;
}

const fs::path& ApplicationDirectories::getTempPath() const {
    return this->_temp;
}

fs::path ApplicationDirectories::getTempFileName(const std::string & filename) const {
    auto tempPath = Base::FileInfo::pathToString(getTempPath());
    if (filename.empty()) {
        return Base::FileInfo::getTempFileName(nullptr, tempPath.c_str());
    }
    return Base::FileInfo::getTempFileName(filename.c_str(), tempPath.c_str());
}

const fs::path& ApplicationDirectories::getUserCachePath() const
{
    return this->_userCache;
}

const fs::path& ApplicationDirectories::getUserAppDataDir() const
{
    return this->_userAppData;
}

const fs::path& ApplicationDirectories::getUserMacroDir() const
{
    return this->_userMacro;
}

const fs::path& ApplicationDirectories::getResourceDir() const
{
    return this->_resource;
}

const fs::path& ApplicationDirectories::getHelpDir() const
{
    return this->_help;
}

const fs::path& ApplicationDirectories::getUserConfigPath() const {
    return this->_userConfig;
}

const fs::path& ApplicationDirectories::getLibraryDir() const {
    return this->_library;
}


/*!
 * \brief findPath
 * Returns the path where to store application files to.
 * If \a customHome is not empty, it will be used, otherwise a path starting from \a stdHome will be
 * used.
 */
fs::path ApplicationDirectories::findPath(const fs::path& stdHome, const fs::path& customHome,
                                          const std::vector<std::string>& subdirs, bool create) {
    fs::path appData = customHome;
    if (appData.empty()) {
        appData = stdHome;
    }

    // If a custom user home path is given, then don't modify it
    if (customHome.empty()) {
        for (const auto& it : subdirs) {
            appData = appData / it;
        }
    }

    // To write to our data path, we must create some directories, first.
    if (create && !fs::exists(appData) && !Py_IsInitialized()) {
        try {
            fs::create_directories(appData);
        } catch (const fs::filesystem_error& e) {
            throw Base::FileSystemError("Could not create directories. Failed with: " + e.code().message());
        }
    }

    return appData;
}

void ApplicationDirectories::appendVersionIfPossible(const fs::path& basePath, std::vector<std::string> &subdirs) const
{
    fs::path pathToCheck = basePath;
    for (const auto& it : subdirs) {
        pathToCheck = pathToCheck / it;
    }
    if (isVersionedPath(pathToCheck)) {
        return; // Bail out if it's already versioned
    }
    if (fs::exists(pathToCheck)) {
        std::string version = mostRecentAvailableConfigVersion(pathToCheck);
        if (!version.empty()) {
            subdirs.emplace_back(std::move(version));
        }
    } else {
        auto [major, minor] = _currentVersion;
        subdirs.emplace_back(versionStringForPath(major, minor));
    }
}

void ApplicationDirectories::configurePaths(std::map<std::string,std::string>& mConfig)
{
    bool keepDeprecatedPaths = mConfig.contains("KeepDeprecatedPaths");

    // std paths
    _home = fs::path(mConfig.at("AppHomePath"));
    mConfig["BinPath"] = mConfig.at("AppHomePath") + "bin" + PATHSEP;
    mConfig["DocPath"] = mConfig.at("AppHomePath") + "doc" + PATHSEP;

    // this is to support a portable version of FreeCAD
    auto [customHome, customData, customTemp] = getCustomPaths();
    _usingCustomDirectories = !customHome.empty() || !customData.empty();

    // get the system standard paths
    auto [configHome, dataHome, cacheHome, tempPath] = getStandardPaths();

    if (mConfig.contains("SafeMode")) {
        if (startSafeMode(mConfig)) {
            // If we're in safe mode, don't try to set any directories here, they've been overridden
            // by temp directories in the SafeMode setup.
            return;
        }
    }

    // User home path
    //
    fs::path homePath = findUserHomePath(customHome);
    mConfig["UserHomePath"] = Base::FileInfo::pathToString(homePath);
    _userHome = homePath;

    // the old path name to save config and data files
    std::vector<std::string> subdirs;
    if (keepDeprecatedPaths) {
        configHome = homePath;
        dataHome = homePath;
        cacheHome = homePath;
        getOldDataLocation(mConfig, subdirs);
    }
    else {
        getSubDirectories(mConfig, subdirs);
    }


    // User data path
    //
    auto dataSubdirs = subdirs;
    appendVersionIfPossible(dataHome, dataSubdirs);
    fs::path data = findPath(dataHome, customData, dataSubdirs, true);
    _userAppData = data;
    mConfig["UserAppData"] = Base::FileInfo::pathToString(data) + PATHSEP;


    // User config path
    //
    auto configSubdirs = subdirs;
    appendVersionIfPossible(configHome, configSubdirs);
    fs::path config = findPath(configHome, customHome, configSubdirs, true);
    _userConfig = config;
    mConfig["UserConfigPath"] = Base::FileInfo::pathToString(config) + PATHSEP;


    // User cache path
    //
    std::vector<std::string> cachedirs = subdirs;
    cachedirs.emplace_back("Cache");
    fs::path cache = findPath(cacheHome, customTemp, cachedirs, true);
    _userCache = cache;
    mConfig["UserCachePath"] = Base::FileInfo::pathToString(cache) + PATHSEP;


    // Set application temporary directory
    //
    std::vector<std::string> empty;
    fs::path tmp = findPath(tempPath, customTemp, empty, true);
    _temp = tmp;
    mConfig["AppTempPath"] = Base::FileInfo::pathToString(tmp) + PATHSEP;


    // Set the default macro directory
    //
    std::vector<std::string> macrodirs{"Macro"};
    fs::path macro = findPath(_userAppData, customData, macrodirs, true);
    _userMacro = macro;
    mConfig["UserMacroPath"] = Base::FileInfo::pathToString(macro) + PATHSEP;
}

bool ApplicationDirectories::startSafeMode(std::map<std::string,std::string>& mConfig)
{
    SafeMode::StartSafeMode();
    if (SafeMode::SafeModeEnabled()) {
        _userAppData = mConfig["UserAppData"];
        _userConfig = mConfig["UserConfigPath"];
        _userCache = mConfig["UserCachePath"];
        _temp = mConfig["AppTempPath"];
        _userMacro = mConfig["UserMacroPath"];
        _userHome = mConfig["UserHomePath"];
        _usingCustomDirectories = true;
        return true;
    }
    return false;
}

std::filesystem::path ApplicationDirectories::sanitizePath(const std::string& pathAsString)
{
    size_t positionOfFirstNull = pathAsString.find('\0');
    if (positionOfFirstNull != std::string::npos) {
        return {pathAsString.substr(0, positionOfFirstNull)};
    }
    return {pathAsString};
}

void ApplicationDirectories::configureResourceDirectory(const std::map<std::string,std::string>& mConfig) {
#ifdef RESOURCEDIR
    // #6892: Conda may inject null characters
    fs::path path = sanitizePath(RESOURCEDIR);
    if (path.is_absolute()) {
        _resource = path;
    } else {
        _resource = Base::FileInfo::stringToPath(mConfig.at("AppHomePath")) / path;
    }
#else
    _resource = fs::path(mConfig.at("AppHomePath"));
#endif
}

void ApplicationDirectories::configureLibraryDirectory(const std::map<std::string,std::string>& mConfig) {
#ifdef LIBRARYDIR
    // #6892: Conda may inject null characters
    fs::path path = sanitizePath(LIBRARYDIR);
    if (path.is_absolute()) {
        _library = path;
    } else {
        _library = Base::FileInfo::stringToPath(mConfig.at("AppHomePath")) / path;
    }
#else
    _library = Base::FileInfo::stringToPath(mConfig.at("AppHomePath")) / "lib";
#endif
}


void ApplicationDirectories::configureHelpDirectory(const std::map<std::string,std::string>& mConfig)
{
#ifdef DOCDIR
    // #6892: Conda may inject null characters
    fs::path path = sanitizePath(DOCDIR);
    if (path.is_absolute()) {
        _help = path;
    } else {
        _help = Base::FileInfo::stringToPath(mConfig.at("AppHomePath")) / path;
    }
#else
    _help = Base::FileInfo::stringToPath(mConfig.at("DocPath"));
#endif
}


fs::path ApplicationDirectories::getUserHome()
{
    fs::path path;
#if defined(FC_OS_LINUX) || defined(FC_OS_CYGWIN) || defined(FC_OS_BSD) || defined(FC_OS_MACOSX)
    // Default paths for the user-specific stuff
    struct passwd pwd {};
    struct passwd *result {};
    constexpr std::size_t bufferLength = 16384;
    std::vector<char> buffer(bufferLength);
    const int error = getpwuid_r(getuid(), &pwd, buffer.data(), buffer.size(), &result);
    if (!result || error != 0) {
        throw Base::RuntimeError("Getting HOME path from system failed!");
    }
    std::string sanitizedPath = sanitizePath(pwd.pw_dir);
    path = Base::FileInfo::stringToPath(sanitizedPath);
#else
    path = Base::FileInfo::stringToPath(QStandardPaths::writableLocation(QStandardPaths::HomeLocation).toStdString());
#endif
    return path;
}

bool ApplicationDirectories::usingCustomDirectories() const
{
    return _usingCustomDirectories;
}

#if defined(FC_OS_WIN32)  // This is ONLY used on Windows now, so don't even compile it elsewhere
#include <codecvt>
#include "ShlObj.h"
QString ApplicationDirectories::getOldGenericDataLocation()
{
    WCHAR szPath[MAX_PATH];
    std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
    if (SUCCEEDED(SHGetFolderPathW(NULL, CSIDL_APPDATA, NULL, 0, szPath))) {
        return QString::fromStdString(converter.to_bytes(szPath));
    }
    return {};
}
#endif

void ApplicationDirectories::getSubDirectories(const std::map<std::string,std::string>& mConfig,
                                               std::vector<std::string>& appData)
{
    // If 'AppDataSkipVendor' is defined, the value of 'ExeVendor' must not be part of
    // the path.
    if (!mConfig.contains("AppDataSkipVendor") && mConfig.contains("ExeVendor")) {
        appData.push_back(mConfig.at("ExeVendor"));
    }
    appData.push_back(mConfig.at("ExeName"));
}

void ApplicationDirectories::getOldDataLocation(const std::map<std::string,std::string>& mConfig,
                                                std::vector<std::string>& appData)
{
    // The name of the directory where the parameters are stored should be the name of
    // the application (for branding reasons).
#if defined(FC_OS_LINUX) || defined(FC_OS_CYGWIN) || defined(FC_OS_BSD)
    // If 'AppDataSkipVendor' is defined, the value of 'ExeVendor' must not be part of
    // the path.
    if (!mConfig.contains("AppDataSkipVendor")) {
        appData.push_back(std::string(".") + mConfig.at("ExeVendor"));
        appData.push_back(mConfig.at("ExeName"));
    } else {
        appData.push_back(std::string(".") + mConfig.at("ExeName"));
    }

#elif defined(FC_OS_MACOSX) || defined(FC_OS_WIN32)
    getSubDirectories(mConfig, appData);
#endif
}

fs::path ApplicationDirectories::findUserHomePath(const fs::path& userHome)
{
    return userHome.empty() ? getUserHome() : userHome;
}

std::tuple<fs::path, fs::path, fs::path> ApplicationDirectories::getCustomPaths()
{
    const QProcessEnvironment env(QProcessEnvironment::systemEnvironment());
    QString userHome = env.value(QStringLiteral("FREECAD_USER_HOME"));
    QString userData = env.value(QStringLiteral("FREECAD_USER_DATA"));
    QString userTemp = env.value(QStringLiteral("FREECAD_USER_TEMP"));

    auto toNativePath = [](QString& path) {
        if (!path.isEmpty()) {
            if (const QDir dir(path); dir.exists()) {
                path = QDir::toNativeSeparators(dir.canonicalPath());
            }
            else {
                path.clear();
            }
        }
    };

    // verify env. variables
    toNativePath(userHome);
    toNativePath(userData);
    toNativePath(userTemp);

    // if FREECAD_USER_HOME is set but not FREECAD_USER_DATA
    if (!userHome.isEmpty() && userData.isEmpty()) {
        userData = userHome;
    }

    // if FREECAD_USER_HOME is set but not FREECAD_USER_TEMP
    if (!userHome.isEmpty() && userTemp.isEmpty()) {
        const QDir dir(userHome);
        dir.mkdir(QStringLiteral("temp"));
        const QFileInfo fi(dir, QStringLiteral("temp"));
        userTemp = fi.absoluteFilePath();
    }

    return {qstringToPath(userHome),
            qstringToPath(userData),
            qstringToPath(userTemp)};
}

std::tuple<fs::path, fs::path, fs::path, fs::path> ApplicationDirectories::getStandardPaths()
{
    QString configHome = QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation);
    QString dataHome = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation);
    QString cacheHome = QStandardPaths::writableLocation(QStandardPaths::GenericCacheLocation);
    QString tempPath = QStandardPaths::writableLocation(QStandardPaths::TempLocation);

    // Keep the old behaviour
#if defined(FC_OS_WIN32)
    configHome = getOldGenericDataLocation();
    dataHome = configHome;

    // On systems with non-7-bit-ASCII application data directories,
    // GetTempPathW will return a path in DOS format. This path will be
    // accepted by boost's file_lock class.
    // Since boost 1.76, there is now a version that accepts a wide string.
#if (BOOST_VERSION < 107600)
    tempPath = QString::fromStdString(Base::FileInfo::getTempPath());
    cacheHome = tempPath;
#endif
#endif

    return {qstringToPath(configHome),
            qstringToPath(dataHome),
            qstringToPath(cacheHome),
            qstringToPath(tempPath)};
}



std::string ApplicationDirectories::versionStringForPath(int major, int minor)
{
    // NOTE: This is intended to be stable over time, so if the format changes, a condition should be added to check for
    // older versions and return this format for them, even if the new format differs.
    return fmt::format("v{}-{}", major, minor);
}

bool ApplicationDirectories::isVersionedPath(const fs::path &startingPath) const {
    for (int major = std::get<0>(_currentVersion); major >= 1; --major) {
        constexpr int largestPossibleMinor = 99;  // We have to start someplace
        int startingMinor = largestPossibleMinor;
        if (major == std::get<0>(_currentVersion)) {
            startingMinor = std::get<1>(_currentVersion);
        }
        for (int minor = startingMinor; minor >= 0; --minor) {
            if (startingPath.filename() == versionStringForPath(major, minor)) {
                return true;
            }
        }
    }
    return false;
}

std::string ApplicationDirectories::mostRecentAvailableConfigVersion(const fs::path &startingPath) const {
    for (int major = std::get<0>(_currentVersion); major >= 1; --major) {
        constexpr int largestPossibleMinor = 99;  // We have to start someplace
        int startingMinor = largestPossibleMinor;
        if (major == std::get<0>(_currentVersion)) {
            startingMinor = std::get<1>(_currentVersion);
        }
        for (int minor = startingMinor; minor >= 0; --minor) {
            auto version = startingPath / versionStringForPath(major, minor);
            if (fs::is_directory(version)) {
                return versionStringForPath(major, minor);
            }
        }
    }
    return "";
}

fs::path ApplicationDirectories::mostRecentConfigFromBase(const fs::path &startingPath) const {
    // Starting in FreeCAD v1.1, we switched to using a versioned config path for the three configuration
    // directories:
    // UserAppData
    // UserConfigPath
    // UserMacroPath
    //
    // Migration to the versioned structured is NOT automatic: at the App level, we just find the most
    // recent directory and use it, regardless of which version of the program is currently running.
    // It is up to user-facing code in Gui to determine whether to ask a user if they want to migrate
    // and to call the App-level functions that do that work.

    // The simplest and most common case is if the current version subfolder already exists
    auto current = startingPath / versionStringForPath(std::get<0>(_currentVersion), std::get<1>(_currentVersion));
    if (fs::is_directory(current)) {
        return current;
    }

    // If the current version doesn't exist, see if a previous version does
    std::string bestVersion = mostRecentAvailableConfigVersion(startingPath);
    if (!bestVersion.empty()) {
        return startingPath / bestVersion;
    }
    return startingPath;  // No versioned config found
}

bool ApplicationDirectories::usingCurrentVersionConfig(fs::path config) const {
    if (config.filename().empty()) {
        config = config.parent_path();
    }
    auto version = Base::FileInfo::pathToString(config.filename());
    return version == versionStringForPath(std::get<0>(_currentVersion), std::get<1>(_currentVersion));
}

void ApplicationDirectories::migrateConfig(const fs::path &oldPath, const fs::path &newPath)
{
    fs::create_directories(newPath);
    for (auto& file : fs::directory_iterator(oldPath)) {
        if (file == newPath) {
            // Handle the case where newPath is a subdirectory of oldPath
            continue;
        }
        fs::copy(file.path(),
                 newPath / file.path().filename(),
                 fs::copy_options::recursive | fs::copy_options::copy_symlinks);
    }
}

void ApplicationDirectories::migrateAllPaths(const std::vector<fs::path> &paths) const {
    auto [major, minor] = _currentVersion;
    std::set<fs::path> uniquePaths (paths.begin(), paths.end());
    for (auto path : uniquePaths) {
        if (path.filename().empty()) {
            // Handle the case where the path was constructed from a std::string with a trailing /
            path = path.parent_path();
        }
        fs::path newPath;
        if (isVersionedPath(path)) {
            newPath = path.parent_path() / versionStringForPath(major, minor);
        } else {
            newPath = path / versionStringForPath(major, minor);
        }
        Base::Console().message("Migrating config from %s to %s\n", Base::FileInfo::pathToString(path), Base::FileInfo::pathToString(newPath));
        if (fs::exists(newPath)) {
            continue;  // Ignore an existing path: not an error, just a migration that was already done
        }
        fs::create_directories(newPath);
        migrateConfig(path, newPath);
    }
}

// TODO: Consider using this for all UNIX-like OSes
#if defined(__OpenBSD__)
#include <cstdio>
#include <cstdlib>
#include <sys/param.h>
#include <QCoreApplication>

fs::path ApplicationDirectories::findHomePath(const char* sCall)
{
    // We have three ways to start this application either use one of the two executables or
    // import the FreeCAD.so module from a running Python session. In the latter case the
    // Python interpreter is already initialized.
    std::string absPath;
    std::string homePath;
    if (Py_IsInitialized()) {
        // Note: `realpath` is known to cause a buffer overflow because it
        // expands the given path to an absolute path of unknown length.
        // Even setting PATH_MAX does not necessarily solve the problem
        // for sure, but the risk of overflow is rather small.
        char resolved[PATH_MAX];
        char* path = realpath(sCall, resolved);
        if (path)
            absPath = path;
    }
    else {
        int argc = 1;
        QCoreApplication app(argc, (char**)(&sCall));
        absPath = QCoreApplication::applicationFilePath().toStdString();
    }

    // should be an absolute path now
    std::string::size_type pos = absPath.find_last_of("/");
    homePath.assign(absPath,0,pos);
    pos = homePath.find_last_of("/");
    homePath.assign(homePath,0,pos+1);

    return Base::FileInfo::stringToPath(homePath);
}

#elif defined (FC_OS_LINUX) || defined(FC_OS_CYGWIN) || defined(FC_OS_BSD)
#include <cstdio>
#include <cstdlib>
#include <sys/param.h>
#if defined(__FreeBSD__)
#include <sys/sysctl.h>
#endif

fs::path ApplicationDirectories::findHomePath(const char* sCall)
{
    // We have three ways to start this application either use one of the two executables or
    // import the FreeCAD.so module from a running Python session. In the latter case the
    // Python interpreter is already initialized.
    std::string absPath;
    std::string homePath;
    if (Py_IsInitialized()) {
        // Note: `realpath` is known to cause a buffer overflow because it
        // expands the given path to an absolute path of unknown length.
        // Even setting PATH_MAX does not necessarily solve the problem
        // for sure, but the risk of overflow is rather small.
        char resolved[PATH_MAX];
        char* path = realpath(sCall, resolved);
        if (path)
            absPath = path;
    }
    else {
        // Find the path of the executable. Theoretically, there could occur a
        // race condition when using readlink, but we only use this method to
        // get the absolute path of the executable to compute the actual home
        // path. In the worst case we simply get q wrong path, and FreeCAD is not
        // able to load its modules.
        char resolved[PATH_MAX];
#if defined(FC_OS_BSD)
        int mib[4];
        mib[0] = CTL_KERN;
        mib[1] = KERN_PROC;
        mib[2] = KERN_PROC_PATHNAME;
        mib[3] = -1;
        size_t cb = sizeof(resolved);
        sysctl(mib, 4, resolved, &cb, NULL, 0);
        int nchars = strlen(resolved);
#else
        int nchars = readlink("/proc/self/exe", resolved, PATH_MAX);
#endif
        if (nchars < 0 || nchars >= PATH_MAX)
            throw Base::FileSystemError("Cannot determine the absolute path of the executable");
        resolved[nchars] = '\0'; // enforce null termination
        absPath = resolved;
    }

    // should be an absolute path now
    std::string::size_type pos = absPath.find_last_of("/");
    homePath.assign(absPath,0,pos);
    pos = homePath.find_last_of("/");
    homePath.assign(homePath,0,pos+1);

    return Base::FileInfo::stringToPath(homePath);
}

#elif defined(FC_OS_MACOSX)
#include <mach-o/dyld.h>
#include <string>
#include <cstdlib>
#include <sys/param.h>

fs::path ApplicationDirectories::findHomePath(const char* sCall)
{
    // If Python is initialized at this point, then we're being run from
    // MainPy.cpp, which hopefully rewrote argv[0] to point at the
    // FreeCAD shared library.
    if (!Py_IsInitialized()) {
        uint32_t sz = 0;

        // function only returns "sz" if the first arg is too small to hold value
        _NSGetExecutablePath(nullptr, &sz);

        if (const auto buf = new char[++sz]; _NSGetExecutablePath(buf, &sz) == 0) {
            std::array<char, PATH_MAX> resolved{};
            const char* path = realpath(buf, resolved.data());
            delete [] buf;

            if (path) {
                const std::string Call(resolved.data());
                std::string TempHomePath;
                std::string::size_type pos = Call.find_last_of(fs::path::preferred_separator);
                TempHomePath.assign(Call,0,pos);
                pos = TempHomePath.find_last_of(fs::path::preferred_separator);
                TempHomePath.assign(TempHomePath,0,pos+1);
                return Base::FileInfo::stringToPath(TempHomePath);
            }
        } else {
            delete [] buf;
        }
    }

    return Base::FileInfo::stringToPath(sCall);
}

#elif defined (FC_OS_WIN32)
fs::path ApplicationDirectories::findHomePath(const char* sCall)
{
    // We have several ways to start this application:
    // * use one of the two executables
    // * import the FreeCAD.pyd module from a running Python session. In this case the
    //   Python interpreter is already initialized.
    // * use a custom dll that links FreeCAD core dlls and that is loaded by its host application
    //   In this case the calling name should be set to FreeCADBase.dll or FreeCADApp.dll in order
    //   to locate the correct home directory
    wchar_t szFileName [MAX_PATH];
    QString dll(QString::fromUtf8(sCall));
    if (Py_IsInitialized() || dll.endsWith(QLatin1String(".dll"))) {
        GetModuleFileNameW(GetModuleHandleA(sCall),szFileName, MAX_PATH-1);
    }
    else {
        GetModuleFileNameW(0, szFileName, MAX_PATH-1);
    }

    std::wstring Call(szFileName), homePath;
    std::wstring::size_type pos = Call.find_last_of(fs::path::preferred_separator);
    homePath.assign(Call,0,pos);
    pos = homePath.find_last_of(fs::path::preferred_separator);
    homePath.assign(homePath,0,pos+1);

    // fixes #0001638 to avoid loading DLLs from Windows' system directories before FreeCAD's bin folder
    std::wstring binPath = homePath;
    binPath += L"bin";
    SetDllDirectoryW(binPath.c_str());

    // https://stackoverflow.com/questions/5625884/conversion-of-stdwstring-to-qstring-throws-linker-error
#ifdef _MSC_VER
    QString str = QString::fromUtf16(reinterpret_cast<const ushort *>(homePath.c_str()));
#else
    QString str = QString::fromStdWString(homePath);
#endif
    return qstringToPath(str);
}

#else
# error "std::string ApplicationDirectories::findHomePath(const char*) not implemented"
#endif

std::tuple<int, int> ApplicationDirectories::extractVersionFromConfigMap(const std::map<std::string,std::string> &config)
{
    try {
        int major = std::stoi(config.at("BuildVersionMajor"));
        int minor = std::stoi(config.at("BuildVersionMinor"));
        return std::make_tuple(major, minor);
    } catch (const std::exception& e) {
        throw Base::RuntimeError("Failed to parse version from config: " + std::string(e.what()));
    }
}
