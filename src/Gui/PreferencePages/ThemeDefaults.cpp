// SPDX-License-Identifier: LGPL-2.1-or-later
/***************************************************************************
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
#include <map>
#include <set>

#include <QFileInfo>
#include <QString>
#include <QWidget>

#include <App/Application.h>
#include <Base/Console.h>
#include <Base/FileInfo.h>
#include <Base/Parameter.h>
#include "Application.h"
#include "PreferencePackManager.h"
#include "PrefWidgets.h"

#include "ThemeDefaults.h"

namespace Gui::ThemeDefaults
{
namespace
{
ParameterGrp::handle userGroup(const std::string& groupPath)
{
    return App::GetApplication().GetParameterGroupByPath(("User parameter:" + groupPath).c_str());
}

// Mirrors DlgSettingsGeneral::loadSettings(): an unset Theme resolves to the pack matching
// the active stylesheet, otherwise to the Classic pack.
std::string resolveThemeName()
{
    auto hMain = userGroup("BaseApp/Preferences/MainWindow");
    std::string theme = hMain->GetASCII("Theme", "");
    if (!theme.empty()) {
        return theme;
    }

    const QString styleSheet
        = QFileInfo(QString::fromStdString(hMain->GetASCII("StyleSheet", ""))).baseName();
    QString classic;
    QString similar;
    for (const auto& [name, pack] : Application::Instance->prefPackManager()->preferencePacks()) {
        if (pack.metadata().type() != "Theme") {
            continue;
        }
        const QString packName = QString::fromStdString(name);
        if (packName.contains(QStringLiteral("classic"), Qt::CaseInsensitive)) {
            classic = packName;
        }
        if (!styleSheet.isEmpty() && packName.contains(styleSheet, Qt::CaseInsensitive)) {
            similar = packName;
        }
    }
    return (!similar.isEmpty() ? similar : classic).toStdString();
}

// Loads the active theme's pack, or returns an invalid reference if there is none.
Base::Reference<ParameterManager> loadThemePack()
{
    const std::string theme = resolveThemeName();
    if (theme.empty()) {
        return {};
    }
    std::filesystem::path cfg = std::filesystem::path(App::Application::getResourceDir()) / "Gui"
        / "PreferencePacks" / theme / (theme + ".cfg");
    if (!std::filesystem::exists(cfg)) {
        return {};
    }
    auto params = ParameterManager::Create();
    params->LoadDocument(Base::FileInfo::pathToString(cfg).c_str());
    return params;
}

}  // namespace

void applyColors(const std::string& groupPath, const std::vector<std::string>& keys)
{
    auto themeParams = loadThemePack();
    if (!themeParams.isValid()) {
        return;
    }
    auto src = themeParams->GetGroup(groupPath.c_str());
    auto dst = userGroup(groupPath);
    const std::set<std::string> wanted(keys.begin(), keys.end());
    for (const auto& [name, value] : src->GetUnsignedMap()) {
        if (wanted.contains(name)) {
            dst->SetUnsigned(name.c_str(), value);
        }
    }
}

void applyBools(const std::string& groupPath, const std::vector<std::string>& keys)
{
    auto themeParams = loadThemePack();
    if (!themeParams.isValid()) {
        return;
    }
    auto src = themeParams->GetGroup(groupPath.c_str());
    auto dst = userGroup(groupPath);
    const std::set<std::string> wanted(keys.begin(), keys.end());
    for (const auto& [name, value] : src->GetBoolMap()) {
        if (wanted.contains(name)) {
            dst->SetBool(name.c_str(), value);
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

void applyWidgetColors(QWidget* page)
{
    std::map<std::string, std::vector<std::string>> byGroup;
    for (auto* button : page->findChildren<PrefColorButton*>()) {
        const QByteArray path = button->paramGrpPath();
        const QByteArray entry = button->entryName();
        if (!path.isEmpty() && !entry.isEmpty()) {
            byGroup["BaseApp/Preferences/" + path.toStdString()].push_back(entry.toStdString());
        }
    }
    for (const auto& [group, keys] : byGroup) {
        applyColors(group, keys);
    }
}

void applyStrings(const std::string& groupPath, const std::vector<std::string>& keys)
{
    auto themeParams = loadThemePack();
    if (!themeParams.isValid()) {
        return;
    }
    auto src = themeParams->GetGroup(groupPath.c_str());
    auto dst = userGroup(groupPath);
    const std::set<std::string> wanted(keys.begin(), keys.end());
    for (const auto& [name, value] : src->GetASCIIMap()) {
        if (wanted.contains(name)) {
            dst->SetASCII(name.c_str(), value.c_str());
        }
    }
}
}  // namespace Gui::ThemeDefaults
