/***************************************************************************
 *   Copyright (c) 2010 Jürgen Riegel <juergen.riegel@web.de>              *
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

#include "ui_DlgLocationAngle.h"
#include "ui_DlgLocationPos.h"

#include "TaskDlgRelocation.h"
#include "BitmapFactory.h"


using namespace Gui;

//**************************************************************************
//**************************************************************************
// TaskBoxPosition
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

/* TRANSLATOR Gui::TaskBoxPosition */

TaskBoxPosition::TaskBoxPosition(QWidget *parent)
    : TaskBox(Gui::BitmapFactory().pixmap("Robot_CreateRobot"),tr("Position"),true, parent)
{
    // we need a separate container widget to add all controls to
    proxy = new QWidget(this);
    ui = new Ui_Position();
    ui->setupUi(proxy);
    QMetaObject::connectSlotsByName(this);

    this->groupLayout()->addWidget(proxy);
}

TaskBoxPosition::~TaskBoxPosition()
{
    delete ui;
}


//**************************************************************************
//**************************************************************************
// TaskBoxAngle
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

/* TRANSLATOR Gui::TaskBoxAngle */

TaskBoxAngle::TaskBoxAngle(QWidget *parent)
    : TaskBox(Gui::BitmapFactory().pixmap("Robot_CreateRobot"),tr("Angle"),true, parent)
{
    // we need a separate container widget to add all controls to
    proxy = new QWidget(this);
    ui = new Ui_Angle();
    ui->setupUi(proxy);
    QMetaObject::connectSlotsByName(this);

    this->groupLayout()->addWidget(proxy);
}

TaskBoxAngle::~TaskBoxAngle()
{
    delete ui;
}


//**************************************************************************
//**************************************************************************
// TaskDialog
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

/* TRANSLATOR Gui::TaskDlgRelocation */

TaskDlgRelocation::TaskDlgRelocation()
    : TaskDialog()
{
}

TaskDlgRelocation::~TaskDlgRelocation() = default;

//==== calls from the TaskView ===============================================================


void TaskDlgRelocation::open()
{
}

void TaskDlgRelocation::clicked(int)
{

}

bool TaskDlgRelocation::accept()
{
    return true;
}

bool TaskDlgRelocation::reject()
{
    return true;
}

void TaskDlgRelocation::helpRequested()
{

}


#include "moc_TaskDlgRelocation.cpp"
