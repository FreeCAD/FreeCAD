/***************************************************************************
 *   Copyright (c) 2022 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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

#include <Mod/Part/App/OCAF/ImportExportSettings.h>
#include <Mod/Part/App/STEP/ImportExportSettings.h>

#include "DlgImportStep.h"
#include "ui_DlgImportStep.h"


using namespace PartGui;

DlgImportStep::DlgImportStep(QWidget* parent)
  : PreferencePage(parent)
  , ui(new Ui_DlgImportStep)
{
    ui->setupUi(this);

    Part::OCAF::ImportExportSettings settings;
    ui->checkBoxMergeCompound->setChecked(settings.getReadShapeCompoundMode());
    ui->checkBoxImportHiddenObj->setChecked(settings.getImportHiddenObject());
    ui->checkBoxUseLinkGroup->setChecked(settings.getUseLinkGroup());
    ui->checkBoxUseBaseName->setChecked(settings.getUseBaseName());
    ui->checkBoxReduceObjects->setChecked(settings.getReduceObjects());
    ui->checkBoxExpandCompound->setChecked(settings.getExpandCompound());
    ui->checkBoxShowProgress->setChecked(settings.getShowProgress());
}

/**
 *  Destroys the object and frees any allocated resources
 */
DlgImportStep::~DlgImportStep() = default;

void DlgImportStep::saveSettings()
{
    // (h)STEP of Import module
    ui->checkBoxMergeCompound->onSave();
    ui->checkBoxImportHiddenObj->onSave();
    ui->checkBoxUseLinkGroup->onSave();
    ui->checkBoxUseBaseName->onSave();
    ui->checkBoxReduceObjects->onSave();
    ui->checkBoxExpandCompound->onSave();
    ui->checkBoxShowProgress->onSave();
    ui->comboBoxImportMode->onSave();
}

void DlgImportStep::loadSettings()
{
    // (h)STEP of Import module
    ui->checkBoxMergeCompound->onRestore();
    ui->checkBoxImportHiddenObj->onRestore();
    ui->checkBoxUseLinkGroup->onRestore();
    ui->checkBoxUseBaseName->onRestore();
    ui->checkBoxReduceObjects->onRestore();
    ui->checkBoxExpandCompound->onRestore();
    ui->checkBoxShowProgress->onRestore();
    ui->comboBoxImportMode->onRestore();
}

/**
 * Sets the strings of the subwidgets using the current language.
 */
void DlgImportStep::changeEvent(QEvent *e)
{
    if (e->type() == QEvent::LanguageChange) {
        ui->retranslateUi(this);
    }
    else {
        QWidget::changeEvent(e);
    }
}


#include "moc_DlgImportStep.cpp"
