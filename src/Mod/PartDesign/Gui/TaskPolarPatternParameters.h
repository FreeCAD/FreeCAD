/******************************************************************************
 *   Copyright (c) 2012 Jan Rheinländer <jrheinlaender@users.sourceforge.net> *
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

#ifndef GUI_TASKVIEW_TaskPolarPatternParameters_H
#define GUI_TASKVIEW_TaskPolarPatternParameters_H

#include "TaskTransformedParameters.h"
#include "ViewProviderPolarPattern.h"


class QTimer;
class Ui_TaskPolarPatternParameters;

namespace App {
class Property;
}

namespace Gui {
class ViewProvider;
}

namespace PartDesignGui {

class TaskMultiTransformParameters;

class TaskPolarPatternParameters : public TaskTransformedParameters
{
    Q_OBJECT

public:
    /// Constructor for task with ViewProvider
    TaskPolarPatternParameters(ViewProviderTransformed *TransformedView, QWidget *parent = nullptr);
    /// Constructor for task with parent task (MultiTransform mode)
    TaskPolarPatternParameters(TaskMultiTransformParameters *parentTask, QLayout *layout);
    virtual ~TaskPolarPatternParameters();

    void apply();

private Q_SLOTS:
    void onUpdateViewTimer();
    void onAxisChanged(int num);
    void onCheckReverse(const bool on);
    void onAngle(const double a);
    void onOccurrences(const uint n);
    virtual void onUpdateView(bool);
    virtual void onFeatureDeleted(void);

protected:
    virtual void addObject(App::DocumentObject*);
    virtual void removeObject(App::DocumentObject*);
    virtual void changeEvent(QEvent *e);
    virtual void onSelectionChanged(const Gui::SelectionChanges& msg);
    virtual void clearButtons();
    void getAxis(App::DocumentObject*& obj, std::vector<std::string>& sub) const;
    const std::string getStdAxis(void) const;
    const std::string getAxis(void) const;
    bool getReverse(void) const;
    double getAngle(void) const;
    unsigned getOccurrences(void) const;

private:
    void connectSignals();
    void setupUI();
    void updateUI();
    void kickUpdateViewTimer() const;

private:
    std::unique_ptr<Ui_TaskPolarPatternParameters> ui;
    QTimer* updateViewTimer;

    ComboLinks axesLinks;
};


/// simulation dialog for the TaskView
class TaskDlgPolarPatternParameters : public TaskDlgTransformedParameters
{
    Q_OBJECT

public:
    TaskDlgPolarPatternParameters(ViewProviderPolarPattern *PolarPatternView);
    virtual ~TaskDlgPolarPatternParameters() {}
};

} //namespace PartDesignGui

#endif // GUI_TASKVIEW_TASKAPPERANCE_H
