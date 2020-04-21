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
# include <QTextStream>
# include <QComboBox>
# include <QBoxLayout>
# include <QSpacerItem>
#endif

#include <array>

#include <Base/Tools.h>
#include <Base/Console.h>
#include "DockWindowManager.h"
#include "MainWindow.h"
#include "ViewParams.h"
#include "View3DInventor.h"
#include "SplitView3DInventor.h"
#include "Application.h"
#include <App/Application.h>
#include "propertyeditor/PropertyEditor.h"

FC_LOG_LEVEL_INIT("Dock", true, true);

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

OverlayProxyWidget::OverlayProxyWidget(OverlayTabWidget *tabOverlay)
    :QWidget(tabOverlay->parentWidget()), owner(tabOverlay)
{
}

void OverlayProxyWidget::enterEvent(QEvent *)
{
    DockWindowManager::instance()->refreshOverlay();
}

OverlayToolButton::OverlayToolButton(QWidget *parent)
    :QToolButton(parent)
{}

OverlayTabWidget::OverlayTabWidget(QWidget *parent, Qt::DockWidgetArea pos)
    :QTabWidget(parent)
{
    // This is necessary to capture any focus lost from switching the tab,
    // otherwise the lost focus will leak to the parent, i.e. MdiArea, which may
    // cause unexpected Mdi sub window switching.
    setFocusPolicy(Qt::StrongFocus);

    proxyWidget = new OverlayProxyWidget(this);
    proxyWidget->hide();
    proxyWidget->setStyleSheet(QLatin1String("background-color: transparent;"));
    _setOverlayMode(proxyWidget,true);

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

    hide();

#define TITLE_BUTTON_COLOR "# c #101010"
    static QIcon pxTransparent;
    if(pxTransparent.isNull()) {
        const char * const bytes[]={
            "10 10 2 1",
            ". c None",
            TITLE_BUTTON_COLOR,
            "..........",
            "...####...",
            ".##....##.",
            "##..##..##",
            "#..####..#",
            "#..####..#",
            "##..##..##",
            ".##....##.",
            "...####...",
            "..........",
        };
        pxTransparent = QIcon(QPixmap(bytes));
    }
    actTransparent.setIcon(pxTransparent);
    actTransparent.setCheckable(true);
    actTransparent.setData(QString::fromLatin1("OBTN Transparent"));
    actTransparent.setParent(this);
    addAction(&actTransparent);

    QPixmap pxAutoHide;
    if(pxAutoHide.isNull()) {
        const char * const bytes[]={
            "10 10 2 1",
            ". c None",
            TITLE_BUTTON_COLOR,
            "...#######",
            ".........#",
            "..##.....#",
            ".##......#",
            "#######..#",
            "#######..#",
            ".##......#",
            "..##.....#",
            ".........#",
            "...#######",
        };
        pxAutoHide = QPixmap(bytes);
    }
    switch(tabPosition()) {
    case West:
        actAutoHide.setIcon(pxAutoHide);
        break;
    case East:
        actAutoHide.setIcon(pxAutoHide.transformed(QTransform().scale(-1,1)));
        break;
    case North:
        actAutoHide.setIcon(pxAutoHide.transformed(QTransform().rotate(90)));
        break;
    case South:
        actAutoHide.setIcon(pxAutoHide.transformed(QTransform().rotate(-90)));
        break;
    }
    actAutoHide.setCheckable(true);
    actAutoHide.setData(QString::fromLatin1("OBTN AutoHide"));
    actAutoHide.setParent(this);
    addAction(&actAutoHide);

    static QIcon pxEditHide;
    if(pxEditHide.isNull()) {
        const char * const bytes[]={
            "10 10 2 1",
            ". c None",
            TITLE_BUTTON_COLOR,
            "......##..",
            ".....#.##.",
            "....#..###",
            "...#..#..#",
            "..##.#..#.",
            ".#.##..#..",
            "##..###...",
            "##...#....",
            "#####.....",
            "####......",
        };
        pxEditHide = QIcon(QPixmap(bytes));
    }
    actEditHide.setIcon(pxEditHide);
    actEditHide.setCheckable(true);
    actEditHide.setData(QString::fromLatin1("OBTN EditHide"));
    actEditHide.setParent(this);
    addAction(&actEditHide);

    static QIcon pxIncrease;
    if(pxIncrease.isNull()) {
        const char * const bytes[]={
            "10 10 2 1",
            ". c None",
            TITLE_BUTTON_COLOR,
            "....##....",
            "....##....",
            "....##....",
            "....##....",
            "##########",
            "##########",
            "....##....",
            "....##....",
            "....##....",
            "....##....",
        };
        pxIncrease = QIcon(QPixmap(bytes));
    }
    actIncrease.setIcon(pxIncrease);
    actIncrease.setData(QString::fromLatin1("OBTN Increase"));
    actIncrease.setParent(this);
    addAction(&actIncrease);

    static QIcon pxDecrease;
    if(pxDecrease.isNull()) {
        const char * const bytes[]={
            "10 10 2 1",
            ". c None",
            TITLE_BUTTON_COLOR,
            "..........",
            "..........",
            "..........",
            "..........",
            "##########",
            "##########",
            "..........",
            "..........",
            "..........",
            "..........",
        };
        pxDecrease = QIcon(QPixmap(bytes));
    }
    actDecrease.setIcon(pxDecrease);
    actDecrease.setData(QString::fromLatin1("OBTN Decrease"));
    actDecrease.setParent(this);
    addAction(&actDecrease);

    retranslate();
}

