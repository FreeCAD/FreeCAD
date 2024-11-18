/***************************************************************************
 *   Copyright (c) YEAR YOUR NAME <Your e-mail address>                    *
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
#endif

#include <App/Document.h>
#include <Base/Console.h>
#include <Gui/Application.h>
#include <Gui/Command.h>


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

//===========================================================================
// Cmd_TEMPLATE_Test THIS IS JUST A TEST COMMAND
//===========================================================================
DEF_STD_CMD(Cmd_TEMPLATE_Test)

Cmd_TEMPLATE_Test::Cmd_TEMPLATE_Test()
    : Command("_TEMPLATE__Test")
{
    sAppModule = "_TEMPLATE_";
    sGroup = QT_TR_NOOP("_TEMPLATE_");
    sMenuText = QT_TR_NOOP("Hello");
    sToolTipText = QT_TR_NOOP("_TEMPLATE_ Test function");
    sWhatsThis = "_TEMPLATE__Test";
    sStatusTip = QT_TR_NOOP("_TEMPLATE_ Test function");
    sPixmap = "_TEMPLATE_Workbench";
    sAccel = "CTRL+H";
}

void Cmd_TEMPLATE_Test::activated(int)
{
    Base::Console().Message("Hello, World!\n");
}

void Create_TEMPLATE_Commands(void)
{
    Gui::CommandManager& rcCmdMgr = Gui::Application::Instance->commandManager();
    rcCmdMgr.addCommand(new Cmd_TEMPLATE_Test());
}
