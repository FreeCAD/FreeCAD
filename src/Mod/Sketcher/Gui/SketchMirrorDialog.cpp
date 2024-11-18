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
#include <QDialog>
#endif

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
