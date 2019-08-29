/***************************************************************************
 *   Copyright (c) 2018 WandererFan <wandererfan@gmail.com>                *
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

#ifndef GUI_TASKVIEW_TASKLINEDECOR_H
#define GUI_TASKVIEW_TASKLINEDECOR_H

#include <App/Material.h>
#include <Gui/TaskView/TaskView.h>
#include <Gui/TaskView/TaskDialog.h>
#include <Gui/FileDialog.h>

#include <Mod/TechDraw/Gui/ui_TaskLineDecor.h>
#include <Mod/TechDraw/Gui/ui_TaskRestoreLines.h>   //????

class Ui_TaskLineDecor;

namespace App
{
class DocumentObject;
}


namespace TechDrawGui
{

class TaskLineDecor : public QWidget
{
    Q_OBJECT

public:
    TaskLineDecor(TechDraw::DrawViewPart* partFeat,
                  std::vector<std::string> edgeNames);
    ~TaskLineDecor();

public:
    virtual bool accept();
    virtual bool reject();
    bool apply(void) { return m_apply; }
    void apply(bool b) { m_apply = b; }

protected Q_SLOTS:
    void onStyleChanged(void);
    void onColorChanged(void);
    void onWeightChanged(void);
    void onVisibleChanged(void);

protected:
    void changeEvent(QEvent *e);
    void initUi(void);
    void applyDecorations(void);
    void getDefaults(void);

private:
    Ui_TaskLineDecor* ui;
    TechDraw::DrawViewPart* m_partFeat;
    std::vector<std::string> m_edges;
    int m_style;
    App::Color m_color;
    double m_weight;
    bool m_visible;
    bool m_apply;
};

class TaskRestoreLines : public QWidget
{
    Q_OBJECT

public:
    TaskRestoreLines(TechDraw::DrawViewPart* partFeat,
                     TechDrawGui::TaskLineDecor* parent);
    ~TaskRestoreLines();

public:
    virtual bool accept();
    virtual bool reject();

protected Q_SLOTS:
    void onAllPressed(void);
    void onGeometryPressed(void);
    void onCosmeticPressed(void);
    void onCenterPressed(void);

protected:
    void changeEvent(QEvent *e);
    void initUi(void);
    int countInvisibleLines(void);
    int countInvisibleGeoms(void);
    int countInvisibleCosmetics(void);
    int countInvisibleCenters(void);
    void restoreInvisibleLines(void);
    void restoreInvisibleGeoms(void);
    void restoreInvisibleCosmetics(void);
    void restoreInvisibleCenters(void);

private:
    Ui_TaskRestoreLines* ui;
    TechDraw::DrawViewPart* m_partFeat;
    TaskLineDecor* m_parent;
};


class TaskDlgLineDecor : public Gui::TaskView::TaskDialog
{
    Q_OBJECT

public:
    TaskDlgLineDecor(TechDraw::DrawViewPart* partFeat,
                     std::vector<std::string> edgeNames);
    ~TaskDlgLineDecor();

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

protected:

private:
    TaskLineDecor * widget;
    TaskRestoreLines* restore;
    Gui::TaskView::TaskBox* taskbox;
    Gui::TaskView::TaskBox* restoreBox;
};

} //namespace TechDrawGui

#endif // #ifndef GUI_TASKVIEW_TASKLINEDECOR_H
