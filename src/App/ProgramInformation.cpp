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

#include <filesystem>
#include <algorithm>
#include <vector>
#include <boost/version.hpp>
#include <boost/tokenizer.hpp>
#include <QDir>
#include <QFileInfo>
#include <QLocale>
#include <QProcessEnvironment>
#include <QRegularExpression>
#include <QRegularExpressionMatch>
#include <QSettings>

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

std::ostream& operator<<(std::ostream& os, const QString& str)
{
    os << str.toStdString();
    return os;
}

}

std::string ProgramInformation::prettyProductInfoWrapper()
{
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
}

static std::string getModuleInfoString(const std::string& path)
{
    QString modPath = QString::fromStdString(path);
    QFileInfo mod(modPath);
    if (mod.isHidden()) {  // Ignore hidden directories
        return {};
    }

    std::string addonName = mod.isDir() ? QDir(modPath).dirName().toStdString()
                                        : mod.fileName().toStdString();
    std::string versionString;
    std::stringstream str;
    try {
        auto metadataFile = std::filesystem::path(mod.absoluteFilePath().toStdString())
            / "package.xml";
        if (std::filesystem::exists(metadataFile)) {
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
        auto what = QString::fromUtf8(e.what()).trimmed().replace(
            QChar::fromLatin1('\n'),
            QChar::fromLatin1(' ')
        );
        str << " (Malformed metadata: " << what << ")";
    }
    str << "  * " << addonName << versionString;
    QFileInfo disablingFile(mod.absoluteFilePath(), QStringLiteral("ADDON_DISABLED"));
    if (disablingFile.exists()) {
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
}

void ProgramInformation::getPackageInformation(std::stringstream& str)
{
#ifdef FC_CONDA
    str << " Conda";
#endif
#ifdef FC_FLATPAK
    str << " Flatpak";
#endif
    auto sysenv = QProcessEnvironment::systemEnvironment();
    const QString appimage = sysenv.value(QStringLiteral("APPIMAGE"));
    if (!appimage.isEmpty()) {
        str << " AppImage";
    }
    const QString snap = sysenv.value(QStringLiteral("SNAP_REVISION"));
    if (!snap.isEmpty()) {
        str << " Snap " << snap;
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
    str << "Qt " << QT_VERSION_STR << ", ";
    str << "Coin " << fcCoin3dVersion << ", ";
    str << "Vtk " << fcVtkVersion << ", ";
    str << "boost " << BOOST_LIB_VERSION << ", ";
    str << "Eigen3 " << fcEigen3Version << ", ";
    str << "PySide " << fcPysideVersion << '\n';
    str << "shiboken " << fcShibokenVersion << ", ";
#ifdef SMESH_VERSION_STR
    str << "SMESH " << SMESH_VERSION_STR << ", ";
#endif
    str << "xerces-c " << fcXercescVersion << ", ";
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
}

void ProgramInformation::getVerboseAddOnsInfo(
    std::stringstream& str,
    const std::map<std::string, std::string>& mConfig)
{
    // Add installed module information:
    const auto modDir = fs::path(Application::getUserAppDataDir()) / "Mod";
    std::vector<std::string> addons;
    if (fs::exists(modDir) && fs::is_directory(modDir)) {
        for (const auto& mod : fs::directory_iterator(modDir)) {
            if (!fs::is_directory(mod)) {
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
