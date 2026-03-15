// SPDX-License-Identifier: LGPL-2.1-or-later

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

#pragma once

#include <Gui/Selection/Selection.h>
#include <Gui/TaskView/TaskDialog.h>
#include <Gui/TaskView/TaskView.h>


namespace PartGui
{

class ShapeBuilderWidget: public QWidget, public Gui::SelectionObserver
{
    Q_OBJECT

public:
    explicit ShapeBuilderWidget(QWidget* parent = nullptr);
    ~ShapeBuilderWidget() override;

    bool accept();
    bool reject();

private:
    void onCreateButtonClicked();
    void onSelectButtonClicked();
    void switchMode(int);

private:
    void onSelectionChanged(const Gui::SelectionChanges& msg) override;

private:
    void createEdgeFromVertex();
    void createWireFromEdge();
    void createFaceFromVertex();
    void createFaceFromEdge();
    void createShellFromFace();
    void createSolidFromShell();
    void changeEvent(QEvent* e) override;

private:
    class Private;
    Private* d;
};

class TaskShapeBuilder: public Gui::TaskView::TaskDialog
{
    Q_OBJECT

public:
    TaskShapeBuilder();
    ~TaskShapeBuilder() override;

public:
    void open() override;
    bool accept() override;
    bool reject() override;
    void clicked(int) override;

    QDialogButtonBox::StandardButtons getStandardButtons() const override
    {
        return QDialogButtonBox::Close;
    }

private:
    ShapeBuilderWidget* widget;
};

}  // namespace PartGui
