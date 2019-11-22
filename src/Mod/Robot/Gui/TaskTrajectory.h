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


#ifndef GUI_TASKVIEW_TaskTrajectory_H
#define GUI_TASKVIEW_TaskTrajectory_H

#include <Base/UnitsApi.h>


#include <Gui/TaskView/TaskView.h>
#include <Gui/Selection.h>

#include <Mod/Robot/App/RobotObject.h>
#include <Mod/Robot/App/Robot6Axis.h>
#include <Mod/Robot/App/TrajectoryObject.h>
#include <Mod/Robot/App/Trajectory.h>
#include <Mod/Robot/App/Simulation.h>

#include "ViewProviderRobotObject.h"


class Ui_TaskTrajectory;

namespace App {
class Property;
}

namespace Gui {
class ViewProvider;
}

namespace RobotGui { 



class TaskTrajectory : public Gui::TaskView::TaskBox
{
    Q_OBJECT

public:
    TaskTrajectory(Robot::RobotObject *pcRobotObject,Robot::TrajectoryObject *pcTrajectoryObject,QWidget *parent = 0);
    ~TaskTrajectory();
    /// Observer message from the Selection
    void OnChange(Gui::SelectionSingleton::SubjectType &rCaller,
                  Gui::SelectionSingleton::MessageType Reason);

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

Q_SIGNALS:
    void axisChanged(float A1,float A2,float A3,float A4,float A5,float A6,const Base::Placement &Tcp);

protected:
    void setTo(void);
    void viewTool(const Base::Placement pos);

    QTimer *timer;

    Robot::Simulation sim;
    Robot::RobotObject *pcRobot;
    ViewProviderRobotObject *ViewProv;

    bool Run;
    bool block;

    float timePos;
    float duration;

private:
    QWidget* proxy;
    Ui_TaskTrajectory* ui;
};

} //namespace PartDesignGui

#endif // GUI_TASKVIEW_TASKAPPERANCE_H
