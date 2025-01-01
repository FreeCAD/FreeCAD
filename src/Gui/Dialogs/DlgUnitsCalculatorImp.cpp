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
#include <QApplication>
#include <QClipboard>
#include <QLocale>
#endif

#include "Dialogs/DlgUnitsCalculatorImp.h"
#include "ui_DlgUnitsCalculator.h"
#include <Base/Quantity.h>
#include <Base/UnitsApi.h>

using namespace Gui::Dialog;
using Base::Quantity;
using Base::Unit;
using Base::UnitsApi;

/* TRANSLATOR Gui::Dialog::DlgUnitsCalculator */

/**
 *  Constructs a DlgUnitsCalculator which is a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'
 *
 *  The dialog will by default be modeless, unless you set 'modal' to
 *  true to construct a modal dialog.
 */
DlgUnitsCalculator::DlgUnitsCalculator(QWidget* parent, Qt::WindowFlags fl)
    : QDialog(parent, fl)
    , ui(new Ui_DlgUnitCalculator)
{
    // create widgets
    ui->setupUi(this);
    this->setAttribute(Qt::WA_DeleteOnClose);

    ui->comboBoxScheme->addItem(QStringLiteral("Preference system"), -1);
    auto addItem = [&, index {0}](const auto& item) mutable {
        ui->comboBoxScheme->addItem(QString::fromStdString(item), index++);
    };
    auto descriptions = UnitsApi::getDescriptions();
    std::for_each(descriptions.begin(), descriptions.end(), addItem);

    // clang-format off
    connect(ui->unitsBox, qOverload<int>(&QComboBox::activated),
            this, &DlgUnitsCalculator::onUnitsBoxActivated);
    connect(ui->comboBoxScheme, qOverload<int>(&QComboBox::activated),
            this, &DlgUnitsCalculator::onComboBoxSchemeActivated);
    connect(ui->spinBoxDecimals, qOverload<int>(&QSpinBox::valueChanged),
            this, &DlgUnitsCalculator::onSpinBoxDecimalsValueChanged);
    connect(ui->ValueInput, qOverload<const Quantity&>(&InputField::valueChanged),
            this, &DlgUnitsCalculator::valueChanged);
    connect(ui->ValueInput, &InputField::returnPressed,
            this, &DlgUnitsCalculator::returnPressed);
    connect(ui->ValueInput, &InputField::parseError,
            this, &DlgUnitsCalculator::parseError);
    connect(ui->UnitInput, &QLineEdit::textChanged,
            this, &DlgUnitsCalculator::textChanged);
    connect(ui->UnitInput, &QLineEdit::returnPressed,
            this, &DlgUnitsCalculator::returnPressed);
    connect(ui->pushButton_Close, &QPushButton::clicked,
            this, &DlgUnitsCalculator::accept);
    connect(ui->pushButton_Copy, &QPushButton::clicked,
            this, &DlgUnitsCalculator::copy);
    // clang-format on

    ui->ValueInput->setParamGrpPath(QByteArray("User parameter:BaseApp/History/UnitsCalculator"));
    // set a default that also illustrates how the dialog works
    ui->ValueInput->setText(QStringLiteral("1 cm"));
    ui->UnitInput->setText(QStringLiteral("in"));

    units << Unit::Acceleration << Unit::AmountOfSubstance << Unit::Angle << Unit::Area
          << Unit::Density << Unit::CurrentDensity << Unit::DissipationRate
          << Unit::DynamicViscosity << Unit::ElectricalCapacitance << Unit::ElectricalInductance
          << Unit::ElectricalConductance << Unit::ElectricalResistance
          << Unit::ElectricalConductivity << Unit::ElectricCharge << Unit::ElectricCurrent
          << Unit::ElectricPotential << Unit::Force << Unit::Frequency << Unit::HeatFlux
          << Unit::InverseArea << Unit::InverseLength << Unit::InverseVolume
          << Unit::KinematicViscosity << Unit::Length << Unit::LuminousIntensity << Unit::Mass
          << Unit::MagneticFieldStrength << Unit::MagneticFlux << Unit::MagneticFluxDensity
          << Unit::Magnetization << Unit::Power << Unit::Pressure << Unit::SpecificEnergy
          << Unit::SpecificHeat << Unit::Stiffness << Unit::Temperature << Unit::ThermalConductivity
          << Unit::ThermalExpansionCoefficient << Unit::ThermalTransferCoefficient << Unit::TimeSpan
          << Unit::VacuumPermittivity << Unit::Velocity << Unit::Volume << Unit::VolumeFlowRate
          << Unit::VolumetricThermalExpansionCoefficient << Unit::Work;
    for (const Unit& it : units) {
        ui->unitsBox->addItem(QString::fromStdString(it.getTypeString()));
    }

    ui->quantitySpinBox->setValue(1.0);
    ui->quantitySpinBox->setUnit(units.front());
    ui->spinBoxDecimals->setValue(UnitsApi::getDecimals());
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

void DlgUnitsCalculator::valueChanged(const Quantity& quant)
{
    std::string unitTypeStr;
    try {
        unitTypeStr =
            Quantity::parse(ui->UnitInput->text().toStdString()).getUnit().getTypeString();
    }
    catch (const Base::ParserError&) {
    }
    // first check the unit, if it is invalid, getTypeString() outputs an empty string
    // explicitly check for "ee" like in "eeV" because this would trigger an exception in Unit
    // since it expects then a scientific notation number like "1e3"
    if ((ui->UnitInput->text().mid(0, 2) == QStringLiteral("ee")) || unitTypeStr.empty()) {
        ui->ValueOutput->setText(
            QStringLiteral("%1 %2").arg(tr("unknown unit:"), ui->UnitInput->text()));
        ui->pushButton_Copy->setEnabled(false);
    }
    else {  // the unit is valid
        // we can only convert units of the same type, thus check
        if (unitTypeStr != quant.getUnit().getTypeString()) {
            ui->ValueOutput->setText(tr("unit mismatch"));
            ui->pushButton_Copy->setEnabled(false);
        }
        else {  // the unit is valid and has the same type
            double convertValue =
                Quantity::parse("1" + ui->UnitInput->text().toStdString()).getValue();
            // we got now e.g. for "1 in" the value '25.4' because 1 in = 25.4 mm
            // the result is now just quant / convertValue because the input is always in a base
            // unit (an input of "1 cm" will immediately be converted to "10 mm" by Gui::InputField
            // of the dialog)
            double value = quant.getValue() / convertValue;
            // determine how many decimals we will need to avoid an output like "0.00"
            // at first use scientific notation, if there is no "e", we can round it to the
            // user-defined decimals, but the user-defined decimals might be too low for cases like
            // "10 um" in "in", thus only if value > 0.005 because FC's default are 2 decimals
            QString val = QLocale().toString(value, 'g');
            if (!val.contains(QChar::fromLatin1('e')) && (value > 0.005)) {
                val = QLocale().toString(value, 'f', UnitsApi::getDecimals());
            }
            // create the output string
            QString out = QStringLiteral("%1 %2").arg(val, ui->UnitInput->text());
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
    QClipboard* cb = QApplication::clipboard();
    cb->setText(ui->ValueOutput->text());
}

void DlgUnitsCalculator::returnPressed()
{
    if (ui->pushButton_Copy->isEnabled()) {
        ui->textEdit->append(ui->ValueInput->text() + QStringLiteral(" = ")
                             + ui->ValueOutput->text());
        ui->ValueInput->pushToHistory();
    }
}

void DlgUnitsCalculator::onUnitsBoxActivated(int index)
{
    // SI units use [m], not [mm] for lengths
    //
    Quantity q = ui->quantitySpinBox->value();
    int32_t old = q.getUnit().length();
    double value = q.getValue();

    Unit unit = units[index];
    int32_t len = unit.length();
    ui->quantitySpinBox->setValue(Quantity(value * std::pow(10.0, 3 * (len - old)), unit));
}

void DlgUnitsCalculator::onComboBoxSchemeActivated(int index)
{
    int item = ui->comboBoxScheme->itemData(index).toInt();
    if (item > 0) {
        ui->quantitySpinBox->setSchema(item);
    }
    else {
        ui->quantitySpinBox->clearSchema();
    }
}

void DlgUnitsCalculator::onSpinBoxDecimalsValueChanged(int value)
{
    ui->quantitySpinBox->setDecimals(value);
}

#include "moc_DlgUnitsCalculatorImp.cpp"
