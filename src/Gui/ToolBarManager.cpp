/***************************************************************************
 *   Copyright (c) 2005 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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
# include <QApplication>
# include <QAction>
# include <QToolBar>
# include <QToolButton>
# include <QMenu>
# include <QPointer>
#endif

#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string.hpp>
#include <Base/Tools.h>
#include <Base/Console.h>
#include "ToolBarManager.h"
#include "MainWindow.h"
#include "Application.h"
#include "Command.h"

FC_LOG_LEVEL_INIT("Gui", true, true)

using namespace Gui;

ToolBarItem::ToolBarItem()
{
}

ToolBarItem::ToolBarItem(ToolBarItem* item)
{
    if ( item )
        item->appendItem(this);
}

ToolBarItem::~ToolBarItem()
{
    clear();
}

void ToolBarItem::setCommand(const std::string& name)
{
    _name = name;
}

const std::string & ToolBarItem::command() const
{
    return _name;
}

void ToolBarItem::setID(const std::string& name)
{
    _id = name;
}

const std::string & ToolBarItem::id() const
{
    return _id;
}

bool ToolBarItem::hasItems() const
{
    return _items.count() > 0;
}

ToolBarItem* ToolBarItem::findItem(const std::string& name)
{
    if ( _name == name ) {
        return this;
    } else {
        for ( QList<ToolBarItem*>::ConstIterator it = _items.begin(); it != _items.end(); ++it ) {
            if ( (*it)->_name == name ) {
                return *it;
            }
        }
    }

    return 0;
}

ToolBarItem* ToolBarItem::copy() const
{
    ToolBarItem* root = new ToolBarItem;
    root->setCommand( command() );
    root->setID( id() );

    QList<ToolBarItem*> items = getItems();
    for ( QList<ToolBarItem*>::ConstIterator it = items.begin(); it != items.end(); ++it ) {
        root->appendItem( (*it)->copy() );
    }

    return root;
}

uint ToolBarItem::count() const
{
    return _items.count();
}

void ToolBarItem::appendItem(ToolBarItem* item)
{
    _items.push_back( item );
}

bool ToolBarItem::insertItem( ToolBarItem* before, ToolBarItem* item)
{
    int pos = _items.indexOf(before);
    if (pos != -1) {
        _items.insert(pos, item);
        return true;
    } else
        return false;
}

void ToolBarItem::removeItem(ToolBarItem* item)
{
    int pos = _items.indexOf(item);
    if (pos != -1)
        _items.removeAt(pos);
}

void ToolBarItem::clear()
{
    for ( QList<ToolBarItem*>::Iterator it = _items.begin(); it != _items.end(); ++it ) {
        delete *it;
    }

    _items.clear();
}

ToolBarItem& ToolBarItem::operator << (ToolBarItem* item)
{
    appendItem(item);
    return *this;
}

ToolBarItem& ToolBarItem::operator << (const std::string& command)
{
    ToolBarItem* item = new ToolBarItem(this);
    item->setCommand(command);
    return *this;
}

QList<ToolBarItem*> ToolBarItem::getItems() const
{
    return _items;
}

// -----------------------------------------------------------

ToolBarManager* ToolBarManager::_instance=0;

ToolBarManager* ToolBarManager::getInstance()
{
    if ( !_instance )
        _instance = new ToolBarManager;
    return _instance;
}

void ToolBarManager::destruct()
{
    delete _instance;
    _instance = 0;
}

ToolBarManager::ToolBarManager()
{
    defaultArea = Qt::TopToolBarArea;
    hMainWindow = App::GetApplication().GetUserParameter().GetGroup("BaseApp/Preferences/MainWindow");
    std::string defarea = hMainWindow->GetASCII("DefaultToolBarArea");
    if (defarea == "Bottom")
        defaultArea = Qt::BottomToolBarArea;
    else if (defarea == "Left")
        defaultArea = Qt::LeftToolBarArea;
    else if (defarea == "Right")
        defaultArea = Qt::RightToolBarArea;

    hPref = App::GetApplication().GetUserParameter().GetGroup("BaseApp/MainWindow/Toolbars");
    hMovable = hPref->GetGroup("Movable");
    connParam = App::GetApplication().GetUserParameter().signalParamChanged.connect(
        [this](ParameterGrp *Param, const char *, const char *Name, const char *) {
            if (Param == hPref || Param == hMovable
                    || (Param == hMainWindow
                        && Name
                        && boost::equals(Name, "DefaultToolBarArea")))
                timer.start(100);
        });
    timer.setSingleShot(true);
    connect(&timer, SIGNAL(timeout()), this, SLOT(onTimer()));

    auto toolbars = toolBars();
    for (auto &v : hPref->GetBoolMap()) {
        if (v.first.empty())
            continue;
        QString name = QString::fromUtf8(v.first.c_str());
        QToolBar* toolbar = toolbars[name];
        if (!toolbar)
            toolbar = createToolBar(name);
        toolbar->toggleViewAction()->setVisible(false);
        setToolBarVisible(toolbar, false);
    }
}

ToolBarManager::~ToolBarManager()
{
}

bool ToolBarManager::isDefaultMovable() const
{
    return hMovable->GetBool("*", true);
}

void ToolBarManager::setDefaultMovable(bool enable)
{
    hMovable->Clear();
    hMovable->SetBool("*", enable);
}

struct ToolBarKey {
    Qt::ToolBarArea area;
    bool visible;
    int a;
    int b;

    ToolBarKey(QToolBar *tb)
        :area(getMainWindow()->toolBarArea(tb))
        ,visible(tb->isVisible())
    {
        int x = tb->x();
        int y = tb->y();
        switch(area) {
        case Qt::BottomToolBarArea:
            a = -y; b = x;
            break;
        case Qt::LeftToolBarArea:
            a = x; b = y;
            break;
        case Qt::RightToolBarArea:
            a = -x, b = y;
            break;
        default:
            a = y; b = x;
            break;
        }
    }

    bool operator<(const ToolBarKey &other) const {
        if (area < other.area)
            return true;
        if (area > other.area)
            return false;
        if (visible > other.visible)
            return true;
        if (visible < other.visible)
            return false;
        if (a < other.a)
            return true;
        if (a > other.a)
            return false;
        return b < other.b;
    }
};

static bool isToolBarAllowed(QToolBar *toolbar, Qt::ToolBarArea area)
{
    if (area == Qt::TopToolBarArea || area == Qt::BottomToolBarArea)
        return true;
    for (auto w : toolbar->findChildren<QWidget*>(QString(), Qt::FindDirectChildrenOnly)) {
        if (qobject_cast<QToolButton*>(w)
                || qobject_cast<QMenu*>(w)
                || strstr(w->metaObject()->className(),"Separator"))
            continue;
        return false;
    }
    return true;
}

void ToolBarManager::onTimer()
{
    std::string defarea = hMainWindow->GetASCII("DefaultToolBarArea");
    auto area = Qt::TopToolBarArea;
    if (defarea == "Bottom")
        area = Qt::BottomToolBarArea;
    else if (defarea == "Left")
        area = Qt::LeftToolBarArea;
    else if (defarea == "Right")
        area = Qt::RightToolBarArea;
    auto mw = getMainWindow();
    bool movable = isDefaultMovable();

    std::multimap<ToolBarKey, QToolBar*> lines;
    for (auto &v : toolBars()) {
        if (!v.second)
            continue;
        QToolBar *tb = v.second;
        if (defaultArea != area && mw->toolBarArea(tb) == defaultArea)
            lines.emplace(ToolBarKey(tb),tb);
        QByteArray name = v.first.toUtf8();
        if (tb->toggleViewAction()->isVisible())
            setToolBarVisible(tb, hPref->GetBool(name, tb->isVisible()));
        tb->setMovable(hMovable->GetBool(name, movable));
    }

    bool first = true;
    int a;
    for (auto &v : lines) {
        if (!isToolBarAllowed(v.second, area))
            continue;
        if (first)
            first = false;
        else if (a != v.first.a)
            mw->addToolBarBreak(area);
        a = v.first.a;
        mw->addToolBar(area, v.second);
    }

    if (defaultArea != area) {
        defaultArea = area;
        mw->saveWindowSettings(true);
    }
}

QToolBar *ToolBarManager::createToolBar(const QString &name)
{
    auto toolbar = new QToolBar(getMainWindow());
    getMainWindow()->addToolBar(defaultArea, toolbar);
    toolbar->setObjectName(name);
    connectToolBar(toolbar);
    return toolbar;
}

void ToolBarManager::connectToolBar(QToolBar *toolbar)
{
    auto &p = connectedToolBars[toolbar];
    if (p == toolbar)
        return;
    p = toolbar;
    connect(toolbar, SIGNAL(visibilityChanged(bool)), this, SLOT(onToggleToolBar(bool)));
    connect(toolbar, SIGNAL(movableChanged(bool)), this, SLOT(onMovableChanged(bool)));
    connect(toolbar, &QToolBar::topLevelChanged, [this](){
        if (this->restored)
            getMainWindow()->saveWindowSettings(true);
    });
}

void ToolBarManager::setup(ToolBarItem* toolBarItems)
{
    if (!toolBarItems)
        return; // empty menu bar

    this->toolbarNames.clear();

    int max_width = getMainWindow()->width();
    int top_width = 0;

    QList<ToolBarItem*> items = toolBarItems->getItems();
    auto toolbars = toolBars();
    bool movable = isDefaultMovable();
    for (auto item : items) {
        // search for the toolbar
        QString name;
        if (item->id().size())
            name = QString::fromLatin1("Custom_%1").arg(QString::fromLatin1(item->id().c_str()));
        else
            name = QString::fromUtf8(item->command().c_str());

        this->toolbarNames << name;
        std::string toolbarName = item->command();
        bool visible = hPref->GetBool(toolbarName.c_str(), true);
        if (item->id().size()) {
            // Migrate to use toolbar ID instead of title for identification to
            // avoid name conflict when using custom toolbar
            bool v = hPref->GetBool(name.toLatin1().constData(), true);
            if (v != hPref->GetBool(name.toLatin1().constData(), false))
                visible = v;
        }

        QToolBar *toolbar = nullptr;
        auto it = toolbars.find(name);
        if (it != toolbars.end()) {
            toolbar = it->second;
            toolbars.erase(it);
        }
        if (!toolbar)
            toolbar = createToolBar(name);

        bool toolbar_added = toolbar->windowTitle().isEmpty();

        // setup the toolbar
        setup(item, toolbar);
        if (toolbar->actions().isEmpty()) {
            if (FC_LOG_INSTANCE.isEnabled(FC_LOGLEVEL_LOG))
                FC_WARN("Empty toolbar " << name.toLatin1().constData());
            continue;
        }

        toolbar->setWindowTitle(QApplication::translate("Workbench", toolbarName.c_str())); // i18n
        toolbar->toggleViewAction()->setVisible(true);
        {
            QSignalBlocker blocker(toolbar);
            setToolBarVisible(toolbar, visible);
            toolbar->setMovable(hMovable->GetBool(toolbarName.c_str(), movable));
        }

        // try to add some breaks to avoid to have all toolbars in one line
        if (toolbar_added) {
            auto area = getMainWindow()->toolBarArea(toolbar);
            if (!isToolBarAllowed(toolbar, area))
                getMainWindow()->addToolBar(toolbar);

            if (top_width > 0 && getMainWindow()->toolBarBreak(toolbar))
                top_width = 0;
            // the width() of a toolbar doesn't return useful results so we estimate
            // its size by the number of buttons and the icon size
            QList<QToolButton*> btns = toolbar->findChildren<QToolButton*>();
            top_width += (btns.size() * toolbar->iconSize().width());
            if (top_width > max_width) {
                top_width = 0;
                getMainWindow()->insertToolBarBreak(toolbar);
            }
        }
    }

    // hide all unneeded toolbars
    for (auto &v : toolbars) {
        QToolBar *toolbar = v.second;
        if (!toolbar)
            continue;
        // ignore toolbars which do not belong to the previously active workbench
        if (!toolbar->toggleViewAction()->isVisible())
            continue;
        toolbar->toggleViewAction()->setVisible(false);
        setToolBarVisible(toolbar, false);
    }
}

void ToolBarManager::setToolBarVisible(QToolBar *toolbar, bool show)
{
    QSignalBlocker blocker(toolbar);
    if (!show) {
        // make sure that the main window has the focus when hiding the toolbar with
        // the combo box inside
        QWidget *fw = QApplication::focusWidget();
        while (fw &&  !fw->isWindow()) {
            if (fw == toolbar) {
                getMainWindow()->setFocus();
                break;
            }
            fw = fw->parentWidget();
        }
    }
    toolbar->setVisible(show);
}

void ToolBarManager::setup(ToolBarItem* item, QToolBar* toolbar) const
{
    CommandManager& mgr = Application::Instance->commandManager();
    QList<ToolBarItem*> items = item->getItems();
    QList<QAction*> actions = toolbar->actions();
    for (QList<ToolBarItem*>::ConstIterator it = items.begin(); it != items.end(); ++it) {
        // search for the action item.
        QString cmdName = QString::fromLatin1((*it)->command().c_str());
        QAction* action = findAction(actions, cmdName);
        if (!action) {
            int size = toolbar->actions().size(); 
            if ((*it)->command() == "Separator")
                toolbar->addSeparator();
            else 
                mgr.addTo((*it)->command().c_str(), toolbar);
            auto actions = toolbar->actions();

            // We now support single command adding multiple actions
            if (actions.size()) {
                for (int i=std::min(actions.size()-1, size); i<actions.size(); ++i)
                    actions[i]->setData(cmdName);
            }
        } else {
            do {
                // Note: For toolbars we do not remove and re-add the actions
                // because this causes flicker effects. So, it could happen that the order of
                // buttons doesn't match with the order of commands in the workbench.
                int index = actions.indexOf(action);
                actions.removeAt(index);
            } while ((*it)->command() != "Separator"
                    && (action = findAction(actions, cmdName)));
        }
    }

    // remove all tool buttons which we don't need for the moment
    for (QList<QAction*>::Iterator it = actions.begin(); it != actions.end(); ++it) {
        toolbar->removeAction(*it);
    }
}

void ToolBarManager::saveState() const
{
    // Toolbar visibility is now synced using Qt signals
}

void ToolBarManager::restoreState()
{
    bool movable = isDefaultMovable();
    for (auto &v : toolBars()) {
        QToolBar *toolbar = v.second;
        if (!toolbar)
            continue;
        if (toolbar->windowTitle().isEmpty()) {
            toolbar->toggleViewAction()->setVisible(false);
            setToolBarVisible(toolbar, false);
        }
        else if (this->toolbarNames.indexOf(toolbar->objectName()) >= 0) {
            QByteArray toolbarName = toolbar->objectName().toUtf8();
            QSignalBlocker block(toolbar);
            setToolBarVisible(toolbar, hPref->GetBool(toolbarName, toolbar->isVisible()));
            toolbar->setMovable(hMovable->GetBool(toolbarName, movable));
        }
    }
    restored = true;
}

void ToolBarManager::retranslate()
{
    for (auto &v : toolBars()) {
        QToolBar *toolbar = v.second;
        if (!toolbar) continue;
        if (toolbar->objectName().startsWith(QLatin1String("Custom_")))
            continue;
        QByteArray toolbarName = toolbar->objectName().toUtf8();
        toolbar->setWindowTitle(QApplication::translate("Workbench", (const char*)toolbarName));
    }
}

QAction* ToolBarManager::findAction(const QList<QAction*>& acts, const QString& item) const
{
    for (QList<QAction*>::ConstIterator it = acts.begin(); it != acts.end(); ++it) {
        if ((*it)->data().toString() == item)
            return *it;
    }

    return 0; // no item with the user data found
}

std::map<QString, QPointer<QToolBar>> ToolBarManager::toolBars()
{
    std::map<QString, QPointer<QToolBar>> res;
    for (auto tb : getMainWindow()->findChildren<QToolBar*>(
                QString(), Qt::FindDirectChildrenOnly))
    {
        QString name = tb->objectName();
        if (name.isEmpty())
            continue;
        auto &p = res[name];
        if (!p) {
            p = tb;
            connectToolBar(tb);
            continue;
        }
        if (p->windowTitle().isEmpty() || tb->windowTitle().isEmpty()) {
            // We now pre-create all recorded toolbars (with an empty title)
            // when application starts to be able to restore the toolbar
            // position properly. However, some user code may later create the
            // toolbar with the same name without checking existence (e.g. Draft
            // tray). So we replace our toolbar here.
            QToolBar *a = p, *b = tb;
            if (a->windowTitle().isEmpty())
                std::swap(a, b);
            if (!a->windowTitle().isEmpty()) {
                // replace toolbar b with a
                getMainWindow()->insertToolBar(b, a);
                p = a;
                if (connectedToolBars.erase(b)) {
                    b->setObjectName(QString::fromLatin1("__scrapped"));
                    b->deleteLater();
                }
                continue;
            }
        }

        if (FC_LOG_INSTANCE.isEnabled(FC_LOGLEVEL_LOG))
            FC_WARN("duplicate toolbar name " << name.toUtf8().constData());
    }
    return res;
}

void ToolBarManager::onToggleToolBar(bool visible)
{
    auto toolbar = qobject_cast<QToolBar*>(sender());
    if (!toolbar)
        return;
    if (visible && toolbar->windowTitle().isEmpty()) {
        setToolBarVisible(toolbar, false);
        return;
    }
    if (!restored)
        return;

    bool enabled = visible;
    if (!visible && !toolbar->toggleViewAction()->isVisible()) {
        // This usually means the toolbar is hidden as a result of
        // workbench switch. The parameter entry however means whether
        // the toolbar is enabled while at its owner workbench
        enabled = true;
    }
    Base::ConnectionBlocker block(connParam);
    hPref->SetBool(toolbar->objectName().toUtf8(), enabled);
}

void ToolBarManager::onMovableChanged(bool movable)
{
    if (!restored)
        return;
    auto toolbar = qobject_cast<QToolBar*>(sender());
    if (toolbar) {
        Base::ConnectionBlocker block(connParam);
        hMovable->SetBool(toolbar->objectName().toUtf8(), movable);
    }
}

#include "moc_ToolBarManager.cpp"
