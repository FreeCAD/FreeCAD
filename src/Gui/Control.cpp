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

#include <App/AutoTransaction.h>
#include <Gui/ComboView.h>
#include <Gui/DockWindowManager.h>
#include <Gui/MainWindow.h>

#include "Control.h"
#include "BitmapFactory.h"
#include "Tree.h"
#include "TaskView/TaskView.h"


using namespace Gui;
using namespace std;

/* TRANSLATOR Gui::ControlSingleton */

ControlSingleton* ControlSingleton::_pcSingleton = nullptr;

ControlSingleton::ControlSingleton()
  : ActiveDialog(nullptr)
  , oldTabIndex(-1)
{

}

ControlSingleton::~ControlSingleton() = default;

Gui::TaskView::TaskView* ControlSingleton::taskPanel() const
{
    auto taskView = qobject_cast<Gui::TaskView::TaskView*>
        (Gui::DockWindowManager::instance()->getDockWindow("Tasks"));
    return taskView;
}

void ControlSingleton::showDockWidget(QWidget* widget)
{
    QWidget* parent = widget->parentWidget();
    if (parent) {
        parent->show();
        parent->raise();
    }
}

QTabBar* ControlSingleton::findTabBar(QDockWidget* widget) const
{
    int count = getMainWindow()->tabifiedDockWidgets(widget).size() + 1;
    if (count > 1) {
        QList<QTabBar*> bars = getMainWindow()->findChildren<QTabBar*>();
        for (auto it : bars) {
            if (it->count() <= count) {
                for (int i = 0; i < count; i++) {
                    if (it->tabText(i) == widget->windowTitle()) {
                        return it;
                    }
                }
            }
        }
    }

    return nullptr;
}

void ControlSingleton::aboutToShowDialog(QDockWidget* widget)
{
    static QIcon icon = Gui::BitmapFactory().pixmap("edit-edit.svg");
    QTabBar* bar = findTabBar(widget);
    if (bar) {
        oldTabIndex = bar->currentIndex();
        for (int i = 0; i < bar->count(); i++) {
            if (bar->tabText(i) == widget->windowTitle()) {
                bar->setTabIcon(i, icon);
                break;
            }
        }
    }

    widget->show();
    widget->raise();
}

void ControlSingleton::aboutToHideDialog(QDockWidget* widget)
{
    QTabBar* bar = findTabBar(widget);
    if (bar) {
        bar->setCurrentIndex(oldTabIndex);
        for (int i = 0; i < bar->count(); i++) {
            if (bar->tabText(i) == widget->windowTitle()) {
                bar->setTabIcon(i, QIcon());
                break;
            }
        }
    }
}

void ControlSingleton::showTaskView()
{
    Gui::TaskView::TaskView* taskView = taskPanel();
    if (taskView) {
        showDockWidget(taskView);
    }
}

void ControlSingleton::showModelView()
{
    auto treeView = qobject_cast<Gui::TreeDockWidget*>
        (Gui::DockWindowManager::instance()->getDockWindow("Tree view"));
    if (treeView) {
        showDockWidget(treeView);
    }
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

    // Since the caller sets up a modeless task panel, it indicates intention
    // for prolonged editing. So disable auto transaction in the current call
    // stack.
    // Do this before showing the dialog because its open() function is called
    // which may open a transaction but fails when auto transaction is still active.
    App::AutoTransaction::setEnable(false);

    Gui::TaskView::TaskView* taskView = taskPanel();
    // should return the pointer to combo view
    if (taskView) {
        taskView->showDialog(dlg);

        // make sure that the combo view is shown
        auto dw = qobject_cast<QDockWidget*>(taskView->parentWidget());
        if (dw) {
            aboutToShowDialog(dw);
            dw->setVisible(true);
            dw->toggleViewAction()->setVisible(true);
            dw->setFeatures(QDockWidget::DockWidgetMovable|QDockWidget::DockWidgetFloatable);
        }

        if (ActiveDialog == dlg)
            return; // dialog is already defined
        ActiveDialog = dlg;
        connect(dlg, &TaskView::TaskDialog::aboutToBeDestroyed,
                this, &ControlSingleton::closedDialog);
    }
}

Gui::TaskView::TaskDialog* ControlSingleton::activeDialog() const
{
    return ActiveDialog;
}

void ControlSingleton::accept()
{
    Gui::TaskView::TaskView* taskView = taskPanel();
    if (taskView) {
        taskView->accept();
        qApp->processEvents(QEventLoop::ExcludeUserInputEvents |
                            QEventLoop::ExcludeSocketNotifiers);
    }
}

void ControlSingleton::reject()
{
    Gui::TaskView::TaskView* taskView = taskPanel();
    if (taskView) {
        taskView->reject();
        qApp->processEvents(QEventLoop::ExcludeUserInputEvents |
                            QEventLoop::ExcludeSocketNotifiers);
    }
}

void ControlSingleton::closeDialog()
{
    Gui::TaskView::TaskView* taskView = taskPanel();
    if (taskView)
        taskView->removeDialog();
}

void ControlSingleton::closedDialog()
{
    ActiveDialog = nullptr;
    Gui::TaskView::TaskView* taskView = taskPanel();
    assert(taskView);

    // make sure that the combo view is shown
    auto dw = qobject_cast<QDockWidget*>(taskView->parentWidget());
    if (dw) {
        aboutToHideDialog(dw);
        dw->setFeatures(QDockWidget::DockWidgetClosable
                        | QDockWidget::DockWidgetMovable
                        | QDockWidget::DockWidgetFloatable);
    }
}

bool ControlSingleton::isAllowedAlterDocument() const
{
    if (ActiveDialog)
        return ActiveDialog->isAllowedAlterDocument();
    return true;
}


bool ControlSingleton::isAllowedAlterView() const
{
    if (ActiveDialog)
        return ActiveDialog->isAllowedAlterView();
    return true;
}

bool ControlSingleton::isAllowedAlterSelection() const
{
    if (ActiveDialog)
        return ActiveDialog->isAllowedAlterSelection();
    return true;
}

// -------------------------------------------

ControlSingleton& ControlSingleton::instance()
{
    if (!_pcSingleton)
        _pcSingleton = new ControlSingleton;
    return *_pcSingleton;
}

void ControlSingleton::destruct ()
{
    if (_pcSingleton)
        delete _pcSingleton;
    _pcSingleton = nullptr;
}


// -------------------------------------------


#include "moc_Control.cpp"

