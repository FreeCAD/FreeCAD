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
# include <QMenu>
# include <QMenuBar>
#endif

#include "MenuManager.h"
#include "Application.h"
#include "Command.h"
#include "MainWindow.h"


using namespace Gui;


MenuItem::MenuItem() = default;

MenuItem::MenuItem(MenuItem* item)
{
    if (item) {
        item->appendItem(this);
    }
}

MenuItem::~MenuItem()
{
    clear();
}

void MenuItem::setCommand(const std::string& name)
{
    _name = name;
}

std::string MenuItem::command() const
{
    return _name;
}

bool MenuItem::hasItems() const
{
    return !_items.isEmpty();
}

MenuItem* MenuItem::findItem(const std::string& name)
{
    if (_name == name) {
        return this;
    }
    else {
        for (auto& item : _items) {
            if (item->_name == name) {
                return item;
            }
        }
    }

    return nullptr;
}

MenuItem* MenuItem::findParentOf(const std::string& name)
{
    for (auto& item : _items) {
        if (item->_name == name) {
            return this;
        }
    }

    for (auto& item : _items) {
        if (item->findParentOf(name)) {
            return item;
        }
    }

    return nullptr;
}

MenuItem* MenuItem::copy() const
{
    auto root = new MenuItem;
    root->setCommand(command());

    for (auto& item : _items)
    {
        root->appendItem(item->copy());
    }

    return root;
}

uint MenuItem::count() const
{
    return _items.count();
}

void MenuItem::appendItem(MenuItem* item)
{
    _items.push_back(item);
}

bool MenuItem::insertItem(MenuItem* before, MenuItem* item)
{
    int pos = _items.indexOf(before);
    if (pos != -1) {
        _items.insert(pos, item);
        return true;
    }

    return false;
}

MenuItem* MenuItem::afterItem(MenuItem* item) const
{
    int pos = _items.indexOf(item);
    if (pos < 0 || pos+1 == _items.size()) {
        return nullptr;
    }
    return _items.at(pos+1);
}

void MenuItem::removeItem(MenuItem* item)
{
    int pos = _items.indexOf(item);
    if (pos != -1) {
        _items.removeAt(pos);
    }
}

void MenuItem::clear()
{
    for (auto& item : _items) {
        delete item;
    }
    _items.clear();
}

MenuItem& MenuItem::operator << (const std::string& command)
{
    auto item = new MenuItem(this);
    item->setCommand(command);
    return *this;
}

MenuItem& MenuItem::operator << (MenuItem* item)
{
    appendItem(item);
    return *this;
}

QList<MenuItem*> MenuItem::getItems() const
{
    return _items;
}

// -----------------------------------------------------------

MenuManager* MenuManager::_instance=nullptr;

MenuManager* MenuManager::getInstance()
{
    if ( !_instance ) {
        _instance = new MenuManager;
    }
    return _instance;
}

void MenuManager::destruct()
{
    delete _instance;
    _instance = nullptr;
}

MenuManager::MenuManager() = default;

MenuManager::~MenuManager() = default;

void MenuManager::setup(MenuItem* menuItems) const
{
    if (!menuItems) {
        return; // empty menu bar
    }

    QMenuBar* menuBar = getMainWindow()->menuBar();

    // By right, it should be fine for more than one command action having the
    // same shortcut but in different workbench. It should not require manual
    // conflict resolving in this case, as the action in an inactive workbench
    // is expected to be inactive as well, or else user may experience
    // seemingly random shortcut miss firing based on the order he/she
    // switches workbenches. In fact, this may be considered as an otherwise
    // difficult to implement feature of context aware shortcut, where a
    // specific shortcut can activate different actions under different
    // workbenches.
    //
    // This works as expected for action adding to a toolbar. As Qt will ignore
    // actions inside an invisible toolbar.  However, Qt refuse to do the same
    // for actions in a hidden menu action of a menu bar. This is very likely a
    // Qt bug, as the behavior does not seem to conform to Qt's documentation
    // of Qt::ShortcutContext.
    //
    // Clearing the menu bar, and recreate it every time when switching
    // workbench with only the active actions can solve this problem.
    menuBar->clear();

    QList<QAction*> actions = menuBar->actions();
    for (auto& item : menuItems->getItems())
    {
        // search for the menu action
        QAction* action = findAction(actions, QString::fromLatin1(item->command().c_str()));
        if (!action) {
            // There must be not more than one separator in the menu bar, so
            // we can safely remove it if available and append it at the end
            if (item->command() == "Separator") {
                action = menuBar->addSeparator();
                action->setObjectName(QLatin1String("Separator"));
            }
            else {
                // create a new menu
                std::string menuName = item->command();
                QMenu* menu = menuBar->addMenu(
                    QApplication::translate("Workbench", menuName.c_str()));
                action = menu->menuAction();
                menu->setObjectName(QString::fromLatin1(menuName.c_str()));
                action->setObjectName(QString::fromLatin1(menuName.c_str()));
            }

            // set the menu user data
            action->setData(QString::fromLatin1(item->command().c_str()));
        }
        else {
            // put the menu at the end
            menuBar->removeAction(action);
            menuBar->addAction(action);
            action->setVisible(true);
            int index = actions.indexOf(action);
            actions.removeAt(index);
        }

        // flll up the menu
        if (!action->isSeparator()) {
            setup(item, action->menu());
        }
    }

    // hide all menus which we don't need for the moment
    for (auto& action : actions) {
        action->setVisible(false);
    }

    // enable update again
    //menuBar->setUpdatesEnabled(true);
}

