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
    sWhatsThis    = "Part_SimpleCylinder";
    sStatusTip    = sToolTipText;
    sPixmap       = "Part_Cylinder";
}

void CmdPartSimpleCylinder::activated(int iMsg)
{
    Q_UNUSED(iMsg);
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
                     ,dlg.radius->value().getValue()
                     ,dlg.length->value().getValue()
                     ,dlg.xPos->value().getValue()
                     ,dlg.yPos->value().getValue()
                     ,dlg.zPos->value().getValue()
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
    sWhatsThis    = "Part_ShapeFromMesh";
    sStatusTip    = sToolTipText;
    sPixmap       = "Part_Shape_from_Mesh";
}

void CmdPartShapeFromMesh::activated(int iMsg)
{
    Q_UNUSED(iMsg);

    double STD_OCC_TOLERANCE = 1e-6;

    ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/Units");
    int decimals = hGrp->GetInt("Decimals");
    double tolerance_from_decimals = pow(10., -decimals);

    double minimal_tolerance = tolerance_from_decimals < STD_OCC_TOLERANCE ? STD_OCC_TOLERANCE : tolerance_from_decimals;

    bool ok;
    double tol = QInputDialog::getDouble(Gui::getMainWindow(), QObject::tr("Sewing Tolerance"),
        QObject::tr("Enter tolerance for sewing shape:"), 0.1, minimal_tolerance, 10.0, decimals, &ok);
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
    sWhatsThis    = "Part_SimpleCopy";
    sStatusTip    = sToolTipText;
    sPixmap       = "Tree_Part";
}

static void _copyShape(const char *cmdName, bool resolve,bool needElement=false, bool refine=false) {
    Gui::WaitCursor wc;
    Gui::Command::openCommand(cmdName);
    for(auto &sel : Gui::Selection().getSelectionEx("*",App::DocumentObject::getClassTypeId(),resolve)) {
        std::map<std::string,App::DocumentObject*> subMap;
        auto obj = sel.getObject();
        if(!obj) continue;
        if(resolve || !sel.hasSubNames())
            subMap.emplace("",obj);
        else {
            for(const auto &sub : sel.getSubNames()) {
                const char *element = 0;
                auto sobj = obj->resolve(sub.c_str(),0,0,&element);
                if(!sobj) continue;
                if(!needElement && element) 
                    subMap.emplace(sub.substr(0,element-sub.c_str()),sobj);
                else
                    subMap.emplace(sub,sobj);
            }
            if(subMap.empty())
                continue;
        }
        auto parentName = Gui::Command::getObjectCmd(obj);
        for(auto &v : subMap) {
            Gui::Command::doCommand(Gui::Command::Doc,
                    "__shape = Part.getShape(%s,'%s',needSubElement=%s,refine=%s)%s\n"
                    "App.ActiveDocument.addObject('Part::Feature','%s').Shape=__shape\n"
                    "App.ActiveDocument.ActiveObject.Label=%s.Label\n",
                        parentName.c_str(), v.first.c_str(),
                        needElement?"True":"False", refine?"True":"False",
                        needElement?".copy()":"", 
                        v.second->getNameInDocument(), 
                        Gui::Command::getObjectCmd(v.second).c_str());
            auto newObj = App::GetApplication().getActiveDocument()->getActiveObject();
            Gui::Command::copyVisual(newObj, "ShapeColor", v.second);
            Gui::Command::copyVisual(newObj, "LineColor", v.second);
            Gui::Command::copyVisual(newObj, "PointColor", v.second);
            FCMD_VOBJ_CMD(newObj,"mapShapeColors(App.getDocument('" 
                    << obj->getDocument()->getName() << "'))");
            if(obj->getDocument()!=App::GetApplication().getActiveDocument()) {
                // clear element map if copied from external object, or else
                // weird stuff will happen
                Gui::Command::doCommand(Gui::Command::Doc,
                        "__shape.ElementMap = {}\n"
                        "__shape.Tag = 0\n"
                        "%s.Shape = __shape\n",
                        Gui::Command::getObjectCmd(newObj).c_str());
            }
            Gui::Command::doCommand(Gui::Command::Doc,"del(__shape)");
        }
    }
    Gui::Command::commitCommand();
    Gui::Command::updateActive();
}

void CmdPartSimpleCopy::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    _copyShape("Simple copy",true);
}

bool CmdPartSimpleCopy::isActive(void)
{
    return Gui::Selection().hasSelection();
}

//===========================================================================
// Part_TransformedCopy
//===========================================================================
DEF_STD_CMD_A(CmdPartTransformedCopy);

CmdPartTransformedCopy::CmdPartTransformedCopy()
  : Command("Part_TransformedCopy")
{
    sAppModule    = "Part";
    sGroup        = QT_TR_NOOP("Part");
    sMenuText     = QT_TR_NOOP("Create transformed copy");
    sToolTipText  = QT_TR_NOOP("Create a non-parametric copy with transformed placement");
    sWhatsThis    = "Part_TransformCopy";
    sStatusTip    = sToolTipText;
    sPixmap       = "Part_Transformed_Copy.svg";
}