void OverlayTabWidget::changeEvent(QEvent *e)
{
    QTabWidget::changeEvent(e);
    if (e->type() == QEvent::LanguageChange)
        retranslate();
}

void OverlayTabWidget::retranslate()
{
    actTransparent.setToolTip(tr("Toggle transparent mode"));
    actAutoHide.setToolTip(tr("Toggle auto hide mode"));
    actEditHide.setToolTip(tr("Toggle auto hide on edit mode"));
    actIncrease.setToolTip(tr("Increase window size"));
    actDecrease.setToolTip(tr("Decrease window size"));
}

void OverlayTabWidget::onAction(QAction *action)
{
    if(action == &actEditHide) {
        if(action->isChecked())
            setAutoHide(false);
    } else if(action == &actAutoHide) {
        if(action->isChecked())
            setEditHide(false);
    } else if(action == &actIncrease)
        changeSize(5);
    else if(action == &actDecrease)
        changeSize(-5);
    DockWindowManager::instance()->refreshOverlay(this);
}

bool OverlayTabWidget::checkAutoHide() const
{
    if(isAutoHide())
        return true;

    if(ViewParams::getDockOverlayAutoView()) {
        auto view = getMainWindow()->activeWindow();
        if(!view || (!view->isDerivedFrom(View3DInventor::getClassTypeId())
                        && !view->isDerivedFrom(SplitView3DInventor::getClassTypeId())))
            return true;
    }

    if(isEditHide() && Application::Instance->editDocument())
        return true;
    return false;
}

static inline OverlayTabWidget *findTabWidget(QWidget *widget=nullptr)
{
    if(!widget)
        widget = qApp->focusWidget();
    for(auto w=widget; w; w=w->parentWidget()) {
        auto dock = qobject_cast<OverlayTabWidget*>(w);
        if(dock)
            return dock;
        auto proxy = qobject_cast<OverlayProxyWidget*>(w);
        if(proxy)
            return proxy->getOwner();
    }
    return nullptr;
}

void OverlayTabWidget::leaveEvent(QEvent*)
{
    DockWindowManager::instance()->refreshOverlay();
}

void OverlayTabWidget::enterEvent(QEvent*)
{
    DockWindowManager::instance()->refreshOverlay();
}

class OverlayStyleSheet: public ParameterGrp::ObserverType {
public:

    OverlayStyleSheet() {
        handle = App::GetApplication().GetParameterGroupByPath(
                "User parameter:BaseApp/Preferences/MainWindow");
        handle->Attach(this);
        update();
    }

    static OverlayStyleSheet *instance() {
        static OverlayStyleSheet *inst;
        if(!inst)
            inst = new OverlayStyleSheet;
        return inst;
    }

    void OnChange(Base::Subject<const char*> &, const char* sReason) {
        if(!sReason)
            return;
        if(strcmp(sReason, "StyleSheet")==0
                || strcmp(sReason, "OverlayActiveStyleSheet")==0
                || strcmp(sReason, "OverlayOnStyleSheet")==0
                || strcmp(sReason, "OverlayOffStyleSheet")==0)
        {
            update();
            updating = true;
            DockWindowManager::instance()->refreshOverlay();
        }
    }

