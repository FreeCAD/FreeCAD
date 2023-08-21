/***************************************************************************
 *   Copyright (c) 2013 Jürgen Riegel <juergen.riegel@web.de>              *
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
# include <QClipboard>
# include <QLocale>
#endif

#include "DlgUnitsCalculatorImp.h"
#include "ui_DlgUnitsCalculator.h"
#include <Base/UnitsApi.h>

using namespace Gui::Dialog;

/* TRANSLATOR Gui::Dialog::DlgUnitsCalculator */

/**
 *  Constructs a DlgUnitsCalculator which is a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'
 *
 *  The dialog will by default be modeless, unless you set 'modal' to
 *  true to construct a modal dialog.
 */
DlgUnitsCalculator::DlgUnitsCalculator( QWidget* parent, Qt::WindowFlags fl )
  : QDialog(parent, fl), ui(new Ui_DlgUnitCalculator)
{
    // create widgets
    ui->setupUi(this);
    this->setAttribute(Qt::WA_DeleteOnClose);

    ui->comboBoxScheme->addItem(QString::fromLatin1("Preference system"), static_cast<int>(-1));
    int num = static_cast<int>(Base::UnitSystem::NumUnitSystemTypes);
    for (int i=0; i<num; i++) {
        QString item = Base::UnitsApi::getDescription(static_cast<Base::UnitSystem>(i));
        ui->comboBoxScheme->addItem(item, i);
    }

    connect(ui->unitsBox, qOverload<int>(&QComboBox::activated),
            this, &DlgUnitsCalculator::onUnitsBoxActivated);
    connect(ui->comboBoxScheme, qOverload<int>(&QComboBox::activated),
            this, &DlgUnitsCalculator::onComboBoxSchemeActivated);
    connect(ui->spinBoxDecimals, qOverload<int>(&QSpinBox::valueChanged),
            this, &DlgUnitsCalculator::onSpinBoxDecimalsValueChanged);
    connect(ui->ValueInput, qOverload<const Base::Quantity&>(&InputField::valueChanged),
            this, &DlgUnitsCalculator::valueChanged);
    connect(ui->ValueInput, &InputField::returnPressed,
            this, &DlgUnitsCalculator::returnPressed);
    connect(ui->ValueInput, &InputField::parseError, this, &DlgUnitsCalculator::parseError);
    connect(ui->UnitInput, &QLineEdit::textChanged, this, &DlgUnitsCalculator::textChanged);
    connect(ui->UnitInput, &QLineEdit::returnPressed, this, &DlgUnitsCalculator::returnPressed);
    connect(ui->pushButton_Close, &QPushButton::clicked, this, &DlgUnitsCalculator::accept);
    connect(ui->pushButton_Copy, &QPushButton::clicked, this, &DlgUnitsCalculator::copy);

    ui->ValueInput->setParamGrpPath(QByteArray("User parameter:BaseApp/History/UnitsCalculator"));
    // set a default that also illustrates how the dialog works
    ui->ValueInput->setText(QString::fromLatin1("1 cm"));
    ui->UnitInput->setText(QString::fromLatin1("in"));

    units << Base::Unit::Acceleration
          << Base::Unit::AmountOfSubstance
          << Base::Unit::Angle
          << Base::Unit::Area
          << Base::Unit::Density
          << Base::Unit::CurrentDensity
          << Base::Unit::DissipationRate
          << Base::Unit::DynamicViscosity
          << Base::Unit::ElectricalCapacitance
          << Base::Unit::ElectricalInductance
          << Base::Unit::ElectricalConductance
          << Base::Unit::ElectricalResistance
          << Base::Unit::ElectricalConductivity
          << Base::Unit::ElectricCharge
          << Base::Unit::ElectricCurrent
          << Base::Unit::ElectricPotential
          << Base::Unit::Force
          << Base::Unit::Frequency
          << Base::Unit::HeatFlux
          << Base::Unit::InverseArea
          << Base::Unit::InverseLength
          << Base::Unit::InverseVolume
          << Base::Unit::KinematicViscosity
          << Base::Unit::Length
          << Base::Unit::LuminousIntensity
          << Base::Unit::Mass
          << Base::Unit::MagneticFieldStrength
          << Base::Unit::MagneticFlux
          << Base::Unit::MagneticFluxDensity
          << Base::Unit::Magnetization
          << Base::Unit::Power
          << Base::Unit::Pressure
          << Base::Unit::SpecificEnergy
          << Base::Unit::SpecificHeat
          << Base::Unit::Stiffness
          << Base::Unit::Temperature
          << Base::Unit::ThermalConductivity
          << Base::Unit::ThermalExpansionCoefficient
          << Base::Unit::ThermalTransferCoefficient
          << Base::Unit::TimeSpan
          << Base::Unit::VacuumPermittivity
          << Base::Unit::Velocity
          << Base::Unit::Volume
          << Base::Unit::VolumeFlowRate
          << Base::Unit::VolumetricThermalExpansionCoefficient
          << Base::Unit::Work;
    for (const Base::Unit& it : units) {
        ui->unitsBox->addItem(it.getTypeString());
    }

    ui->quantitySpinBox->setValue(1.0);
    ui->quantitySpinBox->setUnit(units.front());
    ui->spinBoxDecimals->setValue(Base::UnitsApi::getDecimals());
}

