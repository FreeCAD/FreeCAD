/***************************************************************************
 *   Copyright (c) 2007 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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
# include <QPointer>
# include <QDockWidget>
# include <QAction>
# include <QMap>
#endif

#include "DockWindowManager.h"
#include "MainWindow.h"
#include <App/Application.h>

using namespace Gui;

DockWindowItems::DockWindowItems()
{
}

DockWindowItems::~DockWindowItems()
{
}

void DockWindowItems::addDockWidget(const char* name, Qt::DockWidgetArea pos, bool visibility, bool tabbed)
{
    DockWindowItem item;
    item.name = QString::fromLatin1(name);
    item.pos = pos;
    item.visibility = visibility;
    item.tabbed = tabbed;
    _items << item;
}

void DockWindowItems::setDockingArea(const char* name, Qt::DockWidgetArea pos)
{
    for (QList<DockWindowItem>::iterator it = _items.begin(); it != _items.end(); ++it) {
        if (it->name == QLatin1String(name)) {
            it->pos = pos;
            break;
        }
    }
}

void DockWindowItems::setVisibility(const char* name, bool v)
{
    for (QList<DockWindowItem>::iterator it = _items.begin(); it != _items.end(); ++it) {
        if (it->name == QLatin1String(name)) {
            it->visibility = v;
            break;
        }
    }
}

void DockWindowItems::setVisibility(bool v)
{
    for (QList<DockWindowItem>::iterator it = _items.begin(); it != _items.end(); ++it) {
        it->visibility = v;
    }
}

const QList<DockWindowItem>& DockWindowItems::dockWidgets() const
{
    return this->_items;
}

// -----------------------------------------------------------

namespace Gui {
struct DockWindowManagerP
{
    QList<QDockWidget*> _dockedWindows;
    QMap<QString, QPointer<QWidget> > _dockWindows;
    DockWindowItems _dockWindowItems;
};
} // namespace Gui

DockWindowManager* DockWindowManager::_instance = 0;

DockWindowManager* DockWindowManager::instance()
{
    if ( _instance == 0 )
        _instance = new DockWindowManager;
    return _instance;
}

void DockWindowManager::destruct()
{
    delete _instance;
    _instance = 0;
}

DockWindowManager::DockWindowManager()
{
    d = new DockWindowManagerP;
}

DockWindowManager::~DockWindowManager()
{
    d->_dockedWindows.clear();
    delete d;
}

/**
 * Adds a new dock window to the main window and embeds the given \a widget.
 */
QDockWidget* DockWindowManager::addDockWindow(const char* name, QWidget* widget, Qt::DockWidgetArea pos)
{
    // creates the dock widget as container to embed this widget
    MainWindow* mw = getMainWindow();
    QDockWidget* dw = new QDockWidget(mw);
    // Note: By default all dock widgets are hidden but the user can show them manually in the view menu.
    // First, hide immediately the dock widget to avoid flickering, after setting up the dock widgets
    // MainWindow::loadLayoutSettings() is called to restore the layout.
    dw->hide();
    switch (pos) {
    case Qt::LeftDockWidgetArea:
    case Qt::RightDockWidgetArea:
    case Qt::TopDockWidgetArea:
    case Qt::BottomDockWidgetArea:
        mw->addDockWidget(pos, dw);
    default:
        break;
    }
    connect(dw, SIGNAL(destroyed(QObject*)),
            this, SLOT(onDockWidgetDestroyed(QObject*)));
    connect(widget, SIGNAL(destroyed(QObject*)),
            this, SLOT(onWidgetDestroyed(QObject*)));

    // add the widget to the dock widget
    widget->setParent(dw);
    dw->setWidget(widget);

    // set object name and window title needed for i18n stuff
    dw->setObjectName(QLatin1String(name));
    dw->setWindowTitle(QDockWidget::trUtf8(name));
    dw->setFeatures(QDockWidget::AllDockWidgetFeatures);

    d->_dockedWindows.push_back(dw);
    return dw;
}

/**
 * Returns the widget inside the dock window by name.
 * If it does not exist 0 is returned.
 */
QWidget* DockWindowManager::getDockWindow(const char* name) const
{
    for (QList<QDockWidget*>::ConstIterator it = d->_dockedWindows.begin(); it != d->_dockedWindows.end(); ++it) {
        if ((*it)->objectName() == QLatin1String(name))
            return (*it)->widget();
    }

    return 0;
}

/**
 * Returns a list of all widgets inside the dock windows.
 */
