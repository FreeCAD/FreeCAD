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

#include "QGTracker.h"

#include <Gui/TaskView/TaskDialog.h>
#include <Gui/TaskView/TaskView.h>
#include <Mod/TechDraw/TechDrawGlobal.h>


namespace TechDraw
{
class DrawPage;
class DrawView;
class DrawViewPart;
class DrawCosVertex;
}

namespace TechDrawGui
{
class QGSPage;
class QGIView;
class QGIPrimPath;
class QGTracker;
class QGEPath;
class QGMText;
class QGICosVertex;
class ViewProviderPage;
class ViewProviderLeader;
class Ui_TaskCosVertex;

class TaskCosVertex : public QWidget
{
    Q_OBJECT

public:
    TaskCosVertex(TechDraw::DrawViewPart* baseFeat,
                  TechDraw::DrawPage* page);
    ~TaskCosVertex() override = default;

    virtual bool accept();
    virtual bool reject();
    void updateTask();
    void saveButtons(QPushButton* btnOK,
                     QPushButton* btnCancel);
    void enableTaskButtons(bool button);

public Q_SLOTS:
    void onTrackerClicked(bool clicked);
    void onTrackerFinished(std::vector<QPointF> pts, TechDrawGui::QGIView* qgParent);

protected:
    void changeEvent(QEvent *event) override;
    void startTracker();
    void removeTracker();
    void abandonEditSession();

    void addCosVertex(QPointF qPos);

    void setUiPrimary();
    void updateUi();
    void setEditCursor(QCursor cursor);

   QGIView* findParentQGIV();

private:
    std::unique_ptr<Ui_TaskCosVertex> ui;
    bool blockUpdate;

    QGTracker* m_tracker;

    TechDraw::DrawViewPart* m_baseFeat;
    TechDraw::DrawPage* m_basePage;
    QGIView* m_qgParent;
    std::string m_qgParentName;

    QGTracker::TrackerMode m_trackerMode;
    Qt::ContextMenuPolicy  m_saveContextPolicy;
    bool m_inProgressLock;

    QPushButton* m_btnOK;
    QPushButton* m_btnCancel;

    TrackerAction m_pbTrackerState;
    QPointF m_savePoint;
    bool pointFromTracker;

    ViewProviderPage* m_vpp;
};

class TaskDlgCosVertex : public Gui::TaskView::TaskDialog
{
    Q_OBJECT

public:
    TaskDlgCosVertex(TechDraw::DrawViewPart* baseFeat,
                      TechDraw::DrawPage* page);
    ~TaskDlgCosVertex() override;

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
    bool isAllowedAlterDocument() const override
                        { return false; }
    void update();

    void modifyStandardButtons(QDialogButtonBox* box) override;

protected:

private:
    TaskCosVertex * widget;
    Gui::TaskView::TaskBox* taskbox;
};

} //namespace TechDrawGui