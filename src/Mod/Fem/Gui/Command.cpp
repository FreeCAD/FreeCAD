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
#include <Gui/Command.h>
#include <Gui/MainWindow.h>
#include <Gui/FileDialog.h>
#include <Gui/Selection.h>
#include <Gui/Document.h>

#include <Mod/Fem/App/FemMeshObject.h>


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
    Base::Type type = Base::Type::fromName("Part::Feature");
    std::vector<App::DocumentObject*> obj = Gui::Selection().getObjectsOfType(type);

    openCommand("Create FEM");
    doCommand(Doc, "import Fem");
    for (std::vector<App::DocumentObject*>::iterator it = obj.begin(); it != obj.end(); ++it) {
        App::Document* doc = (*it)->getDocument();
        QString name = QString::fromAscii((*it)->getNameInDocument());
        QString cmd = QString::fromAscii(
            "__fem__=Fem.FemMesh()\n"
            "__fem__.setShape(FreeCAD.getDocument(\"%1\").%2.Shape)\n"
            "h1=Fem.StdMeshers_MaxLength(0,__fem__)\n"
            "h1.setLength(1.0)\n"
            "h2=Fem.StdMeshers_LocalLength(1,__fem__)\n"
            "h2.setLength(1.0)\n"
            "h3=Fem.StdMeshers_QuadranglePreference(2,__fem__)\n"
            "h4=Fem.StdMeshers_Quadrangle_2D(3,__fem__)\n"
            "h5=Fem.StdMeshers_MaxElementArea(4,__fem__)\n"
            "h5.setMaxArea(1.0)\n"
            "h6=Fem.StdMeshers_Regular_1D(5,__fem__)\n"
            "__fem__.addHypothesis(h1)\n"
            "__fem__.addHypothesis(h2)\n"
            "__fem__.addHypothesis(h3)\n"
            "__fem__.addHypothesis(h4)\n"
            "__fem__.addHypothesis(h5)\n"
            "__fem__.addHypothesis(h6)\n"
            "__fem__.compute()\n"
            "FreeCAD.getDocument(\"%1\").addObject"
            "(\"Fem::FemMeshObject\",\"%2\").FemMesh=__fem__\n"
            "del __fem__,h1,h2,h3,h4,h5,h6\n"
        )
        .arg(QString::fromAscii(doc->getName()))
        .arg(name);
        doCommand(Doc, "%s", (const char*)cmd.toAscii());
    }
    commitCommand();
}

bool CmdFemCreateFromShape::isActive(void)
{
    Base::Type type = Base::Type::fromName("Part::Feature");
    return Gui::Selection().countObjectsOfType(type) > 0;
}


void CreateFemCommands(void)
{
    Gui::CommandManager &rcCmdMgr = Gui::Application::Instance->commandManager();
    rcCmdMgr.addCommand(new CmdFemCreateFromShape());
}
