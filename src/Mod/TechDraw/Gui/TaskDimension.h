/***************************************************************************
 *   Copyright (c) 2021 Uwe Stöhr <uwestoehr@lyx.org>                      *
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

#ifndef GUI_TASKVIEW_TASKDIMENSION_H
#define GUI_TASKVIEW_TASKDIMENSION_H

#include <Gui/DocumentObserver.h>
#include <Gui/TaskView/TaskDialog.h>
#include <Gui/TaskView/TaskView.h>
#include <Mod/TechDraw/TechDrawGlobal.h>


namespace TechDrawGui
{

class QGIViewDimension;
class ViewProviderDimension;
class Ui_TaskDimension;
class TaskDimension : public QWidget
{
    Q_OBJECT

public:
    TaskDimension(QGIViewDimension *parent, ViewProviderDimension *dimensionVP);
    ~TaskDimension() override;

public:
    virtual bool accept();
    virtual bool reject();
    void recomputeFeature();

private Q_SLOTS:
    void onTheoreticallyExactChanged();
    void onEqualToleranceChanged();
    void onOvertoleranceChanged();
    void onUndertoleranceChanged();
    void onFormatSpecifierChanged();
    void onArbitraryChanged();
    void onFormatSpecifierOverToleranceChanged();
    void onFormatSpecifierUnderToleranceChanged();
    void onArbitraryTolerancesChanged();
    void onFlipArrowheadsChanged();
    void onColorChanged();
    void onFontsizeChanged();
    void onDrawingStyleChanged();
    void onOverrideToggled();
    void onDimAngleChanged();
    void onExtAngleChanged();
    void onDimUseDefaultClicked();
    void onDimUseSelectionClicked();
    void onExtUseDefaultClicked();
    void onExtUseSelectionClicked();

private:
    std::unique_ptr<Ui_TaskDimension> ui;
    QGIViewDimension *m_parent;
    Gui::WeakPtrT<ViewProviderDimension> m_dimensionVP;
    std::pair<double, bool> getAngleFromSelection();
};

class TaskDlgDimension : public Gui::TaskView::TaskDialog
{
    Q_OBJECT

public:
    TaskDlgDimension(QGIViewDimension *parent, ViewProviderDimension *dimensionVP);
    ~TaskDlgDimension() override;

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
    TaskDimension* widget;
    Gui::TaskView::TaskBox* taskbox;
};

} //namespace TechDrawGui

#endif // #ifndef GUI_TASKVIEW_TASKDIMENSION_H
