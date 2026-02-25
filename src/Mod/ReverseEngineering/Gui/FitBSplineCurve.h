// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2024 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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

#pragma once

#include <Gui/TaskView/TaskDialog.h>
#include <Gui/TaskView/TaskView.h>


namespace ReenGui
{

class FitBSplineCurveWidget: public QWidget
{
    Q_OBJECT

public:
    explicit FitBSplineCurveWidget(const App::DocumentObjectT&, QWidget* parent = nullptr);
    ~FitBSplineCurveWidget() override;

    bool accept();

protected:
    void changeEvent(QEvent* e) override;

private:
    void toggleParametrizationType(bool on);
    void toggleSmoothing(bool on);
    void tryAccept();
    void exeCommand(const QString&);
    void tryCommand(const QString&);

private:
    class Private;
    Private* d;
};

class TaskFitBSplineCurve: public Gui::TaskView::TaskDialog
{
    Q_OBJECT

public:
    explicit TaskFitBSplineCurve(const App::DocumentObjectT&);

public:
    void open() override;
    bool accept() override;

    QDialogButtonBox::StandardButtons getStandardButtons() const override
    {
        return QDialogButtonBox::Ok | QDialogButtonBox::Cancel;
    }

private:
    FitBSplineCurveWidget* widget;
};

}  // namespace ReenGui
