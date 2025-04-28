/***************************************************************************
 *   Copyright (c) 2023 David Carter <dcarter@david.carter.ca>             *
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

#include "PreCompiled.h"
#ifndef _PreComp_
#endif

#include <App/Application.h>

#include <Mod/Material/App/MaterialManager.h>
#include <Mod/Material/App/ModelManager.h>

#include "DlgSettingsExternal.h"
#include "ui_DlgSettingsExternal.h"


using namespace MatGui;

DlgSettingsExternal::DlgSettingsExternal(QWidget* parent)
    : PreferencePage(parent)
    , ui(new Ui_DlgSettingsExternal)
{
    ui->setupUi(this);
}

DlgSettingsExternal::~DlgSettingsExternal()
{}

void DlgSettingsExternal::saveSettings()
{
    ParameterGrp::handle hGrp =
        App::GetApplication().GetParameterGroupByPath(getPreferences().c_str());

    ui->spinModelCacheSize->onSave();
    ui->spinMaterialCacheSize->onSave();

    bool useExternal = ui->groupExternal->isChecked();
    hGrp->SetBool("UseExternal", useExternal);
    hGrp->SetASCII("Current", ui->comboInterface->currentText().toStdString());
}

QString DlgSettingsExternal::toPerCent(double value) const
{
    QString pcString;
    pcString.setNum(int(value * 100.0));
    pcString += QLatin1String("%");

    return pcString;
}

void DlgSettingsExternal::loadSettings()
{
    ParameterGrp::handle hGrp =
        App::GetApplication().GetParameterGroupByPath(getPreferences().c_str());

    loadInterfaces();

    bool useExternal = hGrp->GetBool("UseExternal", false);
    ui->groupExternal->setChecked(useExternal);

    auto cacheSize = hGrp->GetInt("ModelCacheSize", 100);
    ui->spinModelCacheSize->setValue(cacheSize);
    cacheSize = hGrp->GetInt("MaterialCacheSize", 100);
    ui->spinMaterialCacheSize->setValue(cacheSize);

    // Cache stats
    auto hitRate = Materials::ModelManager::modelHitRate();
    ui->inputModelCacheHitRate->setText(toPerCent(hitRate));
    hitRate = Materials::MaterialManager::materialHitRate();
    ui->inputMaterialCacheHitRate->setText(toPerCent(hitRate));
}

void DlgSettingsExternal::loadInterfaces()
{
    ParameterGrp::handle hGrp =
        App::GetApplication().GetParameterGroupByPath(getPreferencesInterfaces().c_str());

    ui->comboInterface->clear();
    ui->comboInterface->addItem(tr("None"));

    for (auto& group : hGrp->GetGroups()) {
        auto moduleName = QString::fromStdString(group->GetGroupName());

        ui->comboInterface->addItem(moduleName);
    }

    hGrp = App::GetApplication().GetParameterGroupByPath(getPreferences().c_str());
    auto current = hGrp->GetASCII("Current", "None");
    ui->comboInterface->setCurrentText(QString::fromStdString(current));
}

std::string DlgSettingsExternal::getPreferences() const
{
    return "User parameter:BaseApp/Preferences/Mod/Material/ExternalInterface";
}

std::string DlgSettingsExternal::getPreferencesInterfaces() const
{
    return "User parameter:BaseApp/Preferences/Mod/Material/ExternalInterface/Interfaces";
}

/**
 * Sets the strings of the subwidgets using the current language.
 */
void DlgSettingsExternal::changeEvent(QEvent* e)
{
    if (e->type() == QEvent::LanguageChange) {
        ui->retranslateUi(this);
    }
    else {
        QWidget::changeEvent(e);
    }
}

#include "moc_DlgSettingsExternal.cpp"
