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

#include "DlgPrefsTechDrawImp.h"
#include <Gui/PrefWidgets.h>

using namespace TechDrawGui;

DlgPrefsTechDrawImp::DlgPrefsTechDrawImp( QWidget* parent )
  : PreferencePage( parent )
{
    this->setupUi(this);
    plsb_LabelSize->setUnit(Base::Unit::Length);
    plsb_TemplateDot->setUnit(Base::Unit::Length);
}

DlgPrefsTechDrawImp::~DlgPrefsTechDrawImp()
{
    // no need to delete child widgets, Qt does it all for us
}

void DlgPrefsTechDrawImp::saveSettings()
{
    cb_HidLine->onSave();
    cb_Angle->onSave();
    cb_Faces->onSave();
    cb_SectionEdges->onSave();
    cb_PageUpdate->onSave();
    cb_AutoDist->onSave();

    pcb_Normal->onSave();
    pcb_Select->onSave();
    pcb_PreSelect->onSave();
    pcb_Hidden->onSave();
    pcb_Surface->onSave();
    pcb_Background->onSave();
    pcb_Hatch->onSave();

    pfb_LabelFont->onSave();
    plsb_LabelSize->onSave();
    plsb_TemplateDot->onSave();

    pfc_DefTemp->onSave();
    pfc_DefDir->onSave();
    pfc_HatchFile->onSave();
    pfc_LineGroup->onSave();
    pfc_FilePattern->onSave();
    le_NamePattern->onSave();
}

void DlgPrefsTechDrawImp::loadSettings()
{
    cb_HidLine->onRestore();
    cb_Angle->onRestore();
    cb_Faces->onRestore();
    cb_SectionEdges->onRestore();
    cb_PageUpdate->onRestore();
    cb_AutoDist->onRestore();

    pcb_Normal->onRestore();
    pcb_Select->onRestore();
    pcb_PreSelect->onRestore();
    pcb_Hidden->onRestore();
    pcb_Surface->onRestore();
    pcb_Background->onRestore();
    pcb_Hatch->onRestore();

    pfb_LabelFont->onRestore();
    plsb_LabelSize->onRestore();
    plsb_TemplateDot->onRestore();

    pfc_DefTemp->onRestore();
    pfc_DefDir->onRestore();
    pfc_HatchFile->onRestore();
    pfc_LineGroup->onRestore();

    pfc_FilePattern->onRestore();
    le_NamePattern->onRestore();
}

/**
 * Sets the strings of the subwidgets using the current language.
 */
void DlgPrefsTechDrawImp::changeEvent(QEvent *e)
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

#include <Mod/TechDraw/Gui/moc_DlgPrefsTechDrawImp.cpp>
