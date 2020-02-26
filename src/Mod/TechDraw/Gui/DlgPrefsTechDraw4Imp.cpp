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

#include "DlgPrefsTechDraw4Imp.h"
#include <Gui/PrefWidgets.h>

using namespace TechDrawGui;

DlgPrefsTechDraw4Imp::DlgPrefsTechDraw4Imp( QWidget* parent )
  : PreferencePage( parent )
{
    this->setupUi(this);
}

DlgPrefsTechDraw4Imp::~DlgPrefsTechDraw4Imp()
{
    // no need to delete child widgets, Qt does it all for us
}

void DlgPrefsTechDraw4Imp::saveSettings()
{
    cbEndCap->onSave();
    cbCrazyEdges->onSave();
    cbDebugSection->onSave();
    cbDetectFaces->onSave();
    cbDebugDetail->onSave();
    cbShowSectionEdges->onSave();
    cbFuseBeforeSection->onSave();
    sbMaxTiles->onSave();
    sbMaxPat->onSave();
    cbShowLoose->onSave();
}

void DlgPrefsTechDraw4Imp::loadSettings()
{
    cbEndCap->onRestore();
    cbCrazyEdges->onRestore();
    cbDebugSection->onRestore();
    cbDetectFaces->onRestore();
    cbDebugDetail->onRestore();
    cbShowSectionEdges->onRestore();
    cbFuseBeforeSection->onRestore();
    sbMaxTiles->onRestore();
    sbMaxPat->onRestore();
    cbShowLoose->onRestore();
}

/**
 * Sets the strings of the subwidgets using the current language.
 */
void DlgPrefsTechDraw4Imp::changeEvent(QEvent *e)
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

#include <Mod/TechDraw/Gui/moc_DlgPrefsTechDraw4Imp.cpp>
