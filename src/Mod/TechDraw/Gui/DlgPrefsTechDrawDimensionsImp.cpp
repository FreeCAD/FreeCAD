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

#include <App/Application.h>

#include <Base/Parameter.h>
#include <Base/Console.h>

#include "DrawGuiUtil.h"
#include "PreferencesGui.h"
#include "DlgPrefsTechDrawDimensionsImp.h"


using namespace TechDrawGui;
using namespace TechDraw;


DlgPrefsTechDrawDimensionsImp::DlgPrefsTechDrawDimensionsImp( QWidget* parent )
  : PreferencePage( parent )
{
    this->setupUi(this);
    plsb_FontSize->setUnit(Base::Unit::Length);
    plsb_FontSize->setMinimum(0);
    plsb_ArrowSize->setUnit(Base::Unit::Length);
    plsb_ArrowSize->setMinimum(0);
}

DlgPrefsTechDrawDimensionsImp::~DlgPrefsTechDrawDimensionsImp()
{
    // no need to delete child widgets, Qt does it all for us
}

void DlgPrefsTechDrawDimensionsImp::saveSettings()
{
    cbGlobalDecimals->onSave();
    cbHiddenLineStyle->onSave();
    cbProjAngle->onSave();
    cbShowUnits->onSave();
    leDiameter->onSave();
    pcbArrow->onSave();
    pcbStandardAndStyle->onSave();
    plsb_ArrowSize->onSave();
    plsb_FontSize->onSave();
    sbAltDecimals->onSave();
}

void DlgPrefsTechDrawDimensionsImp::loadSettings()
{
    //set defaults for Quantity widgets if property not found
    //Quantity widgets do not use preset value since they are based on
    //QAbstractSpinBox
    double fontDefault = Preferences::dimFontSizeMM();
    plsb_FontSize->setValue(fontDefault);
//    double arrowDefault = 5.0;
//    plsb_ArrowSize->setValue(arrowDefault);
    plsb_ArrowSize->setValue(fontDefault);

    cbGlobalDecimals->onRestore();
    cbHiddenLineStyle->onRestore();
    cbProjAngle->onRestore();
    cbShowUnits->onRestore();
    leDiameter->onRestore();
    pcbArrow->onRestore();
    pcbStandardAndStyle->onRestore();
    plsb_ArrowSize->onRestore();
    plsb_FontSize->onRestore();
    sbAltDecimals->onRestore();

    DrawGuiUtil::loadArrowBox(pcbArrow);
    pcbArrow->setCurrentIndex(prefArrowStyle());
}

/**
 * Sets the strings of the subwidgets using the current language.
 */
void DlgPrefsTechDrawDimensionsImp::changeEvent(QEvent *e)
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

int DlgPrefsTechDrawDimensionsImp::prefArrowStyle(void) const
{
    return PreferencesGui::dimArrowStyle();
}

#include <Mod/TechDraw/Gui/moc_DlgPrefsTechDrawDimensionsImp.cpp>
