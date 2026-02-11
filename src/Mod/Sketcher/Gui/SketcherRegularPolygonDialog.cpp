// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileNotice: Part of the FreeCAD project.

/***************************************************************************
 *   Copyright (c) 2017 Abdullah Tahiri <abdullah.tahiri.yo@gmail.com>     *
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
