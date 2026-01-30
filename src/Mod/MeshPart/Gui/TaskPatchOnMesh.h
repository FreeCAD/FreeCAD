// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2026 Werner Mayer <wmayer[at]users.sourceforge.net>     *
 *                                                                         *
 *   This file is part of FreeCAD.                                         *
 *                                                                         *
 *   FreeCAD is free software: you can redistribute it and/or modify it    *
 *   under the terms of the GNU Lesser General Public License as           *
 *   published by the Free Software Foundation, either version 2.1 of the  *
 *   License, or (at your option) any later version.                       *
 *                                                                         *
 *   FreeCAD is distributed in the hope that it will be useful, but        *
 *   WITHOUT ANY WARRANTY; without even the implied warranty of            *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU      *
 *   Lesser General Public License for more details.                       *
 *                                                                         *
 *   You should have received a copy of the GNU Lesser General Public      *
 *   License along with FreeCAD. If not, see                               *
 *   <https://www.gnu.org/licenses/>.                                      *
 *                                                                         *
 **************************************************************************/

#ifndef MESHPART_GUI_TASKPATCHONMESH_H
#define MESHPART_GUI_TASKPATCHONMESH_H

#include <memory>
#include <QPointer>
#include <Gui/TaskView/TaskDialog.h>
#include <Gui/TaskView/TaskView.h>

namespace Gui
{
class View3DInventor;
}

namespace MeshPartGui
{

class Ui_TaskPatchOnMesh;
class PatchOnMeshHandler;

class PatchOnMeshWidget: public QWidget
{
    Q_OBJECT
    Q_DISABLE_COPY_MOVE(PatchOnMeshWidget)

public:
    explicit PatchOnMeshWidget(Gui::View3DInventor* view, QWidget* parent = nullptr);
    ~PatchOnMeshWidget() override;

    void reject();

protected:
    void changeEvent(QEvent* e) override;
    void setup();

private:
    void onStartButtonClicked();

private:
    std::unique_ptr<Ui_TaskPatchOnMesh> ui;
    PatchOnMeshHandler* myGridHandler;
    QPointer<Gui::View3DInventor> myView;
};

class TaskPatchOnMesh: public Gui::TaskView::TaskDialog
{
    Q_OBJECT

public:
    explicit TaskPatchOnMesh(Gui::View3DInventor* view);

public:
    bool reject() override;

    QDialogButtonBox::StandardButtons getStandardButtons() const override
    {
        return QDialogButtonBox::Close;
    }

private:
    PatchOnMeshWidget* widget;
};
}  // namespace MeshPartGui

#endif  // MESHPART_GUI_TASKPATCHONMESH_H
