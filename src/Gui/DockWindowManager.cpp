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
# include <QMdiArea>
# include <QTabBar>
# include <QTreeView>
# include <QHeaderView>
# include <QToolTip>
# include <QAction>
# include <QKeyEvent>
# include <QTimer>
# include <QMap>
#endif

#include <Base/Tools.h>
#include "DockWindowManager.h"
#include "MainWindow.h"
#include "ViewParams.h"
#include <App/Application.h>
#include "propertyeditor/PropertyEditor.h"

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

#ifdef FC_HAS_DOCK_OVERLAY

OverlayTabWidget::OverlayTabWidget(QWidget *parent, Qt::DockWidgetArea pos)
    :QTabWidget(parent)
{
    switch(pos) {
    case Qt::LeftDockWidgetArea:
        setTabPosition(QTabWidget::West);
        break;
    case Qt::RightDockWidgetArea:
        setTabPosition(QTabWidget::East);
        break;
    case Qt::TopDockWidgetArea:
        setTabPosition(QTabWidget::North);
        break;
    case Qt::BottomDockWidgetArea:
        setTabPosition(QTabWidget::South);
        break;
    default:
        break;
    }
    setOverlayMode(true);
    connect(this, SIGNAL(currentChanged(int)), this, SLOT(onCurrentChanged(int)));

    timer.setSingleShot(true);
    connect(&timer, SIGNAL(timeout()), this, SLOT(onTimer()));

    connect(qApp, SIGNAL(focusChanged(QWidget*,QWidget*)), this, SLOT(onFocusChanged(QWidget*,QWidget*)));

    hide();
}

void OverlayTabWidget::onFocusChanged(QWidget *, QWidget *widget)
{
    if(ViewParams::getDockOverlayOnEnter() || !count())
        return;
    timer.stop();
    for(auto w=widget; w; w=w->parentWidget()) {
        auto parent = qobject_cast<OverlayTabWidget*>(w);
        if(parent) {
            if(parent == this) {
                if(overlayed) {
                    setOverlayMode(false);
                    raise();
                }
                return;
            }
            break;
        }
    }
    if(!overlayed)
        setOverlayMode(true);
}

void OverlayTabWidget::leaveEvent(QEvent*)
{
    if(!ViewParams::getDockOverlayOnLeave()) {
        auto focus = focusWidget();
        if(focus && qApp->focusWidget() == focus)
            return;
    }
    timer.stop();
    if(!overlayed)
        setOverlayMode(true);
}

void OverlayTabWidget::enterEvent(QEvent*)
{
    if(!ViewParams::getDockOverlayOnEnter()) {
        auto focus = focusWidget();
        if(!focus || qApp->focusWidget() != focus)
            return;
    }
    timer.start(300);
}

void OverlayTabWidget::onTimer()
{
    if(overlayed)
        setOverlayMode(false);
    raise();
}

void OverlayTabWidget::_setOverlayMode(QWidget *widget, bool enable)
{
    if(!widget)
        return;
    auto tabbar = qobject_cast<QTabBar*>(widget);
    if(tabbar) {
        tabbar->setDrawBase(!enable);
        tabbar->setDocumentMode(enable);
        tabbar->setVisible(!enable);
        return;
    }
    if(enable) {
        widget->setWindowFlags(Qt::FramelessWindowHint);
    } else {
        widget->setWindowFlags(widget->windowFlags() & ~Qt::FramelessWindowHint);
    }
    widget->setAttribute(Qt::WA_NoSystemBackground, enable);
    widget->setAttribute(Qt::WA_TranslucentBackground, enable);

    auto scrollarea = qobject_cast<QAbstractScrollArea*>(widget);
    if(scrollarea) {
        if(enable) {
            scrollarea->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
            scrollarea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        } else {
            scrollarea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
            scrollarea->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
        }
        auto treeview = qobject_cast<QTreeView*>(widget);
        if(treeview) {
            if(treeview->header()) 
                treeview->header()->setVisible(!enable);
            // auto propertyEditor = qobject_cast<Gui::PropertyEditor::PropertyEditor*>(widget);
            // if(propertyEditor)
            //     propertyEditor->setAlternatingRowColors(!enable);
        }
        return;
    }
}