void MenuManager::setup(MenuItem* item, QMenu* menu) const
{
    CommandManager& mgr = Application::Instance->commandManager();
    QList<QAction*> actions = menu->actions();
    for (auto& item : item->getItems()) {
        // search for the menu item
        QList<QAction*> used_actions = findActions(actions, QString::fromLatin1(item->command().c_str()));
        if (used_actions.isEmpty()) {
            if (item->command() == "Separator") {
                QAction* action = menu->addSeparator();
                action->setObjectName(QLatin1String("Separator"));
                action->setData(QLatin1String("Separator"));
                used_actions.append(action);
            }
            else {
                if (item->hasItems()) {
                    // Creste a submenu
                    std::string menuName = item->command();
                    QMenu* submenu = menu->addMenu(QApplication::translate("Workbench", menuName.c_str()));
                    QAction* action = submenu->menuAction();
                    submenu->setObjectName(QString::fromLatin1(item->command().c_str()));
                    action->setObjectName(QString::fromLatin1(item->command().c_str()));
                    // set the menu user data
                    action->setData(QString::fromLatin1(item->command().c_str()));
                    used_actions.append(action);
                }
                else {
                    // A command can have more than one QAction
                    int count = menu->actions().count();
                    // Check if action was added successfully
                    if (mgr.addTo(item->command().c_str(), menu)) {
                        QList<QAction*> acts = menu->actions();
                        for (int i=count; i < acts.count(); i++) {
                            QAction* act = acts[i];
                            // set the menu user data
                            act->setData(QString::fromLatin1(item->command().c_str()));
                            used_actions.append(act);
                        }
                    }
                }
            }
        }
        else {
            for (auto& action : used_actions) {
                // put the menu item at the end
                menu->removeAction(action);
                menu->addAction(action);
                int index = actions.indexOf(action);
                actions.removeAt(index);
            }
        }

        // fill up the submenu
        if (item->hasItems()) {
            setup(item, used_actions.front()->menu());
        }
    }

    // remove all menu items which we don't need for the moment
    for (auto& action : actions) {
        menu->removeAction(action);
    }
}

void MenuManager::retranslate() const
{
    QMenuBar* menuBar = getMainWindow()->menuBar();
    for (auto& action : menuBar->actions()) {
        if (action->menu()) {
            retranslate(action->menu());
        }
    }
}

void MenuManager::retranslate(QMenu* menu) const
{
    // Note: Here we search for all menus and submenus to retranslate their
    // titles. To ease the translation for each menu the native name is set
    // as user data. However, there are special menus that are created by
    // actions for which the name of the according command name is set. For
    // such menus we have to use the command's menu text instead. Examples
    // for such actions are Std_RecentFiles, Std_Workbench or Std_FreezeViews.
    CommandManager& mgr = Application::Instance->commandManager();
    QByteArray menuName = menu->menuAction()->data().toByteArray();
    Command* cmd = mgr.getCommandByName(menuName);
    if (cmd) {
        menu->setTitle(
            QApplication::translate(cmd->className(),
                                    cmd->getMenuText()));
    }
    else {
        menu->setTitle(
            QApplication::translate("Workbench",
                                    (const char*)menuName));
    }
    for (auto& action : menu->actions()) {
        if (action->menu()) {
            retranslate(action->menu());
        }
    }
}

QAction* MenuManager::findAction(const QList<QAction*>& acts, const QString& item) const
{
    for (auto& action : acts) {
        if (action->data().toString() == item) {
            return action;
        }
    }

    return nullptr; // no item with the user data found
}

QList<QAction*> MenuManager::findActions(const QList<QAction*>& acts, const QString& item) const
{
    // It is possible that the user text of several actions match with 'item'.
    // But for the first match all following actions must match. For example
    // the Std_WindowsMenu command provides several actions with the same user
    // name.
    bool first_match = false;
    QList<QAction*> used;
    for (auto& action : acts) {
        if (action->data().toString() == item) {
            used.append(action);
            first_match = true;
            // get only one separator per request
            if (item == QLatin1String("Separator")) {
                break;
            }
        }
        else if (first_match) {
            break;
        }
    }

    return used;
}

void MenuManager::setupContextMenu(MenuItem* item, QMenu &menu) const
{
    setup(item, &menu);
}
