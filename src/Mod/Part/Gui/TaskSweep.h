/***************************************************************************
 *   Copyright (c) 2011 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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


#ifndef PARTGUI_TASKSWEEP_H
#define PARTGUI_TASKSWEEP_H

#include <Gui/TaskView/TaskView.h>
#include <Gui/TaskView/TaskDialog.h>

class QTreeWidgetItem;

namespace Gui {
class SelectionObject;
class StatusWidget;
}
namespace PartGui {

class SweepWidget : public QWidget
{
    Q_OBJECT

public:
    explicit SweepWidget(QWidget* parent = nullptr);
    ~SweepWidget() override;

    bool accept();
    bool reject();

private:
    void onCurrentItemChanged(QTreeWidgetItem*, QTreeWidgetItem*);
    void onButtonPathToggled(bool);

private:
    void changeEvent(QEvent *e) override;
    void findShapes();
    bool isPathValid(const Gui::SelectionObject& sel) const;

private:
    class Private;
    Private* d;
};

class TaskSweep : public Gui::TaskView::TaskDialog
{
    Q_OBJECT

public:
    TaskSweep();
    ~TaskSweep() override;

public:
    void open() override;
    bool accept() override;
    bool reject() override;
    void clicked(int) override;

    QDialogButtonBox::StandardButtons getStandardButtons() const override
    { return QDialogButtonBox::Ok|QDialogButtonBox::Cancel|QDialogButtonBox::Help; }

private:
    SweepWidget* widget;
    Gui::StatusWidget* label;
    Gui::TaskView::TaskBox* taskbox;
};

} //namespace PartGui

#endif // PARTGUI_TASKSWEEP_H
