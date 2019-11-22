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


#ifndef GUI_TASKVIEW_TaskRevolutionParameters_H
#define GUI_TASKVIEW_TaskRevolutionParameters_H

#include <Gui/TaskView/TaskView.h>
#include <Gui/Selection.h>
#include <Gui/TaskView/TaskDialog.h>

#include "TaskSketchBasedParameters.h"
#include "ViewProviderRevolution.h"

class Ui_TaskRevolutionParameters;

namespace App {
class Property;
}

namespace Gui {
class ViewProvider;
}

namespace PartDesignGui {



class TaskRevolutionParameters : public TaskSketchBasedParameters
{
    Q_OBJECT

public:
    TaskRevolutionParameters(ViewProvider* RevolutionView,QWidget *parent = 0);
    ~TaskRevolutionParameters();

    virtual void apply() override;

    /**
     * @brief fillAxisCombo fills the combo and selects the item according to
     * current value of revolution object's axis reference.
     * @param forceRefill if true, the combo box will be completely refilled. If
     * false, the current value of revolution object's axis will be added to the
     * list (if necessary), and selected. If the list is empty, it will be refilled anyway.
     */
    void fillAxisCombo(bool forceRefill = false);
    void addAxisToCombo(App::DocumentObject *linkObj, std::string linkSubname, QString itemText);

private Q_SLOTS:
    void onAngleChanged(double);
    void onAxisChanged(int);
    void onMidplane(bool);
    void onReversed(bool);

protected:
    void onSelectionChanged(const Gui::SelectionChanges& msg) override;
    void changeEvent(QEvent *e) override;
    bool updateView() const;
    void getReferenceAxis(App::DocumentObject *&obj, std::vector<std::string> &sub) const;
    double getAngle(void) const;
    bool getMidplane(void) const;
    bool getReversed(void) const;

    //mirrors of revolution's or groove's properties
    //should have been done by inheriting revolution and groove from common class...
    App::PropertyAngle* propAngle;
    App::PropertyBool* propReversed;
    App::PropertyBool* propMidPlane;
    App::PropertyLinkSub* propReferenceAxis;

private:
    void updateUI();

private:
    QWidget* proxy;
    Ui_TaskRevolutionParameters* ui;    

    /**
     * @brief axesInList is the list of links corresponding to axis combo; must
     * be kept in sync with the combo. A special value of zero-pointer link is
     * for "Select axis" item.
     *
     * It is a list of pointers, because properties prohibit assignment. Use new
     * when adding stuff, and delete when removing stuff.
     */
    std::vector<App::PropertyLinkSub*> axesInList;
};

/// simulation dialog for the TaskView
class TaskDlgRevolutionParameters : public TaskDlgSketchBasedParameters
{
    Q_OBJECT

public:
    TaskDlgRevolutionParameters(PartDesignGui::ViewProvider *RevolutionView);

    ViewProvider* getRevolutionView() const
    { return vp; }
};

} //namespace PartDesignGui

#endif // GUI_TASKVIEW_TASKAPPERANCE_H
