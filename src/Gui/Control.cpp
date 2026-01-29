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

#include <QAction>
#include <QApplication>
#include <QDebug>
#include <QDockWidget>
#include <QPointer>

#include <App/Document.h>
#include <Gui/Application.h>
#include <Gui/ComboView.h>
#include <Gui/DockWindowManager.h>
#include <Gui/MainWindow.h>
#include <Gui/Document.h>

#include "Control.h"
#include "BitmapFactory.h"
#include "Tree.h"
#include "TaskView/TaskView.h"


using namespace Gui;
using namespace std;

/* TRANSLATOR Gui::ControlSingleton */

ControlSingleton* ControlSingleton::_pcSingleton = nullptr;

ControlSingleton::ControlSingleton()
    : oldTabIndex(-1)
{}

ControlSingleton::~ControlSingleton() = default;

Gui::TaskView::TaskView* ControlSingleton::taskPanel() const
{
    auto taskView = qobject_cast<Gui::TaskView::TaskView*>(
        Gui::DockWindowManager::instance()->getDockWindow("Tasks")
    );
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
    auto treeView = qobject_cast<Gui::TreeDockWidget*>(
        Gui::DockWindowManager::instance()->getDockWindow("Tree view")
    );
    if (treeView) {
        showDockWidget(treeView);
    }
    else {
        auto comboView = qobject_cast<Gui::DockWnd::ComboView*>(
            Gui::DockWindowManager::instance()->getDockWindow("Model")
        );
        if (comboView) {
            showDockWidget(comboView);
        }
    }
}

void ControlSingleton::showDialog(Gui::TaskView::TaskDialog* dlg, App::Document* attachTo)
{
    attachTo = docOrDefault(attachTo);
    if (!attachTo) {
        qWarning() << "ControlSingleton::showDialog: Cannot attach to nullptr document";
        return;
    }

    Gui::TaskView::TaskView* taskView = taskPanel();
    // should return the pointer to combo view
    if (!taskView) {
        return;
    }

    // only one dialog at a time, print a warning instead of raising an assert
    TaskView::TaskDialog* foundDialog = taskView->dialog(attachTo);
    if (!dlg || foundDialog == dlg) {
        if (dlg) {
            qWarning() << "ControlSingleton::showDialog: Can't show "
                       << dlg->metaObject()->className()
                       << " since there is already an active task dialog in Document "
                       << (attachTo ? attachTo->getName() : "''");
        }
        else {
            qWarning() << "ControlSingleton::showDialog: Task dialog is null";
        }
        return;
    }

    bool addedDialog = taskView->showDialog(dlg, attachTo);

    // make sure that the combo view is shown
    if (auto dw = qobject_cast<QDockWidget*>(taskView->parentWidget())) {
        aboutToShowDialog(dw);
        dw->setVisible(true);
        dw->toggleViewAction()->setVisible(true);
        dw->setFeatures(QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable);
    }

    if (!addedDialog) {
        return;  // dialog is already defined
    }

    connect(dlg, &TaskView::TaskDialog::aboutToBeDestroyed, this, [this, attachTo] {
        closedDialog(attachTo);
    });
}

Gui::TaskView::TaskDialog* ControlSingleton::activeDialog(App::Document* attachedTo) const
{
    attachedTo = docOrDefault(attachedTo);
    if (!attachedTo) {
        return nullptr;
    }

    Gui::TaskView::TaskView* taskView = taskPanel();

    if (taskView) {
        return taskView->dialog(attachedTo);
    }
    return nullptr;
}

void ControlSingleton::accept(App::Document* attachedTo)
{
    attachedTo = docOrDefault(attachedTo);
    if (!attachedTo) {
        qWarning() << "ControlSingleton::accept: Cannot accept dialog of nullptr document";
        return;
    }

    Gui::TaskView::TaskView* taskView = taskPanel();
    if (taskView) {
        taskView->accept(attachedTo);
        qApp->processEvents(QEventLoop::ExcludeUserInputEvents | QEventLoop::ExcludeSocketNotifiers);
    }
}

void ControlSingleton::reject(App::Document* attachedTo)
{
    attachedTo = docOrDefault(attachedTo);
    if (!attachedTo) {
        qWarning() << "ControlSingleton::reject: Cannot reject dialog of nullptr document";
        return;
    }


    Gui::TaskView::TaskView* taskView = taskPanel();
    if (taskView) {
        taskView->reject(attachedTo);
        qApp->processEvents(QEventLoop::ExcludeUserInputEvents | QEventLoop::ExcludeSocketNotifiers);
    }
}

void ControlSingleton::closeDialog(App::Document* attachedTo)
{
    attachedTo = docOrDefault(attachedTo);
    if (!attachedTo) {
        qWarning() << "ControlSingleton::closeDialog: Cannot close dialog of nullptr document";
        return;
    }

    Gui::TaskView::TaskView* taskView = taskPanel();
    if (taskView) {
        taskView->removeDialog(attachedTo);
    }
}

void ControlSingleton::closedDialog(App::Document* attachedTo)
{
    Gui::TaskView::TaskView* taskView = taskPanel();
    assert(taskView);

    // make sure that the combo view is shown
    auto dw = qobject_cast<QDockWidget*>(taskView->parentWidget());
    if (dw) {
        aboutToHideDialog(dw);
        dw->setFeatures(
            QDockWidget::DockWidgetClosable | QDockWidget::DockWidgetMovable
            | QDockWidget::DockWidgetFloatable
        );
    }
}

bool ControlSingleton::isAllowedAlterDocument(App::Document* attachedTo) const
{
    attachedTo = docOrDefault(attachedTo);
    if (!attachedTo) {
        return true;
    }
    Gui::TaskView::TaskDialog* dlg = activeDialog(attachedTo);

    if (dlg) {
        return dlg->isAllowedAlterDocument();
    }
    return true;
}

bool ControlSingleton::isAllowedAlterView(App::Document* attachedTo) const
{
    attachedTo = docOrDefault(attachedTo);
    if (!attachedTo) {
        return true;
    }
    Gui::TaskView::TaskDialog* dlg = activeDialog(attachedTo);

    if (dlg) {
        return dlg->isAllowedAlterView();
    }
    return true;
}

bool ControlSingleton::isAllowedAlterSelection(App::Document* attachedTo) const
{
    attachedTo = docOrDefault(attachedTo);
    if (!attachedTo) {
        return true;
    }

    Gui::TaskView::TaskDialog* dlg = activeDialog(attachedTo);

    if (dlg) {
        return dlg->isAllowedAlterSelection();
    }
    return true;
}

App::Document* ControlSingleton::docOrDefault(App::Document* attachedTo)
{
    if (!attachedTo && Application::Instance->activeDocument()) {
        attachedTo = Application::Instance->activeDocument()->getDocument();
    }
    return attachedTo;
}

// -------------------------------------------

ControlSingleton& ControlSingleton::instance()
{
    if (!_pcSingleton) {
        _pcSingleton = new ControlSingleton;
    }
    return *_pcSingleton;
}

void ControlSingleton::destruct()
{
    if (_pcSingleton) {
        delete _pcSingleton;
    }
    _pcSingleton = nullptr;
}


// -------------------------------------------


#include "moc_Control.cpp"
