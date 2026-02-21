// SPDX-License-Identifier: LGPL-2.1-or-later

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

#pragma once

#include "TaskTransformedParameters.h"
#include "ViewProviderMirrored.h"


class Ui_TaskMirroredParameters;

namespace App
{
class Property;
}

namespace Gui
{
class ViewProvider;
}

namespace PartDesignGui
{

class TaskMultiTransformParameters;

class TaskMirroredParameters: public TaskTransformedParameters
{
    Q_OBJECT

public:
    /// Constructor for task with ViewProvider
    explicit TaskMirroredParameters(ViewProviderTransformed* TransformedView, QWidget* parent = nullptr);
    /// Constructor for task with parent task (MultiTransform mode)
    TaskMirroredParameters(TaskMultiTransformParameters* parentTask, QWidget* parameterWidget);

    ~TaskMirroredParameters() override;

    void apply() override;

protected:
    void onSelectionChanged(const Gui::SelectionChanges& msg) override;

private Q_SLOTS:
    void onPlaneChanged(int num);
    void onUpdateView(bool /*unused*/) override;

private:
    void setupParameterUI(QWidget* widget) override;
    void retranslateParameterUI(QWidget* widget) override;
    void updateUI();
    void getMirrorPlane(App::DocumentObject*& obj, std::vector<std::string>& sub) const;

private:
    Gui::ComboLinks planeLinks;
    std::unique_ptr<Ui_TaskMirroredParameters> ui;
};


/// simulation dialog for the TaskView
class TaskDlgMirroredParameters: public TaskDlgTransformedParameters
{
    Q_OBJECT

public:
    explicit TaskDlgMirroredParameters(ViewProviderMirrored* MirroredView);
};

}  // namespace PartDesignGui
