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
# include <QCoreApplication>
# include <QStatusBar>
# include <QEvent>
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
// Std_TileWindows
//===========================================================================
DEF_STD_CMD_A(StdCmdTileWindows)

StdCmdTileWindows::StdCmdTileWindows()
  : Command("Std_TileWindows")
{
    sGroup        = "Window";
    sMenuText     = QT_TR_NOOP("&Tile");
    sToolTipText  = QT_TR_NOOP("Tile the windows");
    sWhatsThis    = "Std_TileWindows";
    sStatusTip    = QT_TR_NOOP("Tile the windows");
    sPixmap       = "Std_WindowTileVer";
    eType         = 0;
}

void StdCmdTileWindows::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    getMainWindow()->tile();
}

bool StdCmdTileWindows::isActive()
{
    return !(getMainWindow()->windows().isEmpty());
}

//===========================================================================
// Std_CascadeWindows
//===========================================================================
DEF_STD_CMD_A(StdCmdCascadeWindows)

StdCmdCascadeWindows::StdCmdCascadeWindows()
  : Command("Std_CascadeWindows")
{
    sGroup        = "Window";
    sMenuText     = QT_TR_NOOP("&Cascade");
    sToolTipText  = QT_TR_NOOP("Tile pragmatic");
    sWhatsThis    = "Std_CascadeWindows";
    sStatusTip    = QT_TR_NOOP("Tile pragmatic");
    sPixmap       = "Std_WindowCascade";
    eType         = 0;
}

void StdCmdCascadeWindows::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    getMainWindow()->cascade();
}

bool StdCmdCascadeWindows::isActive()
{
    return !(getMainWindow()->windows().isEmpty());
}

//===========================================================================
// Std_CloseActiveWindow
//===========================================================================
DEF_STD_CMD_A(StdCmdCloseActiveWindow)

StdCmdCloseActiveWindow::StdCmdCloseActiveWindow()
  : Command("Std_CloseActiveWindow")
{
    sGroup        = "Window";
    sMenuText     = QT_TR_NOOP("Cl&ose");
    sToolTipText  = QT_TR_NOOP("Close active window");
    sWhatsThis    = "Std_CloseActiveWindow";
    sStatusTip    = QT_TR_NOOP("Close active window");
    // In QMdiSubWindow the 'QKeySequence::Close' shortcut is set which will
    // collide with this shortcut. Thus the shortcut of QMdiSubWindow will be
    // reset in MainWindow::addWindow() (#0002631)
    sAccel        = keySequenceToAccel(QKeySequence::Close);
    sPixmap       = "Std_CloseActiveWindow";
    eType         = NoTransaction;
}

void StdCmdCloseActiveWindow::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    getMainWindow()->closeActiveWindow();
}

bool StdCmdCloseActiveWindow::isActive()
{
    return !(getMainWindow()->windows().isEmpty());
}

//===========================================================================
// Std_CloseAllWindows
//===========================================================================
DEF_STD_CMD_A(StdCmdCloseAllWindows)

StdCmdCloseAllWindows::StdCmdCloseAllWindows()
  : Command("Std_CloseAllWindows")
{
    sGroup        = "Window";
    sMenuText     = QT_TR_NOOP("Close Al&l");
    sToolTipText  = QT_TR_NOOP("Close all windows");
    sWhatsThis    = "Std_CloseAllWindows";
    sStatusTip    = QT_TR_NOOP("Close all windows");
    sPixmap       = "Std_CloseAllWindows";
    eType         = NoTransaction;
}

void StdCmdCloseAllWindows::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    getMainWindow()->closeAllDocuments();
}

bool StdCmdCloseAllWindows::isActive()
{
    return !(getMainWindow()->windows().isEmpty()) || !App::GetApplication().getDocuments().empty();
}

//===========================================================================
// Std_ActivateNextWindow
//===========================================================================
DEF_STD_CMD_A(StdCmdActivateNextWindow)

StdCmdActivateNextWindow::StdCmdActivateNextWindow()
  : Command("Std_ActivateNextWindow")
{
    sGroup        = "Window";
    sMenuText     = QT_TR_NOOP("Ne&xt");
    sToolTipText  = QT_TR_NOOP("Activate next window");
    sWhatsThis    = "Std_ActivateNextWindow";
    sStatusTip    = QT_TR_NOOP("Activate next window");
    sPixmap       = "Std_WindowNext";
    sAccel        = keySequenceToAccel(QKeySequence::NextChild);
    eType         = 0;
}

