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

#include "ui_SketchLinearArrayDialog.h"
#include "SketchLinearArrayDialog.h"

using namespace SketcherGui;

SketchLinearArrayDialog::SketchLinearArrayDialog(void)
  : QDialog(Gui::getMainWindow()), ui(new Ui_SketchLinearArrayDialog)
{
    ui->setupUi(this);
    
    ui->RowsQuantitySpinBox->onRestore();
    ui->ColsQuantitySpinBox->onRestore();
    ui->ConstraintSeparationCheckBox->onRestore();
    ui->EqualVerticalHorizontalSpacingCheckBox->onRestore();
    
    updateValues();
}

SketchLinearArrayDialog::~SketchLinearArrayDialog()
{

}

void SketchLinearArrayDialog::accept()
{
    ui->RowsQuantitySpinBox->onSave();
    ui->ColsQuantitySpinBox->onSave();
    ui->ConstraintSeparationCheckBox->onSave();
    ui->EqualVerticalHorizontalSpacingCheckBox->onSave();
    
    updateValues();
    
    QDialog::accept();
}

void SketchLinearArrayDialog::updateValues(void)
{
    Rows = ui->RowsQuantitySpinBox->value();
    Cols = ui->ColsQuantitySpinBox->value();
    ConstraintSeparation = ui->ConstraintSeparationCheckBox->isChecked();
    EqualVerticalHorizontalSpacing = ui->EqualVerticalHorizontalSpacingCheckBox->isChecked();    
}

#include "moc_SketchLinearArrayDialog.cpp"
