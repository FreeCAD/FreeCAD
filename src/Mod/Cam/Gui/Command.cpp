/***************************************************************************
 *   Copyright (c) 2007 Werner Mayer <wmayer@users.sourceforge.net>        *
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

#include "Cutting.h"

#include <Gui/Command.h>
#include <Gui/Application.h>
#include <Gui/MainWindow.h>
#include <QPointer>




//===========================================================================
// StdCamCutting
//===========================================================================
DEF_STD_CMD(StdCamCutting);

StdCamCutting::StdCamCutting()
        :Command("Cam_Cutting")
{
    sGroup        = QT_TR_NOOP("Tools");
    sMenuText     = QT_TR_NOOP("Cutting...");
    sToolTipText  = QT_TR_NOOP("Cutting");
    sWhatsThis    = QT_TR_NOOP("Cutting");
    sStatusTip    = QT_TR_NOOP("Cutting");
}

void StdCamCutting::activated(int iMsg)
{
    static QPointer<QDialog> dlg = 0;
    if (!dlg)
        dlg = new CamGui::Cutting(Gui::getMainWindow());
    dlg->setAttribute(Qt::WA_DeleteOnClose);
    dlg->show();
}

void CreateCamCommands()
{
    Gui::CommandManager &rcCmdMgr = Gui::Application::Instance->commandManager();
    rcCmdMgr.addCommand(new StdCamCutting());
}