void StdCmdActivateNextWindow::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    getMainWindow()->activateNextWindow();
}

bool StdCmdActivateNextWindow::isActive()
{
    return !(getMainWindow()->windows().isEmpty());
}

//===========================================================================
// Std_ActivatePrevWindow
//===========================================================================
DEF_STD_CMD_A(StdCmdActivatePrevWindow)

StdCmdActivatePrevWindow::StdCmdActivatePrevWindow()
  : Command("Std_ActivatePrevWindow")
{
    sGroup        = "Window";
    sMenuText     = QT_TR_NOOP("Pre&vious");
    sToolTipText  = QT_TR_NOOP("Activate previous window");
    sWhatsThis    = "Std_ActivatePrevWindow";
    sStatusTip    = QT_TR_NOOP("Activate previous window");
    sPixmap       = "Std_WindowPrev";
    // Depending on the OS 'QKeySequence::PreviousChild' gives
    // Ctrl+Shift+Backtab instead of Ctrl+Shift+Tab which leads
    // to a strange behaviour when using it.
    // A workaround is to create a shortcut as Shift + QKeySequence::NextChild
    static std::string previousChild = std::string("Shift+") + keySequenceToAccel(QKeySequence::NextChild);
    sAccel        = previousChild.c_str();
    eType         = 0;
}

void StdCmdActivatePrevWindow::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    getMainWindow()->activatePreviousWindow();
}

bool StdCmdActivatePrevWindow::isActive()
{
    return !(getMainWindow()->windows().isEmpty());
}

//===========================================================================
// Std_Windows
//===========================================================================
DEF_STD_CMD(StdCmdWindows)

StdCmdWindows::StdCmdWindows()
  : Command("Std_Windows")
{
    sGroup        = "Window";
    sMenuText     = QT_TR_NOOP("&Windows...");
    sToolTipText  = QT_TR_NOOP("Windows list");
    sWhatsThis    = "Std_Windows";
    sStatusTip    = QT_TR_NOOP("Windows list");
    sPixmap       = "Std_Windows";
    eType         = 0;
}

void StdCmdWindows::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    Gui::Dialog::DlgActivateWindowImp dlg( getMainWindow() );
    dlg.exec();
}

//===========================================================================
// Std_UserInterface
//===========================================================================
DEF_STD_CMD(StdCmdUserInterface)

