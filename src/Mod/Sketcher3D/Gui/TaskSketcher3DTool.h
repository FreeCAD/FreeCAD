// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2026 Yash Suthar <yashsuthar983@gmail.com>              *
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

#include <QStringList>

#include <Gui/TaskView/TaskView.h>

class QLabel;
class QListWidget;
class QListWidgetItem;

namespace Sketcher3DGui
{

class ViewProviderSketch3D;

class TaskSketcher3DTool: public Gui::TaskView::TaskBox
{
    Q_OBJECT

public:
    explicit TaskSketcher3DTool(ViewProviderSketch3D* sketchView);
    ~TaskSketcher3DTool() override;

    /// rebuild lists
    void refresh();

    /// Update the hint line.
    void setHint(const QString& text);

private Q_SLOTS:
    void onElementRowClicked(QListWidgetItem* item);
    void onConstraintRowClicked(QListWidgetItem* item);

private:
    void populateStatus();
    void populateElements();
    void populateConstraints();
    void selectSubNames(const QStringList& subnames) const;
    /// Delete every currently-selected row of constraintsList.

    ViewProviderSketch3D* sketchView {nullptr};

    QLabel* hintLabel {nullptr};
    QLabel* statusLabel {nullptr};
    QLabel* planeLabel {nullptr};
    QListWidget* elementsList {nullptr};
    QListWidget* constraintsList {nullptr};
    QLabel* elementsHeader {nullptr};
    QLabel* constraintsHeader {nullptr};
};

}  // namespace Sketcher3DGui