QList<QWidget*> DockWindowManager::getDockWindows() const
{
    QList<QWidget*> docked;
    for (QList<QDockWidget*>::ConstIterator it = d->_dockedWindows.begin(); it != d->_dockedWindows.end(); ++it)
        docked.push_back((*it)->widget());
    return docked;
}

/**
 * Removes the specified dock window with name \name without deleting it.
 */
QWidget* DockWindowManager::removeDockWindow(const char* name)
{
    QWidget* widget=0;
    for (QList<QDockWidget*>::Iterator it = d->_dockedWindows.begin(); it != d->_dockedWindows.end(); ++it) {
        if ((*it)->objectName() == QLatin1String(name)) {
            QDockWidget* dw = *it;
            d->_dockedWindows.erase(it);
            getMainWindow()->removeDockWidget(dw);
            // avoid to destruct the embedded widget
            widget = dw->widget();
            widget->setParent(0);
            dw->setWidget(0);
            disconnect(dw, SIGNAL(destroyed(QObject*)),
                       this, SLOT(onDockWidgetDestroyed(QObject*)));
            disconnect(widget, SIGNAL(destroyed(QObject*)),
                       this, SLOT(onWidgetDestroyed(QObject*)));
            delete dw; // destruct the QDockWidget, i.e. the parent of the widget
            break;
        }
    }

    return widget;
}

/**
 * Method provided for convenience. Does basically the same as the method above unless that
 * it accepts a pointer.
 */
void DockWindowManager::removeDockWindow(QWidget* widget)
{
    for (QList<QDockWidget*>::Iterator it = d->_dockedWindows.begin(); it != d->_dockedWindows.end(); ++it) {
        if ((*it)->widget() == widget) {
            QDockWidget* dw = *it;
            d->_dockedWindows.erase(it);
            getMainWindow()->removeDockWidget(dw);
            // avoid to destruct the embedded widget
            widget->setParent(0);
            dw->setWidget(0);
            disconnect(dw, SIGNAL(destroyed(QObject*)),
                       this, SLOT(onDockWidgetDestroyed(QObject*)));
            disconnect(widget, SIGNAL(destroyed(QObject*)),
                       this, SLOT(onWidgetDestroyed(QObject*)));
            delete dw; // destruct the QDockWidget, i.e. the parent of the widget
            break;
        }
    }
}

/**
 * Sets the window title for the dockable windows.
 */
void DockWindowManager::retranslate()
{
    for (QList<QDockWidget*>::Iterator it = d->_dockedWindows.begin(); it != d->_dockedWindows.end(); ++it) {
        (*it)->setWindowTitle(QDockWidget::tr((*it)->objectName().toLatin1()));
    }
}

/**
 * Appends a new \a widget with \a name to the list of available dock widgets. The caller must make sure that
 * the name is unique. If a widget with this name is already registered nothing is done but false is returned,
 * otherwise it is appended and true is returned.
 *
 * As default the following widgets are already registered:
 * \li Std_TreeView
 * \li Std_PropertyView
 * \li Std_ReportView
 * \li Std_ToolBox
 * \li Std_CombiView
 * \li Std_SelectionView
 *
 * To avoid name clashes the caller should use names of the form \a module_widgettype, i. e. if a analyse dialog for
 * the mesh module is added the name must then be Mesh_AnalyzeDialog. 
 *
 * To make use of dock windows when a workbench gets loaded the method setupDockWindows() must reimplemented in a 
 * subclass of Gui::Workbench. 
 */
bool DockWindowManager::registerDockWindow(const char* name, QWidget* widget)
{
    QMap<QString, QPointer<QWidget> >::Iterator it = d->_dockWindows.find(QLatin1String(name));
    if (it != d->_dockWindows.end() || !widget)
        return false;
    d->_dockWindows[QLatin1String(name)] = widget;
    widget->hide(); // hide the widget if not used
    return true;
}

