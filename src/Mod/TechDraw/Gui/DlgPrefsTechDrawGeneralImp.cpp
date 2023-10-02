/***************************************************************************
 *   Copyright (c) 2015 FreeCAD Developers                                 *
 *   Author: WandererFan <wandererfan@gmail.com>                           *
 *   Based on src/Mod/FEM/Gui/DlgSettingsFEMImp.cpp                        *
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

#include "DlgPrefsTechDrawGeneralImp.h"
#include "ui_DlgPrefsTechDrawGeneral.h"
#include "PreferencesGui.h"


using namespace TechDrawGui;
using namespace TechDraw;

DlgPrefsTechDrawGeneralImp::DlgPrefsTechDrawGeneralImp( QWidget* parent )
  : PreferencePage( parent )
  , ui(new Ui_DlgPrefsTechDrawGeneralImp)
{
    ui->setupUi(this);
    ui->plsb_LabelSize->setUnit(Base::Unit::Length);
    ui->plsb_LabelSize->setMinimum(0);

    ui->psb_GridSpacing->setUnit(Base::Unit::Length);
    ui->psb_GridSpacing->setMinimum(0);
}

DlgPrefsTechDrawGeneralImp::~DlgPrefsTechDrawGeneralImp()
{
    // no need to delete child widgets, Qt does it all for us
}

void DlgPrefsTechDrawGeneralImp::saveSettings()
{
    ui->cb_Global->onSave();
    ui->cb_Override->onSave();
    ui->cb_PageUpdate->onSave();
    ui->cb_AutoDist->onSave();

    ui->pfb_LabelFont->onSave();
    ui->plsb_LabelSize->onSave();

    ui->cbProjAngle->onSave();
    ui->cbHiddenLineStyle->onSave();

    ui->pfc_DefTemp->onSave();
    ui->pfc_DefDir->onSave();
    ui->pfc_HatchFile->onSave();
    ui->pfc_LineGroup->onSave();
    ui->pfc_Welding->onSave();
    ui->pfc_FilePattern->onSave();
    ui->le_NamePattern->onSave();
    ui->cb_ShowGrid->onSave();
    ui->psb_GridSpacing->onSave();
}

void DlgPrefsTechDrawGeneralImp::loadSettings()
{
    ui->cb_Global->onRestore();
    ui->cb_Override->onRestore();
    ui->cb_PageUpdate->onRestore();
    ui->cb_AutoDist->onRestore();

    double labelDefault = Preferences::labelFontSizeMM();
    ui->plsb_LabelSize->setValue(labelDefault);
    QFont prefFont(Preferences::labelFontQString());
    ui->pfb_LabelFont->setCurrentFont(prefFont);
    //    ui->pfb_LabelFont->setCurrentText(Preferences::labelFontQString());   //only works in Qt5

    ui->pfb_LabelFont->onRestore();
    ui->plsb_LabelSize->onRestore();

    ui->cbProjAngle->onRestore();
    ui->cbHiddenLineStyle->onRestore();

    ui->pfc_DefTemp->onRestore();
    ui->pfc_DefDir->onRestore();
    ui->pfc_HatchFile->onRestore();
    ui->pfc_LineGroup->onRestore();
    ui->pfc_Welding->onRestore();
    ui->pfc_FilePattern->onRestore();
    ui->le_NamePattern->onRestore();

    bool gridDefault = PreferencesGui::showGrid();
    ui->cb_ShowGrid->setChecked(gridDefault);
    ui->cb_ShowGrid->onRestore();

    double spacingDefault = PreferencesGui::gridSpacing();
    ui->psb_GridSpacing->setValue(spacingDefault);
    ui->psb_GridSpacing->onRestore();
}

/**
 * Sets the strings of the subwidgets using the current language.
 */
void DlgPrefsTechDrawGeneralImp::changeEvent(QEvent *e)
{
    if (e->type() == QEvent::LanguageChange) {
        saveSettings();
        ui->retranslateUi(this);
        loadSettings();
    }
    else {
        QWidget::changeEvent(e);
    }
}

#include <Mod/TechDraw/Gui/moc_DlgPrefsTechDrawGeneralImp.cpp>
