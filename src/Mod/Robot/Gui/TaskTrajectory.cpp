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

#include "ui_TaskTrajectory.h"
#include "TaskTrajectory.h"
#include <Gui/Application.h>
#include <Gui/Document.h>
#include <Gui/BitmapFactory.h>
#include <Gui/ViewProvider.h>
#include <Gui/WaitCursor.h>
#include <Base/Console.h>
#include <Gui/Selection.h>


using namespace RobotGui;
using namespace Gui;

TaskTrajectory::TaskTrajectory(Robot::RobotObject *pcRobotObject,Robot::TrajectoryObject *pcTrajectoryObject,QWidget *parent)
    : TaskBox(Gui::BitmapFactory().pixmap("document-new"),tr("Trajectory"),true, parent),
      sim(pcTrajectoryObject->Trajectory.getValue(),pcRobotObject->getRobot()),
      pcRobot(pcRobotObject),
      Run(false),
      block(false),
      timePos(0.0)
{
    // we need a separate container widget to add all controls to
    proxy = new QWidget(this);
    ui = new Ui_TaskTrajectory();
    ui->setupUi(proxy);
    QMetaObject::connectSlotsByName(this);

    this->groupLayout()->addWidget(proxy);

      // set Tool
    sim.Tool = pcRobotObject->Tool.getValue();

    ui->trajectoryTable->setSortingEnabled(false);

    Robot::Trajectory trac = pcTrajectoryObject->Trajectory.getValue();
    ui->trajectoryTable->setRowCount(trac.getSize());
    duration = trac.getDuration();
    ui->timeSpinBox->setMaximum(duration);

    for(unsigned int i=0;i<trac.getSize();i++){
        Robot::Waypoint pt = trac.getWaypoint(i);
        switch(pt.Type){
            case Robot::Waypoint::UNDEF: ui->trajectoryTable->setItem(i, 0, new QTableWidgetItem(QString::fromLatin1("UNDEF")));break;
            case Robot::Waypoint::CIRC:  ui->trajectoryTable->setItem(i, 0, new QTableWidgetItem(QString::fromLatin1("CIRC")));break;
            case Robot::Waypoint::PTP:   ui->trajectoryTable->setItem(i, 0, new QTableWidgetItem(QString::fromLatin1("PTP")));break;
            case Robot::Waypoint::LINE:  ui->trajectoryTable->setItem(i, 0, new QTableWidgetItem(QString::fromLatin1("LIN")));break;
            default: ui->trajectoryTable->setItem(i, 0, new QTableWidgetItem(QString::fromLatin1("UNDEF")));break;
        }
        ui->trajectoryTable->setItem(i, 1, new QTableWidgetItem(QString::fromUtf8(pt.Name.c_str())));
        if(pt.Cont)
            ui->trajectoryTable->setItem(i, 2, new QTableWidgetItem(QString::fromLatin1("|")));
        else
            ui->trajectoryTable->setItem(i, 2, new QTableWidgetItem(QString::fromLatin1("-")));
        ui->trajectoryTable->setItem(i, 3, new QTableWidgetItem(QString::number(pt.Velocity)));
        ui->trajectoryTable->setItem(i, 4, new QTableWidgetItem(QString::number(pt.Acceleration)));

    }

    QObject::connect(ui->ButtonStepStart    ,SIGNAL(clicked()),this,SLOT(start()));
    QObject::connect(ui->ButtonStepStop     ,SIGNAL(clicked()),this,SLOT(stop()));
    QObject::connect(ui->ButtonStepRun      ,SIGNAL(clicked()),this,SLOT(run()));
    QObject::connect(ui->ButtonStepBack     ,SIGNAL(clicked()),this,SLOT(back()));
    QObject::connect(ui->ButtonStepForward  ,SIGNAL(clicked()),this,SLOT(forward()));
    QObject::connect(ui->ButtonStepEnd      ,SIGNAL(clicked()),this,SLOT(end()));


    // set up timer
    timer = new QTimer( this );
    timer->setInterval(100);
    QObject::connect(timer      ,SIGNAL(timeout ()),this,SLOT(timerDone()));

    QObject::connect( ui->timeSpinBox       ,SIGNAL(valueChanged(double)), this, SLOT(valueChanged(double)) );
    QObject::connect( ui->timeSlider        ,SIGNAL(valueChanged(int)   ), this, SLOT(valueChanged(int)) );

    // get the view provider
    ViewProv = dynamic_cast<ViewProviderRobotObject*>(Gui::Application::Instance->activeDocument()->getViewProvider(pcRobotObject) );

    setTo();
}

TaskTrajectory::~TaskTrajectory()
{
    delete ui;
   
}

void TaskTrajectory::viewTool(const Base::Placement& pos)
{
    double A,B,C;
    pos.getRotation().getYawPitchRoll(A,B,C);

    QString result = QString::fromLatin1("Pos:(%1, %2, %3, %4, %5, %6)")
        .arg(pos.getPosition().x,0,'f',1)
        .arg(pos.getPosition().y,0,'f',1)
        .arg(pos.getPosition().z,0,'f',1)
        .arg(A,0,'f',1)
        .arg(B,0,'f',1)
        .arg(C,0,'f',1);

    ui->label_Pos->setText(result);
}

void TaskTrajectory::setTo(void)
{
    sim.Tool = pcRobot->Tool.getValue();

    if(timePos < 0.0001)
        sim.reset();
    else{
        sim.setToTime(timePos);
    }
    ViewProv->setAxisTo(sim.Axis[0],sim.Axis[1],sim.Axis[2],sim.Axis[3],sim.Axis[4],sim.Axis[5],sim.Rob.getTcp());
    axisChanged(sim.Axis[0],sim.Axis[1],sim.Axis[2],sim.Axis[3],sim.Axis[4],sim.Axis[5],sim.Rob.getTcp());
    viewTool(sim.Rob.getTcp());
}

void TaskTrajectory::start(void)
{
    timePos = 0.0f;
    ui->timeSpinBox->setValue(timePos);
    ui->timeSlider->setValue(int((timePos/duration)*1000));
    setTo();

}
void TaskTrajectory::stop(void)
{
    timer->stop();
    Run = false;
}
void TaskTrajectory::run(void)
{
    timer->start();
    Run = true;
}
void TaskTrajectory::back(void)
{
}
void TaskTrajectory::forward(void)
{
}
void TaskTrajectory::end(void)
{
    timePos = duration;
    ui->timeSpinBox->setValue(timePos);
    ui->timeSlider->setValue(int((timePos/duration)*1000));
    setTo();
}

void TaskTrajectory::timerDone(void)
{
    if(timePos < duration){
        timePos += .1f;
        ui->timeSpinBox->setValue(timePos);
        ui->timeSlider->setValue(int((timePos/duration)*1000));
        setTo();
        timer->start();
    }else{
        timer->stop();
        Run = false;
    }
}

void TaskTrajectory::valueChanged ( int value )
{
    if(!block){
        timePos = duration*(value/1000.0);
        block=true;
        ui->timeSpinBox->setValue(timePos);
        block=false;
        setTo();
    }
}

void TaskTrajectory::valueChanged ( double value )
{
    if(!block){
        timePos = value;
        block=true;
        ui->timeSlider->setValue(int((timePos/duration)*1000));
        block=false;
        setTo();
    }
}


#include "moc_TaskTrajectory.cpp"
