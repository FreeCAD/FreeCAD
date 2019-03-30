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

#include "DlgPrefsTechDraw2Imp.h"
#include <Gui/PrefWidgets.h>

using namespace TechDrawGui;

DlgPrefsTechDraw2Imp::DlgPrefsTechDraw2Imp( QWidget* parent )
  : PreferencePage( parent )
{
    this->setupUi(this);
    plsb_FontSize->setUnit(Base::Unit::Length);
    plsb_ArrowSize->setUnit(Base::Unit::Length);
}

DlgPrefsTechDraw2Imp::~DlgPrefsTechDraw2Imp()
{
    // no need to delete child widgets, Qt does it all for us
}

void DlgPrefsTechDraw2Imp::saveSettings()
{
    cbShowUnits->onSave();
    plsb_FontSize->onSave();
    colDimColor->onSave();
    leDiameter->onSave();
    pcbMatting->onSave();
    pcbCenterStyle->onSave();
    colCenterLine->onSave();
    pcbSectionStyle->onSave();
    colSectionLine->onSave();
    pcbArrow->onSave();
    cbGlobalDecimals->onSave();
    sbAltDecimals->onSave();
    leformatSpec->onSave();
    plsb_ArrowSize->onSave();
    leLineGroup->onSave();
    pdsb_VertexScale->onSave();
    pcb_VertexColor->onSave();
}

void DlgPrefsTechDraw2Imp::loadSettings()
{
    cbShowUnits->onRestore();
    plsb_FontSize->onRestore();
    colDimColor->onRestore();
    leDiameter->onRestore();
    pcbMatting->onRestore();
    pcbCenterStyle->onRestore();
    colCenterLine->onRestore();
    pcbSectionStyle->onRestore();
    colSectionLine->onRestore();
    pcbArrow->onRestore();
    cbGlobalDecimals->onRestore();
    sbAltDecimals->onRestore();
    leformatSpec->onRestore();
    plsb_ArrowSize->onRestore();
    leLineGroup->onRestore();
    pdsb_VertexScale->onRestore();
    pcb_VertexColor->onRestore();
}

/**
 * Sets the strings of the subwidgets using the current language.
 */
void DlgPrefsTechDraw2Imp::changeEvent(QEvent *e)
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

#include <Mod/TechDraw/Gui/moc_DlgPrefsTechDraw2Imp.cpp>
