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
# include <QStatusBar>
#endif

#include "Command.h"
#include "Action.h"
#include "Application.h"
#include "MainWindow.h"
#include "View.h"
#include "Document.h"
#include "DlgActivateWindowImp.h"
#include "DockWindowManager.h"

#include <Base/Exception.h>
#include <App/Document.h>

using namespace Gui;


//===========================================================================
// Std_ArrangeIcons
//===========================================================================
DEF_STD_CMD_A(StdCmdArrangeIcons);

StdCmdArrangeIcons::StdCmdArrangeIcons()
  : Command("Std_ArrangeIcons")
{
    sGroup        = QT_TR_NOOP("Window");
    sMenuText     = QT_TR_NOOP("Arrange &Icons");
    sToolTipText  = QT_TR_NOOP("Arrange Icons");
    sWhatsThis    = QT_TR_NOOP("Arrange Icons");
    sStatusTip    = QT_TR_NOOP("Arrange Icons");
    eType         = 0;
}

void StdCmdArrangeIcons::activated(int iMsg)
{
    getMainWindow()->arrangeIcons ();
}

bool StdCmdArrangeIcons::isActive(void)
{
    return !(getMainWindow()->windows().isEmpty());
}

//===========================================================================
// Std_TileWindows
//===========================================================================
DEF_STD_CMD_A(StdCmdTileWindows);

StdCmdTileWindows::StdCmdTileWindows()
  : Command("Std_TileWindows")
{
    sGroup        = QT_TR_NOOP("Window");
    sMenuText     = QT_TR_NOOP("&Tile");
    sToolTipText  = QT_TR_NOOP("Tile the windows");
    sWhatsThis    = QT_TR_NOOP("Tile the windows");
    sStatusTip    = QT_TR_NOOP("Tile the windows");
    sPixmap       = "Std_WindowTileVer";
    eType         = 0;
}

void StdCmdTileWindows::activated(int iMsg)
{
    getMainWindow()->tile();
}

bool StdCmdTileWindows::isActive(void)
{
    return !(getMainWindow()->windows().isEmpty());
}

//===========================================================================
// Std_CascadeWindows
//===========================================================================
DEF_STD_CMD_A(StdCmdCascadeWindows);

StdCmdCascadeWindows::StdCmdCascadeWindows()
  : Command("Std_CascadeWindows")
{
    sGroup        = QT_TR_NOOP("Window");
    sMenuText     = QT_TR_NOOP("&Cascade");
    sToolTipText  = QT_TR_NOOP("Tile pragmatic");
    sWhatsThis    = QT_TR_NOOP("Tile pragmatic");
    sStatusTip    = QT_TR_NOOP("Tile pragmatic");
    sPixmap       = "Std_WindowCascade";
    eType         = 0;
}

void StdCmdCascadeWindows::activated(int iMsg)
{
    getMainWindow()->cascade();
}

bool StdCmdCascadeWindows::isActive(void)
{
    return !(getMainWindow()->windows().isEmpty());
}

//===========================================================================
// Std_CloseActiveWindow
//===========================================================================
DEF_STD_CMD_A(StdCmdCloseActiveWindow);

StdCmdCloseActiveWindow::StdCmdCloseActiveWindow()
  : Command("Std_CloseActiveWindow")
{
    sGroup        = QT_TR_NOOP("Window");
    sMenuText     = QT_TR_NOOP("Cl&ose");
    sToolTipText  = QT_TR_NOOP("Close active window");
    sWhatsThis    = QT_TR_NOOP("Close active window");
    sStatusTip    = QT_TR_NOOP("Close active window");
    // CTRL+F4 is already set by an QMdiSubWindow and thus we must use the
    // alternative CTRL+W here to avoid an ambiguous shortcut overload
	sAccel        = "Ctrl+W";
    eType         = 0;
}

void StdCmdCloseActiveWindow::activated(int iMsg)
{
    getMainWindow()->closeActiveWindow();
}

bool StdCmdCloseActiveWindow::isActive(void)
{
    return !(getMainWindow()->windows().isEmpty());
}

//===========================================================================
// Std_CloseAllWindows
//===========================================================================
DEF_STD_CMD_A(StdCmdCloseAllWindows);

StdCmdCloseAllWindows::StdCmdCloseAllWindows()
  : Command("Std_CloseAllWindows")
{
    sGroup        = QT_TR_NOOP("Window");
    sMenuText     = QT_TR_NOOP("Close Al&l");
    sToolTipText  = QT_TR_NOOP("Close all windows");
    sWhatsThis    = QT_TR_NOOP("Close all windows");
    sStatusTip    = QT_TR_NOOP("Close all windows");
    eType         = 0;
}

void StdCmdCloseAllWindows::activated(int iMsg)
{
    getMainWindow()->closeAllWindows();
}

