/******************************************************************************
 *   Copyright (c)2012 Jan Rheinlaender <jrheinlaender@users.sourceforge.net> *
 *                                                                            *
 *   This file is part of the FreeCAD CAx development system.                 *
 *                                                                            *
 *   This library is free software; you can redistribute it and/or            *
 *   modify it under the terms of the GNU Library General Public              *
 *   License as published by the Free Software Foundation; either             *
 *   version 2 of the License, or (at your option) any later version.         *
 *                                                                            *
 *   This library  is distributed in the hope that it will be useful,         *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of           *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the            *
 *   GNU Library General Public License for more details.                     *
 *                                                                            *
 *   You should have received a copy of the GNU Library General Public        *
 *   License along with this library; see the file COPYING.LIB. If not,       *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,            *
 *   Suite 330, Boston, MA  02111-1307, USA                                   *
 *                                                                            *
 ******************************************************************************/


#ifndef GUI_TASKVIEW_TaskLinearPatternParameters_H
#define GUI_TASKVIEW_TaskLinearPatternParameters_H

#include <Gui/TaskView/TaskView.h>
#include <Gui/Selection.h>
#include <Gui/TaskView/TaskDialog.h>

#include "TaskTransformedParameters.h"
#include "ViewProviderLinearPattern.h"

class QTimer;
class Ui_TaskLinearPatternParameters;

namespace App {
class Property;
}

namespace Gui {
class ViewProvider;
}

namespace PartDesignGui {

class TaskMultiTransformParameters;

class TaskLinearPatternParameters : public TaskTransformedParameters
{
    Q_OBJECT

public:
    /// Constructor for task with ViewProvider
    TaskLinearPatternParameters(ViewProviderTransformed *TransformedView, QWidget *parent = 0);
    /// Constructor for task with parent task (MultiTransform mode)
    TaskLinearPatternParameters(TaskMultiTransformParameters *parentTask, QLayout *layout);
    virtual ~TaskLinearPatternParameters();

    virtual void apply();

private Q_SLOTS:
    void onUpdateViewTimer();
    void onDirectionChanged(int num);
    void onCheckReverse(const bool on);
    void onLength(const double l);
    void onOccurrences(const uint n);
    virtual void onUpdateView(bool);

protected:
    virtual void changeEvent(QEvent *e);
    virtual void onSelectionChanged(const Gui::SelectionChanges& msg);
    virtual void clearButtons();
    void getDirection(App::DocumentObject*& obj, std::vector<std::string>& sub) const;
    bool getReverse(void) const;
    double getLength(void) const;
    unsigned getOccurrences(void) const;

private:
    void setupUI();
    void updateUI();
    void kickUpdateViewTimer() const;

private:
    Ui_TaskLinearPatternParameters* ui;
    QTimer* updateViewTimer;

    ComboLinks dirLinks;
};


/// simulation dialog for the TaskView
class TaskDlgLinearPatternParameters : public TaskDlgTransformedParameters
{
    Q_OBJECT

public:
    TaskDlgLinearPatternParameters(ViewProviderLinearPattern *LinearPatternView);
    virtual ~TaskDlgLinearPatternParameters() {}
};

} //namespace PartDesignGui

#endif // GUI_TASKVIEW_TASKAPPERANCE_H
