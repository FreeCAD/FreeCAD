// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2023 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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
# include <QDialog>
# include <map>
#endif

#include <Base/Tools.h>
#include <App/Document.h>
#include <Gui/BitmapFactory.h>
#include <Gui/Camera.h>
#include <Gui/TaskView/TaskView.h>

#include "TaskImageScale.h"
#include "ui_TaskImageScale.h"


using namespace Gui;

TaskImageScale::TaskImageScale(App::ImagePlane* obj, QWidget* parent)
  : QWidget(parent)
  , ui(new Ui_TaskImageScale)
  , feature(obj)
  , aspectRatio{1.0}
{
    ui->setupUi(this);
    ui->spinBoxWidth->setValue(obj->getXSizeInPixel());
    ui->spinBoxHeight->setValue(obj->getYSizeInPixel());

    aspectRatio = obj->XSize.getValue() / obj->YSize.getValue();

    connect(ui->spinBoxWidth, qOverload<int>(&QSpinBox::valueChanged), this, &TaskImageScale::changeWidth);
    connect(ui->spinBoxHeight, qOverload<int>(&QSpinBox::valueChanged), this, &TaskImageScale::changeHeight);
}

TaskImageScale::~TaskImageScale()
{
}

void TaskImageScale::changeWidth()
{
    if (!feature.expired()) {
        int value = ui->spinBoxWidth->value();
        feature->setXSizeInPixel(value);

        if (ui->checkBoxRatio->isChecked()) {
            QSignalBlocker block(ui->spinBoxWidth);
            ui->spinBoxHeight->setValue(int(double(value) / aspectRatio));
        }
    }
}

void TaskImageScale::changeHeight()
{
    if (!feature.expired()) {
        int value = ui->spinBoxHeight->value();
        feature->setYSizeInPixel(value);

        if (ui->checkBoxRatio->isChecked()) {
            QSignalBlocker block(ui->spinBoxHeight);
            ui->spinBoxWidth->setValue(int(double(value) * aspectRatio));
        }
    }
}

#include "moc_TaskImageScale.cpp"
