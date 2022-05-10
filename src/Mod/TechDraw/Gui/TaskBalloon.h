/***************************************************************************
 *   Copyright (c) 2016 WandererFan <wandererfan@gmail.com>                *
 *   Copyright (c) 2019 Franck Jullien <franck.jullien@gmail.com>          *
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

#ifndef GUI_TASKVIEW_TASKBALLOON_H
#define GUI_TASKVIEW_TASKBALLOON_H

#include <Gui/TaskView/TaskDialog.h>
#include <Gui/TaskView/TaskView.h>

namespace TechDrawGui
{

class QGIViewBalloon;
class ViewProviderBalloon;
class Ui_TaskBalloon;
class TaskBalloon : public QWidget
{
    Q_OBJECT

public:
    TaskBalloon(QGIViewBalloon *parent, ViewProviderBalloon *balloonVP);
    ~TaskBalloon();

public:
    virtual bool accept();
    virtual bool reject();
    void recomputeFeature();

private Q_SLOTS:
    void onTextChanged();
    void onColorChanged();
    void onFontsizeChanged();
    void onBubbleShapeChanged();
    void onShapeScaleChanged();
    void onEndSymbolChanged();
    void onEndSymbolScaleChanged();
    void onLineVisibleChanged();
    void onLineWidthChanged();
    void onKinkLengthChanged();

private:
    std::unique_ptr<Ui_TaskBalloon> ui;
    QGIViewBalloon *m_parent;
    ViewProviderBalloon* m_balloonVP;
};

class TaskDlgBalloon : public Gui::TaskView::TaskDialog
{
    Q_OBJECT

public:
    TaskDlgBalloon(QGIViewBalloon *parent, ViewProviderBalloon *balloonVP);
    ~TaskDlgBalloon();

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
    TaskBalloon * widget;
    Gui::TaskView::TaskBox* taskbox;
};

} //namespace TechDrawGui

#endif // #ifndef GUI_TASKVIEW_TASKBALLOON_H
