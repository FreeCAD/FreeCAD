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

#include "DlgPrefsTechDraw1Imp.h"
#include <Gui/PrefWidgets.h>

using namespace TechDrawGui;

DlgPrefsTechDraw1Imp::DlgPrefsTechDraw1Imp( QWidget* parent )
  : PreferencePage( parent )
{
    this->setupUi(this);
    plsb_LabelSize->setUnit(Base::Unit::Length);
}

DlgPrefsTechDraw1Imp::~DlgPrefsTechDraw1Imp()
{
    // no need to delete child widgets, Qt does it all for us
}

void DlgPrefsTechDraw1Imp::saveSettings()
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

    pcbDimColor->onSave();
    pcb_Hatch->onSave();
    pcb_Background->onSave();
    pcb_PreSelect->onSave();
    pcb_Hidden->onSave();
    pcb_Select->onSave();
    pcb_Normal->onSave();
    pcb_Surface->onSave();
    pcb_GeomHatch->onSave();
    pcb_Face->onSave();
    pcb_PaintFaces->onSave();
    pcbSectionLine->onSave();
    pcbCenterColor->onSave();
    pcbVertexColor->onSave();
}

void DlgPrefsTechDraw1Imp::loadSettings()
{
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

    pcbDimColor->onRestore();
    pcb_Hatch->onRestore();
    pcb_Background->onRestore();
    pcb_PreSelect->onRestore();
    pcb_Hidden->onRestore();
    pcb_Select->onRestore();
    pcb_Normal->onRestore();
    pcb_Surface->onRestore();
    pcb_GeomHatch->onRestore();
    pcb_Face->onRestore();
    pcb_PaintFaces->onRestore();
    pcbSectionLine->onRestore();
    pcbCenterColor->onRestore();
    pcbVertexColor->onRestore();
}

/**
 * Sets the strings of the subwidgets using the current language.
 */
void DlgPrefsTechDraw1Imp::changeEvent(QEvent *e)
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

#include <Mod/TechDraw/Gui/moc_DlgPrefsTechDraw1Imp.cpp>
