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


#ifndef PARTGUI_TASKTHICKNESS_H
#define PARTGUI_TASKTHICKNESS_H

#include <Gui/TaskView/TaskView.h>
#include <Gui/TaskView/TaskDialog.h>

namespace Part { class Thickness; }
namespace PartGui { 

class ThicknessWidget : public QWidget
{
    Q_OBJECT

public:
    ThicknessWidget(Part::Thickness*, QWidget* parent = 0);
    ~ThicknessWidget();

    bool accept();
    bool reject();
    Part::Thickness* getObject() const;

private Q_SLOTS:
    void on_spinOffset_valueChanged(double);
    void on_modeType_activated(int);
    void on_joinType_activated(int);
    void on_intersection_toggled(bool);
    void on_selfIntersection_toggled(bool);
    void on_facesButton_clicked();
    void on_updateView_toggled(bool);

private:
    void changeEvent(QEvent *e);

private:
    class Private;
    Private* d;
};

class TaskThickness : public Gui::TaskView::TaskDialog
{
    Q_OBJECT

public:
    TaskThickness(Part::Thickness*);
    ~TaskThickness();

public:
    void open();
    bool accept();
    bool reject();
    void clicked(int);
    Part::Thickness* getObject() const;

    QDialogButtonBox::StandardButtons getStandardButtons() const
    { return QDialogButtonBox::Ok|QDialogButtonBox::Cancel; }

private:
    ThicknessWidget* widget;
    Gui::TaskView::TaskBox* taskbox;
};

} //namespace PartGui

#endif // PARTGUI_TASKTHICKNESS_H
