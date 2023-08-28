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

#ifndef TECHDRAWGUI_TASKACTIVEVIEW_H
#define TECHDRAWGUI_TASKACTIVEVIEW_H

#include <Gui/TaskView/TaskDialog.h>
#include <Gui/TaskView/TaskView.h>
#include <Mod/TechDraw/TechDrawGlobal.h>


class QPushButton;
class Ui_TaskActiveView;

namespace TechDraw
{
class DrawPage;
class DrawView;
class DrawViewImage;
}

namespace TechDrawGui
{
class QGSPage;
class QGVPage;
class QGIView;
class MDIViewPage;

class TechDrawGuiExport TaskActiveView : public QWidget
{
    Q_OBJECT

public:
    TaskActiveView(TechDraw::DrawPage* pageFeat);
    ~TaskActiveView() override;

public Q_SLOTS:

public:
    virtual bool accept();
    virtual bool reject();
    void updateTask();
    void saveButtons(QPushButton* btnOK,
                     QPushButton* btnCancel);
    void enableTaskButtons(bool b);

protected:
    void changeEvent(QEvent *e) override;

    void blockButtons(bool b);
    void setUiPrimary(void);

    TechDraw::DrawViewImage* createActiveView();

private:
    std::unique_ptr<Ui_TaskActiveView> ui;

    TechDraw::DrawPage*       m_pageFeat;
    TechDraw::DrawViewImage*  m_imageFeat;

    QPushButton* m_btnOK;
    QPushButton* m_btnCancel;

};


class TaskDlgActiveView : public Gui::TaskView::TaskDialog
{
    Q_OBJECT

public:
    explicit TaskDlgActiveView(TechDraw::DrawPage* pageFeat);
    ~TaskDlgActiveView() override;

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

    void modifyStandardButtons(QDialogButtonBox* box) override;

protected:

private:
    TaskActiveView* widget;
    Gui::TaskView::TaskBox* taskbox;

};

} //namespace TechDrawGui

#endif // #ifndef TECHDRAWGUI_TASKACTIVEVIEW_H
