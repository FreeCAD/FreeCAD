// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2007 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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

class QTreeWidgetItem;

namespace App
{
class DocumentObject;
class Property;
}  // namespace App
namespace PartGui
{

class Ui_Mirroring;
class Mirroring: public QWidget, public Gui::SelectionObserver
{
    Q_OBJECT

public:
    explicit Mirroring(QWidget* parent = nullptr);
    ~Mirroring() override;
    bool accept();
    bool reject();

protected:
    void changeEvent(QEvent* e) override;

private:
    void findShapes();
    void onSelectButtonClicked();

private:
    void onSelectionChanged(const Gui::SelectionChanges& msg) override;
    QString document;
    std::unique_ptr<Ui_Mirroring> ui;
};

class TaskMirroring: public Gui::TaskView::TaskDialog
{
    Q_OBJECT

public:
    TaskMirroring();

public:
    bool accept() override;
    bool reject() override;

    QDialogButtonBox::StandardButtons getStandardButtons() const override
    {
        return QDialogButtonBox::Ok | QDialogButtonBox::Cancel;
    }
    bool isAllowedAlterDocument() const override
    {
        return false;
    }
    bool needsFullSpace() const override
    {
        return false;
    }

private:
    Mirroring* widget;
};

}  // namespace PartGui
