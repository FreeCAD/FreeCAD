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

    connect(ui->ValueInput, SIGNAL(valueChanged(Base::Quantity)), this, SLOT(valueChanged(Base::Quantity)));
    connect(ui->ValueInput, SIGNAL(returnPressed () ), this, SLOT(returnPressed()));
    connect(ui->UnitInput, SIGNAL(valueChanged(Base::Quantity)), this, SLOT(unitValueChanged(Base::Quantity)));
    connect(ui->UnitInput, SIGNAL(returnPressed()), this, SLOT(returnPressed()));

    connect(ui->pushButton_Help, SIGNAL(clicked()), this, SLOT(help()));
    connect(ui->pushButton_Close, SIGNAL(clicked()), this, SLOT(accept()));
    connect(ui->pushButton_Copy, SIGNAL(clicked()), this, SLOT(copy()));

    connect(ui->ValueInput, SIGNAL(parseError(QString)), this, SLOT(parseError(QString)));
    connect(ui->UnitInput, SIGNAL(parseError(QString)), this, SLOT(parseError(QString)));

    ui->ValueInput->setParamGrpPath(QByteArray("User parameter:BaseApp/History/UnitsCalculator"));
    actUnit.setInvalid();

    units << Base::Unit::Length << Base::Unit::Mass << Base::Unit::Angle << Base::Unit::Density
          << Base::Unit::Area << Base::Unit::Volume << Base::Unit::TimeSpan << Base::Unit::Velocity
          << Base::Unit::Acceleration << Base::Unit::Temperature
          << Base::Unit::ElectricCurrent << Base::Unit::ElectricPotential
          << Base::Unit::AmountOfSubstance << Base::Unit::LuminousIntensity << Base::Unit::Stress
          << Base::Unit::Pressure << Base::Unit::Force << Base::Unit::Work << Base::Unit::Power
          << Base::Unit::ThermalConductivity << Base::Unit::ThermalExpansionCoefficient
          << Base::Unit::SpecificHeat << Base::Unit::ThermalTransferCoefficient <<Base::Unit::HeatFlux;
    for (QList<Base::Unit>::iterator it = units.begin(); it != units.end(); ++it) {
        ui->unitsBox->addItem(it->getTypeString());
    }

    ui->quantitySpinBox->setUnit(units.front());
}

/** Destroys the object and frees any allocated resources */
DlgUnitsCalculator::~DlgUnitsCalculator()
{
}

void DlgUnitsCalculator::accept()
{
    QDialog::accept();
}

void DlgUnitsCalculator::reject()
{
    QDialog::reject();
}

void DlgUnitsCalculator::unitValueChanged(const Base::Quantity& unit)
{
    actUnit = unit;
    valueChanged(actValue);
}

void DlgUnitsCalculator::valueChanged(const Base::Quantity& quant)
{
    if (actUnit.isValid()) {
        if (actUnit.getUnit() != quant.getUnit()) {
            ui->ValueOutput->setText(tr("Unit mismatch"));
            ui->pushButton_Copy->setEnabled(false);
        } else {
            double value = quant.getValue()/actUnit.getValue();
            QString val = QLocale::system().toString(value, 'f', Base::UnitsApi::getDecimals());
            QString out = QString::fromLatin1("%1 %2").arg(val, ui->UnitInput->text());
            ui->ValueOutput->setText(out);
            ui->pushButton_Copy->setEnabled(true);
        }
    } else {
        //ui->ValueOutput->setValue(quant);
        ui->ValueOutput->setText(quant.getUserString());
        ui->pushButton_Copy->setEnabled(true);
    }

    actValue = quant;
}

void DlgUnitsCalculator::parseError(const QString& errorText)
{
    ui->pushButton_Copy->setEnabled(false);
    ui->ValueOutput->setText(errorText);
}

void DlgUnitsCalculator::copy(void)
{
    QClipboard *cb = QApplication::clipboard();
    cb->setText(ui->ValueOutput->text());
}

void DlgUnitsCalculator::help(void)
{
    //TODO: call help page Std_UnitsCalculator
}

void DlgUnitsCalculator::returnPressed(void)
{
    if (ui->pushButton_Copy->isEnabled()) {
        ui->textEdit->append(ui->ValueInput->text() + QString::fromLatin1(" = ") + ui->ValueOutput->text());
        ui->ValueInput->pushToHistory();
    }
}

void DlgUnitsCalculator::on_unitsBox_activated(int index)
{
    ui->quantitySpinBox->setUnit(units[index]);
}

#include "moc_DlgUnitsCalculatorImp.cpp"