void OverlayTabWidget::setOverlayMode(QWidget *widget, bool enable)
{
    _setOverlayMode(widget, enable);
    for(auto child : widget->findChildren<QObject*>())
        _setOverlayMode(qobject_cast<QWidget*>(child), enable);
}

void OverlayTabWidget::setOverlayMode(bool enable)
{
    overlayed = enable;
    if(enable)
        setStyleSheet(QLatin1String(
                    "background:transparent;"
                    "border:1px solid black;"
                    "alternate-background-color:transparent;"
                    "QToolTip {background-color:lightgray;}"));
    else
        setStyleSheet(QLatin1String());
    setOverlayMode(this, enable);
    if(!enable && count() == 1)
        tabBar()->hide();

    if(enable) {
        if(!rectOverlay.isNull())
            setGeometry(rectOverlay);
    } else if(!rectActive.isNull())
        setGeometry(rectActive);
}

void OverlayTabWidget::addWidget(QDockWidget *dock, const QString &title)
{
    if(!dock->titleBarWidget()) {
        auto w = new QWidget();
        w->setObjectName(QLatin1String("OverlayTitle"));
        dock->setTitleBarWidget(w);
        w->hide();
    }

    setOverlayMode(dock, true);
    addTab(dock, title);

    if(count() == 1) 
        show();
}

void OverlayTabWidget::removeWidget(QDockWidget *dock)
{
    int index = indexOf(dock);
    if(index < 0)
        return;

    QWidget *w = dock->titleBarWidget();
    if(w && w->objectName() == QLatin1String("OverlayTitle")) {
        delete w;
        dock->setTitleBarWidget(nullptr);
    }

    setOverlayMode(dock, false);
    removeTab(index);

    if(!count())
        hide();
}

void OverlayTabWidget::onCurrentChanged(int index)
{
    (void)index;
    // for(int i=0,c=count(); i<c; ++i)
    //     widget(i)->setVisible(i==index);
}

void OverlayTabWidget::setCurrent(QWidget *widget)
{
    int index = indexOf(widget);
    if(index >= 0) {
        setCurrentIndex(index);
        onCurrentChanged(index);
    }
}

// -----------------------------------------------------------

struct OverlayInfo {
    const char *name;
    OverlayTabWidget *tabWidget;
    Qt::DockWidgetArea dockArea;
    QMap<QDockWidget*, OverlayInfo*> &overlayMap;
    ParameterGrp::handle hGrp;

    OverlayInfo(QWidget *parent, const char *name, Qt::DockWidgetArea pos, QMap<QDockWidget*, OverlayInfo*> &map)
        : name(name), dockArea(pos), overlayMap(map)
    {
        tabWidget = new OverlayTabWidget(parent, dockArea);

        hGrp = App::GetApplication().GetUserParameter().GetGroup("BaseApp")
                            ->GetGroup("MainWindow")->GetGroup("DockWindows")->GetGroup(name);
    }

    bool addWidget(QDockWidget *dock, bool forced=true) {
        if(!dock)
            return false;
        if(tabWidget->indexOf(dock) >= 0)
            return false;
        overlayMap[dock] = this;
        bool visible = dock->isVisible();

        auto focus = qApp->focusWidget();
        if(focus && focus != dock->focusWidget())
            focus = nullptr;

        tabWidget->addWidget(dock, dock->windowTitle());

        if(focus) {
            tabWidget->setCurrent(dock);
            focus = qApp->focusWidget();
            if(focus)
                focus->clearFocus();
        }

        if(forced) {
            auto mw = getMainWindow();
            for(auto d : mw->findChildren<QDockWidget*>()) {
                if(mw->dockWidgetArea(d) == dockArea)
                    addWidget(d, false);
            }
            if(visible) {
                dock->show();
                tabWidget->setCurrent(dock);
            }
        }
        return true;
    }

    void removeWidget() {
        if(!tabWidget->count())
            return;

        tabWidget->hide();

        QPointer<QWidget> focus = qApp->focusWidget();

        MainWindow *mw = getMainWindow();
        QDockWidget *lastDock = nullptr;
        QDockWidget *activeDock = qobject_cast<QDockWidget*>(tabWidget->currentWidget());
        for(auto dock : tabWidget->findChildren<QDockWidget*>()) {
            overlayMap.remove(dock);
            tabWidget->removeWidget(dock);
            dock->show();
            if(dock == activeDock)
                continue;
            if(lastDock)
                mw->tabifyDockWidget(lastDock, dock);
            else
                mw->addDockWidget(dockArea,dock);
            lastDock = dock;
        }

        if(activeDock) {
            if(lastDock)
                mw->tabifyDockWidget(lastDock, activeDock);
            else
                mw->addDockWidget(dockArea, activeDock);
        }

        if(focus)
            focus->setFocus();
    }

