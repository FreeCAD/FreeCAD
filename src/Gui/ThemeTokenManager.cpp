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

#include "PreCompiled.h"

#include "ThemeTokenManager.h"

#include <QString>
#include <ranges>

Gui::ThemeTokenManager::ThemeTokenManager()
{
    reload();
}

void Gui::ThemeTokenManager::reload()
{
    _tokens = {};
    _resolved = {};

    const auto addUserColor = [this](const std::string& name, unsigned long fallback) {
        unsigned long color = hGrp->GetUnsigned(name.c_str(), fallback);
        QString hexColor =
            QStringLiteral("#%1").arg(color, 8, 16, QLatin1Char('0')).toUpper().mid(0, 7);

        _tokens[name] = hexColor.toStdString();
    };

    for (const auto& [token, value] : hTokensGrp->GetASCIIMap()) {
        _tokens[token] = value;
    }

    addUserColor("ThemeAccentColor1", 0);
    addUserColor("ThemeAccentColor2", 0);
    addUserColor("ThemeAccentColor3", 0);
}

std::string Gui::ThemeTokenManager::get(const std::string& token)
{
    std::set<std::string> visited;
    return get(token, visited);
}

std::string Gui::ThemeTokenManager::get(const std::string& token, std::set<std::string>& visited)
{
    if (_resolved.contains(token)) {
        return _resolved.at(token);
    }

    if (visited.contains(token)) {
        Base::Console().Warning("The token '%s' contains circular-reference.", token);
        return "";
    }

    std::string value = _tokens.at(token);

    if (isReference(token)) {
        const std::string referenced = token.substr(1);

        visited.insert(token);
        return get(referenced, visited);
    }

    _resolved[token] = value;
    return value;
}

bool Gui::ThemeTokenManager::isReference(const std::string& token)
{
    return !token.empty() && token[0] == '@';
}