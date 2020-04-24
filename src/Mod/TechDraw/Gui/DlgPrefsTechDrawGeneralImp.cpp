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
#include <Gui/PrefWidgets.h>

#include "PreferencesGui.h"
using namespace TechDrawGui;
using namespace TechDraw;

DlgPrefsTechDrawGeneralImp::DlgPrefsTechDrawGeneralImp( QWidget* parent )
  : PreferencePage( parent )
{
    this->setupUi(this);
    plsb_LabelSize->setUnit(Base::Unit::Length);
    plsb_LabelSize->setMinimum(0);
}

DlgPrefsTechDrawGeneralImp::~DlgPrefsTechDrawGeneralImp()
{
    // no need to delete child widgets, Qt does it all for us
}

void DlgPrefsTechDrawGeneralImp::saveSettings()
{
    pfc_DefTemp->onSave();
    pfc_DefDir->onSave();
    pfc_HatchFile->onSave();
    pfc_FilePattern->onSave();
    pfc_LineGroup->onSave();
    pfc_Welding->onSave();
    le_NamePattern->onSave();

    pfb_LabelFont->onSave();
    plsb_LabelSize->onSave();

    cb_Global->onSave();
    cb_Override->onSave();
    cb_PageUpdate->onSave();
    cb_AutoDist->onSave();
}

void DlgPrefsTechDrawGeneralImp::loadSettings()
{
//    double labelDefault = 8.0;
    double labelDefault = Preferences::labelFontSizeMM();
    plsb_LabelSize->setValue(labelDefault);
    QFont prefFont(Preferences::labelFontQString());
    pfb_LabelFont->setCurrentFont(prefFont);
//    pfb_LabelFont->setCurrentText(Preferences::labelFontQString());   //only works in Qt5

    pfc_DefTemp->setFileName(Preferences::defaultTemplate());
    pfc_DefDir->setFileName(Preferences::defaultTemplateDir());
    pfc_HatchFile->setFileName(QString::fromStdString(DrawHatch::prefSvgHatch()));
    pfc_FilePattern->setFileName(QString::fromStdString(DrawGeomHatch::prefGeomHatchFile()));
    pfc_Welding->setFileName(PreferencesGui::weldingDirectory());
    pfc_LineGroup->setFileName(QString::fromUtf8(Preferences::lineGroupFile().c_str()));

    pfc_DefTemp->onRestore();
    pfc_DefDir->onRestore();
    pfc_HatchFile->onRestore();
    pfc_FilePattern->onRestore();
    pfc_LineGroup->onRestore();
    pfc_Welding->onRestore();
    le_NamePattern->onRestore();

    pfb_LabelFont->onRestore();
    plsb_LabelSize->onRestore();

    cb_Global->onRestore();
    cb_Override->onRestore();
    cb_PageUpdate->onRestore();
    cb_AutoDist->onRestore();
}

/**
 * Sets the strings of the subwidgets using the current language.
 */
void DlgPrefsTechDrawGeneralImp::changeEvent(QEvent *e)
{
    if (e->type() == QEvent::LanguageChange) {
        saveSettings();
        retranslateUi(this);
        loadSettings();
    }
    else {
        QWidget::changeEvent(e);
    }
}

#include <Mod/TechDraw/Gui/moc_DlgPrefsTechDrawGeneralImp.cpp>
