/***************************************************************************
 *   Copyright (c) 2026 Théo Veilleux-Trinh <theo.veilleux.trinh@proton.me>*
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

#include "TaskView/TaskDialog.h"
#include "TaskView/TaskView.h"

#include <QTreeWidgetItem>

#include <functional>
#include <vector>

namespace App
{
class DocumentObject;
}

namespace Gui
{
class Document;
class Ui_TaskCommandLinkDialog;

class TaskCommandLink: public Gui::TaskView::TaskBox
{
public:
    TaskCommandLink();
    ~TaskCommandLink();

    std::vector<App::DocumentObject*> selectedObjects();

private:
    void buildObjectsList();

private:
    Ui_TaskCommandLinkDialog* ui {nullptr};
    QWidget* proxy {nullptr};
};

class TaskCommandLinkDialog: public Gui::TaskView::TaskDialog
{
    Q_OBJECT

public:
    TaskCommandLinkDialog(std::function<void(std::vector<App::DocumentObject*>)> executor_);
    ~TaskCommandLinkDialog() override = default;

    QDialogButtonBox::StandardButtons getStandardButtons() const override
    {
        return QDialogButtonBox::Ok | QDialogButtonBox::Cancel;
    }

    void open() override;
    bool accept() override;

private:
    TaskCommandLink* commandLink {nullptr};
    Gui::Document* document {nullptr};
    std::function<void(std::vector<App::DocumentObject*>)> executor;
};
}  // namespace Gui
