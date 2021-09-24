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
    TaskSectionView(TechDraw::DrawViewPart* base);
    TaskSectionView(TechDraw::DrawViewSection* section);
    ~TaskSectionView();

public:
    virtual bool accept();
    virtual bool reject();

protected Q_SLOTS:
    void onUpClicked();
    void onDownClicked();
    void onLeftClicked();
    void onRightClicked();
    void onIdentifierChanged();
    void onScaleChanged();
    void onXChanged();
    void onYChanged();
    void onZChanged();

protected:
    void changeEvent(QEvent *e);
    void saveSectionState();
    void restoreSectionState();

    bool apply(void);
    void applyQuick(std::string dir);
    void applyAligned(void);

    void createSectionView(void);
    void updateSectionView(void);

    void setUiPrimary();
    void setUiEdit();

    void checkAll(bool b);
    void enableAll(bool b);

    void failNoObject(std::string objName);
    bool isBaseValid(void);
    bool isSectionValid(void);

private:
    std::unique_ptr<Ui_TaskSectionView> ui;
    TechDraw::DrawViewPart* m_base;
    TechDraw::DrawViewSection* m_section;
    std::string m_symbol;
    Base::Vector3d m_normal;
    Base::Vector3d m_direction;
    Base::Vector3d m_origin;
    
    std::string m_saveSymbol;
    std::string m_saveDirName;
    Base::Vector3d m_saveNormal;
    Base::Vector3d m_saveDirection;
    Base::Vector3d m_saveOrigin;
    double m_saveScale;

    std::string m_dirName;
    std::string m_sectionName;
    std::string m_baseName;
    App::Document* m_doc;

    bool m_createMode;
    bool m_saved;

    std::string m_saveBaseName;
    std::string m_savePageName;

    bool m_abort;

};

class TaskDlgSectionView : public Gui::TaskView::TaskDialog
{
    Q_OBJECT

public:
    TaskDlgSectionView(TechDraw::DrawViewPart* base);
    TaskDlgSectionView(TechDraw::DrawViewSection* section);
    ~TaskDlgSectionView();

public:
    /// is called the TaskView when the dialog is opened
    virtual void open();
    /// is called by the framework if an button is clicked which has no accept or reject role
/*    virtual void clicked(int);*/
    /// is called by the framework if the dialog is accepted (Ok)
    virtual bool accept();
    /// is called by the framework if the dialog is rejected (Cancel)
    virtual bool reject();
    /// is called by the framework if the user presses the help button
    virtual void helpRequested() { return;}

    virtual QDialogButtonBox::StandardButtons getStandardButtons() const
    { return QDialogButtonBox::Ok | QDialogButtonBox::Cancel; }
/*    virtual void modifyStandardButtons(QDialogButtonBox* box);*/

    void update();

    virtual bool isAllowedAlterSelection(void) const
    { return false; }
    virtual bool isAllowedAlterDocument(void) const
    { return false; }

protected:

private:
    TaskSectionView * widget;
    Gui::TaskView::TaskBox* taskbox;
};

} //namespace TechDrawGui

#endif // #ifndef GUI_TASKVIEW_TASKSECTIONVIEW_H
