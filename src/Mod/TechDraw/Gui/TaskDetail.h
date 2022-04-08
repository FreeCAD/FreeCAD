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


//TODO: make this a proper enum
#define TRACKERPICK 0
#define TRACKEREDIT 1
#define TRACKERCANCEL 2
#define TRACKERCANCELEDIT 3

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
class QGVPage;
class QGIView;
class QGIPrimPath;
class MDIViewPage;
class QGEPath;
class QGIDetail;
class QGIGhostHighlight;
class ViewProviderLeader;
class Ui_TaskDetail;

class TaskDetail : public QWidget
{
    Q_OBJECT

public:
    TaskDetail(TechDraw::DrawViewPart* baseFeat);
    TaskDetail(TechDraw::DrawViewDetail* detailFeat);
    ~TaskDetail();

public Q_SLOTS:
    void onDraggerClicked(bool b);
    void onHighlightMoved(QPointF newPos);
    void onXEdit();
    void onYEdit();
    void onRadiusEdit();
    void onScaleTypeEdit();
    void onScaleEdit();
    void onReferenceEdit();

public:
    virtual bool accept();
    virtual bool reject();
    void updateTask();
    void saveButtons(QPushButton* btnOK,
                     QPushButton* btnCancel);
    void enableTaskButtons(bool b);
    
protected:
    void changeEvent(QEvent *e);
    void startDragger(void);

    void createDetail();
    void updateDetail();
    
    void editByHighlight();

    void blockButtons(bool b);
    void setUiFromFeat(void);
    void updateUi(QPointF p);
    void enableInputFields(bool b);

    void saveDetailState();
    void restoreDetailState();
    QPointF getAnchorScene();

    TechDraw::DrawViewPart* getBaseFeat();
    TechDraw::DrawViewDetail* getDetailFeat();

private:
    std::unique_ptr<Ui_TaskDetail> ui;
    bool blockUpdate;

    QGIGhostHighlight* m_ghost;

    MDIViewPage* m_mdi;
    QGSPage* m_scene;
    QGVPage* m_view;
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
    TaskDlgDetail(TechDraw::DrawViewPart* baseFeat);
    TaskDlgDetail(TechDraw::DrawViewDetail* detailFeat);
    ~TaskDlgDetail();

public:
    /// is called the TaskView when the dialog is opened
    virtual void open();
    /// is called by the framework if an button is clicked which has no accept or reject role
    virtual void clicked(int);
    /// is called by the framework if the dialog is accepted (Ok)
    virtual bool accept();
    /// is called by the framework if the dialog is rejected (Cancel)
    virtual bool reject();
    /// is called by the framework if the user presses the help button
    virtual void helpRequested() { return;}
    virtual bool isAllowedAlterDocument(void) const
                        { return false; }
    void update();

    void modifyStandardButtons(QDialogButtonBox* box);

protected:

private:
    TaskDetail * widget;
    Gui::TaskView::TaskBox* taskbox;
};

} //namespace TechDrawGui

#endif // #ifndef TECHDRAWGUI_TASKDETAIL_H
