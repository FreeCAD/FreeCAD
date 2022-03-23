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


#ifndef GUI_TASKVIEW_TaskTrajectoryDressUpParameter_h
#define GUI_TASKVIEW_TaskTrajectoryDressUpParameter_h

#include <Gui/TaskView/TaskView.h>
#include <Gui/Selection.h>

#include <Mod/Robot/App/Edge2TracObject.h>
#include <Mod/Robot/App/TrajectoryObject.h>
#include <Mod/Robot/App/TrajectoryDressUpObject.h>


class Ui_TaskTrajectoryDressUpParameter;

 
namespace RobotGui { 



class TaskTrajectoryDressUpParameter : public Gui::TaskView::TaskBox
{
    Q_OBJECT

public:
    TaskTrajectoryDressUpParameter(Robot::TrajectoryDressUpObject *obj,QWidget *parent = nullptr);
    ~TaskTrajectoryDressUpParameter();

    /// this methode write the values from the Gui to the object, usually in accept()
    void writeValues(void);
 
private Q_SLOTS:
    /// edit the placement
    void createPlacementDlg(void);

   
protected:
    Base::Placement PosAdd;

    void viewPlacement(void);

private:
    QWidget* proxy;
    Ui_TaskTrajectoryDressUpParameter* ui;
    Robot::TrajectoryDressUpObject *pcObject;
};

} //namespace PartDesignGui

#endif // GUI_TASKVIEW_TASKAPPERANCE_H
