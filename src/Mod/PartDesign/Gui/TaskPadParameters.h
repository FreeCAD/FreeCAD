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


#ifndef GUI_TASKVIEW_TaskPadParameters_H
#define GUI_TASKVIEW_TaskPadParameters_H

#include <App/DocumentObserver.h>
#include <Gui/TaskView/TaskView.h>
#include <Gui/Selection.h>
#include <Gui/TaskView/TaskDialog.h>

#include "TaskSketchBasedParameters.h"
#include "ViewProviderPad.h"

class Ui_TaskPadParameters;

namespace App {
class Property;
}

namespace Gui {
class ViewProvider;
}

namespace PartDesignGui {


class TaskPadParameters : public TaskSketchBasedParameters
{
    Q_OBJECT

public:
    TaskPadParameters(ViewProviderPad *PadView, QWidget *parent = 0, bool newObj=false);

    TaskPadParameters(ViewProviderPad *PadView, QWidget *parent, bool newObj,
                      const std::string& pixmapname, const QString& parname);

    ~TaskPadParameters();

    virtual void saveHistory() override;
    virtual void apply() override;

    void fillDirectionCombo();
    void addAxisToCombo(App::DocumentObject* linkObj, std::string linkSubname, QString itemText);

private Q_SLOTS:
    void onLengthChanged(double);
    void onLength2Changed(double);
    void onDirectionCBChanged(int);
    void onAlongSketchNormalChanged(bool);
    void onXDirectionEditChanged(double);
    void onYDirectionEditChanged(double);
    void onZDirectionEditChanged(double);
    void onOffsetChanged(double);
    void onMidplaneChanged(bool);
    void onReversedChanged(bool);
    void onUsePipeChanged(bool);
    void onCheckFaceLimitsChanged(bool);
    void onButtonFace(const bool pressed = true);
    void onFaceName(const QString& text);
    void onModeChanged(int);
    void onAngleChanged(double);
    void onAngle2Changed(double);
    void onInnerAngleChanged(double);
    void onInnerAngle2Changed(double);

protected:
    void changeEvent(QEvent *e) override;
    bool eventFilter(QObject*, QEvent*) override;
    void refresh() override;
    void getReferenceAxis(App::DocumentObject*& obj, std::vector<std::string>& sub) const;

private:
    double getLength(void) const;
    double getLength2(void) const;
    bool   getAlongSketchNormal(void) const;
    bool   getCustom(void) const;
    std::string getReferenceAxis(void) const;
    double getXDirection(void) const;
    double getYDirection(void) const;
    double getZDirection(void) const;
    double getOffset(void) const;
    bool   getReversed(void) const;
    bool   getMidplane(void) const;
    int    getMode(void) const;
    QString getFaceName(void) const;
    void onSelectionChanged(const Gui::SelectionChanges& msg) override;
    void updateUI(int index);
    void updateDirectionEdits(void);
    void setupUI(bool newObj);

private:
    QWidget* proxy;
    std::unique_ptr<Ui_TaskPadParameters> ui;
    bool useElement;
    bool selectionFace;
    App::PropertyLinkSub* propReferenceAxis = nullptr;
    std::vector<App::SubObjectT> axesInList;
};

/// simulation dialog for the TaskView
class TaskDlgPadParameters : public TaskDlgSketchBasedParameters
{
    Q_OBJECT

public:
    TaskDlgPadParameters(ViewProviderPad *PadView, bool newObj=false);
    TaskDlgPadParameters(ViewProviderPad *PadView, bool newObj,
                         const std::string& pixmapname, const QString& parname);
    virtual bool accept();

    ViewProviderPad* getPadView() const
    { return static_cast<ViewProviderPad*>(vp); }
};

} //namespace PartDesignGui

#endif // GUI_TASKVIEW_TASKAPPERANCE_H
