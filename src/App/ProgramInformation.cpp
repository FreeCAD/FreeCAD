// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2025 Werner Mayer <wmayer[at]users.sourceforge.net>     *
 *                                                                         *
 *   This file is part of FreeCAD.                                         *
 *                                                                         *
 *   FreeCAD is free software: you can redistribute it and/or modify it    *
 *   under the terms of the GNU Lesser General Public License as           *
 *   published by the Free Software Foundation, either version 2.1 of the  *
 *   License, or (at your option) any later version.                       *
 *                                                                         *
 *   FreeCAD is distributed in the hope that it will be useful, but        *
 *   WITHOUT ANY WARRANTY; without even the implied warranty of            *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU      *
 *   Lesser General Public License for more details.                       *
 *                                                                         *
 *   You should have received a copy of the GNU Lesser General Public      *
 *   License along with FreeCAD. If not, see                               *
 *   <https://www.gnu.org/licenses/>.                                      *
 *                                                                         *
 **************************************************************************/

#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <optional>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>
#include <vector>

#include <boost/version.hpp>
#include <boost/tokenizer.hpp>

#if defined(FREECAD_BUILD_QT) && FREECAD_BUILD_QT
# include <QFile>
# include <QFileInfo>
# include <QLocale>
# include <QProcessEnvironment>
# include <QRegularExpression>
# include <QRegularExpressionMatch>
# include <QSettings>
# include <QStringList>
# include <QSysInfo>
#endif

#if !(defined(FREECAD_BUILD_QT) && FREECAD_BUILD_QT) && !defined(FC_OS_WIN32)
# include <sys/utsname.h>
#endif

#include <LibraryVersions.h>

#include <Base/Console.h>
#include <Base/Exception.h>
#include <Base/Interpreter.h>

#include "Application.h"
#include "Metadata.h"
#include "ProgramInformation.h"

using namespace App;
namespace fs = std::filesystem;

namespace {

#if defined(FREECAD_BUILD_QT) && FREECAD_BUILD_QT
std::ostream& operator<<(std::ostream& os, const QString& str)
{
    os << str.toStdString();
    return os;
}
#endif

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

#if !(defined(FREECAD_BUILD_QT) && FREECAD_BUILD_QT)
void appendIfNotEmpty(std::vector<std::string>& values, std::string value)
{
    if (!value.empty()) {
        values.push_back(std::move(value));
    }
}

std::string joinStrings(const std::vector<std::string>& values, std::string_view separator)
{
    std::string result;
    for (const auto& value : values) {
        if (!result.empty()) {
            result += separator;
        }
        result += value;
    }
    return result;
}

#endif

std::string trim(std::string text)
{
    const auto first = text.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) {
        return {};
    }

    const auto last = text.find_last_not_of(" \t\r\n");
    return text.substr(first, last - first + 1);
}

std::string cleanLine(std::string text)
{
    std::ranges::replace(text, '\n', ' ');
    std::ranges::replace(text, '\r', ' ');
    return trim(std::move(text));
}

std::string displayNameFromPath(fs::path path)
{
    while (!path.empty() && path.filename().empty()) {
        auto parent = path.parent_path();
        if (parent == path) {
            break;
        }
        path = std::move(parent);
    }

    return path.filename().string();
}

#if !(defined(FREECAD_BUILD_QT) && FREECAD_BUILD_QT)
std::string readLinuxPrettyName()
{
    std::ifstream osRelease("/etc/os-release");
    std::string line;
    while (std::getline(osRelease, line)) {
        constexpr std::string_view key = "PRETTY_NAME=";
        if (!line.starts_with(key)) {
            continue;
        }

        auto value = line.substr(key.size());
        if (value.size() >= 2 && value.front() == '"' && value.back() == '"') {
            value = value.substr(1, value.size() - 2);
        }
        return value;
    }

    return {};
}

std::string compileTimeArchitecture()
{
# if defined(__x86_64__) || defined(_M_X64)
    return "x86_64";
# elif defined(__aarch64__) || defined(_M_ARM64)
    return "arm64";
# elif defined(__i386__) || defined(_M_IX86)
    return "i386";
# elif defined(__arm__) || defined(_M_ARM)
    return "arm";
# else
    return {};
# endif
}

std::string currentArchitecture()
{
# if defined(FC_OS_WIN32)
    return compileTimeArchitecture();
# else
    struct utsname info {};
    if (uname(&info) == 0) {
        return info.machine;
    }
    return compileTimeArchitecture();
# endif
}
#endif

}

