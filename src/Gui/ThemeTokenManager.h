// SPDX-License-Identifier: LGPL-2.1-or-later
/****************************************************************************
 *                                                                          *
 *   Copyright (c) 2025 Kacper Donat <kacper@kadet.net>                     *
 *                                                                          *
 *   This file is part of FreeCAD.                                          *
 *                                                                          *
 *   FreeCAD is free software: you can redistribute it and/or modify it     *
 *   under the terms of the GNU Lesser General Public License as            *
 *   published by the Free Software Foundation, either version 2.1 of the   *
 *   License, or (at your option) any later version.                        *
 *                                                                          *
 *   FreeCAD is distributed in the hope that it will be useful, but         *
 *   WITHOUT ANY WARRANTY; without even the implied warranty of             *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU       *
 *   Lesser General Public License for more details.                        *
 *                                                                          *
 *   You should have received a copy of the GNU Lesser General Public       *
 *   License along with FreeCAD. If not, see                                *
 *   <https://www.gnu.org/licenses/>.                                       *
 *                                                                          *
 ***************************************************************************/

#ifndef THEMETOKENMANAGER_H
#define THEMETOKENMANAGER_H

#include <ranges>

#include <App/Application.h>
#include <Base/Parameter.h>

namespace Gui
{

class ThemeTokenManager
{
public:
    ThemeTokenManager();

    void reload();

    std::ranges::view auto tokens() const
    {
        return std::ranges::views::keys(_tokens);
    }

    std::string get(const std::string& token);

private:
    std::string get(const std::string& token, std::set<std::string>& visited);

    static bool isReference(const std::string& token);

    std::map<std::string, std::string> _tokens;
    std::map<std::string, std::string> _resolved;

    ParameterGrp::handle hGrp =
        App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/Themes");
    ParameterGrp::handle hTokensGrp =
        App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/Themes/Tokens");
};

}  // namespace Gui

#endif  // THEMETOKENMANAGER_H
