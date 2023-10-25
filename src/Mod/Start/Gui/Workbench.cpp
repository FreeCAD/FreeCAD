/***************************************************************************
 *   Copyright (c) 2008 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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
#include <QCoreApplication>
#endif

#include <Base/Console.h>
#include <Base/Exception.h>
#include <Base/Interpreter.h>
#include <Base/Tools.h>
#include <Gui/Command.h>
#include <Gui/DockWindowManager.h>
#include <Gui/MDIView.h>
#include <Gui/MainWindow.h>
#include <Gui/ToolBarManager.h>

#include "Workbench.h"


using namespace StartGui;

TYPESYSTEM_SOURCE(StartGui::Workbench, Gui::StdWorkbench)

StartGui::Workbench::Workbench() = default;

StartGui::Workbench::~Workbench() = default;

void StartGui::Workbench::activated()
{
    // Automatically display the StartPage only the very first time
    static bool first = true;
    if (first) {
        loadStartPage();
        first = false;
    }
}

void StartGui::Workbench::loadStartPage()
{
    // Ensure that we don't open the Start page multiple times
    QString title = QCoreApplication::translate("Workbench", "Start page");
    QList<QWidget*> ch = Gui::getMainWindow()->windows();
    for (auto c : ch) {
        if (c->windowTitle() == title) {
            Gui::MDIView* mdi = qobject_cast<Gui::MDIView*>(c);
            if (mdi) {
                Gui::getMainWindow()->setActiveWindow(mdi);
            }
            return;
        }
    }

    try {
        QByteArray utf8Title = title.toUtf8();
        std::string escapedstr = Base::Tools::escapedUnicodeFromUtf8(utf8Title);
        std::stringstream str;
        str << "import WebGui,sys,Start\n"
            << "from StartPage import StartPage\n\n"
            << "class WebPage(object):\n"
            << "    def __init__(self):\n"
            << "        self.browser=WebGui.openBrowserWindow(u\"" << escapedstr.c_str() << "\")\n"
#if defined(FC_OS_WIN32)
            << "        self.browser.setHtml(StartPage.handle(), App.getResourceDir() + "
               "'Mod/Start/StartPage/')\n"
#else
            << "        self.browser.setHtml(StartPage.handle(), 'file://' + App.getResourceDir() "
               "+ 'Mod/Start/StartPage/')\n"
#endif
            << "    def onChange(self, par, reason):\n"
            << "        try:\n"
            << "            if reason == 'RecentFiles':\n"
#if defined(FC_OS_WIN32)
            << "                self.browser.setHtml(StartPage.handle(), App.getResourceDir() + "
               "'Mod/Start/StartPage/')\n\n"
#else
            << "                self.browser.setHtml(StartPage.handle(), 'file://' + "
               "App.getResourceDir() + 'Mod/Start/StartPage/')\n\n"
#endif
            << "        except RuntimeError as e:\n"
            << "            pass\n"
            << "class WebView(object):\n"
            << "    def __init__(self):\n"
            << "        self.pargrp = FreeCAD.ParamGet('User "
               "parameter:BaseApp/Preferences/RecentFiles')\n"
            << "        self.webPage = WebPage()\n"
            << "        self.pargrp.Attach(self.webPage)\n"
            << "    def __del__(self):\n"
            << "        self.pargrp.Detach(self.webPage)\n\n"
            << "webView = WebView()\n"
            << "StartPage.checkPostOpenStartPage()\n";

        Base::Interpreter().runString(str.str().c_str());
    }
    catch (const Base::Exception& e) {
        Base::Console().Error("%s\n", e.what());
    }
}

void StartGui::Workbench::setupContextMenu(const char* recipient, Gui::MenuItem* item) const
{
    Gui::StdWorkbench::setupContextMenu(recipient, item);
}

Gui::MenuItem* StartGui::Workbench::setupMenuBar() const
{
    return Gui::StdWorkbench::setupMenuBar();
}

Gui::ToolBarItem* StartGui::Workbench::setupToolBars() const
{
    Gui::ToolBarItem* root = StdWorkbench::setupToolBars();

    // web navigation toolbar
    Gui::ToolBarItem* navigation = new Gui::ToolBarItem(root);
    navigation->setCommand("Navigation");
    *navigation << "Web_BrowserSetURL"
                << "Separator"
                << "Web_OpenWebsite"
                << "Start_StartPage"
                << "Separator"
                << "Web_BrowserBack"
                << "Web_BrowserNext"
                << "Web_BrowserRefresh"
                << "Web_BrowserStop"
                << "Separator"
                << "Web_BrowserZoomIn"
                << "Web_BrowserZoomOut";

    return root;
}

Gui::ToolBarItem* StartGui::Workbench::setupCommandBars() const
{
    Gui::ToolBarItem* root = new Gui::ToolBarItem;
    return root;
}

Gui::DockWindowItems* StartGui::Workbench::setupDockWindows() const
{
    Gui::DockWindowItems* root = Gui::StdWorkbench::setupDockWindows();
    root->setVisibility(false);                  // hide all dock windows by default
    root->setVisibility("Std_ComboView", true);  // except of the combo view
    root->setVisibility("Std_TaskView", true);   // and the task view
    return root;
}
