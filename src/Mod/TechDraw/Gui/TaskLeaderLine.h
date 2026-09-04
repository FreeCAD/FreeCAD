/***************************************************************************
 *   Copyright (c) 2019 WandererFan <wandererfan@gmail.com>                *
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

#include "QGVPage.h"
#include "TechDrawLeaderLineHandler.h"
#include <Base/Vector3D.h>
#include <Gui/TaskView/TaskDialog.h>
#include <Gui/TaskView/TaskView.h>
#include <Mod/TechDraw/TechDrawGlobal.h>


namespace TechDraw
{
class DrawPage;
class DrawView;
class DrawLeaderLine;
}

namespace TechDrawGui
{
class QGVPage;
class ViewProviderPage;
class ViewProviderLeader;
class Ui_TaskLeaderLine;

class TaskLeaderLine : public QWidget
{
    Q_OBJECT

public:
    //ctor for creation
    explicit TaskLeaderLine(TechDraw::DrawPage* page);
    //ctor for edit
    explicit TaskLeaderLine(TechDrawGui::ViewProviderLeader* leadVP);
    ~TaskLeaderLine() override = default;

    virtual bool accept();
    virtual bool reject();
    virtual void setCreateMode(bool mode) { m_createMode = mode; }
    virtual bool getCreateMode() { return m_createMode; }
    void updateTask();
    void saveButtons(QPushButton* btnOK,
                     QPushButton* btnCancel);
    void enableTaskButtons(bool enable);
    void recomputeFeature();

protected:
    void changeEvent(QEvent *event) override;

    void updateLeaderFeature();
    void commonFeatureUpdate();

    void setUiPrimary();
    void setUiEdit();
    void enableVPUi(bool enable);

    void saveState();
    void restoreState();

private:
    std::unique_ptr<Ui_TaskLeaderLine> ui;
    ViewProviderLeader* m_lineVP;
    TechDraw::DrawView* m_baseFeat;
    TechDraw::DrawPage* m_basePage;
    TechDraw::DrawLeaderLine* m_lineFeat;
    bool m_createMode;
    Qt::ContextMenuPolicy  m_saveContextPolicy;

    QPushButton* m_btnOK;
    QPushButton* m_btnCancel;

    double m_saveX;
    double m_saveY;

    TechDrawLeaderLineHandler* handler = nullptr;

    ViewProviderPage* m_vpp;
    QGVPage* m_viewPage;

    std::vector<Base::Vector3d> m_savePoints;

private Q_SLOTS:
    void onStartSymbolChanged();
    void onEndSymbolChanged();
    void onColorChanged();
    void onLineWidthChanged();
    void onLineStyleChanged();
    void onLeaderTypeChanged(int index);
    void onKinkLengthChanged();
};

class TaskDlgLeaderLine : public Gui::TaskView::TaskDialog
{
    Q_OBJECT

public:
    explicit TaskDlgLeaderLine(TechDraw::DrawPage* page);
    explicit TaskDlgLeaderLine(TechDrawGui::ViewProviderLeader* leadVP);
    ~TaskDlgLeaderLine() override;

    /// is called the TaskView when the dialog is opened
    void open() override;
    /// is called by the framework if an button is clicked which has no accept or reject role
    void clicked(int) override;
    /// is called by the framework if the dialog is accepted (Ok)
    bool accept() override;
    /// is called by the framework if the dialog is rejected (Cancel)
    bool reject() override;
    /// is called by the framework if the user presses the help button
    bool isAllowedAlterDocument() const override
                        { return false; }
    void update();

    void modifyStandardButtons(QDialogButtonBox* box) override;

private:
    TaskLeaderLine * widget;
    Gui::TaskView::TaskBox* taskbox;
};

} //namespace TechDrawGui