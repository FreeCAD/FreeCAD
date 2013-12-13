/***************************************************************************
 *   Copyright (c) 2008 Jürgen Riegel (juergen.riegel@web.de)              *
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

#include <Gui/Application.h>
#include <Gui/Command.h>
#include <Gui/Control.h>
#include <Gui/MainWindow.h>
#include <Gui/FileDialog.h>
#include "Tessellation.h"

using namespace std;

//===========================================================================
// MeshPart_Mesher
//===========================================================================
DEF_STD_CMD_A(CmdMeshPartMesher);

CmdMeshPartMesher::CmdMeshPartMesher()
  : Command("MeshPart_Mesher")
{
    sAppModule    = "MeshPart";
    sGroup        = QT_TR_NOOP("Mesh");
    sMenuText     = QT_TR_NOOP("Create mesh from shape...");
    sToolTipText  = QT_TR_NOOP("Tessellate shape");
    sWhatsThis    = sToolTipText;
    sStatusTip    = sToolTipText;
}

void CmdMeshPartMesher::activated(int iMsg)
{
    Gui::Control().showDialog(new MeshPartGui::TaskTessellation());
}

bool CmdMeshPartMesher::isActive(void)
{
    return (hasActiveDocument() && !Gui::Control().activeDialog());
}


void CreateMeshPartCommands(void)
{
    Gui::CommandManager &rcCmdMgr = Gui::Application::Instance->commandManager();
    rcCmdMgr.addCommand(new CmdMeshPartMesher());
}