std::string ProgramInformation::prettyProductInfoWrapper()
{
#if defined(FREECAD_BUILD_QT) && FREECAD_BUILD_QT
    auto productName = QSysInfo::prettyProductName();
#ifdef FC_OS_MACOSX
    auto macosVersionFile = QStringLiteral(
        "/System/Library/CoreServices/.SystemVersionPlatform.plist"
    );
    auto fi = QFileInfo(macosVersionFile);
    if (fi.exists() && fi.isReadable()) {
        auto plistFile = QFile(macosVersionFile);
        plistFile.open(QIODevice::ReadOnly);
        while (!plistFile.atEnd()) {
            auto line = plistFile.readLine();
            if (line.contains("ProductUserVisibleVersion")) {
                auto nextLine = plistFile.readLine();
                if (nextLine.contains("<string>")) {
                    QRegularExpression re(QStringLiteral("\\s*<string>(.*)</string>"));
                    auto matches = re.match(QString::fromUtf8(nextLine));
                    if (matches.hasMatch()) {
                        productName = QStringLiteral("macOS ") + matches.captured(1);
                        break;
                    }
                }
            }
        }
    }
#endif
#ifdef FC_OS_WIN64
    QSettings regKey {
        QStringLiteral("HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion"),
        QSettings::NativeFormat
    };
    if (regKey.contains(QStringLiteral("CurrentBuildNumber"))) {
        auto buildNumber = regKey.value(QStringLiteral("CurrentBuildNumber")).toInt();
        if (buildNumber > 0) {
            if (buildNumber < 9200) {
                productName = QStringLiteral("Windows 7 build %1").arg(buildNumber);
            }
            else if (buildNumber < 10240) {
                productName = QStringLiteral("Windows 8 build %1").arg(buildNumber);
            }
            else if (buildNumber < 22000) {
                productName = QStringLiteral("Windows 10 build %1").arg(buildNumber);
            }
            else {
                productName = QStringLiteral("Windows 11 build %1").arg(buildNumber);
            }
        }
    }
#endif
    return productName.toStdString();
#else
# if defined(FC_OS_LINUX)
    if (auto prettyName = readLinuxPrettyName(); !prettyName.empty()) {
        return prettyName;
    }
# endif
# if defined(FC_OS_WIN32)
    return "Windows";
# elif defined(FC_OS_MACOSX)
    return "macOS";
# elif defined(FC_OS_BSD)
    return "BSD";
# else
    struct utsname info {};
    if (uname(&info) == 0) {
        return std::string(info.sysname) + " " + info.release;
    }
    return "Unknown";
# endif
#endif
}

static std::string getModuleInfoString(const std::string& path)
{
    const fs::path modPath(path);
    const auto fileName = displayNameFromPath(modPath);
    if (fileName.empty() || fileName.starts_with(".")) {  // Ignore hidden directories
        return {};
    }

    std::string addonName = fileName;
    std::string versionString;
    std::stringstream str;
    try {
        auto metadataFile = modPath / "package.xml";
        std::error_code error;
        if (std::filesystem::exists(metadataFile, error) && !error) {
            App::Metadata metadata(metadataFile);
            if (!metadata.name().empty()) {
                addonName = metadata.name();
            }
            if (metadata.version() != App::Meta::Version()) {
                versionString = " " + metadata.version().str();
            }
        }
    }
    catch (const Base::Exception& e) {
        str << " (Malformed metadata: " << cleanLine(e.what()) << ")";
    }
    str << "  * " << addonName << versionString;
    std::error_code error;
    if (fs::exists(modPath / "ADDON_DISABLED", error) && !error) {
        str << " (Disabled)";
    }

    str << "\n";
    return str.str();
}

std::string ProgramInformation::getValueOrEmpty(
    const std::map<std::string, std::string>& map,
    const std::string& key)
{
    auto it = map.find(key);
    return (it != map.end()) ? it->second : std::string();
}

void ProgramInformation::getVerboseCommonInfo(
    std::stringstream& str,
    const std::map<std::string, std::string>& mConfig)
{
    getSystemInformation(str);
    getVersionInformation(mConfig, str);
    getPackageInformation(str);
    getBuildInformation(mConfig, str);
    getLibraryVersions(str);
    getLocale(str);
}

