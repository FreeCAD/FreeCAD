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

#ifndef GUI_TASKVIEW_TASKSECTIONVIEW_H
#define GUI_TASKVIEW_TASKSECTIONVIEW_H

#include <Gui/TaskView/TaskView.h>
#include <Gui/TaskView/TaskDialog.h>

#include <Mod/TechDraw/Gui/ui_TaskSectionView.h>

#include <Mod/TechDraw/App/DrawViewPart.h>
#include <Mod/TechDraw/App/DrawViewSection.h>


class Ui_TaskSectionView;

namespace TechDrawGui
{

class TaskSectionView : public QWidget
{
    Q_OBJECT

public:
    TaskSectionView(TechDraw::DrawViewPart* base, TechDraw::DrawViewSection* section);
    ~TaskSectionView();

public:
    virtual bool accept();
    virtual bool reject();

protected Q_SLOTS:
    void onUpClicked(bool b);
    void onDownClicked(bool b);
    void onLeftClicked(bool b);
    void onRightClicked(bool b);
    void onResetClicked(bool b);

protected:
    void turnOnUp(void);
    void turnOnDown(void);
    void turnOnLeft(void);
    void turnOnRight(void);
    void checkAll(bool b);
    void enableAll(bool b);
    void blockButtons(bool b);
    void changeEvent(QEvent *e);
    void resetValues();
    bool calcValues();
    void saveInitialValues();
    void updateValues();
    QString formatVector(Base::Vector3d v);

private:
    Ui_TaskSectionView * ui;
    TechDraw::DrawViewPart* m_base;
    TechDraw::DrawViewSection* m_section;
    Base::Vector3d sectionNormal;
    Base::Vector3d sectionProjDir;
    Base::Vector3d sectionOrigin;
    char* sectionDir;
    Base::Vector3d arrowDir;

    std::string saveSym;
    std::string saveLabel;
  //bool saveHorizSectionLine;
  //bool saveArrowUpSection;
    Base::Vector3d saveSectionProjDir;
    Base::Vector3d saveSectionOrigin;
    Base::Vector3d saveSectionNormal;

};

class TaskDlgSectionView : public Gui::TaskView::TaskDialog
{
    Q_OBJECT

public:
    TaskDlgSectionView(TechDraw::DrawViewPart* base, TechDraw::DrawViewSection* section);
    ~TaskDlgSectionView();

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
    TaskSectionView * widget;
    Gui::TaskView::TaskBox* taskbox;
};

} //namespace TechDrawGui

#endif // #ifndef GUI_TASKVIEW_TASKSECTIONVIEW_H
