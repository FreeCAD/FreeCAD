/***************************************************************************
 *   Copyright (c) Juergen Riegel         <juergen.riegel@web.de>          *
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


#include "PreCompiled.h"

#ifndef _PreComp_
# include <QDockWidget>
# include <QPointer>
#endif

/// Here the FreeCAD includes sorted by Base,App,Gui......
#include "Control.h"
#include "TaskView/TaskView.h"

#include <Gui/MainWindow.h>
#include <Gui/CombiView.h>
#include <Gui/DockWindowManager.h>


using namespace Gui;
using namespace std;

/* TRANSLATOR Gui::ControlSingleton */

ControlSingleton* ControlSingleton::_pcSingleton = 0;
static QPointer<Gui::TaskView::TaskView> _taskPanel = 0;

ControlSingleton::ControlSingleton()
  : ActiveDialog(0)
{

}

ControlSingleton::~ControlSingleton()
{

}

Gui::TaskView::TaskView* ControlSingleton::taskPanel() const
{
    Gui::DockWnd::CombiView* pcCombiView = qobject_cast<Gui::DockWnd::CombiView*>
        (Gui::DockWindowManager::instance()->getDockWindow("Combo View"));
    // should return the pointer to combo view
    if (pcCombiView)
        return pcCombiView->getTaskPanel();
    // not all workbenches have the combo view enabled
    else if (_taskPanel)
        return _taskPanel;
    // no task panel available
    else
        return 0;
}

void ControlSingleton::showTaskView()
{
    Gui::DockWnd::CombiView* pcCombiView = qobject_cast<Gui::DockWnd::CombiView*>
        (Gui::DockWindowManager::instance()->getDockWindow("Combo View"));
    if (pcCombiView)
        pcCombiView->showTaskView();
    else if (_taskPanel)
        _taskPanel->raise();
}

void ControlSingleton::showModelView()
{
    Gui::DockWnd::CombiView* pcCombiView = qobject_cast<Gui::DockWnd::CombiView*>
        (Gui::DockWindowManager::instance()->getDockWindow("Combo View"));
    if (pcCombiView)
        pcCombiView->showTreeView();
    else if (_taskPanel)
        _taskPanel->raise();
}

void ControlSingleton::showDialog(Gui::TaskView::TaskDialog *dlg)
{
    // only one dialog at a time
    assert(!ActiveDialog || ActiveDialog==dlg);
    Gui::DockWnd::CombiView* pcCombiView = qobject_cast<Gui::DockWnd::CombiView*>
        (Gui::DockWindowManager::instance()->getDockWindow("Combo View"));
    // should return the pointer to combo view
    if (pcCombiView) {
        pcCombiView->showDialog(dlg);
        // make sure that the combo view is shown
        QDockWidget* dw = qobject_cast<QDockWidget*>(pcCombiView->parentWidget());
        if (dw) {
            dw->setVisible(true);
            dw->toggleViewAction()->setVisible(true);
            dw->setFeatures(QDockWidget::DockWidgetMovable|QDockWidget::DockWidgetFloatable);
        }

        if (ActiveDialog == dlg)
            return; // dialog is already defined
        ActiveDialog = dlg;
        connect(dlg, SIGNAL(destroyed()), this, SLOT(closedDialog()));
    }
    // not all workbenches have the combo view enabled
    else if (!_taskPanel) {
        QDockWidget* dw = new QDockWidget();
        dw->setWindowTitle(tr("Task panel"));
        dw->setFeatures(QDockWidget::DockWidgetMovable);
        _taskPanel = new Gui::TaskView::TaskView(dw);
        dw->setWidget(_taskPanel);
        _taskPanel->showDialog(dlg);
        getMainWindow()->addDockWidget(Qt::LeftDockWidgetArea, dw);
        connect(dlg, SIGNAL(destroyed()), dw, SLOT(deleteLater()));

        // if we have the normal tree view available then just tabify with it
        QWidget* treeView = Gui::DockWindowManager::instance()->getDockWindow("Tree view");
        QDockWidget* par = treeView ? qobject_cast<QDockWidget*>(treeView->parent()) : 0;
        if (par && par->isVisible()) {
            getMainWindow()->tabifyDockWidget(par, dw);
            qApp->processEvents(); // make sure that the task panel is tabified now
            dw->show();
            dw->raise();
        }
    }
}

Gui::TaskView::TaskDialog* ControlSingleton::activeDialog() const
{
    return ActiveDialog;
}

void ControlSingleton::closeDialog()
{
    Gui::DockWnd::CombiView* pcCombiView = qobject_cast<Gui::DockWnd::CombiView*>
        (Gui::DockWindowManager::instance()->getDockWindow("Combo View"));
    // should return the pointer to combo view
    if (pcCombiView)
        pcCombiView->closeDialog();
    else if (_taskPanel)
        _taskPanel->removeDialog();
}

void ControlSingleton::closedDialog()
{
    ActiveDialog = 0;
    Gui::DockWnd::CombiView* pcCombiView = qobject_cast<Gui::DockWnd::CombiView*>
        (Gui::DockWindowManager::instance()->getDockWindow("Combo View"));
    // should return the pointer to combo view
    assert(pcCombiView);
    pcCombiView->closedDialog();
    // make sure that the combo view is shown
    QDockWidget* dw = qobject_cast<QDockWidget*>(pcCombiView->parentWidget());
    if (dw)
        dw->setFeatures(QDockWidget::AllDockWidgetFeatures);
}

bool ControlSingleton::isAllowedAlterDocument(void) const
{
    if (ActiveDialog)
        return ActiveDialog->isAllowedAlterDocument();
    return true;
}


bool ControlSingleton::isAllowedAlterView(void) const
{
    if (ActiveDialog)
        return ActiveDialog->isAllowedAlterView();
    return true;
}

bool ControlSingleton::isAllowedAlterSelection(void) const
{
    if (ActiveDialog)
        return ActiveDialog->isAllowedAlterSelection();
    return true;
}

// -------------------------------------------

ControlSingleton& ControlSingleton::instance(void)
{
    if (_pcSingleton == NULL)
        _pcSingleton = new ControlSingleton;
    return *_pcSingleton;
}

void ControlSingleton::destruct (void)
{
    if (_pcSingleton != NULL)
        delete _pcSingleton;
    _pcSingleton = 0;
}


// -------------------------------------------


#include "moc_Control.cpp"