bool StdCmdCloseAllWindows::isActive(void)
{
    return !(getMainWindow()->windows().isEmpty());
}

//===========================================================================
// Std_ActivateNextWindow
//===========================================================================
DEF_STD_CMD_A(StdCmdActivateNextWindow);

StdCmdActivateNextWindow::StdCmdActivateNextWindow()
  : Command("Std_ActivateNextWindow")
{
    sGroup        = QT_TR_NOOP("Window");
    sMenuText     = QT_TR_NOOP("Ne&xt");
    sToolTipText  = QT_TR_NOOP("Activate next window");
    sWhatsThis    = QT_TR_NOOP("Activate next window");
    sStatusTip    = QT_TR_NOOP("Activate next window");
    sPixmap       = "Std_WindowNext";
#ifndef NO_USE_QT_MDI_AREA
    sAccel        = keySequenceToAccel(QKeySequence::NextChild);
#else
    sAccel        = "";
#endif
    eType         = 0;
}

void StdCmdActivateNextWindow::activated(int iMsg)
{
    getMainWindow()->activateNextWindow();
}

bool StdCmdActivateNextWindow::isActive(void)
{
    return !(getMainWindow()->windows().isEmpty());
}

//===========================================================================
// Std_ActivatePrevWindow
//===========================================================================
DEF_STD_CMD_A(StdCmdActivatePrevWindow);

StdCmdActivatePrevWindow::StdCmdActivatePrevWindow()
  : Command("Std_ActivatePrevWindow")
{
    sGroup        = QT_TR_NOOP("Window");
    sMenuText     = QT_TR_NOOP("Pre&vious");
    sToolTipText  = QT_TR_NOOP("Activate previous window");
    sWhatsThis    = QT_TR_NOOP("Activate previous window");
    sStatusTip    = QT_TR_NOOP("Activate previous window");
    sPixmap       = "Std_WindowPrev";
#ifndef NO_USE_QT_MDI_AREA
    sAccel        = keySequenceToAccel(QKeySequence::PreviousChild);
#else
    sAccel        = "";
#endif
    eType         = 0;
}

void StdCmdActivatePrevWindow::activated(int iMsg)
{
    getMainWindow()->activatePreviousWindow();
}

bool StdCmdActivatePrevWindow::isActive(void)
{
    return !(getMainWindow()->windows().isEmpty());
}

//===========================================================================
// Std_Windows
//===========================================================================
DEF_STD_CMD(StdCmdWindows);

StdCmdWindows::StdCmdWindows()
  : Command("Std_Windows")
{
    sGroup        = QT_TR_NOOP("Window");
    sMenuText     = QT_TR_NOOP("&Windows...");
    sToolTipText  = QT_TR_NOOP("Windows list");
    sWhatsThis    = QT_TR_NOOP("Windows list");
    sStatusTip    = QT_TR_NOOP("Windows list");
    //sPixmap     = "";
    eType         = 0;
}

void StdCmdWindows::activated(int iMsg)
{
    Gui::Dialog::DlgActivateWindowImp dlg( getMainWindow() );
    dlg.exec();
}

//===========================================================================
// Std_UserInterface
//===========================================================================
DEF_STD_CMD(StdCmdUserInterface);

StdCmdUserInterface::StdCmdUserInterface()
  : Command("Std_UserInterface")
{
    sGroup        = QT_TR_NOOP("View");
    sMenuText     = QT_TR_NOOP("Dock views");
    sToolTipText  = QT_TR_NOOP("Dock all top-level views");
    sWhatsThis    = QT_TR_NOOP("Dock all top-level views");
    sStatusTip    = QT_TR_NOOP("Dock all top-level views");
    eType         = 0;
}

void StdCmdUserInterface::activated(int)
{
    getMainWindow()->switchToDockedMode();
}

//===========================================================================
// Std_DockWindowMenu
//===========================================================================

DEF_STD_CMD_AC(StdCmdDockViewMenu);

StdCmdDockViewMenu::StdCmdDockViewMenu()
  : Command("Std_DockViewMenu")
{
    sGroup        = QT_TR_NOOP("View");
    sMenuText     = QT_TR_NOOP("Vie&ws");
    sToolTipText  = QT_TR_NOOP("Toggles this window");
    sWhatsThis    = QT_TR_NOOP("Toggles this window");
    sStatusTip    = QT_TR_NOOP("Toggles this window");
    eType         = 0;
}

void StdCmdDockViewMenu::activated(int iMsg)
{
    // Handled by the related QAction objects
}

bool StdCmdDockViewMenu::isActive(void)
{
    return true;
}

Action * StdCmdDockViewMenu::createAction(void)
{
    Action *pcAction;
    pcAction = new DockWidgetAction(this, getMainWindow());
    applyCommandData(pcAction);
    return pcAction;
}

//===========================================================================
// Std_ToolBarMenu
//===========================================================================

