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

#include "DlgPrefsTechDrawAdvancedImp.h"
#include "ui_DlgPrefsTechDrawAdvanced.h"


using namespace TechDrawGui;

DlgPrefsTechDrawAdvancedImp::DlgPrefsTechDrawAdvancedImp( QWidget* parent )
  : PreferencePage( parent )
  , ui(new Ui_DlgPrefsTechDrawAdvancedImp)
{
    ui->setupUi(this);
}

DlgPrefsTechDrawAdvancedImp::~DlgPrefsTechDrawAdvancedImp()
{
    // no need to delete child widgets, Qt does it all for us
}

void DlgPrefsTechDrawAdvancedImp::saveSettings()
{
    ui->cbDetectFaces->onSave();
    ui->cbShowSectionEdges->onSave();
    ui->cbDebugSection->onSave();
    ui->cbDebugDetail->onSave();
    ui->cbCrazyEdges->onSave();
    ui->cbFuseBeforeSection->onSave();
    ui->cbShowLoose->onSave();
    ui->pdsbEdgeFuzz->onSave();
    ui->pdsbMarkFuzz->onSave();
    ui->cbEndCap->onSave();
    ui->sbMaxTiles->onSave();
    ui->sbMaxPat->onSave();
    ui->cbReportProgress->onSave();
    ui->cbAutoCorrectRefs->onSave();
    ui->cbNewFaceFinder->onSave();
    ui->sbScrubCount->onSave();
}

void DlgPrefsTechDrawAdvancedImp::loadSettings()
{
    ui->cbDetectFaces->onRestore();
    ui->cbShowSectionEdges->onRestore();
    ui->cbDebugSection->onRestore();
    ui->cbDebugDetail->onRestore();
    ui->cbCrazyEdges->onRestore();
    ui->cbFuseBeforeSection->onRestore();
    ui->cbShowLoose->onRestore();
    ui->pdsbEdgeFuzz->onRestore();
    ui->pdsbMarkFuzz->onRestore();
    ui->cbEndCap->onRestore();
    ui->sbMaxTiles->onRestore();
    ui->sbMaxPat->onRestore();
    ui->cbReportProgress->onRestore();
    ui->cbAutoCorrectRefs->onRestore();
    ui->cbNewFaceFinder->onRestore();
    ui->sbScrubCount->onRestore();
}

/**
 * Sets the strings of the subwidgets using the current language.
 */
void DlgPrefsTechDrawAdvancedImp::changeEvent(QEvent *e)
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

#include <Mod/TechDraw/Gui/moc_DlgPrefsTechDrawAdvancedImp.cpp>
