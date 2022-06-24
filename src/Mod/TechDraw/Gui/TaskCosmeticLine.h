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

#ifndef TECHDRAWGUI_TASKCOSMETICLINE_H
#define TECHDRAWGUI_TASKCOSMETICLINE_H

#include <Base/Vector3D.h>
#include <Gui/TaskView/TaskDialog.h>
#include <Gui/TaskView/TaskView.h>


namespace TechDraw
{
class DrawPage;
class DrawView;
class DrawViewPart;
class CosmeticEdge;
class Face;
class LineFormat;
}

namespace TechDrawGui
{
class QGSPage;
class QGVPage;
class QGIView;
class QGIPrimPath;
class MDIViewPage;
class ViewProviderViewPart;
class Ui_TaskCosmeticLine;

class TaskCosmeticLine : public QWidget
{
    Q_OBJECT

public:
    TaskCosmeticLine(TechDraw::DrawViewPart* baseFeat,
                        std::vector<Base::Vector3d> points,
                        std::vector<bool> is3d);
    TaskCosmeticLine(TechDraw::DrawViewPart* baseFeat,
                        std::string edgeName);
    ~TaskCosmeticLine();

public Q_SLOTS:

public:
    virtual bool accept();
    virtual bool reject();
    void updateTask();

protected Q_SLOTS:

protected:
    void changeEvent(QEvent *e);

    void setUiPrimary(void);
    void setUiEdit(void);

    void createCosmeticLine(void);
    void updateCosmeticLine(void);

private:
    std::unique_ptr<Ui_TaskCosmeticLine> ui;

    TechDraw::DrawViewPart* m_partFeat;

    std::string m_edgeName;
    //int m_edgeIndex;
    TechDraw::CosmeticEdge* m_ce;
    TechDraw::CosmeticEdge* m_saveCE;
    std::vector<Base::Vector3d> m_points;
    std::vector<bool> m_is3d;
    bool m_createMode;
    std::string m_tag;
};

class TaskDlgCosmeticLine : public Gui::TaskView::TaskDialog
{
    Q_OBJECT

public:
    TaskDlgCosmeticLine(TechDraw::DrawViewPart* baseFeat,
                        std::vector<Base::Vector3d> points,
                        std::vector<bool> is3d);
    TaskDlgCosmeticLine(TechDraw::DrawViewPart* baseFeat,
                        std::string edgeName);
    ~TaskDlgCosmeticLine();

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

protected:

private:
    TaskCosmeticLine* widget;

    Gui::TaskView::TaskBox* taskbox;
};

} //namespace TechDrawGui

#endif // #ifndef TECHDRAWGUI_TASKCOSMETICLINE_H
