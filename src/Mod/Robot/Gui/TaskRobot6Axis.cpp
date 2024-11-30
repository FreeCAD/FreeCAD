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
#include <QString>
#include <qpalette.h>
#endif

#include <Gui/BitmapFactory.h>
#include <Gui/Placement.h>
#include <Gui/Selection.h>

#include "TaskRobot6Axis.h"
#include "ui_TaskRobot6Axis.h"


using namespace RobotGui;
using namespace Gui;

TaskRobot6Axis::TaskRobot6Axis(Robot::RobotObject* pcRobotObject, QWidget* parent)
    : TaskBox(Gui::BitmapFactory().pixmap("Robot_CreateRobot"), tr("TaskRobot6Axis"), true, parent)
    , pcRobot(pcRobotObject)
    , Rob()
{
    // we need a separate container widget to add all controls to
    proxy = new QWidget(this);
    ui = new Ui_TaskRobot6Axis();
    ui->setupUi(proxy);
    QMetaObject::connectSlotsByName(this);

    this->groupLayout()->addWidget(proxy);

    // clang-format off
    connect(ui->horizontalSlider_Axis1, &QSlider::sliderMoved,
            this, &TaskRobot6Axis::changeSliderA1);
    connect(ui->horizontalSlider_Axis2, &QSlider::sliderMoved,
            this, &TaskRobot6Axis::changeSliderA2);
    connect(ui->horizontalSlider_Axis3, &QSlider::sliderMoved,
            this, &TaskRobot6Axis::changeSliderA3);
    connect(ui->horizontalSlider_Axis4, &QSlider::sliderMoved,
            this, &TaskRobot6Axis::changeSliderA4);
    connect(ui->horizontalSlider_Axis5, &QSlider::sliderMoved,
            this, &TaskRobot6Axis::changeSliderA5);
    connect(ui->horizontalSlider_Axis6, &QSlider::sliderMoved,
            this, &TaskRobot6Axis::changeSliderA6);
    connect(ui->pushButtonChooseTool, &QPushButton::clicked,
            this, &TaskRobot6Axis::createPlacementDlg);
    // clang-format on

    if (pcRobotObject) {
        setRobot(pcRobotObject);
    }
}

TaskRobot6Axis::~TaskRobot6Axis()
{
    delete ui;
}

void TaskRobot6Axis::setRobot(Robot::RobotObject* pcRobotObject)
{
    pcRobot = pcRobotObject;
    if (!pcRobotObject) {
        delete Rob;
        return;
    }

    Rob = new Robot::Robot6Axis(pcRobot->getRobot());
    ui->horizontalSlider_Axis1->setMaximum((int)Rob->getMaxAngle(0));
    ui->horizontalSlider_Axis1->setMinimum((int)Rob->getMinAngle(0));

    ui->horizontalSlider_Axis2->setMaximum((int)Rob->getMaxAngle(1));
    ui->horizontalSlider_Axis2->setMinimum((int)Rob->getMinAngle(1));

    ui->horizontalSlider_Axis3->setMaximum((int)Rob->getMaxAngle(2));
    ui->horizontalSlider_Axis3->setMinimum((int)Rob->getMinAngle(2));

    ui->horizontalSlider_Axis4->setMaximum((int)Rob->getMaxAngle(3));
    ui->horizontalSlider_Axis4->setMinimum((int)Rob->getMinAngle(3));

    ui->horizontalSlider_Axis5->setMaximum((int)Rob->getMaxAngle(4));
    ui->horizontalSlider_Axis5->setMinimum((int)Rob->getMinAngle(4));

    ui->horizontalSlider_Axis6->setMaximum((int)Rob->getMaxAngle(5));
    ui->horizontalSlider_Axis6->setMinimum((int)Rob->getMinAngle(5));

    setAxis(pcRobot->Axis1.getValue(),
            pcRobot->Axis2.getValue(),
            pcRobot->Axis3.getValue(),
            pcRobot->Axis4.getValue(),
            pcRobot->Axis5.getValue(),
            pcRobot->Axis6.getValue(),
            pcRobot->Tcp.getValue());
    viewTool(pcRobot->Tool.getValue());
}

void TaskRobot6Axis::createPlacementDlg()
{
    Gui::Dialog::Placement plc;
    plc.setSelection(Gui::Selection().getSelectionEx());
    plc.setPlacement(pcRobot->Tool.getValue());
    if (plc.exec() == QDialog::Accepted) {
        pcRobot->Tool.setValue(plc.getPlacement());
    }
    viewTool(pcRobot->Tool.getValue());
}


void TaskRobot6Axis::viewTcp(const Base::Placement& pos)
{
    double A, B, C;
    pos.getRotation().getYawPitchRoll(A, B, C);

    QString result = QString::fromLatin1("TCP:( %1, %2, %3, %4, %5, %6 )")
                         .arg(pos.getPosition().x, 0, 'f', 1)
                         .arg(pos.getPosition().y, 0, 'f', 1)
                         .arg(pos.getPosition().z, 0, 'f', 1)
                         .arg(A, 0, 'f', 1)
                         .arg(B, 0, 'f', 1)
                         .arg(C, 0, 'f', 1);

    ui->label_TCP->setText(result);
}

