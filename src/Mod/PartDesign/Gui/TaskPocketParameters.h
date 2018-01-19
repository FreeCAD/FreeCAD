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


#ifndef GUI_TASKVIEW_TaskPocketParameters_H
#define GUI_TASKVIEW_TaskPocketParameters_H

#include <Gui/TaskView/TaskView.h>
#include <Gui/Selection.h>
#include <Gui/TaskView/TaskDialog.h>

#include "TaskSketchBasedParameters.h"
#include "ViewProviderPocket.h"

class Ui_TaskPocketParameters;

namespace App {
class Property;
}

namespace Gui {
class ViewProvider;
}

namespace PartDesignGui {


class TaskPocketParameters : public TaskSketchBasedParameters
{
    Q_OBJECT

public:
    TaskPocketParameters(ViewProviderPocket *PocketView, QWidget *parent = 0, bool newObj=false);
    ~TaskPocketParameters();

    virtual void saveHistory() override;
    virtual void apply() override;

private Q_SLOTS:
    void onLengthChanged(double);
    void onLength2Changed(double);
    void onOffsetChanged(double);
    void onMidplaneChanged(bool);
    void onReversedChanged(bool);
    void onButtonFace(const bool pressed = true);
    void onFaceName(const QString& text);
    void onModeChanged(int);

protected:
    void changeEvent(QEvent *e) override;

private:
    double getLength(void) const;
    double getLength2(void) const;
    double getOffset(void) const;
    int    getMode(void) const;
    bool   getMidplane(void) const;
    bool   getReversed(void) const;
    QString getFaceName(void) const;

    void onSelectionChanged(const Gui::SelectionChanges& msg) override;
    void updateUI(int index);

private:
    QWidget* proxy;
    Ui_TaskPocketParameters* ui;
    double oldLength;
};

/// simulation dialog for the TaskView
class TaskDlgPocketParameters : public TaskDlgSketchBasedParameters
{
    Q_OBJECT

public:
    TaskDlgPocketParameters(ViewProviderPocket *PocketView);

    ViewProviderPocket* getPocketView() const
    { return static_cast<ViewProviderPocket*>(vp); }
};

} //namespace PartDesignGui

#endif // GUI_TASKVIEW_TASKAPPERANCE_H
