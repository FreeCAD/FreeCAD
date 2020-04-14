/***************************************************************************
 *   Copyright (c) 2005 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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
# include <QLocale>
# include <QMessageBox>
# include <QDoubleValidator>
#endif

#ifndef _PreComp_
#include <cmath>
#endif

#include "DlgSettingsColorGradientImp.h"
#include "ui_DlgSettingsColorGradient.h"
#include "SpinBox.h"

using namespace Gui::Dialog;

/* TRANSLATOR Gui::Dialog::DlgSettingsColorGradientImp */

/**
 *  Constructs a DlgSettingsColorGradientImp as a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'.
 */
DlgSettingsColorGradientImp::DlgSettingsColorGradientImp( QWidget* parent, Qt::WindowFlags fl )
  : QDialog( parent, fl )
  , ui(new Ui_DlgSettingsColorGradient)
{
    ui->setupUi(this);
    fMaxVal = new QDoubleValidator(-1000,1000,ui->spinBoxDecimals->maximum(),this);
    ui->floatLineEditMax->setValidator(fMaxVal);
    fMinVal = new QDoubleValidator(-1000,1000,ui->spinBoxDecimals->maximum(),this);
    ui->floatLineEditMin->setValidator(fMinVal);
}

/**
 *  Destroys the object and frees any allocated resources
 */
DlgSettingsColorGradientImp::~DlgSettingsColorGradientImp()
{
    // no need to delete child widgets, Qt does it all for us
}

void DlgSettingsColorGradientImp::setColorModel( App::ColorGradient::TColorModel tModel)
{
    switch (tModel)
    {
    case App::ColorGradient::TRIA:
        ui->comboBoxModel->setCurrentIndex(0);
        break;
    case App::ColorGradient::INVERSE_TRIA:
        ui->comboBoxModel->setCurrentIndex(1);
        break;
    case App::ColorGradient::GRAY:
        ui->comboBoxModel->setCurrentIndex(2);
        break;
    case App::ColorGradient::INVERSE_GRAY:
        ui->comboBoxModel->setCurrentIndex(3);
        break;
    }
}

App::ColorGradient::TColorModel DlgSettingsColorGradientImp::colorModel() const
{
    int item = ui->comboBoxModel->currentIndex();
    if ( item == 0 )
        return App::ColorGradient::TRIA;
    else if ( item == 1 )
        return App::ColorGradient::INVERSE_TRIA;
    else if ( item == 2 )
        return App::ColorGradient::GRAY;
    else
        return App::ColorGradient::INVERSE_GRAY;
}

void DlgSettingsColorGradientImp::setColorStyle( App::ColorGradient::TStyle tStyle )
{
    switch ( tStyle )
    {
    case App::ColorGradient::FLOW:
        ui->radioButtonFlow->setChecked(true);
        break;
    case App::ColorGradient::ZERO_BASED:
        ui->radioButtonZero->setChecked(true);
        break;
    }
}

App::ColorGradient::TStyle DlgSettingsColorGradientImp::colorStyle() const
{
    return ui->radioButtonZero->isChecked() ? App::ColorGradient::ZERO_BASED : App::ColorGradient::FLOW;
}

void DlgSettingsColorGradientImp::setOutGrayed( bool grayed )
{
    ui->checkBoxGrayed->setChecked( grayed );
}

bool DlgSettingsColorGradientImp::isOutGrayed() const
{
    return ui->checkBoxGrayed->isChecked();
}

void DlgSettingsColorGradientImp::setOutInvisible( bool invisible )
{
    ui->checkBoxInvisible->setChecked( invisible );
}

bool DlgSettingsColorGradientImp::isOutInvisible() const
{
    return ui->checkBoxInvisible->isChecked();
}

void DlgSettingsColorGradientImp::setRange( float fMin, float fMax )
{
    ui->floatLineEditMax->blockSignals(true);
    ui->floatLineEditMax->setText(QLocale().toString(fMax, 'f', numberOfDecimals()));
    ui->floatLineEditMax->blockSignals(false);
    ui->floatLineEditMin->blockSignals(true);
    ui->floatLineEditMin->setText(QLocale().toString(fMin, 'f', numberOfDecimals()));
    ui->floatLineEditMin->blockSignals(false);
}

void DlgSettingsColorGradientImp::getRange(float& fMin, float& fMax) const
{
    fMax = QLocale().toFloat(ui->floatLineEditMax->text());
    fMin = QLocale().toFloat(ui->floatLineEditMin->text());
}

void DlgSettingsColorGradientImp::setNumberOfLabels(int val)
{
    ui->spinBoxLabel->setValue( val );
}

int DlgSettingsColorGradientImp::numberOfLabels() const
{
    return ui->spinBoxLabel->value();
}

void DlgSettingsColorGradientImp::setNumberOfDecimals(int val)
{
    ui->spinBoxDecimals->setValue(val);
}

int DlgSettingsColorGradientImp::numberOfDecimals() const
{
    return ui->spinBoxDecimals->value();
}

void DlgSettingsColorGradientImp::accept()
{
    double fMax = QLocale().toDouble(ui->floatLineEditMax->text());
    double fMin = QLocale().toDouble(ui->floatLineEditMin->text());

    if (fMax <= fMin) {
        QMessageBox::warning(this, tr("Wrong parameter"),
            tr("The maximum value must be higher than the minimum value."));
    }
    else {
        QDialog::accept();
    }
}

#include "moc_DlgSettingsColorGradientImp.cpp"

