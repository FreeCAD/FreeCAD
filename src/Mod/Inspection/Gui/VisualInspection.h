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

#include <Gui/TaskView/TaskDialog.h>

#include <QWidget>


class QTreeWidgetItem;
class QDialogButtonBox;

namespace InspectionGui
{
class Ui_VisualInspection;
class VisualInspection: public QWidget
{
    Q_OBJECT

public:
    explicit VisualInspection(QWidget* parent = nullptr);
    ~VisualInspection() override;

    bool accept();
    bool isAcceptable() const;

Q_SIGNALS:
    void acceptabilityChanged(bool acceptable);

protected Q_SLOTS:
    void onActivateItem(QTreeWidgetItem*);
    void loadSettings();
    void saveSettings();

private:
    Ui_VisualInspection* ui;
    bool acceptable {false};
};

class TaskVisualInspection: public Gui::TaskView::TaskDialog
{
    Q_OBJECT

public:
    TaskVisualInspection();
    ~TaskVisualInspection() override;

    QDialogButtonBox::StandardButtons getStandardButtons() const override;
    bool accept() override;
    void modifyStandardButtons(QDialogButtonBox* box) override;

private:
    VisualInspection* widget;
};

}  // namespace InspectionGui
