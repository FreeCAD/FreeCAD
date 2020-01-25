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

#include "DlgPrefsTechDraw3Imp.h"
#include <Gui/PrefWidgets.h>

using namespace TechDrawGui;

DlgPrefsTechDraw3Imp::DlgPrefsTechDraw3Imp( QWidget* parent )
  : PreferencePage( parent )
{
    this->setupUi(this);
    plsb_FontSize->setUnit(Base::Unit::Length);
    plsb_ArrowSize->setUnit(Base::Unit::Length);
}

DlgPrefsTechDraw3Imp::~DlgPrefsTechDraw3Imp()
{
    // no need to delete child widgets, Qt does it all for us
}

void DlgPrefsTechDraw3Imp::saveSettings()
{
    pcbMatting->onSave();
    pcbBalloonShape->onSave();
    cbSectionLineStd->onSave();
    cbPyramidOrtho->onSave();
    pcbCenterStyle->onSave();
    pcbSectionStyle->onSave();
    leLineGroup->onSave();
    pcbBalloonArrow->onSave();
    cbAutoHoriz->onSave();
    leDiameter->onSave();
    pcbArrow->onSave();
    sbAltDecimals->onSave();
    plsb_FontSize->onSave();
    plsb_ArrowSize->onSave();
    leformatSpec->onSave();
    cbGlobalDecimals->onSave();
    cbShowUnits->onSave();
    pcbStandardAndStyle->onSave();
    cbProjAngle->onSave();
    cbHiddenLineStyle->onSave();
    pdsbBalloonKink->onSave();
}

void DlgPrefsTechDraw3Imp::loadSettings()
{
    pcbMatting->onRestore();
    pcbBalloonShape->onRestore();
    cbSectionLineStd->onRestore();
    cbPyramidOrtho->onRestore();
    pcbCenterStyle->onRestore();
    pcbSectionStyle->onRestore();
    leLineGroup->onRestore();
    pcbBalloonArrow->onRestore();
    cbAutoHoriz->onRestore();
    leDiameter->onRestore();
    pcbArrow->onRestore();
    sbAltDecimals->onRestore();
    plsb_FontSize->onRestore();
    plsb_ArrowSize->onRestore();
    leformatSpec->onRestore();
    cbGlobalDecimals->onRestore();
    cbShowUnits->onRestore();
    pcbStandardAndStyle->onRestore();
    cbProjAngle->onRestore();
    cbHiddenLineStyle->onRestore();
    pdsbBalloonKink->onRestore();
}

/**
 * Sets the strings of the subwidgets using the current language.
 */
void DlgPrefsTechDraw3Imp::changeEvent(QEvent *e)
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

#include <Mod/TechDraw/Gui/moc_DlgPrefsTechDraw3Imp.cpp>
