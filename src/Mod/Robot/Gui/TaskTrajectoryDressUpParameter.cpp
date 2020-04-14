/***************************************************************************
 *   Copyright (c) 2009 Jürgen Riegel <juergen.riegel@web.de>              *
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

#include <QString>
#include <QSlider>
#include "ui_TaskTrajectoryDressUpParameter.h"
#include "TaskTrajectoryDressUpParameter.h"
#include <Gui/Application.h>
#include <Gui/Document.h>
#include <Gui/BitmapFactory.h>
#include <Gui/ViewProvider.h>
#include <Gui/WaitCursor.h>
#include <Base/Console.h>
#include <Gui/Selection.h>
#include <Gui/Placement.h>


using namespace RobotGui;
using namespace Gui;

TaskTrajectoryDressUpParameter::TaskTrajectoryDressUpParameter(Robot::TrajectoryDressUpObject *obj,QWidget *parent)
    : TaskBox(Gui::BitmapFactory().pixmap("Robot_TrajectoryDressUp"),
      tr("Dress Up Parameter"),
      true, 
      parent),
      pcObject(obj)
{
    // we need a separate container widget to add all controls to
    proxy = new QWidget(this);
    ui = new Ui_TaskTrajectoryDressUpParameter();
    ui->setupUi(proxy);
    QMetaObject::connectSlotsByName(this);

    this->groupLayout()->addWidget(proxy);

    // pump the actual values in the Gui
    ui->doubleSpinBoxSpeed->setValue        (pcObject->Speed.getValue() / 1000.0) ;
    ui->doubleSpinBoxAccel->setValue        (pcObject->Acceleration.getValue() / 1000.0) ;
    ui->checkBoxUseSpeed  ->setChecked      (pcObject->UseSpeed.getValue()) ;
    ui->checkBoxUseAccel  ->setChecked      (pcObject->UseAcceleration.getValue()) ;
    ui->comboBoxCont      ->setCurrentIndex (pcObject->ContType.getValue()) ;
    ui->comboBoxOrientation->setCurrentIndex (pcObject->AddType.getValue()) ;

    PosAdd = pcObject->PosAdd.getValue();
    viewPlacement();

    QObject::connect(ui->toolButtonChoosePlacement,SIGNAL(clicked()),this,SLOT(createPlacementDlg()));

}



TaskTrajectoryDressUpParameter::~TaskTrajectoryDressUpParameter()
{
    delete ui;
}

void TaskTrajectoryDressUpParameter::writeValues(void)
{
    pcObject->Speed.setValue          ( ui->doubleSpinBoxSpeed->value()*1000.0);
    pcObject->Acceleration.setValue   ( ui->doubleSpinBoxAccel->value()*1000.0);
    pcObject->UseSpeed.setValue       ( ui->checkBoxUseSpeed  ->isChecked()   );
    pcObject->UseAcceleration.setValue( ui->checkBoxUseAccel  ->isChecked()   );
    pcObject->ContType.setValue       ( ui->comboBoxCont      ->currentIndex());
    pcObject->AddType.setValue        ( ui->comboBoxOrientation->currentIndex());
    pcObject->PosAdd.setValue(PosAdd);
}

void TaskTrajectoryDressUpParameter::createPlacementDlg(void)
{
    Gui::Dialog::Placement plc;
    plc.setPlacement(PosAdd);
    if (plc.exec() == QDialog::Accepted) {
        PosAdd = plc.getPlacement();
        viewPlacement();
    }

}


void TaskTrajectoryDressUpParameter::viewPlacement(void)
{
    double A,B,C;
    Base::Vector3d pos = PosAdd.getPosition();
    PosAdd.getRotation().getYawPitchRoll(A,B,C);
    QString val = QString::fromLatin1("(%1,%2,%3),(%4,%5,%6)\n")
         .arg(pos.x,0,'g',6)
         .arg(pos.y,0,'g',6)
         .arg(pos.z,0,'g',6)
         .arg(A,0,'g',6)
         .arg(B,0,'g',6)
         .arg(C,0,'g',6);

    ui->lineEditPlacement->setText(val);
}

#include "moc_TaskTrajectoryDressUpParameter.cpp"