StdCmdUserInterface::StdCmdUserInterface()
  : Command("Std_UserInterface")
{
    sGroup        = "View";
    sMenuText     = QT_TR_NOOP("Dock views");
    sToolTipText  = QT_TR_NOOP("Dock all top-level views");
    sWhatsThis    = "Std_UserInterface";
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

DEF_STD_CMD_AC(StdCmdDockViewMenu)

StdCmdDockViewMenu::StdCmdDockViewMenu()
  : Command("Std_DockViewMenu")
{
    sGroup        = "View";
    sMenuText     = QT_TR_NOOP("Panels");
    sToolTipText  = QT_TR_NOOP("List of available dock panels");
    sWhatsThis    = "Std_DockViewMenu";
    sStatusTip    = QT_TR_NOOP("List of available dock panels");
    eType         = 0;
}

void StdCmdDockViewMenu::activated(int iMsg)
{
    // Handled by the related QAction objects
    Q_UNUSED(iMsg);
}

bool StdCmdDockViewMenu::isActive()
{
    return true;
}

Action * StdCmdDockViewMenu::createAction()
{
    Action *pcAction;
    pcAction = new DockWidgetAction(this, getMainWindow());
    applyCommandData(this->className(), pcAction);
    return pcAction;
}

//===========================================================================
// Std_ToolBarMenu
//===========================================================================

DEF_STD_CMD_AC(StdCmdToolBarMenu)

StdCmdToolBarMenu::StdCmdToolBarMenu()
  : Command("Std_ToolBarMenu")
{
    sGroup        = "View";
    sMenuText     = QT_TR_NOOP("Tool&bars");
    sToolTipText  = QT_TR_NOOP("Toggles this window");
    sWhatsThis    = "Std_ToolBarMenu";
    sStatusTip    = QT_TR_NOOP("Toggles this window");
    eType         = 0;
}

void StdCmdToolBarMenu::activated(int iMsg)
{
    // Handled by the related QAction objects
    Q_UNUSED(iMsg);
}

bool StdCmdToolBarMenu::isActive()
{
    return true;
}

Action * StdCmdToolBarMenu::createAction()
{
    Action *pcAction;
    pcAction = new ToolBarAction(this, getMainWindow());
    applyCommandData(this->className(), pcAction);
    return pcAction;
}

//===========================================================================
// Std_ViewStatusBar
//===========================================================================

class FilterStatusBar : public QObject
{
//    Q_OBJECT

public:
    FilterStatusBar(Action * action):QObject(action) {this->action = action;}
//    virtual ~FilterStatusBar() {}
protected:
    Action * action;
    bool eventFilter(QObject *obj, QEvent *event) override
    {
        if (getMainWindow() && getMainWindow()->findChild<QStatusBar *>() && obj == getMainWindow()->statusBar() &&
            ((event->type() == QEvent::Hide) || (event->type() == QEvent::Show))) {
            this->action->setChecked(getMainWindow()->statusBar()->isVisible());
        }
        return false;
    }
};

DEF_STD_CMD_AC(StdCmdStatusBar)

StdCmdStatusBar::StdCmdStatusBar()
  : Command("Std_ViewStatusBar")
{
    sGroup        = "View";
    sMenuText     = QT_TR_NOOP("Status bar");
    sToolTipText  = QT_TR_NOOP("Toggles the status bar");
    sWhatsThis    = "Std_ViewStatusBar";
    sStatusTip    = QT_TR_NOOP("Toggles the status bar");
    eType         = 0;
}

Action * StdCmdStatusBar::createAction()
{
    Action *pcAction = Command::createAction();
    pcAction->setCheckable(true);
    pcAction->setChecked(false, true);
    auto fsb = new FilterStatusBar(pcAction);
    getMainWindow()->statusBar()->installEventFilter(fsb);

    return pcAction;
}

void StdCmdStatusBar::activated(int iMsg)
{
    getMainWindow()->statusBar()->setVisible(iMsg != 0);
}

bool StdCmdStatusBar::isActive()
{
    static bool checked = false;
    if (!checked) {
        Action* act = this->getAction();
        if (act) {
            act->setChecked(getMainWindow()->statusBar()->isVisible());
            checked = true;
        }
    }
    return true;
}

//===========================================================================
// Std_WindowsMenu
//===========================================================================

DEF_STD_CMD_AC(StdCmdWindowsMenu)

StdCmdWindowsMenu::StdCmdWindowsMenu()
  : Command("Std_WindowsMenu")
{
    sGroup        = "Window";
    sMenuText     = QT_TR_NOOP("Activate window"); // Replaced with the name of the window
    sToolTipText  = QT_TR_NOOP("Activates this window");
    sWhatsThis    = "Std_WindowsMenu";
    sStatusTip    = QT_TR_NOOP("Activates this window");
    eType         = 0;
}

void StdCmdWindowsMenu::activated(int iMsg)
{
    // already handled by the main window
    Q_UNUSED(iMsg);
}

bool StdCmdWindowsMenu::isActive()
{
    return true;
}

Action * StdCmdWindowsMenu::createAction()
{
    // Allow to show 10 menu items in the 'Window' menu and one separator.
    // If we have more windows then the user can use the 'Windows...' item.
    WindowAction *pcAction;
    pcAction = new WindowAction(this, getMainWindow());
    for ( int i=0; i<10; i++ ) {
        QAction* window = pcAction->addAction(QObject::tr(getToolTipText()));
        window->setCheckable(true);
        window->setToolTip(QCoreApplication::translate(
            this->className(), getToolTipText()));
        window->setStatusTip(QCoreApplication::translate(
            this->className(), getStatusTip()));
        window->setWhatsThis(QCoreApplication::translate(
            this->className(), getWhatsThis()));
    }

    QAction* sep = pcAction->addAction(QLatin1String(""));
    sep->setSeparator(true);

    return pcAction;
}

//===========================================================================
// Instantiation
//===========================================================================


namespace Gui {

void CreateWindowStdCommands()
{
    CommandManager &rcCmdMgr = Application::Instance->commandManager();

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
