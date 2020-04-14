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

#ifndef TECHDRAWGUI_TASKCOSVERTEX_H
#define TECHDRAWGUI_TASKCOSVERTEX_H

#include <App/DocumentObject.h>
#include <Base/Vector3D.h>
#include <Gui/TaskView/TaskView.h>
#include <Gui/TaskView/TaskDialog.h>

#include <Mod/TechDraw/Gui/ui_TaskCosVertex.h>

#include "QGTracker.h"

//TODO: make this a proper enum
#define TRACKERPICK 0
#define TRACKEREDIT 1
#define TRACKERCANCEL 2
#define TRACKERCANCELEDIT 3

class Ui_TaskCosVertex;

namespace App {
class DocumentObject;
}

namespace TechDraw
{
class DrawPage;
class DrawView;
class DrawCosVertex;
}

namespace TechDrawGui
{
class QGVPage;
class QGIView;
class QGIPrimPath;
class MDIViewPage;
class QGTracker;
class QGEPath;
class QGMText;
class QGICosVertex;
class ViewProviderLeader;

class TaskCosVertex : public QWidget
{
    Q_OBJECT

public:
    TaskCosVertex(TechDraw::DrawViewPart* baseFeat,
                  TechDraw::DrawPage* page);
    ~TaskCosVertex();

public Q_SLOTS:
    void onTrackerClicked(bool b);
    void onTrackerFinished(std::vector<QPointF> pts, QGIView* qgParent);

public:
    virtual bool accept();
    virtual bool reject();
    void updateTask();
    void saveButtons(QPushButton* btnOK,
                     QPushButton* btnCancel);
    void enableTaskButtons(bool b);

protected:
    void changeEvent(QEvent *e);
    void startTracker(void);
    void removeTracker(void);
    void abandonEditSession(void);

    void addCosVertex(QPointF qPos);

    void blockButtons(bool b);
    void setUiPrimary(void);
    void updateUi(void);
    void setEditCursor(QCursor c);

   QGIView* findParentQGIV();

private:
    Ui_TaskCosVertex * ui;
    bool blockUpdate;

    QGTracker* m_tracker;
    
    MDIViewPage* m_mdi;
    QGraphicsScene* m_scene;
    QGVPage* m_view;
    TechDraw::DrawViewPart* m_baseFeat;
    TechDraw::DrawPage* m_basePage;
    QGIView* m_qgParent;
    std::string m_qgParentName;

    QGTracker::TrackerMode m_trackerMode;
    Qt::ContextMenuPolicy  m_saveContextPolicy;
    bool m_inProgressLock;

    QPushButton* m_btnOK;
    QPushButton* m_btnCancel;
    
    int m_pbTrackerState;
    QPointF m_savePoint;
    bool pointFromTracker;
};

class TaskDlgCosVertex : public Gui::TaskView::TaskDialog
{
    Q_OBJECT

public:
    TaskDlgCosVertex(TechDraw::DrawViewPart* baseFeat,
                      TechDraw::DrawPage* page);
    ~TaskDlgCosVertex();

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
    TaskCosVertex * widget;
    Gui::TaskView::TaskBox* taskbox;
};

} //namespace TechDrawGui

#endif // #ifndef TECHDRAWGUI_TASKCOSVERTEX_H
