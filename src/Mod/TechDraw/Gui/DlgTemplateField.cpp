/***************************************************************************
 *   Copyright (c) 2016 WandererFan <wandererfan@gmail.com>                *
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

#include "DlgTemplateField.h"
#include "ui_DlgTemplateField.h"


using namespace TechDrawGui;

DlgTemplateField::DlgTemplateField( QWidget *parent /* = nullptr */ ) :
    QDialog(parent), ui(new Ui_dlgTemplateField)
{
    ui->setupUi(this);
    ui->leInput->setFocus();
}

void DlgTemplateField::changeEvent(QEvent *e)
{
    if (e->type() == QEvent::LanguageChange) {
        ui->retranslateUi(this);
    }
    else {
        QWidget::changeEvent(e);
    }
}

void DlgTemplateField::setFieldName(std::string name)
{
    QString qs = QString::fromUtf8(name.data(), name.size());
    ui->lblName->setText(qs);
}

void DlgTemplateField::setFieldContent(std::string content)
{
    QString qs = QString::fromUtf8(content.data(), content.size());
    ui->leInput->setText(qs);
}

QString DlgTemplateField::getFieldContent()
{
    return ui->leInput->text();
}

void DlgTemplateField::accept()
{
    QDialog::accept();
}

void DlgTemplateField::reject()
{
    QDialog::reject();
}

#include <Mod/TechDraw/Gui/moc_DlgTemplateField.cpp>
