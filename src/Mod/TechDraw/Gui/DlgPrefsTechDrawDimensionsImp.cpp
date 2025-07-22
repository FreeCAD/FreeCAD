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
    ui->pdsbLineSpacingFactorISO->onSave();

    enum
    {
        DimensionSingleTool,
        DimensionSeparateTools,
        DimensionBoth
    };

    // Dimensioning constraints mode
    ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath(
        "User parameter:BaseApp/Preferences/Mod/TechDraw/dimensioning");
    bool singleTool = true;
    bool SeparatedTools = false;
    int index = ui->dimensioningMode->currentIndex();
    switch (index) {
    case DimensionSeparateTools:
        singleTool = false;
        SeparatedTools = true;
        break;
    case DimensionBoth:
        singleTool = true;
        SeparatedTools = true;
        break;
    }
    hGrp->SetBool("SingleDimensioningTool", singleTool);
    hGrp->SetBool("SeparatedDimensioningTools", SeparatedTools);

    ui->radiusDiameterMode->setEnabled(index != 1);

    enum
    {
        DimensionAutoRadiusDiam,
        DimensionDiameter,
        DimensionRadius
    };

    bool Diameter = true;
    bool Radius = true;
    index = ui->radiusDiameterMode->currentIndex();
    switch (index) {
    case DimensionDiameter:
        Diameter = true;
        Radius = false;
        break;
    case DimensionRadius:
        Diameter = false;
        Radius = true;
        break;
    }
    hGrp->SetBool("DimensioningDiameter", Diameter);
    hGrp->SetBool("DimensioningRadius", Radius);

    if (property("dimensioningMode").toInt() != ui->dimensioningMode->currentIndex()) {
        requireRestart();
    }
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
    ui->pcbArrow->setCurrentIndex(static_cast<int>(prefArrowStyle()));

    ui->leFormatSpec->setText(QString::fromStdString(Preferences::formatSpec()));
    ui->leFormatSpec->onRestore();

    ui->pdsbGapISO->onRestore();
    ui->pdsbGapASME->onRestore();
    ui->pdsbLineSpacingFactorISO->onRestore();


    // Dimensioning constraints mode
    ui->dimensioningMode->clear();
    ui->dimensioningMode->addItem(tr("Single tool"));
    ui->dimensioningMode->addItem(tr("Separated tools"));
    ui->dimensioningMode->addItem(tr("Both"));

    ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath(
        "User parameter:BaseApp/Preferences/Mod/TechDraw/dimensioning");
    bool singleTool = hGrp->GetBool("SingleDimensioningTool", true);
    bool SeparatedTools = hGrp->GetBool("SeparatedDimensioningTools", false);
    int index = SeparatedTools ? (singleTool ? 2 : 1) : 0;
    ui->dimensioningMode->setCurrentIndex(index);
    setProperty("dimensioningMode", index);
    connect(ui->dimensioningMode,
        QOverload<int>::of(&QComboBox::currentIndexChanged),
        this,
        &DlgPrefsTechDrawDimensionsImp::dimensioningModeChanged);

    ui->radiusDiameterMode->setEnabled(index != 1);

    // Dimensioning constraints mode
    ui->radiusDiameterMode->clear();
    ui->radiusDiameterMode->addItem(tr("Auto"));
    ui->radiusDiameterMode->addItem(tr("Diameter"));
    ui->radiusDiameterMode->addItem(tr("Radius"));

    bool Diameter = hGrp->GetBool("DimensioningDiameter", true);
    bool Radius = hGrp->GetBool("DimensioningRadius", true);
    index = Diameter ? (Radius ? 0 : 1) : 2;
    ui->radiusDiameterMode->setCurrentIndex(index);
}

void DlgPrefsTechDrawDimensionsImp::dimensioningModeChanged(int index)
{
    ui->radiusDiameterMode->setEnabled(index != 1);
}

/**
 * Sets the strings of the subwidgets using the current language.
 */
void DlgPrefsTechDrawDimensionsImp::changeEvent(QEvent *e)
{
    if (e->type() == QEvent::LanguageChange) {
        ui->retranslateUi(this);
    }
    else {
        QWidget::changeEvent(e);
    }
}

void DlgPrefsTechDrawDimensionsImp::resetSettingsToDefaults()
{
    ParameterGrp::handle hGrp;

    hGrp = App::GetApplication().GetParameterGroupByPath(
        "User parameter:BaseApp/Preferences/Mod/TechDraw/dimensioning");
    // reset "Dimension tools" parameters
    hGrp->RemoveBool("SingleDimensioningTool");
    hGrp->RemoveBool("SeparatedDimensioningTools");

    // reset "radius/diameter mode for dimensioning" parameter
    hGrp->RemoveBool("DimensioningDiameter");
    hGrp->RemoveBool("DimensioningRadius");

    // finally reset all the parameters associated to Gui::Pref* widgets
    PreferencePage::resetSettingsToDefaults();
}

TechDraw::ArrowType DlgPrefsTechDrawDimensionsImp::prefArrowStyle() const
{
    return PreferencesGui::dimArrowStyle();
}

#include <Mod/TechDraw/Gui/moc_DlgPrefsTechDrawDimensionsImp.cpp>
