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
# include <QCoreApplication>
#endif

#include "Workbench.h"
#include <Gui/ToolBarManager.h>
#include <Gui/MenuManager.h>
#include <Gui/ToolBarManager.h>
#include <Gui/DockWindowManager.h>
#include <Gui/Application.h>
#include <Gui/Action.h>
#include <Gui/Command.h>
#include <Gui/Selection.h>
#include <Gui/ToolBoxManager.h>
#include <Gui/MainWindow.h>
#include <App/Document.h>
#include <App/DocumentObject.h>
#include <Base/Console.h>
#include <Base/Exception.h>
#include <Base/Interpreter.h>
#include <Base/Tools.h>

#include <Mod/Start/App/StartConfiguration.h>

using namespace StartGui;

TYPESYSTEM_SOURCE(StartGui::Workbench, Gui::StdWorkbench)

StartGui::Workbench::Workbench()
{
}

StartGui::Workbench::~Workbench()
{
}

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
    for (QList<QWidget*>::const_iterator c = ch.begin(); c != ch.end(); ++c) {
        if ((*c)->windowTitle() == title)
            return;
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
            << "        self.browser.setHtml(StartPage.handle(), App.getResourceDir() + 'Mod/Start/StartPage/')\n"
#else
            << "        self.browser.setHtml(StartPage.handle(), 'file://' + App.getResourceDir() + 'Mod/Start/StartPage/')\n"
#endif
            << "    def onChange(self, par, reason):\n"
            << "        try:\n"
            << "            if reason == 'RecentFiles':\n"
#if defined(FC_OS_WIN32)
            << "                self.browser.setHtml(StartPage.handle(), App.getResourceDir() + 'Mod/Start/StartPage/')\n\n"
#else
            << "                self.browser.setHtml(StartPage.handle(), 'file://' + App.getResourceDir() + 'Mod/Start/StartPage/')\n\n"
#endif
            << "        except RuntimeError as e:\n"
            << "            pass\n"
            << "class WebView(object):\n"
            << "    def __init__(self):\n"
            << "        self.pargrp = FreeCAD.ParamGet('User parameter:BaseApp/Preferences/RecentFiles')\n"
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
    Q_UNUSED(recipient);
    Q_UNUSED(item);
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
    root->setVisibility(false); // hide all dock windows by default
    root->setVisibility("Std_ComboView",true); // except of the combo view
    return root;
}