    void update() {
        QString mainstyle = QString::fromLatin1(handle->GetASCII("StyleSheet").c_str());

        QLatin1String prefix("qss:");

        QString name = QString::fromLatin1("%1.overlay").arg(mainstyle);
        if(!QFile::exists(name)) {
            name = prefix + name;
            if(!QFile::exists(name)) {
                name = QString::fromUtf8(handle->GetASCII("OverlayOnStyleSheet").c_str());
                if(!QFile::exists(name))
                    name = prefix + name;
            }
        }

        onStyleSheet.clear();
        if(QFile::exists(name)) {
            QFile f(name);
            if(f.open(QFile::ReadOnly)) {
                QTextStream str(&f);
                onStyleSheet = str.readAll();
            }
        }
        if(onStyleSheet.isEmpty()) {
            static QLatin1String _default(
                "* { background-color: transparent; border: 1px solid palette(dark); alternate-background-color: transparent;}"
                // "QTabBar {qproperty-drawBase: 0; qproperty-documentMode: 1;}"
                // "QTabBar::tab {background-color: transparent; border: 1px solid darkgray;}"
                // "QHeaderView::section { background-color: transparent; border: 1px solid darkgray;}"
                "QTreeWidget, QListWidget {background: palette(base);}"
                "QToolTip { background-color: palette(base); }"
            );
            onStyleSheet = _default;
        }

        name = QString::fromLatin1("%1.overlay2").arg(mainstyle);
        if(!QFile::exists(name)) {
            name = prefix + name;
            if(!QFile::exists(name)) {
                name = QString::fromUtf8(handle->GetASCII("OverlayOffStyleSheet").c_str());
                if(!QFile::exists(name))
                    name = prefix + name;
            }
        }
        offStyleSheet.clear();
        if(QFile::exists(name)) {
            QFile f(name);
            if(f.open(QFile::ReadOnly)) {
                QTextStream str(&f);
                offStyleSheet = str.readAll();
            }
        }
        if(offStyleSheet.isEmpty()) {
            static QLatin1String _default(
                "Gui--OverlayToolButton { background: transparent; padding: 0px; border: none }"
                "Gui--OverlayToolButton:hover { background: palette(light); border: 1px solid palette(dark) }"
                "Gui--OverlayToolButton:focus { background: palette(dark); border: 1px solid palette(dark) }"
                "Gui--OverlayToolButton:pressed { background: palette(dark); border: 1px inset palette(dark) }"
                "Gui--OverlayToolButton:checked { background: palette(dark); border: 1px inset palette(dark) }"
                "Gui--OverlayToolButton:checked:hover { background: palette(light); border: 1px inset palette(dark) }"
            );
            offStyleSheet = _default;
        }

        hideTab = (onStyleSheet.indexOf(QLatin1String("QTabBar")) < 0);
        hideHeader = (onStyleSheet.indexOf(QLatin1String("QHeaderView")) < 0);
        hideScrollBar = (onStyleSheet.indexOf(QLatin1String("QAbstractScrollArea")) < 0);

        name = QString::fromLatin1("%1.overlay3").arg(mainstyle);
        if(!QFile::exists(name)) {
            name = prefix + name;
            if(!QFile::exists(name)) {
                name = QString::fromUtf8(handle->GetASCII("OverlayActiveStyleSheet").c_str());
                if(!QFile::exists(name))
                    name = prefix + name;
            }
        }
        activeStyleSheet.clear();
        if(QFile::exists(name)) {
            QFile f(name);
            if(f.open(QFile::ReadOnly)) {
                QTextStream str(&f);
                activeStyleSheet = str.readAll();
            }
        }
        if(activeStyleSheet.isEmpty()) {
            static QLatin1String _default(
                "* { background-color: transparent;"
                    "color: palette(window-text);"
                    "border: 1px solid palette(dark);"
                    "alternate-background-color: transparent;}"
                "QComboBox { background : palette(base);"
                            "selection-background-color: palette(highlight);}"
                "QComboBox:editable { background : palette(base);}"
                "QComboBox:!editable { background : palette(base);}"
                "QLineEdit { background : palette(base);}"
                "QAbstractSpinBox { background : palette(base);}"
                "QTabBar {border: none;}"
                "QTabBar::tab {background-color: transparent; border: 1px solid palette(dark);}"
                "QTabBar::tab:selected {background-color: palette(mid);}"
                "QTabBar::tab:hover {background-color: palette(light);}"
                "QHeaderView::section {background-color: transparent; border: 1px solid palette(dark);}"
                "QTreeWidget, QListWidget {background: palette(base)}" // necessary for checkable item to work in linux
                "QToolTip {background-color: palette(base);}"
                "Gui--CallTipsList::item { background-color: palette(base);}"
                "Gui--CallTipsList::item::selected { background-color: palette(highlight);}"
                "QDialog { background-color: palette(window); }"
                "QAbstractButton { background: palette(window);"
                                  "padding: 2px 4px;"
                                  "border: 1px solid palette(dark) }"
                "QAbstractButton:hover { background: palette(light); border: 1px solid palette(dark) }"
                "QAbstractButton:focus { background: palette(dark) ; border: 1px solid palette(dark)}"
                "QAbstractButton:pressed { background: palette(dark); border: 1px inset palette(dark) }"
                "QAbstractButton:checked { background: palette(dark); border: 1px inset palette(dark) }"
                "QAbstractButton:checked:hover { background: palette(light); border: 1px inset palette(dark) }"
                "Gui--OverlayToolButton { background: transparent; padding: 0px; border: none }"
                "QMenu { background: palette(window) }"
                );
            activeStyleSheet = _default;
        }
    }

    ParameterGrp::handle handle;
    QString onStyleSheet;
    QString offStyleSheet;
    QString activeStyleSheet;
    bool hideTab = true;
    bool hideHeader = false;
    bool hideScrollBar = false;
    bool updating = false;
};

void OverlayTabWidget::_setOverlayMode(QWidget *widget, int enable)
{
    if(!widget)
        return;

#if QT_VERSION>QT_VERSION_CHECK(5,12,2) && QT_VERSION < QT_VERSION_CHECK(5,12,6)
    // Work around Qt bug https://bugreports.qt.io/browse/QTBUG-77006
    if(enable < 0)
        widget->setStyleSheet(OverlayStyleSheet::instance()->activeStyleSheet);
    else if(enable)
        widget->setStyleSheet(OverlayStyleSheet::instance()->onStyleSheet);
    else
        widget->setStyleSheet(OverlayStyleSheet::instance()->offStyleSheet);
#endif

    if(enable<0 || OverlayStyleSheet::instance()->hideTab) {
        auto tabbar = qobject_cast<QTabBar*>(widget);
        if(tabbar) {
            tabbar->setDrawBase(enable>0);
            // QTabWidget insist on drawing the base unless documentMode is set
            // to true. This should be considered as a Qt bug.
            tabbar->setDocumentMode(enable!=0);
            tabbar->setVisible(enable<=0);
            return;
        }
    }
    if(enable!=0) {
        widget->setWindowFlags(widget->windowFlags() & Qt::FramelessWindowHint);
    } else {
        widget->setWindowFlags(widget->windowFlags() & ~Qt::FramelessWindowHint);
    }
    widget->setAttribute(Qt::WA_NoSystemBackground, enable!=0);
    widget->setAttribute(Qt::WA_TranslucentBackground, enable!=0);

    if(enable<0 || OverlayStyleSheet::instance()->hideScrollBar) {
        auto scrollarea = qobject_cast<QAbstractScrollArea*>(widget);
        if(scrollarea) {
            if(enable>0) {
                scrollarea->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
                scrollarea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
            } else {
                scrollarea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
                scrollarea->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
            }
        }
    }

    if(enable<0 || OverlayStyleSheet::instance()->hideHeader) {
        auto treeview = qobject_cast<QTreeView*>(widget);
        if(treeview) {
            if(treeview->header()) 
                treeview->header()->setVisible(enable<=0);
        }
    }
}

