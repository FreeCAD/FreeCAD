// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2023 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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
#include <App/DocumentObserver.h>
#include <App/GeoFeature.h>
#include <memory>

namespace Gui
{

class Ui_TaskOrientation;
class TaskOrientation: public QWidget
{
    Q_OBJECT

public:
    explicit TaskOrientation(App::GeoFeature* obj, QWidget* parent = nullptr);
    ~TaskOrientation() override;

    void open();
    void accept();
    void reject();

private:
    void restore(const Base::Placement&);
    void onPreview();
    void updateIcon();
    void updatePlacement();

private:
    std::unique_ptr<Ui_TaskOrientation> ui;
    App::WeakPtrT<App::GeoFeature> feature;
};

class TaskOrientationDialog: public Gui::TaskView::TaskDialog
{
    Q_OBJECT

public:
    explicit TaskOrientationDialog(App::GeoFeature* obj);

public:
    void open() override;
    bool accept() override;
    bool reject() override;

    QDialogButtonBox::StandardButtons getStandardButtons() const override
    {
        return QDialogButtonBox::Ok | QDialogButtonBox::Cancel;
    }

private:
    TaskOrientation* widget;
};

}  // namespace Gui
