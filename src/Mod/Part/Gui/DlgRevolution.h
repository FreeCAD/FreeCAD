/***************************************************************************
 *   Copyright (c) 2009 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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

#ifndef PARTGUI_DLGREVOLUTION_H
#define PARTGUI_DLGREVOLUTION_H

#include <Gui/Selection.h>
#include <Gui/InputVector.h>
#include <Gui/TaskView/TaskDialog.h>
#include <Gui/TaskView/TaskView.h>

namespace PartGui {

class Ui_DlgRevolution;
class DlgRevolution : public Gui::LocationDialog, public Gui::SelectionObserver
{
    Q_OBJECT

public:
    DlgRevolution(QWidget* parent = 0, Qt::WFlags fl = 0);
    ~DlgRevolution();
    void accept();

    Base::Vector3d getDirection() const;

protected:
    void changeEvent(QEvent *e);

private Q_SLOTS:
    void on_selectLine_clicked();

private:
    void findShapes();
    void directionActivated(int);
    void onSelectionChanged(const Gui::SelectionChanges& msg);

private:
    typedef Gui::LocationInterfaceComp<Ui_DlgRevolution> Ui_RevolutionComp;
    Ui_RevolutionComp* ui;
    class EdgeSelection;
    EdgeSelection* filter;
};

class TaskRevolution : public Gui::TaskView::TaskDialog
{
    Q_OBJECT

public:
    TaskRevolution();
    ~TaskRevolution();

public:
    bool accept();

    virtual QDialogButtonBox::StandardButtons getStandardButtons() const
    { return QDialogButtonBox::Ok | QDialogButtonBox::Cancel; }

private:
    DlgRevolution* widget;
    Gui::TaskView::TaskBox* taskbox;
};

} // namespace PartGui

#endif // PARTGUI_DLGREVOLUTION_H