void OverlayTabWidget::setOverlayMode(QWidget *widget, int enable)
{
    if(!widget)
        return;
    _setOverlayMode(widget, enable);

    if(qobject_cast<QComboBox*>(widget)) {
        // do not set child QAbstractItemView of QComboBox, otherwise the drop down box
        // won't be shown
        return;
    }
    for(auto child : widget->children())
        setOverlayMode(qobject_cast<QWidget*>(child), enable);
}

void OverlayTabWidget::setAutoHide(bool enable)
{
    if(actAutoHide.isChecked() == enable)
        return;
    actAutoHide.setChecked(enable);
    if(enable)
        setEditHide(false);
    DockWindowManager::instance()->refreshOverlay(this);
}

void OverlayTabWidget::setTransparent(bool enable)
{
    if(actTransparent.isChecked() == enable)
        return;
    actTransparent.setChecked(enable);
    DockWindowManager::instance()->refreshOverlay(this);
}

void OverlayTabWidget::setEditHide(bool enable)
{
    if(actEditHide.isChecked() == enable)
        return;
    actEditHide.setChecked(enable);
    if(enable)
        setAutoHide(false);
    DockWindowManager::instance()->refreshOverlay(this);
}

void OverlayTabWidget::setOverlayMode(bool enable)
{
    overlayed = enable;

    if(!isVisible() || !count())
        return;

    auto w = currentWidget();
    if(w) {
        w = w->findChild<QWidget*>(QLatin1String("OverlayTitle"),Qt::FindDirectChildrenOnly);
        if(w)
            w->setVisible(!enable);
    }

    if(!enable && isTransparent())
    {
        setStyleSheet(OverlayStyleSheet::instance()->activeStyleSheet);
        setOverlayMode(this, -1);
    } else {
        if(enable)
            setStyleSheet(OverlayStyleSheet::instance()->onStyleSheet);
        else
            setStyleSheet(OverlayStyleSheet::instance()->offStyleSheet);

        setOverlayMode(this, enable?1:0);
    }

    if(!enable)
        tabBar()->setVisible(count()>1);
    else
        tabBar()->hide();

    setRect(enable?rectOverlay:rectActive, enable);
}

const QRect &OverlayTabWidget::getRect(bool overlay)
{
    if(overlay || rectActive.isNull())
        return rectOverlay;
    return rectActive;
}

bool OverlayTabWidget::getAutoHideRect(QRect &rect) const
{
    if(!overlayed || !checkAutoHide())
        return false;
    rect = rectOverlay;
    switch(tabPosition()) {
    case East:
        rect.setLeft(rect.left() + std::max(rect.width()-8,0));
        break;
    case West:
        rect.setRight(rect.right() - std::max(rect.width()-8,0));
        break;
    case North:
        rect.setBottom(rect.bottom() - std::max(rect.height()-8,0));
        break;
    case South:
        rect.setTop(rect.top() + std::max(rect.height()-8,0));
        break;
    }
    return true;
}

void OverlayTabWidget::setRect(QRect rect, bool overlay)
{
    if(rect.width()<=0 || rect.height()<=0)
        return;

    if(!overlay)
        rectActive = rect;
    else
        rectOverlay = rect;

    if(getAutoHideRect(rect)) {
        proxyWidget->setGeometry(rect);
        proxyWidget->show();
        hide();
    } else if(overlay == overlayed) {
        if(!isVisible() && count()) {
            proxyWidget->hide();
            show();
            setOverlayMode(overlay);
        }
        if(!overlay && !isTransparent())
            setGeometry(rectActive);
        else
            setGeometry(rectOverlay);
    }
}

void OverlayTabWidget::addWidget(QDockWidget *dock, const QString &title)
{
    QRect rect = dock->geometry();

    setOverlayMode(dock, 1);
    addTab(dock, title);

    DockWindowManager::instance()->setupTitleBar(dock);

    dock->setFeatures(dock->features() & ~QDockWidget::DockWidgetFloatable);
    if(count() == 1)
        setRect(rect, true);
}

void OverlayTabWidget::removeWidget(QDockWidget *dock)
{
    int index = indexOf(dock);
    if(index < 0)
        return;

    QWidget *w = dock->titleBarWidget();
    if(w && w->objectName() == QLatin1String("OverlayTitle")) {
        for(auto action : actions()) {
            auto button = w->findChild<OverlayToolButton*>(
                    action->data().toString(), Qt::FindDirectChildrenOnly);
            if(button) {
                button->setDefaultAction(nullptr);
                button->removeAction(action);
                button->hide();
            }
        }
        w->show();
    }

    dock->setFeatures(dock->features() | QDockWidget::DockWidgetFloatable);

    setOverlayMode(dock, 0);
    removeTab(index);

    if(!count())
        hide();
}

