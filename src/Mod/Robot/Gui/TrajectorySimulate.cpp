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
#include <QTimer>
#endif

#include <Gui/Application.h>
#include <Gui/Document.h>
#include <Mod/Robot/App/Waypoint.h>

#include "TrajectorySimulate.h"
#include "ui_TrajectorySimulate.h"


using namespace RobotGui;
using namespace Gui;

TrajectorySimulate::TrajectorySimulate(Robot::RobotObject* pcRobotObject,
                                       Robot::TrajectoryObject* pcTrajectoryObject,
                                       QWidget* parent)
    : QDialog(parent)
    , sim(pcTrajectoryObject->Trajectory.getValue(), pcRobotObject->getRobot())
    , Run(false)
    , block(false)
    , timePos(0.0)
    , ui(new Ui_DlgTrajectorySimulate)
{
    ui->setupUi(this);
    QMetaObject::connectSlotsByName(this);

    // set Tool
    sim.Tool = pcRobotObject->Tool.getValue();

    ui->trajectoryTable->setSortingEnabled(false);

    Robot::Trajectory trac = pcTrajectoryObject->Trajectory.getValue();
    ui->trajectoryTable->setRowCount(trac.getSize());
    duration = trac.getDuration();
    ui->timeSpinBox->setMaximum(duration);

    for (unsigned int i = 0; i < trac.getSize(); i++) {
        Robot::Waypoint pt = trac.getWaypoint(i);
        switch (pt.Type) {
            case Robot::Waypoint::UNDEF:
                ui->trajectoryTable->setItem(i,
                                             0,
                                             new QTableWidgetItem(QString::fromLatin1("UNDEF")));
                break;
            case Robot::Waypoint::CIRC:
                ui->trajectoryTable->setItem(i,
                                             0,
                                             new QTableWidgetItem(QString::fromLatin1("CIRC")));
                break;
            case Robot::Waypoint::PTP:
                ui->trajectoryTable->setItem(i,
                                             0,
                                             new QTableWidgetItem(QString::fromLatin1("PTP")));
                break;
            case Robot::Waypoint::LINE:
                ui->trajectoryTable->setItem(i,
                                             0,
                                             new QTableWidgetItem(QString::fromLatin1("LIN")));
                break;
            default:
                ui->trajectoryTable->setItem(i,
                                             0,
                                             new QTableWidgetItem(QString::fromLatin1("UNDEF")));
                break;
        }
        ui->trajectoryTable->setItem(i,
                                     1,
                                     new QTableWidgetItem(QString::fromUtf8(pt.Name.c_str())));
        if (pt.Cont) {
            ui->trajectoryTable->setItem(i, 2, new QTableWidgetItem(QString::fromLatin1("|")));
        }
        else {
            ui->trajectoryTable->setItem(i, 2, new QTableWidgetItem(QString::fromLatin1("-")));
        }
        ui->trajectoryTable->setItem(i, 3, new QTableWidgetItem(QString::number(pt.Velocity)));
        ui->trajectoryTable->setItem(i, 4, new QTableWidgetItem(QString::number(pt.Acceleration)));
    }

    // clang-format off
    QObject::connect(ui->ButtonStepStart, &QPushButton::clicked, this, &TrajectorySimulate::start);
    QObject::connect(ui->ButtonStepStop, &QPushButton::clicked, this, &TrajectorySimulate::stop);
    QObject::connect(ui->ButtonStepRun, &QPushButton::clicked, this, &TrajectorySimulate::run);
    QObject::connect(ui->ButtonStepBack, &QPushButton::clicked, this, &TrajectorySimulate::back);
    QObject::connect(ui->ButtonStepForward, &QPushButton::clicked, this, &TrajectorySimulate::forward);
    QObject::connect(ui->ButtonStepEnd, &QPushButton::clicked, this, &TrajectorySimulate::end);

    // set up timer
    timer = new QTimer(this);
    timer->setInterval(100);
    QObject::connect(timer, &QTimer::timeout, this, &TrajectorySimulate::timerDone);
    QObject::connect(ui->timeSpinBox, qOverload<double>(&QDoubleSpinBox::valueChanged),
                     this, qOverload<double>(&TrajectorySimulate::valueChanged));
    QObject::connect(ui->timeSlider, qOverload<int>(&QSlider::valueChanged),
                     this, qOverload<int>(&TrajectorySimulate::valueChanged));
    // clang-format on

    // get the view provider
    ViewProv = static_cast<ViewProviderRobotObject*>(
        Gui::Application::Instance->activeDocument()->getViewProvider(pcRobotObject));

    setTo();
}

TrajectorySimulate::~TrajectorySimulate() = default;

void TrajectorySimulate::setTo()
{
    sim.setToTime(timePos);
    ViewProv->setAxisTo(sim.Axis[0],
                        sim.Axis[1],
                        sim.Axis[2],
                        sim.Axis[3],
                        sim.Axis[4],
                        sim.Axis[5],
                        sim.Rob.getTcp());
}

void TrajectorySimulate::start()
{
    timePos = 0.0f;
    ui->timeSpinBox->setValue(timePos);
    ui->timeSlider->setValue(int((timePos / duration) * 1000));
    setTo();
}
void TrajectorySimulate::stop()
{
    timer->stop();
    Run = false;
}
void TrajectorySimulate::run()
{
    timer->start();
    Run = true;
}
void TrajectorySimulate::back()
{}
void TrajectorySimulate::forward()
{}
void TrajectorySimulate::end()
{
    timePos = duration;
    ui->timeSpinBox->setValue(timePos);
    ui->timeSlider->setValue(int((timePos / duration) * 1000));
    setTo();
}

void TrajectorySimulate::timerDone()
{
    if (timePos < duration) {
        timePos += .1f;
        ui->timeSpinBox->setValue(timePos);
        ui->timeSlider->setValue(int((timePos / duration) * 1000));
        setTo();
        timer->start();
    }
    else {
        timer->stop();
        Run = false;
    }
}

void TrajectorySimulate::valueChanged(int value)
{
    if (!block) {
        timePos = duration * (value / 1000.0);
        block = true;
        ui->timeSpinBox->setValue(timePos);
        block = false;
        setTo();
    }
}

void TrajectorySimulate::valueChanged(double value)
{
    if (!block) {
        timePos = value;
        block = true;
        ui->timeSlider->setValue(int((timePos / duration) * 1000));
        block = false;
        setTo();
    }
}


#include "moc_TrajectorySimulate.cpp"
