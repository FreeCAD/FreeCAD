/***************************************************************************
 *   Copyright (c) 2013 Jan Rheinlaender <jrheinlaender@users.sourceforge.net>        *
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


#ifndef GUI_TASKVIEW_TaskDatumParameters_H
#define GUI_TASKVIEW_TaskDatumParameters_H

#include <Gui/TaskView/TaskView.h>
#include <Gui/Selection.h>
#include <Gui/TaskView/TaskDialog.h>

#include "ViewProviderDatum.h"

class Ui_TaskDatumParameters;
class QLineEdit;

namespace App {
class Property;
}

namespace Gui {
class ViewProvider;
}

namespace PartDesignGui { 



class TaskDatumParameters : public Gui::TaskView::TaskBox, public Gui::SelectionObserver
{
    Q_OBJECT

public:
    TaskDatumParameters(ViewProviderDatum *DatumView,QWidget *parent = 0);
    ~TaskDatumParameters();

    QString getReference(const int idx) const;
    double getValue1(void) const;
    bool   getCheck1(void) const;

private Q_SLOTS:
    void onValue1Changed(double);
    void onCheckBox1(bool);
    void onRefName1(const QString& text);
    void onRefName2(const QString& text);
    void onRefName3(const QString& text);
    void onButtonRef1(const bool pressed = true);
    void onButtonRef2(const bool pressed = true);
    void onButtonRef3(const bool pressed = true);

protected:
    void changeEvent(QEvent *e);

private:
    void onSelectionChanged(const Gui::SelectionChanges& msg);
    void updateUI();

    void makeRefStrings(std::vector<QString>& refstrings, std::vector<std::string>& refnames);
    QLineEdit* getLine(const int idx);
    void onButtonRef(const bool pressed, const int idx);
    void onRefName(const QString& text, const int idx);

private:
    QWidget* proxy;
    Ui_TaskDatumParameters* ui;
    ViewProviderDatum *DatumView;

    int refSelectionMode;

};

/// simulation dialog for the TaskView
class TaskDlgDatumParameters : public Gui::TaskView::TaskDialog
{
    Q_OBJECT

public:
    TaskDlgDatumParameters(ViewProviderDatum *DatumView);
    ~TaskDlgDatumParameters();

    ViewProviderDatum* getDatumView() const
    { return DatumView; }


public:
    /// is called the TaskView when the dialog is opened
    virtual void open();
    /// is called by the framework if an button is clicked which has no accept or reject role
    virtual void clicked(int);
    /// is called by the framework if the dialog is accepted (Ok)
    virtual bool accept();
    /// is called by the framework if the dialog is rejected (Cancel)
    virtual bool reject();
    /// is called by the framework if the user presses the help button 
    virtual bool isAllowedAlterDocument(void) const
    { return false; }

    /// returns for Close and Help button 
    virtual QDialogButtonBox::StandardButtons getStandardButtons(void) const
    { return QDialogButtonBox::Ok|QDialogButtonBox::Cancel; }

protected:
    ViewProviderDatum   *DatumView;

    TaskDatumParameters  *parameter;
};

} //namespace PartDesignGui

#endif // GUI_TASKVIEW_TASKAPPERANCE_H
