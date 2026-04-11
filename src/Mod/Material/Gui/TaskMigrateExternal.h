// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2024 David Carter <dcarter@david.carter.ca>             *
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

#include <memory>

#include <QPushButton>

#include <Gui/TaskView/TaskDialog.h>

#include <Mod/Material/App/ModelManager.h>
#include <Mod/Material/App/MaterialManager.h>

namespace MatGui {

class Ui_TaskMigrateExternal;

class DlgMigrateExternal: public QWidget
{
    Q_OBJECT

public:
    explicit DlgMigrateExternal(QWidget* parent = nullptr);
    ~DlgMigrateExternal() override = default;
    void migrate();
    void statusUpdate(const QString& status);

private:
    void showLibraries();

    std::shared_ptr<Ui_TaskMigrateExternal> ui;
};

class TaskMigrateExternal: public Gui::TaskView::TaskDialog
{
    Q_OBJECT

public:
    TaskMigrateExternal();

public:
    bool accept() override;
    bool reject() override;
    QDialogButtonBox::StandardButtons getStandardButtons() const override;
    void modifyStandardButtons(QDialogButtonBox*) override;
    void onMigrate(bool checked);

private:
    DlgMigrateExternal* _widget;
    QPushButton* _migrateButton;
    QPushButton* _closeButton;
};

} // namespace MatGui