void TaskRobot6Axis::viewTool(const Base::Placement& pos)
{
    double A, B, C;
    pos.getRotation().getYawPitchRoll(A, B, C);

    QString result = QString::fromLatin1("Tool:( %1, %2, %3, %4, %5, %6 )")
                         .arg(pos.getPosition().x, 0, 'f', 1)
                         .arg(pos.getPosition().y, 0, 'f', 1)
                         .arg(pos.getPosition().z, 0, 'f', 1)
                         .arg(A, 0, 'f', 1)
                         .arg(B, 0, 'f', 1)
                         .arg(C, 0, 'f', 1);

    ui->label_Tool->setText(result);
}

void TaskRobot6Axis::changeSliderA1(int value)
{
    pcRobot->Axis1.setValue(float(value));
    viewTcp(pcRobot->Tcp.getValue());
    ui->lineEdit_Axis1->setText(QString::fromUtf8("%1\xc2\xb0").arg((float)value, 0, 'f', 1));
    setColor(0, float(value), *(ui->lineEdit_Axis1));
}

void TaskRobot6Axis::changeSliderA2(int value)
{
    pcRobot->Axis2.setValue(float(value));
    viewTcp(pcRobot->Tcp.getValue());
    ui->lineEdit_Axis2->setText(QString::fromUtf8("%1\xc2\xb0").arg((float)value, 0, 'f', 1));
    setColor(1, float(value), *(ui->lineEdit_Axis2));
}

void TaskRobot6Axis::changeSliderA3(int value)
{
    pcRobot->Axis3.setValue(float(value));
    viewTcp(pcRobot->Tcp.getValue());
    ui->lineEdit_Axis3->setText(QString::fromUtf8("%1\xc2\xb0").arg((float)value, 0, 'f', 1));
    setColor(2, float(value), *(ui->lineEdit_Axis3));
}

void TaskRobot6Axis::changeSliderA4(int value)
{
    pcRobot->Axis4.setValue(float(value));
    viewTcp(pcRobot->Tcp.getValue());
    ui->lineEdit_Axis4->setText(QString::fromLatin1("%1°").arg((float)value, 0, 'f', 1));
    setColor(3, float(value), *(ui->lineEdit_Axis4));
}

void TaskRobot6Axis::changeSliderA5(int value)
{
    pcRobot->Axis5.setValue(float(value));
    viewTcp(pcRobot->Tcp.getValue());
    ui->lineEdit_Axis5->setText(QString::fromLatin1("%1°").arg((float)value, 0, 'f', 1));
    setColor(4, float(value), *(ui->lineEdit_Axis5));
}

void TaskRobot6Axis::changeSliderA6(int value)
{
    pcRobot->Axis6.setValue(float(value));
    viewTcp(pcRobot->Tcp.getValue());
    ui->lineEdit_Axis6->setText(QString::fromLatin1("%1°").arg((float)value, 0, 'f', 1));
    setColor(5, float(value), *(ui->lineEdit_Axis6));
}
void TaskRobot6Axis::setColor(int i, float angle, QLineEdit& lineEdit)
{

    if (angle > Rob->getMaxAngle(i) || angle < Rob->getMinAngle(i)) {
        QPalette p = lineEdit.palette();
        p.setColor(QPalette::Base, QColor(255, 220, 220));  // green color
        lineEdit.setPalette(p);
    }
    else {
        QPalette p = lineEdit.palette();
        p.setColor(QPalette::Base, QColor(220, 255, 220));  // green color
        lineEdit.setPalette(p);
    }
}

void TaskRobot6Axis::setAxis(float A1,
                             float A2,
                             float A3,
                             float A4,
                             float A5,
                             float A6,
                             const Base::Placement& Tcp)
{
    ui->horizontalSlider_Axis1->setSliderPosition((int)A1);
    ui->lineEdit_Axis1->setText(QString::fromLatin1("%1°").arg(A1, 0, 'f', 1));
    setColor(0, A1, *(ui->lineEdit_Axis1));

    ui->horizontalSlider_Axis2->setSliderPosition((int)A2);
    ui->lineEdit_Axis2->setText(QString::fromLatin1("%1°").arg(A2, 0, 'f', 1));
    setColor(1, A2, *(ui->lineEdit_Axis2));

    ui->horizontalSlider_Axis3->setSliderPosition((int)A3);
    ui->lineEdit_Axis3->setText(QString::fromLatin1("%1°").arg(A3, 0, 'f', 1));
    setColor(2, A3, *(ui->lineEdit_Axis3));

    ui->horizontalSlider_Axis4->setSliderPosition((int)A4);
    ui->lineEdit_Axis4->setText(QString::fromLatin1("%1°").arg(A4, 0, 'f', 1));
    setColor(3, A4, *(ui->lineEdit_Axis4));

    ui->horizontalSlider_Axis5->setSliderPosition((int)A5);
    ui->lineEdit_Axis5->setText(QString::fromLatin1("%1°").arg(A5, 0, 'f', 1));
    setColor(4, A5, *(ui->lineEdit_Axis5));

    ui->horizontalSlider_Axis6->setSliderPosition((int)A6);
    ui->lineEdit_Axis6->setText(QString::fromLatin1("%1°").arg(A6, 0, 'f', 1));
    setColor(5, A6, *(ui->lineEdit_Axis6));

    viewTcp(Tcp);
}


#include "moc_TaskRobot6Axis.cpp"
