/***************************************************************************
 *   Copyright (c) 2007 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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
#endif

#include "MaterialsEditor.h"
#include "ui_MaterialsEditor.h"


using namespace MatGui;

/* TRANSLATOR MatGui::MaterialsEditor */

MaterialsEditor::MaterialsEditor(QWidget* parent)
  : QDialog(parent), ui(new Ui_MaterialsEditor)
{
    ui->setupUi(this);
    connect(ui->standardButtons, &QDialogButtonBox::accepted,
            this, &MaterialsEditor::accept);
    connect(ui->standardButtons, &QDialogButtonBox::rejected,
            this, &MaterialsEditor::reject);
}

/*
 *  Destroys the object and frees any allocated resources
 */
MaterialsEditor::~MaterialsEditor()
{
    // no need to delete child widgets, Qt does it all for us
}

void MaterialsEditor::accept()
{
    reject();
}

void MaterialsEditor::reject()
{
    QDialog::reject();
    auto dw = qobject_cast<QDockWidget*>(parent());
    if (dw) {
        dw->deleteLater();
    }
}

#include "moc_MaterialsEditor.cpp"
