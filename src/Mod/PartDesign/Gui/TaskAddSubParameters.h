/***************************************************************************
 *   Copyright (c) 2023 <bgbsww@gmail.com>                                 *
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


#ifndef GUI_TASKVIEW_TaskAddSubParameters_H
#define GUI_TASKVIEW_TaskAddSubParameters_H

// #include <Gui/Selection.h>
#include "ViewProviderAddSub.h"

#include "TaskFeatureParameters.h"
#include <Mod/PartDesign/App/FeatureAddSub.h>

namespace App {
class Property;
}

namespace PartDesignGui {


/// Convenience class to collect common methods for all AddSub features
class TaskAddSubParameters : public PartDesignGui::TaskFeatureParameters,
                                  public Gui::SelectionObserver
{
    Q_OBJECT

public:
    TaskAddSubParameters(PartDesignGui::ViewProvider* vp, QWidget *parent,
                              const std::string& pixmapname, const QString& parname);
    // ~TaskAddSubParameters() override;

// protected:
// protected Q_SLOTS:
    void onOutsideChanged(bool);
    bool enableOutside(PartDesignGui::ViewProvider* vp);

protected:
    void onSelectionChanged(const Gui::SelectionChanges& msg) override;
};

class TaskDlgAddSubParameters : public PartDesignGui::TaskDlgFeatureParameters
{
    Q_OBJECT

public:
    explicit TaskDlgAddSubParameters(PartDesignGui::ViewProvider *vp);
    // ~TaskDlgAddSubParameters() override;

public:
    /// is called by the framework if the dialog is accepted (Ok)
    // bool accept() override;
    /// is called by the framework if the dialog is rejected (Cancel)
    // bool reject() override;
};

} //namespace PartDesignGui

#endif // GUI_TASKVIEW_TaskAddSubParameters_H
