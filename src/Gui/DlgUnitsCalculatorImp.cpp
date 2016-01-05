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
#include <Base/UnitsApi.h>

using namespace Gui::Dialog;

/* TRANSLATOR Gui::Dialog::DlgUnitsCalculator */

/**
 *  Constructs a DlgUnitsCalculator which is a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'
 *
 *  The dialog will by default be modeless, unless you set 'modal' to
 *  TRUE to construct a modal dialog.
 */
DlgUnitsCalculator::DlgUnitsCalculator( QWidget* parent, Qt::WindowFlags fl )
  : QDialog( parent, fl )
{
    // create widgets
    setupUi(this);
    this->setAttribute(Qt::WA_DeleteOnClose);

    connect(this->ValueInput, SIGNAL(valueChanged(Base::Quantity)), this, SLOT(valueChanged(Base::Quantity)));
    connect(this->ValueInput, SIGNAL(returnPressed () ), this, SLOT(returnPressed()));
    connect(this->UnitInput, SIGNAL(valueChanged(Base::Quantity)), this, SLOT(unitValueChanged(Base::Quantity)));
    connect(this->UnitInput, SIGNAL(returnPressed()), this, SLOT(returnPressed()));

    connect(this->pushButton_Help, SIGNAL(clicked()), this, SLOT(help()));
    connect(this->pushButton_Close, SIGNAL(clicked()), this, SLOT(accept()));
    connect(this->pushButton_Copy, SIGNAL(clicked()), this, SLOT(copy()));

    connect(this->ValueInput, SIGNAL(parseError(QString)), this, SLOT(parseError(QString)));
    connect(this->UnitInput, SIGNAL(parseError(QString)), this, SLOT(parseError(QString)));

    this->ValueInput->setParamGrpPath(QByteArray("User parameter:BaseApp/History/UnitsCalculator"));
    actUnit.setInvalid();
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
            this->ValueOutput->setText(tr("Unit mismatch"));
            this->pushButton_Copy->setEnabled(false);
        } else {
            double value = quant.getValue()/actUnit.getValue();
            QString val = QLocale::system().toString(value, 'f', Base::UnitsApi::getDecimals());
            QString out = QString::fromLatin1("%1 %2").arg(val).arg(this->UnitInput->text());
            this->ValueOutput->setText(out);
            this->pushButton_Copy->setEnabled(true);
        }
    } else {
        //this->ValueOutput->setValue(quant);
        this->ValueOutput->setText(quant.getUserString());
        this->pushButton_Copy->setEnabled(true);
    }

    actValue = quant;
}

void DlgUnitsCalculator::parseError(const QString& errorText)
{
    this->pushButton_Copy->setEnabled(false);
    this->ValueOutput->setText(errorText);
}

void DlgUnitsCalculator::copy(void)
{
    QClipboard *cb = QApplication::clipboard();
    cb->setText(ValueOutput->text());
}

void DlgUnitsCalculator::help(void)
{
    //TODO: call help page Std_UnitsCalculator
}

void DlgUnitsCalculator::returnPressed(void)
{
    if (this->pushButton_Copy->isEnabled()) {
        this->textEdit->append(this->ValueInput->text() + QString::fromLatin1(" = ") + this->ValueOutput->text());
        this->ValueInput->pushToHistory();
    }
}



#include "moc_DlgUnitsCalculatorImp.cpp"
