/***************************************************************************
 *   Copyright (c) 2006 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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
# include <QPushButton>
#endif

#include "DlgInputDialogImp.h"
#include "ui_DlgInputDialog.h"
#include "SpinBox.h"


using namespace Gui::Dialog;

/* TRANSLATOR Gui::Dialog::DlgInputDialogImp */

/**
 *  Constructs a Gui::Dialog::DlgInputDialogImp as a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'.
 *
 *  The dialog will by default be modeless, unless you set 'modal' to
 *  true to construct a modal dialog.
 */
DlgInputDialogImp::DlgInputDialogImp( const QString& labelTxt, QWidget* parent, bool modal, Type type )
  : QDialog(parent)
  , ui(new Ui_DlgInputDialog)
{
    this->setModal(modal);
    ui->setupUi(this);
    ui->label->setText(labelTxt);

    QSize sh = sizeHint();
    setType(type);
    resize(qMax(sh.width(), 400), 1);

    connect(ui->lineEdit, &QLineEdit::returnPressed, this, &DlgInputDialogImp::tryAccept);
    connect(ui->lineEdit, &QLineEdit::textChanged, this, &DlgInputDialogImp::textChanged);
}

/**
 *  Destroys the object and frees any allocated resources
 */
DlgInputDialogImp::~DlgInputDialogImp() = default;

void DlgInputDialogImp::textChanged( const QString &s )
{
    bool on = true;

    if (ui->lineEdit->validator()) {
        QString str = ui->lineEdit->text();
        int index = ui->lineEdit->cursorPosition();
        on = ( ui->lineEdit->validator()->validate(str, index) == QValidator::Acceptable );
    }
    else if ( type() != LineEdit ) {
        on = !s.isEmpty();
    }

    ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled( on );
}

void DlgInputDialogImp::tryAccept()
{
    if (!ui->lineEdit->text().isEmpty())
        accept();
}

void DlgInputDialogImp::setType( DlgInputDialogImp::Type t )
{
    inputtype = t;

    QWidget *input = nullptr;
    switch (inputtype)
    {
    case LineEdit:
        input = ui->lineEdit;
        break;
    case SpinBox:
        input = ui->spinBox;
        break;
    case UIntBox:
        input = ui->uIntSpinBox;
        break;
    case FloatSpinBox:
        input = ui->floatSpinBox;
        break;
    case ComboBox:
        input = ui->comboBox;
        break;
    default:
        break;
    }

    if (input) {
        ui->stack->setCurrentWidget(input->parentWidget());
        ui->stack->setFixedHeight( input->sizeHint().height() );
        input->setFocus();
        ui->label->setBuddy( input );
    }
}

DlgInputDialogImp::Type DlgInputDialogImp::type() const
{
    return inputtype;
}

QSpinBox *DlgInputDialogImp::getSpinBox() const
{
    return ui->spinBox;
}

Gui::UIntSpinBox *DlgInputDialogImp::getUIntBox() const
{
    return ui->uIntSpinBox;
}

QDoubleSpinBox *DlgInputDialogImp::getFloatSpinBox() const
{
    return ui->floatSpinBox;
}

QLineEdit *DlgInputDialogImp::getLineEdit() const
{
    return ui->lineEdit;
}

QComboBox *DlgInputDialogImp::getComboBox() const
{
    return ui->comboBox;
}

#include "moc_DlgInputDialogImp.cpp"
