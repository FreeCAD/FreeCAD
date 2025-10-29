// SPDX-License-Identifier: LGPL-2.1-or-later
/****************************************************************************
 *                                                                          *
 *   Copyright (c) 2026 Chris Jones <chris.r.jones.1983@gmail.com>          *
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

#include <filesystem>
#include <set>

#include <App/Application.h>
#include <Base/Console.h>
#include <Base/FileInfo.h>
#include <Base/Parameter.h>

#include "ThemeDefaults.h"

namespace Gui::ThemeDefaults
{
namespace
{
ParameterGrp::handle userGroup(const std::string& groupPath)
{
    return App::GetApplication().GetParameterGroupByPath(("User parameter:" + groupPath).c_str());
}

}  // namespace

void applyColors(const std::string& groupPath, const std::vector<std::string>& keys)
{
    auto hMain = userGroup("BaseApp/Preferences/MainWindow");
    std::string theme = hMain->GetASCII("Theme", "");
    if (theme.empty()) {
        return;
    }

    std::filesystem::path cfg = std::filesystem::path(App::Application::getResourceDir()) / "Gui"
        / "PreferencePacks" / theme / (theme + ".cfg");
    if (!std::filesystem::exists(cfg)) {
#ifdef FC_DEBUG
        Base::Console().warning("ThemeDefaults: no config for theme '%s'\n", theme.c_str());
#endif
        return;
    }

    auto themeParams = ParameterManager::Create();
    themeParams->LoadDocument(Base::FileInfo::pathToString(cfg).c_str());

    auto src = themeParams->GetGroup(groupPath.c_str());
    auto dst = userGroup(groupPath);

    const std::set<std::string> wanted(keys.begin(), keys.end());
    for (const auto& [name, value] : src->GetUnsignedMap()) {
        if (wanted.contains(name)) {
            dst->SetUnsigned(name.c_str(), value);
        }
    }
}

void removeColors(const std::string& groupPath, const std::vector<std::string>& keys)
{
    auto grp = userGroup(groupPath);
    for (const auto& key : keys) {
        grp->RemoveUnsigned(key.c_str());
    }
}

}  // namespace Gui::ThemeDefaults
