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
#include "DlgPrefsTechDraw3Imp.h"


using namespace TechDrawGui;
using namespace TechDraw;


DlgPrefsTechDraw3Imp::DlgPrefsTechDraw3Imp( QWidget* parent )
  : PreferencePage( parent )
{
    this->setupUi(this);
    plsb_FontSize->setUnit(Base::Unit::Length);
    plsb_FontSize->setMinimum(0);
    plsb_ArrowSize->setUnit(Base::Unit::Length);
    plsb_ArrowSize->setMinimum(0);
    pdsbBalloonKink->setUnit(Base::Unit::Length);
    pdsbBalloonKink->setMinimum(0);
}

DlgPrefsTechDraw3Imp::~DlgPrefsTechDraw3Imp()
{
    // no need to delete child widgets, Qt does it all for us
}

void DlgPrefsTechDraw3Imp::saveSettings()
{
    cbAutoHoriz->onSave();
    cbGlobalDecimals->onSave();
    cbHiddenLineStyle->onSave();
    cbPrintCenterMarks->onSave();
    cbProjAngle->onSave();
    cbPyramidOrtho->onSave();
    cbSectionLineStd->onSave();
    cbShowCenterMarks->onSave();
    cbShowUnits->onSave();
    leDiameter->onSave();
    leformatSpec->onSave();
    leLineGroup->onSave();
    pcbArrow->onSave();
    pcbBalloonArrow->onSave();
    pcbBalloonShape->onSave();
    pcbCenterStyle->onSave();
    pcbMatting->onSave();
    pcbSectionStyle->onSave();
    pcbStandardAndStyle->onSave();
    pdsbBalloonKink->onSave();
    plsb_ArrowSize->onSave();
    plsb_FontSize->onSave();
    sbAltDecimals->onSave();
    cbCutSurface->onSave();
}

void DlgPrefsTechDraw3Imp::loadSettings()
{
    //set defaults for Quantity widgets if property not found
    //Quantity widgets do not use preset value since they are based on
    //QAbstractSpinBox
    double kinkDefault = 5.0;
    pdsbBalloonKink->setValue(kinkDefault);
    double arrowDefault = 5.0;
    plsb_ArrowSize->setValue(arrowDefault);
    double fontDefault = 4.0;
    plsb_FontSize->setValue(fontDefault);

    cbAutoHoriz->onRestore();
    cbGlobalDecimals->onRestore();
    cbHiddenLineStyle->onRestore();
    cbPrintCenterMarks->onRestore();
    cbProjAngle->onRestore();
    cbPyramidOrtho->onRestore();
    cbSectionLineStd->onRestore();
    cbShowCenterMarks->onRestore();
    cbShowUnits->onRestore();
    leDiameter->onRestore();
    leformatSpec->onRestore();
    leLineGroup->onRestore();
    pcbArrow->onRestore();
    pcbBalloonArrow->onRestore();
    pcbBalloonShape->onRestore();
    pcbCenterStyle->onRestore();
    pcbMatting->onRestore();
    pcbSectionStyle->onRestore();
    pcbStandardAndStyle->onRestore();
    pdsbBalloonKink->onRestore();
    plsb_ArrowSize->onRestore();
    plsb_FontSize->onRestore();
    sbAltDecimals->onRestore();
    cbCutSurface->onRestore();

    DrawGuiUtil::loadArrowBox(pcbBalloonArrow);
    pcbBalloonArrow->setCurrentIndex(prefBalloonArrow());
    DrawGuiUtil::loadArrowBox(pcbArrow);
    pcbArrow->setCurrentIndex(prefArrowStyle());
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

int DlgPrefsTechDraw3Imp::prefBalloonArrow(void) const
{
    Base::Reference<ParameterGrp> hGrp = App::GetApplication().GetUserParameter().
                                         GetGroup("BaseApp")->GetGroup("Preferences")->
                                         GetGroup("Mod/TechDraw/Decorations");
    int end = hGrp->GetInt("BalloonArrow", 0);
    return end;
}

int DlgPrefsTechDraw3Imp::prefArrowStyle(void) const
{
    Base::Reference<ParameterGrp> hGrp = App::GetApplication().GetUserParameter().
                                         GetGroup("BaseApp")->GetGroup("Preferences")->
                                         GetGroup("Mod/TechDraw/Dimensions");
    int style = hGrp->GetInt("ArrowStyle", 0);
    return style;
}



#include <Mod/TechDraw/Gui/moc_DlgPrefsTechDraw3Imp.cpp>
