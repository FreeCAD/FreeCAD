// SPDX-License-Identifier: LGPL-2.1-or-later
/****************************************************************************
 *                                                                          *
 *   Copyright (c) 2026 AstoCAD     <hello@astocad.com>                     *
 *                                                                          *
 *   This file is part of FreeCAD.                                          *
 *                                                                          *
 *   FreeCAD is free software: you can redistribute it and/or modify it     *
 *   under the terms of the GNU Lesser General Public License as            *
 *   published by the Free Software Foundation, either version 2.1 of the   *
 *   License, or (at your option) any later version.                        *
 *                                                                          *
 *   FreeCAD is distributed in the hope that it will be useful, but         *
 *   WITHOUT ANY WARRANTY; without even the implied warranty of             *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU       *
 *   Lesser General Public License for more details.                        *
 *                                                                          *
 *   You should have received a copy of the GNU Lesser General Public       *
 *   License along with FreeCAD. If not, see                                *
 *   <https://www.gnu.org/licenses/>.                                       *
 *                                                                          *
 ***************************************************************************/

#pragma once

#include <QDialogButtonBox>
#include <QWidget>
#include <vector>

#include <Gui/TaskView/TaskDialog.h>
#include <Mod/TechDraw/TechDrawGlobal.h>

class QTreeWidget;
class QTreeWidgetItem;

namespace App
{
class DocumentObject;
}

namespace TechDraw
{
class DrawPage;
class DrawView;
}

namespace TechDrawGui
{

class TechDrawGuiExport TaskNewSketch: public QWidget
{
    Q_OBJECT

public:
    explicit TaskNewSketch(TechDraw::DrawPage* page, QWidget* parent = nullptr);
    bool createSketch();

private:
    void addViewChildren(TechDraw::DrawView* owner, QTreeWidgetItem* parent);
    TechDraw::DrawView* selectedOwner() const;

    TechDraw::DrawPage* m_page;
    QTreeWidget* m_ownerTree;
    std::vector<TechDraw::DrawView*> m_views;
};

class TechDrawGuiExport TaskDlgNewSketch: public Gui::TaskView::TaskDialog
{
    Q_OBJECT

public:
    explicit TaskDlgNewSketch(TechDraw::DrawPage* page);

    bool accept() override;
    bool reject() override;
    QDialogButtonBox::StandardButtons getStandardButtons() const override
    {
        return QDialogButtonBox::Ok | QDialogButtonBox::Cancel;
    }

private:
    TaskNewSketch* m_widget;
};

}  // namespace TechDrawGui
