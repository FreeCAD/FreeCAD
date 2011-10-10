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


#ifndef GUI_TASKVIEW_TaskEdge2TracParameter_H
#define GUI_TASKVIEW_TaskEdge2TracParameter_H

#include <Gui/TaskView/TaskView.h>
#include <Gui/Selection.h>

#include <Mod/Robot/App/Edge2TracObject.h>
#include <Mod/Robot/App/TrajectoryObject.h>


class Ui_TaskEdge2TracParameter;

namespace App {
class Property;
}

namespace Gui {
class ViewProvider;
}

namespace RobotGui { 



class TaskEdge2TracParameter : public Gui::TaskView::TaskBox
{
    Q_OBJECT

public:
    TaskEdge2TracParameter(Robot::Edge2TracObject *pcObject,QWidget *parent = 0);
    ~TaskEdge2TracParameter();

    void setEdgeAndClusterNbr(int NbrEdges,int NbrClusters);

private Q_SLOTS:
    void hideShow(void);
    void sizingValueChanged(double Value);
    void orientationToggled(bool Value);

protected:
    Robot::Edge2TracObject *pcObject;
    App::DocumentObject *HideShowObj;

    void setHideShowObject(void);
private:

private:
    QWidget* proxy;
    Ui_TaskEdge2TracParameter* ui;
};

} //namespace PartDesignGui

#endif // GUI_TASKVIEW_TASKAPPERANCE_H
