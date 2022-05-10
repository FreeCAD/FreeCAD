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


#ifndef GUI_TASKVIEW_TrajectorySimulate_H
#define GUI_TASKVIEW_TrajectorySimulate_H


#include <QDialog>
#include <memory>

#include <Mod/Robot/App/RobotObject.h>
#include <Mod/Robot/App/Robot6Axis.h>
#include <Mod/Robot/App/TrajectoryObject.h>
#include <Mod/Robot/App/Trajectory.h>
#include <Mod/Robot/App/Simulation.h>

#include "ViewProviderRobotObject.h"


namespace RobotGui { 

class Ui_DlgTrajectorySimulate;

class TrajectorySimulate : public QDialog
{
    Q_OBJECT

public:
    TrajectorySimulate(Robot::RobotObject *pcRobotObject,Robot::TrajectoryObject *pcTrajectoryObject,QWidget *parent = nullptr);
    ~TrajectorySimulate();

private Q_SLOTS:
    void start(void);
    void stop(void);
    void run(void);
    void back(void);
    void forward(void);
    void end(void);

    void timerDone(void);
    void valueChanged ( int value );
    void valueChanged ( double d );

protected:
    void setTo(void);

    QTimer *timer;

    Robot::Simulation sim;

    ViewProviderRobotObject *ViewProv;

    bool Run;
    bool block;

    float timePos;
    float duration;

private:
    std::unique_ptr<Ui_DlgTrajectorySimulate> ui;
};

} //namespace PartDesignGui

#endif // GUI_TASKVIEW_TrajectorySimulate_H
