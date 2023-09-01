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

#ifndef GUI_TASKVIEW_TaskMirroredParameters_H
#define GUI_TASKVIEW_TaskMirroredParameters_H

#include "TaskTransformedParameters.h"
#include "ViewProviderMirrored.h"


class Ui_TaskMirroredParameters;

namespace App {
class Property;
}

namespace Gui {
class ViewProvider;
}

namespace PartDesignGui {

class TaskMultiTransformParameters;

class TaskMirroredParameters : public TaskTransformedParameters
{
    Q_OBJECT

public:
    /// Constructor for task with ViewProvider
    explicit TaskMirroredParameters(ViewProviderTransformed *TransformedView, QWidget *parent = nullptr);
    /// Constructor for task with parent task (MultiTransform mode)
    TaskMirroredParameters(TaskMultiTransformParameters *parentTask, QLayout *layout);

    ~TaskMirroredParameters() override;

    void getMirrorPlane(App::DocumentObject*& obj, std::vector<std::string>& sub) const;

    void apply() override;

private Q_SLOTS:
    void onPlaneChanged(int num);
    void onUpdateView(bool) override;
    void onFeatureDeleted() override;

protected:
    void addObject(App::DocumentObject*) override;
    void removeObject(App::DocumentObject*) override;
    void changeEvent(QEvent *e) override;
    void onSelectionChanged(const Gui::SelectionChanges& msg) override;
    void clearButtons() override;

private:
    void setupUI();
    void updateUI();
    ComboLinks planeLinks;

private:
    std::unique_ptr<Ui_TaskMirroredParameters> ui;
};


/// simulation dialog for the TaskView
class TaskDlgMirroredParameters : public TaskDlgTransformedParameters
{
    Q_OBJECT

public:
    explicit TaskDlgMirroredParameters(ViewProviderMirrored *MirroredView);
    ~TaskDlgMirroredParameters() override = default;

public:
    /// is called by the framework if the dialog is accepted (Ok)
    bool accept() override;
};

} //namespace PartDesignGui

#endif // GUI_TASKVIEW_TASKAPPERANCE_H
