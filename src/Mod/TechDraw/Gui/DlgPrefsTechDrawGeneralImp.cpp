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

#include <Mod/TechDraw/App/DrawHatch.h>
#include <Mod/TechDraw/App/DrawGeomHatch.h>

#include "DlgPrefsTechDrawGeneralImp.h"
#include "ui_DlgPrefsTechDrawGeneral.h"
#include <Gui/PrefWidgets.h>

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
}

DlgPrefsTechDrawGeneralImp::~DlgPrefsTechDrawGeneralImp()
{
    // no need to delete child widgets, Qt does it all for us
}

void DlgPrefsTechDrawGeneralImp::saveSettings()
{
    ui->pfc_DefTemp->onSave();
    ui->pfc_DefDir->onSave();
    ui->pfc_HatchFile->onSave();
    ui->pfc_FilePattern->onSave();
    ui->pfc_LineGroup->onSave();
    ui->pfc_Welding->onSave();
    ui->le_NamePattern->onSave();

    ui->pfb_LabelFont->onSave();
    ui->plsb_LabelSize->onSave();

    ui->cb_Global->onSave();
    ui->cb_Override->onSave();
    ui->cb_PageUpdate->onSave();
    ui->cb_AutoDist->onSave();
}

void DlgPrefsTechDrawGeneralImp::loadSettings()
{
//    double labelDefault = 8.0;
    double labelDefault = Preferences::labelFontSizeMM();
    ui->plsb_LabelSize->setValue(labelDefault);
    QFont prefFont(Preferences::labelFontQString());
    ui->pfb_LabelFont->setCurrentFont(prefFont);
//    ui->pfb_LabelFont->setCurrentText(Preferences::labelFontQString());   //only works in Qt5

    ui->pfc_DefTemp->setFileName(Preferences::defaultTemplate());
    ui->pfc_DefDir->setFileName(Preferences::defaultTemplateDir());
    ui->pfc_HatchFile->setFileName(QString::fromStdString(DrawHatch::prefSvgHatch()));
    ui->pfc_FilePattern->setFileName(QString::fromStdString(DrawGeomHatch::prefGeomHatchFile()));
    ui->pfc_Welding->setFileName(PreferencesGui::weldingDirectory());
    ui->pfc_LineGroup->setFileName(QString::fromUtf8(Preferences::lineGroupFile().c_str()));

    ui->pfc_DefTemp->onRestore();
    ui->pfc_DefDir->onRestore();
    ui->pfc_HatchFile->onRestore();
    ui->pfc_FilePattern->onRestore();
    ui->pfc_LineGroup->onRestore();
    ui->pfc_Welding->onRestore();
    ui->le_NamePattern->onRestore();

    ui->pfb_LabelFont->onRestore();
    ui->plsb_LabelSize->onRestore();

    ui->cb_Global->onRestore();
    ui->cb_Override->onRestore();
    ui->cb_PageUpdate->onRestore();
    ui->cb_AutoDist->onRestore();
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
