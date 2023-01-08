/***************************************************************************
 *   Copyright (c) 2020 FreeCAD Developers                                 *
 *   Author: Uwe Stöhr <uwestoehr@lyx.org>                                 *
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

#include "DlgPrefsTechDrawColorsImp.h"
#include "ui_DlgPrefsTechDrawColors.h"


using namespace TechDrawGui;

DlgPrefsTechDrawColorsImp::DlgPrefsTechDrawColorsImp( QWidget* parent )
  : PreferencePage( parent )
  , ui(new Ui_DlgPrefsTechDrawColorsImp)
{
    ui->setupUi(this);
}

DlgPrefsTechDrawColorsImp::~DlgPrefsTechDrawColorsImp()
{
    // no need to delete child widgets, Qt does it all for us
}

void DlgPrefsTechDrawColorsImp::saveSettings()
{
    ui->pcbDimColor->onSave();
    ui->pcb_Hatch->onSave();
    ui->pcb_Background->onSave();
    ui->pcb_PreSelect->onSave();
    ui->pcb_Hidden->onSave();
    ui->pcb_Select->onSave();
    ui->pcb_Normal->onSave();
    ui->pcb_Surface->onSave();
    ui->pcb_GeomHatch->onSave();
    ui->pcb_Face->onSave();
    ui->pcb_PaintFaces->onSave();
    ui->pcbSectionLine->onSave();
    ui->pcbCenterColor->onSave();
    ui->pcbVertexColor->onSave();
    ui->pcbMarkup->onSave();
    ui->pcbHighlight->onSave();
    ui->pcb_Grid->onSave();
    ui->pcbPageColor->onSave();
    ui->pcbLightOnDark->onSave();
    ui->pcbMonochrome->onSave();
    ui->pcbLightTextColor->onSave();
}

void DlgPrefsTechDrawColorsImp::loadSettings()
{
    ui->pcbDimColor->onRestore();
    ui->pcb_Hatch->onRestore();
    ui->pcb_Background->onRestore();
    ui->pcb_PreSelect->onRestore();
    ui->pcb_Hidden->onRestore();
    ui->pcb_Select->onRestore();
    ui->pcb_Normal->onRestore();
    ui->pcb_Surface->onRestore();
    ui->pcb_GeomHatch->onRestore();
    ui->pcb_Face->onRestore();
    ui->pcb_PaintFaces->onRestore();
    ui->pcbSectionLine->onRestore();
    ui->pcbCenterColor->onRestore();
    ui->pcbVertexColor->onRestore();
    ui->pcbMarkup->onRestore();
    ui->pcbHighlight->onRestore();
    ui->pcb_Grid->onRestore();
    ui->pcbPageColor->onRestore();
    ui->pcbLightOnDark->onRestore();
    ui->pcbMonochrome->onRestore();
    ui->pcbLightTextColor->onRestore();
}

/**
 * Sets the strings of the subwidgets using the current language.
 */
void DlgPrefsTechDrawColorsImp::changeEvent(QEvent *e)
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

#include <Mod/TechDraw/Gui/moc_DlgPrefsTechDrawColorsImp.cpp>
