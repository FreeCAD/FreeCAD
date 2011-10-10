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

#include "DlgInputDialogImp.h"
#include "SpinBox.h"


using namespace Gui::Dialog;

/* TRANSLATOR Gui::Dialog::DlgInputDialogImp */

/**
 *  Constructs a Gui::Dialog::DlgInputDialogImp as a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'.
 *
 *  The dialog will by default be modeless, unless you set 'modal' to
 *  TRUE to construct a modal dialog.
 */
DlgInputDialogImp::DlgInputDialogImp( const QString& labelTxt, QWidget* parent, bool modal, Type type )
  : QDialog( parent )
{
    this->setModal(modal);
    this->setupUi(this);
    label->setText(labelTxt);

    QSize bs = buttonOk->sizeHint().expandedTo(buttonCancel->sizeHint());
    buttonOk->setFixedSize( bs );
    buttonCancel->setFixedSize( bs );

    QSize sh = sizeHint();
    setType(type);
    resize(qMax(sh.width(), 400), 1);

    connect(lineEdit, SIGNAL(returnPressed()), this, SLOT(tryAccept()));
    connect(lineEdit, SIGNAL(textChanged(const QString&)), this, SLOT(textChanged(const QString&)));
}

/**
 *  Destroys the object and frees any allocated resources
 */
DlgInputDialogImp::~DlgInputDialogImp()
{
    // no need to delete child widgets, Qt does it all for us
}

void DlgInputDialogImp::textChanged( const QString &s )
{
    bool on = TRUE;

    if (lineEdit->validator()) {
        QString str = lineEdit->text();
        int index = lineEdit->cursorPosition();
        on = ( lineEdit->validator()->validate(str, index) == QValidator::Acceptable );
    }
    else if ( type() != LineEdit ) {
        on = !s.isEmpty();
    }

    buttonOk->setEnabled( on );
}

void DlgInputDialogImp::tryAccept()
{
    if (!lineEdit->text().isEmpty())
        accept();
}

void DlgInputDialogImp::setType( DlgInputDialogImp::Type t )
{
    inputtype = t;

    QWidget *input = 0;
    switch (inputtype)
    {
    case LineEdit:
        input = lineEdit;
        break;
    case SpinBox:
        input = spinBox;
        break;
    case UIntBox:
        input = uIntSpinBox;
        break;
    case FloatSpinBox:
        input = floatSpinBox;
        break;
    case ComboBox:
        input = comboBox;
        break;
    default:
        break;
    }

    if (input) {
        stack->setCurrentWidget(input->parentWidget());
        stack->setFixedHeight( input->sizeHint().height() );
        input->setFocus();
        label->setBuddy( input );
    }
}

DlgInputDialogImp::Type DlgInputDialogImp::type() const
{
    return inputtype;
}

QSpinBox *DlgInputDialogImp::getSpinBox() const
{
    return spinBox;
}

Gui::UIntSpinBox *DlgInputDialogImp::getUIntBox() const
{
    return uIntSpinBox;
}

QDoubleSpinBox *DlgInputDialogImp::getFloatSpinBox() const
{
    return floatSpinBox;
}

QLineEdit *DlgInputDialogImp::getLineEdit() const
{
    return lineEdit;
}

QComboBox *DlgInputDialogImp::getComboBox() const
{
    return comboBox;
}

#include "moc_DlgInputDialogImp.cpp"
