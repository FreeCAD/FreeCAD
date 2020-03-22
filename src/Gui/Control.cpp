/***************************************************************************
 *   Copyright 2011 (c) Jürgen Riegel <juergen.riegel@web.de>              *
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
# include <QAction>
# include <QApplication>
# include <QDebug>
# include <QDockWidget>
# include <QPointer>
#endif

/// Here the FreeCAD includes sorted by Base,App,Gui......
#include "Control.h"
#include "TaskView/TaskView.h"

#include <App/AutoTransaction.h>
#include <Gui/MainWindow.h>
#include <Gui/ComboView.h>
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
    Gui::DockWnd::ComboView* pcComboView = qobject_cast<Gui::DockWnd::ComboView*>
        (Gui::DockWindowManager::instance()->getDockWindow("Combo View"));
    // should return the pointer to combo view
    if (pcComboView)
        return pcComboView->getTaskPanel();
    // not all workbenches have the combo view enabled
    else if (_taskPanel)
        return _taskPanel;
    // no task panel available
    else
        return 0;
}

void ControlSingleton::showTaskView()
{
    Gui::DockWnd::ComboView* pcComboView = qobject_cast<Gui::DockWnd::ComboView*>
        (Gui::DockWindowManager::instance()->getDockWindow("Combo View"));
    if (pcComboView)
        pcComboView->showTaskView();
    else if (_taskPanel)
        _taskPanel->raise();
}

void ControlSingleton::showModelView()
{
    Gui::DockWnd::ComboView* pcComboView = qobject_cast<Gui::DockWnd::ComboView*>
        (Gui::DockWindowManager::instance()->getDockWindow("Combo View"));
    if (pcComboView)
        pcComboView->showTreeView();
    else if (_taskPanel)
        _taskPanel->raise();
}

void ControlSingleton::showDialog(Gui::TaskView::TaskDialog *dlg)
{
    // only one dialog at a time, print a warning instead of raising an assert
    if (ActiveDialog && ActiveDialog != dlg) {
        if (dlg) {
            qWarning() << "ControlSingleton::showDialog: Can't show "
                       << dlg->metaObject()->className()
                       << " since there is already an active task dialog";
        }
        else {
            qWarning() << "ControlSingleton::showDialog: Task dialog is null";
        }
        return;
    }

    // Since the caller sets up a modeless task panel, it indicates intension
    // for prolonged editing. So disable auto transaction in the current call
    // stack.
    // Do this before showing the dialog because its open() function is called
    // which may open a transaction but fails when auto transaction is still active.
    App::AutoTransaction::setEnable(false);

    Gui::DockWnd::ComboView* pcComboView = qobject_cast<Gui::DockWnd::ComboView*>
        (Gui::DockWindowManager::instance()->getDockWindow("Combo View"));
    // should return the pointer to combo view
    if (pcComboView) {
        pcComboView->showDialog(dlg);
        // make sure that the combo view is shown
        QDockWidget* dw = qobject_cast<QDockWidget*>(pcComboView->parentWidget());
        if (dw) {
            dw->setVisible(true);
            dw->toggleViewAction()->setVisible(true);
            dw->setFeatures(QDockWidget::DockWidgetMovable|QDockWidget::DockWidgetFloatable);
        }

        if (ActiveDialog == dlg)
            return; // dialog is already defined
        ActiveDialog = dlg;
        connect(dlg, SIGNAL(aboutToBeDestroyed()), this, SLOT(closedDialog()));
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

QTabWidget* ControlSingleton::tabPanel() const
{
    Gui::DockWnd::ComboView* pcComboView = qobject_cast<Gui::DockWnd::ComboView*>
        (Gui::DockWindowManager::instance()->getDockWindow("Combo View"));
    // should return the pointer to combo view
    if (pcComboView)
        return pcComboView->getTabPanel();
    return 0;
}

Gui::TaskView::TaskDialog* ControlSingleton::activeDialog() const
{
    return ActiveDialog;
}

Gui::TaskView::TaskView* ControlSingleton::getTaskPanel()
{
    // should return the pointer to combo view
    Gui::DockWnd::ComboView* pcComboView = qobject_cast<Gui::DockWnd::ComboView*>
        (Gui::DockWindowManager::instance()->getDockWindow("Combo View"));
    if (pcComboView)
        return pcComboView->getTaskPanel();
    else
        return _taskPanel;
}

void ControlSingleton::accept()
{
    Gui::TaskView::TaskView* taskPanel = getTaskPanel();
    if (taskPanel) {
        taskPanel->accept();
        qApp->processEvents(QEventLoop::ExcludeUserInputEvents |
                            QEventLoop::ExcludeSocketNotifiers);
    }
}

void ControlSingleton::reject()
{
    Gui::TaskView::TaskView* taskPanel = getTaskPanel();
    if (taskPanel) {
        taskPanel->reject();
        qApp->processEvents(QEventLoop::ExcludeUserInputEvents |
                            QEventLoop::ExcludeSocketNotifiers);
    }
}

void ControlSingleton::closeDialog()
{
    Gui::DockWnd::ComboView* pcComboView = qobject_cast<Gui::DockWnd::ComboView*>
        (Gui::DockWindowManager::instance()->getDockWindow("Combo View"));
    // should return the pointer to combo view
    if (pcComboView)
        pcComboView->closeDialog();
    else if (_taskPanel)
        _taskPanel->removeDialog();
}

void ControlSingleton::closedDialog()
{
    ActiveDialog = 0;
    Gui::DockWnd::ComboView* pcComboView = qobject_cast<Gui::DockWnd::ComboView*>
        (Gui::DockWindowManager::instance()->getDockWindow("Combo View"));
    // should return the pointer to combo view
    assert(pcComboView);
    pcComboView->closedDialog();
    // make sure that the combo view is shown
    QDockWidget* dw = qobject_cast<QDockWidget*>(pcComboView->parentWidget());
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