    bool geometry(QRect &rect) {
        if(!tabWidget->count())
            return false;
        rect = tabWidget->geometry();
        return true;
    }

    void setGeometry(int x, int y, int w, int h,
            int activeX, int activeY, int activeW, int activeH)
    {
        if(!tabWidget->count())
            return;
        tabWidget->rectOverlay = QRect(x,y,w,h);
        tabWidget->rectActive = QRect(activeX,activeY,activeW,activeH);
        tabWidget->setGeometry(tabWidget->rectOverlay);
    }

    void save()
    {
        QRect rect = tabWidget->geometry();
        hGrp->SetInt("Width", rect.width());
        hGrp->SetInt("Height", rect.height());
        hGrp->SetInt("Active", tabWidget->currentIndex());

        std::ostringstream os;
        for(int i=0,c=tabWidget->count(); i<c; ++i)
            os << tabWidget->widget(i)->objectName().toLatin1().constData() << ",";
        hGrp->SetASCII("Widgets", os.str().c_str());
    }

    void restore()
    {
        std::string widgets = hGrp->GetASCII("Widgets","");
        for(auto &name : QString::fromLatin1(widgets.c_str()).split(QLatin1Char(','))) {
            if(name.isEmpty())
                continue;
            auto dock = getMainWindow()->findChild<QDockWidget*>(name);
            if(dock)
                addWidget(dock);
        }
        int width = hGrp->GetInt("Width", 0);
        int height = hGrp->GetInt("Height", 0);
        if(width && height) {
            QRect rect = tabWidget->geometry();
            tabWidget->setGeometry(rect.left(),rect.top(),width,height);
        }
        int index = hGrp->GetInt("Active", -1);
        if(index >= 0)
            tabWidget->setCurrentIndex(index);
    }
};

#endif // FC_HAS_DOCK_OVERLAY

enum OverlayToggleMode {
    OverlayUnset,
    OverlaySet,
    OverlayToggle,
    OverlayCheck,
};

namespace Gui {
struct DockWindowManagerP
{
    QList<QDockWidget*> _dockedWindows;
    QMap<QString, QPointer<QWidget> > _dockWindows;
    DockWindowItems _dockWindowItems;
    QTimer _timer;

#ifdef FC_HAS_DOCK_OVERLAY
    QMap<QDockWidget*, OverlayInfo*> _overlays;
    OverlayInfo _left;
    OverlayInfo _right;
    OverlayInfo _top;
    OverlayInfo _bottom;
    bool busy = false;

    DockWindowManagerP(QWidget *parent)
        :_left(parent,"OverlayLeft", Qt::LeftDockWidgetArea,_overlays)
        ,_right(parent,"OverlayRight", Qt::RightDockWidgetArea,_overlays)
        ,_top(parent,"OverlayTop", Qt::TopDockWidgetArea,_overlays)
        ,_bottom(parent,"OverlayBottom",Qt::BottomDockWidgetArea,_overlays)
    {
    }

    bool toggleOverlay(QDockWidget *dock, OverlayToggleMode toggle,
            Qt::DockWidgetArea dockPos=Qt::NoDockWidgetArea)
    {
        if(!dock)
            return false;

        Base::StateLocker guard(busy);

        auto it = _overlays.find(dock);
        if(it != _overlays.end()) {
            if(toggle == OverlaySet || toggle == OverlayCheck)
                return true;
            auto overlay = it.value();
            _overlays.erase(it);
            overlay->removeWidget();
            return false;
        }
        if(toggle == OverlayUnset)
            return false;
        if(dockPos == Qt::NoDockWidgetArea)
            dockPos = getMainWindow()->dockWidgetArea(dock);
        OverlayInfo *overlay;
        switch(dockPos) {
        case Qt::LeftDockWidgetArea:
            overlay = &_left;
            break;
        case Qt::RightDockWidgetArea:
            overlay = &_right;
            break;
        case Qt::TopDockWidgetArea:
            overlay = &_top;
            break;
        case Qt::BottomDockWidgetArea:
            overlay = &_bottom;
            break;
        default:
            return false;
        }
        if(toggle == OverlayCheck && !overlay->tabWidget->count())
            return false;
        QRect rect = dock->geometry();
        if(overlay->addWidget(dock) && overlay->tabWidget->count()==1) {
            overlay->tabWidget->setGeometry(rect);
            _timer.start(50);
        }
        return true;
    }