void ProgramInformation::getSystemInformation(std::stringstream& str)
{
#if defined(FREECAD_BUILD_QT) && FREECAD_BUILD_QT
    auto sysenv = QProcessEnvironment::systemEnvironment();
    const QString deskEnv = sysenv.value(QStringLiteral("XDG_CURRENT_DESKTOP"));
    const QString deskSess = sysenv.value(QStringLiteral("DESKTOP_SESSION"));

    QStringList deskInfoList;
    QString deskInfo;

    if (!deskEnv.isEmpty()) {
        deskInfoList.append(deskEnv);
    }
    if (!deskSess.isEmpty()) {
        deskInfoList.append(deskSess);
    }

    const QString sysType = QSysInfo::productType();
    if (sysType != QLatin1String("windows") && sysType != QLatin1String("macos")) {
        QString sessionType = sysenv.value(QStringLiteral("XDG_SESSION_TYPE"));
        if (sessionType == QLatin1String("x11")) {
            sessionType = QStringLiteral("xcb");
        }
        deskInfoList.append(sessionType);
    }
    if (!deskInfoList.isEmpty()) {
        deskInfo = QLatin1String(" (") + deskInfoList.join(QLatin1String("/")) + QLatin1String(")");
    }

    str << "OS: " << prettyProductInfoWrapper() << deskInfo << '\n';
    if (QSysInfo::buildCpuArchitecture() == QSysInfo::currentCpuArchitecture()) {
        str << "Architecture: " << QSysInfo::buildCpuArchitecture() << "\n";
    }
    else {
        str << "Architecture: " << QSysInfo::buildCpuArchitecture()
            << "(running on: " << QSysInfo::currentCpuArchitecture() << ")\n";
    }
#else
    std::vector<std::string> deskInfoList;
    appendIfNotEmpty(deskInfoList, getenvString("XDG_CURRENT_DESKTOP").value_or(std::string()));
    appendIfNotEmpty(deskInfoList, getenvString("DESKTOP_SESSION").value_or(std::string()));

    auto sessionType = getenvString("XDG_SESSION_TYPE").value_or(std::string());
    if (sessionType == "x11") {
        sessionType = "xcb";
    }
    appendIfNotEmpty(deskInfoList, std::move(sessionType));

    const auto deskInfo = deskInfoList.empty()
        ? std::string()
        : " (" + joinStrings(deskInfoList, "/") + ")";

    str << "OS: " << prettyProductInfoWrapper() << deskInfo << '\n';
    const auto architecture = currentArchitecture();
    str << "Architecture: " << (architecture.empty() ? "Unknown" : architecture) << "\n";
#endif
}

void ProgramInformation::getPackageInformation(std::stringstream& str)
{
#ifdef FC_CONDA
    str << " Conda";
#endif
#ifdef FC_FLATPAK
    str << " Flatpak";
#endif
    if (getenvString("APPIMAGE")) {
        str << " AppImage";
    }
    if (const auto snap = getenvString("SNAP_REVISION")) {
        str << " Snap " << *snap;
    }
    str << '\n';
}

void ProgramInformation::getVersionInformation(
    const std::map<std::string, std::string>& mConfig,
    std::stringstream& str)
{
    const auto major = getValueOrEmpty(mConfig, "BuildVersionMajor");
    const auto minor = getValueOrEmpty(mConfig, "BuildVersionMinor");
    const auto point = getValueOrEmpty(mConfig, "BuildVersionPoint");
    const auto suffix = getValueOrEmpty(mConfig, "BuildVersionSuffix");
    const auto build = getValueOrEmpty(mConfig, "BuildRevision");
    str << "Version: " << major << "." << minor << "." << point << suffix << "." << build;
}

void ProgramInformation::getBuildInformation(
    const std::map<std::string, std::string>& mConfig,
    std::stringstream& str)
{
    const auto buildDate = getValueOrEmpty(mConfig, "BuildRevisionDate");
    str << "Build date: " << buildDate << "\n";

#if defined(_DEBUG) || defined(DEBUG)
    str << "Build type: Debug\n";
#elif defined(NDEBUG)
    str << "Build type: Release\n";
#elif defined(CMAKE_BUILD_TYPE)
    str << "Build type: " << CMAKE_BUILD_TYPE << '\n';
#else
    str << "Build type: Unknown\n";
#endif
    const auto buildRevisionBranch = getValueOrEmpty(mConfig, "BuildRevisionBranch");
    if (!buildRevisionBranch.empty()) {
        str << "Branch: " << buildRevisionBranch << '\n';
    }
    const auto buildRevisionHash = getValueOrEmpty(mConfig, "BuildRevisionHash");
    if (!buildRevisionHash.empty()) {
        str << "Hash: " << buildRevisionHash << '\n';
    }
}

