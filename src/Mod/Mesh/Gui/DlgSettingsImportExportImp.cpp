/***************************************************************************
 *   Copyright (c) 2016 Ian Rees         <ian.rees@gmail.com>              *
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

#include "DlgSettingsImportExportImp.h"
#include "ui_DlgSettingsImportExport.h"
#include <App/Application.h>


using namespace MeshGui;

DlgSettingsImportExport::DlgSettingsImportExport(QWidget* parent)
  : PreferencePage(parent), ui(new Ui_DlgSettingsImportExport)
{
    ui->setupUi(this);
    ui->exportAmfCompressed->setToolTip(tr("This parameter indicates whether ZIP compression\n"
                                           "is used when writing a file in AMF format"));
}

DlgSettingsImportExport::~DlgSettingsImportExport()
{
    // no need to delete child widgets, Qt does it all for us
    delete ui;
}

void DlgSettingsImportExport::saveSettings()
{
    ParameterGrp::handle handle = App::GetApplication().GetParameterGroupByPath
        ("User parameter:BaseApp/Preferences/Mod/Mesh");
    double value = ui->maxDeviationExport->value().getValue();
    handle->SetFloat("MaxDeviationExport", value);
    
    ui->exportAmfCompressed->onSave();
}

void DlgSettingsImportExport::loadSettings()
{
    ParameterGrp::handle handle = App::GetApplication().GetParameterGroupByPath
        ("User parameter:BaseApp/Preferences/Mod/Mesh");
    double value = ui->maxDeviationExport->value().getValue();
    value = handle->GetFloat("MaxDeviationExport", value);
    ui->maxDeviationExport->setValue(value);
    
    ui->exportAmfCompressed->onRestore();
}

/**
 * Sets the strings of the subwidgets using the current language.
 */
void DlgSettingsImportExport::changeEvent(QEvent *e)
{
    if (e->type() == QEvent::LanguageChange) {
        ui->retranslateUi(this);
    }
    else {
        QWidget::changeEvent(e);
    }
}

#include "moc_DlgSettingsImportExportImp.cpp"

