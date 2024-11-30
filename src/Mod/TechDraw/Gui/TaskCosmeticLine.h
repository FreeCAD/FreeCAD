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
#include <Mod/TechDraw/TechDrawGlobal.h>


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
    TaskCosmeticLine(TechDraw::DrawViewPart* partFeat,
                        std::vector<Base::Vector3d> points,
                        std::vector<bool> is3d);
    TaskCosmeticLine(TechDraw::DrawViewPart* partFeat,
                        std::string edgeName);
    ~TaskCosmeticLine() override;

    virtual bool accept();
    virtual bool reject();
    void updateTask();

protected:
    void changeEvent(QEvent *e) override;

    void setUiPrimary();
    void setUiEdit();

    void createCosmeticLine();
    void updateCosmeticLine();

private:
    std::unique_ptr<Ui_TaskCosmeticLine> ui;

    TechDraw::DrawViewPart* m_partFeat;

    std::string m_edgeName;
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
    TaskDlgCosmeticLine(TechDraw::DrawViewPart* partFeat,
                        std::vector<Base::Vector3d> points,
                        std::vector<bool> is3d);
    TaskDlgCosmeticLine(TechDraw::DrawViewPart* partFeat,
                        std::string edgeName);
    ~TaskDlgCosmeticLine() override;

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
    void helpRequested() override { return;}
    bool isAllowedAlterDocument() const override
                        { return false; }
    void update();

protected:

private:
    TaskCosmeticLine* widget;

    Gui::TaskView::TaskBox* taskbox;
};

} //namespace TechDrawGui

#endif // #ifndef TECHDRAWGUI_TASKCOSMETICLINE_H