void OverlayTabWidget::setCurrent(QWidget *widget)
{
    int index = indexOf(widget);
    if(index >= 0)
        setCurrentIndex(index);
}

void OverlayTabWidget::changeSize(int changes)
{
    QRect rect = overlayed?rectOverlay:rectActive;
    switch(tabPosition()) {
    case West:
        rect.setRight(rect.right() + changes);
        break;
    case East:
        rect.setLeft(rect.left() - changes);
        break;
    case North:
        rect.setBottom(rect.bottom() + changes);
        break;
    case South:
        rect.setTop(rect.top() - changes);
        break;
    default:
        break;
    }
    setRect(rect, overlayed);
}

// -----------------------------------------------------------

struct OverlayInfo {
    const char *name;
    OverlayTabWidget *tabWidget;
    Qt::DockWidgetArea dockArea;
    QMap<QDockWidget*, OverlayInfo*> &overlayMap;
    ParameterGrp::handle hGrp;
    bool updating = false;

    OverlayInfo(QWidget *parent, const char *name, Qt::DockWidgetArea pos, QMap<QDockWidget*, OverlayInfo*> &map)
        : name(name), dockArea(pos), overlayMap(map)
    {
        tabWidget = new OverlayTabWidget(parent, dockArea);
        tabWidget->setObjectName(QString::fromLatin1(name));
        tabWidget->getProxyWidget()->setObjectName(tabWidget->objectName() + QString::fromLatin1("Proxy"));
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
        if(focus && findTabWidget(focus) != tabWidget)
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
        QDockWidget *lastDock = qobject_cast<QDockWidget*>(tabWidget->currentWidget());
        if(lastDock) {
            tabWidget->removeWidget(lastDock);
            lastDock->show();
            mw->addDockWidget(dockArea, lastDock);
        }
        while(tabWidget->count()) {
            QDockWidget *dock = qobject_cast<QDockWidget*>(tabWidget->widget(0));
            if(!dock) {
                tabWidget->removeTab(0);
                continue;
            }
            tabWidget->removeWidget(dock);
            dock->show();
            if(lastDock)
                mw->tabifyDockWidget(lastDock, dock);
            else
                mw->addDockWidget(dockArea, dock);
            lastDock = dock;
        }

        if(focus)
            focus->setFocus();
    }

    bool geometry(QRect &rect) {
        if(!tabWidget->count())
            return false;
        rect = tabWidget->getRect(tabWidget->isOverlayed());
        return true;
    }

    void setGeometry(int x, int y, int w, int h,
            int activeX, int activeY, int activeW, int activeH)
    {
        if(!tabWidget->count())
            return;
        tabWidget->setRect(QRect(x,y,w,h),true);
        tabWidget->setRect(QRect(activeX,activeY,activeW,activeH),false);
    }

    void save()
    {
        QRect rect = tabWidget->getRect(true);
        hGrp->SetInt("Width", rect.width());
        hGrp->SetInt("Height", rect.height());
        hGrp->SetInt("Active", tabWidget->currentIndex());
        hGrp->SetBool("AutoHide", tabWidget->isAutoHide());
        hGrp->SetBool("Transparent", tabWidget->isTransparent());
        hGrp->SetBool("EditHide", tabWidget->isEditHide());

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
            tabWidget->setRect(QRect(rect.left(),rect.top(),width,height), true);
        }
        int index = hGrp->GetInt("Active", -1);
        if(index >= 0)
            tabWidget->setCurrentIndex(index);
        tabWidget->setAutoHide(hGrp->GetBool("AutoHide", false));
        tabWidget->setTransparent(hGrp->GetBool("Transparent", false));
        tabWidget->setEditHide(hGrp->GetBool("EditHide", false));
    }

};

#endif // FC_HAS_DOCK_OVERLAY

enum OverlayToggleMode {
    OverlayUnset,
    OverlaySet,
    OverlayToggle,
    OverlayToggleAutoHide,
    OverlayToggleTransparent,
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
    std::array<OverlayInfo*,4> _overlayInfos;

    QAction _actClose;
    QAction _actFloat;
    QAction _actOverlay;
    std::array<QAction*, 3> _actions;

