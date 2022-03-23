/***************************************************************************
 *   Copyright (c) 2017 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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

#ifndef MESHPART_GUI_TASKCURVEONMESH_H
#define MESHPART_GUI_TASKCURVEONMESH_H

#include <Gui/TaskView/TaskDialog.h>
#include <Gui/TaskView/TaskView.h>
#include <QPointer>

namespace Gui {
class View3DInventor;
}

namespace MeshPartGui
{

class Ui_TaskCurveOnMesh;
class CurveOnMeshHandler;

class CurveOnMeshWidget : public QWidget
{
    Q_OBJECT

public:
    CurveOnMeshWidget(Gui::View3DInventor* view, QWidget* parent=nullptr);
    ~CurveOnMeshWidget();

    void reject();

protected:
    void changeEvent(QEvent *e);
    void setup();

private Q_SLOTS:
    void on_startButton_clicked();

private:
    Ui_TaskCurveOnMesh* ui;
    CurveOnMeshHandler* myCurveHandler;
    QPointer<Gui::View3DInventor> myView;
};

class TaskCurveOnMesh : public Gui::TaskView::TaskDialog
{
    Q_OBJECT

public:
    TaskCurveOnMesh(Gui::View3DInventor* view);
    ~TaskCurveOnMesh();

public:
    bool reject();

    virtual QDialogButtonBox::StandardButtons getStandardButtons() const
    { return QDialogButtonBox::Close; }

private:
    CurveOnMeshWidget* widget;
    Gui::TaskView::TaskBox* taskbox;
};

} //namespace MeshPartGui

#endif // MESHPART_GUI_TASKCURVEONMESH_H
