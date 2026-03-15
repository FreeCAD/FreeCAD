// SPDX-License-Identifier: LGPL-2.1-or-later

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

#pragma once

#include <QPointer>

#include <Gui/TaskView/TaskDialog.h>
#include <Gui/TaskView/TaskView.h>


namespace Gui
{
class View3DInventor;
}

namespace MeshPartGui
{

class Ui_TaskCurveOnMesh;
class CurveOnMeshHandler;

class CurveOnMeshWidget: public QWidget
{
    Q_OBJECT

public:
    explicit CurveOnMeshWidget(Gui::View3DInventor* view, QWidget* parent = nullptr);
    ~CurveOnMeshWidget() override;

    void reject();

protected:
    void changeEvent(QEvent* e) override;
    void setup();

private:
    void onStartButtonClicked();

private:
    Ui_TaskCurveOnMesh* ui;
    CurveOnMeshHandler* myCurveHandler;
    QPointer<Gui::View3DInventor> myView;
};

class TaskCurveOnMesh: public Gui::TaskView::TaskDialog
{
    Q_OBJECT

public:
    explicit TaskCurveOnMesh(Gui::View3DInventor* view);

public:
    bool reject() override;

    QDialogButtonBox::StandardButtons getStandardButtons() const override
    {
        return QDialogButtonBox::Close;
    }

private:
    CurveOnMeshWidget* widget;
};

}  // namespace MeshPartGui
