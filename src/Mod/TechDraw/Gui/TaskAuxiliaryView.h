/***************************************************************************
 *   Copyright (c) 2026 FreeCAD contributors                               *
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

#include <string>

#include <Gui/TaskView/TaskDialog.h>
#include <Gui/TaskView/TaskView.h>
#include <Mod/TechDraw/TechDrawGlobal.h>

class QCheckBox;
class QComboBox;
class QLineEdit;

namespace TechDraw
{
class DrawAuxiliaryView;
class DrawViewPart;
}

namespace TechDrawGui
{

class TaskAuxiliaryView: public QWidget
{
    Q_OBJECT

public:
    explicit TaskAuxiliaryView(TechDraw::DrawAuxiliaryView* auxiliaryView);
    ~TaskAuxiliaryView() override = default;

    bool accept();
    bool reject();
    std::string getAuxiliaryName() const;

protected:
    void changeEvent(QEvent* event) override;

private:
    void initUi();
    TechDraw::DrawViewPart* getBaseView() const;

    TechDraw::DrawAuxiliaryView* m_auxiliaryView;
    QLineEdit* m_baseViewEdit;
    QLineEdit* m_referenceLabelEdit;
    QComboBox* m_projectionModeCombo;
    QCheckBox* m_reverseDirectionCheck;
    QCheckBox* m_keepAlignedCheck;
};

class TaskDlgAuxiliaryView: public Gui::TaskView::TaskDialog
{
    Q_OBJECT

public:
    explicit TaskDlgAuxiliaryView(TechDraw::DrawAuxiliaryView* auxiliaryView);
    ~TaskDlgAuxiliaryView() override = default;

    void open() override;
    void clicked(int) override;
    bool accept() override;
    bool reject() override;
    void helpRequested() override {}
    bool isAllowedAlterDocument() const override
    {
        return false;
    }
    std::string getAuxiliaryName() const;

private:
    TaskAuxiliaryView* widget;
    Gui::TaskView::TaskBox* taskbox;
};

}  // namespace TechDrawGui
