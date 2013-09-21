/***************************************************************************
 *   Copyright (c) 2007 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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


#include "PreCompiled.h"

#include <Interface_Static.hxx>

#include <Base/Parameter.h>
#include <App/Application.h>

#include "DlgSettingsGeneral.h"
#include "ui_DlgSettingsGeneral.h"

using namespace PartGui;

DlgSettingsGeneral::DlgSettingsGeneral(QWidget* parent)
  : PreferencePage(parent)
{
    ui = new Ui_DlgSettingsGeneral();
    ui->setupUi(this);
}

/** 
 *  Destroys the object and frees any allocated resources
 */
DlgSettingsGeneral::~DlgSettingsGeneral()
{
    // no need to delete child widgets, Qt does it all for us
    delete ui;
}

void DlgSettingsGeneral::saveSettings()
{
    int unit = ui->comboBoxUnits->currentIndex();
    Base::Reference<ParameterGrp> hGrp = App::GetApplication().GetUserParameter()
        .GetGroup("BaseApp")->GetGroup("Preferences")->GetGroup("Mod/Part");
    hGrp->SetInt("Unit", unit);
    switch (unit) {
        case 1:
            Interface_Static::SetCVal("write.iges.unit","M");
            Interface_Static::SetCVal("write.step.unit","M");
            break;
        case 2:
            Interface_Static::SetCVal("write.iges.unit","IN");
            Interface_Static::SetCVal("write.step.unit","IN");
            break;
        default:
            Interface_Static::SetCVal("write.iges.unit","MM");
            Interface_Static::SetCVal("write.step.unit","MM");
            break;
    }
    ui->checkBooleanCheck->onSave();
    ui->checkBooleanRefine->onSave();
    ui->checkSketchBaseRefine->onSave();
}

void DlgSettingsGeneral::loadSettings()
{
    Base::Reference<ParameterGrp> hGrp = App::GetApplication().GetUserParameter()
        .GetGroup("BaseApp")->GetGroup("Preferences")->GetGroup("Mod/Part");
    int unit = hGrp->GetInt("Unit", 0);
    ui->comboBoxUnits->setCurrentIndex(unit);
    ui->checkBooleanCheck->onRestore();
    ui->checkBooleanRefine->onRestore();
    ui->checkSketchBaseRefine->onRestore();
}

/**
 * Sets the strings of the subwidgets using the current language.
 */
void DlgSettingsGeneral::changeEvent(QEvent *e)
{
    if (e->type() == QEvent::LanguageChange) {
        ui->retranslateUi(this);
    }
    else {
        QWidget::changeEvent(e);
    }
}
#include "moc_DlgSettingsGeneral.cpp"

