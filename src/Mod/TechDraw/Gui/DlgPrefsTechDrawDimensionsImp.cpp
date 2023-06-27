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

#include <Base/Tools.h>

#include "DlgPrefsTechDrawDimensionsImp.h"
#include "ui_DlgPrefsTechDrawDimensions.h"
#include "DrawGuiUtil.h"
#include "PreferencesGui.h"


using namespace TechDrawGui;
using namespace TechDraw;


DlgPrefsTechDrawDimensionsImp::DlgPrefsTechDrawDimensionsImp( QWidget* parent )
  : PreferencePage( parent )
  , ui(new Ui_DlgPrefsTechDrawDimensionsImp)
{
    ui->setupUi(this);
    ui->plsb_FontSize->setUnit(Base::Unit::Length);
    ui->plsb_FontSize->setMinimum(0);
    ui->plsb_ArrowSize->setUnit(Base::Unit::Length);
    ui->plsb_ArrowSize->setMinimum(0);
}

DlgPrefsTechDrawDimensionsImp::~DlgPrefsTechDrawDimensionsImp()
{
    // no need to delete child widgets, Qt does it all for us
}

void DlgPrefsTechDrawDimensionsImp::saveSettings()
{
    ui->pcbStandardAndStyle->onSave();
    ui->cbGlobalDecimals->onSave();
    ui->cbShowUnits->onSave();
    ui->sbAltDecimals->onSave();
    ui->plsb_FontSize->onSave();
    ui->pdsbToleranceScale->onSave();
    ui->leDiameter->onSave();
    ui->pcbArrow->onSave();
    ui->plsb_ArrowSize->onSave();
    ui->leFormatSpec->onSave();
    ui->pdsbGapISO->onSave();
    ui->pdsbGapASME->onSave();
}

void DlgPrefsTechDrawDimensionsImp::loadSettings()
{
    //set defaults for Quantity widgets if property not found
    //Quantity widgets do not use preset value since they are based on
    //QAbstractSpinBox
    double fontDefault = Preferences::dimFontSizeMM();
    double arrowDefault = Preferences::dimArrowSize();
    ui->plsb_FontSize->setValue(fontDefault);
//    double arrowDefault = 5.0;
//    plsb_ArrowSize->setValue(arrowDefault);
    ui->plsb_ArrowSize->setValue(arrowDefault);

    ui->pcbStandardAndStyle->onRestore();
    ui->cbGlobalDecimals->onRestore();
    ui->cbShowUnits->onRestore();
    ui->sbAltDecimals->onRestore();
    ui->plsb_FontSize->onRestore();
    ui->pdsbToleranceScale->onRestore();
    ui->leDiameter->onRestore();
    ui->pcbArrow->onRestore();
    ui->plsb_ArrowSize->onRestore();

    DrawGuiUtil::loadArrowBox(ui->pcbArrow);
    ui->pcbArrow->setCurrentIndex(prefArrowStyle());

    ui->leFormatSpec->setText(Base::Tools::fromStdString(Preferences::formatSpec()));
    ui->leFormatSpec->onRestore();

    ui->pdsbGapISO->onRestore();
    ui->pdsbGapASME->onRestore();
}

/**
 * Sets the strings of the subwidgets using the current language.
 */
void DlgPrefsTechDrawDimensionsImp::changeEvent(QEvent *e)
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

int DlgPrefsTechDrawDimensionsImp::prefArrowStyle() const
{
    return PreferencesGui::dimArrowStyle();
}

#include <Mod/TechDraw/Gui/moc_DlgPrefsTechDrawDimensionsImp.cpp>
