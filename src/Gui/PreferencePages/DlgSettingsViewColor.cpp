/***************************************************************************
 *   Copyright (c) 2009 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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

#include <filesystem>
#include <vector>

#include <QPushButton>

#include <App/Application.h>
#include <Base/Console.h>

#include "DlgSettingsViewColor.h"
#include "ui_DlgSettingsViewColor.h"


using namespace Gui::Dialog;

/* TRANSLATOR Gui::Dialog::DlgSettingsViewColor */

/**
 *  Constructs a DlgSettingsViewColor which is a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'
 */
DlgSettingsViewColor::DlgSettingsViewColor(QWidget* parent)
    : PreferencePage(parent)
    , ui(new Ui_DlgSettingsViewColor)
{
    // clang-format off
    ui->setupUi(this);
    connect(ui->SwitchGradientColors, &QPushButton::pressed, this,
        &DlgSettingsViewColor::onSwitchGradientColorsPressed);

    connect(ui->radioButtonSimple, &QRadioButton::toggled, this,
        &DlgSettingsViewColor::onRadioButtonSimpleToggled);

    connect(ui->radioButtonGradient, &QRadioButton::toggled, this,
        &DlgSettingsViewColor::onRadioButtonGradientToggled);

    connect(ui->rbRadialGradient, &QRadioButton::toggled, this,
        &DlgSettingsViewColor::onRadioButtonRadialGradientToggled);

    connect(ui->checkMidColor, &QCheckBox::toggled, this,
        &DlgSettingsViewColor::onCheckMidColorToggled);
    // clang-format on
}

/**
 *  Destroys the object and frees any allocated resources
 */
DlgSettingsViewColor::~DlgSettingsViewColor() = default;

void DlgSettingsViewColor::saveSettings()
{
    ui->SelectionColor_Background->onSave();
    ui->backgroundColorFrom->onSave();
    ui->backgroundColorTo->onSave();
    ui->backgroundColorMid->onSave();
    ui->radioButtonSimple->onSave();
    ui->radioButtonGradient->onSave();
    ui->rbRadialGradient->onSave();
    ui->checkMidColor->onSave();
    ui->TreeEditColor->onSave();
    ui->TreeActiveColor->onSave();
    ui->CbLabelColor->onSave();
    ui->CbLabelTextSize->onSave();
}

void DlgSettingsViewColor::loadSettings()
{
    ui->SelectionColor_Background->onRestore();
    ui->backgroundColorFrom->onRestore();
    ui->backgroundColorTo->onRestore();
    ui->backgroundColorMid->onRestore();
    ui->radioButtonSimple->onRestore();
    ui->radioButtonGradient->onRestore();
    ui->rbRadialGradient->onRestore();
    ui->checkMidColor->onRestore();
    ui->TreeEditColor->onRestore();
    ui->TreeActiveColor->onRestore();
    ui->CbLabelColor->onRestore();
    ui->CbLabelTextSize->onRestore();

    if (ui->radioButtonSimple->isChecked()) {
        onRadioButtonSimpleToggled(true);
    }
    else if (ui->radioButtonGradient->isChecked()) {
        onRadioButtonGradientToggled(true);
    }
    else {
        onRadioButtonRadialGradientToggled(true);
    }
}

void DlgSettingsViewColor::loadThemeDefaults()
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
    Base::Console().message("Loading View Colors defaults for theme: %s\n", themeName.c_str());
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
    auto themeViewGroup =
        themeParams->GetGroup("BaseApp")->GetGroup("Preferences")->GetGroup("View");

    // get the current user's View group
    auto userViewGroup =
        App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/View");

    // list of View color parameters to copy from theme
    std::vector<std::string> viewColors = {"BackgroundColor",
                                           "BackgroundColor2",
                                           "BackgroundColor3",
                                           "BackgroundColor4",
                                           "CbLabelColor"};

    // copy only the view colors from theme to user config
    for (const auto& colorName : viewColors) {
        unsigned long colorValue = themeViewGroup->GetUnsigned(colorName.c_str(), UINT_MAX);
        if (colorValue != UINT_MAX) {
            userViewGroup->SetUnsigned(colorName.c_str(), colorValue);
        }
    }

    // get the TreeView group from the theme config
    auto themeTreeViewGroup =
        themeParams->GetGroup("BaseApp")->GetGroup("Preferences")->GetGroup("TreeView");

    // get the current user's TreeView group
    auto userTreeViewGroup =
        App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/TreeView");

    // list of TreeView color parameters to copy from theme
    std::vector<std::string> treeViewColors = {"TreeEditColor", "TreeActiveColor"};

    // copy only the tree view colors from theme to user config
    for (const auto& colorName : treeViewColors) {
        unsigned long colorValue = themeTreeViewGroup->GetUnsigned(colorName.c_str(), UINT_MAX);
        if (colorValue != UINT_MAX) {
            userTreeViewGroup->SetUnsigned(colorName.c_str(), colorValue);
        }
    }
}

