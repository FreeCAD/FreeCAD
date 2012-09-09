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
# include <QMessageBox>
#endif

#include <App/Application.h>
#include <App/Document.h>
#include <App/DocumentObject.h>
#include <Gui/Application.h>
#include <Gui/Control.h>
#include <Gui/Command.h>
#include <Gui/MainWindow.h>
#include <Gui/FileDialog.h>
#include <Gui/Selection.h>
#include <Gui/Document.h>

#include <Mod/Fem/App/FemMeshObject.h>

#include "Hypothesis.h"

using namespace std;

DEF_STD_CMD_A(CmdFemCreateFromShape);

CmdFemCreateFromShape::CmdFemCreateFromShape()
  : Command("Fem_CreateFromShape")
{
    sAppModule      = "Fem";
    sGroup          = QT_TR_NOOP("Fem");
    sMenuText       = QT_TR_NOOP("Create FEM mesh");
    sToolTipText    = QT_TR_NOOP("Create FEM mesh from shape");
    sWhatsThis      = sToolTipText;
    sStatusTip      = sToolTipText;
    sPixmap         = "Fem_FemMesh";
}

void CmdFemCreateFromShape::activated(int iMsg)
{
    FemGui::TaskHypothesis* dlg = new FemGui::TaskHypothesis();
    Gui::Control().showDialog(dlg);
}

bool CmdFemCreateFromShape::isActive(void)
{
    if (Gui::Control().activeDialog())
        return false;
    Base::Type type = Base::Type::fromName("Part::Feature");
    return Gui::Selection().countObjectsOfType(type) > 0;
}


void CreateFemCommands(void)
{
    Gui::CommandManager &rcCmdMgr = Gui::Application::Instance->commandManager();
    rcCmdMgr.addCommand(new CmdFemCreateFromShape());
}
