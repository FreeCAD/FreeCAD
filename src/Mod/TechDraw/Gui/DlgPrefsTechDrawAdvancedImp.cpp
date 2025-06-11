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

#include <Mod/TechDraw/App/Preferences.h>
#include "DlgPrefsTechDrawAdvancedImp.h"
#include "ui_DlgPrefsTechDrawAdvanced.h"


using namespace TechDrawGui;
using namespace TechDraw;

DlgPrefsTechDrawAdvancedImp::DlgPrefsTechDrawAdvancedImp( QWidget* parent )
  : PreferencePage( parent )
  , ui(new Ui_DlgPrefsTechDrawAdvancedImp)
{
    ui->setupUi(this);

    makeBalloonBoxConnections();
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
    ui->pdsbEdgeFuzz->onSave();
    ui->pdsbMarkFuzz->onSave();
    ui->sbMaxTiles->onSave();
    ui->sbMaxPat->onSave();
    ui->cbReportProgress->onSave();
    ui->cbAutoCorrectRefs->onSave();
    ui->cbNewFaceFinder->onSave();
    ui->sbScrubCount->onSave();

    ui->cbDebugBadShape->onSave();
    ui->cbValidateShapes->onSave();

    saveBalloonOverride();

    ui->cbSwitchWB->onSave();
}


void DlgPrefsTechDrawAdvancedImp::saveBalloonOverride()
{
    if (ui->cbBalloonDefault->isChecked()) {
        Preferences::setBalloonDragModifiers(Qt::ControlModifier);
        return;
    }

    Qt::KeyboardModifiers result{Qt::NoModifier};

    if (ui->cbBalloonShift->isChecked()) {
        result |= Qt::ShiftModifier;
    }

     if (ui->cbBalloonControl->isChecked()) {
        result |= Qt::ControlModifier;
     }

     if (ui->cbBalloonAlt->isChecked()) {
        result |= Qt::AltModifier;
     }

     if (ui->cbBalloonMeta->isChecked()) {
         result |= Qt::MetaModifier;
     }

     Preferences::setBalloonDragModifiers(result);
}


void DlgPrefsTechDrawAdvancedImp::loadSettings()
{
    ui->cbDetectFaces->onRestore();
    ui->cbShowSectionEdges->onRestore();
    ui->cbDebugSection->onRestore();
    ui->cbDebugDetail->onRestore();
    ui->cbCrazyEdges->onRestore();
    ui->cbFuseBeforeSection->onRestore();
    ui->pdsbEdgeFuzz->onRestore();
    ui->pdsbMarkFuzz->onRestore();
    ui->sbMaxTiles->onRestore();
    ui->sbMaxPat->onRestore();
    ui->cbReportProgress->onRestore();
    ui->cbAutoCorrectRefs->onRestore();
    ui->cbNewFaceFinder->onRestore();
    ui->sbScrubCount->onRestore();

    ui->cbDebugBadShape->onRestore();
    ui->cbValidateShapes->onRestore();

    loadBalloonOverride();

    ui->cbSwitchWB->onRestore();
}

void DlgPrefsTechDrawAdvancedImp::loadBalloonOverride()
{
    uint prefOverride = Preferences::balloonDragModifiers();
    if (prefOverride == Qt::ControlModifier) {
        // default case
        ui->cbBalloonDefault->setChecked(true);
        clearBalloonOptions();
        enableBalloonOptions(false);
        return;
    }

    ui->cbBalloonDefault->setChecked(false);
    enableBalloonOptions(true);

    if (flagsContainValue(prefOverride, Qt::ShiftModifier)) {
        ui->cbBalloonShift->setChecked(true);
    }
    if (flagsContainValue(prefOverride, Qt::ControlModifier)) {
        ui->cbBalloonControl->setChecked(true);
    }

    if (flagsContainValue(prefOverride, Qt::AltModifier)) {
        ui->cbBalloonAlt->setChecked(true);
    }

    if (flagsContainValue(prefOverride, Qt::MetaModifier)) {
        ui->cbBalloonMeta->setChecked(true);
    }
}

