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
#include <Gui/PrefWidgets.h>

using namespace TechDrawGui;

DlgPrefsTechDrawColorsImp::DlgPrefsTechDrawColorsImp( QWidget* parent )
  : PreferencePage( parent )
{
    this->setupUi(this);
}

DlgPrefsTechDrawColorsImp::~DlgPrefsTechDrawColorsImp()
{
    // no need to delete child widgets, Qt does it all for us
}

void DlgPrefsTechDrawColorsImp::saveSettings()
{
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
    pcbMarkup->onSave();
    pcbHighlight->onSave();
}

void DlgPrefsTechDrawColorsImp::loadSettings()
{
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
    pcbMarkup->onRestore();
    pcbHighlight->onRestore();
}

/**
 * Sets the strings of the subwidgets using the current language.
 */
void DlgPrefsTechDrawColorsImp::changeEvent(QEvent *e)
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

#include <Mod/TechDraw/Gui/moc_DlgPrefsTechDrawColorsImp.cpp>
