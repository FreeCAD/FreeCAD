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
# include <QAction>
# include <QApplication>
# include <QToolBar>
# include <QToolButton>
# include <QPointer>
#endif

#include "ToolBarManager.h"
#include "Application.h"
#include "Command.h"
#include "MainWindow.h"


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

    for (auto it : qAsConst(_items)) {
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
    for (auto it : qAsConst(_items)) {
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

ToolBarManager* ToolBarManager::_instance=nullptr;

ToolBarManager* ToolBarManager::getInstance()
{
    if ( !_instance )
        _instance = new ToolBarManager;
    return _instance;
}

void ToolBarManager::destruct()
{
    delete _instance;
    _instance = nullptr;
}

ToolBarManager::ToolBarManager() = default;

ToolBarManager::~ToolBarManager() = default;

namespace {
QPointer<QWidget> createActionWidget()
{
    static QPointer<QWidget> _ActionWidget;
    if (!_ActionWidget) {
        _ActionWidget = new QWidget(getMainWindow());
        _ActionWidget->setObjectName(QStringLiteral("_fc_action_widget_"));
        /* TODO This is a temporary hack until a longterm solution
        is found, thanks to @realthunder for this pointer.
        Although _ActionWidget has zero size, it somehow has a
        'phantom' size without any visible content and will block the top
        left tool buttons and menus of the application main window.
        Therefore it is moved out of the way. */
        _ActionWidget->move(QPoint(-100,-100));
    }
    else {
        auto actions = _ActionWidget->actions();
        for (auto action : actions) {
            _ActionWidget->removeAction(action);
        }
    }

    return _ActionWidget;
}
}

void ToolBarManager::setup(ToolBarItem* toolBarItems)
{
    if (!toolBarItems)
        return; // empty menu bar

    QPointer<QWidget> _ActionWidget = createActionWidget();

    saveState();
    this->toolbarNames.clear();

    int max_width = getMainWindow()->width();
    int top_width = 0;

    ParameterGrp::handle hPref = App::GetApplication().GetUserParameter().GetGroup("BaseApp")
                               ->GetGroup("MainWindow")->GetGroup("Toolbars");
    bool nameAsToolTip = App::GetApplication().GetUserParameter().GetGroup("BaseApp")
            ->GetGroup("Preferences")->GetGroup("MainWindow")->GetBool("ToolBarNameAsToolTip",true);
    QList<ToolBarItem*> items = toolBarItems->getItems();
    QList<QToolBar*> toolbars = toolBars();
    for (QList<ToolBarItem*>::Iterator it = items.begin(); it != items.end(); ++it) {
        // search for the toolbar
        QString name = QString::fromUtf8((*it)->command().c_str());
        this->toolbarNames << name;
        QToolBar* toolbar = findToolBar(toolbars, name);
        std::string toolbarName = (*it)->command();
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
        if((*it)->visibilityPolicy != ToolBarItem::DefaultVisibility::Unavailable) {
            bool defaultvisibility = (*it)->visibilityPolicy == ToolBarItem::DefaultVisibility::Visible;

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
        toolbar->toggleViewAction()->setProperty("DefaultVisibility", static_cast<int>((*it)->visibilityPolicy));

        // setup the toolbar
        setup(*it, toolbar);
        auto actions = toolbar->actions();
        for (auto action : actions) {
            _ActionWidget->addAction(action);
        }

        // try to add some breaks to avoid to have all toolbars in one line
        if (toolbar_added) {
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
    for (QList<QToolBar*>::Iterator it = toolbars.begin(); it != toolbars.end(); ++it) {
        // make sure that the main window has the focus when hiding the toolbar with
        // the combo box inside
        QWidget *fw = QApplication::focusWidget();
        while (fw &&  !fw->isWindow()) {
            if (fw == *it) {
                getMainWindow()->setFocus();
                break;
            }
            fw = fw->parentWidget();
        }
        // ignore toolbars which do not belong to the previously active workbench
        //QByteArray toolbarName = (*it)->objectName().toUtf8();
        if (!(*it)->toggleViewAction()->isVisible())
            continue;
        //hPref->SetBool(toolbarName.constData(), (*it)->isVisible());
        (*it)->hide();
        (*it)->toggleViewAction()->setVisible(false);
    }

    hPref = App::GetApplication().GetUserParameter().GetGroup("BaseApp")
        ->GetGroup("Preferences")->GetGroup("General");
    bool lockToolBars = hPref->GetBool("LockToolBars", false);
    setMovable(!lockToolBars);
}

void ToolBarManager::setup(ToolBarItem* item, QToolBar* toolbar) const
{
    CommandManager& mgr = Application::Instance->commandManager();
    QList<ToolBarItem*> items = item->getItems();
    QList<QAction*> actions = toolbar->actions();
    for (QList<ToolBarItem*>::Iterator it = items.begin(); it != items.end(); ++it) {
        // search for the action item
        QAction* action = findAction(actions, QString::fromLatin1((*it)->command().c_str()));
        if (!action) {
            if ((*it)->command() == "Separator") {
                action = toolbar->addSeparator();
            }
            else {
                // Check if action was added successfully
                if (mgr.addTo((*it)->command().c_str(), toolbar))
                    action = toolbar->actions().constLast();
            }

            // set the tool button user data
            if (action) {
                action->setData(QString::fromLatin1((*it)->command().c_str()));
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
    for (QList<QAction*>::Iterator it = actions.begin(); it != actions.end(); ++it) {
        toolbar->removeAction(*it);
    }
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

    ParameterGrp::handle hPref = App::GetApplication().GetUserParameter().GetGroup("BaseApp")
                               ->GetGroup("MainWindow")->GetGroup("Toolbars");

    QList<QToolBar*> toolbars = toolBars();
    for (QStringList::ConstIterator it = this->toolbarNames.begin(); it != this->toolbarNames.end(); ++it) {
        QToolBar* toolbar = findToolBar(toolbars, *it);
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
    ParameterGrp::handle hPref = App::GetApplication().GetUserParameter().GetGroup("BaseApp")
                               ->GetGroup("MainWindow")->GetGroup("Toolbars");

    QList<QToolBar*> toolbars = toolBars();
    for (QStringList::ConstIterator it = this->toolbarNames.begin(); it != this->toolbarNames.end(); ++it) {
        QToolBar* toolbar = findToolBar(toolbars, *it);
        if (toolbar) {
            QByteArray toolbarName = toolbar->objectName().toUtf8();
            toolbar->setVisible(hPref->GetBool(toolbarName.constData(), toolbar->isVisible()));
        }
    }


    hPref = App::GetApplication().GetUserParameter().GetGroup("BaseApp")
        ->GetGroup("Preferences")->GetGroup("General");
    bool lockToolBars = hPref->GetBool("LockToolBars", false);
    setMovable(!lockToolBars);
}

void ToolBarManager::retranslate() const
{
    QList<QToolBar*> toolbars = toolBars();
    for (QList<QToolBar*>::Iterator it = toolbars.begin(); it != toolbars.end(); ++it) {
        QByteArray toolbarName = (*it)->objectName().toUtf8();
        (*it)->setWindowTitle(
            QApplication::translate("Workbench",
                                    (const char*)toolbarName));
    }
}

void Gui::ToolBarManager::setMovable(bool moveable) const
{
    for (auto& tb : toolBars()) {
        tb->setMovable(moveable);
    }
}

QToolBar* ToolBarManager::findToolBar(const QList<QToolBar*>& toolbars, const QString& item) const
{
    for (QList<QToolBar*>::ConstIterator it = toolbars.begin(); it != toolbars.end(); ++it) {
        if ((*it)->objectName() == item)
            return *it;
    }

    return nullptr; // no item with the user data found
}

QAction* ToolBarManager::findAction(const QList<QAction*>& acts, const QString& item) const
{
    for (QList<QAction*>::ConstIterator it = acts.begin(); it != acts.end(); ++it) {
        if ((*it)->data().toString() == item)
            return *it;
    }

    return nullptr; // no item with the user data found
}

QList<QToolBar*> ToolBarManager::toolBars() const
{
    QWidget* mw = getMainWindow();
    QList<QToolBar*> tb;
    QList<QToolBar*> bars = getMainWindow()->findChildren<QToolBar*>();
    for (QList<QToolBar*>::Iterator it = bars.begin(); it != bars.end(); ++it) {
        if ((*it)->parentWidget() == mw)
            tb.push_back(*it);
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
    ParameterGrp::handle hPref = App::GetApplication().GetUserParameter().GetGroup("BaseApp")->GetGroup("MainWindow")->GetGroup("Toolbars");

    auto visibility = [hPref, name](bool defaultvalue) {
        return hPref->GetBool(name.toStdString().c_str(), defaultvalue);
    };

    auto saveVisibility = [hPref, name](bool value) {
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