void CmdPartTransformedCopy::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    _copyShape("Transformed copy",false);
}

bool CmdPartTransformedCopy::isActive(void)
{
    return Gui::Selection().hasSelection();
}

//===========================================================================
// Part_ElementCopy
//===========================================================================
DEF_STD_CMD_A(CmdPartElementCopy);

CmdPartElementCopy::CmdPartElementCopy()
  : Command("Part_ElementCopy")
{
    sAppModule    = "Part";
    sGroup        = QT_TR_NOOP("Part");
    sMenuText     = QT_TR_NOOP("Create shape element copy");
    sToolTipText  = QT_TR_NOOP("Create a non-parametric copy of the selected shape element");
    sWhatsThis    = "Part_ElementCopy";
    sStatusTip    = sToolTipText;
    sPixmap       = "Part_Element_Copy.svg";
}

void CmdPartElementCopy::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    _copyShape("Element copy",false,true);
}

bool CmdPartElementCopy::isActive(void)
{
    return Gui::Selection().hasSelection();
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
    sWhatsThis    = "Part_RefineShape";
    sStatusTip    = sToolTipText;
    sPixmap       = "Part_Refine_Shape";
}

void CmdPartRefineShape::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    _copyShape("Refined copy",true,false,true);
}

bool CmdPartRefineShape::isActive(void)
{
    return Gui::Selection().hasSelection();
}

//===========================================================================
// Part_Defeaturing
//===========================================================================
DEF_STD_CMD_A(CmdPartDefeaturing);

CmdPartDefeaturing::CmdPartDefeaturing()
  : Command("Part_Defeaturing")
{
    sAppModule    = "Part";
    sGroup        = QT_TR_NOOP("Part");
    sMenuText     = QT_TR_NOOP("Defeaturing");
    sToolTipText  = QT_TR_NOOP("Remove feature from a shape");
    sWhatsThis    = "Part_Defeaturing";
    sStatusTip    = sToolTipText;
    sPixmap       = "Part_Defeaturing";
}

void CmdPartDefeaturing::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    Gui::WaitCursor wc;
    Base::Type partid = Base::Type::fromName("Part::Feature");
    std::vector<Gui::SelectionObject> objs = Gui::Selection().getSelectionEx(0, partid);
    openCommand("Defeaturing");
    for (std::vector<Gui::SelectionObject>::iterator it = objs.begin(); it != objs.end(); ++it) {
        try {
            std::string shape;
            shape.append("sh=App.");
            shape.append(it->getDocName());
            shape.append(".");
            shape.append(it->getFeatName());
            shape.append(".Shape\n");

            std::string faces;
            std::vector<std::string> subnames = it->getSubNames();
            for (std::vector<std::string>::iterator sub = subnames.begin(); sub != subnames.end(); ++sub) {
                faces.append("sh.");
                faces.append(*sub);
                faces.append(",");
            }

            doCommand(Doc,"\nsh = App.getDocument('%s').%s.Shape\n"
                          "nsh = sh.defeaturing([%s])\n"
                          "if not sh.isPartner(nsh):\n"
                          "\t\tdefeat = App.ActiveDocument.addObject('Part::Feature','Defeatured').Shape = nsh\n"
                          "\t\tGui.ActiveDocument.%s.hide()\n"
                          "else:\n"
                          "\t\tFreeCAD.Console.PrintError('Defeaturing failed\\n')",
                          it->getDocName(),
                          it->getFeatName(),
                          faces.c_str(),
                          it->getFeatName());
        }
        catch (const Base::Exception& e) {
            Base::Console().Warning("%s: %s\n", it->getFeatName(), e.what());
        }
    }
    commitCommand();
    updateActive();
}

bool CmdPartDefeaturing::isActive(void)
{
    Base::Type partid = Base::Type::fromName("Part::Feature");
    std::vector<Gui::SelectionObject> objs = Gui::Selection().getSelectionEx(0, partid);
    for (std::vector<Gui::SelectionObject>::iterator it = objs.begin(); it != objs.end(); ++it) {
        std::vector<std::string> subnames = it->getSubNames();
        for (std::vector<std::string>::iterator sub = subnames.begin(); sub != subnames.end(); ++sub) {
            if (sub->substr(0,4) == "Face") {
                return true;
            }
        }
    }
    return false;
}


// {
//     if (getActiveGuiDocument())
// #if OCC_VERSION_HEX < 0x060900
//         return false;
// #else
//         return true;
// #endif
//     else
//         return false;
// }


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

void CreateSimplePartCommands(void)
{
    Gui::CommandManager &rcCmdMgr = Gui::Application::Instance->commandManager();
    rcCmdMgr.addCommand(new CmdPartSimpleCylinder());
    rcCmdMgr.addCommand(new CmdPartShapeFromMesh());
    rcCmdMgr.addCommand(new CmdPartSimpleCopy());
    rcCmdMgr.addCommand(new CmdPartElementCopy());
    rcCmdMgr.addCommand(new CmdPartTransformedCopy());
    rcCmdMgr.addCommand(new CmdPartRefineShape());
    rcCmdMgr.addCommand(new CmdPartDefeaturing());
}
