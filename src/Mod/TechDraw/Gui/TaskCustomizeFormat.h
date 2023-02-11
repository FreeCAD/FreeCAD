/***************************************************************************
 *   Copyright (c) 2021 edi                                                *
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

#ifndef TECHDRAWGUI_TASKCUSTOMIZEFORMAT_H
#define TECHDRAWGUI_TASKCUSTOMIZEFORMAT_H

#include <Gui/TaskView/TaskDialog.h>
#include <Gui/TaskView/TaskView.h>
#include <Mod/TechDraw/TechDrawGlobal.h>


namespace App {
class DocumentObject;
}

namespace TechDraw
{
class DrawPage;
class DrawView;
class DrawViewPart;
class CosmeticEdge;
class LineFormat;
}

namespace TechDraw
{
class Face;
}

namespace TechDrawGui
{
class QGSPage;
class QGVPage;
class QGIView;
class QGIPrimPath;
class MDIViewPage;
class ViewProviderViewPart;
class Ui_TaskCustomizeFormat;

class TaskCustomizeFormat : public QWidget
{
    Q_OBJECT

public:
    explicit TaskCustomizeFormat(App::DocumentObject * object);
    ~TaskCustomizeFormat() override;

    virtual bool accept();
    virtual bool reject();
    void updateTask();

private Q_SLOTS:
    void onSymbolClicked();
    void onFormatChanged();

protected:
    void changeEvent(QEvent *e) override;

    void setUiEdit();

private:
    App::DocumentObject* selectedObject;
    bool isDimension;
    double dimRawValue;
    std::unique_ptr<Ui_TaskCustomizeFormat> ui;
}; // class TaskCustomizeFormat

class TaskDlgCustomizeFormat : public Gui::TaskView::TaskDialog
{
    Q_OBJECT

public:
    explicit TaskDlgCustomizeFormat(App::DocumentObject * object);
    ~TaskDlgCustomizeFormat() override;

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
    TaskCustomizeFormat* widget;

    Gui::TaskView::TaskBox* taskbox;
}; // class TaskDlgCustomizeFormat

} // namespace TechDrawGui

#endif // #ifndef TECHDRAWGUI_TASKCUSTOMIZEFORMAT_H

