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

#include "ProgramInformation.h"

#include <QApplication>
#include <QScreen>
#include <QStyle>

#include <App/Application.h>
#include <App/ProgramInformation.h>

#include "FileDialog.h"

using namespace Gui;

void ProgramInformation::getStyleInformation(std::stringstream& str)
{
    ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath(
        "User parameter:BaseApp/Preferences/MainWindow"
    );

    // Add Stylesheet/Theme/Qtstyle information
    std::string styleSheet = hGrp->GetASCII("StyleSheet");
    std::string theme = hGrp->GetASCII("Theme");
#if QT_VERSION >= QT_VERSION_CHECK(6, 1, 0)
    std::string style = qApp->style()->name().toStdString();
#else
    std::string style = hGrp->GetASCII("QtStyle");
    if (style.empty()) {
        style = "Qt default";
    }
#endif
    if (styleSheet.empty()) {
        styleSheet = "unset";
    }
    if (theme.empty()) {
        theme = "unset";
    }

    str << "Stylesheet/Theme/QtStyle: " << styleSheet << "/" << theme << "/" << style << "\n";
}

void ProgramInformation::getNavigationStyleInformation(std::stringstream& str)
{
    ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath(
        "User parameter:BaseApp/Preferences/View"
    );

    const std::string navStyle = hGrp->GetASCII("NavigationStyle", "Gui::CADNavigationStyle");
    constexpr auto orbitStyle = std::to_array<std::string_view>(
        {"Turntable", "Trackball", "Free Turntable", "Trackball Classic", "Rounded Arcball"}
    );
    constexpr auto rotMode = std::to_array<std::string_view>(
        {"Window center", "Drag at cursor", "Object center"}
    );
    // All navigation styles are named on the format "Gui::<Name>NavigationStyle"
    // so we remove the "Gui::" prefix and the "NavigationStyle" suffix before printing.
    constexpr auto pLen = std::string_view("Gui::").length();
    constexpr auto sLen = std::string_view("NavigationStyle").length();
    str << "Navigation Style/Orbit Style/Rotation Mode: "
        << navStyle.substr(pLen, navStyle.length() - sLen - pLen) << "/"
        << orbitStyle[hGrp->GetInt("OrbitStyle", 4)] << "/"
        << rotMode[hGrp->GetInt("RotationMode", 0)] << "\n";
}

void ProgramInformation::getDpiInformation(std::stringstream& str)
{
    // Add DPI information
    str << "Logical DPI/Physical DPI/Pixel Ratio: "
        << QApplication::primaryScreen()->logicalDotsPerInch() << "/"
        << QApplication::primaryScreen()->physicalDotsPerInch() << "/"
        << QApplication::primaryScreen()->devicePixelRatio() << "\n";
}

void ProgramInformation::getQtBackendInformation(std::stringstream& str)
{
    constexpr auto dialogBackendToString = [](DialogOptions::Backend backend) {
        switch (backend) {
            case DialogOptions::Backend::NonNative:
                return "non-native";
            case DialogOptions::Backend::ViaQt:
                return "via Qt";
            case DialogOptions::Backend::ViaWin32:
                return "via Win32";
            default:
                return "?";
        }
    };
    str << "QPA/File dialog/Color dialog: " << QApplication::platformName().toStdString() << "/"
        << dialogBackendToString(DialogOptions::fileDialogBackend()) << "/"
        << dialogBackendToString(DialogOptions::colorDialogBackend()) << "\n";
}

std::string ProgramInformation::collect(const std::map<std::string, std::string>& config)
{
    std::stringstream str;
    App::ProgramInformation::getVerboseCommonInfo(str, config);
    Gui::ProgramInformation::getStyleInformation(str);
    Gui::ProgramInformation::getQtBackendInformation(str);
    Gui::ProgramInformation::getNavigationStyleInformation(str);
    Gui::ProgramInformation::getDpiInformation(str);
    App::ProgramInformation::getVerboseAddOnsInfo(str, config);
    return str.str();
}

std::string ProgramInformation::collect()
{
    return collect(App::Application::Config());
}
