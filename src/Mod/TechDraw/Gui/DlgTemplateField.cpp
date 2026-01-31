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


#include "DlgTemplateField.h"
#include "ui_DlgTemplateField.h"

#include <QPainter>


using namespace TechDrawGui;

void LineEditFrame::paintEvent(QPaintEvent*)
{
    QPainter p(this);
    QStyleOptionFrame opt;
    initStyleOption(&opt);

    opt.state = focused ? opt.state | QStyle::State_HasFocus : opt.state &= ~QStyle::State_HasFocus;
    style()->drawPrimitive(QStyle::PE_PanelLineEdit, &opt, &p, this);
}

DlgTemplateField::DlgTemplateField( QWidget *parent /* = nullptr */ ) :
    QDialog(parent), templateObj(nullptr),  ui(new Ui_dlgTemplateField)
{
    ui->setupUi(this);

    ui->leAutofill->setReadOnly(true);
    QPalette palette = ui->leAutofill->palette();
    palette.setColor(QPalette::Base, palette.color(QPalette::Disabled, QPalette::Base));
    ui->leAutofill->setPalette(palette);

    connect(qApp, &QApplication::focusChanged, this, &DlgTemplateField::focusChanged);
    connect(ui->btnAutofill, &QAbstractButton::clicked, this, &DlgTemplateField::autofillClicked);
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

void DlgTemplateField::focusChanged(QWidget*, QWidget*)
{
    ui->leFrame->drawFocused(ui->leInput->hasFocus());
    ui->leFrame->repaint();
}

void DlgTemplateField::setTemplate(TechDraw::DrawTemplate* tmplte)
{
    templateObj = tmplte;
}

void DlgTemplateField::setFieldName(QString name)
{
    ui->lblName->setText(name);

    if (templateObj) {
        ui->leInput->bind(templateObj->EditableTexts.getItemPath(name.toStdString()));
    }
}

void DlgTemplateField::setFieldContent(QString content)
{
    ui->leInput->setText(content);
}

void DlgTemplateField::setAutofillContent(QString autofill)
{
    ui->leAutofill->setText(autofill);
}

QString DlgTemplateField::getFieldContent()
{
    return ui->leInput->text();
}

int DlgTemplateField::exec()
{
    if (!ui->leInput->hasExpression()) {
        ui->leInput->setFocus();
    }
    else {
        ui->bbButtons->button(QDialogButtonBox::Cancel)->setFocus();
    }

    ui->btnAutofill->setDisabled(ui->leAutofill->text().isEmpty());

    return QDialog::exec();
}

void DlgTemplateField::autofillClicked(bool) {
    ui->leInput->setExpression(std::shared_ptr<App::Expression>());
    ui->leInput->setText(ui->leAutofill->text());
}

#include <Mod/TechDraw/Gui/moc_DlgTemplateField.cpp>