void DlgSettingsViewColor::resetSettingsToDefaults()
{
    // get parameter groups
    ParameterGrp::handle hView =
        App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/View");
    ParameterGrp::handle hTreeView = App::GetApplication().GetParameterGroupByPath(
        "User parameter:BaseApp/Preferences/TreeView");

    // remove all View color parameters so theme defaults can be applied fresh
    std::vector<std::string> viewColors = {"BackgroundColor",
                                           "BackgroundColor2",
                                           "BackgroundColor3",
                                           "BackgroundColor4",
                                           "CbLabelColor"};

    for (const auto& colorName : viewColors) {
        hView->RemoveUnsigned(colorName.c_str());
    }

    // remove all TreeView color parameters
    std::vector<std::string> treeViewColors = {"TreeEditColor", "TreeActiveColor"};

    for (const auto& colorName : treeViewColors) {
        hTreeView->RemoveUnsigned(colorName.c_str());
    }

    // reset all the parameters associated to Gui::Pref* widgets
    PreferencePage::resetSettingsToDefaults();

    // apply theme default colors
    loadThemeDefaults();

    // reload the settings to update the GUI
    loadSettings();
}

/**
 * Sets the strings of the subwidgets using the current language.
 */
void DlgSettingsViewColor::changeEvent(QEvent* e)
{
    if (e->type() == QEvent::LanguageChange) {
        ui->retranslateUi(this);
    }
    else {
        QWidget::changeEvent(e);
    }
}

void DlgSettingsViewColor::onSwitchGradientColorsPressed()
{
    QColor tempColor = ui->backgroundColorFrom->color();
    ui->backgroundColorFrom->setColor(ui->backgroundColorTo->color());
    ui->backgroundColorTo->setColor(tempColor);
}

void DlgSettingsViewColor::onCheckMidColorToggled(bool val)
{
    ui->color2Label->setEnabled(val);
    ui->backgroundColorMid->setEnabled(val);
}

void DlgSettingsViewColor::onRadioButtonSimpleToggled(bool val)
{
    setGradientColorVisibility(!val);
}

void DlgSettingsViewColor::onRadioButtonGradientToggled(bool val)
{
    setGradientColorVisibility(val);
    ui->color1Label->setText(tr("Top:"));
    ui->color2Label->setText(tr("Middle:"));
    ui->color3Label->setText(tr("Bottom:"));
}

void DlgSettingsViewColor::onRadioButtonRadialGradientToggled(bool val)
{
    setGradientColorVisibility(val);
    ui->color1Label->setText(tr("Central:"));
    ui->color2Label->setText(tr("Midway:"));
    ui->color3Label->setText(tr("End:"));
}

void DlgSettingsViewColor::setGradientColorVisibility(bool val)
{
    ui->SelectionColor_Background->setVisible(!val);
    ui->color1Label->setVisible(val);
    ui->backgroundColorFrom->setVisible(val);
    ui->color2Label->setVisible(val);
    ui->backgroundColorMid->setVisible(val);
    ui->color3Label->setVisible(val);
    ui->backgroundColorTo->setVisible(val);
    ui->checkMidColor->setVisible(val);
    ui->SwitchGradientColors->setVisible(val);

    if (val) {
        onCheckMidColorToggled(ui->checkMidColor->isChecked());
    }
}

#include "moc_DlgSettingsViewColor.cpp"
