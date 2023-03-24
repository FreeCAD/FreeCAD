/***************************************************************************
 *   Copyright (c) 2012 Jan Rheinländer                                    *
 *                                   <jrheinlaender@users.sourceforge.net> *
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


#ifndef GUI_TASKVIEW_TaskDraftParameters_H
#define GUI_TASKVIEW_TaskDraftParameters_H

#include "TaskDressUpParameters.h"
#include "ViewProviderDraft.h"

class Ui_TaskDraftParameters;

namespace PartDesignGui {

class TaskDraftParameters : public TaskDressUpParameters
{
    Q_OBJECT

public:
    explicit TaskDraftParameters(ViewProviderDressUp *DressUpView, QWidget *parent=nullptr);
    ~TaskDraftParameters() override;

    void apply() override;

    double getAngle() const;
    bool getReversed() const;
    const std::vector<std::string> getFaces() const;
    void getPlane(App::DocumentObject*& obj, std::vector<std::string>& sub) const;
    void getLine(App::DocumentObject*& obj, std::vector<std::string>& sub) const;

private Q_SLOTS:
    void onAngleChanged(double angle);
    void onReversedChanged(bool reversed);
    void onButtonPlane(const bool checked);
    void onButtonLine(const bool checked);
    void onRefDeleted() override;

protected:
    void setButtons(const selectionModes mode) override;
    bool event(QEvent *e) override;
    void changeEvent(QEvent *e) override;
    void onSelectionChanged(const Gui::SelectionChanges& msg) override;

private:
    std::unique_ptr<Ui_TaskDraftParameters> ui;
};

/// simulation dialog for the TaskView
class TaskDlgDraftParameters : public TaskDlgDressUpParameters
{
    Q_OBJECT

public:
    explicit TaskDlgDraftParameters(ViewProviderDraft *DraftView);
    ~TaskDlgDraftParameters() override;

public:
    /// is called by the framework if the dialog is accepted (Ok)
    bool accept() override;
};

} //namespace PartDesignGui

#endif // GUI_TASKVIEW_TASKAPPERANCE_H
