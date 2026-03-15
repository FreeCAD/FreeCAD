/***************************************************************************
 *   Copyright (c) 2022 WandererFan <wandererfan@gmail.com>                *
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

#include <QListWidget>
#include <QTableWidget>

#include <Gui/TaskView/TaskDialog.h>
#include <Gui/TaskView/TaskView.h>
#include <Mod/TechDraw/App/DrawViewDimension.h>
#include <Mod/TechDraw/TechDrawGlobal.h>


class Ui_TaskDimRepair;

namespace App
{
class DocumentObject;
}

namespace TechDrawGui
{

class TaskDimRepair: public QWidget
{
    Q_OBJECT

public:
    TaskDimRepair(TechDraw::DrawViewDimension* inDvd);
    ~TaskDimRepair() override;

public:
    virtual bool accept();
    virtual bool reject();

protected Q_SLOTS:
    void slotUseSelection();

protected:
    void changeEvent(QEvent* e) override;

    void setUiPrimary();
    void replaceReferences();
    void updateUi();
    void fillList(QListWidget* lwItems, std::vector<std::string> labels,
                  std::vector<std::string> names);
    void loadTableWidget(QTableWidget* tw, TechDraw::ReferenceVector refs);
    void saveDimState();
    void restoreDimState();

private:
    std::unique_ptr<Ui_TaskDimRepair> ui;
    TechDraw::DrawViewDimension* m_dim;

    long int m_saveMeasureType;
    long int m_saveDimType;
    TechDraw::DrawViewPart* m_saveDvp;
    TechDraw::ReferenceVector m_saveRefs2d;
    TechDraw::ReferenceVector m_saveRefs3d;
    TechDraw::ReferenceVector m_toApply2d;
    TechDraw::ReferenceVector m_toApply3d;
};

class TaskDlgDimReference: public Gui::TaskView::TaskDialog
{
    Q_OBJECT

public:
    explicit TaskDlgDimReference(TechDraw::DrawViewDimension* inDvd);
    ~TaskDlgDimReference() override;

public:
    /// is called the TaskView when the dialog is opened
    void open() override;
    /// is called by the framework if an button is clicked which has no accept or reject role
    void clicked(int) override;
    /// is called by the framework if the dialog is accepted (Ok)
    bool accept() override;
    /// is called by the framework if the dialog is rejected (Cancel)
    bool reject() override;
    /// is called by the framework if the user presses the help button
    void helpRequested() override
    {
        return;
    }
    bool isAllowedAlterDocument() const override
    {
        return false;
    }

    void update();

protected:
private:
    TaskDimRepair* widget;
    Gui::TaskView::TaskBox* taskbox;
};

}//namespace TechDrawGui