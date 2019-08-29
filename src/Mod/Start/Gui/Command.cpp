/***************************************************************************
 *   Copyright (c) 2010 Jürgen Riegel (juergen.riegel@web.de)              *
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
# include <QTextStream>
#endif

#include <Gui/Application.h>
#include <Gui/Command.h>
#include <Gui/MainWindow.h>
#include <Gui/FileDialog.h>


using namespace std;

DEF_STD_CMD(CmdStartPage);

CmdStartPage::CmdStartPage()
	:Command("Start_StartPage")
{
    sAppModule      = "Start";
    sGroup          = QT_TR_NOOP("Start");
    sMenuText       = QT_TR_NOOP("Start Page");
    sToolTipText    = QT_TR_NOOP("Displays the start page in a browser view");
    sWhatsThis      = "Start_StartPage";
    sStatusTip      = sToolTipText;
    sPixmap         = "StartWorkbench";
}


void CmdStartPage::activated(int iMsg)
{
    Q_UNUSED(iMsg);

    // Ensure that we don't open the Start page multiple times
    QString title = QCoreApplication::translate("Workbench", "Start page");
    QList<QWidget*> ch = Gui::getMainWindow()->windows();
    for (QList<QWidget*>::const_iterator c = ch.begin(); c != ch.end(); ++c) {
        if ((*c)->windowTitle() == title)
            return;
    }

    try {
        QByteArray utf8Title = title.toUtf8();
        QByteArray cmd;
        QTextStream str(&cmd);
        str << "import WebGui" << endl;
        str << "from StartPage import StartPage" << endl;
        str << endl;
        str << "class WebPage(object):" << endl;
        str << "    def __init__(self):" << endl;
        str << "        self.browser=WebGui.openBrowserWindow('" << utf8Title << "')" << endl;
#if defined(FC_OS_WIN32)
        str << "        self.browser.setHtml(StartPage.handle(), App.getResourceDir() + 'Mod/Start/StartPage/')" << endl;
#else
        str << "        self.browser.setHtml(StartPage.handle(), 'file://' + App.getResourceDir() + 'Mod/Start/StartPage/')" << endl;
#endif
        str << "    def onChange(self, par, reason):" << endl;
        str << "        if reason == 'RecentFiles':" << endl;
#if defined(FC_OS_WIN32)
        str << "            self.browser.setHtml(StartPage.handle(), App.getResourceDir() + 'Mod/Start/StartPage/')" << endl;
#else
        str << "            self.browser.setHtml(StartPage.handle(), 'file://' + App.getResourceDir() + 'Mod/Start/StartPage/')" << endl;
#endif
        str << endl;
        str << "class WebView(object):" << endl;
        str << "    def __init__(self):" << endl;
        str << "        self.pargrp = FreeCAD.ParamGet('User parameter:BaseApp/Preferences/RecentFiles')" << endl;
        str << "        self.webPage = WebPage()" << endl;
        str << "        self.pargrp.Attach(self.webPage)" << endl;
        str << "    def __del__(self):" << endl;
        str << "        self.pargrp.Detach(self.webPage)" << endl;
        str << endl;
        str << "webView=WebView()" << endl;

        //Base::Interpreter().runString(cmd);
        // Gui::Command::runCommand(Gui::Command::Gui, cmd);
        Command::doCommand(Command::Gui, "import Start, StartGui");
        Command::doCommand(Command::Gui, cmd);
    }
    catch (const Base::Exception& e) {
        Base::Console().Error("%s\n", e.what());
    }
}



void CreateStartCommands(void)
{
    Gui::CommandManager &rcCmdMgr = Gui::Application::Instance->commandManager();

    rcCmdMgr.addCommand(new CmdStartPage());
 }