//! true if bit pattern of value is found in flags.
bool DlgPrefsTechDrawAdvancedImp::flagsContainValue(uint flags, uint value)
{
    uint matchResult = flags & value;
    if (matchResult == value) {
        return true;
    }
    return false;
}


void DlgPrefsTechDrawAdvancedImp::clearBalloonOptions()
{
    ui->cbBalloonShift->setChecked(false);
    ui->cbBalloonControl->setChecked(false);
    ui->cbBalloonAlt->setChecked(false);
    ui->cbBalloonMeta->setChecked(false);
}


void DlgPrefsTechDrawAdvancedImp::enableBalloonOptions(bool newState)
{
    ui->cbBalloonShift->setEnabled(newState);
    ui->cbBalloonControl->setEnabled(newState);
    ui->cbBalloonAlt->setEnabled(newState);
    ui->cbBalloonMeta->setEnabled(newState);
}


void DlgPrefsTechDrawAdvancedImp::slotBalloonBoxChecked()
{
    if (ui->cbBalloonDefault->isChecked()) {
        clearBalloonOptions();
        enableBalloonOptions(false);
    } else {
        enableBalloonOptions(true);
    }

}


void DlgPrefsTechDrawAdvancedImp::makeBalloonBoxConnections()
{
#if QT_VERSION >= QT_VERSION_CHECK(6,7,0)
    connect(ui->cbBalloonDefault,
            &QCheckBox::checkStateChanged,
            this,
            &DlgPrefsTechDrawAdvancedImp::slotBalloonBoxChecked);
    connect(ui->cbBalloonShift,
            &QCheckBox::checkStateChanged,
            this,
            &DlgPrefsTechDrawAdvancedImp::slotBalloonBoxChecked);
    connect(ui->cbBalloonControl,
            &QCheckBox::checkStateChanged,
            this,
            &DlgPrefsTechDrawAdvancedImp::slotBalloonBoxChecked);
    connect(ui->cbBalloonAlt,
            &QCheckBox::checkStateChanged,
            this,
            &DlgPrefsTechDrawAdvancedImp::slotBalloonBoxChecked);
    connect(ui->cbBalloonMeta,
            &QCheckBox::checkStateChanged,
            this,
            &DlgPrefsTechDrawAdvancedImp::slotBalloonBoxChecked);
#else
    connect(ui->cbBalloonDefault,
            qOverload<int>(&QCheckBox::stateChanged),
            this,
            &DlgPrefsTechDrawAdvancedImp::slotBalloonBoxChecked);
    connect(ui->cbBalloonShift,
            qOverload<int>(&QCheckBox::stateChanged),
            this,
            &DlgPrefsTechDrawAdvancedImp::slotBalloonBoxChecked);
    connect(ui->cbBalloonControl,
            qOverload<int>(&QCheckBox::stateChanged),
            this,
            &DlgPrefsTechDrawAdvancedImp::slotBalloonBoxChecked);
    connect(ui->cbBalloonAlt,
            qOverload<int>(&QCheckBox::stateChanged),
            this,
            &DlgPrefsTechDrawAdvancedImp::slotBalloonBoxChecked);
    connect(ui->cbBalloonMeta,
            qOverload<int>(&QCheckBox::stateChanged),
            this,
            &DlgPrefsTechDrawAdvancedImp::slotBalloonBoxChecked);
#endif
}


/**
 * Sets the strings of the subwidgets using the current language.
 */
void DlgPrefsTechDrawAdvancedImp::changeEvent(QEvent *event)
{
    if (event->type() == QEvent::LanguageChange) {
        ui->retranslateUi(this);
    }
    else {
        QWidget::changeEvent(event);
    }
}

#include <Mod/TechDraw/Gui/moc_DlgPrefsTechDrawAdvancedImp.cpp>