void ProgramInformation::getLibraryVersions(std::stringstream& str)
{
    // report also the version numbers of the most important libraries in FreeCAD
    str << "Python " << PY_VERSION << ", ";
#if defined(FREECAD_BUILD_QT) && FREECAD_BUILD_QT
    str << "Qt " << QT_VERSION_STR << ", ";
    str << "Coin " << fcCoin3dVersion;
    if (*fcCoin3dSource) {
        str << " (" << fcCoin3dSource << ")";
    }
    str << ", ";
    str << "Pivy " << fcPivyVersion;
    if (*fcPivySource) {
        str << " (" << fcPivySource << ")";
    }
    str << ", ";
#else
    str << "Coin " << fcCoin3dVersion << ", ";
#endif
    str << "Vtk " << fcVtkVersion << ", ";
    str << "boost " << BOOST_LIB_VERSION << ", ";
    str << "Eigen3 " << fcEigen3Version;
#if defined(FREECAD_BUILD_QT) && FREECAD_BUILD_QT
    str << ", ";
    str << "PySide " << fcPysideVersion << '\n';
    str << "shiboken " << fcShibokenVersion << ", ";
#else
    str << '\n';
#endif
#ifdef SMESH_VERSION_STR
    str << "SMESH " << SMESH_VERSION_STR << ", ";
#endif
    str << "xerces-c " << fcXercescVersion << ", ";
    str << "Clipper2 " << fcClipper2Version << ", ";
    getIfcInfo(str);
#if defined(OCC_VERSION_STRING_EXT)
    str << "OCC " << OCC_VERSION_STRING_EXT << '\n';
#endif
}

void ProgramInformation::getIfcInfo(std::stringstream& str)
{
    try {
        Base::PyGILStateLocker lock;
        Py::Module module(PyImport_ImportModule("ifcopenshell"), true);
        if (!module.isNull() && module.hasAttr("version")) {
            Py::String version(module.getAttr("version"));
            auto ver_str = static_cast<std::string>(version);
            str << "IfcOpenShell " << ver_str << ", ";
        }
        else {
            Base::Console().log("Module 'ifcopenshell' not found (safe to ignore, unless using "
                                "the BIM workbench and IFC).\n");
        }
    }
    catch (const Py::Exception&) {
        Base::PyGILStateLocker lock;
        Base::PyException e;
        Base::Console().log("%s\n", e.what());
    }
}

void ProgramInformation::getLocale(std::stringstream& str)
{
#if defined(FREECAD_BUILD_QT) && FREECAD_BUILD_QT
    QLocale loc;
    str << "Locale: " << QLocale::languageToString(loc.language()) << "/"
#if QT_VERSION < QT_VERSION_CHECK(6, 6, 0)
        << QLocale::countryToString(loc.country())
#else
        << QLocale::territoryToString(loc.territory())
#endif
        << " (" << loc.name() << ")";
    if (loc != QLocale::system()) {
        loc = QLocale::system();
        str << " [ OS: " << QLocale::languageToString(loc.language()) << "/"
#if QT_VERSION < QT_VERSION_CHECK(6, 6, 0)
            << QLocale::countryToString(loc.country())
#else
            << QLocale::territoryToString(loc.territory())
#endif
            << " (" << loc.name() << ") ]";
    }
    str << "\n";
#else
    auto locale = getenvString("LC_ALL").value_or(std::string());
    if (locale.empty()) {
        locale = getenvString("LC_MESSAGES").value_or(std::string());
    }
    if (locale.empty()) {
        locale = getenvString("LANG").value_or(std::string());
    }
    if (locale.empty()) {
        locale = "C";
    }
    str << "Locale: " << locale << "\n";
#endif
}

void ProgramInformation::getVerboseAddOnsInfo(
    std::stringstream& str,
    const std::map<std::string, std::string>& mConfig)
{
    // Add installed module information:
    const auto modDir = fs::path(Application::getUserAppDataDir()) / "Mod";
    std::vector<std::string> addons;
    std::error_code error;
    if (fs::exists(modDir, error) && !error && fs::is_directory(modDir, error) && !error) {
        auto iterator =
            fs::directory_iterator(modDir, fs::directory_options::skip_permission_denied, error);
        for (auto it = fs::begin(iterator); it != fs::end(iterator); it.increment(error)) {
            if (error) {
                error.clear();
                continue;
            }
            const auto& mod = *it;
            std::error_code modError;
            if (!mod.is_directory(modError) || modError) {
                continue;  // Ignore files, only show directories
            }
            auto dirName = mod.path().string();
            auto moduleInfo = getModuleInfoString(dirName);
            if (!moduleInfo.empty()) {
                addons.push_back(std::move(moduleInfo));
            }
        }
    }
    const auto additionalModules = getValueOrEmpty(mConfig, "AdditionalModulePaths");

    if (!additionalModules.empty()) {
        boost::char_separator<char> sep(";");
        boost::tokenizer<boost::char_separator<char>> mods(additionalModules, sep);
        for (const auto& mod : mods) {
            auto moduleInfo = getModuleInfoString(mod);
            if (!moduleInfo.empty()) {
                addons.push_back(std::move(moduleInfo));
            }
        }
    }

    std::sort(addons.begin(), addons.end());
    if (!addons.empty()) {
        str << "Installed mods:\n";
        for (const auto& addon : addons) {
            str << addon;
        }
    }
}
