/***************************************************************************
 *   Copyright (c) 2011 Juergen Riegel <FreeCAD@juergen-riegel.net>        *
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


#ifndef GUI_TASKVIEW_TaskHoleParameters_H
#define GUI_TASKVIEW_TaskHoleParameters_H

#include <Gui/TaskView/TaskView.h>
#include <Gui/Selection.h>
#include <Gui/TaskView/TaskDialog.h>

#include "TaskSketchBasedParameters.h"
#include "ViewProviderHole.h"

class Ui_TaskHoleParameters;

namespace App {
class Property;
}

namespace Gui {
class ViewProvider;
}

namespace PartDesignGui { 



class TaskHoleParameters : public TaskSketchBasedParameters
{
    Q_OBJECT

public:
    TaskHoleParameters(ViewProviderHole *HoleView, QWidget *parent = 0);
    ~TaskHoleParameters();

    bool   getThreaded() const;
    long   getThreadType() const;
    long   getThreadSize() const;
    long   getThreadClass() const;
    long   getThreadFit() const;
    double getDiameter() const;
    bool   getThreadDirection() const;
    long   getHoleCutType() const;
    double getHoleCutDiameter() const;
    double getHoleCutDepth() const;
    double getHoleCutCountersinkAngle() const;
    long   getType() const;
    double getLength() const;
    long   getDrillPoint() const;
    double getDrillPointAngle() const;
    bool   getTapered() const;
    double getTaperedAngle() const;

private Q_SLOTS:
    void threadedChanged();
    void threadTypeChanged(int index);
    void threadSizeChanged(int index);
    void threadClassChanged(int index);
    void threadFitChanged(int index);
    void threadDiameterChanged(double value);
    void threadDirectionChanged();
    void holeCutChanged(int index);
    void holeCutDiameterChanged(double value);
    void holeCutDepthChanged(double value);
    void holeCutCountersinkAngleChanged(int value);
    void depthChanged(int index);
    void depthValueChanged(double value);
    void drillPointChanged();
    void drillPointAngledValueChanged(int value);
    void taperedChanged();
    void taperedAngleChanged(double value);

protected:
    void changeEvent(QEvent *e);
    void updateHoleCutParams();

private:
    void onSelectionChanged(const Gui::SelectionChanges &msg);
    void updateUi();

private:
    QWidget* proxy;
    Ui_TaskHoleParameters* ui;
};

/// simulation dialog for the TaskView
class TaskDlgHoleParameters : public TaskDlgSketchBasedParameters
{
    Q_OBJECT

public:
    TaskDlgHoleParameters(ViewProviderHole *HoleView);
    ~TaskDlgHoleParameters();

    ViewProviderHole* getHoleView() const { return static_cast<ViewProviderHole*>(vp); }

public:
    virtual bool accept();

protected:
    TaskHoleParameters  *parameter;
};

} //namespace PartDesignGui

#endif // GUI_TASKVIEW_TASKAPPERANCE_H