DEF_STD_CMD_AC(StdCmdToolBarMenu);

StdCmdToolBarMenu::StdCmdToolBarMenu()
  : Command("Std_ToolBarMenu")
{
    sGroup        = QT_TR_NOOP("View");
    sMenuText     = QT_TR_NOOP("Tool&bars");
    sToolTipText  = QT_TR_NOOP("Toggles this window");
    sWhatsThis    = QT_TR_NOOP("Toggles this window");
    sStatusTip    = QT_TR_NOOP("Toggles this window");
    eType         = 0;
}

void StdCmdToolBarMenu::activated(int iMsg)
{
    // Handled by the related QAction objects
}

bool StdCmdToolBarMenu::isActive(void)
{
    return true;
}

Action * StdCmdToolBarMenu::createAction(void)
{
    Action *pcAction;
    pcAction = new ToolBarAction(this, getMainWindow());
    applyCommandData(pcAction);
    return pcAction;
}

//===========================================================================
// Std_ViewStatusBar
//===========================================================================

DEF_STD_CMD_C(StdCmdStatusBar);

StdCmdStatusBar::StdCmdStatusBar()
  : Command("Std_ViewStatusBar")
{
    sGroup        = QT_TR_NOOP("View");
    sMenuText     = QT_TR_NOOP("Status bar");
    sToolTipText  = QT_TR_NOOP("Toggles the status bar");
    sWhatsThis    = QT_TR_NOOP("Toggles the status bar");
    sStatusTip    = QT_TR_NOOP("Toggles the status bar");
    eType         = 0;
}

Action * StdCmdStatusBar::createAction(void)
{
    Action *pcAction = Command::createAction();
    pcAction->setCheckable(true);
    pcAction->setChecked(true);

    return pcAction;
}

void StdCmdStatusBar::activated(int iMsg)
{
    getMainWindow()->statusBar()->setShown(iMsg != 0);
}

//===========================================================================
// Std_WindowsMenu
//===========================================================================

DEF_STD_CMD_AC(StdCmdWindowsMenu );

StdCmdWindowsMenu::StdCmdWindowsMenu()
  : Command("Std_WindowsMenu")
{
    sGroup        = QT_TR_NOOP("Window");
    sMenuText     = QT_TR_NOOP("Activates this window");
    sToolTipText  = QT_TR_NOOP("Activates this window");
    sWhatsThis    = QT_TR_NOOP("Activates this window");
    sStatusTip    = QT_TR_NOOP("Activates this window");
    eType         = 0;
}

void StdCmdWindowsMenu::activated(int iMsg)
{
    // already handled by the main window
}

bool StdCmdWindowsMenu::isActive(void)
{
    return true;
}

Action * StdCmdWindowsMenu::createAction(void)
{
    // Allow to show 10 menu items in the 'Window' menu and one separator.
    // If we have more windows then the user can use the 'Windows...' item.
    WindowAction *pcAction;
    pcAction = new WindowAction(this, getMainWindow());
    for ( int i=0; i<10; i++ ) {
        QAction* window = pcAction->addAction(QObject::tr(sToolTipText));
        window->setCheckable(true);
        window->setToolTip(QCoreApplication::translate(
            this->className(), sToolTipText, 0,
            QCoreApplication::CodecForTr));
        window->setStatusTip(QCoreApplication::translate(
            this->className(), sStatusTip, 0,
            QCoreApplication::CodecForTr));
        window->setWhatsThis(QCoreApplication::translate(
            this->className(), sWhatsThis, 0,
            QCoreApplication::CodecForTr));
    }

    QAction* sep = pcAction->addAction(QLatin1String(""));
    sep->setSeparator(true);

    return pcAction;
}

//===========================================================================
// Instanciation
//===========================================================================


namespace Gui {

void CreateWindowStdCommands(void)
{
    CommandManager &rcCmdMgr = Application::Instance->commandManager();

    rcCmdMgr.addCommand(new StdCmdArrangeIcons());
    rcCmdMgr.addCommand(new StdCmdTileWindows());
    rcCmdMgr.addCommand(new StdCmdCascadeWindows());
    rcCmdMgr.addCommand(new StdCmdCloseActiveWindow());
    rcCmdMgr.addCommand(new StdCmdCloseAllWindows());
    rcCmdMgr.addCommand(new StdCmdActivateNextWindow());
    rcCmdMgr.addCommand(new StdCmdActivatePrevWindow());
    rcCmdMgr.addCommand(new StdCmdWindows());
    rcCmdMgr.addCommand(new StdCmdDockViewMenu());
    rcCmdMgr.addCommand(new StdCmdToolBarMenu());
    rcCmdMgr.addCommand(new StdCmdWindowsMenu());
    rcCmdMgr.addCommand(new StdCmdStatusBar());
    rcCmdMgr.addCommand(new StdCmdUserInterface());
}

} // namespace Gui


