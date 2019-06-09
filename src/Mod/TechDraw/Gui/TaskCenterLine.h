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

#ifndef TECHDRAWGUI_TASKCENTERLINE_H
#define TECHDRAWGUI_TASKCENTERLINE_H

#include <App/DocumentObject.h>
#include <Base/Vector3D.h>
#include <Gui/TaskView/TaskView.h>
#include <Gui/TaskView/TaskDialog.h>

/*#include <Mod/TechDraw/Gui/ui_TaskCenterLine.h>*/

/*#include "QGTracker.h"*/

//TODO: make this a proper enum
#define TRACKERPICK 0
#define TRACKEREDIT 1
#define TRACKERCANCEL 2
#define TRACKERCANCELEDIT 3
#define TRACKERFINISHED 4
#define TRACKERSAVE 5

class Ui_TaskCenterLine;

namespace App {
class DocumentObject;
}

namespace TechDraw
{
class DrawPage;
class DrawView;
class DrawViewPart;
}

namespace TechDrawGeometry
{
class Face;
}

namespace TechDrawGui
{
class QGVPage;
class QGIView;
class QGIPrimPath;
class MDIViewPage;
class ViewProviderViewPart;

class TaskCenterLine : public QWidget
{
    Q_OBJECT

public:
    TaskCenterLine(TechDraw::DrawViewPart* baseFeat,
                   TechDraw::DrawPage* page,
                   std::vector<std::string> subNames);
/*    TaskCenterLine(TechDrawGui::ViewProviderViewPart* partVP);*/
    ~TaskCenterLine();

public Q_SLOTS:

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

protected:
    void changeEvent(QEvent *e);

    void blockButtons(bool b);
    void setUiPrimary(void);
/*    void setUiEdit(void);*/
/*    void enableVPUi(bool b);*/
/*    void setEditCursor(QCursor c);*/

    void addCenterLine(void);
    void createCenterLine(void);


    QGIView* findParentQGIV();
    void updateCenterLine(void);
    void removeCenterLine(void);
    TechDraw::CosmeticEdge* calcEndPoints(std::vector<std::string> faceNames,
                                        bool vert,
                                        double ext);

   void saveState(void);
   void restoreState(void);

    double getCenterWidth();
    QColor getCenterColor();
    Qt::PenStyle getCenterStyle();
    double getExtendBy();


private:
    Ui_TaskCenterLine * ui;
    bool blockUpdate;

    MDIViewPage* m_mdi;
    QGraphicsScene* m_scene;
    QGVPage* m_view;
    ViewProviderViewPart* m_partVP;
    TechDraw::DrawViewPart* m_partFeat;
    TechDraw::DrawPage* m_basePage;
    bool m_createMode;

    Qt::ContextMenuPolicy  m_saveContextPolicy;
    bool m_inProgressLock;

    QPushButton* m_btnOK;
    QPushButton* m_btnCancel;

    std::vector<std::string> m_subNames;
    double m_extendBy;
};

class TaskDlgCenterLine : public Gui::TaskView::TaskDialog
{
    Q_OBJECT

public:
    TaskDlgCenterLine(TechDraw::DrawViewPart* baseFeat,
                      TechDraw::DrawPage* page,
                      std::vector<std::string> subNames);
/*    TaskDlgCenterLine(TechDrawGui::ViewProviderLeader* partVP);*/
    ~TaskDlgCenterLine();

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
    TaskCenterLine * widget;
    Gui::TaskView::TaskBox* taskbox;
};

} //namespace TechDrawGui

#endif // #ifndef TECHDRAWGUI_TASKCENTERLINE_H
