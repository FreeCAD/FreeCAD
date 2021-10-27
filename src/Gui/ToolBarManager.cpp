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
# include <QPointer>
# include <QHBoxLayout>
# include <QStatusBar>
# include <QMouseEvent>
# include <QCheckBox>
#endif

#include <QWidgetAction>

#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string.hpp>
#include <Base/Tools.h>
#include <Base/Console.h>
#include "ToolBarManager.h"
#include "MainWindow.h"
#include "Application.h"
#include "Command.h"
#include "Action.h"
#include "OverlayWidgets.h"

FC_LOG_LEVEL_INIT("Toolbar", true, 2)

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

class StatusBarArea : public QWidget
{
public:
    StatusBarArea(QWidget *parent)
        :QWidget(parent)
    {
        _layout = new QHBoxLayout(this);
        // _layout->addSpacing(2);
        // _layout->setSpacing(6);
        _layout->setMargin(0);
        // setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    }

    void addWidget(QWidget *w) {
        if (_layout->indexOf(w) < 0) {
            _layout->addWidget(w);
        }
    }

    void insertWidget(int idx, QWidget *w) {
        int index = _layout->indexOf(w);
        if (index == idx)
            return;
        if (index > 0)
            _layout->removeWidget(w);
        _layout->insertWidget(idx, w);
    }

    void removeWidget(QWidget *w) {
        _layout->removeWidget(w);
    }

    QWidget *widgetAt(int index) const {
        auto item = _layout->itemAt(index);
        return item ? item->widget() : nullptr;
    }

    int count() const {
        return _layout->count();
    }

    int indexOf(QWidget *w) const {
        return _layout->indexOf(w);
    }

    template<class F>
    void foreachToolbar(F &&f) {
        for (int i = 0, c = _layout->count(); i < c; ++i) {
            auto toolbar = qobject_cast<QToolBar*>(widgetAt(i));
            if (!toolbar || toolbar->objectName().isEmpty()
                         || toolbar->objectName().startsWith(QStringLiteral("*")))
                continue;
            f(toolbar, i);
        }
    }

private:
    QHBoxLayout *_layout;
};

ToolBarManager::ToolBarManager()
{
    if (auto sb = getMainWindow()->statusBar()) {
        sb->installEventFilter(this);
        statusBarArea = new StatusBarArea(sb);
        statusBarArea->setObjectName(QStringLiteral("StatusBarArea"));
        sb->insertPermanentWidget(2, statusBarArea);
        statusBarArea->show();
    }

    globalArea = defaultArea = Qt::TopToolBarArea;
    hMainWindow = App::GetApplication().GetUserParameter().GetGroup("BaseApp/Preferences/MainWindow");
    std::string defarea = hMainWindow->GetASCII("DefaultToolBarArea");
    if (defarea == "Bottom")
        defaultArea = Qt::BottomToolBarArea;
    else if (defarea == "Left")
        defaultArea = Qt::LeftToolBarArea;
    else if (defarea == "Right")
        defaultArea = Qt::RightToolBarArea;
    defarea = hMainWindow->GetASCII("GlobalToolBarArea");
    if (defarea == "Bottom")
        globalArea = Qt::BottomToolBarArea;
    else if (defarea == "Left")
        globalArea = Qt::LeftToolBarArea;
    else if (defarea == "Right")
        globalArea = Qt::RightToolBarArea;

    hGlobal = App::GetApplication().GetUserParameter().GetGroup(
            "BaseApp/Workbench/Global/Toolbar");
    getGlobalToolbarNames();

    hStatusBar = App::GetApplication().GetUserParameter().GetGroup(
            "BaseApp/MainWindow/StatusBar");

    hPref = App::GetApplication().GetUserParameter().GetGroup(
            "BaseApp/MainWindow/Toolbars");
    hMovable = hPref->GetGroup("Movable");
    connParam = App::GetApplication().GetUserParameter().signalParamChanged.connect(
        [this](ParameterGrp *Param, ParameterGrp::ParamType, const char *Name, const char *) {
            if (Param == hPref || Param == hMovable || Param == hStatusBar
                    || (Param == hMainWindow
                        && Name
                        && boost::equals(Name, "DefaultToolBarArea")))
                timer.start(100);
            else if (Param == hGlobal)
                getGlobalToolbarNames();
        });
    timer.setSingleShot(true);
    connect(&timer, SIGNAL(timeout()), this, SLOT(onTimer()));

    timerChild.setSingleShot(true);
    QObject::connect(&timerChild, &QTimer::timeout, [this]() {
        toolBars();
    });

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

void ToolBarManager::getGlobalToolbarNames()
{
    globalToolBarNames.clear();
    globalToolBarNames.insert(QStringLiteral("File"));
    globalToolBarNames.insert(QStringLiteral("Structure"));
    globalToolBarNames.insert(QStringLiteral("Macro"));
    globalToolBarNames.insert(QStringLiteral("View"));
    globalToolBarNames.insert(QStringLiteral("Workbench"));
    for (auto &hGrp : this->hGlobal->GetGroups())
        globalToolBarNames.insert(QString::fromUtf8(hGrp->GetGroupName()));
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
    defarea = hMainWindow->GetASCII("GlobalToolBarArea");
    auto gArea = Qt::TopToolBarArea;
    if (defarea == "Bottom")
        gArea = Qt::BottomToolBarArea;
    else if (defarea == "Left")
        gArea = Qt::LeftToolBarArea;
    else if (defarea == "Right")
        gArea = Qt::RightToolBarArea;

    auto mw = getMainWindow();
    bool movable = isDefaultMovable();

    std::map<int, QToolBar*> sbToolbars;
    std::multimap<ToolBarKey, QToolBar*> lines;
    for (auto &v : toolBars()) {
        if (!v.second)
            continue;
        QToolBar *tb = v.second;
        bool isGlobal = globalToolBarNames.count(v.first);
        auto defArea = isGlobal ? globalArea : defaultArea;
        auto curArea = isGlobal ? gArea : area;
        QByteArray name = v.first.toUtf8();
        int idx = hStatusBar->GetInt(name, -1);

        if (idx >= 0) {
            sbToolbars[idx] = tb;
            continue;
        } else if (tb->parentWidget() != getMainWindow()) {
            Base::StateLocker guard(adding);
            getMainWindow()->addToolBar(tb);
        }

        if (defArea != curArea && mw->toolBarArea(tb) == defArea)
            lines.emplace(ToolBarKey(tb),tb);
        if (tb->toggleViewAction()->isVisible())
            setToolBarVisible(tb, hPref->GetBool(name, tb->isVisible()));
        tb->setMovable(hMovable->GetBool(name, movable));
    }

    bool first = true;
    int a;
    for (auto &v : lines) {
        auto toolbarArea = globalToolBarNames.count(
                v.second->objectName()) ? gArea : area;
        if (!isToolBarAllowed(v.second, toolbarArea))
            continue;
        Base::StateLocker guard(adding);
        if (first)
            first = false;
        else if (a != v.first.a)
            mw->addToolBarBreak(toolbarArea);
        a = v.first.a;
        mw->addToolBar(toolbarArea, v.second);
    }

    if (defaultArea != area || globalArea != gArea) {
        defaultArea = area;
        globalArea = gArea;
        mw->saveWindowSettings(true);
    }

    for (auto &v : sbToolbars) {
        bool vis = v.second->isVisible();
        getMainWindow()->removeToolBar(v.second);
        v.second->setOrientation(Qt::Horizontal);
        statusBarArea->insertWidget(v.first, v.second);
        setToolBarVisible(v.second, vis);
    }

    for (auto &v : hStatusBar->GetBoolMap()) {
        auto w = getMainWindow()->statusBar()->findChild<QWidget*>(
                QString::fromLatin1(v.first.c_str()));
        if (w)
            w->setVisible(v.second);
    }
}

QToolBar *ToolBarManager::createToolBar(const QString &name)
{
    Base::StateLocker guard(adding);
    auto toolbar = new QToolBar(getMainWindow());
    getMainWindow()->addToolBar(
            globalToolBarNames.count(name) ? globalArea : defaultArea, toolbar);
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
    toolbar->installEventFilter(this);
    connect(toolbar, SIGNAL(visibilityChanged(bool)), this, SLOT(onToggleToolBar(bool)));
    connect(toolbar, SIGNAL(movableChanged(bool)), this, SLOT(onMovableChanged(bool)));
    connect(toolbar, &QToolBar::topLevelChanged, [this](){
        if (this->restored)
            getMainWindow()->saveWindowSettings(true);
    });
    QByteArray name = p->objectName().toUtf8();
    p->setMovable(hMovable->GetBool(name, isDefaultMovable()));
    FC_LOG("connect toolbar " << name.constData() << ' ' << p);
}

void ToolBarManager::setup(ToolBarItem* toolBarItems)
{
    if (!toolBarItems)
        return; // empty menu bar

    this->toolbarNames.clear();

    std::map<int,int> widths;

    QList<ToolBarItem*> items = toolBarItems->getItems();
    auto toolbars = toolBars();
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
        bool newToolbar = false;
        if (!toolbar) {
            newToolbar = true;
            toolbar = createToolBar(name);
            QByteArray n(name.toLatin1());
            if (hPref->GetBool(n, true) != hPref->GetBool(n, false)) {
                // Make sure we remember the toolbar so that we can pre-create
                // it the next time the application is launched
                Base::ConnectionBlocker block(connParam);
                hPref->SetBool(n, true);
            }
        }

        bool toolbar_added = toolbar->windowTitle().isEmpty();

        // setup the toolbar
        setup(item, toolbar);
        if (toolbar->actions().isEmpty()) {
            FC_LOG("Empty toolbar " << name.toLatin1().constData());
            continue;
        }

        toolbar->setWindowTitle(QApplication::translate("Workbench", toolbarName.c_str())); // i18n
        toolbar->toggleViewAction()->setVisible(true);
        setToolBarVisible(toolbar, visible);

        // try to add some breaks to avoid to have all toolbars in one line
        if (toolbar_added) {
            auto area = getMainWindow()->toolBarArea(toolbar);
            if (!isToolBarAllowed(toolbar, area)) {
                Base::StateLocker guard(adding);
                getMainWindow()->addToolBar(toolbar);
            }

            int &top_width = widths[area];
            if (top_width > 0 && getMainWindow()->toolBarBreak(toolbar))
                top_width = 0;
            // the width() of a toolbar doesn't return useful results so we estimate
            // its size by the number of buttons and the icon size
            QList<QToolButton*> btns = toolbar->findChildren<QToolButton*>();
            top_width += (btns.size() * toolbar->iconSize().width());
            int max_width = toolbar->orientation() == Qt::Vertical
                ? getMainWindow()->height() : getMainWindow()->width();
            if (newToolbar && top_width > max_width) {
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
    if (items.empty())
        return;
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
    Base::ConnectionBlocker block(const_cast<ToolBarManager*>(this)->connParam);
    QHBoxLayout *layout = nullptr;
    for (auto l : getMainWindow()->statusBar()->findChildren<QHBoxLayout*>()) {
        if (l->indexOf(statusBarArea) >= 0) {
            layout = l;
            break;
        }
    }
    if (!layout)
        return;
    for (int i = 0, c = layout->count(); i < c; ++i) {
        auto w = layout->itemAt(i)->widget();
        if (!w)
            continue;
        if (w != statusBarArea) {
            if (w->windowTitle().isEmpty()
                    || w->objectName().isEmpty()
                    || w->objectName().startsWith(QStringLiteral("*")))
                continue;
            hStatusBar->SetBool(w->objectName().toUtf8().constData(), w->isVisible());
            continue;
        }
        for (auto &v : hStatusBar->GetIntMap())
            hStatusBar->RemoveInt(v.first.c_str());

        statusBarArea->foreachToolbar([this](QToolBar *toolbar, int idx) {
            hStatusBar->SetInt(toolbar->objectName().toUtf8().constData(), idx);
        });
    }

    // Toolbar visibility is now synced using Qt signals
}

void ToolBarManager::restoreState()
{
    std::map<int, QToolBar*> sbToolbars;
    for (auto &v : toolBars()) {
        QToolBar *toolbar = v.second;
        if (!toolbar)
            continue;
        QByteArray toolbarName = toolbar->objectName().toUtf8();
        if (toolbar->windowTitle().isEmpty()) {
            toolbar->toggleViewAction()->setVisible(false);
            setToolBarVisible(toolbar, false);
        }
        else if (this->toolbarNames.indexOf(toolbar->objectName()) >= 0)
            setToolBarVisible(toolbar, hPref->GetBool(toolbarName, toolbar->isVisible()));
        else
            setToolBarVisible(toolbar, false);
        int idx = hStatusBar->GetInt(toolbarName, -1);
        if (idx >= 0)
            sbToolbars[idx] = toolbar;
        else if (toolbar->parentWidget() != getMainWindow()) {
            Base::StateLocker guard(adding);
            getMainWindow()->addToolBar(toolbar);
        }
    }

    for (auto &v : sbToolbars) {
        bool vis = v.second->isVisible();
        getMainWindow()->removeToolBar(v.second);
        v.second->setOrientation(Qt::Horizontal);
        statusBarArea->insertWidget(v.first, v.second);
        setToolBarVisible(v.second, vis);
    }

    for (auto &v : hStatusBar->GetBoolMap()) {
        auto w = getMainWindow()->statusBar()->findChild<QWidget*>(
                QString::fromLatin1(v.first.c_str()));
        if (w)
            w->setVisible(v.second);
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
        if (toolbar->windowTitle().size())
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
    auto mw = getMainWindow();
    for (auto tb : mw->findChildren<QToolBar*>()) {
        auto parent = tb->parentWidget();
        if (parent != mw && parent != mw->statusBar() && parent != statusBarArea)
            continue;
        QString name = tb->objectName();
        if (name.isEmpty() || name.startsWith(QStringLiteral("*")))
            continue;
        auto &p = res[name];
        if (!p) {
            p = tb;
            connectToolBar(tb);
            continue;
        }
        if (p->windowTitle().isEmpty() || tb->windowTitle().isEmpty()) {
            // We now pre-create all recorded toolbars (with an empty title)
            // so that when application starts, Qt can restore the toolbar
            // position properly. However, some user code may later create the
            // toolbar with the same name without checking existence (e.g. Draft
            // tray). So we will replace our toolbar here.
            //
            // Also, because findChildren() may return object in any order, we
            // will only replace the toolbar having an empty title with the
            // other that has a title.

            QToolBar *a = p, *b = tb;
            if (a->windowTitle().isEmpty())
                std::swap(a, b);
            if (!a->windowTitle().isEmpty()) {
                // replace toolbar and insert it at the same location
                FC_LOG("replacing " << name.toUtf8().constData() << ' ' << b << " -> " << a);
                getMainWindow()->insertToolBar(b, a);
                p = a;
                connectToolBar(a);
                if (connectedToolBars.erase(b)) {
                    b->setObjectName(QString::fromLatin1("__scrapped"));
                    b->deleteLater();
                }
                continue;
            }
        }

        if (FC_LOG_INSTANCE.isEnabled(FC_LOGLEVEL_LOG))
            FC_WARN("duplicate toolbar " << name.toUtf8().constData());
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
    if (!restored || getMainWindow()->isRestoringWindowState())
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

void ToolBarManager::checkToolbar()
{
    if (_instance && !_instance->adding) {
        // In case some user code creates its own toolbar without going through
        // the toolbar manager, we shall call toolBars() using a timer to try
        // to hook it up. One example is 'Draft Snap'. See comment in
        // toolBars() on how we deal with this and move the toolBar to previous
        // saved position.
        _instance->timerChild.start();
    }
}

bool ToolBarManager::addToolbarToStatusBar(QObject *o, QMouseEvent *ev)
{
    auto statusBar = getMainWindow()->statusBar();
    if (!statusBar || !statusBar->isVisible())
        return false;

    auto tb = qobject_cast<QToolBar*>(o);
    if (!tb || !tb->isFloating())
        return false;

    static OverlayDragFrame *tbPlaceholder;
    static int tbIndex = -1;
    if (ev->type() == QEvent::MouseMove && ev->buttons() != Qt::LeftButton) {
        if (tbIndex >= 0) {
            statusBarArea->removeWidget(tbPlaceholder);
            tbPlaceholder->hide();
            tbIndex = -1;
        }
        return false;
    }

    if (ev->type() == QEvent::MouseButtonRelease && ev->button() != Qt::LeftButton)
        return false;

    QPoint pos = QCursor::pos();
    QRect rect(statusBar->mapToGlobal(QPoint(0,0)), statusBar->size());
    if (!rect.contains(pos)) {
        if (tbPlaceholder) {
            statusBarArea->removeWidget(tbPlaceholder);
            tbPlaceholder->hide();
            tbIndex = -1;
        }
        return false;
    }

    int idx = 0;
    for (int c = statusBarArea->count(); idx < c ;++idx) {
        auto w = statusBarArea->widgetAt(idx);
        if (!w || w->isHidden())
            continue;
        int p = w->mapToGlobal(w->rect().center()).x();
        if (pos.x() < p)
            break;
    }
    if (tbIndex >= 0 && tbIndex == idx-1)
        idx = tbIndex;
    if (ev->type() == QEvent::MouseMove) {
        if (!tbPlaceholder) {
            tbPlaceholder = new OverlayDragFrame(getMainWindow());
            tbPlaceholder->hide();
            tbIndex = -1;
        }
        if (tbIndex != idx) {
            tbIndex = idx;
            tbPlaceholder->setSizePolicy(tb->sizePolicy());
            tbPlaceholder->setMinimumWidth(tb->minimumWidth());
            tbPlaceholder->resize(tb->size());
            statusBarArea->insertWidget(idx, tbPlaceholder);
            tbPlaceholder->adjustSize();
            tbPlaceholder->show();
        }
    } else {
        tbIndex = idx;
        QTimer::singleShot(10, tb, [tb,this]() {
            tbPlaceholder->hide();
            {
                QSignalBlocker block(tb);
                statusBarArea->removeWidget(tbPlaceholder);
                getMainWindow()->removeToolBar(tb);
                tb->setOrientation(Qt::Horizontal);
                statusBarArea->insertWidget(tbIndex, tb);
                ToolBarManager::getInstance()->setToolBarVisible(tb, true);
            }
            tb->topLevelChanged(false);
            tbIndex = -1;
        });
    }
    return false;
}

void ToolBarManager::showStatusBarContextMenu()
{
    QMenu menu;
    QMenu menuUndock(tr("Undock toolbars"));
    QHBoxLayout *layout = nullptr;
    for (auto l : getMainWindow()->statusBar()->findChildren<QHBoxLayout*>()) {
        if (l->indexOf(statusBarArea) >= 0) {
            layout = l;
            break;
        }
    }
    if (!layout)
        return;
    auto tooltip = QObject::tr("Toggles visibility");
    auto ltooltip = QObject::tr("Undock from status bar");
    QCheckBox *checkbox;
    for (int i = 0, c = layout->count(); i < c; ++i) {
        auto w = layout->itemAt(i)->widget();
        if (!w)
            continue;
        if (w != statusBarArea) {
            if (w->objectName().isEmpty()
                    || w->objectName().startsWith(QStringLiteral("*")))
                continue;
            QString name = w->windowTitle();
            if (name.isEmpty()) {
                name = w->objectName();
                name.replace(QLatin1Char('_'), QLatin1Char(' '));
                name = name.simplified();
            }
            auto wa = Action::addCheckBox(&menu, name,
                    tooltip, QIcon(), w->isVisible(), &checkbox);
            QObject::connect(checkbox, SIGNAL(toggled(bool)), w, SLOT(setVisible(bool)));
            QObject::connect(wa, SIGNAL(triggered(bool)), w, SLOT(setVisible(bool)));
            continue;
        }

        statusBarArea->foreachToolbar([&](QToolBar *toolbar, int) {
            auto action = toolbar->toggleViewAction();
            if ((action->isVisible() || toolbar->isVisible()) && action->text().size()) {
                action->setVisible(true);
                auto wa = Action::addCheckBox(&menu, action->text(),
                        tooltip, action->icon(), action->isChecked(), &checkbox);
                QObject::connect(checkbox, SIGNAL(toggled(bool)), action, SIGNAL(triggered(bool)));
                QObject::connect(wa, SIGNAL(triggered(bool)), action, SIGNAL(triggered(bool)));

                wa = Action::addCheckBox(&menuUndock, action->text(),
                        ltooltip, action->icon(), true, &checkbox);
                QObject::connect(wa, &QAction::toggled, [checkbox](bool checked){
                    QSignalBlocker block(checkbox);
                    checkbox->setChecked(checked);
                });
                QObject::connect(wa, &QWidgetAction::toggled, [toolbar, this](bool checked) {
                    if (checked) {
                        QSignalBlocker blocker(toolbar);
                        statusBarArea->addWidget(toolbar);
                    } else if (toolbar->parentWidget() == getMainWindow())
                        return;
                    else {
                        auto pos = toolbar->mapToGlobal(QPoint(0, 0));
                        QSignalBlocker blocker(toolbar);
                        statusBarArea->removeWidget(toolbar);
                        {
                            Base::StateLocker guard(adding);
                            getMainWindow()->addToolBar(toolbar);
                        }
                        toolbar->setWindowFlags(Qt::Tool
                                | Qt::FramelessWindowHint
                                | Qt::X11BypassWindowManagerHint);
                        toolbar->move(pos.x(), pos.y()-toolbar->height()-10);
                        toolbar->adjustSize();
                        setToolBarVisible(toolbar, true);
                    }
                    toolbar->topLevelChanged(!checked);
                });
            }
        });
    }
    if (menuUndock.actions().size()) {
        menu.addSeparator();
        menu.addMenu(&menuUndock);
    }
    menu.exec(QCursor::pos());
}

bool ToolBarManager::eventFilter(QObject *o, QEvent *e)
{
    bool res = false;
    switch(e->type()) {
    case QEvent::MouseButtonRelease: {
        auto mev = static_cast<QMouseEvent*>(e);
        if (o == getMainWindow()->statusBar() && mev->button() == Qt::RightButton) {
            showStatusBarContextMenu();
            return true;
        }
    }
    // fall through
    case QEvent::MouseMove:
        res = addToolbarToStatusBar(o, static_cast<QMouseEvent*>(e));
        break;
    default:
        break;
    }
    return res;
}

#include "moc_ToolBarManager.cpp"
