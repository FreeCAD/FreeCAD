// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2015 Abdullah Tahiri
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

#include "SketchRectangularArrayDialog.h"
#include "ui_SketchRectangularArrayDialog.h"


using namespace SketcherGui;

SketchRectangularArrayDialog::SketchRectangularArrayDialog()
    : QDialog(Gui::getMainWindow())
    , ui(new Ui_SketchRectangularArrayDialog)
{
    ui->setupUi(this);

    ui->RowsQuantitySpinBox->onRestore();
    ui->ColsQuantitySpinBox->onRestore();
    ui->ConstraintSeparationCheckBox->onRestore();
    ui->EqualVerticalHorizontalSpacingCheckBox->onRestore();
    ui->CloneCheckBox->onRestore();

    updateValues();
}

SketchRectangularArrayDialog::~SketchRectangularArrayDialog()
{}

void SketchRectangularArrayDialog::accept()
{
    ui->RowsQuantitySpinBox->onSave();
    ui->ColsQuantitySpinBox->onSave();
    ui->ConstraintSeparationCheckBox->onSave();
    ui->EqualVerticalHorizontalSpacingCheckBox->onSave();
    ui->CloneCheckBox->onSave();

    updateValues();

    QDialog::accept();
}

void SketchRectangularArrayDialog::updateValues()
{
    Rows = ui->RowsQuantitySpinBox->value();
    Cols = ui->ColsQuantitySpinBox->value();
    ConstraintSeparation = ui->ConstraintSeparationCheckBox->isChecked();
    EqualVerticalHorizontalSpacing = ui->EqualVerticalHorizontalSpacingCheckBox->isChecked();
    Clone = ui->CloneCheckBox->isChecked();
}

#include "moc_SketchRectangularArrayDialog.cpp"
