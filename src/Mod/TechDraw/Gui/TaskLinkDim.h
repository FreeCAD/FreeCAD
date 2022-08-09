/***************************************************************************
 *   Copyright (c) 2016 WandererFan <wandererfan@gmail.com>                *
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

#ifndef GUI_TASKVIEW_TASKLINKDIM_H
#define GUI_TASKVIEW_TASKLINKDIM_H

#include <Gui/TaskView/TaskDialog.h>
#include <Gui/TaskView/TaskView.h>

class QTreeWidgetItem;

namespace Gui {
class Document;
}

namespace TechDraw {
class DrawViewDimension;
}

namespace TechDrawGui
{

class Ui_TaskLinkDim;
class TaskLinkDim : public QWidget
{
    Q_OBJECT

public:
    TaskLinkDim(std::vector<App::DocumentObject*> parts,std::vector<std::string>& subs, TechDraw::DrawPage* page);
    ~TaskLinkDim() override;

public:
    bool accept();
    bool reject();

protected Q_SLOTS:
    void onCurrentItemChanged(QTreeWidgetItem*, QTreeWidgetItem*);

protected:
    void changeEvent(QEvent *e) override;
    void loadAvailDims();
    void updateDims();
    void loadToTree(const TechDraw::DrawViewDimension* dim, const bool selected, Gui::Document* guiDoc);
    bool dimReferencesSelection(const TechDraw::DrawViewDimension* dim) const;

private:
    std::unique_ptr<Ui_TaskLinkDim> ui;
    const std::vector<App::DocumentObject*> m_parts;
    const std::vector<std::string> m_subs;
    TechDraw::DrawPage* m_page;
};

class TaskDlgLinkDim : public Gui::TaskView::TaskDialog
{
    Q_OBJECT

public:
    TaskDlgLinkDim(std::vector<App::DocumentObject*> parts,std::vector<std::string>& subs, TechDraw::DrawPage* page);
    ~TaskDlgLinkDim() override;

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
    TaskLinkDim * widget;
    Gui::TaskView::TaskBox* taskbox;
};

} //namespace TechDrawGui

#endif // #ifndef GUI_TASKVIEW_TASKLINKDIM_H
