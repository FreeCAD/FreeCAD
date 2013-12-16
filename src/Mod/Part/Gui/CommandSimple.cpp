/***************************************************************************
 *   Copyright (c) Jürgen Riegel          (juergen.riegel@web.de) 2002     *
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
# include <QDir>
# include <QFileInfo>
# include <QLineEdit>
# include <QInputDialog>
# include <Standard_math.hxx>
#endif

#include <Base/Exception.h>
#include <App/Document.h>
#include <App/DocumentObject.h>
#include <Gui/Application.h>
#include <Gui/Command.h>
#include <Gui/Document.h>
#include <Gui/MainWindow.h>
#include <Gui/Selection.h>
#include <Gui/WaitCursor.h>

#include "../App/PartFeature.h"
#include "../App/TopoShape.h"
#include "DlgPartCylinderImp.h"


//===========================================================================
// Part_SimpleCylinder
//===========================================================================
DEF_STD_CMD_A(CmdPartSimpleCylinder);

CmdPartSimpleCylinder::CmdPartSimpleCylinder()
  :Command("Part_SimpleCylinder")
{
    sAppModule    = "Part";
    sGroup        = QT_TR_NOOP("Part");
    sMenuText     = QT_TR_NOOP("Create Cylinder...");
    sToolTipText  = QT_TR_NOOP("Create a Cylinder");
    sWhatsThis    = sToolTipText;
    sStatusTip    = sToolTipText;
    sPixmap       = "Part_Cylinder";
}

void CmdPartSimpleCylinder::activated(int iMsg)
{
    PartGui::DlgPartCylinderImp dlg(Gui::getMainWindow());
    if (dlg.exec()== QDialog::Accepted) {
        Base::Vector3d dir = dlg.getDirection();
        openCommand("Create Part Cylinder");
        doCommand(Doc,"from FreeCAD import Base");
        doCommand(Doc,"import Part");
        doCommand(Doc,"App.ActiveDocument.addObject(\"Part::Feature\",\"Cylinder\")"
                      ".Shape=Part.makeCylinder(%f,%f,"
                      "Base.Vector(%f,%f,%f),"
                      "Base.Vector(%f,%f,%f))"
                     ,dlg.radius->value()
                     ,dlg.length->value()
                     ,dlg.xPos->value()
                     ,dlg.yPos->value()
                     ,dlg.zPos->value()
                     ,dir.x,dir.y,dir.z);
        commitCommand();
        updateActive();
        doCommand(Gui, "Gui.SendMsgToActiveView(\"ViewFit\")");
    }
}

bool CmdPartSimpleCylinder::isActive(void)
{
    if (getActiveGuiDocument())
        return true;
    else
        return false;
}


//===========================================================================
// Part_ShapeFromMesh
//===========================================================================
DEF_STD_CMD_A(CmdPartShapeFromMesh);

CmdPartShapeFromMesh::CmdPartShapeFromMesh()
  :Command("Part_ShapeFromMesh")
{
    sAppModule    = "Part";
    sGroup        = QT_TR_NOOP("Part");
    sMenuText     = QT_TR_NOOP("Create shape from mesh...");
    sToolTipText  = QT_TR_NOOP("Create shape from selected mesh object");
    sWhatsThis    = sToolTipText;
    sStatusTip    = sToolTipText;
    sPixmap       = "Part_Shape_from_Mesh.svg";
}

void CmdPartShapeFromMesh::activated(int iMsg)
{
    bool ok;
    double tol = QInputDialog::getDouble(Gui::getMainWindow(), QObject::tr("Sewing Tolerance"),
        QObject::tr("Enter tolerance for sewing shape:"), 0.1, 0.01,10.0,2,&ok);
    if (!ok)
        return;
    Base::Type meshid = Base::Type::fromName("Mesh::Feature");
    std::vector<App::DocumentObject*> meshes;
    meshes = Gui::Selection().getObjectsOfType(meshid);
    Gui::WaitCursor wc;
    std::vector<App::DocumentObject*>::iterator it;
    openCommand("Convert mesh");
    for (it = meshes.begin(); it != meshes.end(); ++it) {
        App::Document* doc = (*it)->getDocument();
        std::string mesh = (*it)->getNameInDocument();
        std::string name = doc->getUniqueObjectName(mesh.c_str());
        doCommand(Doc,"import Part");
        doCommand(Doc,"FreeCAD.getDocument(\"%s\").addObject(\"Part::Feature\",\"%s\")"
                     ,doc->getName()
                     ,name.c_str());
        doCommand(Doc,"__shape__=Part.Shape()");
        doCommand(Doc,"__shape__.makeShapeFromMesh("
                      "FreeCAD.getDocument(\"%s\").getObject(\"%s\").Mesh.Topology,%f"
                      ")"
                     ,doc->getName()
                     ,mesh.c_str()
                     ,tol);
        doCommand(Doc,"FreeCAD.getDocument(\"%s\").getObject(\"%s\").Shape=__shape__"
                     ,doc->getName()
                     ,name.c_str());
        doCommand(Doc,"FreeCAD.getDocument(\"%s\").getObject(\"%s\").purgeTouched()"
                     ,doc->getName()
                     ,name.c_str());
        doCommand(Doc,"del __shape__");
    }

    commitCommand();
}

bool CmdPartShapeFromMesh::isActive(void)
{
    Base::Type meshid = Base::Type::fromName("Mesh::Feature");
    return Gui::Selection().countObjectsOfType(meshid) > 0;
}

//===========================================================================
// Part_SimpleCopy
//===========================================================================
DEF_STD_CMD_A(CmdPartSimpleCopy);

CmdPartSimpleCopy::CmdPartSimpleCopy()
  : Command("Part_SimpleCopy")
{
    sAppModule    = "Part";
    sGroup        = QT_TR_NOOP("Part");
    sMenuText     = QT_TR_NOOP("Create simple copy");
    sToolTipText  = QT_TR_NOOP("Create a simple non-parametric copy");
    sWhatsThis    = sToolTipText;
    sStatusTip    = sToolTipText;
}

void CmdPartSimpleCopy::activated(int iMsg)
{
    Base::Type partid = Base::Type::fromName("Part::Feature");
    std::vector<App::DocumentObject*> objs = Gui::Selection().getObjectsOfType(partid);
    openCommand("Create Copy");
    for (std::vector<App::DocumentObject*>::iterator it = objs.begin(); it != objs.end(); ++it) {
        doCommand(Doc,"App.ActiveDocument.addObject('Part::Feature','%s').Shape="
                      "App.ActiveDocument.%s.Shape\n"
                      "App.ActiveDocument.ActiveObject.Label="
                      "App.ActiveDocument.%s.Label\n",
                      (*it)->getNameInDocument(),
                      (*it)->getNameInDocument(),
                      (*it)->getNameInDocument());
        copyVisual("ActiveObject", "ShapeColor", (*it)->getNameInDocument());
        copyVisual("ActiveObject", "LineColor", (*it)->getNameInDocument());
        copyVisual("ActiveObject", "PointColor", (*it)->getNameInDocument());
    }
    commitCommand();
    updateActive();
}

bool CmdPartSimpleCopy::isActive(void)
{
    Base::Type partid = Base::Type::fromName("Part::Feature");
    return Gui::Selection().countObjectsOfType(partid) > 0;
}

//===========================================================================
// Part_RefineShape
//===========================================================================
DEF_STD_CMD_A(CmdPartRefineShape);

CmdPartRefineShape::CmdPartRefineShape()
  : Command("Part_RefineShape")
{
    sAppModule    = "Part";
    sGroup        = QT_TR_NOOP("Part");
    sMenuText     = QT_TR_NOOP("Refine shape");
    sToolTipText  = QT_TR_NOOP("Refine the copy of a shape");
    sWhatsThis    = sToolTipText;
    sStatusTip    = sToolTipText;
}

void CmdPartRefineShape::activated(int iMsg)
{
    Gui::WaitCursor wc;
    Base::Type partid = Base::Type::fromName("Part::Feature");
    std::vector<App::DocumentObject*> objs = Gui::Selection().getObjectsOfType(partid);
    openCommand("Refine shape");
    for (std::vector<App::DocumentObject*>::iterator it = objs.begin(); it != objs.end(); ++it) {
        try {
            doCommand(Doc,"App.ActiveDocument.addObject('Part::Feature','%s').Shape="
                          "App.ActiveDocument.%s.Shape.removeSplitter()\n"
                          "App.ActiveDocument.ActiveObject.Label="
                          "App.ActiveDocument.%s.Label\n"
                          "Gui.ActiveDocument.%s.hide()\n",
                          (*it)->getNameInDocument(),
                          (*it)->getNameInDocument(),
                          (*it)->getNameInDocument(),
                          (*it)->getNameInDocument());
            copyVisual("ActiveObject", "ShapeColor", (*it)->getNameInDocument());
            copyVisual("ActiveObject", "LineColor", (*it)->getNameInDocument());
            copyVisual("ActiveObject", "PointColor", (*it)->getNameInDocument());
        }
        catch (const Base::Exception& e) {
            Base::Console().Warning("%s: %s\n", (*it)->Label.getValue(), e.what());
        }
    }
    commitCommand();
    updateActive();
}

bool CmdPartRefineShape::isActive(void)
{
    Base::Type partid = Base::Type::fromName("Part::Feature");
    return Gui::Selection().countObjectsOfType(partid) > 0;
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

void CreateSimplePartCommands(void)
{
    Gui::CommandManager &rcCmdMgr = Gui::Application::Instance->commandManager();
    rcCmdMgr.addCommand(new CmdPartSimpleCylinder());
    rcCmdMgr.addCommand(new CmdPartShapeFromMesh());
    rcCmdMgr.addCommand(new CmdPartSimpleCopy());
    rcCmdMgr.addCommand(new CmdPartRefineShape());
}
