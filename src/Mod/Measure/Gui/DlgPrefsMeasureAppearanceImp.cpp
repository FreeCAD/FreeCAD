// SPDX-License-Identifier: LGPL-2.1-or-later

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

#include <string>
#include <vector>
#include <App/Application.h>
#include <Base/Console.h>

#include <Gui/PreferencePages/ThemeDefaults.h>
#include "DlgPrefsMeasureAppearanceImp.h"
#include "ui_DlgPrefsMeasureAppearanceImp.h"

namespace
{
constexpr const char* measureGroup = "BaseApp/Preferences/Mod/Measure/Appearance";

const std::vector<std::string>& measureColors()
{
    static const std::vector<std::string> colors = {
        "DefaultTextColor",
        "DefaultLineColor",
        "DefaultTextBackgroundColor",
    };
    return colors;
}
}  // namespace

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
    ui->sbArrowRadius->onSave();
    ui->sbArrowHeight->onSave();
}

void DlgPrefsMeasureAppearanceImp::loadSettings()
{
    ui->sbFontSize->onRestore();
    ui->cbText->onRestore();
    ui->cbBackground->onRestore();
    ui->cbLine->onRestore();
    ui->sbArrowRadius->onRestore();
    ui->sbArrowHeight->onRestore();
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
    Gui::ThemeDefaults::applyColors(measureGroup, measureColors());
}

void DlgPrefsMeasureAppearanceImp::resetSettingsToDefaults()
{
    Gui::ThemeDefaults::removeColors(measureGroup, measureColors());

    PreferencePage::resetSettingsToDefaults();

    // theme colors are applied after the base reset, which clears Pref* widget params
    loadThemeDefaults();
    loadSettings();
}

#include <Mod/Measure/Gui/moc_DlgPrefsMeasureAppearanceImp.cpp>
