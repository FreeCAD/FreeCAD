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
}

DlgPrefsTechDrawImp::~DlgPrefsTechDrawImp()
{
    // no need to delete child widgets, Qt does it all for us
}

void DlgPrefsTechDrawImp::saveSettings()
{
    pcb_Normal->onSave();
    pcb_Select->onSave();
    pcb_PreSelect->onSave();
    pcb_Hidden->onSave();
    pcb_Surface->onSave();
    le_LabelFont->onSave();
    pfc_DefTemp->onSave();
    pfc_DefDir->onSave();
    cb_HidLine->onSave();
    cb_Angle->onSave();
    pfc_HatchFile->onSave();
    //pfc_HatchDir->onSave();
}

void DlgPrefsTechDrawImp::loadSettings()
{
    pcb_Normal->onRestore();
    pcb_Select->onRestore();
    pcb_PreSelect->onRestore();
    pcb_Hidden->onRestore();
    pcb_Surface->onRestore();
    le_LabelFont->onRestore();
    pfc_DefTemp->onRestore();
    pfc_DefDir->onRestore();
    cb_HidLine->onRestore();
    cb_Angle->onRestore();
    pfc_HatchFile->onRestore();
    //pfc_HatchDir->onRestore();
}

/**
 * Sets the strings of the subwidgets using the current language.
 */
void DlgPrefsTechDrawImp::changeEvent(QEvent *e)
{
    if (e->type() == QEvent::LanguageChange) {
        retranslateUi(this);
    }
    else {
        QWidget::changeEvent(e);
    }
}

#include "moc_DlgPrefsTechDrawImp.cpp"
