/***************************************************************************
 *   Copyright (c) 2002 Jürgen Riegel <juergen.riegel@web.de>              *
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
# include <Standard_math.hxx>
# include <QInputDialog>
#endif

#include <App/Document.h>
#include <App/DocumentObject.h>
#include <Base/Exception.h>
#include <Base/Interpreter.h>
#include <Gui/Application.h>
#include <Gui/Command.h>
#include <Gui/MainWindow.h>
#include <Gui/Selection.h>
#include <Gui/SelectionObject.h>
#include <Gui/WaitCursor.h>

#include "DlgPartCylinderImp.h"
#include "ShapeFromMesh.h"
#include <Mod/Part/App/PartFeature.h>


//===========================================================================
// Part_SimpleCylinder
//===========================================================================
DEF_STD_CMD_A(CmdPartSimpleCylinder)

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
        Base::Vector3d pos = dlg.getPosition();
        openCommand(QT_TRANSLATE_NOOP("Command", "Create Part Cylinder"));
        doCommand(Doc,"from FreeCAD import Base");
        doCommand(Doc,"import Part");
        doCommand(Doc,"App.ActiveDocument.addObject(\"Part::Feature\",\"Cylinder\")"
                      ".Shape=Part.makeCylinder(%f,%f,"
                      "Base.Vector(%f,%f,%f),"
                      "Base.Vector(%f,%f,%f))"
                     ,dlg.getRadius()
                     ,dlg.getLength()
                     ,pos.x,pos.y,pos.z
                     ,dir.x,dir.y,dir.z);
        commitCommand();
        updateActive();
        doCommand(Gui, "Gui.SendMsgToActiveView(\"ViewFit\")");
    }
}

bool CmdPartSimpleCylinder::isActive()
{
    if (getActiveGuiDocument())
        return true;
    else
        return false;
}


//===========================================================================
// Part_ShapeFromMesh
//===========================================================================
DEF_STD_CMD_A(CmdPartShapeFromMesh)

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
    PartGui::ShapeFromMesh dlg(Gui::getMainWindow());
    dlg.exec();
}

bool CmdPartShapeFromMesh::isActive()
{
    Base::Type meshid = Base::Type::fromName("Mesh::Feature");
    return Gui::Selection().countObjectsOfType(meshid) > 0;
}
//===========================================================================
// Part_PointsFromMesh
//===========================================================================
DEF_STD_CMD_A(CmdPartPointsFromMesh)

CmdPartPointsFromMesh::CmdPartPointsFromMesh()
  :Command("Part_PointsFromMesh")
{
    sAppModule    = "Part";
    sGroup        = QT_TR_NOOP("Part");
    sMenuText     = QT_TR_NOOP("Create points object from geometry");
    sToolTipText  = QT_TR_NOOP("Create selectable points object from selected geometric object");
    sWhatsThis    = "Part_PointsFromMesh";
    sStatusTip    = sToolTipText;
    sPixmap       = "Part_PointsFromMesh";
}

void CmdPartPointsFromMesh::activated(int iMsg)
{
    Q_UNUSED(iMsg);

    auto getDefaultDistance = [](Part::Feature* geometry) {
        auto bbox = geometry->Shape.getBoundingBox();
        int steps{20};
        return bbox.CalcDiagonalLength() / steps;
    };

    Base::Type geoid = Base::Type::fromName("App::GeoFeature");
    std::vector<App::DocumentObject*> geoms;
    geoms = Gui::Selection().getObjectsOfType(geoid);

    double distance{1.0};
    auto found = std::find_if(geoms.begin(), geoms.end(), [](App::DocumentObject* obj) {
        return Base::freecad_dynamic_cast<Part::Feature>(obj);
    });

    if (found != geoms.end()) {

        double defaultDistance = getDefaultDistance(Base::freecad_dynamic_cast<Part::Feature>(*found));

        double STD_OCC_TOLERANCE = 1e-6;

        int decimals = Base::UnitsApi::getDecimals();
        double tolerance_from_decimals = pow(10., -decimals);

        double minimal_tolerance = tolerance_from_decimals < STD_OCC_TOLERANCE ? STD_OCC_TOLERANCE : tolerance_from_decimals;

        bool ok;
        distance = QInputDialog::getDouble(Gui::getMainWindow(), QObject::tr("Distance in parameter space"),
            QObject::tr("Enter distance:"), defaultDistance, minimal_tolerance, 10.0 * defaultDistance, decimals, &ok, Qt::MSWindowsFixedSizeDialogHint);
        if (!ok) {
            return;
        }
    }

    Gui::WaitCursor wc;
    openCommand(QT_TRANSLATE_NOOP("Command", "Points from geometry"));

    Base::PyGILStateLocker lock;
    try {
        PyObject* module = PyImport_ImportModule("BasicShapes.Utils");
        if (!module) {
            throw Py::Exception();
        }
        Py::Module utils(module, true);

        for (auto it : geoms) {
            Py::Tuple args(2);
            args.setItem(0, Py::asObject(it->getPyObject()));
            args.setItem(1, Py::Float(distance));
            utils.callMemberFunction("showCompoundFromPoints", args);
        }
    }
    catch (Py::Exception&) {
        Base::PyException e;
        e.ReportException();
    }

    commitCommand();
}

bool CmdPartPointsFromMesh::isActive()
{
    Base::Type meshid = Base::Type::fromName("App::GeoFeature");
    return Gui::Selection().countObjectsOfType(meshid) > 0;
}

//===========================================================================
// Part_SimpleCopy
//===========================================================================
DEF_STD_CMD_A(CmdPartSimpleCopy)

CmdPartSimpleCopy::CmdPartSimpleCopy()
  : Command("Part_SimpleCopy")
{
    sAppModule    = "Part";
    sGroup        = QT_TR_NOOP("Part");
    sMenuText     = QT_TR_NOOP("Create simple copy");
    sToolTipText  = QT_TR_NOOP("Create a simple non-parametric copy");
    sWhatsThis    = "Part_SimpleCopy";
    sStatusTip    = sToolTipText;
    sPixmap       = "Part_3D_object";
}

static void _copyShape(const char *cmdName, bool resolve,bool needElement=false, bool refine=false) {
    Gui::WaitCursor wc;
    Gui::Command::openCommand(cmdName);
    for(auto &sel : Gui::Selection().getSelectionEx("*", App::DocumentObject::getClassTypeId(),
                                                    resolve ? Gui::ResolveMode::OldStyleElement : Gui::ResolveMode::NoResolve)) {
        std::map<std::string,App::DocumentObject*> subMap;
        auto obj = sel.getObject();
        if (!obj)
            continue;
        if (resolve || !sel.hasSubNames()) {
            subMap.emplace("",obj);
        }
        else {
            for(const auto &sub : sel.getSubNames()) {
                const char *element = nullptr;
                auto sobj = obj->resolve(sub.c_str(),nullptr,nullptr,&element);
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
                        needElement ? "True" : "False",
                        refine ? "True" : "False",
                        needElement ? ".copy()" : "",
                        v.second->getNameInDocument(),
                        Gui::Command::getObjectCmd(v.second).c_str());
            auto newObj = App::GetApplication().getActiveDocument()->getActiveObject();
            Gui::Command::copyVisual(newObj, "ShapeColor", v.second);
            Gui::Command::copyVisual(newObj, "LineColor", v.second);
            Gui::Command::copyVisual(newObj, "PointColor", v.second);
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

bool CmdPartSimpleCopy::isActive()
{
    return Gui::Selection().hasSelection();
}

//===========================================================================
// Part_TransformedCopy
//===========================================================================
DEF_STD_CMD_A(CmdPartTransformedCopy)

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

bool CmdPartTransformedCopy::isActive()
{
    return Gui::Selection().hasSelection();
}

//===========================================================================
// Part_ElementCopy
//===========================================================================
DEF_STD_CMD_A(CmdPartElementCopy)

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

bool CmdPartElementCopy::isActive()
{
    return Gui::Selection().hasSelection();
}

//===========================================================================
// Part_RefineShape
//===========================================================================
DEF_STD_CMD_A(CmdPartRefineShape)

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
    ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/Mod/Part");
    bool parametric = hGrp->GetBool("ParametricRefine", true);
    if (parametric) {
        Gui::WaitCursor wc;
        Base::Type partid = Base::Type::fromName("Part::Feature");
        std::vector<App::DocumentObject*> objs = Gui::Selection().getObjectsOfType(partid);
        openCommand(QT_TRANSLATE_NOOP("Command", "Refine shape"));
        std::for_each(objs.begin(), objs.end(), [](App::DocumentObject* obj) {
            try {
                doCommand(Doc,"App.ActiveDocument.addObject('Part::Refine','%s').Source="
                              "App.ActiveDocument.%s\n"
                              "App.ActiveDocument.ActiveObject.Label="
                              "App.ActiveDocument.%s.Label\n"
                              "Gui.ActiveDocument.%s.hide()\n",
                              obj->getNameInDocument(),
                              obj->getNameInDocument(),
                              obj->getNameInDocument(),
                              obj->getNameInDocument());

                copyVisual("ActiveObject", "ShapeColor", obj->getNameInDocument());
                copyVisual("ActiveObject", "LineColor", obj->getNameInDocument());
                copyVisual("ActiveObject", "PointColor", obj->getNameInDocument());
            }
            catch (const Base::Exception& e) {
                Base::Console().Warning("%s: %s\n", obj->Label.getValue(), e.what());
            }
        });
        commitCommand();
        updateActive();
    }
    else {
        _copyShape("Refined copy",true,false,true);
    }
}

bool CmdPartRefineShape::isActive()
{
    return Gui::Selection().hasSelection();
}

//===========================================================================
// Part_Defeaturing
//===========================================================================
DEF_STD_CMD_A(CmdPartDefeaturing)

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
    std::vector<Gui::SelectionObject> objs = Gui::Selection().getSelectionEx(nullptr, partid);
    openCommand(QT_TRANSLATE_NOOP("Command", "Defeaturing"));
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
            for (const auto & subname : subnames) {
                faces.append("sh.");
                faces.append(subname);
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

bool CmdPartDefeaturing::isActive()
{
    Base::Type partid = Base::Type::fromName("Part::Feature");
    std::vector<Gui::SelectionObject> objs = Gui::Selection().getSelectionEx(nullptr, partid);
    for (const auto & obj : objs) {
        std::vector<std::string> subnames = obj.getSubNames();
        for (const auto & subname : subnames) {
            if (subname.substr(0,4) == "Face") {
                return true;
            }
        }
    }
    return false;
}


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

void CreateSimplePartCommands()
{
    Gui::CommandManager &rcCmdMgr = Gui::Application::Instance->commandManager();
    rcCmdMgr.addCommand(new CmdPartSimpleCylinder());
    rcCmdMgr.addCommand(new CmdPartShapeFromMesh());
    rcCmdMgr.addCommand(new CmdPartPointsFromMesh());
    rcCmdMgr.addCommand(new CmdPartSimpleCopy());
    rcCmdMgr.addCommand(new CmdPartElementCopy());
    rcCmdMgr.addCommand(new CmdPartTransformedCopy());
    rcCmdMgr.addCommand(new CmdPartRefineShape());
    rcCmdMgr.addCommand(new CmdPartDefeaturing());
}
