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

#include "QGTracker.h"

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
class QGIView;
class QGIPrimPath;
class QGTracker;
class QGEPath;
class QGMText;
class QGILeaderLine;
class ViewProviderPage;
class ViewProviderLeader;
class Ui_TaskLeaderLine;

class TaskLeaderLine : public QWidget
{
    Q_OBJECT

public:
    TaskLeaderLine(TechDraw::DrawView* baseFeat,
                   TechDraw::DrawPage* page);
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

public Q_SLOTS:
    void onTrackerClicked(bool clicked);
    void onCancelEditClicked(bool clicked);
    void onTrackerFinished(std::vector<QPointF> pts, TechDrawGui::QGIView* qgParent);

protected:
    void trackerPointsFromQPoints(std::vector<QPointF> pts);
    void changeEvent(QEvent *event) override;
    void startTracker();
    void removeTracker();
    void abandonEditSession();

    void createLeaderFeature(std::vector<Base::Vector3d> converted);
    void updateLeaderFeature();
    void commonFeatureUpdate();
    void removeFeature();

    void setUiPrimary();
    void setUiEdit();
    void enableVPUi(bool enable);
    void setEditCursor(QCursor cursor);

    QGIView* findParentQGIV();
    int getPrefArrowStyle();
    double prefWeight() const;
    App::Color prefLineColor();

   void saveState();
   void restoreState();

protected Q_SLOTS:
    void onPointEditComplete();

private:
    std::unique_ptr<Ui_TaskLeaderLine> ui;

    QGTracker* m_tracker;

    ViewProviderPage* m_vpp;
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

private Q_SLOTS:
    void onStartSymbolChanged();
    void onEndSymbolChanged();
    void onColorChanged();
    void onLineWidthChanged();
    void onLineStyleChanged();
};

class TaskDlgLeaderLine : public Gui::TaskView::TaskDialog
{
    Q_OBJECT

public:
    TaskDlgLeaderLine(TechDraw::DrawView* baseFeat,
                      TechDraw::DrawPage* page);
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

#endif // #ifndef TECHDRAWGUI_TASKTEXTLEADER_H
