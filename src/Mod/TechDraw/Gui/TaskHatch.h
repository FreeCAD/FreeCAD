/***************************************************************************
 *   Copyright (c) 2020 FreeCAD Developers                                 *
 *   Author: Uwe Stöhr <uwestoehr@lyx.org>                                 *
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

#ifndef GUI_TASKVIEW_TASKHATCH_H
#define GUI_TASKVIEW_TASKHATCH_H

#include <App/Material.h>
#include <Gui/TaskView/TaskDialog.h>
#include <Gui/TaskView/TaskView.h>

#include <Mod/TechDraw/App/DrawHatch.h>
#include <Mod/TechDraw/Gui/ui_TaskHatch.h>


class Ui_TaskHatch;

namespace App
{
class DocumentObject;
}

namespace TechDrawGui
{
class ViewProviderHatch;

class TaskHatch : public QWidget
{
    Q_OBJECT

public:
    TaskHatch(TechDraw::DrawHatch* inHatch,TechDrawGui::ViewProviderHatch* inVp, bool mode);
    ~TaskHatch();

public:
    virtual bool accept();
    virtual bool reject();
    void setCreateMode(bool b) { m_createMode = b;}
    bool getCreateMode() { return m_createMode; }

protected Q_SLOTS:
    void onFileChanged(void);

protected:
    void changeEvent(QEvent *e);
    void initUi();
//    bool resetUi();
    void updateValues();
    void getParameters();
    QStringList listToQ(std::vector<std::string> in);

private Q_SLOTS:
    void onScaleChanged();
    void onColorChanged();

private:
    std::unique_ptr<Ui_TaskHatch> ui;
    TechDraw::DrawHatch* m_hatch;
    TechDrawGui::ViewProviderHatch* m_Vp;
    App::DocumentObject* m_source;
    std::string m_file;
    double m_scale;
    App::Color m_color;
    std::string m_origFile;
    double m_origScale;
    App::Color m_origColor;

    bool m_createMode;

};

class TaskDlgHatch : public Gui::TaskView::TaskDialog
{
    Q_OBJECT

public:
    TaskDlgHatch(TechDraw::DrawHatch* inHatch,TechDrawGui::ViewProviderHatch* inVp, bool mode);
    ~TaskDlgHatch();
    const ViewProviderHatch * getViewProvider() const { return viewProvider; }

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
    void setCreateMode(bool b);

    void update();

protected:
    const ViewProviderHatch *viewProvider;

private:
    TaskHatch * widget;
    Gui::TaskView::TaskBox* taskbox;
};

} //namespace TechDrawGui

#endif // #ifndef GUI_TASKVIEW_TASKHATCH_H
