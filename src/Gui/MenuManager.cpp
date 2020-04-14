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
#include "MainWindow.h"
#include "Command.h"

using namespace Gui;


MenuItem::MenuItem()
{
}

MenuItem::MenuItem(MenuItem* item)
{
    if (item)
        item->appendItem(this);
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
    return _items.count() > 0;
}

MenuItem* MenuItem::findItem(const std::string& name)
{
    if (_name == name)
    {
        return this;
    }
    else
    {
        for (QList<MenuItem*>::Iterator it = _items.begin(); it != _items.end(); ++it)
        {
            if ((*it)->_name == name)
                return *it;
        }
    }

    return 0;
}

MenuItem* MenuItem::copy() const
{
    MenuItem* root = new MenuItem;
    root->setCommand(command());

    QList<MenuItem*> items = getItems();
    for (QList<MenuItem*>::ConstIterator it = items.begin(); it != items.end(); ++it)
    {
        root->appendItem((*it)->copy());
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
    if (pos != -1)
    {
        _items.insert(pos, item);
        return true;
    }
    else
        return false;
}

MenuItem* MenuItem::afterItem(MenuItem* item) const
{
    int pos = _items.indexOf(item);
    if (pos < 0 || pos+1 == _items.size())
        return 0;
    return _items.at(pos+1);
}

void MenuItem::removeItem(MenuItem* item)
{
    int pos = _items.indexOf(item);
    if (pos != -1)
        _items.removeAt(pos);
}

void MenuItem::clear()
{
    for (QList<MenuItem*>::Iterator it = _items.begin(); it != _items.end(); ++it)
        delete *it;
    _items.clear();
}

MenuItem& MenuItem::operator << (const std::string& command)
{
    MenuItem* item = new MenuItem(this);
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

MenuManager* MenuManager::_instance=0;

MenuManager* MenuManager::getInstance()
{
    if ( !_instance )
        _instance = new MenuManager;
    return _instance;
}

void MenuManager::destruct()
{
    delete _instance;
    _instance = 0;
}

MenuManager::MenuManager()
{
}

MenuManager::~MenuManager()
{
}

void MenuManager::setup(MenuItem* menuItems) const
{
    if (!menuItems)
        return; // empty menu bar

    QMenuBar* menuBar = getMainWindow()->menuBar();

#if defined(FC_OS_MACOSX) && QT_VERSION >= 0x050900
    // Unknown Qt macOS bug observed with Qt >= 5.9.4 causes random crashes when viewing reused top level menus.
    menuBar->clear();
#endif

    // On Kubuntu 18.10 global menu has issues with FreeCAD 0.18 menu bar.
    // Optional parameter, clearing the menu bar, can be set as a workaround.
    // Clearing the menu bar can cause issues, when trying to access menu bar through Python.
    // https://forum.freecadweb.org/viewtopic.php?f=10&t=30340&start=440#p289330
    if (App::GetApplication().GetParameterGroupByPath
        ("User parameter:BaseApp/Preferences/MainWindow")->GetBool("ClearMenuBar",false)) {
        menuBar->clear();
    }

    QList<MenuItem*> items = menuItems->getItems();
    QList<QAction*> actions = menuBar->actions();
    for (QList<MenuItem*>::ConstIterator it = items.begin(); it != items.end(); ++it)
    {
        // search for the menu action
        QAction* action = findAction(actions, QString::fromLatin1((*it)->command().c_str()));
        if (!action) {
            // There must be not more than one separator in the menu bar, so
            // we can safely remove it if available and append it at the end
            if ((*it)->command() == "Separator") {
                action = menuBar->addSeparator();
                action->setObjectName(QLatin1String("Separator"));
            }
            else {
                // create a new menu
                std::string menuName = (*it)->command();
                QMenu* menu = menuBar->addMenu(
                    QApplication::translate("Workbench", menuName.c_str()));
                action = menu->menuAction();
                menu->setObjectName(QString::fromLatin1(menuName.c_str()));
                action->setObjectName(QString::fromLatin1(menuName.c_str()));
            }

            // set the menu user data
            action->setData(QString::fromLatin1((*it)->command().c_str()));
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
        if (!action->isSeparator())
            setup(*it, action->menu());
    }

    // hide all menus which we don't need for the moment
    for (QList<QAction*>::Iterator it = actions.begin(); it != actions.end(); ++it) {
        (*it)->setVisible(false);
    }

    // enable update again
    //menuBar->setUpdatesEnabled(true);
}

void MenuManager::setup(MenuItem* item, QMenu* menu) const
{
    CommandManager& mgr = Application::Instance->commandManager();
    QList<MenuItem*> items = item->getItems();
    QList<QAction*> actions = menu->actions();
    for (QList<MenuItem*>::ConstIterator it = items.begin(); it != items.end(); ++it) {
        // search for the menu item
        QList<QAction*> used_actions = findActions(actions, QString::fromLatin1((*it)->command().c_str()));
        if (used_actions.isEmpty()) {
            if ((*it)->command() == "Separator") {
                QAction* action = menu->addSeparator();
                action->setObjectName(QLatin1String("Separator"));
                // set the menu user data
                action->setData(QLatin1String("Separator"));
                used_actions.append(action);
            }
            else {
                if ((*it)->hasItems()) {
                    // Creste a submenu
                    std::string menuName = (*it)->command();
                    QMenu* submenu = menu->addMenu(
                        QApplication::translate("Workbench", menuName.c_str()));
                    QAction* action = submenu->menuAction();
                    submenu->setObjectName(QString::fromLatin1((*it)->command().c_str()));
                    action->setObjectName(QString::fromLatin1((*it)->command().c_str()));
                    // set the menu user data
                    action->setData(QString::fromLatin1((*it)->command().c_str()));
                    used_actions.append(action);
                }
                else {
                    // A command can have more than one QAction
                    int count = menu->actions().count();
                    // Check if action was added successfully
                    if (mgr.addTo((*it)->command().c_str(), menu)) {
                        QList<QAction*> acts = menu->actions();
                        for (int i=count; i < acts.count(); i++) {
                            QAction* a = acts[i];
                            // set the menu user data
                            a->setData(QString::fromLatin1((*it)->command().c_str()));
                            used_actions.append(a);
                        }
                    }
                }
            }
        }
        else {
            for (QList<QAction*>::Iterator it = used_actions.begin(); it != used_actions.end(); ++it) {
                // put the menu item at the end
                menu->removeAction(*it);
                menu->addAction(*it);
                int index = actions.indexOf(*it);
                actions.removeAt(index);
            }
        }

        // fill up the submenu
        if ((*it)->hasItems())
            setup(*it, used_actions.front()->menu());
    }

    // remove all menu items which we don't need for the moment
    for (QList<QAction*>::Iterator it = actions.begin(); it != actions.end(); ++it) {
        menu->removeAction(*it);
    }
}

void MenuManager::retranslate() const
{
    QMenuBar* menuBar = getMainWindow()->menuBar();
    QList<QAction*> actions = menuBar->actions();
    for (QList<QAction*>::Iterator it = actions.begin(); it != actions.end(); ++it) {
        if ((*it)->menu())
            retranslate((*it)->menu());
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
    QList<QAction*> actions = menu->actions();
    for (QList<QAction*>::Iterator it = actions.begin(); it != actions.end(); ++it) {
        if ((*it)->menu()) {
            retranslate((*it)->menu());
        }
    }
}

QAction* MenuManager::findAction(const QList<QAction*>& acts, const QString& item) const
{
    for (QList<QAction*>::ConstIterator it = acts.begin(); it != acts.end(); ++it) {
        if ((*it)->data().toString() == item)
            return *it;
    }

    return 0; // no item with the user data found
}

QList<QAction*> MenuManager::findActions(const QList<QAction*>& acts, const QString& item) const
{
    // It is possible that the user text of several actions match with 'item'.
    // But for the first match all following actions must match. For example
    // the Std_WindowsMenu command provides several actions with the same user
    // name.
    bool first_match = false;
    QList<QAction*> used;
    for (QList<QAction*>::ConstIterator it = acts.begin(); it != acts.end(); ++it) {
        if ((*it)->data().toString() == item) {
            used.append(*it);
            first_match = true;
            // get only one separator per request
            if (item == QLatin1String("Separator"))
                break;
        }
        else if (first_match)
            break;
    }

    return used;
}

void MenuManager::setupContextMenu(MenuItem* item, QMenu &menu) const
{
    setup(item, &menu);
}
