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

#include <App/Application.h>

#include <Base/Parameter.h>
#include <Base/Console.h>

#include "DrawGuiUtil.h"
#include "PreferencesGui.h"
#include "DlgPrefsTechDrawAnnotationImp.h"
#include "ui_DlgPrefsTechDrawAnnotation.h"


using namespace TechDrawGui;
using namespace TechDraw;


DlgPrefsTechDrawAnnotationImp::DlgPrefsTechDrawAnnotationImp( QWidget* parent )
  : PreferencePage( parent )
  , ui(new Ui_DlgPrefsTechDrawAnnotationImp)
{
    ui->setupUi(this);
    ui->pdsbBalloonKink->setUnit(Base::Unit::Length);
    ui->pdsbBalloonKink->setMinimum(0);
}

DlgPrefsTechDrawAnnotationImp::~DlgPrefsTechDrawAnnotationImp()
{
    // no need to delete child widgets, Qt does it all for us
}

void DlgPrefsTechDrawAnnotationImp::saveSettings()
{
    ui->cbAutoHoriz->onSave();
    ui->cbPrintCenterMarks->onSave();
    ui->cbPyramidOrtho->onSave();
    ui->cbSectionLineStd->onSave();
    ui->cbShowCenterMarks->onSave();
    ui->leLineGroup->onSave();
    ui->pcbBalloonArrow->onSave();
    ui->pcbBalloonShape->onSave();
    ui->pcbCenterStyle->onSave();
    ui->pcbMatting->onSave();
    ui->pcbSectionStyle->onSave();
    ui->pdsbBalloonKink->onSave();
    ui->cbCutSurface->onSave();
    ui->pcbHighlightStyle->onSave();
}

void DlgPrefsTechDrawAnnotationImp::loadSettings()
{
    //set defaults for Quantity widgets if property not found
    //Quantity widgets do not use preset value since they are based on
    //QAbstractSpinBox
    double kinkDefault = 5.0;
    ui->pdsbBalloonKink->setValue(kinkDefault);

    ui->cbAutoHoriz->onRestore();
    ui->cbPrintCenterMarks->onRestore();
    ui->cbPyramidOrtho->onRestore();
    ui->cbSectionLineStd->onRestore();
    ui->cbShowCenterMarks->onRestore();
    ui->leLineGroup->onRestore();
    ui->pcbBalloonArrow->onRestore();
    ui->pcbBalloonShape->onRestore();
    ui->pcbCenterStyle->onRestore();
    ui->pcbMatting->onRestore();
    ui->pcbSectionStyle->onRestore();
    ui->pdsbBalloonKink->onRestore();
    ui->cbCutSurface->onRestore();
    ui->pcbHighlightStyle->onRestore();

    DrawGuiUtil::loadArrowBox(ui->pcbBalloonArrow);
    ui->pcbBalloonArrow->setCurrentIndex(prefBalloonArrow());
}

/**
 * Sets the strings of the subwidgets using the current language.
 */
void DlgPrefsTechDrawAnnotationImp::changeEvent(QEvent *e)
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

int DlgPrefsTechDrawAnnotationImp::prefBalloonArrow(void) const
{
    return Preferences::balloonArrow();
}

#include <Mod/TechDraw/Gui/moc_DlgPrefsTechDrawAnnotationImp.cpp>