    DockWindowManagerP(DockWindowManager *host, QWidget *parent)
        :_left(parent,"OverlayLeft", Qt::LeftDockWidgetArea,_overlays)
        ,_right(parent,"OverlayRight", Qt::RightDockWidgetArea,_overlays)
        ,_top(parent,"OverlayTop", Qt::TopDockWidgetArea,_overlays)
        ,_bottom(parent,"OverlayBottom",Qt::BottomDockWidgetArea,_overlays)
        ,_overlayInfos({&_left,&_right,&_top,&_bottom})
        ,_actions({&_actOverlay,&_actFloat,&_actClose})
    {
        Application::Instance->signalActivateView.connect([this](const MDIView *) {
            _timer.start(100);
        });
        Application::Instance->signalInEdit.connect([this](const ViewProviderDocumentObject &) {
            _timer.start(100);
        });
        Application::Instance->signalResetEdit.connect([this](const ViewProviderDocumentObject &) {
            _timer.start(100);
        });

        const char * const pixmapOverlay[]={
            "10 10 2 1",
            ". c None",
            TITLE_BUTTON_COLOR,
            "##########",
            "#........#",
            "#........#",
            "##########",
            "#........#",
            "#........#",
            "#........#",
            "#........#",
            "#........#",
            "##########",
        };
        _actOverlay.setIcon(QPixmap(pixmapOverlay));
        _actOverlay.setData(QString::fromLatin1("OBTN Overlay"));

        const char * const pixmapFloat[]={
            "10 10 2 1",
            ". c None",
            TITLE_BUTTON_COLOR,
            "...#######",
            "...#.....#",
            "...#.....#",
            "#######..#",
            "#.....#..#",
            "#.....#..#",
            "#.....####",
            "#.....#...",
            "#.....#...",
            "#######...",
        };
        _actFloat.setIcon(QPixmap(pixmapFloat));
        _actFloat.setData(QString::fromLatin1("OBTN Float"));

        const char * const pixmapClose[]={
            "10 10 2 1",
            ". c None",
            TITLE_BUTTON_COLOR,
            "##......##",
            "###....###",
            ".###..###.",
            "..######..",
            "...####...",
            "...####...",
            "..######..",
            ".###..###.",
            "###....###",
            "##......##",
        };
        _actClose.setIcon(QPixmap(pixmapClose));
        _actClose.setData(QString::fromLatin1("OBTN Close"));

        retranslate();

        for(auto action : _actions) {
            QObject::connect(action, SIGNAL(triggered(bool)), host, SLOT(onAction()));
        }
        for(auto o : _overlayInfos) {
            for(auto action : o->tabWidget->actions()) {
                QObject::connect(action, SIGNAL(triggered(bool)), host, SLOT(onAction()));
            }
        }
    }

    bool toggleOverlay(QDockWidget *dock, OverlayToggleMode toggle,
            Qt::DockWidgetArea dockPos=Qt::NoDockWidgetArea)
    {
        if(!dock)
            return false;

        auto it = _overlays.find(dock);
        if(it != _overlays.end()) {
            auto o = it.value();
            switch(toggle) {
            case OverlaySet:
                o->tabWidget->setAutoHide(false);
                break;
            case OverlayToggleAutoHide:
                o->tabWidget->setAutoHide(!o->tabWidget->isAutoHide());
                break;
            case OverlayToggleTransparent:
                o->tabWidget->setTransparent(!o->tabWidget->isTransparent());
                break;
            case OverlayUnset:
            case OverlayToggle:
                _overlays.erase(it);
                o->removeWidget();
                return false;
            default:
                break;
            }
            return true;
        }

        if(toggle == OverlayUnset)
            return false;

        if(dockPos == Qt::NoDockWidgetArea)
            dockPos = getMainWindow()->dockWidgetArea(dock);
        OverlayInfo *o;
        switch(dockPos) {
        case Qt::LeftDockWidgetArea:
            o = &_left;
            break;
        case Qt::RightDockWidgetArea:
            o = &_right;
            break;
        case Qt::TopDockWidgetArea:
            o = &_top;
            break;
        case Qt::BottomDockWidgetArea:
            o = &_bottom;
            break;
        default:
            return false;
        }
        if(toggle == OverlayCheck && !o->tabWidget->count())
            return false;
        if(o->addWidget(dock)) {
            o->tabWidget->setAutoHide(toggle == OverlayToggleAutoHide);
            o->tabWidget->setTransparent(toggle == OverlayToggleTransparent);
        }
        return true;
    }

    void refreshOverlay(QWidget *widget)
    {
        if(widget) {
            auto tabWidget = findTabWidget(widget);
            if(tabWidget && tabWidget->count()) {
                for(auto o : _overlayInfos) {
                    if(tabWidget == o->tabWidget) {
                        o->updating = true;
                        break;
                    }
                }
            }
        }
        _timer.start(50);
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
        _left.restore();
        _right.restore();
        _top.restore();
        _bottom.restore();
        _timer.start(100);
    }

