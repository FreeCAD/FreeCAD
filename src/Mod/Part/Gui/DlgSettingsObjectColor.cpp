// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2013 Werner Mayer <wmayer[at]users.sourceforge.net>     *
 *                                                                         *
 *   This file is part of the FreeCAD CAx development system.              *
 *                                                                         *
 *   This library is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU Library General Public           *
 *   License as published by the Free Software Foundation; either          *
 *   version 2 of the License, or (at your option) any later version.      *
 *                                                                         *
 *   This library  is distributed in the hope that it will be useful,      *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU Library General Public License for more details.                  *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this library; see the file COPYING.LIB. If not,    *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,         *
 *   Suite 330, Boston, MA  02111-1307, USA                                *
 *                                                                         *
 ***************************************************************************/

#include <map>
#include <filesystem>

#include <App/Application.h>
#include <Base/Console.h>

#include "DlgSettingsObjectColor.h"
#include "ui_DlgSettingsObjectColor.h"


using namespace PartGui;

/* TRANSLATOR PartGui::DlgSettingsObjectColor */

/**
 *  Constructs a DlgSettingsObjectColor which is a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'
 */
DlgSettingsObjectColor::DlgSettingsObjectColor(QWidget* parent)
    : PreferencePage(parent)
    , ui(new Ui_DlgSettingsObjectColor)
{
    ui->setupUi(this);
    ui->DefaultShapeColor->setDisabled(ui->checkRandomColor->isChecked());
}

/**
 *  Destroys the object and frees any allocated resources
 */
DlgSettingsObjectColor::~DlgSettingsObjectColor() = default;

void DlgSettingsObjectColor::saveSettings()
{
    // Part
    ui->DefaultShapeColor->onSave();
    ui->DefaultAmbientColor->onSave();
    ui->DefaultEmissiveColor->onSave();
    ui->DefaultSpecularColor->onSave();
    ui->checkRandomColor->onSave();
    ui->DefaultShapeTransparency->onSave();
    ui->DefaultShapeShininess->onSave();
    ui->DefaultShapeLineColor->onSave();
    ui->DefaultShapeLineWidth->onSave();
    ui->DefaultShapeVertexColor->onSave();
    ui->DefaultShapeVertexSize->onSave();
    ui->BoundingBoxColor->onSave();
    ui->BoundingBoxFontSize->onSave();
    ui->twosideRendering->onSave();
    // Annotations
    ui->AnnotationTextColor->onSave();
}

void DlgSettingsObjectColor::loadSettings()
{
    // Part
    ui->DefaultShapeColor->onRestore();
    ui->DefaultAmbientColor->onRestore();
    ui->DefaultEmissiveColor->onRestore();
    ui->DefaultSpecularColor->onRestore();
    ui->checkRandomColor->onRestore();
    ui->DefaultShapeTransparency->onRestore();
    ui->DefaultShapeShininess->onRestore();
    ui->DefaultShapeLineColor->onRestore();
    ui->DefaultShapeLineWidth->onRestore();
    ui->DefaultShapeVertexColor->onRestore();
    ui->DefaultShapeVertexSize->onRestore();
    ui->BoundingBoxColor->onRestore();
    ui->BoundingBoxFontSize->onRestore();
    ui->twosideRendering->onRestore();
    // Annotations
    ui->AnnotationTextColor->onRestore();
}

/**
 * Sets the strings of the subwidgets using the current language.
 */
void DlgSettingsObjectColor::changeEvent(QEvent *e)
{
    if (e->type() == QEvent::LanguageChange) {
        ui->retranslateUi(this);
    }
    else {
        QWidget::changeEvent(e);
    }
}

void DlgSettingsObjectColor::loadThemeDefaults()
{
    // get current theme name
    ParameterGrp::handle hMainWindow = App::GetApplication().GetParameterGroupByPath(
        "User parameter:BaseApp/Preferences/MainWindow");
    std::string themeName = hMainWindow->GetASCII("Theme", "");

    if (themeName.empty()) {
#ifdef FC_DEBUG
        Base::Console().message("No theme set, skipping theme color defaults\n");
#endif
        return;  // no theme active, use current colors
    }

#ifdef FC_DEBUG
    Base::Console().message("Loading Part color defaults for theme: %s\n", themeName.c_str());
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

    // get the view group from the theme config
    auto themeViewGroup = themeParams->GetGroup("BaseApp")->GetGroup("Preferences")->GetGroup("View");

    // get the current user's View group
    auto userViewGroup = App::GetApplication().GetParameterGroupByPath(
        "User parameter:BaseApp/Preferences/View");

    // list of Part/PartDesign color parameters to copy from theme
    std::vector<std::string> partColors = {
        "DefaultShapeColor",
        "DefaultAmbientColor",
        "DefaultEmissiveColor",
        "DefaultSpecularColor",
        "DefaultShapeLineColor",
        "DefaultShapeVertexColor",
        "BoundingBoxColor",
        "AnnotationTextColor"
    };

    // copy only the Part colors from theme to user config
    for (const auto& colorName : partColors) {
        unsigned long colorValue = themeViewGroup->GetUnsigned(colorName.c_str(), UINT_MAX);
        if (colorValue != UINT_MAX) {
            userViewGroup->SetUnsigned(colorName.c_str(), colorValue);
        }
    }
}

void DlgSettingsObjectColor::resetSettingsToDefaults()
{
    // get parameter groups
    ParameterGrp::handle hView = App::GetApplication().GetParameterGroupByPath(
        "User parameter:BaseApp/Preferences/View");
    ParameterGrp::handle hModPart = App::GetApplication().GetParameterGroupByPath(
        "User parameter:BaseApp/Preferences/Mod/Part");

    // remove all color parameters so theme defaults can be applied fresh
    std::vector<std::string> partColors = {
        "DefaultShapeColor",
        "DefaultAmbientColor",
        "DefaultEmissiveColor",
        "DefaultSpecularColor",
        "DefaultShapeLineColor",
        "DefaultShapeVertexColor",
        "BoundingBoxColor",
        "AnnotationTextColor"
    };

    for (const auto& colorName : partColors) {
        hView->RemoveUnsigned(colorName.c_str());
    }

    // remove non-color parameters from View group
    hView->RemoveBool("RandomColor");
    hView->RemoveInt("DefaultShapeTransparency");
    hView->RemoveInt("DefaultShapeShininess");
    hView->RemoveInt("DefaultShapeLineWidth");
    hView->RemoveInt("DefaultShapePointSize");
    hView->RemoveFloat("BoundingBoxFontSize");

    // remove Mod/Part parameters
    hModPart->RemoveBool("TwoSideRendering");

    // apply theme specific color defaults
    loadThemeDefaults();

    // reload the settings from parameter storage to update gui
    loadSettings();
}

#include "moc_DlgSettingsObjectColor.cpp"

