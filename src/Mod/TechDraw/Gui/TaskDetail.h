/***************************************************************************
 *   Copyright (c) 2020 WandererFan <wandererfan@gmail.com>                *
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

#ifndef TECHDRAWGUI_TASKDETAIL_H
#define TECHDRAWGUI_TASKDETAIL_H

#include <Base/Vector3D.h>
#include <Gui/TaskView/TaskDialog.h>
#include <Gui/TaskView/TaskView.h>
#include <Mod/TechDraw/TechDrawGlobal.h>


//TODO: make this a proper enum
static constexpr int TRACKERPICK(0);
static constexpr int TRACKEREDIT(1);
static constexpr int TRACKERCANCEL(2);
static constexpr int TRACKERCANCELEDIT(3);


namespace TechDraw
{
class DrawPage;
class DrawView;
class DrawViewDetail;
class DrawViewPart;
}

namespace TechDrawGui
{
class QGSPage;
class QGIView;
class QGIPrimPath;
class QGEPath;
class QGIDetail;
class QGIGhostHighlight;
class ViewProviderPage;
class Ui_TaskDetail;

class TaskDetail : public QWidget
{
    Q_OBJECT

public:
    explicit TaskDetail(TechDraw::DrawViewPart* baseFeat);
    explicit TaskDetail(TechDraw::DrawViewDetail* detailFeat);
    ~TaskDetail() override = default;

    virtual bool accept();
    virtual bool reject();
    void updateTask();
    void saveButtons(QPushButton* btnOK,
                     QPushButton* btnCancel);
    void enableTaskButtons(bool button);

public Q_SLOTS:
        void onDraggerClicked(bool clicked);
        void onHighlightMoved(QPointF dragEnd);
        void onXEdit();
        void onYEdit();
        void onRadiusEdit();
        void onScaleTypeEdit();
        void onScaleEdit();
        void onReferenceEdit();

protected:
    void changeEvent(QEvent *event) override;
    void startDragger();

    void createDetail();
    void updateDetail();

    void editByHighlight();

    void blockButtons(bool isBlocked);
    void setUiFromFeat();
    void updateUi(QPointF pos);
    void enableInputFields(bool isEnabled);

    void saveDetailState();
    void restoreDetailState();
    QPointF getAnchorScene();

    TechDraw::DrawViewPart* getBaseFeat();
    TechDraw::DrawViewDetail* getDetailFeat();

private:
    std::unique_ptr<Ui_TaskDetail> ui;
    bool blockUpdate;

    QGIGhostHighlight* m_ghost;

    ViewProviderPage* m_vpp;
    TechDraw::DrawViewDetail* m_detailFeat;
    TechDraw::DrawViewPart* m_baseFeat;
    TechDraw::DrawPage* m_basePage;
    QGIView* m_qgParent;
    std::string m_qgParentName;

    bool m_inProgressLock;

    QPushButton* m_btnOK;
    QPushButton* m_btnCancel;

    Base::Vector3d m_saveAnchor;
    double m_saveRadius;
    bool m_saved;
    QPointF m_dragStart;

    std::string    m_baseName;
    std::string    m_pageName;
    std::string    m_detailName;
    App::Document* m_doc;

    bool m_mode;
    bool m_created;
};

class TaskDlgDetail : public Gui::TaskView::TaskDialog
{
    Q_OBJECT

public:
    explicit TaskDlgDetail(TechDraw::DrawViewPart* baseFeat);
    explicit TaskDlgDetail(TechDraw::DrawViewDetail* detailFeat);
    ~TaskDlgDetail() override;

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
    TaskDetail * widget;
    Gui::TaskView::TaskBox* taskbox;
};

} //namespace TechDrawGui

#endif // #ifndef TECHDRAWGUI_TASKDETAIL_H