    void refreshOverlay(QWidget *widget)
    {
        for(auto w=widget;w;w=w->parentWidget()) {
            auto dock = qobject_cast<QDockWidget*>(widget);
            if(dock) {
                auto it = _overlays.find(dock);
                if(it != _overlays.end())
                    OverlayTabWidget::setOverlayMode(widget,true);
                return;
            }
        }
    }

    void saveOverlay()
    {
        _left.save();
        _right.save();
        _top.save();
        _bottom.save();
    }

    void restoreOverlay()
    {
        Base::StateLocker guard(busy);

        _left.restore();
        _right.restore();
        _top.restore();
        _bottom.restore();
        _timer.start(100);
    }

    void resizeOverlay()
    {
        QMdiArea *mdi = getMainWindow()->findChild<QMdiArea*>();
        if(!mdi)
            return;

        int w = mdi->geometry().width();
        int h = mdi->geometry().height();
        auto tabbar = mdi->findChild<QTabBar*>();
        if(tabbar)
            h -= tabbar->height();

        QRect rectBottom(0,0,0,0);
        if(_bottom.geometry(rectBottom)) {
            _bottom.setGeometry(0,h-rectBottom.height(),w-ViewParams::getNaviWidgetSize()-10,rectBottom.height(),
                                0,h-rectBottom.height(),w,rectBottom.height());
        }
        QRect rectLeft(0,0,0,0);
        if(_left.geometry(rectLeft)) {
            _left.setGeometry(0,0,rectLeft.width(),h-rectBottom.height(),
                                0,0,rectLeft.width(),h);
        }
        QRect rectRight(0,0,0,0);
        if(_right.geometry(rectRight)) {
            int bh = rectBottom.height();
            if(!bh)
                bh = ViewParams::getNaviWidgetSize()+10;
            _right.setGeometry(w-rectRight.width(),0,rectRight.width(),h-bh,
                                w-rectRight.width(),0,rectRight.width(),h);
        }
        QRect rectTop(0,0,0,0);
        if(_top.geometry(rectTop)) {
            _top.setGeometry(rectLeft.width(),0,w-rectLeft.width()-rectRight.width(),rectTop.height(),
                                0,0,w,rectTop.height());
        }
    }

    void setOverlayMode(DockWindowManager::OverlayMode mode)
    {
        Base::StateLocker guard(busy);

        if(mode == DockWindowManager::DisableAll
                || mode == DockWindowManager::EnableAll)
        {
            for(auto dock : getMainWindow()->findChildren<QDockWidget*>())
                toggleOverlay(dock, mode==DockWindowManager::DisableAll ? OverlayUnset : OverlaySet);
            return;
        }

        QDockWidget *dock = nullptr;
        for(auto w=qApp->widgetAt(QCursor::pos()); w; w=w->parentWidget()) {
            dock = qobject_cast<QDockWidget*>(w);
            if(dock)
                break;
        }
        if(!dock) {
            for(auto w=qApp->focusWidget(); w; w=w->parentWidget()) {
                dock = qobject_cast<QDockWidget*>(w);
                if(dock)
                    break;
            }
        }
        toggleOverlay(dock, mode==DockWindowManager::ToggleActive
                ? OverlayToggle : (mode==DockWindowManager::EnableActive ? OverlaySet : OverlayUnset));
    }

    void onToggleDockWidget(QDockWidget *dock, bool checked)
    {
        if(!dock || busy)
            return;

        Base::StateLocker guard(busy);

        auto it = _overlays.find(dock);
        if(it == _overlays.end()) {
            if(checked)
                toggleOverlay(dock, OverlayCheck);
            return;
        }
        if(checked)
            it.value()->tabWidget->setCurrent(dock);
        else {
            it.value()->tabWidget->removeWidget(dock);
            getMainWindow()->addDockWidget(it.value()->dockArea, dock);
            _overlays.erase(it);
        }
    }

#else // FC_HAS_DOCK_OVERLAY

