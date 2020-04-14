/***************************************************************************
 *   Copyright (c) 2015 Abdullah Tahiri <abdullah.tahiri.yo@gmail.com>     *
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
# include <QPixmap>
# include <QDialog>
#endif

#include <Gui/BitmapFactory.h>
#include <Gui/MainWindow.h>
#include <Base/Tools.h>
#include <Base/UnitsApi.h>

#include "ui_SketchRectangularArrayDialog.h"
#include "SketchRectangularArrayDialog.h"

using namespace SketcherGui;

SketchRectangularArrayDialog::SketchRectangularArrayDialog(void)
  : QDialog(Gui::getMainWindow()), ui(new Ui_SketchRectangularArrayDialog)
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
{
    delete ui;
}

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

void SketchRectangularArrayDialog::updateValues(void)
{
    Rows = ui->RowsQuantitySpinBox->value();
    Cols = ui->ColsQuantitySpinBox->value();
    ConstraintSeparation = ui->ConstraintSeparationCheckBox->isChecked();
    EqualVerticalHorizontalSpacing = ui->EqualVerticalHorizontalSpacingCheckBox->isChecked();
    Clone = ui->CloneCheckBox->isChecked();    
}

#include "moc_SketchRectangularArrayDialog.cpp"