    void onTimer()
    {
        QMdiArea *mdi = getMainWindow()->findChild<QMdiArea*>();
        if(!mdi)
            return;

        auto focus = findTabWidget(qApp->focusWidget());
        auto active = findTabWidget(qApp->widgetAt(QCursor::pos()));

        bool updateFocus = false;
        bool updateActive = false;

        for(auto o : _overlayInfos) {
            if(o->tabWidget->count()
                    && (o->updating || OverlayStyleSheet::instance()->updating))
            {
                if(o->tabWidget == focus)
                    updateFocus = true;
                else if(o->tabWidget == active)
                    updateActive = true;
                else 
                    o->tabWidget->setOverlayMode(true);
            }
            o->updating = false;
        }
        OverlayStyleSheet::instance()->updating = false;

        if(focus && (focus->isOverlayed() || updateFocus)) {
            focus->setOverlayMode(false);
            focus->raise();
        }

        if(active) {
            if(active != focus && (active->isOverlayed() || updateActive)) 
                active->setOverlayMode(false);
            active->raise();
        }

        for(auto o : _overlayInfos) {
            if(o->tabWidget != focus 
                    && o->tabWidget != active
                    && o->tabWidget->count()
                    && !o->tabWidget->isOverlayed())
            {
                o->tabWidget->setOverlayMode(true);
            }
        }

        int w = mdi->geometry().width();
        int h = mdi->geometry().height();
        auto tabbar = mdi->findChild<QTabBar*>();
        if(tabbar)
            h -= tabbar->height();

        QRect rectBottom(0,0,0,0);
        if(_bottom.geometry(rectBottom)) {
            int bw = std::max(w-ViewParams::getNaviWidgetSize()-10, 10);
            // Bottom width is maintain the same to reduce QTextEdit re-layout
            // which may be expensive if there are lots of text, e.g. for
            // ReportView or PythonConsole.
            _bottom.setGeometry(0,h-rectBottom.height(),bw,rectBottom.height(),
                                0,h-rectBottom.height(),bw,rectBottom.height());
            _bottom.tabWidget->getAutoHideRect(rectBottom);
        }
        QRect rectLeft(0,0,0,0);
        if(_left.geometry(rectLeft)) {
            int lh = std::max(h-rectBottom.height(),10);
            _left.setGeometry(0,0,rectLeft.width(),lh, 0,0,rectLeft.width(),h);
            _left.tabWidget->getAutoHideRect(rectLeft);
        }
        QRect rectRight(0,0,0,0);
        if(_right.geometry(rectRight)) {
            int dh = std::max(rectBottom.height(), ViewParams::getNaviWidgetSize()-10);
            int rh = std::max(h-dh, 10);
            _right.setGeometry(w-rectRight.width(),0,rectRight.width(),rh,
                                w-rectRight.width(),0,rectRight.width(),rh);
            _right.tabWidget->getAutoHideRect(rectRight);
        }
        QRect rectTop(0,0,0,0);
        if(_top.geometry(rectTop)) {
            int tw = std::max(w-rectLeft.width()-rectRight.width(),10);
            _top.setGeometry(rectLeft.width(),0,tw,rectTop.height(),0,0,w,rectTop.height());
        }
    }

    void setOverlayMode(DockWindowManager::OverlayMode mode)
    {
        switch(mode) {
        case DockWindowManager::DisableAll:
        case DockWindowManager::EnableAll: {
            auto docks = getMainWindow()->findChildren<QDockWidget*>();
            // put visible dock widget first
            std::sort(docks.begin(),docks.end(),
                [](const QDockWidget *a, const QDockWidget *) {
                    return !a->visibleRegion().isEmpty();
                });
            for(auto dock : docks) {
                if(mode == DockWindowManager::DisableAll)
                    toggleOverlay(dock, OverlayUnset);
                else
                    toggleOverlay(dock, OverlaySet);
            }
            return;
        }
        case DockWindowManager::ToggleAll:
            for(auto o : _overlayInfos) {
                if(o->tabWidget->count()) {
                    setOverlayMode(DockWindowManager::DisableAll);
                    return;
                }
            }
            setOverlayMode(DockWindowManager::EnableAll);
            return;
        case DockWindowManager::AutoHideAll: {
            bool found = false;
            for(auto o : _overlayInfos) {
                if(o->tabWidget->count())
                    found = true;
            }
            if(!found)
                setOverlayMode(DockWindowManager::EnableAll);
        }
        // fall through
        case DockWindowManager::AutoHideNone:
            for(auto o : _overlayInfos)
                o->tabWidget->setAutoHide(mode == DockWindowManager::AutoHideAll);
            _timer.start(500);
            return;
        case DockWindowManager::ToggleAutoHideAll:
            for(auto o : _overlayInfos) {
                if(o->tabWidget->count() && o->tabWidget->isAutoHide()) {
                    setOverlayMode(DockWindowManager::AutoHideNone);
                    return;
                }
            }
            setOverlayMode(DockWindowManager::AutoHideAll);
            return;
        case DockWindowManager::TransparentAll: {
            bool found = false;
            for(auto o : _overlayInfos) {
                if(o->tabWidget->count())
                    found = true;
            }
            if(!found)
                setOverlayMode(DockWindowManager::EnableAll);
        }
        // fall through
        case DockWindowManager::TransparentNone:
            for(auto o : _overlayInfos)
                o->tabWidget->setTransparent(mode == DockWindowManager::TransparentAll);
            _timer.start(500);
            return;
        case DockWindowManager::ToggleTransparentAll:
            for(auto o : _overlayInfos) {
                if(o->tabWidget->count() && o->tabWidget->isTransparent()) {
                    setOverlayMode(DockWindowManager::TransparentNone);
                    return;
                }
            }
            setOverlayMode(DockWindowManager::TransparentAll);
            return;
        default:
            break;
        }

        OverlayToggleMode m;
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

        switch(mode) {
        case DockWindowManager::ToggleActive:
            m = OverlayToggle;
            break;
        case DockWindowManager::ToggleAutoHide:
            m = OverlayToggleAutoHide;
            break;
        case DockWindowManager::ToggleTransparent:
            m = OverlayToggleTransparent;
            break;
        case DockWindowManager::EnableActive:
            m = OverlaySet;
            break;
        case DockWindowManager::DisableActive:
            m = OverlayUnset;
            break;
        default:
            return;
        }
        toggleOverlay(dock, m);
    }

