// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2017 Abdullah Tahiri
// SPDX-FileNotice: Part of the FreeCAD project.

/******************************************************************************
 *                                                                            *
 *   FreeCAD is free software: you can redistribute it and/or modify          *
 *   it under the terms of the GNU Lesser General Public License as           *
 *   published by the Free Software Foundation, either version 2.1            *
 *   of the License, or (at your option) any later version.                   *
 *                                                                            *
 *   FreeCAD is distributed in the hope that it will be useful,               *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty              *
 *   of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.                  *
 *   See the GNU Lesser General Public License for more details.              *
 *                                                                            *
 *   You should have received a copy of the GNU Lesser General Public         *
 *   License along with FreeCAD. If not, see https://www.gnu.org/licenses     *
 *                                                                            *
 ******************************************************************************/

#include <QDialog>


#include <Gui/MainWindow.h>

#include "SketcherRegularPolygonDialog.h"
#include "ui_SketcherRegularPolygonDialog.h"


using namespace SketcherGui;

SketcherRegularPolygonDialog::SketcherRegularPolygonDialog()
    : QDialog(Gui::getMainWindow())
    , ui(new Ui_SketcherRegularPolygonDialog)
{
    ui->setupUi(this);

    ui->sidesQuantitySpinBox->onRestore();

    updateValues();
}

SketcherRegularPolygonDialog::~SketcherRegularPolygonDialog()
{}

void SketcherRegularPolygonDialog::accept()
{
    ui->sidesQuantitySpinBox->onSave();

    updateValues();

    QDialog::accept();
}

void SketcherRegularPolygonDialog::updateValues()
{
    sides = ui->sidesQuantitySpinBox->value();
}

#include "moc_SketcherRegularPolygonDialog.cpp"
