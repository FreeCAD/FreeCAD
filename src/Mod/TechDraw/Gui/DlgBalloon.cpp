/***************************************************************************
 *   Copyright (c) 2019 Franck Jullien <franck.jullien@gmail.com>          *
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
#include <Base/Console.h>
#include "DlgBalloon.h"

using namespace TechDrawGui;

DlgBalloon::DlgBalloon( QWidget *parent /* = nullptr */ ) :
    QDialog(parent)
{
    setupUi(this);
    inputValue->setFocus();
}

void DlgBalloon::changeEvent(QEvent *e)
{
    if (e->type() == QEvent::LanguageChange) {
        retranslateUi(this);
    }
    else {
        QWidget::changeEvent(e);
    }
}

void DlgBalloon::setValue(std::string value)
{
    QString qs = QString::fromUtf8(value.data(), value.size());
    inputValue->setText(qs);
}

QString DlgBalloon::getValue(void)
{
    return inputValue->text();
}

void DlgBalloon::setScale(double value)
{
    QString qs = QString::number(value, 'f', 2);
    inputScale->setText(qs);
}

double DlgBalloon::getScale(void)
{
    return inputScale->text().toDouble();
}

void DlgBalloon::accept()
{
    QDialog::accept();
}

void DlgBalloon::reject()
{
    QDialog::reject();
}

void DlgBalloon::populateComboBox(QComboBox *box, const char **values, const char *setVal)
{
    QStringList symbols;
    int i = 0;

    while (values[i] != NULL)
        symbols << QString::fromUtf8(values[i++]);

    box->addItems(symbols);
    i = box->findText(QString::fromUtf8(setVal));
    box->setCurrentIndex(i);
}

#include <Mod/TechDraw/Gui/moc_DlgBalloon.cpp>
