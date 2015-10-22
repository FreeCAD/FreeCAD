/***************************************************************************
 *   Copyright (c) 2012 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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


#ifndef PARTGUI_TASKOFFSET_H
#define PARTGUI_TASKOFFSET_H

#include <Gui/TaskView/TaskView.h>
#include <Gui/TaskView/TaskDialog.h>

namespace Part { class Offset; }
namespace PartGui { 

class OffsetWidget : public QWidget
{
    Q_OBJECT

public:
    OffsetWidget(Part::Offset*, QWidget* parent = 0);
    ~OffsetWidget();

    bool accept();
    bool reject();
    Part::Offset* getObject() const;

private Q_SLOTS:
    void on_spinOffset_valueChanged(double);
    void on_modeType_activated(int);
    void on_joinType_activated(int);
    void on_intersection_toggled(bool);
    void on_selfIntersection_toggled(bool);
    void on_fillOffset_toggled(bool);
    void on_updateView_toggled(bool);

private:
    void changeEvent(QEvent *e);

private:
    class Private;
    Private* d;
};

class TaskOffset : public Gui::TaskView::TaskDialog
{
    Q_OBJECT

public:
    TaskOffset(Part::Offset*);
    ~TaskOffset();

public:
    void open();
    bool accept();
    bool reject();
    void clicked(int);
    Part::Offset* getObject() const;

    QDialogButtonBox::StandardButtons getStandardButtons() const
    { return QDialogButtonBox::Ok|QDialogButtonBox::Cancel; }

private:
    OffsetWidget* widget;
    Gui::TaskView::TaskBox* taskbox;
};

} //namespace PartGui

#endif // PARTGUI_TASKOFFSET_H