    DockWindowManagerP(QWidget *) {}
    void refreshOverlay(QWidget *) {}
    void saveOverlay() {}
    void restoreOverlay() {}
    void resizeOverlay() {}
    void setOverlayMode(DockWindowManager::OverlayMode) {}
    void onToggleDockWidget(QDockWidget *, bool) {}

    bool toggleOverlay(QDockWidget *, OverlayToggleMode,
            Qt::DockWidgetArea dockPos=Qt::NoDockWidgetArea)
    {
        (void)dockPos;
        return false;
    }

#endif // FC_HAS_DOCK_OVERLAY
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
    auto mdi = getMainWindow()->findChild<QMdiArea*>();
    assert(mdi);
    mdi->installEventFilter(this);
    d = new DockWindowManagerP(mdi);
    connect(&d->_timer, SIGNAL(timeout()), this, SLOT(onResize()));
    d->_timer.setSingleShot(true);
}

DockWindowManager::~DockWindowManager()
{
    d->_dockedWindows.clear();
    delete d;
}

void DockWindowManager::setOverlayMode(OverlayMode mode)
{
    d->setOverlayMode(mode);
}

/**
 * Adds a new dock window to the main window and embeds the given \a widget.
 */
QDockWidget* DockWindowManager::addDockWindow(const char* name, QWidget* widget, Qt::DockWidgetArea pos)
{
    if(!widget)
        return nullptr;
    QDockWidget *dw = qobject_cast<QDockWidget*>(widget->parentWidget());
    if(dw)
        return dw;

    // creates the dock widget as container to embed this widget
    MainWindow* mw = getMainWindow();
    dw = new QDockWidget(mw);
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

    connect(dw->toggleViewAction(), SIGNAL(triggered(bool)), this, SLOT(onToggleDockWidget(bool)));
    return dw;
}

void DockWindowManager::onToggleDockWidget(bool checked)
{
    auto action = qobject_cast<QAction*>(sender());
    if(!action)
        return;
    d->onToggleDockWidget(qobject_cast<QDockWidget*>(action->parent()), checked);
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
            d->toggleOverlay(dw, OverlayUnset);
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
    if (!widget)
        return;
    for (QList<QDockWidget*>::Iterator it = d->_dockedWindows.begin(); it != d->_dockedWindows.end(); ++it) {
        if ((*it)->widget() == widget) {
            QDockWidget* dw = *it;
            d->_dockedWindows.erase(it);
            d->toggleOverlay(dw, OverlayUnset);
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
 * \li Std_ComboView
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

QWidget* DockWindowManager::unregisterDockWindow(const char* name)
{
    QWidget* widget = 0;
    QMap<QString, QPointer<QWidget> >::Iterator it = d->_dockWindows.find(QLatin1String(name));
    if (it != d->_dockWindows.end()) {
        widget = d->_dockWindows.take(QLatin1String(name));
    }

    return widget;
}

QWidget* DockWindowManager::findRegisteredDockWindow(const char* name)
{
    QMap<QString, QPointer<QWidget> >::Iterator it = d->_dockWindows.find(QLatin1String(name));
    if (it != d->_dockWindows.end())
        return it.value();
    return nullptr;
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
                if(!visible)
                    continue;
            }
        }
        else {
            dw->setVisible(visible);
            dw->toggleViewAction()->setVisible(true);
            int index = docked.indexOf(dw);
            docked.removeAt(index);
        }

        if(dw)
            d->toggleOverlay(dw, OverlayCheck);
    }
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

void DockWindowManager::onResize()
{
    d->resizeOverlay();
}

bool DockWindowManager::eventFilter(QObject *o, QEvent *ev)
{
    switch(ev->type()) {
    case QEvent::Resize:
        if(qobject_cast<QMdiArea*>(o))
            d->_timer.start(50);
        return false;
    default:
        break;
    }
    return false;
}

void DockWindowManager::refreshOverlay(QWidget *widget)
{
    d->refreshOverlay(widget);
}

void DockWindowManager::saveOverlay()
{
    d->saveOverlay();
}

void DockWindowManager::restoreOverlay()
{
    d->restoreOverlay();
}

#include "moc_DockWindowManager.cpp"