/** Sets up the dock windows of the activated workbench. */
void DockWindowManager::setup(DockWindowItems* items)
{
    // save state of current dock windows
    saveState();
    d->_dockWindowItems = *items;

    ParameterGrp::handle hPref = App::GetApplication().GetUserParameter().GetGroup("BaseApp")
                               ->GetGroup("MainWindow")->GetGroup("DockWindows");
    QList<QDockWidget*> docked = d->_dockedWindows;
    const QList<DockWindowItem>& dws = items->dockWidgets();
    QList<QDockWidget*> areas[4];
    for (QList<DockWindowItem>::ConstIterator it = dws.begin(); it != dws.end(); ++it) {
        QDockWidget* dw = findDockWidget(docked, it->name);
        QByteArray dockName = it->name.toLatin1();
        bool visible = hPref->GetBool(dockName.constData(), it->visibility);

        if (!dw) {
            QMap<QString, QPointer<QWidget> >::ConstIterator jt = d->_dockWindows.find(it->name);
            if (jt != d->_dockWindows.end()) {
                dw = addDockWindow(jt.value()->objectName().toUtf8(), jt.value(), it->pos);
                jt.value()->show();
                dw->toggleViewAction()->setData(it->name);
                dw->setVisible(visible);
            }
        }
        else {
            dw->setVisible(visible);
            dw->toggleViewAction()->setVisible(true);
            int index = docked.indexOf(dw);
            docked.removeAt(index);
        }

        if (it->tabbed && dw) {
            Qt::DockWidgetArea pos = getMainWindow()->dockWidgetArea(dw);
            switch (pos) {
                case Qt::LeftDockWidgetArea:
                    areas[0] << dw;
                    break;
                case Qt::RightDockWidgetArea:
                    areas[1] << dw;
                    break;
                case Qt::TopDockWidgetArea:
                    areas[2] << dw;
                    break;
                case Qt::BottomDockWidgetArea:
                    areas[3] << dw;
                    break;
                default:
                    break;
            }
        }
    }

#if 0 // FIXME: don't tabify always after switching the workbench
    // tabify dock widgets for which "tabbed" is true and which have the same position
    for (int i=0; i<4; i++) {
        const QList<QDockWidget*>& dws = areas[i];
        for (QList<QDockWidget*>::ConstIterator it = dws.begin(); it != dws.end(); ++it) {
            if (*it != dws.front()) {
                getMainWindow()->tabifyDockWidget(dws.front(), *it);
            }
        }
    }
#endif

#if 0
    // hide all dock windows which we don't need for the moment
    for (QList<QDockWidget*>::Iterator it = docked.begin(); it != docked.end(); ++it) {
        QByteArray dockName = (*it)->toggleViewAction()->data().toByteArray();
        hPref->SetBool(dockName.constData(), (*it)->isVisible());
        (*it)->hide();
        (*it)->toggleViewAction()->setVisible(false);
    }
#endif
}

void DockWindowManager::saveState()
{
    ParameterGrp::handle hPref = App::GetApplication().GetUserParameter().GetGroup("BaseApp")
                               ->GetGroup("MainWindow")->GetGroup("DockWindows");

    const QList<DockWindowItem>& dockItems = d->_dockWindowItems.dockWidgets();
    for (QList<DockWindowItem>::ConstIterator it = dockItems.begin(); it != dockItems.end(); ++it) {
        QDockWidget* dw = findDockWidget(d->_dockedWindows, it->name);
        if (dw) {
            QByteArray dockName = dw->toggleViewAction()->data().toByteArray();
            hPref->SetBool(dockName.constData(), dw->isVisible());
        }
    }
}

QDockWidget* DockWindowManager::findDockWidget(const QList<QDockWidget*>& dw, const QString& name) const
{
    for (QList<QDockWidget*>::ConstIterator it = dw.begin(); it != dw.end(); ++it) {
        if ((*it)->toggleViewAction()->data().toString() == name)
            return *it;
    }

    return 0;
}

void DockWindowManager::onDockWidgetDestroyed(QObject* dw)
{
    for (QList<QDockWidget*>::Iterator it = d->_dockedWindows.begin(); it != d->_dockedWindows.end(); ++it) {
        if (*it == dw) {
            d->_dockedWindows.erase(it);
            break;
        }
    }
}

void DockWindowManager::onWidgetDestroyed(QObject* widget)
{
    for (QList<QDockWidget*>::Iterator it = d->_dockedWindows.begin(); it != d->_dockedWindows.end(); ++it) {
        // make sure that the dock widget is not about to being deleted
        if ((*it)->metaObject() != &QDockWidget::staticMetaObject) {
            disconnect(*it, SIGNAL(destroyed(QObject*)),
                       this, SLOT(onDockWidgetDestroyed(QObject*)));
            d->_dockedWindows.erase(it);
            break;
        }

        if ((*it)->widget() == widget) {
            // Delete the widget if not used anymore
            QDockWidget* dw = *it;
            dw->deleteLater();
            break;
        }
    }
}

#include "moc_DockWindowManager.cpp"
