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
    TaskHatch(TechDraw::DrawViewPart* inDvp, std::vector<std::string> subs);
    TaskHatch(TechDrawGui::ViewProviderHatch* inVp);
    ~TaskHatch() override;

public:
    virtual bool accept();
    virtual bool reject();

protected Q_SLOTS:
    void onFileChanged();
    void onScaleChanged();
    void onColorChanged();

protected:
    void changeEvent(QEvent *e) override;
    void apply(bool forceUpdate = false);

    void createHatch();
    void updateHatch();

    void setUiPrimary();
    void setUiEdit();

    void saveHatchState();
    void restoreHatchState();
    void getParameters();

private:
    std::unique_ptr<Ui_TaskHatch> ui;
    TechDraw::DrawHatch* m_hatch;
    TechDrawGui::ViewProviderHatch* m_vp;
    TechDraw::DrawViewPart* m_dvp;
    std::vector<std::string> m_subs;
    std::string m_file;
    double m_scale;
    App::Color m_color;

    std::string m_saveFile;
    double m_saveScale;
    App::Color m_saveColor;
    std::vector<std::string> m_saveSubs;

};

class TaskDlgHatch : public Gui::TaskView::TaskDialog
{
    Q_OBJECT

public:
    TaskDlgHatch(TechDraw::DrawViewPart* inDvp, std::vector<std::string> subs);
    TaskDlgHatch(TechDrawGui::ViewProviderHatch* inVp);
    ~TaskDlgHatch() override;

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
    TaskHatch * widget;
    Gui::TaskView::TaskBox* taskbox;
};

} //namespace TechDrawGui

#endif // #ifndef GUI_TASKVIEW_TASKHATCH_H
