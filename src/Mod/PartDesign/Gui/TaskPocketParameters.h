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

    void fillDirectionCombo();
    void addAxisToCombo(App::DocumentObject* linkObj, std::string linkSubname, QString itemText,
        bool hasSketch = true);

private Q_SLOTS:
    void onLengthChanged(double);
    void onLength2Changed(double);
    void onOffsetChanged(double);
    void onDirectionCBChanged(int);
    void onAlongSketchNormalChanged(bool);
    void onDirectionToggled(bool);
    void onXDirectionEditChanged(double);
    void onYDirectionEditChanged(double);
    void onZDirectionEditChanged(double);
    void onMidplaneChanged(bool);
    void onReversedChanged(bool);
    void onButtonFace(const bool pressed = true);
    void onFaceName(const QString& text);
    void onModeChanged(int);

protected:
    void changeEvent(QEvent *e) override;
    App::PropertyLinkSub* propReferenceAxis;
    void getReferenceAxis(App::DocumentObject*& obj, std::vector<std::string>& sub) const;

private:
    double getLength(void) const;
    double getLength2(void) const;
    double getOffset(void) const;
    bool   getAlongSketchNormal(void) const;
    bool   getCustom(void) const;
    std::string getReferenceAxis(void) const;
    double getXDirection(void) const;
    double getYDirection(void) const;
    double getZDirection(void) const;
    int    getMode(void) const;
    bool   getMidplane(void) const;
    bool   getReversed(void) const;
    QString getFaceName(void) const;

    void onSelectionChanged(const Gui::SelectionChanges& msg) override;
    void updateUI(int index);
    void updateDirectionEdits(bool Reversed = false);

private:
    QWidget* proxy;
    std::unique_ptr<Ui_TaskPocketParameters> ui;
    double oldLength;
    bool selectionFace;
    std::vector<std::unique_ptr<App::PropertyLinkSub>> axesInList;
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
