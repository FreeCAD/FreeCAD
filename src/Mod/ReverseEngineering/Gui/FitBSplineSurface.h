// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2015 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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

#include <Gui/TaskView/TaskDialog.h>
#include <Gui/TaskView/TaskView.h>


namespace ReenGui
{

class FitBSplineSurfaceWidget: public QWidget
{
    Q_OBJECT

public:
    explicit FitBSplineSurfaceWidget(const App::DocumentObjectT&, QWidget* parent = nullptr);
    ~FitBSplineSurfaceWidget() override;

    bool accept();

private:
    void restoreSettings();
    void saveSettings();
    void changeEvent(QEvent* e) override;

private:
    void onMakePlacementClicked();

private:
    class Private;
    Private* d;
};

class TaskFitBSplineSurface: public Gui::TaskView::TaskDialog
{
    Q_OBJECT

public:
    explicit TaskFitBSplineSurface(const App::DocumentObjectT&);

public:
    void open() override;
    bool accept() override;

    QDialogButtonBox::StandardButtons getStandardButtons() const override
    {
        return QDialogButtonBox::Ok | QDialogButtonBox::Cancel;
    }

private:
    FitBSplineSurfaceWidget* widget;
};

}  // namespace ReenGui
