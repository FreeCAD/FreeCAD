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

#include <memory>

#include <Gui/TaskView/TaskDialog.h>
#include <Mod/TechDraw/App/DrawViewPart.h>
#include <Mod/TechDraw/TechDrawGlobal.h>

class Ui_TaskBrokenView;

namespace TechDraw
{
class DrawPage;
}

namespace TechDrawGui
{

class QGVPage;

class TechDrawGuiExport TaskBrokenView : public QWidget
{
public:
    explicit TaskBrokenView(QWidget* parent = nullptr);
    ~TaskBrokenView() override;

    TechDraw::DrawViewPart::BreakType breakType() const;
    double gap() const;
    double angle() const;

private:
    std::unique_ptr<Ui_TaskBrokenView> ui;
};

class TechDrawGuiExport TaskDlgBrokenView : public Gui::TaskView::TaskDialog
{
public:
    TaskDlgBrokenView(TechDraw::DrawPage* page, QGVPage* graphicsView);
    ~TaskDlgBrokenView() override;

    void open() override;
    bool accept() override;
    bool reject() override;

    QDialogButtonBox::StandardButtons getStandardButtons() const override
    {
        return QDialogButtonBox::Close;
    }
    bool isAllowedAlterSelection() const override { return true; }
    bool isAllowedAlterDocument() const override { return true; }

private:
    TechDraw::DrawPage* m_page{nullptr};
    QGVPage* m_graphicsView{nullptr};
    TaskBrokenView* m_widget{nullptr};
    Gui::TaskView::TaskBox* m_taskBox{nullptr};
};

} // namespace TechDrawGui