    void onToggleDockWidget(QDockWidget *dock, bool checked)
    {
        if(!dock)
            return;

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

    void changeOverlaySize(int changes)
    {
        auto tabWidget = findTabWidget(qApp->widgetAt(QCursor::pos()));
        if(tabWidget) {
            tabWidget->changeSize(changes);
            _timer.start(500);
        }
    }

    void onFocusChanged(QWidget *, QWidget *) {
        _timer.start(100);
    }

    void setupTitleBar(QDockWidget *dock)
    {
        int size = 12;
        auto it = _overlays.find(dock);
        OverlayTabWidget *tabWidget = it!=_overlays.end()?it.value()->tabWidget:nullptr;

        auto widget = dock->titleBarWidget();
        if(widget) {
            if(widget->objectName() != QString::fromLatin1("OverlayTitle"))
                return;

            auto layout = qobject_cast<QBoxLayout*>(widget->layout());
            if(tabWidget && layout) {
                int index = 0;
                for(auto action : tabWidget->actions()) {
                    ++index;
                    auto button = widget->findChild<OverlayToolButton*>(
                            action->data().toString(), Qt::FindDirectChildrenOnly);
                    if(button) {
                        button->setDefaultAction(action);
                        button->show();
                    } else 
                        layout->insertWidget(index,createTitleButton(action, size));
                }
            }
            return;
        }

        widget = new QWidget();
        widget->setObjectName(QLatin1String("OverlayTitle"));

        bool vertical = (dock->features() & QDockWidget::DockWidgetVerticalTitleBar);
        auto layout = new QBoxLayout(vertical?QBoxLayout::BottomToTop:QBoxLayout::LeftToRight, widget); 
        layout->setContentsMargins(1,1,1,1);
        layout->addSpacerItem(new QSpacerItem(size,size,
                    vertical?QSizePolicy::Minimum:QSizePolicy::Expanding,
                    vertical?QSizePolicy::Expanding:QSizePolicy::Minimum));

        if(tabWidget) {
            for(auto action : tabWidget->actions())
                layout->addWidget(createTitleButton(action, size));
        }
        for(auto action : _actions)
            layout->addWidget(createTitleButton(action, size));

        dock->setTitleBarWidget(widget);
    }

    QWidget *createTitleButton(QAction *action, int size)
    {
        auto button = new OverlayToolButton(nullptr);
        button->setObjectName(action->data().toString());
        button->setDefaultAction(action);
        button->setAutoRaise(true);
        button->setContentsMargins(0,0,0,0);
        button->setFixedSize(size,size);
        return button;
    }

    void onAction(QAction *action) {
        if(action == &_actOverlay) {
            DockWindowManager::instance()->setOverlayMode(DockWindowManager::ToggleActive);
        } else if(action == &_actFloat || action == &_actClose) {
            for(auto w=qApp->widgetAt(QCursor::pos());w;w=w->parentWidget()) {
                auto dock = qobject_cast<QDockWidget*>(w);
                if(!dock)
                    continue;
                if(action == &_actClose) {
                    dock->toggleViewAction()->activate(QAction::Trigger);
                } else {
                    auto it = _overlays.find(dock);
                    if(it != _overlays.end()) {
                        it.value()->tabWidget->removeWidget(dock);
                        getMainWindow()->addDockWidget(it.value()->dockArea, dock);
                        _overlays.erase(it);
                        dock->show();
                        dock->setFloating(true);
                        refreshOverlay(nullptr);
                    } else 
                        dock->setFloating(!dock->isFloating());
                }
                return;
            }
        } else {
            auto tabWidget = qobject_cast<OverlayTabWidget*>(action->parent());
            if(tabWidget)
                tabWidget->onAction(action);
        }
    }

    void retranslate()
    {
        _actOverlay.setToolTip(QObject::tr("Toggle overlay"));
        _actFloat.setToolTip(QObject::tr("Toggle floating window"));
        _actClose.setToolTip(QObject::tr("Close dock window"));
    }

#else // FC_HAS_DOCK_OVERLAY

    DockWindowManagerP(DockWindowManager *, QWidget *) {}
    void refreshOverlay(QWidget *) {}
    void saveOverlay() {}
    void restoreOverlay() {}
    void onTimer() {}
    void setOverlayMode(DockWindowManager::OverlayMode) {}
    void onToggleDockWidget(QDockWidget *, bool) {}
    void changeOverlaySize(int) {}
    void onFocusChanged(QWidget *, QWidget *) {}
    void onAction(QAction *) {}
    void setupTitleBar(QDockWidget *) {}
    void retranslate() {}

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
    d = new DockWindowManagerP(this,mdi);
    connect(&d->_timer, SIGNAL(timeout()), this, SLOT(onTimer()));
    d->_timer.setSingleShot(true);

    connect(qApp, SIGNAL(focusChanged(QWidget*,QWidget*)),
            this, SLOT(onFocusChanged(QWidget*,QWidget*)));
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
    setupTitleBar(dw);

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
    d->retranslate();
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

void DockWindowManager::onTimer()
{
    d->onTimer();
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

void DockWindowManager::changeOverlaySize(int changes)
{
    d->changeOverlaySize(changes);
}

void DockWindowManager::onFocusChanged(QWidget *old, QWidget *now)
{
    d->onFocusChanged(old, now);
}

void DockWindowManager::setupTitleBar(QDockWidget *dock)
{
    d->setupTitleBar(dock);
}

void DockWindowManager::onAction()
{
    QAction *action = qobject_cast<QAction*>(sender());
    if(action)
        d->onAction(action);
}

#include "moc_DockWindowManager.cpp"
