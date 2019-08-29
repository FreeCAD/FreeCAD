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

#ifndef TECHDRAWGUI_TASKTEXTLEADER_H
#define TECHDRAWGUI_TASKTEXTLEADER_H

#include <App/DocumentObject.h>
#include <Base/Vector3D.h>
#include <Gui/TaskView/TaskView.h>
#include <Gui/TaskView/TaskDialog.h>

#include <Mod/TechDraw/Gui/ui_TaskLeaderLine.h>

#include "QGTracker.h"

//TODO: make this a proper enum
#define TRACKERPICK 0
#define TRACKEREDIT 1
#define TRACKERCANCEL 2
#define TRACKERCANCELEDIT 3
#define TRACKERFINISHED 4
#define TRACKERSAVE 5

class Ui_TaskLeaderLine;

namespace App {
class DocumentObject;
}

namespace TechDraw
{
class DrawPage;
class DrawView;
class DrawLeaderLine;
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
class QGILeaderLine;
class ViewProviderLeader;

class TaskLeaderLine : public QWidget
{
    Q_OBJECT

public:
    TaskLeaderLine(TechDraw::DrawView* baseFeat,
                   TechDraw::DrawPage* page);
    TaskLeaderLine(TechDrawGui::ViewProviderLeader* leadVP);
    ~TaskLeaderLine();

public Q_SLOTS:
    void onTrackerClicked(bool b);
    void onCancelEditClicked(bool b);
    void onTrackerFinished(std::vector<QPointF> pts, QGIView* qgParent);

public:
    virtual bool accept();
    virtual bool reject();
    virtual void setCreateMode(bool b) { m_createMode = b; }
    virtual bool getCreateMode(void) { return m_createMode; }
    void updateTask();
    void saveButtons(QPushButton* btnOK,
                     QPushButton* btnCancel);
    void enableTaskButtons(bool b);


protected Q_SLOTS:
    void onPointEditComplete(void);

protected:
    void trackerPointsFromQPoints(std::vector<QPointF> pts);
    void changeEvent(QEvent *e);
    void startTracker(void);
    void removeTracker(void);
    void abandonEditSession(void);

    void createLeaderFeature(std::vector<Base::Vector3d> converted);
    void updateLeaderFeature();
    void commonFeatureUpdate(void);
    void removeFeature(void);

    void blockButtons(bool b);
    void setUiPrimary(void);
    void setUiEdit(void);
    void enableTextUi(bool b);
    void enableVPUi(bool b);
    void setEditCursor(QCursor c);

   QGIView* findParentQGIV();
   int getPrefArrowStyle();

   void saveState(void);
   void restoreState(void);

private:
    Ui_TaskLeaderLine * ui;
    bool blockUpdate;

    QGTracker* m_tracker;
    
    MDIViewPage* m_mdi;
    QGraphicsScene* m_scene;
    QGVPage* m_view;
    ViewProviderLeader* m_lineVP;
    TechDraw::DrawView* m_baseFeat;
    TechDraw::DrawPage* m_basePage;
    TechDraw::DrawLeaderLine* m_lineFeat;
    std::string m_leaderName;
    std::string m_leaderType;
    QGIView* m_qgParent;
    std::string m_qgParentName;

    std::vector<Base::Vector3d> m_trackerPoints;
    Base::Vector3d m_attachPoint;
    
    bool m_createMode;
    QGEPath* m_leadLine;

    QGTracker::TrackerMode m_trackerMode;
    Qt::ContextMenuPolicy  m_saveContextPolicy;
    bool m_inProgressLock;

    QGILeaderLine* m_qgLine;
    QPushButton* m_btnOK;
    QPushButton* m_btnCancel;
    
    int m_pbTrackerState;
    
    std::vector<Base::Vector3d> m_savePoints;
    double m_saveX;
    double m_saveY;
};

class TaskDlgLeaderLine : public Gui::TaskView::TaskDialog
{
    Q_OBJECT

public:
    TaskDlgLeaderLine(TechDraw::DrawView* baseFeat,
                      TechDraw::DrawPage* page);
    TaskDlgLeaderLine(TechDrawGui::ViewProviderLeader* leadVP);
    ~TaskDlgLeaderLine();

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
    TaskLeaderLine * widget;
    Gui::TaskView::TaskBox* taskbox;
};

} //namespace TechDrawGui

#endif // #ifndef TECHDRAWGUI_TASKTEXTLEADER_H
