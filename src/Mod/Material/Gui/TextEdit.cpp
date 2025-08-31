/***************************************************************************
 *   Copyright (c) 2023 David Carter <dcarter@david.carter.ca>             *
 *                                                                         *
 *   This file is part of FreeCAD.                                         *
 *                                                                         *
 *   FreeCAD is free software: you can redistribute it and/or modify it    *
 *   under the terms of the GNU Lesser General Public License as           *
 *   published by the Free Software Foundation, either version 2.1 of the  *
 *   License, or (at your option) any later version.                       *
 *                                                                         *
 *   FreeCAD is distributed in the hope that it will be useful, but        *
 *   WITHOUT ANY WARRANTY; without even the implied warranty of            *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU      *
 *   Lesser General Public License for more details.                       *
 *                                                                         *
 *   You should have received a copy of the GNU Lesser General Public      *
 *   License along with FreeCAD. If not, see                               *
 *   <https://www.gnu.org/licenses/>.                                      *
 *                                                                         *
 **************************************************************************/

#include "PreCompiled.h"
#ifndef _PreComp_
#include <QMenu>
#include <QMessageBox>
#endif

#include <Gui/MainWindow.h>

#include <Mod/Material/App/Exceptions.h>
#include <Mod/Material/App/Materials.h>

#include "TextEdit.h"
#include "ui_TextEdit.h"


using namespace MatGui;

/* TRANSLATOR MatGui::TextEdit */

TextEdit::TextEdit(const QString& propertyName,
                   const std::shared_ptr<Materials::Material>& material,
                   QWidget* parent)
    : QDialog(parent)
    , ui(new Ui_TextEdit)
    , _material(material)
{
    ui->setupUi(this);

    if (material->hasPhysicalProperty(propertyName)) {
        _property = material->getPhysicalProperty(propertyName);
    }
    else if (material->hasAppearanceProperty(propertyName)) {
        _property = material->getAppearanceProperty(propertyName);
    }
    else {
        Base::Console().log("Property '%s' not found\n", propertyName.toStdString().c_str());
        _property = nullptr;
    }
    if (_property) {
        _value = _property->getString();
    }
    else {
        Base::Console().log("No value loaded\n");
        _value = QString();
    }

    ui->textEdit->setText(_value);
    ui->textEdit->setAcceptRichText(false);
    ui->textEdit->setWordWrapMode(QTextOption::NoWrap);

    connect(ui->standardButtons, &QDialogButtonBox::accepted, this, &TextEdit::accept);
    connect(ui->standardButtons, &QDialogButtonBox::rejected, this, &TextEdit::reject);
}

void TextEdit::accept()
{
    QString newText = ui->textEdit->toPlainText();
    if (newText != _value) {
        _property->setValue(ui->textEdit->toPlainText());
        _material->setEditStateAlter();
    }
    QDialog::accept();
}

void TextEdit::reject()
{
    QDialog::reject();
}

#include "moc_TextEdit.cpp"
