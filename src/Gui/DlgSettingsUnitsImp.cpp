/***************************************************************************
 *   Copyright (c) 2010 Jürgen Riegel <juergen.riegel@web.de>              *
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

#ifndef _PreComp_
# include <QApplication>
# include <QRegExp>
#endif

#include <limits>

#include "ui_DlgSettingsUnits.h"
#include "DlgSettingsUnitsImp.h"
#include "NavigationStyle.h"
#include "PrefWidgets.h"
#include <App/Application.h>
#include <Base/Console.h>
#include <Base/Parameter.h>
#include <Base/UnitsApi.h>

using namespace Gui::Dialog;
using namespace Base;

/* TRANSLATOR Gui::Dialog::DlgSettingsUnitsImp */

#if 0 // needed for Qt's lupdate utility
    qApp->translate("Gui::Dialog::DlgSettingsUnits", "Standard (mm/kg/s/degree)");
    qApp->translate("Gui::Dialog::DlgSettingsUnits", "MKS (m/kg/s/degree)");
    qApp->translate("Gui::Dialog::DlgSettingsUnits", "US customary (in/lb)");
    qApp->translate("Gui::Dialog::DlgSettingsUnits", "Imperial decimal (in/lb)");
    qApp->translate("Gui::Dialog::DlgSettingsUnits", "Building Euro (cm/m²/m³)");
    qApp->translate("Gui::Dialog::DlgSettingsUnits", "Building US (ft-in/sqft/cft)");
    qApp->translate("Gui::Dialog::DlgSettingsUnits", "Metric small parts & CNC(mm, mm/min)");
    qApp->translate("Gui::Dialog::DlgSettingsUnits", "Imperial for Civil Eng (ft, ft/sec)");
    qApp->translate("Gui::Dialog::DlgSettingsUnits", "FEM (mm, N, sec)");
#endif

/**
 *  Constructs a DlgSettingsUnitsImp which is a child of 'parent', with the 
 *  name 'name' and widget flags set to 'f' 
 */
DlgSettingsUnitsImp::DlgSettingsUnitsImp(QWidget* parent)
    : PreferencePage( parent ), ui(new Ui_DlgSettingsUnits)
{
    ui->setupUi(this);
    ui->spinBoxDecimals->setMaximum(std::numeric_limits<double>::digits10 + 1);

    int num = static_cast<int>(Base::UnitSystem::NumUnitSystemTypes);
    for (int i = 0; i < num; i++) {
        QString item = qApp->translate("Gui::Dialog::DlgSettingsUnits", Base::UnitsApi::getDescription(static_cast<Base::UnitSystem>(i)));
        ui->comboBox_ViewSystem->addItem(item, i);
    }

    //fillUpListBox();
    ui->tableWidget->setVisible(false);
    //
    // Enable/disable the fractional inch option depending on system
    if( UnitsApi::getSchema() == UnitSystem::ImperialBuilding )
    {
        ui->comboBox_FracInch->setEnabled(true);
    }
    else
    {
        ui->comboBox_FracInch->setEnabled(false);
    }
}

/** 
 *  Destroys the object and frees any allocated resources
 */
DlgSettingsUnitsImp::~DlgSettingsUnitsImp()
{
    // no need to delete child widgets, Qt does it all for us
    delete ui;
}

void DlgSettingsUnitsImp::on_comboBox_ViewSystem_currentIndexChanged(int index)
{
    if (index < 0)
        return; // happens when clearing the combo box in retranslateUi()

    // Enable/disable the fractional inch option depending on system
    if( (UnitSystem)index == UnitSystem::ImperialBuilding )
    {
        ui->comboBox_FracInch->setEnabled(true);
    }
    else
    {
        ui->comboBox_FracInch->setEnabled(false);
    }
}

void DlgSettingsUnitsImp::saveSettings()
{
    // must be done as very first because we create a new instance of NavigatorStyle
    // where we set some attributes afterwards
    int FracInch;  // minimum fractional inch to display
    int viewSystemIndex; // currently selected View System (unit system)

    ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath
        ("User parameter:BaseApp/Preferences/Units");
    hGrp->SetInt("UserSchema", ui->comboBox_ViewSystem->currentIndex());
    hGrp->SetInt("Decimals", ui->spinBoxDecimals->value());

    // Set actual value
    Base::UnitsApi::setDecimals(ui->spinBoxDecimals->value());
    
    // Convert the combobox index to the its integer denominator. Currently
    // with 1/2, 1/4, through 1/128, this little equation directly computes the
    // denominator given the combobox integer.
    //
    // The inverse conversion is done when loaded. That way only one thing (the
    // numerical fractional inch value) needs to be stored.
    FracInch = std::pow(2, ui->comboBox_FracInch->currentIndex() + 1);
    hGrp->SetInt("FracInch", FracInch);

    // Set the actual format value
    Base::QuantityFormat::setDefaultDenominator(FracInch);

    // Set and save the Unit System
    viewSystemIndex = ui->comboBox_ViewSystem->currentIndex();
    UnitsApi::setSchema((UnitSystem)viewSystemIndex);
}

void DlgSettingsUnitsImp::loadSettings()
{
    int FracInch;
    int cbIndex;

    ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath
        ("User parameter:BaseApp/Preferences/Units");
    ui->comboBox_ViewSystem->setCurrentIndex(hGrp->GetInt("UserSchema",0));
    ui->spinBoxDecimals->setValue(hGrp->GetInt("Decimals",Base::UnitsApi::getDecimals()));

    // Get the current user setting for the minimum fractional inch
    FracInch = hGrp->GetInt("FracInch", Base::QuantityFormat::getDefaultDenominator());

    // Convert fractional inch to the corresponding combobox index using this
    // handy little equation.
    cbIndex = std::log2(FracInch) - 1;
    ui->comboBox_FracInch->setCurrentIndex(cbIndex);
}

/**
 * Sets the strings of the subwidgets using the current language.
 */
void DlgSettingsUnitsImp::changeEvent(QEvent *e)
{
    if (e->type() == QEvent::LanguageChange) {
        int index = ui->comboBox_ViewSystem->currentIndex();
        ui->retranslateUi(this);
        ui->comboBox_ViewSystem->setCurrentIndex(index);
    }
    else {
        QWidget::changeEvent(e);
    }
}

#include "moc_DlgSettingsUnitsImp.cpp"
