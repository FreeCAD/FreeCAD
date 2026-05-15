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

#pragma once

#include <FCGlobal.h>

#include <map>
#include <sstream>
#include <string>

class QOpenGLWidget;

namespace Gui
{

class GuiExport ProgramInformation
{
public:
    static void getStyleInformation(std::stringstream& str);
    static void getNavigationStyleInformation(std::stringstream& str);
    static void getDpiInformation(std::stringstream& str);
    static void getDialogInformation(std::stringstream& str);
    static void initOpenGLInformation(QOpenGLWidget&);
    static void getOpenGLInformation(std::stringstream& str);

    static std::string collect(const std::map<std::string, std::string>& config);
    static std::string collect();
};

}  // namespace Gui
