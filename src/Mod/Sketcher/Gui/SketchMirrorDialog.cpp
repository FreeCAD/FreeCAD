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

#include "SketchMirrorDialog.h"
#include "ui_SketchMirrorDialog.h"


using namespace SketcherGui;

SketchMirrorDialog::SketchMirrorDialog()
    : QDialog(Gui::getMainWindow())
    , RefGeoid(-1)
    , RefPosid(Sketcher::PointPos::none)
    , ui(new Ui_SketchMirrorDialog)
{
    ui->setupUi(this);
}

SketchMirrorDialog::~SketchMirrorDialog()
{}

void SketchMirrorDialog::accept()
{
    if (ui->XAxisRadioButton->isChecked()) {
        RefGeoid = Sketcher::GeoEnum::HAxis;
        RefPosid = Sketcher::PointPos::none;
    }
    else if (ui->YAxisRadioButton->isChecked()) {
        RefGeoid = Sketcher::GeoEnum::VAxis;
        RefPosid = Sketcher::PointPos::none;
    }
    else if (ui->OriginRadioButton->isChecked()) {
        RefGeoid = Sketcher::GeoEnum::RtPnt;
        RefPosid = Sketcher::PointPos::start;
    }

    QDialog::accept();
}

#include "moc_SketchMirrorDialog.cpp"
