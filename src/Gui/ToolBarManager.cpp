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
#include "Selection.h"
#include <boost/signals2/connection.hpp>
#ifndef _PreComp_
# include <QAction>
# include <QApplication>
# include <QHBoxLayout>
# include <QMenuBar>
# include <QMouseEvent>
# include <QPointer>
# include <QStatusBar>
# include <QToolBar>
# include <QToolButton>
#endif

#include <boost/algorithm/string/predicate.hpp>

#include <Base/Tools.h>

#include "ToolBarManager.h"
#include "Application.h"
#include "Command.h"
#include "MainWindow.h"
#include "OverlayWidgets.h"


using namespace Gui;

ToolBarItem::ToolBarItem() : visibilityPolicy(DefaultVisibility::Visible)
{
}

ToolBarItem::ToolBarItem(ToolBarItem* item, DefaultVisibility visibilityPolicy) : visibilityPolicy(visibilityPolicy)
{
    if (item) {
        item->appendItem(this);
    }
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

bool ToolBarItem::hasItems() const
{
    return _items.count() > 0;
}

ToolBarItem* ToolBarItem::findItem(const std::string& name)
{
    if ( _name == name ) {
        return this;
    }

    for (auto it : std::as_const(_items)) {
        if (it->_name == name) {
            return it;
        }
    }

    return nullptr;
}

ToolBarItem* ToolBarItem::copy() const
{
    auto root = new ToolBarItem;
    root->setCommand( command() );

    QList<ToolBarItem*> items = getItems();
    for (auto it : items) {
        root->appendItem(it->copy());
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
    }

    return false;
}

void ToolBarItem::removeItem(ToolBarItem* item)
{
    int pos = _items.indexOf(item);
    if (pos != -1) {
        _items.removeAt(pos);
    }
}

void ToolBarItem::clear()
{
    for (auto it : std::as_const(_items)) {
        delete it;
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
    auto item = new ToolBarItem(this);
    item->setCommand(command);
    return *this;
}

QList<ToolBarItem*> ToolBarItem::getItems() const
{
    return _items;
}

// -----------------------------------------------------------

namespace Gui {

class ToolBarAreaWidget : public QWidget
{
    using inherited = QWidget;

public:
    ToolBarAreaWidget(QWidget *parent,
                ToolBarArea area,
                const ParameterGrp::handle& hParam,
                boost::signals2::scoped_connection &conn,
                QTimer *timer = nullptr)
        : QWidget(parent)
        , _sizingTimer(timer)
        , _hParam(hParam)
        , _conn(conn)
        , _area(area)
    {
        _layout = new QHBoxLayout(this);
        _layout->setContentsMargins(QMargins());
    }

    void addWidget(QWidget *widget)
    {
        // if widget already exist don't do anything
        if (_layout->indexOf(widget) >= 0) {
            return;
        }

        _layout->addWidget(widget);
        adjustParent();

        QString name = widget->objectName();

        if (!name.isEmpty()) {
            Base::ConnectionBlocker block(_conn);
            _hParam->SetInt(widget->objectName().toUtf8().constData(), _layout->count() - 1);
        }
    }

    void insertWidget(int index, QWidget *widget)
    {
        int currentIndex = _layout->indexOf(widget);

        // we are inserting widget at the same place, this is no-op
        if (currentIndex == index) {
            return;
        }

        // widget already exists in the area, we need to first remove it and then recreate
        if (currentIndex > 0) {
            _layout->removeWidget(widget);
        }

        _layout->insertWidget(index, widget);

        adjustParent();
        saveState();
    }

    void adjustParent()
    {
        if (_sizingTimer) {
            _sizingTimer->start(10);
        }
    }

    void removeWidget(QWidget *widget)
    {
        _layout->removeWidget(widget);

        QString name = widget->objectName();
        if (!name.isEmpty()) {
            Base::ConnectionBlocker block(_conn);
            _hParam->RemoveInt(name.toUtf8().constData());
        }

        adjustParent();
    }

    QWidget *widgetAt(int index) const
    {
        auto item = _layout->itemAt(index);

        return item ? item->widget() : nullptr;
    }

    int count() const
    {
        return _layout->count();
    }

    int indexOf(QWidget *widget) const
    {
        return _layout->indexOf(widget);
    }

    ToolBarArea area() const
    {
        return _area;
    }

    template<class FuncT>
    void foreachToolBar(FuncT &&func)
    {
        for (int i = 0, c = _layout->count(); i < c; ++i) {
            auto toolbar = qobject_cast<QToolBar*>(widgetAt(i));

            if (!toolbar || toolbar->objectName().isEmpty()
                         || toolbar->objectName().startsWith(QLatin1String("*"))) {
                continue;
            }

            func(toolbar, i, this);
        }
    }

    void saveState()
    {
        Base::ConnectionBlocker block(_conn);

        for (auto &v : _hParam->GetIntMap()) {
            _hParam->RemoveInt(v.first.c_str());
        }

        foreachToolBar([this](QToolBar *toolbar, int idx, ToolBarAreaWidget*) {
            _hParam->SetInt(toolbar->objectName().toUtf8().constData(), idx);
        });
    }

    void restoreState(const std::map<int, QToolBar*> &toolbars)
    {
        for (const auto &[index, toolbar] : toolbars) {
            bool visible = toolbar->isVisible();
            getMainWindow()->removeToolBar(toolbar);
            toolbar->setOrientation(Qt::Horizontal);
            insertWidget(index, toolbar);
            toolbar->setVisible(visible);
        }

        for (const auto &[name, visible] : _hParam->GetBoolMap()) {
            auto widget = findChild<QWidget*>(QString::fromUtf8(name.c_str()));

            if (widget) {
                widget->setVisible(visible);
            }
        }
    }

private:
    QHBoxLayout *_layout;
    QPointer<QTimer> _sizingTimer;
    ParameterGrp::handle _hParam;
    boost::signals2::scoped_connection &_conn;
    ToolBarArea _area;
};

} // namespace Gui

// -----------------------------------------------------------

ToolBarManager* ToolBarManager::_instance = nullptr;  // NOLINT

ToolBarManager* ToolBarManager::getInstance()
{
    if (!_instance) {
        _instance = new ToolBarManager;
    }
    return _instance;
}

void ToolBarManager::destruct()
{
    delete _instance;
    _instance = nullptr;
}

ToolBarManager::ToolBarManager()
{
    setupParameters();
    setupStatusBar();
    setupMenuBar();

    setupSizeTimer();
    setupResizeTimer();
    setupConnection();
    setupTimer();
    setupMenuBarTimer();
}

ToolBarManager::~ToolBarManager() = default;

void ToolBarManager::setupParameters()
{
    auto& mgr = App::GetApplication().GetUserParameter();
    hGeneral = mgr.GetGroup("BaseApp/Preferences/General");
    hStatusBar = mgr.GetGroup("BaseApp/MainWindow/StatusBar");
    hMenuBarRight = mgr.GetGroup("BaseApp/MainWindow/MenuBarRight");
    hMenuBarLeft = mgr.GetGroup("BaseApp/MainWindow/MenuBarLeft");
    hPref = mgr.GetGroup("BaseApp/MainWindow/Toolbars");
}

void ToolBarManager::setupStatusBar()
{
    if (auto sb = getMainWindow()->statusBar()) {
        sb->installEventFilter(this);
        statusBarAreaWidget = new ToolBarAreaWidget(sb, ToolBarArea::StatusBarToolBarArea, hStatusBar, connParam);
        statusBarAreaWidget->setObjectName(QLatin1String("StatusBarArea"));
        sb->insertPermanentWidget(2, statusBarAreaWidget);
        statusBarAreaWidget->show();
    }
}

void ToolBarManager::setupMenuBar()
{
    if (auto mb = getMainWindow()->menuBar()) {
        mb->installEventFilter(this);
        menuBarLeftAreaWidget = new ToolBarAreaWidget(mb, ToolBarArea::LeftMenuToolBarArea, hMenuBarLeft, connParam, &menuBarTimer);
        menuBarLeftAreaWidget->setObjectName(QLatin1String("MenuBarLeftArea"));
        mb->setCornerWidget(menuBarLeftAreaWidget, Qt::TopLeftCorner);
        menuBarLeftAreaWidget->show();
        menuBarRightAreaWidget = new ToolBarAreaWidget(mb, ToolBarArea::RightMenuToolBarArea, hMenuBarRight, connParam, &menuBarTimer);
        menuBarRightAreaWidget->setObjectName(QLatin1String("MenuBarRightArea"));
        mb->setCornerWidget(menuBarRightAreaWidget, Qt::TopRightCorner);
        menuBarRightAreaWidget->show();
    }
}

void ToolBarManager::setupConnection()
{
    auto refreshParams = [this](const char *name) {
        bool sizeChanged = false;
        if (!name || boost::equals(name, "ToolbarIconSize")) {
            _toolBarIconSize = hGeneral->GetInt("ToolbarIconSize", 24);
            sizeChanged = true;
        }
        if (!name || boost::equals(name, "StatusBarIconSize")) {
            _statusBarIconSize = hGeneral->GetInt("StatusBarIconSize", 0);
            sizeChanged = true;
        }
        if (!name || boost::equals(name, "MenuBarIconSize")) {
            _menuBarIconSize = hGeneral->GetInt("MenuBarIconSize", 0);
            sizeChanged = true;
        }
        if (sizeChanged) {
            sizeTimer.start(100);
        }
    };

    refreshParams(nullptr);
    connParam = App::GetApplication().GetUserParameter().signalParamChanged.connect(
        [this, refreshParams](ParameterGrp *hParam, ParameterGrp::ParamType, const char *name, const char *) {
            if (hParam == hGeneral && name) {
                refreshParams(name);
            }
            if (hParam == hPref
                    || hParam == hStatusBar
                    || hParam == hMenuBarRight
                    || hParam == hMenuBarLeft) {
                timer.start(100);
            }
        });
}

void ToolBarManager::setupTimer()
{
    timer.setSingleShot(true);
    connect(&timer, &QTimer::timeout, [this]{
        onTimer();
    });
}

void ToolBarManager::setupSizeTimer()
{
    sizeTimer.setSingleShot(true);
    connect(&sizeTimer, &QTimer::timeout, [this]{
        setupToolBarIconSize();
    });
}

void ToolBarManager::setupResizeTimer()
{
    resizeTimer.setSingleShot(true);
    connect(&resizeTimer, &QTimer::timeout, [this]{
        for (const auto &[toolbar, guard] : resizingToolbars) {
            if (guard) {
                setToolBarIconSize(toolbar);
            }
        }
        resizingToolbars.clear();
    });
}

void ToolBarManager::setupMenuBarTimer()
{
    menuBarTimer.setSingleShot(true);
    QObject::connect(&menuBarTimer, &QTimer::timeout, [] {
        if (auto menuBar = getMainWindow()->menuBar()) {
            menuBar->adjustSize();
        }
    });
}

ToolBarArea ToolBarManager::toolBarArea(QWidget *widget) const
{
    if (auto toolBar = qobject_cast<QToolBar*>(widget)) {
        if (toolBar->isFloating()) {
            return ToolBarArea::NoToolBarArea;
        }

        auto qtToolBarArea = getMainWindow()->toolBarArea(toolBar);
        switch (qtToolBarArea) {
            case Qt::LeftToolBarArea:
                return ToolBarArea::LeftToolBarArea;
            case Qt::RightToolBarArea:
                return ToolBarArea::RightToolBarArea;
            case Qt::TopToolBarArea:
                return ToolBarArea::TopToolBarArea;
            case Qt::BottomToolBarArea:
                return ToolBarArea::BottomToolBarArea;
            default:
                // no-op
                break;
        }
    }

    for (auto &areaWidget : { statusBarAreaWidget, menuBarLeftAreaWidget, menuBarRightAreaWidget }) {
        if (areaWidget->indexOf(widget) >= 0) {
            return areaWidget->area();
        }
    }

    return ToolBarArea::NoToolBarArea;
}

namespace {
QPointer<QWidget> createActionWidget()
{
    static QPointer<QWidget> actionWidget;
    if (!actionWidget) {
        actionWidget = new QWidget(getMainWindow());
        actionWidget->setObjectName(QLatin1String("_fc_action_widget_"));
        /* TODO This is a temporary hack until a longterm solution
        is found, thanks to @realthunder for this pointer.
        Although actionWidget has zero size, it somehow has a
        'phantom' size without any visible content and will block the top
        left tool buttons and menus of the application main window.
        Therefore it is moved out of the way. */
        actionWidget->move(QPoint(-100,-100));
    }
    else {
        auto actions = actionWidget->actions();
        for (auto action : actions) {
            actionWidget->removeAction(action);
        }
    }

    return actionWidget;
}
}

int ToolBarManager::toolBarIconSize(QWidget *widget) const
{
    int s = _toolBarIconSize;
    if (widget) {
        if (widget->parentWidget() == statusBarAreaWidget) {
            if (_statusBarIconSize > 0) {
                s = _statusBarIconSize;
            }
            else {
                s *= 0.6;
            }
        }
        else if (widget->parentWidget() == menuBarLeftAreaWidget
                || widget->parentWidget() == menuBarRightAreaWidget) {
            if (_menuBarIconSize > 0) {
                s = _menuBarIconSize;
            }
            else {
                s *= 0.6;
            }
        }
    }
    return std::max(s, 5);
}

void ToolBarManager::setupToolBarIconSize()
{
    int s = toolBarIconSize();
    getMainWindow()->setIconSize(QSize(s, s));
    // Most of the the toolbar will have explicit icon size, so the above call
    // to QMainWindow::setIconSize() will have no effect. We need to explicitly
    // change the icon size.
    QList<QToolBar*> bars = getMainWindow()->findChildren<QToolBar*>();
    for (auto toolbar : std::as_const(bars)) {
        setToolBarIconSize(toolbar);
    }
}

void ToolBarManager::setToolBarIconSize(QToolBar *toolbar)
{
    int s = toolBarIconSize(toolbar);
    toolbar->setIconSize(QSize(s, s));
    if (toolbar->parentWidget() == menuBarLeftAreaWidget) {
        menuBarLeftAreaWidget->adjustParent();
    }
    else if (toolbar->parentWidget() == menuBarRightAreaWidget) {
        menuBarRightAreaWidget->adjustParent();
    }
}

void ToolBarManager::setup(ToolBarItem* toolBarItems)
{
    if (!toolBarItems) {
        return; // empty menu bar
    }

    QPointer<QWidget> actionWidget = createActionWidget();

    saveState();
    this->toolbarNames.clear();

    int max_width = getMainWindow()->width();
    int top_width = 0;

    bool nameAsToolTip = App::GetApplication().GetUserParameter().GetGroup("BaseApp")
            ->GetGroup("Preferences")->GetGroup("MainWindow")->GetBool("ToolBarNameAsToolTip",true);
    QList<ToolBarItem*> items = toolBarItems->getItems();
    QList<QToolBar*> toolbars = toolBars();
    for (ToolBarItem* it : items) {
        // search for the toolbar
        QString name = QString::fromUtf8(it->command().c_str());
        this->toolbarNames << name;
        QToolBar* toolbar = findToolBar(toolbars, name);
        std::string toolbarName = it->command();
        bool toolbar_added = false;

        if (!toolbar) {
            toolbar = getMainWindow()->addToolBar(
                QApplication::translate("Workbench",
                                        toolbarName.c_str())); // i18n
            toolbar->setObjectName(name);
            if (nameAsToolTip){
                auto tooltip = QChar::fromLatin1('[')
                    + QApplication::translate("Workbench", toolbarName.c_str())
                    + QChar::fromLatin1(']');
                toolbar->setToolTip(tooltip);
            }
            toolbar_added = true;
        }
        else {
            int index = toolbars.indexOf(toolbar);
            toolbars.removeAt(index);
        }

        bool visible = false;

        // If visibility policy is custom, the toolbar is initialised as not visible, and the
        // toggleViewAction to control its visibility is not visible either.
        //
        // Both are managed under the responsibility of the client code
        if (it->visibilityPolicy != ToolBarItem::DefaultVisibility::Unavailable) {
            bool defaultvisibility = it->visibilityPolicy == ToolBarItem::DefaultVisibility::Visible;

            visible = hPref->GetBool(toolbarName.c_str(), defaultvisibility);

            // Enable automatic handling of visibility via, for example, (contextual) menu
            toolbar->toggleViewAction()->setVisible(true);
        }
        else { // ToolBarItem::DefaultVisibility::Unavailable
            // Prevent that the action to show/hide a toolbar appears on the (contextual) menus.
            // This is also managed by the client code for a toolbar with custom policy
            toolbar->toggleViewAction()->setVisible(false);
        }

        // Initialise toolbar item visibility
        toolbar->setVisible(visible);

        // Store item visibility policy within the action
        QAction* toggle = toolbar->toggleViewAction();
        toggle->setProperty("DefaultVisibility", static_cast<int>(it->visibilityPolicy));

        // setup the toolbar
        setup(it, toolbar);
        auto actions = toolbar->actions();
        for (auto action : actions) {
            actionWidget->addAction(action);
        }

        // try to add some breaks to avoid to have all toolbars in one line
        if (toolbar_added) {
            if (top_width > 0 && getMainWindow()->toolBarBreak(toolbar)) {
                top_width = 0;
            }

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
    for (QToolBar* it : std::as_const(toolbars)) {
        // make sure that the main window has the focus when hiding the toolbar with
        // the combo box inside
        QWidget *fw = QApplication::focusWidget();
        while (fw &&  !fw->isWindow()) {
            if (fw == it) {
                getMainWindow()->setFocus();
                break;
            }
            fw = fw->parentWidget();
        }
        // ignore toolbars which do not belong to the previously active workbench
        //QByteArray toolbarName = it->objectName().toUtf8();
        if (!it->toggleViewAction()->isVisible()) {
            continue;
        }
        //hPref->SetBool(toolbarName.constData(), it->isVisible());
        it->hide();
        it->toggleViewAction()->setVisible(false);
    }

    setMovable(!areToolBarsLocked());
}

void ToolBarManager::setup(ToolBarItem* item, QToolBar* toolbar) const
{
    CommandManager& mgr = Application::Instance->commandManager();
    QList<ToolBarItem*> items = item->getItems();
    QList<QAction*> actions = toolbar->actions();
    for (ToolBarItem* it : items) {
        // search for the action item
        QAction* action = findAction(actions, QString::fromLatin1(it->command().c_str()));
        if (!action) {
            if (it->command() == "Separator") {
                action = toolbar->addSeparator();
            }
            else {
                // Check if action was added successfully
                if (mgr.addTo(it->command().c_str(), toolbar)) {
                    action = toolbar->actions().constLast();
                }
            }

            // set the tool button user data
            if (action) {
                action->setData(QString::fromLatin1(it->command().c_str()));
            }
        }
        else {
            // Note: For toolbars we do not remove and re-add the actions
            // because this causes flicker effects. So, it could happen that the order of
            // buttons doesn't match with the order of commands in the workbench.
            int index = actions.indexOf(action);
            actions.removeAt(index);
        }
    }

    // remove all tool buttons which we don't need for the moment
    for (QAction* it : std::as_const(actions)) {
        toolbar->removeAction(it);
    }
}

void ToolBarManager::onTimer()
{
    restoreState();
}

void ToolBarManager::saveState() const
{
    auto ignoreSave = [](QAction* action) {
        // If the toggle action is invisible then it's controlled by the application.
        // In this case the current state is not saved.
        if (!action->isVisible()) {
            return true;
        }

        QVariant property = action->property("DefaultVisibility");
        if (property.isNull()) {
            return false;
        }

        // If DefaultVisibility is Unavailable then never save the state because it's
        // always controlled by the client code.
        auto value = static_cast<ToolBarItem::DefaultVisibility>(property.toInt());
        return value == ToolBarItem::DefaultVisibility::Unavailable;
    };

    QList<QToolBar*> toolbars = toolBars();
    for (const QString& it : toolbarNames) {
        QToolBar* toolbar = findToolBar(toolbars, it);
        if (toolbar) {
            if (ignoreSave(toolbar->toggleViewAction())) {
                continue;
            }

            QByteArray toolbarName = toolbar->objectName().toUtf8();
            hPref->SetBool(toolbarName.constData(), toolbar->isVisible());
        }
    }
}

void ToolBarManager::restoreState() const
{
    std::map<int, QToolBar*> sbToolBars;
    std::map<int, QToolBar*> mbRightToolBars;
    std::map<int, QToolBar*> mbLeftToolBars;
    QList<QToolBar*> toolbars = toolBars();
    for (const QString& it : toolbarNames) {
        QToolBar* toolbar = findToolBar(toolbars, it);
        if (toolbar) {
            QByteArray toolbarName = toolbar->objectName().toUtf8();
            if (getToolbarPolicy(toolbar) != ToolBarItem::DefaultVisibility::Unavailable) {
                toolbar->setVisible(hPref->GetBool(toolbarName.constData(), toolbar->isVisible()));
            }

            int idx = hStatusBar->GetInt(toolbarName, -1);
            if (idx >= 0) {
                sbToolBars[idx] = toolbar;
                continue;
            }
            idx = hMenuBarLeft->GetInt(toolbarName, -1);
            if (idx >= 0) {
                mbLeftToolBars[idx] = toolbar;
                continue;
            }
            idx = hMenuBarRight->GetInt(toolbarName, -1);
            if (idx >= 0) {
                mbRightToolBars[idx] = toolbar;
                continue;
            }
            if (toolbar->parentWidget() != getMainWindow()) {
                getMainWindow()->addToolBar(toolbar);
            }
        }
    }

    setMovable(!areToolBarsLocked());

    statusBarAreaWidget->restoreState(sbToolBars);
    menuBarRightAreaWidget->restoreState(mbRightToolBars);
    menuBarLeftAreaWidget->restoreState(mbLeftToolBars);
}

bool ToolBarManager::addToolBarToArea(QObject *source, QMouseEvent *ev)
{
    auto statusBar = getMainWindow()->statusBar();
    if (!statusBar || !statusBar->isVisible()) {
        statusBar = nullptr;
    }

    auto menuBar = getMainWindow()->menuBar();
    if (!menuBar || !menuBar->isVisible()) {
        if (!statusBar) {
            return false;
        }
        menuBar = nullptr;
    }

    auto tb = qobject_cast<QToolBar*>(source);
    if (!tb || !tb->isFloating()) {
        return false;
    }

    static QPointer<OverlayDragFrame> tbPlaceholder;
    static QPointer<ToolBarAreaWidget> lastArea;
    static int tbIndex = -1;
    if (ev->type() == QEvent::MouseMove) {
        if (tb->orientation() != Qt::Horizontal
            || ev->buttons() != Qt::LeftButton) {
            if (tbIndex >= 0) {
                if (lastArea) {
                    lastArea->removeWidget(tbPlaceholder);
                    lastArea = nullptr;
                }
                tbPlaceholder->hide();
                tbIndex = -1;
            }
            return false;
        }
    }

    if (ev->type() == QEvent::MouseButtonRelease && ev->button() != Qt::LeftButton) {
        return false;
    }

    QPoint pos = QCursor::pos();
    ToolBarAreaWidget *area = nullptr;
    if (statusBar) {
        QRect rect(statusBar->mapToGlobal(QPoint(0,0)), statusBar->size());
        if (rect.contains(pos)) {
            area = statusBarAreaWidget;
        }
    }
    if (!area) {
        if (!menuBar) {
            return false;
        }
        QRect rect(menuBar->mapToGlobal(QPoint(0,0)), menuBar->size());
        if (rect.contains(pos)) {
            if (pos.x() - rect.left() < menuBar->width()/2) {
                area = menuBarLeftAreaWidget;
            }
            else {
                area = menuBarRightAreaWidget;
            }
        }
        else {
            if (tbPlaceholder) {
                if (lastArea) {
                    lastArea->removeWidget(tbPlaceholder);
                    lastArea = nullptr;
                }
                tbPlaceholder->hide();
                tbIndex = -1;
            }
            return false;
        }
    }

    int idx = 0;
    for (int c = area->count(); idx < c ;++idx) {
        auto widget = area->widgetAt(idx);
        if (!widget || widget->isHidden()) {
            continue;
        }
        int p = widget->mapToGlobal(widget->rect().center()).x();
        if (pos.x() < p) {
            break;
        }
    }
    if (tbIndex >= 0 && tbIndex == idx-1) {
        idx = tbIndex;
    }
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
            area->insertWidget(idx, tbPlaceholder);
            lastArea = area;
            tbPlaceholder->adjustSize();
            tbPlaceholder->show();
        }
    } else {
        tbIndex = idx;
        QTimer::singleShot(10, tb, [tb]() {
            if (!lastArea) {
                return;
            }

            {
                tbPlaceholder->hide();
                QSignalBlocker block(tb);
                lastArea->removeWidget(tbPlaceholder);
                getMainWindow()->removeToolBar(tb);
                tb->setOrientation(Qt::Horizontal);
                lastArea->insertWidget(tbIndex, tb);
                tb->setVisible(true);
                lastArea = nullptr;
            }

            Q_EMIT tb->topLevelChanged(false);
            tbIndex = -1;
        });
    }
    return false;
}

void ToolBarManager::populateUndockMenu(QMenu *menu, ToolBarAreaWidget *area)
{
    menu->setTitle(tr("Undock toolbars"));
    auto tooltip = QObject::tr("Undock from toolbar area");

    auto addMenuUndockItem = [&](QToolBar *toolbar, int, ToolBarAreaWidget *area) {
        auto toggleViewAction = toolbar->toggleViewAction();
        auto undockAction = new QAction(menu);

        undockAction->setText(toggleViewAction->text());
        undockAction->setToolTip(tooltip);

        menu->addAction(undockAction);
        QObject::connect(undockAction, &QAction::triggered, [area, toolbar]() {
            if (toolbar->parentWidget() == getMainWindow()) {
                return;
            }

            auto pos = toolbar->mapToGlobal(QPoint(0, 0));
            auto yOffset = toolbar->height();

            // if widget is on the bottom move it up instead
            if (area->area() == Gui::ToolBarArea::StatusBarToolBarArea) {
                yOffset *= -1;
            }

            {
                // Block signals caused by manually floating the widget
                QSignalBlocker blocker(toolbar);

                area->removeWidget(toolbar);
                getMainWindow()->addToolBar(toolbar);

                // this will make toolbar floating, there is no better way to do that.
                toolbar->setWindowFlags(Qt::Tool
                        | Qt::FramelessWindowHint
                        | Qt::X11BypassWindowManagerHint);
                toolbar->move(pos.x(), pos.y() + yOffset);
                toolbar->adjustSize();
                toolbar->setVisible(true);
            }

            // but don't block actual information about widget being floated
            Q_EMIT toolbar->topLevelChanged(true);
        });
    };

    if (area) { 
        area->foreachToolBar(addMenuUndockItem);
    }
    else {
        statusBarAreaWidget->foreachToolBar(addMenuUndockItem);
        menuBarLeftAreaWidget->foreachToolBar(addMenuUndockItem);
        menuBarRightAreaWidget->foreachToolBar(addMenuUndockItem);
    }
}

bool ToolBarManager::showContextMenu(QObject *source)
{
    QMenu menu;
    QMenu menuUndock;
    QLayout* layout = nullptr;
    ToolBarAreaWidget* area = nullptr;
    if (getMainWindow()->statusBar() == source) {
        area = statusBarAreaWidget;
        layout = findLayoutOfObject(source, area);
    }
    else if (getMainWindow()->menuBar() == source) {
        area = findToolBarAreaWidget();
        if (!area) {
            return false;
        }
    }
    else {
        return false;
    }

    auto addMenuVisibleItem = [&](QToolBar *toolbar, int, ToolBarAreaWidget *) {
        auto action = toolbar->toggleViewAction();
        if ((action->isVisible() || toolbar->isVisible()) && action->text().size()) {
            action->setVisible(true);
            menu.addAction(action);
        }
    };

    if (layout) {
        addToMenu(layout, area, &menu);
    }

    area->foreachToolBar(addMenuVisibleItem);
    populateUndockMenu(&menuUndock, area);

    if (!menuUndock.actions().empty()) {
        menu.addSeparator();
        menu.addMenu(&menuUndock);
    }
    menu.exec(QCursor::pos());
    return true;
}

QLayout* ToolBarManager::findLayoutOfObject(QObject* source, QWidget* area) const
{
    QLayout* layout = nullptr;
    auto layouts = source->findChildren<QHBoxLayout*>();
    for (auto l : std::as_const(layouts)) {
        if (l->indexOf(area) >= 0) {
            layout = l;
            break;
        }
    }
    return layout;
}

ToolBarAreaWidget* ToolBarManager::findToolBarAreaWidget() const
{
    ToolBarAreaWidget* area = nullptr;

    QPoint pos = QCursor::pos();
    QRect rect(menuBarLeftAreaWidget->mapToGlobal(QPoint(0,0)), menuBarLeftAreaWidget->size());
    if (rect.contains(pos)) {
        area = menuBarLeftAreaWidget;
    }
    else {
        rect = QRect(menuBarRightAreaWidget->mapToGlobal(QPoint(0,0)), menuBarRightAreaWidget->size());
        if (rect.contains(pos)) {
            area = menuBarRightAreaWidget;
        }
    }

    return area;
}

void ToolBarManager::addToMenu(QLayout* layout, QWidget* area, QMenu* menu)
{
    for (int i = 0, c = layout->count(); i < c; ++i) {
        auto widget = layout->itemAt(i)->widget();
        if (!widget || widget == area
            || widget->objectName().isEmpty()
            || widget->objectName().startsWith(QLatin1String("*")))
        {
            continue;
        }
        QString name = widget->windowTitle();
        if (name.isEmpty()) {
            name = widget->objectName();
            name.replace(QLatin1Char('_'), QLatin1Char(' '));
            name = name.simplified();
        }

        auto action = new QAction(menu);
        action->setText(name);
        action->setCheckable(true);
        action->setChecked(widget->isVisible());
        menu->addAction(action);

        auto onToggle = [widget, this](bool visible) {
            onToggleStatusBarWidget(widget, visible);
        };
        QObject::connect(action, &QAction::triggered, onToggle);
    }
}

void ToolBarManager::onToggleStatusBarWidget(QWidget *widget, bool visible)
{
    Base::ConnectionBlocker block(connParam);
    widget->setVisible(visible);
    hStatusBar->SetBool(widget->objectName().toUtf8().constData(), widget->isVisible());
}

bool ToolBarManager::eventFilter(QObject *source, QEvent *ev)
{
    bool res = false;
    switch(ev->type()) {
    case QEvent::Show:
    case QEvent::Hide:
        if (auto toolbar = qobject_cast<QToolBar*>(source)) {
            auto parent = toolbar->parentWidget();
            if (parent == menuBarLeftAreaWidget || parent == menuBarRightAreaWidget) {
                menuBarTimer.start(10);
            }
        }
        break;
    case QEvent::MouseButtonRelease: {
        auto mev = static_cast<QMouseEvent*>(ev);
        if (mev->button() == Qt::RightButton) {
            if (showContextMenu(source)) {
                return true;
            }
        }
    }
    // fall through
    case QEvent::MouseMove:
        res = addToolBarToArea(source, static_cast<QMouseEvent*>(ev));
        break;
    case QEvent::ParentChange:
        if (auto toolbar = qobject_cast<QToolBar*>(source)) {
            resizingToolbars[toolbar] = toolbar;
            resizeTimer.start(100);
        }
        break;
    default:
        break;
    }
    return res;
}

void ToolBarManager::retranslate() const
{
    QList<QToolBar*> toolbars = toolBars();
    for (QToolBar* it : toolbars) {
        QByteArray toolbarName = it->objectName().toUtf8();
        it->setWindowTitle(QApplication::translate("Workbench", (const char*)toolbarName));
    }
}

bool Gui::ToolBarManager::areToolBarsLocked() const
{
    return hGeneral->GetBool("LockToolBars", false);
}

void Gui::ToolBarManager::setToolBarsLocked(bool locked) const
{
    hGeneral->SetBool("LockToolBars", locked);

    setMovable(!locked);
}

void Gui::ToolBarManager::setMovable(bool moveable) const
{
    for (auto& tb : toolBars()) {
        tb->setMovable(moveable);
    }
}

QToolBar* ToolBarManager::findToolBar(const QList<QToolBar*>& toolbars, const QString& item) const
{
    for (QToolBar* it : toolbars) {
        if (it->objectName() == item) {
            return it;
        }
    }

    return nullptr; // no item with the user data found
}

QAction* ToolBarManager::findAction(const QList<QAction*>& acts, const QString& item) const
{
    for (QAction* it : acts) {
        if (it->data().toString() == item) {
            return it;
        }
    }

    return nullptr; // no item with the user data found
}

QList<QToolBar*> ToolBarManager::toolBars() const
{
    auto mw = getMainWindow();
    QList<QToolBar*> tb;
    QList<QToolBar*> bars = getMainWindow()->findChildren<QToolBar*>();
    for (QToolBar* it : bars) {
        auto parent = it->parentWidget();
        if (parent == mw
                || parent == mw->statusBar()
                || parent == statusBarAreaWidget
                || parent == menuBarLeftAreaWidget
                || parent == menuBarRightAreaWidget) {
            tb.push_back(it);
            it->installEventFilter(const_cast<ToolBarManager*>(this));
        }
    }

    return tb;
}

ToolBarItem::DefaultVisibility ToolBarManager::getToolbarPolicy(const QToolBar* toolbar) const
{
    auto* action = toolbar->toggleViewAction();

    QVariant property = action->property("DefaultVisibility");
    if (property.isNull()) {
        return ToolBarItem::DefaultVisibility::Visible;
    }

    return static_cast<ToolBarItem::DefaultVisibility>(property.toInt());
}

void ToolBarManager::setState(const QList<QString>& names, State state)
{
    for (auto& name : names) {
        setState(name, state);
    }
}

void ToolBarManager::setState(const QString& name, State state)
{
    auto visibility = [this, name](bool defaultvalue) {
        return hPref->GetBool(name.toStdString().c_str(), defaultvalue);
    };

    auto saveVisibility = [this, name](bool value) {
        hPref->SetBool(name.toStdString().c_str(), value);
    };

    auto showhide = [visibility](QToolBar* toolbar, ToolBarItem::DefaultVisibility policy) {

        auto show = visibility( policy == ToolBarItem::DefaultVisibility::Visible );

        if(show) {
            toolbar->show();
        }
        else {
            toolbar->hide();
        }
    };

    QToolBar* tb = findToolBar(toolBars(), name);
    if (tb) {

        if (state == State::RestoreDefault) {

            auto policy = getToolbarPolicy(tb);

            if(policy == ToolBarItem::DefaultVisibility::Unavailable) {
                tb->hide();
                tb->toggleViewAction()->setVisible(false);
            }
            else  {
                tb->toggleViewAction()->setVisible(true);

                showhide(tb, policy);
            }
        }
        else if (state == State::ForceAvailable) {

            auto policy = getToolbarPolicy(tb);

            tb->toggleViewAction()->setVisible(true);

            // Unavailable policy defaults to a Visible toolbars when made available
            auto show = visibility( policy == ToolBarItem::DefaultVisibility::Visible ||
                                    policy == ToolBarItem::DefaultVisibility::Unavailable);

            if(show) {
                tb->show();
            }
            else {
                tb->hide();
            }
        }
        else if (state == State::ForceHidden) {
            tb->toggleViewAction()->setVisible(false); // not visible in context menus
            tb->hide(); // toolbar not visible

        }
        else if (state == State::SaveState) {
            auto show = tb->isVisible();
            saveVisibility(show);
        }
    }
}

#include "moc_ToolBarManager.cpp"
