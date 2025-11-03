/**************************************************************************
 *   Copyright (c) 2023 Wanderer Fan <wandererfan@gmail.com>               *
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
#include <vector>
#include <App/Application.h>
#include <Base/Console.h>

#include "DlgPrefsMeasureAppearanceImp.h"
#include "ui_DlgPrefsMeasureAppearanceImp.h"

using namespace MeasureGui;

DlgPrefsMeasureAppearanceImp::DlgPrefsMeasureAppearanceImp(QWidget* parent)
    : PreferencePage(parent)
    , ui(new Ui_DlgPrefsMeasureAppearanceImp)
{
    ui->setupUi(this);
}

DlgPrefsMeasureAppearanceImp::~DlgPrefsMeasureAppearanceImp()
{
    // no need to delete child widgets, Qt does it all for us
}

void DlgPrefsMeasureAppearanceImp::saveSettings()
{
    ui->sbFontSize->onSave();
    ui->cbText->onSave();
    ui->cbLine->onSave();
    ui->cbBackground->onSave();
}

void DlgPrefsMeasureAppearanceImp::loadSettings()
{
    ui->sbFontSize->onRestore();
    ui->cbText->onRestore();
    ui->cbBackground->onRestore();
    ui->cbLine->onRestore();
}

/**
 * Sets the strings of the subwidgets using the current language.
 */
void DlgPrefsMeasureAppearanceImp::changeEvent(QEvent* e)
{
    if (e->type() == QEvent::LanguageChange) {
        ui->retranslateUi(this);
    }
    else {
        QWidget::changeEvent(e);
    }
}

void DlgPrefsMeasureAppearanceImp::loadThemeDefaults()
{
    // get current theme name
    ParameterGrp::handle hMainWindow = App::GetApplication().GetParameterGroupByPath(
        "User parameter:BaseApp/Preferences/MainWindow");
    std::string themeName = hMainWindow->GetASCII("Theme", "");

    if (themeName.empty()) {
#ifdef FC_DEBUG
        Base::Console().message("No theme set, skipping theme color defaults\n");
#endif
        return;
    }

#ifdef FC_DEBUG
    Base::Console().message("Loading Measure color defaults for theme: %s\n", themeName.c_str());
#endif

    // construct path to the theme's config file
    std::string appPath = App::Application::getResourceDir();
    std::filesystem::path configFile = std::filesystem::path(appPath) / "Gui" / "PreferencePacks"
        / themeName / (themeName + ".cfg");

    if (!std::filesystem::exists(configFile)) {
#ifdef FC_DEBUG
        Base::Console().warning("Config file not found for theme '%s'\n", themeName.c_str());
#endif
        return;
    }

    // load theme config into temporary parameters
    auto themeParams = ParameterManager::Create();
    themeParams->LoadDocument(Base::FileInfo::pathToString(configFile).c_str());

    // try to get the Measure/Appearance group from the theme config
    // Note: Not all themes may have Measure colors defined
    try {
        auto themeMeasureGroup = themeParams->GetGroup("BaseApp")
                                     ->GetGroup("Preferences")
                                     ->GetGroup("Mod")
                                     ->GetGroup("Measure")
                                     ->GetGroup("Appearance");

        // get the current user's Measure/Appearance group
        auto userMeasureGroup = App::GetApplication().GetParameterGroupByPath(
            "User parameter:BaseApp/Preferences/Mod/Measure/Appearance");

        // list of Measure color parameters to copy from theme
        std::vector<std::string> measureColors = {"DefaultTextColor",
                                                  "DefaultLineColor",
                                                  "DefaultTextBackgroundColor"};

        // copy only the Measure colors from theme to user config
        for (const auto& colorName : measureColors) {
            unsigned long colorValue = themeMeasureGroup->GetUnsigned(colorName.c_str(), UINT_MAX);
            if (colorValue != UINT_MAX) {
                userMeasureGroup->SetUnsigned(colorName.c_str(), colorValue);
            }
        }
    }
    catch (...) {
#ifdef FC_DEBUG
        Base::Console().message("Theme '%s' does not define Measure colors, using defaults\n",
                                themeName.c_str());
#endif
    }
}

void DlgPrefsMeasureAppearanceImp::resetSettingsToDefaults()
{
    // get parameter group
    ParameterGrp::handle hMeasureAppearance = App::GetApplication().GetParameterGroupByPath(
        "User parameter:BaseApp/Preferences/Mod/Measure/Appearance");

    // remove all color parameters so theme defaults can be applied fresh
    std::vector<std::string> measureColors = {"DefaultTextColor",
                                              "DefaultLineColor",
                                              "DefaultTextBackgroundColor"};

    for (const auto& colorName : measureColors) {
        hMeasureAppearance->RemoveUnsigned(colorName.c_str());
    }

    // remove non-color parameters
    hMeasureAppearance->RemoveInt("DefaultFontSize");

    // apply theme specific color defaults
    loadThemeDefaults();

    // reload the settings from parameter storage to update gui
    loadSettings();
}

#include <Mod/Measure/Gui/moc_DlgPrefsMeasureAppearanceImp.cpp>