/** Destroys the object and frees any allocated resources */
DlgUnitsCalculator::~DlgUnitsCalculator() = default;

void DlgUnitsCalculator::accept()
{
    QDialog::accept();
}

void DlgUnitsCalculator::reject()
{
    QDialog::reject();
}

void DlgUnitsCalculator::textChanged(QString unit)
{
    Q_UNUSED(unit)
    valueChanged(actValue);
}

void DlgUnitsCalculator::valueChanged(const Base::Quantity& quant)
{
    // first check the unit, if it is invalid, getTypeString() outputs an empty string
    // explicitly check for "ee" like in "eeV" because this would trigger an exception in Base::Unit
    // since it expects then a scientific notation number like "1e3"
    if ( (ui->UnitInput->text().mid(0, 2) == QString::fromLatin1("ee")) ||
        Base::Unit(ui->UnitInput->text()).getTypeString().isEmpty()) {
        ui->ValueOutput->setText(QString::fromLatin1("%1 %2").arg(tr("unknown unit:"), ui->UnitInput->text()));
        ui->pushButton_Copy->setEnabled(false);
    } else { // the unit is valid
        // we can only convert units of the same type, thus check
        if (Base::Unit(ui->UnitInput->text()).getTypeString() != quant.getUnit().getTypeString()) {
            ui->ValueOutput->setText(tr("unit mismatch"));
            ui->pushButton_Copy->setEnabled(false);
        } else { // the unit is valid and has the same type
            double convertValue = Base::Quantity::parse(QString::fromLatin1("1") + ui->UnitInput->text()).getValue();
            // we got now e.g. for "1 in" the value '25.4' because 1 in = 25.4 mm
            // the result is now just quant / convertValue because the input is always in a base unit
            // (an input of "1 cm" will immediately be converted to "10 mm" by Gui::InputField of the dialog)
            double value = quant.getValue() / convertValue;
            // determine how many decimals we will need to avoid an output like "0.00"
            // at first use scientific notation, if there is no "e", we can round it to the user-defined decimals,
            // but the user-defined decimals might be too low for cases like "10 um" in "in",
            // thus only if value > 0.005 because FC's default are 2 decimals
            QString val = QLocale().toString(value, 'g');
            if (!val.contains(QChar::fromLatin1('e')) && (value > 0.005))
                val = QLocale().toString(value, 'f', Base::UnitsApi::getDecimals());
            // create the output string
            QString out = QString::fromLatin1("%1 %2").arg(val, ui->UnitInput->text());
            ui->ValueOutput->setText(out);
            ui->pushButton_Copy->setEnabled(true);
        }
    }
    // store the input value
    actValue = quant;
}

void DlgUnitsCalculator::parseError(const QString& errorText)
{
    ui->pushButton_Copy->setEnabled(false);
    ui->ValueOutput->setText(errorText);
}

void DlgUnitsCalculator::copy()
{
    QClipboard *cb = QApplication::clipboard();
    cb->setText(ui->ValueOutput->text());
}

void DlgUnitsCalculator::returnPressed()
{
    if (ui->pushButton_Copy->isEnabled()) {
        ui->textEdit->append(ui->ValueInput->text() + QString::fromLatin1(" = ") + ui->ValueOutput->text());
        ui->ValueInput->pushToHistory();
    }
}

void DlgUnitsCalculator::onUnitsBoxActivated(int index)
{
    // SI units use [m], not [mm] for lengths
    //
    Base::Quantity q = ui->quantitySpinBox->value();
    int32_t old = q.getUnit().getSignature().Length;
    double value = q.getValue();

    Base::Unit unit = units[index];
    int32_t len = unit.getSignature().Length;
    ui->quantitySpinBox->setValue(Base::Quantity(value * std::pow(10.0, 3*(len-old)), unit));
}

void DlgUnitsCalculator::onComboBoxSchemeActivated(int index)
{
    int item = ui->comboBoxScheme->itemData(index).toInt();
    if (item > 0)
        ui->quantitySpinBox->setSchema(static_cast<Base::UnitSystem>(item));
    else
        ui->quantitySpinBox->clearSchema();
}

void DlgUnitsCalculator::onSpinBoxDecimalsValueChanged(int value)
{
    ui->quantitySpinBox->setDecimals(value);
}

#include "moc_DlgUnitsCalculatorImp.cpp"
