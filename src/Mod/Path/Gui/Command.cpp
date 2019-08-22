/***************************************************************************
 *   Copyright (c) Yorik van Havre (yorik@uncreated.net) 2014              *
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
# include <TopExp_Explorer.hxx>
#endif

#include <Base/Console.h>
#include <App/Application.h>
#include <Gui/Application.h>
#include <Gui/MainWindow.h>
#include <Gui/Command.h>
#include <Gui/Selection.h>
#include <Gui/SelectionFilter.h>
#include <Gui/Document.h>
#include <Gui/Control.h>

#include <Mod/Path/App/FeaturePath.h>
#include <Mod/Path/App/FeaturePathCompound.h>
#include <Mod/Path/App/FeaturePathShape.h>
#include <Mod/Part/App/PartFeature.h>
#include <Mod/Path/App/FeatureArea.h>


// Path Area  #####################################################################################################


DEF_STD_CMD_A(CmdPathArea)

CmdPathArea::CmdPathArea()
    :Command("Path_Area")
{
    sAppModule      = "Path";
    sGroup          = QT_TR_NOOP("Path");
    sMenuText       = QT_TR_NOOP("Area");
    sToolTipText    = QT_TR_NOOP("Creates a feature area from selected objects");
    sWhatsThis      = "Path_Area";
    sStatusTip      = sToolTipText;
    sPixmap         = "Path-Area";
}

void CmdPathArea::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    std::list<std::string> cmds;
    std::ostringstream sources;
    std::string areaName;
    bool addView = true;
    for(const Gui::SelectionObject &selObj :
        getSelection().getSelectionEx(NULL, Part::Feature::getClassTypeId()))
    {
        const Part::Feature *pcObj = static_cast<const Part::Feature*>(selObj.getObject());
        const std::vector<std::string> &subnames = selObj.getSubNames();
        if(addView && areaName.size()) addView = false;

        if(subnames.empty()) {
            if(addView && pcObj->getTypeId().isDerivedFrom(Path::FeatureArea::getClassTypeId()))
                areaName = pcObj->getNameInDocument();
            sources << "FreeCAD.activeDocument()." << pcObj->getNameInDocument() << ",";
            continue;
        }
        for(const std::string &name : subnames) {
            if(name.compare(0,4,"Face") && name.compare(0,4,"Edge")) {
                Base::Console().Error("Selected shape is not 2D\n");
                return;
            }

            std::ostringstream subname;
            subname << pcObj->getNameInDocument() << '_' << name;
            std::string sub_fname = getUniqueObjectName(subname.str().c_str());

            std::ostringstream cmd;
            cmd << "FreeCAD.activeDocument().addObject('Part::Feature','" << sub_fname <<
                "').Shape = PathCommands.findShape(FreeCAD.activeDocument()." <<
                pcObj->getNameInDocument() << ".Shape,'" << name << "'";
            if(!name.compare(0,4,"Edge"))
                cmd << ",'Wires'";
            cmd << ')';
            cmds.push_back(cmd.str());
            sources << "FreeCAD.activeDocument()." << sub_fname << ",";
        }
    }
    if(addView && areaName.size()) {
        std::string FeatName = getUniqueObjectName("FeatureAreaView");
        openCommand("Create Path Area View");
        doCommand(Doc,"FreeCAD.activeDocument().addObject('Path::FeatureAreaView','%s')",FeatName.c_str());
        doCommand(Doc,"FreeCAD.activeDocument().%s.Source = FreeCAD.activeDocument().%s",
                FeatName.c_str(),areaName.c_str());
        commitCommand();
        updateActive();
        return;
    }
    std::string FeatName = getUniqueObjectName("FeatureArea");
    openCommand("Create Path Area");
    doCommand(Doc,"import PathCommands");
    for(const std::string &cmd : cmds)
        doCommand(Doc,cmd.c_str());
    doCommand(Doc,"FreeCAD.activeDocument().addObject('Path::FeatureArea','%s')",FeatName.c_str());
    doCommand(Doc,"FreeCAD.activeDocument().%s.Sources = [ %s ]",FeatName.c_str(),sources.str().c_str());
    commitCommand();
    updateActive();
}

bool CmdPathArea::isActive(void)
{
    return hasActiveDocument();
}


DEF_STD_CMD_A(CmdPathAreaWorkplane)

CmdPathAreaWorkplane::CmdPathAreaWorkplane()
    :Command("Path_Area_Workplane")
{
    sAppModule      = "Path";
    sGroup          = QT_TR_NOOP("Path");
    sMenuText       = QT_TR_NOOP("Area workplane");
    sToolTipText    = QT_TR_NOOP("Select a workplane for a FeatureArea");
    sWhatsThis      = "Path_Area_Workplane";
    sStatusTip      = sToolTipText;
    sPixmap         = "Path-Area-Workplane";
}

void CmdPathAreaWorkplane::activated(int iMsg)
{
    Q_UNUSED(iMsg);

    std::string areaName;
    std::string planeSubname;
    std::string planeName;

    for(Gui::SelectionObject &selObj :
        getSelection().getSelectionEx(NULL, Part::Feature::getClassTypeId()))
    {
        const std::vector<std::string> &subnames = selObj.getSubNames();
        if(subnames.size()>1) {
            Base::Console().Error("Please select one sub shape object for plane only\n");
            return;
        }
        const Part::Feature *pcObj = static_cast<Part::Feature*>(selObj.getObject());
        if(subnames.empty()) {
            if(pcObj->getTypeId().isDerivedFrom(Path::FeatureArea::getClassTypeId()))  {
                if(areaName.size()){
                    Base::Console().Error("Please select one FeatureArea only\n");
                    return;
                }
                areaName = pcObj->getNameInDocument();
                continue;
            }
            for (TopExp_Explorer it(pcObj->Shape.getShape().getShape(), TopAbs_SHELL); it.More(); it.Next()) {
                Base::Console().Error("Selected shape is not 2D\n");
                return;
            }
        }
        if(planeName.size()){
            Base::Console().Error("Please select one shape object for plane only\n");
            return;
        }else{
            planeSubname = planeName = pcObj->getNameInDocument();
            planeSubname += ".Shape";
        }

        for(const std::string &name : subnames) {
            if(name.compare(0,4,"Face") && name.compare(0,4,"Edge")) {
                Base::Console().Error("Selected shape is not 2D\n");
                return;
            }
            std::ostringstream subname;
            subname << planeSubname << ",'" << name << "','Wires'";
            planeSubname = subname.str();
        }
    }
    if(areaName.empty()) {
        Base::Console().Error("Please select one FeatureArea\n");
        return;
    }
    if(planeName.empty()) {
        Base::Console().Error("Please select one shape object\n");
        return;
    }

    openCommand("Select Workplane for Path Area");
    doCommand(Doc,"import PathCommands");
    doCommand(Doc,"FreeCAD.activeDocument().%s.WorkPlane = PathCommands.findShape("
            "FreeCAD.activeDocument().%s)", areaName.c_str(),planeSubname.c_str());
    doCommand(Doc,"FreeCAD.activeDocument().%s.ViewObject.Visibility = True",areaName.c_str());
    commitCommand();
    updateActive();
}

bool CmdPathAreaWorkplane::isActive(void)
{
    return !getSelection().getSelectionEx(NULL, Path::FeatureArea::getClassTypeId()).empty();
}


// Path compound #####################################################################################################


DEF_STD_CMD_A(CmdPathCompound)

CmdPathCompound::CmdPathCompound()
    :Command("Path_Compound")
{
    sAppModule      = "Path";
    sGroup          = QT_TR_NOOP("Path");
    sMenuText       = QT_TR_NOOP("Compound");
    sToolTipText    = QT_TR_NOOP("Creates a compound from selected paths");
    sWhatsThis      = "Path_Compound";
    sStatusTip      = sToolTipText;
    sPixmap         = "Path-Compound";
}

void CmdPathCompound::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    std::vector<Gui::SelectionSingleton::SelObj> Sel = getSelection().getSelection();
    if (Sel.size() > 0) {
        std::ostringstream cmd;
        cmd << "[";
        Path::Feature *pcPathObject;
        for (std::vector<Gui::SelectionSingleton::SelObj>::const_iterator it=Sel.begin();it!=Sel.end();++it) {
            if ((*it).pObject->getTypeId().isDerivedFrom(Path::Feature::getClassTypeId())) {
                pcPathObject = static_cast<Path::Feature*>((*it).pObject);
                cmd << "FreeCAD.activeDocument()." << pcPathObject->getNameInDocument() << ",";
            } else {
                Base::Console().Error("Only Path objects must be selected before running this command\n");
                return;
            }
        }
        cmd << "]";
        std::string FeatName = getUniqueObjectName("PathCompound");
        openCommand("Create Path Compound");
        doCommand(Doc,"FreeCAD.activeDocument().addObject('Path::FeatureCompound','%s')",FeatName.c_str());
        doCommand(Doc,"FreeCAD.activeDocument().%s.Group = %s",FeatName.c_str(),cmd.str().c_str());
        commitCommand();
        updateActive();
    } else {
        Base::Console().Error("At least one Path object must be selected\n");
        return;
    }
}

bool CmdPathCompound::isActive(void)
{
    return hasActiveDocument();
}

 // Path Shape #####################################################################################################


DEF_STD_CMD_A(CmdPathShape)

CmdPathShape::CmdPathShape()
    :Command("Path_Shape")
{
    sAppModule      = "Path";
    sGroup          = QT_TR_NOOP("Path");
    sMenuText       = QT_TR_NOOP("From Shape");
    sToolTipText    = QT_TR_NOOP("Creates a path from a selected shape");
    sWhatsThis      = "Path_Shape";
    sStatusTip      = sToolTipText;
    sPixmap         = "Path-Shape";
}

void CmdPathShape::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    std::list<std::string> cmds;
    std::ostringstream sources;
    for(const Gui::SelectionObject &selObj :
        getSelection().getSelectionEx(NULL, Part::Feature::getClassTypeId()))
    {
        const Part::Feature *pcObj = static_cast<const Part::Feature*>(selObj.getObject());
        const std::vector<std::string> &subnames = selObj.getSubNames();
        if(subnames.empty()) {
            sources << "FreeCAD.activeDocument()." << pcObj->getNameInDocument() << ",";
            continue;
        }
        for(const std::string &name : subnames) {
            if(name.compare(0,4,"Face") && name.compare(0,4,"Edge")) {
                Base::Console().Warning("Ignored shape %s %s\n",
                        pcObj->getNameInDocument(), name.c_str());
                continue;
            }

            std::ostringstream subname;
            subname << pcObj->getNameInDocument() << '_' << name;
            std::string sub_fname = getUniqueObjectName(subname.str().c_str());

            std::ostringstream cmd;
            cmd << "FreeCAD.activeDocument().addObject('Part::Feature','" << sub_fname <<
                "').Shape = PathCommands.findShape(FreeCAD.activeDocument()." <<
                pcObj->getNameInDocument() << ".Shape,'" << name << "'";
            if(!name.compare(0,4,"Edge"))
                cmd << ",'Wires'";
            cmd << ')';
            cmds.push_back(cmd.str());
            sources << "FreeCAD.activeDocument()." << sub_fname << ",";
        }
    }
    std::string FeatName = getUniqueObjectName("PathShape");
    openCommand("Create Path Shape");
    doCommand(Doc,"import PathCommands");
    for(const std::string &cmd : cmds)
        doCommand(Doc,cmd.c_str());
    doCommand(Doc,"FreeCAD.activeDocument().addObject('Path::FeatureShape','%s')",FeatName.c_str());
    doCommand(Doc,"FreeCAD.activeDocument().%s.Sources = [ %s ]",FeatName.c_str(),sources.str().c_str());
    commitCommand();
    updateActive();
}

bool CmdPathShape::isActive(void)
{
    return hasActiveDocument();
}



void CreatePathCommands(void)
{
    Gui::CommandManager &rcCmdMgr = Gui::Application::Instance->commandManager();
    rcCmdMgr.addCommand(new CmdPathCompound());
    rcCmdMgr.addCommand(new CmdPathShape());
    rcCmdMgr.addCommand(new CmdPathArea());
    rcCmdMgr.addCommand(new CmdPathAreaWorkplane());
}
