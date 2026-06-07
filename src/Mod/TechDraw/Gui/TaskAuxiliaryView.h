// SPDX-License-Identifier: LGPL-2.0-or-later

/***************************************************************************
 *   Copyright (c) 2026 meaqua9420                                        *
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
#include <string>

#include <Base/Vector3D.h>
#include <Gui/TaskView/TaskDialog.h>
#include <Gui/TaskView/TaskView.h>
#include <Mod/TechDraw/TechDrawGlobal.h>

class QPushButton;
class QDialogButtonBox;

namespace App
{
class Document;
}

namespace TechDraw
{
class DrawAuxiliaryView;
class DrawPage;
class DrawViewPart;
}

namespace TechDrawGui
{

class Ui_TaskAuxiliaryView;

class TaskAuxiliaryView : public QWidget
{
    Q_OBJECT

public:
    TaskAuxiliaryView(TechDraw::DrawViewPart* baseFeat,
                      Base::Vector3d referenceStart,
                      Base::Vector3d referenceEnd);
    explicit TaskAuxiliaryView(TechDraw::DrawAuxiliaryView* auxiliaryFeat);
    ~TaskAuxiliaryView() override;

    bool accept();
    bool reject();

    TechDraw::DrawAuxiliaryView* getAuxiliaryFeat();

protected:
    void changeEvent(QEvent* event) override;

private:
    void setUiFromFeat();
    void createAuxiliaryView();
    void updateAuxiliaryView();
    void saveAuxiliaryState();
    void restoreAuxiliaryState();
    Base::Vector3d referenceDirection() const;

    std::unique_ptr<Ui_TaskAuxiliaryView> ui;
    TechDraw::DrawAuxiliaryView* m_auxiliaryFeat;
    TechDraw::DrawViewPart* m_baseFeat;
    TechDraw::DrawPage* m_basePage;
    App::Document* m_doc;
    std::string m_auxiliaryName;
    std::string m_baseName;
    std::string m_pageName;
    Base::Vector3d m_referenceStart;
    Base::Vector3d m_referenceEnd;
    Base::Vector3d m_saveReferenceStart;
    Base::Vector3d m_saveReferenceEnd;
    Base::Vector3d m_saveDirection;
    std::string m_saveLabel;
    std::string m_saveOrientation;
    bool m_saveReverse;
    bool m_editMode;
};

class TaskDlgAuxiliaryView : public Gui::TaskView::TaskDialog
{
    Q_OBJECT

public:
    TaskDlgAuxiliaryView(TechDraw::DrawViewPart* baseFeat,
                         Base::Vector3d referenceStart,
                         Base::Vector3d referenceEnd);
    explicit TaskDlgAuxiliaryView(TechDraw::DrawAuxiliaryView* auxiliaryFeat);
    ~TaskDlgAuxiliaryView() override;

    void open() override;
    bool accept() override;
    bool reject() override;
    bool isAllowedAlterDocument() const override
    {
        return false;
    }
    void modifyStandardButtons(QDialogButtonBox* box) override;

    std::string getAuxiliaryName() const;

private:
    TaskAuxiliaryView* widget;
    Gui::TaskView::TaskBox* taskbox;
};

}  // namespace TechDrawGui
